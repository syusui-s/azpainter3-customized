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

#ifndef MLK_TAB_H
#define MLK_TAB_H

#define MLK_TAB(p)     ((mTab *)(p))
#define MLK_TABITEM(p) ((mTabItem *)(p))
#define MLK_TAB_DEF    mWidget wg; mTabData tab;

typedef struct
{
	mListItem i;
	char *text;
	int width,pos;
	uint32_t flags;
	intptr_t param;
}mTabItem;

typedef struct
{
	uint32_t fstyle;
	mList list;

	mTabItem *focusitem;
}mTabData;

struct _mTab
{
	mWidget wg;
	mTabData tab;
};


enum MTAB_STYLE
{
	MTAB_S_TOP    = 1<<0,
	MTAB_S_BOTTOM = 1<<1,
	MTAB_S_LEFT   = 1<<2,
	MTAB_S_RIGHT  = 1<<3,
	MTAB_S_HAVE_SEP = 1<<4,
	MTAB_S_SHORTEN = 1<<5
};

enum MTAB_NOTIFY
{
	MTAB_N_CHANGE_SEL
};

enum MTABITEM_FLAGS
{
	MTABITEM_F_COPYTEXT = 1<<0
};


#ifdef __cplusplus
extern "C" {
#endif

mTab *mTabNew(mWidget *parent,int size,uint32_t fstyle);
mTab *mTabCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mTabDestroy(mWidget *p);
void mTabHandle_calcHint(mWidget *p);
void mTabHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mTabHandle_event(mWidget *wg,mEvent *ev);
void mTabHandle_resize(mWidget *wg);

void mTabAddItem(mTab *p,const char *text,int w,uint32_t flags,intptr_t param);
void mTabAddItem_text_static(mTab *p,const char *text);
mlkbool mTabDelItem_atIndex(mTab *p,int index);

int mTabGetSelItemIndex(mTab *p);
mTabItem *mTabGetItem_atIndex(mTab *p,int index);
intptr_t mTabGetItemParam_atIndex(mTab *p,int index);

void mTabSetSel_atIndex(mTab *p,int index);
void mTabSetHintSize(mTab *p);

#ifdef __cplusplus
}
#endif

#endif
