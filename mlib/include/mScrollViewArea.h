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

#ifndef MLIB_SCROLLVIEWAREA_H
#define MLIB_SCROLLVIEWAREA_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SCROLLVIEWAREA(p)  ((mScrollViewArea *)(p))

typedef struct _mScrollViewArea mScrollViewArea;
typedef struct _mScrollBar mScrollBar;

typedef struct
{
	mBool (*isBarVisible)(mScrollViewArea *,int size,mBool horz);
}mScrollViewAreaData;

struct _mScrollViewArea
{
	mWidget wg;
	mScrollViewAreaData sva;
};

enum MSCROLLVIEWAREA_NOTIFY
{
	MSCROLLVIEWAREA_N_SCROLL_HORZ,
	MSCROLLVIEWAREA_N_SCROLL_VERT
};


mScrollViewArea *mScrollViewAreaNew(int size,mWidget *parent);

void mScrollViewAreaGetScrollPos(mScrollViewArea *p,mPoint *pt);
int mScrollViewAreaGetHorzScrollPos(mScrollViewArea *p);
int mScrollViewAreaGetVertScrollPos(mScrollViewArea *p);
mScrollBar *mScrollViewAreaGetScrollBar(mScrollViewArea *p,mBool vert);

#ifdef __cplusplus
}
#endif

#endif
