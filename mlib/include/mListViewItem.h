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

#ifndef MLIB_LISTVIEWITEM_H
#define MLIB_LISTVIEWITEM_H

#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LISTVIEWITEM(p)  ((mListViewItem *)(p))

typedef struct _mListViewItem mListViewItem;

typedef struct _mListViewItemDraw
{
	mWidget *widget;
	mBox box;
	uint8_t flags;
}mListViewItemDraw;

struct _mListViewItem
{
	mListItem i;

	const char *text;
	int textlen,icon;
	uint32_t flags;
	intptr_t param;
	void (*draw)(mPixbuf *,mListViewItem *,mListViewItemDraw *);
};

enum MLISTVIEW_ITEM_FLAGS
{
	MLISTVIEW_ITEM_F_SELECTED = 1<<0,
	MLISTVIEW_ITEM_F_CHECKED  = 1<<1,
	MLISTVIEW_ITEM_F_HEADER   = 1<<2,
	MLISTVIEW_ITEM_F_STATIC_TEXT = 1<<3
};

enum MLISTVIEWITEMDRAW_FLAGS
{
	MLISTVIEWITEMDRAW_F_SELECTED = 1<<0,
	MLISTVIEWITEMDRAW_F_ENABLED  = 1<<1,
	MLISTVIEWITEMDRAW_F_FOCUSED  = 1<<2,
	MLISTVIEWITEMDRAW_F_FOCUS_ITEM = 1<<3
};

void mListViewItemDestroy(mListItem *p);

#ifdef __cplusplus
}
#endif

#endif
