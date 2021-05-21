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
 * mMenu [メニュー]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_menu.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_util.h"
#include "mlk_widget_def.h"

#include "mlk_pv_menu.h"
#include "mlk_pv_window.h"



//============================
// sub
//============================


/* アイテム削除ハンドラ */

static void _item_destroy(mList *list,mListItem *pi)
{
	mMenuItem *p = MLK_MENUITEM(pi);
	
	//ショートカットキー文字列

	mFree(p->sckeytxt);

	//ラベル文字列 (複製時)

	if(p->info.flags & MMENUITEM_F_COPYTEXT)
		mFree((void *)p->info.text);

	//サブメニュー削除

	mMenuDestroy(p->info.submenu);
}

/* インデックス位置からアイテムポインタ取得 */

static mMenuItem *_get_item_index(mMenu *menu,int index)
{
	return (mMenuItem *)mListGetItemAtIndex(&menu->list, index);
}

/* ID からアイテム検索 (サブメニュー以下も含む) */

static mMenuItem *_find_item_id(mMenu *menu,int id)
{
	mMenuItem *pi = NULL;

	while(1)
	{
		pi = __mMenuItemGetNext(pi, menu);
		if(!pi) break;

		if(pi->info.id == id)
			return pi;
	}

	return NULL;
}

/* 16bit 配列からアイテム追加
 *
 * radio: TRUE でラジオタイプ有効 */

static void _append_item_array16(mMenu *top,const uint16_t *buf,mlkbool radio)
{
	mMenu *current,*parent;
	mMenuItem *item;
	int val,id;
	uint32_t flags;

	current = top;
	parent = NULL;

	while(1)
	{
		val = *(buf++);
		if(val == MMENU_ARRAY16_END) break;

		//

		if(val == MMENU_ARRAY16_SUB_START)
		{
			//---- サブメニュー開始 (最後のアイテムにセット)

			item = MLK_MENUITEM(current->list.bottom);
			if(item)
			{
				parent = current;
				current = mMenuNew();

				__mMenuItemSetSubmenu(item, current);
			}
		}
		else if(val == MMENU_ARRAY16_SUB_END)
		{
			//---- サブメニュー終了

			if(parent)
			{
				current = parent;

				if(current->parent)
					parent = current->parent->parent;
				else
					parent = NULL;
			}
		}
		else
		{
			//---- 項目

			if(val == MMENU_ARRAY16_SEP)
				//セパレータ
				mMenuAppendSep(current);
			else
			{
				flags = 0;

				if(radio)
					id = val & 0x3fff;
				else
					id = val & 0x7fff;

				if(val & MMENU_ARRAY16_CHECK)
					flags |= MMENUITEM_F_CHECK_TYPE;
				else if(radio && (val & MMENU_ARRAY16_RADIO))
					flags |= MMENUITEM_F_RADIO_TYPE;
			
				mMenuAppend(current, id, MLK_TR(id), 0, flags);
			}
		}
	}
}


//========================
// mMenuItem
//========================


/**@ 次のアイテム取得
 *
 * @g:mMenuItem */

mMenuItem *mMenuItemGetNext(mMenuItem *item)
{
	return MLK_MENUITEM(item->i.next);
}

/**@ mMenuItem から、アイテム ID 取得 */

int mMenuItemGetID(mMenuItem *item)
{
	return item->info.id;
}

/**@ mMenuItem から、テキストのポインタ取得 */

const char *mMenuItemGetText(mMenuItem *item)
{
	return item->info.text;
}

/**@ mMenuItem から、param1 取得 */

intptr_t mMenuItemGetParam1(mMenuItem *item)
{
	return item->info.param1;
}

/**@ mMenuItem から mMenuItemInfo 取得 */

mMenuItemInfo *mMenuItemGetInfo(mMenuItem *item)
{
	return &item->info;
}


//****************************
// mMenu
//****************************


