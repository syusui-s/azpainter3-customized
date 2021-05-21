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

#ifndef MLK_PV_MENU_H
#define MLK_PV_MENU_H

#define MLK_MENUITEM(p)  ((mMenuItem *)(p))

/** mMenu 実体 */

struct _mMenu
{
	mList list;
	mMenuItem *parent;	//サブメニューの場合、親のアイテム
};

/** mMenuItem 実体 */

struct _mMenuItem
{
	mListItem i;

	mMenuItemInfo info;	//基本情報

	mMenu *parent;		//親の mMenu
	char *sckeytxt;		//ショートカットキーの文字列 ("Ctrl+A" など)
	int16_t text_w,		//ラベルテキストの幅 (-1 でサイズ計算)
		sckey_w;		//ショートカットキーテキストの幅
	char hotkey;		//&+* のホットキー文字 (0 でなし。0-9,A-Z のみ)
};


/* mMenuItem */

void __mMenuItemInitLabel(mMenuItem *p);
void __mMenuItemInitSckey(mMenuItem *p);
mMenuItem *__mMenuItemGetNext(mMenuItem *p,mMenu *root);
void __mMenuItemSetSubmenu(mMenuItem *p,mMenu *submenu);
mlkbool __mMenuItemIsSelectable(mMenuItem *p);
mlkbool __mMenuItemIsShowableSubmenu(mMenuItem *p);
mMenuItem *__mMenuItemGetRadioInfo(mMenuItem *p,mMenuItem **pptop,mMenuItem **ppend);
void __mMenuItemSetCheckRadio(mMenuItem *p);

/* mMenu */

mMenuItem *__mMenuFindItem_hotkey(mMenu *menu,char key);
void __mMenuInitTextSize(mMenu *p,mFont *font);
mMenuItem *__mMenuGetSelectableTopItem(mMenu *menu);
mMenuItem *__mMenuGetSelectableBottomItem(mMenu *menu);
mMenuItem *__mMenuGetPrevSelectableItem(mMenu *p,mMenuItem *item);
mMenuItem *__mMenuGetNextSelectableItem(mMenu *p,mMenuItem *item);

#endif
