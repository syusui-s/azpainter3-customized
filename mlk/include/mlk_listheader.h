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

#ifndef MLK_LISTHEADER_H
#define MLK_LISTHEADER_H

#define MLK_LISTHEADER(p)      ((mListHeader *)(p))
#define MLK_LISTHEADER_ITEM(p) ((mListHeaderItem *)(p))
#define MLK_LISTHEADER_DEF     mWidget wg; mListHeaderData lh;

typedef struct _mListHeaderItem mListHeaderItem;

typedef struct
{
	mList list;
	mListHeaderItem *item_drag,
		*item_sort;
	uint32_t fstyle;
	int scrx,
		is_sort_rev,
		fpress,
		drag_left;
}mListHeaderData;

struct _mListHeader
{
	mWidget wg;
	mListHeaderData lh;
};

struct _mListHeaderItem
{
	mListItem i;
	char *text;
	int width;
	uint32_t flags;
	intptr_t param;
};

enum MLISTHEADER_STYLE
{
	MLISTHEADER_S_SORT = 1<<0
};

enum MLISTHEADER_NOTIFY
{
	MLISTHEADER_N_RESIZE,
	MLISTHEADER_N_CHANGE_SORT
};

enum MLISTHEADER_ITEM_FLAGS
{
	MLISTHEADER_ITEM_F_COPYTEXT = 1<<0,
	MLISTHEADER_ITEM_F_RIGHT  = 1<<1,
	MLISTHEADER_ITEM_F_FIX    = 1<<2,
	MLISTHEADER_ITEM_F_EXPAND = 1<<3
};

#ifdef __cplusplus
extern "C" {
#endif

mListHeader *mListHeaderNew(mWidget *parent,int size,uint32_t fstyle);
mListHeader *mListHeaderCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mListHeaderDestroy(mWidget *p);
void mListHeaderHandle_calcHint(mWidget *p);
int mListHeaderHandle_event(mWidget *wg,mEvent *ev);

void mListHeaderAddItem(mListHeader *p,const char *text,int width,uint32_t flags,intptr_t param);
mListHeaderItem *mListHeaderGetTopItem(mListHeader *p);
mListHeaderItem *mListHeaderGetItem_atIndex(mListHeader *p,int index);
int mListHeaderGetItemWidth(mListHeader *p,int index);
int mListHeaderGetFullWidth(mListHeader *p);
void mListHeaderSetScrollPos(mListHeader *p,int scrx);
void mListHeaderSetSort(mListHeader *p,int index,mlkbool rev);
void mListHeaderSetExpandItemWidth(mListHeader *p);
void mListHeaderSetItemWidth(mListHeader *p,int index,int width,mlkbool add_pad);
int mListHeaderGetPos_atX(mListHeader *p,int x);
int mListHeaderGetItemX(mListHeader *p,int index);

#ifdef __cplusplus
}
#endif

#endif
