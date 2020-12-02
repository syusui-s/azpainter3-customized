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

#ifndef MLIB_WINDOWDEF_H
#define MLIB_WINDOWDEF_H

#include "mContainerDef.h"

typedef struct _mWindowSysDat mWindowSysDat;
typedef struct _mMenuBar mMenuBar;
typedef struct _mAccelerator mAccelerator;

typedef struct
{
	mWindowSysDat *sys;

	mWindow *owner;
	mWidget *focus_widget;
	mPixbuf *pixbuf;
	mMenuBar *menubar;
	mAccelerator *accelerator;
	
	uint32_t fStyle;
	mRect rcUpdate;
}mWindowData;

struct _mWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
};

enum MWINDOW_STYLE
{
	MWINDOW_S_POPUP      = 1<<0,
	MWINDOW_S_TITLE      = 1<<1,
	MWINDOW_S_BORDER     = 1<<2,
	MWINDOW_S_CLOSE      = 1<<3,
	MWINDOW_S_SYSMENU    = 1<<4,
	MWINDOW_S_MINIMIZE   = 1<<5,
	MWINDOW_S_MAXIMIZE   = 1<<6,
	MWINDOW_S_OWNER      = 1<<7,
	MWINDOW_S_TOOL       = 1<<8,
	MWINDOW_S_TABMOVE    = 1<<9,
	MWINDOW_S_NO_IM      = 1<<10,
	MWINDOW_S_NO_RESIZE  = 1<<11,
	MWINDOW_S_DIALOG     = 1<<12,
	MWINDOW_S_ENABLE_DND = 1<<13,
	
	MWINDOW_S_NORMAL = MWINDOW_S_TITLE | MWINDOW_S_BORDER
		| MWINDOW_S_CLOSE | MWINDOW_S_SYSMENU | MWINDOW_S_MINIMIZE
		| MWINDOW_S_MAXIMIZE,

	MWINDOW_S_DIALOG_NORMAL = MWINDOW_S_NORMAL | MWINDOW_S_DIALOG
		| MWINDOW_S_TABMOVE | MWINDOW_S_OWNER
};

#endif
