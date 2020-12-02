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

#ifndef MLIB_CONTAINERVIEW_H
#define MLIB_CONTAINERVIEW_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_CONTAINERVIEW(p)  ((mContainerView *)(p))

typedef struct _mScrollBar mScrollBar;

typedef struct
{
	mWidget *area;
	mScrollBar *scrv;
	uint32_t style;
}mContainerViewData;

typedef struct _mContainerView
{
	mWidget wg;
	mContainerViewData cv;
}mContainerView;


enum MCONTAINERVIEW_STYLE
{
	MCONTAINERVIEW_S_FIX_SCROLL = 1<<0
};


mContainerView *mContainerViewNew(int size,mWidget *parent,uint32_t style);
void mContainerViewConstruct(mContainerView *p);

#ifdef __cplusplus
}
#endif

#endif
