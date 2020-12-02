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

#ifndef MLIB_TAB_H
#define MLIB_TAB_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_TAB(p)     ((mTab *)(p))
#define M_TABITEM(p) ((mTabItem *)(p))

typedef struct
{
	mListItem i;
	char *label;
	intptr_t param;
	int pos,w;
}mTabItem;

typedef struct
{
	uint32_t style;
	mList list;

	mTabItem *focusitem;
}mTabData;

typedef struct _mTab
{
	mWidget wg;
	mTabData tab;
}mTab;


enum MTAB_STYLE
{
	MTAB_S_TOP    = 1<<0,
	MTAB_S_BOTTOM = 1<<1,
	MTAB_S_LEFT   = 1<<2,
	MTAB_S_RIGHT  = 1<<3,
	MTAB_S_HAVE_SEP = 1<<4,
	MTAB_S_FIT_SIZE = 1<<5
};

enum MTAB_NOTIFY
{
	MTAB_N_CHANGESEL
};


void mTabDestroyHandle(mWidget *p);
void mTabCalcHintHandle(mWidget *p);
void mTabDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mTabEventHandle(mWidget *wg,mEvent *ev);
void mTabOnsizeHandle(mWidget *wg);

mTab *mTabCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mTab *mTabNew(int size,mWidget *parent,uint32_t style);

void mTabAddItem(mTab *p,const char *label,int w,intptr_t param);
void mTabAddItemText(mTab *p,const char *label);
void mTabDelItem_index(mTab *p,int index);

int mTabGetSelItem_index(mTab *p);
mTabItem *mTabGetItemByIndex(mTab *p,int index);
intptr_t mTabGetItemParam_index(mTab *p,int index);

void mTabSetSel_index(mTab *p,int index);
void mTabSetHintSize_byItems(mTab *p);

#ifdef __cplusplus
}
#endif

#endif
