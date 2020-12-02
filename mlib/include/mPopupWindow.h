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

#ifndef MLIB_POPUPWINDOW_H
#define MLIB_POPUPWINDOW_H

#include "mWindowDef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mPopupWindow mPopupWindow;

#define M_POPUPWINDOW(p)  ((mPopupWindow *)(p))

typedef struct
{
	mBool (*quit)(mPopupWindow *,mBool bCancel);
	mBool bEnd;
}mPopupWindowData;

struct _mPopupWindow
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mPopupWindowData pop;
};


int mPopupWindowEventHandle(mWidget *wg,mEvent *ev);

mPopupWindow *mPopupWindowNew(int size,mWindow *owner,uint32_t style);

void mPopupWindowRun(mPopupWindow *p,int rootx,int rooty);
void mPopupWindowRun_show(mPopupWindow *p);

void mPopupWindowQuit(mPopupWindow *p,mBool bCancel);

#ifdef __cplusplus
}
#endif

#endif
