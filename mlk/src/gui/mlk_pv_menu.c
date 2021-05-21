/*$
 Copyright (C) 2013-2021 Azel.

 This file is part of AzPainter.

 AzPainter is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AzPainter is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
$*/

/*****************************************
 * mMenu 内部処理
 *****************************************/

#include "mlk_gui.h"
#include "mlk_menu.h"
#include "mlk_font.h"
#include "mlk_accelerator.h"
#include "mlk_str.h"

#include "mlk_pv_menu.h"


//-----------------

#define _MENUITEM_SEP_H   5	//セパレータのアイテム高さ
#define _MENUITEM_SPACE_V 3	//テキストアイテムの垂直余白

//-----------------


//==========================
// mMenuItem
//==========================


/** ラベルテキストのセット時 */

void __mMenuItemInitLabel(mMenuItem *p)
{
	const char *pc;
	char c;

	p->hotkey = 0;

	if(!p->info.text) return;

	//文字列の複製
	
	if(p->info.flags & MMENUITEM_F_COPYTEXT)
		p->info.text = mStrdup(p->info.text);

	//文字列からホットキー文字取得 (0-9,A-Z,a-z)

	for(pc = p->info.text; *pc; pc++)
	{
		if(*pc == '&')
		{
			c = *(++pc);

			if(c == '&')
				continue;
			else if((c >= '0' && c <= '9')
					|| (c >= 'A' && c <= 'Z')
					|| (c >= 'a' && c <= 'z'))
			{
				if(c >= 'a' && c <= 'z')
					c -= 0x20;
			
				p->hotkey = c;
				break;
			}
		}
	}
}

/** ショートカットキーセット時 */

void __mMenuItemInitSckey(mMenuItem *p)
{
	//解放

	mFree(p->sckeytxt);
	p->sckeytxt = NULL;

	//テキストセット

	if(p->info.shortcut_key)
	{
		mStr str = MSTR_INIT;

		mAcceleratorGetKeyText(&str, p->info.shortcut_key);

		p->sckeytxt = mStrdup(str.buf);

		mStrFree(&str);
	}
}

/** 次のアイテム取得 (root 以下のすべての階層が対象)
 *
 * p:  NULL で、root の先頭から開始
 * root: このメニュー以下で検索 */

mMenuItem *__mMenuItemGetNext(mMenuItem *p,mMenu *root)
{
	if(!p)
	{
		//root の先頭から

		return MLK_MENUITEM(root->list.top);
	}
	else if(p->info.submenu
		&& (p->info.submenu)->list.top)
	{
		//サブメニューにアイテムがある場合

		return MLK_MENUITEM((p->info.submenu)->list.top);
	}
	else if(p->i.next)
		//次のアイテムがある場合
		return MLK_MENUITEM(p->i.next);
	else
	{
		//終端アイテムの場合。
		//親に戻って次の位置へ

		mMenu *menu = p->parent;

		while(menu != root)
		{
			p = menu->parent;
			menu = p->parent;

			p = MLK_MENUITEM(p->i.next);
			if(p) return p;
		}

		return NULL;
	}
}

/** アイテムにサブメニューセット */

void __mMenuItemSetSubmenu(mMenuItem *p,mMenu *submenu)
{
	p->info.submenu = submenu;

	submenu->parent = p;
}

/** 選択可能なアイテムか
 *
 * return: 無効、もしくはセパレータ時は FALSE */

mlkbool __mMenuItemIsSelectable(mMenuItem *p)
{
	return !(p->info.flags & (MMENUITEM_F_DISABLE | MMENUITEM_F_SEP));
}

/** アイテムにサブメニューがあり、かつ表示可能な状態か */

mlkbool __mMenuItemIsShowableSubmenu(mMenuItem *p)
{
	return (p->info.submenu
			&& !(p->info.flags & MMENUITEM_F_DISABLE)
			&& (p->info.submenu)->list.top);
}

/** ラジオグループの情報取得
 *
 * pptop: NULL 以外で、先頭位置が入る
 * ppend: NULL 以外で、終端位置が入る
 * return: 現在の選択アイテム */

mMenuItem *__mMenuItemGetRadioInfo(mMenuItem *p,mMenuItem **pptop,mMenuItem **ppend)
{
	mMenuItem *sel = NULL,*p2;
	
	//先頭検索

	while(1)
	{
		p2 = MLK_MENUITEM(p->i.prev);
		if(!p2 || (p2->info.flags & MMENUITEM_F_SEP)) break;

		p = p2;
	}

	if(pptop) *pptop = p;

	//先頭位置から、終端と選択を検索

	while(1)
	{
		if(p->info.flags & MMENUITEM_F_CHECKED)
			sel = p;
	
		p2 = MLK_MENUITEM(p->i.next);
		if(!p2 || (p2->info.flags & MMENUITEM_F_SEP)) break;

		p = p2;
	}

	if(ppend) *ppend = p;

	return sel;
}