/**@ メニュー削除
 *
 * @d:下位のサブメニューもすべて削除される。 */

void mMenuDestroy(mMenu *p)
{
	if(p)
	{
		mListDeleteAll(&p->list);
		
		mFree(p);
	}
}

/**@ メニュー作成
 *
 * @g:mMenu */

mMenu *mMenuNew(void)
{
	mMenu *p;

	p = (mMenu *)mMalloc0(sizeof(mMenu));
	if(p)
		p->list.item_destroy = _item_destroy;

	return p;
}


//========================
// ポップアップ


/**@ ポップアップ表示
 *
 * @d:メニューを破棄した後に、戻り値の mMenuItem を参照しないよう注意。
 *
 * @p:parent  配置の親
 * @p:x,y     表示位置 (parent からの相対位置)。\
 *  box = NULL の場合は、位置として使われる。\
 *  box != NULL の場合は、box の位置に加算される。
 * @p:box 表示位置の基準矩形。NULL で x,y を使う。
 * @p:flags ポップアップの表示位置などのフラグ (MPOPUP_F_*)。\
 *  MPOPUP_F_MENU_SEND_COMMAND が ON の場合、notify ウィジェットに対して、COMMAND イベントが送られる。
 * @p:notify  MEVENT_MENU_POPUP/MEVENT_COMMAND イベントの通知先。NULL でなし。
 * @r:選択されたアイテム。NULL で、項目が選択されずにキャンセルされた。 */

mMenuItem *mMenuPopup(mMenu *p,mWidget *parent,int x,int y,mBox *box,uint32_t flags,mWidget *notify)
{
	if(p->list.num == 0) return NULL;

	return __mMenuWindowRunPopup(parent, x, y, box, flags, p, notify);
}


//====================
// 情報取得


/**@ アイテム個数取得 */

int mMenuGetNum(mMenu *p)
{
	return p->list.num;
}

/**@ メニューの先頭アイテムを取得 */

mMenuItem *mMenuGetTopItem(mMenu *p)
{
	return MLK_MENUITEM(p->list.top);
}

/**@ メニューの終端アイテムを取得 */

mMenuItem *mMenuGetBottomItem(mMenu *p)
{
	return MLK_MENUITEM(p->list.bottom);
}

/**@ インデックス位置からアイテム取得 */

mMenuItem *mMenuGetItemAtIndex(mMenu *p,int index)
{
	return (mMenuItem *)mListGetItemAtIndex(&p->list, index);
}


//===========================
// 削除


/**@ 項目をすべて削除
 * 
 * @d:下位のサブメニューも含む。 */

void mMenuDeleteAll(mMenu *p)
{
	mListDeleteAll(&p->list);
}

/**@ ID からアイテム削除
 *
 * @r:削除したか */

mlkbool mMenuDeleteItem_id(mMenu *p,int id)
{
	mMenuItem *pi;
	
	pi = _find_item_id(p, id);
	if(!pi)
		return FALSE;
	else
	{
		mListDelete(&(pi->parent)->list, MLISTITEM(p));
		return TRUE;
	}
}

/**@ インデックス位置からアイテム削除
 *
 * @r:削除したか */

mlkbool mMenuDeleteItem_index(mMenu *p,int index)
{
	return mListDelete_index(&p->list, index);
}


//========================
// アイテム追加
//========================


/**@ アイテム追加 */

