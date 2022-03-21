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

#ifndef MLK_LISTVIEW_H
#define MLK_LISTVIEW_H

#include "mlk_scrollview.h"

#define MLK_LISTVIEW(p)  ((mListView *)(p))
#define MLK_LISTVIEW_DEF MLK_SCROLLVIEW_DEF mListViewData lv;

typedef struct
{
	mCIManager manager;
	mImageList *imglist;
	uint32_t fstyle;
	int item_height,
		item_height_min,
		horz_width;
}mListViewData;

struct _mListView
{
	MLK_SCROLLVIEW_DEF
	mListViewData lv;
};

enum MLISTVIEW_STYLE
{
	MLISTVIEW_S_MULTI_COLUMN = 1<<0,
	MLISTVIEW_S_MULTI_SEL = 1<<1,
	MLISTVIEW_S_HAVE_HEADER = 1<<2,
	MLISTVIEW_S_CHECKBOX  = 1<<3,
	MLISTVIEW_S_GRID_ROW  = 1<<4,
	MLISTVIEW_S_GRID_COL  = 1<<5,
	MLISTVIEW_S_AUTO_WIDTH = 1<<6,
	MLISTVIEW_S_DESTROY_IMAGELIST = 1<<7,
	MLISTVIEW_S_HEADER_SORT = 1<<8
};

enum MLISTVIEW_NOTIFY
{
	MLISTVIEW_N_CHANGE_FOCUS,
	MLISTVIEW_N_CLICK_ON_FOCUS,
	MLISTVIEW_N_CHANGE_CHECK,
	MLISTVIEW_N_ITEM_L_DBLCLK,
	MLISTVIEW_N_ITEM_R_CLICK,
	MLISTVIEW_N_CHANGE_SORT
};


#ifdef __cplusplus
extern "C" {
#endif

mListView *mListViewNew(mWidget *parent,int size,uint32_t fstyle,uint32_t fstyle_scrollview);
mListView *mListViewCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,
	uint32_t fstyle,uint32_t fstyle_scrollview);

void mListViewDestroy(mWidget *p);
void mListViewHandle_calcHint(mWidget *p);
int mListViewHandle_event(mWidget *wg,mEvent *ev);

void mListViewSetImageList(mListView *p,mImageList *img);
void mListViewSetItemHeight_min(mListView *p,int h);
void mListViewSetItemDestroyHandle(mListView *p,void (*func)(mList *,mListItem *));

mListHeader *mListViewGetHeaderWidget(mListView *p);
int mListViewGetItemHeight(mListView *p);
int mListViewCalcItemWidth(mListView *p,int itemw);
int mListViewCalcWidgetWidth(mListView *p,int itemw);

int mListViewGetItemNum(mListView *p);
mColumnItem *mListViewGetTopItem(mListView *p);
mColumnItem *mListViewGetFocusItem(mListView *p);
mColumnItem *mListViewGetItem_atIndex(mListView *p,int index);
mColumnItem *mListViewGetItem_fromParam(mListView *p,intptr_t param);
mColumnItem *mListViewGetItem_fromText(mListView *p,const char *text);

void mListViewDeleteAllItem(mListView *p);
void mListViewDeleteItem(mListView *p,mColumnItem *item);
mColumnItem *mListViewDeleteItem_focus(mListView *p);

mColumnItem *mListViewInsertItem(mListView *p,mColumnItem *ins,const char *text,int icon,uint32_t flags,intptr_t param);
mColumnItem *mListViewAddItem(mListView *p,const char *text,int icon,uint32_t flags,intptr_t param);
mColumnItem *mListViewAddItem_size(mListView *p,int size,const char *text,int icon,uint32_t flags,intptr_t param);
mColumnItem *mListViewAddItem_text_static(mListView *p,const char *text);
mColumnItem *mListViewAddItem_text_static_param(mListView *p,const char *text,intptr_t param);
mColumnItem *mListViewAddItem_text_copy(mListView *p,const char *text);
mColumnItem *mListViewAddItem_text_copy_param(mListView *p,const char *text,intptr_t param);

void mListViewSetFocusItem(mListView *p,mColumnItem *item);
void mListViewSetFocusItem_scroll(mListView *p,mColumnItem *item);
mColumnItem *mListViewSetFocusItem_index(mListView *p,int index);
mColumnItem *mListViewSetFocusItem_param(mListView *p,intptr_t param);
mColumnItem *mListViewSetFocusItem_text(mListView *p,const char *text);

int mListViewGetItemIndex(mListView *p,mColumnItem *item);
intptr_t mListViewGetItemParam(mListView *p,int index);
void mListViewGetItemColumnText(mListView *p,mColumnItem *pi,int index,mStr *str);

void mListViewSetItemText_static(mListView *p,mColumnItem *pi,const char *text);
void mListViewSetItemText_copy(mListView *p,mColumnItem *pi,const char *text);
void mListViewSetItemColumnText(mListView *p,mColumnItem *pi,int col,const char *text);

mlkbool mListViewMoveItem_updown(mListView *p,mColumnItem *item,mlkbool down);
mlkbool mListViewMoveItem(mListView *p,mColumnItem *item,mColumnItem *insert);

void mListViewSortItem(mListView *p,int (*comp)(mListItem *,mListItem *,void *),void *param);
void mListViewSetAutoWidth(mListView *p,mlkbool hintsize);
void mListViewSetColumnWidth_auto(mListView *p,int index);
void mListViewScrollToItem(mListView *p,mColumnItem *pi,int align);
void mListViewScrollToFocus(mListView *p);
void mListViewScrollToFocus_margin(mListView *p,int margin_itemnum);

#ifdef __cplusplus
}
#endif

#endif
