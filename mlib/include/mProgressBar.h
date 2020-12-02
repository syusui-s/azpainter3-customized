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

#ifndef MLIB_PROGRESSBAR_H
#define MLIB_PROGRESSBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_PROGRESSBAR(p)  ((mProgressBar *)(p))

typedef struct
{
	uint32_t style;
	int min,max,pos,
		textlen,
		sub_step,sub_max,sub_toppos,
		sub_curcnt,sub_curstep,sub_nextcnt;
	char *text;
}mProgressBarData;

typedef struct _mProgressBar
{
	mWidget wg;
	mProgressBarData pb;
}mProgressBar;


enum MPROGRESSBAR_STYLE
{
	MPROGRESSBAR_S_FRAME = 1<<0,
	MPROGRESSBAR_S_TEXT  = 1<<1,
	MPROGRESSBAR_S_TEXT_PERS = 1<<2,
};


void mProgressBarDestroyHandle(mWidget *p);
void mProgressBarCalcHintHandle(mWidget *p);
void mProgressBarDrawHandle(mWidget *p,mPixbuf *pixbuf);

mProgressBar *mProgressBarNew(int size,mWidget *parent,uint32_t style);
mProgressBar *mProgressBarCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4);

void mProgressBarSetStatus(mProgressBar *p,int min,int max,int pos);
void mProgressBarSetText(mProgressBar *p,const char *text);
mBool mProgressBarSetPos(mProgressBar *p,int pos);
void mProgressBarIncPos(mProgressBar *p);
mBool mProgressBarAddPos(mProgressBar *p,int add);

void mProgressBarBeginSubStep(mProgressBar *p,int stepnum,int max);
void mProgressBarBeginSubStep_onestep(mProgressBar *p,int stepnum,int max);
mBool mProgressBarIncSubStep(mProgressBar *p);

#ifdef __cplusplus
}
#endif

#endif
