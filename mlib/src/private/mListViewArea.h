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

#ifndef MLIB_LISTVIEWAREA_H
#define MLIB_LISTVIEWAREA_H

#include "mScrollViewArea.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LISTVIEWAREA(p)  ((mListViewArea *)(p))

typedef struct _mListHeader mListHeader;

typedef struct
{
	mLVItemMan *manager;
	mImageList *iconimg;
	mListHeader *header;
	mWidget *owner;
	uint32_t style,
		styleLV;
	int itemH,
		headerH;
}mListViewAreaData;

typedef struct _mListViewArea
{
	mWidget wg;
	mScrollViewAreaData sva;
	mListViewAreaData lva;
}mListViewArea;


enum MLISTVIEWAREA_STYLE
{
	MLISTVIEWAREA_S_POPUP = 1
};

enum MLISTVIEWAREA_NOTIFY
{
	MLISTVIEWAREA_N_POPUPEND = 10000
};


#define MLISTVIEW_DRAW_ITEM_MARGIN    3
#define MLISTVIEW_DRAW_CHECKBOX_SIZE  13
#define MLISTVIEW_DRAW_CHECKBOX_SPACE 4
#define MLISTVIEW_DRAW_ICON_SPACE     3


/*------*/

mListViewArea *mListViewAreaNew(int size,mWidget *parent,
	uint32_t style,uint32_t style_listview,
	mLVItemMan *manager,mWidget *owner);

void mListViewArea_setImageList(mListViewArea *p,mImageList *img);
void mListViewArea_scrollToFocus(mListViewArea *p,int dir,int margin_num);

#ifdef __cplusplus
}
#endif

#endif
