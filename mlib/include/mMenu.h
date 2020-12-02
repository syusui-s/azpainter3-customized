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

#ifndef MLIB_MENU_H
#define MLIB_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mMenu mMenu;
typedef struct _mMenuItemInfo mMenuItemInfo;
typedef struct _mMenuItem mMenuItem;

typedef struct _mMenuItemDraw
{
	mBox box;
	uint32_t flags;
}mMenuItemDraw;

struct _mMenuItemInfo
{
	int id;
	short width,height;
	uint32_t flags,shortcutkey;
	intptr_t param1,param2;
	const char *label;
	mMenu *submenu;
	void (*draw)(mPixbuf *,mMenuItemInfo *,mMenuItemDraw *);
};

/*---------*/

enum MMENUITEM_FLAGS
{
	MMENUITEM_F_LABEL_COPY = 1<<0,
	MMENUITEM_F_SEP        = 1<<1,
	MMENUITEM_F_DISABLE    = 1<<2,
	MMENUITEM_F_RADIO      = 1<<3,
	MMENUITEM_F_CHECKED    = 1<<4,
	MMENUITEM_F_AUTOCHECK  = 1<<5
};

enum MMENUITEM_DRAW_FLAGS
{
	MMENUITEM_DRAW_F_SELECTED = 1
};

enum MMENU_POPUP_FLAGS
{
	MMENU_POPUP_F_RIGHT  = 1<<0,
	MMENU_POPUP_F_NOCOMMAND = 1<<1
};

/*---------*/

mMenu *mMenuNew(void);
void mMenuDestroy(mMenu *p);

int mMenuGetNum(mMenu *menu);
mMenu *mMenuGetSubMenu(mMenu *menu,int id);
mMenuItemInfo *mMenuGetLastItemInfo(mMenu *menu);
mMenuItemInfo *mMenuGetItemByIndex(mMenu *menu,int index);

mMenuItemInfo *mMenuGetInfoInItem(mMenuItem *item);
mMenuItem *mMenuGetTopItem(mMenu *menu);
mMenuItem *mMenuGetNextItem(mMenuItem *item);

mMenuItemInfo *mMenuPopup(mMenu *menu,mWidget *notify,int x,int y,uint32_t flags);

void mMenuDeleteAll(mMenu *menu);
void mMenuDeleteByID(mMenu *menu,int id);
void mMenuDeleteByIndex(mMenu *menu,int index);

mMenuItemInfo *mMenuAdd(mMenu *menu,mMenuItemInfo *info);
mMenuItemInfo *mMenuAddNormal(mMenu *menu,int id,const char *label,uint32_t sckey,uint32_t flags);
mMenuItemInfo *mMenuAddText_static(mMenu *menu,int id,const char *label);
mMenuItemInfo *mMenuAddText_copy(mMenu *menu,int id,const char *label);
void mMenuAddSubmenu(mMenu *menu,int id,const char *label,mMenu *submenu);
void mMenuAddSep(mMenu *menu);
void mMenuAddCheck(mMenu *menu,int id,const char *label,mBool checked);
void mMenuAddRadio(mMenu *menu,int id,const char *label);

void mMenuAddTrTexts(mMenu *menu,int tridtop,int num);
void mMenuAddTrArray16(mMenu *menu,const uint16_t *buf);
void mMenuSetStrArray(mMenu *menu,int idtop,mStr *str,int num);
void mMenuAddStrArray(mMenu *menu,int idtop,mStr *str,int num);

void mMenuSetEnable(mMenu *menu,int id,int type);
void mMenuSetCheck(mMenu *menu,int id,int type);
void mMenuSetSubmenu(mMenu *menu,int id,mMenu *submenu);
void mMenuSetShortcutKey(mMenu *menu,int id,uint32_t sckey);

const char *mMenuGetTextByIndex(mMenu *menu,int index);

#ifdef __cplusplus
}
#endif

#endif
