/*$
 Copyright (C) 2013-2022 Azel.

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

#ifndef MLK_LISTVIEWPAGE_H
#define MLK_LISTVIEWPAGE_H

#include "mlk_scrollview.h"

#define MLK_LISTVIEWPAGE(p)  ((mListViewPage *)(p))
#define MLK_LISTVIEWPAGE_DEF MLK_SCROLLVIEWPAGE_DEF mListViewPageData lvp;

#define MLK_LISTVIEWPAGE_ITEM_PADDING_X 3
#define MLK_LISTVIEWPAGE_ITEM_PADDING_Y 2
#define MLK_LISTVIEWPAGE_CHECKBOX_SIZE  13
#define MLK_LISTVIEWPAGE_CHECKBOX_SPACE_RIGHT 4
#define MLK_LISTVIEWPAGE_ICON_SPACE_RIGHT     3

typedef struct
{
	mCIManager *manager;
	mImageList *imglist;
	mListHeader *header;
	mWidget *widget_send;
	uint32_t fstyle,
		fstyle_listview;
	int item_height,
		header_height;
}mListViewPageData;

struct _mListViewPage
{
	MLK_SCROLLVIEWPAGE_DEF
	mListViewPageData lvp;
};

enum MLISTVIEWPAGE_STYLE
{
	MLISTVIEWPAGE_S_POPUP = 1
};

enum MLISTVIEWPAGE_NOTIFY
{
	MLISTVIEWPAGE_N_POPUP_QUIT = 10000
};


mListViewPage *mListViewPageNew(mWidget *parent,int size,
	uint32_t fstyle,uint32_t fstyle_listview,
	mCIManager *manager,mWidget *send);

void mListViewPageDestroy(mWidget *wg);
int mListViewPageHandle_event(mWidget *wg,mEvent *ev);

void mListViewPage_setImageList(mListViewPage *p,mImageList *img);
void mListViewPage_scrollToFocus(mListViewPage *p,int dir,int margin_num);
int mListViewPage_getItemY(mListViewPage *p,mColumnItem *pi);

#endif
