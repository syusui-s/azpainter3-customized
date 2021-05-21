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

#ifndef MLK_MENU_H
#define MLK_MENU_H

typedef struct _mMenuItemInfo mMenuItemInfo;
typedef struct _mMenuItemDraw mMenuItemDraw;

struct _mMenuItemDraw
{
	mBox box;
	uint32_t flags;
};

struct _mMenuItemInfo
{
	int id;
	int16_t width,
		height;
	uint32_t flags,
		shortcut_key;
	const char *text;
	mMenu *submenu;
	void (*draw)(mPixbuf *,mMenuItemInfo *,mMenuItemDraw *);
	intptr_t param1,param2;
};


enum MMENUITEM_FLAGS
{
	MMENUITEM_F_COPYTEXT   = 1<<0,
	MMENUITEM_F_SEP        = 1<<1,
	MMENUITEM_F_DISABLE    = 1<<2,
	MMENUITEM_F_CHECK_TYPE = 1<<3,
	MMENUITEM_F_RADIO_TYPE = 1<<4,
	MMENUITEM_F_CHECKED    = 1<<5
};

enum MMENUITEM_DRAW_FLAGS
{
	MMENUITEM_DRAW_F_SELECTED = 1
};

enum MMENU_ARRAY16
{
	MMENU_ARRAY16_END = 0xffff,
	MMENU_ARRAY16_SEP = 0xfffe,
	MMENU_ARRAY16_SUB_START = 0xfffd,
	MMENU_ARRAY16_SUB_END = 0xfffc,
	MMENU_ARRAY16_CHECK = 0x8000,
	MMENU_ARRAY16_RADIO = 0x4000
};

/*---------*/

#ifdef __cplusplus
extern "C" {
#endif

/* mMenuItem */

mMenuItem *mMenuItemGetNext(mMenuItem *item);
int mMenuItemGetID(mMenuItem *item);
const char *mMenuItemGetText(mMenuItem *item);
intptr_t mMenuItemGetParam1(mMenuItem *item);
mMenuItemInfo *mMenuItemGetInfo(mMenuItem *item);

/* mMenu */

mMenu *mMenuNew(void);
void mMenuDestroy(mMenu *p);

mMenuItem *mMenuPopup(mMenu *p,mWidget *parent,int x,int y,mBox *box,uint32_t flags,mWidget *notify);

int mMenuGetNum(mMenu *p);
mMenuItem *mMenuGetTopItem(mMenu *p);
mMenuItem *mMenuGetBottomItem(mMenu *p);
mMenuItem *mMenuGetItemAtIndex(mMenu *p,int index);

void mMenuDeleteAll(mMenu *p);
mlkbool mMenuDeleteItem_id(mMenu *p,int id);
mlkbool mMenuDeleteItem_index(mMenu *p,int index);

mMenuItemInfo *mMenuAppend(mMenu *p,int id,const char *text,uint32_t sckey,uint32_t flags);
mMenuItemInfo *mMenuAppendInfo(mMenu *p,mMenuItemInfo *info);
mMenuItemInfo *mMenuAppendText(mMenu *p,int id,const char *text);
mMenuItemInfo *mMenuAppendText_copy(mMenu *p,int id,const char *text,int len);
mMenuItemInfo *mMenuAppendText_param(mMenu *p,int id,const char *text,intptr_t param);
mMenuItemInfo *mMenuAppendSubmenu(mMenu *p,int id,const char *text,mMenu *submenu,uint32_t flags);
void mMenuAppendSep(mMenu *p);
mMenuItemInfo *mMenuAppendCheck(mMenu *p,int id,const char *text,int checked);
mMenuItemInfo *mMenuAppendRadio(mMenu *p,int id,const char *text);

void mMenuAppendTrText(mMenu *p,int idtop,int num);
void mMenuAppendTrText_array16(mMenu *p,const uint16_t *buf);
void mMenuAppendTrText_array16_radio(mMenu *p,const uint16_t *buf);
void mMenuAppendStrArray(mMenu *p,mStr *str,int idtop,int num);

void mMenuSetItemEnable(mMenu *p,int id,int type);
void mMenuSetItemCheck(mMenu *p,int id,int type);
void mMenuSetItemSubmenu(mMenu *p,int id,mMenu *submenu);
void mMenuSetItemShortcutKey(mMenu *p,int id,uint32_t sckey);

mMenu *mMenuGetItemSubmenu(mMenu *p,int id);
const char *mMenuGetItemText_atIndex(mMenu *p,int index);

#ifdef __cplusplus
}
#endif

#endif
