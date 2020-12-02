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

#ifndef MLIB_LISTHEADER_H
#define MLIB_LISTHEADER_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LISTHEADER(p)      ((mListHeader *)(p))
#define M_LISTHEADER_ITEM(p) ((mListHeaderItem *)(p))

typedef struct _mListHeaderItem
{
	mListItem i;
	char *text;
	int width;
	uint32_t flags;
	intptr_t param;
}mListHeaderItem;


typedef struct
{
	mList list;
	uint32_t style;
	int sortup,scrx,fpress,dragLeft;
	mListHeaderItem *dragitem,*sortitem;
}mListHeaderData;

typedef struct _mListHeader
{
	mWidget wg;
	mListHeaderData lh;
}mListHeader;


enum MLISTHEADER_STYLE
{
	MLISTHEADER_STYLE_SORT = 1<<0
};

enum MLISTHEADER_NOTIFY
{
	MLISTHEADER_N_CHANGE_WIDTH,
	MLISTHEADER_N_CHANGE_SORT
};

enum MLISTHEADER_ITEM_FLAGS
{
	MLISTHEADER_ITEM_F_RIGHT  = 1<<0,
	MLISTHEADER_ITEM_F_FIX    = 1<<1,
	MLISTHEADER_ITEM_F_EXPAND = 1<<2
};


void mListHeaderDestroyHandle(mWidget *p);
void mListHeaderCalcHintHandle(mWidget *p);
int mListHeaderEventHandle(mWidget *wg,mEvent *ev);

mListHeader *mListHeaderNew(int size,mWidget *parent,uint32_t style);

mListHeaderItem *mListHeaderGetTopItem(mListHeader *p);
void mListHeaderAddItem(mListHeader *p,const char *text,int width,uint32_t flags,intptr_t param);
int mListHeaderGetFullWidth(mListHeader *p);
void mListHeaderSetScrollPos(mListHeader *p,int scrx);
void mListHeaderSetSortParam(mListHeader *p,int index,mBool up);

#ifdef __cplusplus
}
#endif

#endif
