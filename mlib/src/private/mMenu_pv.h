/*$
 Copyright (C) 2013-2020 Azel.

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

#ifndef MLIB_MENU_PV_H
#define MLIB_MENU_PV_H

#include "mListDef.h"

#define M_MENUITEM(p)  ((mMenuItem *)(p))

struct _mMenu
{
	mList list;
};

typedef struct _mMenuItem
{
	mListItem i;
	mMenuItemInfo item;

	char *labelcopy,*sctext;
	const char *labelsrc;
	
	short labelLeftW,
		labelRightW;
	char hotkey;
	uint8_t fTmp;
}mMenuItem;

#define MMENUITEM_TMPF_INIT_SIZE 1

#define MMENU_INITTEXT_LABEL    1
#define MMENU_INITTEXT_SHORTCUT 2
#define MMENU_INITTEXT_ALL      3

/*---------*/

#ifdef __cplusplus
extern "C" {
#endif

mBool __mMenuItemIsDisableItem(mMenuItem *p);
mBool __mMenuItemIsEnableSubmenu(mMenuItem *p);
void __mMenuItemCheckRadio(mMenuItem *p,int check);

void __mMenuInitText(mMenuItem *p,int flags);
void __mMenuInit(mMenu *p,mFont *font);
mMenuItem *__mMenuFindByHotkey(mMenu *menu,char key);

#ifdef __cplusplus
}
#endif

#endif
