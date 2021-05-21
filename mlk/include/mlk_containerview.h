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

#ifndef MLK_CONTAINERVIEW_H
#define MLK_CONTAINERVIEW_H

#define MLK_CONTAINERVIEW(p)  ((mContainerView *)(p))
#define MLK_CONTAINERVIEW_DEF mWidget wg; mContainerViewData cv;

typedef struct
{
	mWidget *page;
	mScrollBar *scrv;
	uint32_t fstyle;
}mContainerViewData;

struct _mContainerView
{
	mWidget wg;
	mContainerViewData cv;
};

enum MCONTAINERVIEW_STYLE
{
	MCONTAINERVIEW_S_FIX_SCROLLBAR = 1<<0
};


#ifdef __cplusplus
extern "C" {
#endif

mContainerView *mContainerViewNew(mWidget *parent,int size,uint32_t fstyle);

void mContainerViewSetPage(mContainerView *p,mWidget *page);
void mContainerViewLayout(mContainerView *p);
void mContainerViewReLayout(mContainerView *p);

#ifdef __cplusplus
}
#endif

#endif