mMenuItemInfo *mMenuAppend(mMenu *p,int id,const char *text,uint32_t sckey,uint32_t flags)
{
	mMenuItemInfo info;
	
	mMemset0(&info, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.text = text;
	info.shortcut_key = sckey;
	info.flags = flags;
	
	return mMenuAppendInfo(p, &info);
}

/**@ アイテム追加 (詳細指定)
 *
 * @r:追加されたアイテムの mMenuItemInfo が返る。
 *  このポインタを使って、直接値を変更することが可能。 */

mMenuItemInfo *mMenuAppendInfo(mMenu *p,mMenuItemInfo *info)
{
	mMenuItem *pi;

	pi = (mMenuItem *)mListAppendNew(&p->list, sizeof(mMenuItem));
	if(!pi) return NULL;

	pi->info = *info;
	pi->parent = p;
	pi->text_w = -1;	//表示時にサイズを計算させる

	if(info->submenu)
		info->submenu->parent = pi;

	__mMenuItemInitLabel(pi);
	__mMenuItemInitSckey(pi);

	return &pi->info;
}

/**@ アイテム追加 (テキストのみ:静的文字列) */

mMenuItemInfo *mMenuAppendText(mMenu *p,int id,const char *text)
{
	return mMenuAppend(p, id, text, 0, 0);
}

/**@ アイテム追加 (テキストのみ:複製文字列)
 *
 * @p:len テキスト長さ。負の値で自動。 */

mMenuItemInfo *mMenuAppendText_copy(mMenu *p,int id,const char *text,int len)
{
	mMenuItemInfo info,*ret;
	char *tmp;

	tmp = mStrndup(text, len);
	
	mMemset0(&info, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.text = tmp;
	info.flags = MMENUITEM_F_COPYTEXT;
	
	ret = mMenuAppendInfo(p, &info);

	mFree(tmp);

	return ret;
}

/**@ アイテム追加 (パラメータ値 param1 指定) */

mMenuItemInfo *mMenuAppendText_param(mMenu *p,int id,const char *text,intptr_t param)
{
	mMenuItemInfo info;
	
	mMemset0(&info, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.text = text;
	info.param1 = param;
	
	return mMenuAppendInfo(p, &info);
}

/**@ アイテム追加 (サブメニュー付き) */

mMenuItemInfo *mMenuAppendSubmenu(mMenu *p,int id,const char *text,mMenu *submenu,uint32_t flags)
{
	mMenuItemInfo info;
	
	mMemset0(&info, sizeof(mMenuItemInfo));
	
	info.id = id;
	info.text = text;
	info.submenu = submenu;
	info.flags = flags;
	
	return mMenuAppendInfo(p, &info);
}

/**@ アイテム追加 (セパレータ)
 *
 * @d:id は -1 にセットされる。 */

void mMenuAppendSep(mMenu *p)
{
	mMenuAppend(p, -1, NULL, 0, MMENUITEM_F_SEP);
}

/**@ アイテム追加 (チェックタイプ)
 *
 * @p:checked 0 以外で、チェック状態にする */

mMenuItemInfo *mMenuAppendCheck(mMenu *p,int id,const char *text,int checked)
{
	return mMenuAppend(p, id, text, 0,
		MMENUITEM_F_CHECK_TYPE | (checked? MMENUITEM_F_CHECKED: 0));
}

/**@ アイテム追加 (ラジオタイプ) */

mMenuItemInfo *mMenuAppendRadio(mMenu *p,int id,const char *text)
{
	return mMenuAppend(p, id, text, 0, MMENUITEM_F_RADIO_TYPE);
}


//===========================
// 追加 (翻訳文字列から)


/**@ 翻訳文字列のテキストからアイテム追加
 *
 * @d:翻訳文字列はカレントグループから取得される。
 *
 * @p:idtop  翻訳文字列 ID の開始位置。\
 *  翻訳文字列の ID と、アイテム ID は同じになる。
 * @p:num アイテムの追加数 */

void mMenuAppendTrText(mMenu *p,int idtop,int num)
{
	int i;

	for(i = 0; i < num; i++)
		mMenuAppendText(p, idtop + i, MLK_TR(idtop + i));
}

/**@ 翻訳文字列 ID の配列データ (16bit) からアイテムを追加
 *
 * @d:アイテムの id は、文字列 ID と同じになる。\
 * ラジオタイプはなし。
 *
 * @p:buf 各アイテムの文字列 ID とフラグをセットした配列データ。 */

void mMenuAppendTrText_array16(mMenu *p,const uint16_t *buf)
{
	_append_item_array16(p, buf, FALSE);
}

/**@ 翻訳文字列 ID の配列データ (16bit) からアイテムを追加・ラジオタイプあり */

void mMenuAppendTrText_array16_radio(mMenu *p,const uint16_t *buf)
{
	_append_item_array16(p, buf, TRUE);
}


//========================
// 追加 (mStr から)


/**@ mStr 配列からアイテムを追加
 *
 * @p:str アイテムテキストの mStr の配列。空文字列以降はセットされない。
 * @p:idtop アイテム ID の開始位置
 * @p:num 追加するアイテム数 */

void mMenuAppendStrArray(mMenu *p,mStr *str,int idtop,int num)
{
	int i;

	for(i = 0; i < num; i++)
	{
		if(mStrIsEmpty(str + i)) break;

		mMenuAppendText_copy(p, idtop + i, str[i].buf, -1);
	}
}


//==========================
// アイテム関連
//==========================


/**@ アイテムの有効/無効状態を変更
 *
 * @p:type [0] 無効 [正] 有効 [負] 反転 */

void mMenuSetItemEnable(mMenu *p,int id,int type)
{
	mMenuItem *pi;
	
	pi = _find_item_id(p, id);

	if(pi && mIsChangeFlagState(type, !(pi->info.flags & MMENUITEM_F_DISABLE)))
		pi->info.flags ^= MMENUITEM_F_DISABLE;
}

/**@ アイテムのチェック状態を変更 (サブメニュー以下すべて対象)
 * 
 * @p:type [0] OFF [正] ON [負] 反転 (ラジオの場合はON) */

void mMenuSetItemCheck(mMenu *p,int id,int type)
{
	mMenuItem *pi,*sel;
	
	pi = _find_item_id(p, id);
	if(!pi) return;
	
	//ラジオの場合、対象以外が ON の状態で、対象を ON にする時は、
	//現在の選択を OFF にする。
	
	if(pi->info.flags & MMENUITEM_F_RADIO_TYPE)
	{
		type = (type != 0); //反転は 1 に
		
		if(type)
		{
			sel = __mMenuItemGetRadioInfo(pi, NULL, NULL);
			if(sel && pi != sel)
				sel->info.flags &= ~MMENUITEM_F_CHECKED;
		}
	}

	//変更

	if(mIsChangeFlagState(type, pi->info.flags & MMENUITEM_F_CHECKED))
		pi->info.flags ^= MMENUITEM_F_CHECKED;
}

/**@ アイテムにサブメニューをセット */

void mMenuSetItemSubmenu(mMenu *p,int id,mMenu *submenu)
{
	mMenuItem *pi;
	
	pi = _find_item_id(p, id);
	if(pi)
		__mMenuItemSetSubmenu(pi, submenu);
}

/**@ アイテムのショートカットキーをセット
 *
 * @d:キーが同じ場合は、何もしない。 */

void mMenuSetItemShortcutKey(mMenu *p,int id,uint32_t sckey)
{
	mMenuItem *pi;

	pi = _find_item_id(p, id);
	if(!pi) return;

	if(sckey != pi->info.shortcut_key)
	{
		pi->info.shortcut_key = sckey;
		pi->text_w = -1;

		__mMenuItemInitSckey(pi);
	}
}



/**@ アイテムのサブメニュー取得
 *
 * @d:指定 ID のアイテムにセットされているサブメニューを取得する。 */

mMenu *mMenuGetItemSubmenu(mMenu *p,int id)
{
	mMenuItem *pi = _find_item_id(p, id);

	return (pi)? pi->info.submenu: NULL;
}

/**@ 指定インデックス位置のアイテムのテキストを取得 */

const char *mMenuGetItemText_atIndex(mMenu *p,int index)
{
	mMenuItem *pi = _get_item_index(p, index);

	return (pi)? pi->info.text: NULL;
}