/** アイテムのラジオチェックを ON */

void __mMenuItemSetCheckRadio(mMenuItem *p)
{
	mMenuItem *sel;

	//現在の選択を OFF

	sel = __mMenuItemGetRadioInfo(p, NULL, NULL);
	if(sel) sel->info.flags &= ~MMENUITEM_F_CHECKED;

	//ON

	p->info.flags |= MMENUITEM_F_CHECKED;
}


//==========================
// mMenu
//==========================


/** ホットキー押し時のアイテム検索
 *
 * mMenuBar での Alt+? 押し時と、メニュー表示中のキー押し時。
 * アイテムが無効状態の場合は NULL。
 *
 * key: 大文字 */

mMenuItem *__mMenuFindItem_hotkey(mMenu *menu,char key)
{
	mMenuItem *p;
	
	for(p = MLK_MENUITEM(menu->list.top); p; p = MLK_MENUITEM(p->i.next))
	{
		if(p->hotkey == key)
			return __mMenuItemIsSelectable(p)? p: NULL;
	}
	
	return NULL;
}

/** 全てのアイテムのテキストサイズの初期化 */

void __mMenuInitTextSize(mMenu *menu,mFont *font)
{
	mMenuItem *p;
	int fh;

	if(!menu) return;

	fh = mFontGetHeight(font);
	
	for(p = MLK_MENUITEM(menu->list.top); p; p = MLK_MENUITEM(p->i.next))
	{
		//計算済み

		if(p->text_w >= 0) continue;
	
		//表示サイズを計算

		p->text_w = 0;
		p->sckey_w = 0;
		
		if(p->info.flags & MMENUITEM_F_SEP)
		{
			//セパレータ
			
			p->text_w = 1;
			p->info.height = _MENUITEM_SEP_H;
		}
		else if(!p->info.draw && p->info.text)
		{
			//------- 通常テキスト
					
			//テキスト幅
			
			p->text_w  = mFontGetTextWidth_hotkey(font, p->info.text, -1);
			p->sckey_w = mFontGetTextWidth(font, p->sckeytxt, -1);
			
			//高さ
			
			p->info.height = fh;

			if(p->info.height < 9)
				p->info.height = 9;
			
			p->info.height += _MENUITEM_SPACE_V * 2;
		}
	}
}

/** 選択可能な一番先頭のアイテム取得 */

mMenuItem *__mMenuGetSelectableTopItem(mMenu *menu)
{
	mMenuItem *pi;

	for(pi = MLK_MENUITEM(menu->list.top); pi; pi = MLK_MENUITEM(pi->i.next))
	{
		if(__mMenuItemIsSelectable(pi))
			return pi;
	}

	return NULL;
}

/** 選択可能な一番終端のアイテム取得 */

mMenuItem *__mMenuGetSelectableBottomItem(mMenu *menu)
{
	mMenuItem *pi;

	for(pi = MLK_MENUITEM(menu->list.bottom); pi; pi = MLK_MENUITEM(pi->i.prev))
	{
		if(__mMenuItemIsSelectable(pi))
			return pi;
	}

	return NULL;
}

/** 選択可能な一つ前のアイテム取得 (キーでの上下移動時) */

mMenuItem *__mMenuGetPrevSelectableItem(mMenu *p,mMenuItem *item)
{
	mlkbool flag = FALSE;

	do
	{
		if(item)
			item = MLK_MENUITEM(item->i.prev);

		if(!item)
		{
			item = MLK_MENUITEM(p->list.bottom);

			//終端に飛ぶのは一度だけ
			if(!item || flag) return NULL;
			flag = TRUE;
		}

	} while(!__mMenuItemIsSelectable(item));

	return item;
}

/** 選択可能な一つ次のアイテム取得 */

mMenuItem *__mMenuGetNextSelectableItem(mMenu *p,mMenuItem *item)
{
	mlkbool flag = FALSE;

	do
	{
		if(item)
			item = MLK_MENUITEM(item->i.next);

		if(!item)
		{
			item = MLK_MENUITEM(p->list.top);

			if(!item || flag) return NULL;
			flag = TRUE;
		}

	} while(!__mMenuItemIsSelectable(item));

	return item;
}
