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

#ifndef MLIB_EXPANDER_H
#define MLIB_EXPANDER_H

#include "mContainerDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_EXPANDER(p)  ((mExpander *)(p))

typedef struct
{
	uint32_t style;
	char *text;
	mRect padding;
	int headerH;
	mBool expand;
}mExpanderData;

typedef struct _mExpander
{
	mWidget wg;
	mContainerData ct;
	mExpanderData exp;
}mExpander;


enum MEXPANDER_STYLE
{
	MEXPANDER_S_BORDER_TOP  = 1<<0,
	MEXPANDER_S_HEADER_DARK = 1<<1
};

enum MEXPANDER_NOTIFY
{
	MEXPANDER_N_TOGGLE
};


void mExpanderDestroyHandle(mWidget *p);
void mExpanderCalcHintHandle(mWidget *p);
int mExpanderEventHandle(mWidget *wg,mEvent *ev);
void mExpanderDrawHandle(mWidget *wg,mPixbuf *pixbuf);

mExpander *mExpanderNew(int size,mWidget *parent,uint32_t style);
mExpander *mExpanderCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,const char *text);

void mExpanderSetText(mExpander *p,const char *text);
void mExpanderSetPadding_b4(mExpander *p,uint32_t val);
void mExpanderSetExpand(mExpander *p,int type);

#ifdef __cplusplus
}
#endif

#endif
