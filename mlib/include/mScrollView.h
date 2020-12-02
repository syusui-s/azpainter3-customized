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

#ifndef MLIB_SCROLLVIEW_H
#define MLIB_SCROLLVIEW_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SCROLLVIEW(p)  ((mScrollView *)(p))

typedef struct _mScrollBar mScrollBar;
typedef struct _mScrollViewArea mScrollViewArea;

typedef struct
{
	uint32_t style;
	mScrollBar *scrh,*scrv;
	mScrollViewArea *area;
}mScrollViewData;

typedef struct _mScrollView
{
	mWidget wg;
	mScrollViewData sv;
}mScrollView;

enum MSCROLLVIEW_STYLE
{
	MSCROLLVIEW_S_HORZ     = 1<<0,
	MSCROLLVIEW_S_VERT     = 1<<1,
	MSCROLLVIEW_S_FIX_HORZ = 1<<2,
	MSCROLLVIEW_S_FIX_VERT = 1<<3,
	MSCROLLVIEW_S_FRAME    = 1<<4,
	MSCROLLVIEW_S_SCROLL_NOTIFY_SELF = 1<<5,
	
	MSCROLLVIEW_S_HORZVERT = MSCROLLVIEW_S_HORZ | MSCROLLVIEW_S_VERT,
	MSCROLLVIEW_S_FIX_HORZVERT = MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FIX_HORZ | MSCROLLVIEW_S_FIX_VERT,
	MSCROLLVIEW_S_HORZVERT_FRAME = MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FRAME
};


mScrollView *mScrollViewNew(int size,mWidget *parent,uint32_t style);
void mScrollViewConstruct(mScrollView *p);

#ifdef __cplusplus
}
#endif

#endif
