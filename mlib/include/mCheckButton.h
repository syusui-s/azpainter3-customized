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

#ifndef MLIB_CHECKBUTTON_H
#define MLIB_CHECKBUTTON_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_CHECKBUTTON(p)  ((mCheckButton *)(p))

typedef struct _mWidgetLabelTextRowInfo mWidgetLabelTextRowInfo;

typedef struct
{
	char *text;
	mWidgetLabelTextRowInfo *rowbuf;
	uint32_t style,flags;
	mSize sztext;
}mCheckButtonData;

typedef struct _mCheckButton
{
	mWidget wg;
	mCheckButtonData ck;
}mCheckButton;

enum MCHECKBUTTON_STYLE
{
	MCHECKBUTTON_S_RADIO = 1<<0,
	MCHECKBUTTON_S_RADIO_GROUP = 1<<1
};

enum MCHECKBUTTON_NOTIFY
{
	MCHECKBUTTON_N_PRESS
};


void mCheckButtonDestroyHandle(mWidget *p);
void mCheckButtonCalcHintHandle(mWidget *p);
void mCheckButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mCheckButtonEventHandle(mWidget *wg,mEvent *ev);

mCheckButton *mCheckButtonCreate(mWidget *parent,int id,
	uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *text,int checked);

mCheckButton *mCheckButtonNew(int size,mWidget *parent,uint32_t style);
void mCheckButtonSetText(mCheckButton *p,const char *text);
void mCheckButtonSetState(mCheckButton *p,int type);
void mCheckButtonSetRadioSel(mCheckButton *p,int no);
mBool mCheckButtonIsChecked(mCheckButton *p);
mCheckButton *mCheckButtonGetRadioInfo(mCheckButton *start,mCheckButton **ppTop,mCheckButton **ppEnd);
int mCheckButtonGetGroupSelIndex(mCheckButton *p);

#ifdef __cplusplus
}
#endif

#endif
