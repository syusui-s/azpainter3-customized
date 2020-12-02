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

#ifndef MLIB_SCROLLBAR_H
#define MLIB_SCROLLBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SCROLLBAR(p)  ((mScrollBar *)(p))

typedef struct _mScrollBar mScrollBar;

typedef struct
{
	uint32_t style;
	int min,max,page,pos,
		range,fpress,dragDiff;
	void (*handle)(mScrollBar *,int,int);
}mScrollBarData;

struct _mScrollBar
{
	mWidget wg;
	mScrollBarData sb;
};


enum MSCROLLBAR_STYLE
{
	MSCROLLBAR_S_HORZ = 0,
	MSCROLLBAR_S_VERT = 1<<0
};

enum MSCROLLBAR_NOTIFY
{
	MSCROLLBAR_N_HANDLE
};

enum MSCROLLBAR_NOTIFY_HANDLE_FLAGS
{
	MSCROLLBAR_N_HANDLE_F_CHANGE  = 1<<0,
	MSCROLLBAR_N_HANDLE_F_PRESS   = 1<<1,
	MSCROLLBAR_N_HANDLE_F_MOTION  = 1<<2,
	MSCROLLBAR_N_HANDLE_F_RELEASE = 1<<3,
	MSCROLLBAR_N_HANDLE_F_PAGE    = 1<<4
};


#define MSCROLLBAR_WIDTH  15


void mScrollBarDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mScrollBarEventHandle(mWidget *wg,mEvent *ev);

mScrollBar *mScrollBarNew(int size,mWidget *parent,uint32_t style);

mBool mScrollBarIsTopPos(mScrollBar *p);
mBool mScrollBarIsBottomPos(mScrollBar *p);
int mScrollBarGetPos(mScrollBar *p);
void mScrollBarSetStatus(mScrollBar *p,int min,int max,int page);
void mScrollBarSetPage(mScrollBar *p,int page);
mBool mScrollBarSetPos(mScrollBar *p,int pos);
mBool mScrollBarSetPosToEnd(mScrollBar *p);
mBool mScrollBarMovePos(mScrollBar *p,int dir);

#ifdef __cplusplus
}
#endif

#endif
