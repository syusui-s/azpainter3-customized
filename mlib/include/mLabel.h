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

#ifndef MLIB_LABEL_H
#define MLIB_LABEL_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LABEL(p)  ((mLabel *)(p))

typedef struct _mWidgetLabelTextRowInfo mWidgetLabelTextRowInfo;

typedef struct
{
	char *text;
	mWidgetLabelTextRowInfo *rowbuf;
	uint32_t style;
	mSize sztext;
}mLabelData;

typedef struct _mLabel
{
	mWidget wg;
	mLabelData lb;
}mLabel;

enum MLABEL_STYLE
{
	MLABEL_S_RIGHT  = 1<<0,
	MLABEL_S_CENTER = 1<<1,
	MLABEL_S_BOTTOM = 1<<2,
	MLABEL_S_MIDDLE = 1<<3,
	MLABEL_S_BORDER = 1<<4
};


void mLabelDestroyHandle(mWidget *p);
void mLabelCalcHintHandle(mWidget *p);
void mLabelDrawHandle(mWidget *p,mPixbuf *pixbuf);

mLabel *mLabelCreate(mWidget *parent,uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *text);

mLabel *mLabelNew(int size,mWidget *parent,uint32_t style);
void mLabelSetText(mLabel *p,const char *text);
void mLabelSetText_int(mLabel *p,int val);
void mLabelSetText_floatint(mLabel *p,int val,int dig);

#ifdef __cplusplus
}
#endif

#endif
