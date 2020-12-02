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

#ifndef MLIB_MENUBAR_H
#define MLIB_MENUBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_MENUBAR(p)  ((mMenuBar *)(p))

typedef struct _mMenu mMenu;
typedef struct _mMenuItem mMenuItem;

typedef struct
{
	mMenu *menu;
	mMenuItem *itemCur;
}mMenuBarData;

typedef struct _mMenuBar
{
	mWidget wg;
	mMenuBarData mb;
}mMenuBar;


enum MMENUBAR_ARRAY16
{
	MMENUBAR_ARRAY16_END = 0xffff,
	MMENUBAR_ARRAY16_SEP = 0xfffe,
	MMENUBAR_ARRAY16_SUBMENU = 0xfffd,
	MMENUBAR_ARRAY16_AUTOCHECK = 0x8000,
	MMENUBAR_ARRAY16_RADIO = 0x4000
};


mMenuBar *mMenuBarNew(int size,mWidget *parent,uint32_t style);

mMenu *mMenuBarGetMenu(mMenuBar *bar);
void mMenuBarSetMenu(mMenuBar *bar,mMenu *menu);
void mMenuBarSetItemSubmenu(mMenuBar *bar,int id,mMenu *submenu);

void mMenuBarCreateMenuTrArray16(mMenuBar *bar,const uint16_t *buf,int idtop);
void mMenuBarCreateMenuTrArray16_radio(mMenuBar *bar,const uint16_t *buf,int idtop);

#ifdef __cplusplus
}
#endif

#endif
