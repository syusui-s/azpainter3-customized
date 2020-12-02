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

#ifndef MLIB_MENUWINDOW_H
#define MLIB_MENUWINDOW_H

#include "mWindowDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_MENUWINDOW(p) ((mMenuWindow *)(p))

typedef struct _mMenuWindow mMenuWindow;
typedef struct _mMenuBar mMenuBar;

typedef struct
{
	mMenu *dat;

	mMenuItem *itemCur,
		*itemRet;
	mMenuWindow *winSub,
		*winTop,
		*winParent,
		*winFocus,
		*winScroll;
	mWidget *widgetNotify;
	mMenuBar *menubar;
	
	int labelLeftMaxW,
		maxH,
		scrY;
	uint32_t flags;
}mMenuWindowData;

struct _mMenuWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mMenuWindowData menu;
};

#define MMENUWINDOW_F_HAVE_SCROLL    1
#define MMENUWINDOW_F_SCROLL_DOWN    2
#define MMENUWINDOW_F_MENUBAR_CHANGE 4


mMenuWindow *mMenuWindowNew(mMenu *menu);

mMenuItem *mMenuWindowShowPopup(mMenuWindow *win,
	mWidget *send,int rootx,int rooty,uint32_t flags);

mMenuItem *mMenuWindowShowMenuBar(mMenuWindow *win,
	mMenuBar *menubar,int rootx,int rooty,int itemid);

#ifdef __cplusplus
}
#endif

#endif
