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

#ifndef MLIB_SLIDERBAR_H
#define MLIB_SLIDERBAR_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mSliderBar mSliderBar;

#define M_SLIDERBAR(p)  ((mSliderBar *)(p))

typedef struct
{
	uint32_t style;
	int min,max,pos;
	uint8_t barw_hf,fpress;
	void (*handle)(mSliderBar *,int,int);
}mSliderBarData;

struct _mSliderBar
{
	mWidget wg;
	mSliderBarData sl;
};


enum MSLIDERBAR_STYLE
{
	MSLIDERBAR_S_VERT  = 1<<0,
	MSLIDERBAR_S_SMALL = 1<<1
};

enum MSLIDERBAR_NOTIFY
{
	MSLIDERBAR_N_HANDLE
};

enum MSLIDERBAR_HANDLE_FLAGS
{
	MSLIDERBAR_HANDLE_F_CHANGE  = 1<<0,
	MSLIDERBAR_HANDLE_F_KEY     = 1<<1,
	MSLIDERBAR_HANDLE_F_PRESS   = 1<<2,
	MSLIDERBAR_HANDLE_F_MOTION  = 1<<3,
	MSLIDERBAR_HANDLE_F_RELEASE = 1<<4
};


void mSliderBarDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mSliderBarEventHandle(mWidget *wg,mEvent *ev);

mSliderBar *mSliderBarCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mSliderBar *mSliderBarNew(int size,mWidget *parent,uint32_t style);

int mSliderBarGetPos(mSliderBar *p);
void mSliderBarSetStatus(mSliderBar *p,int min,int max,int pos);
void mSliderBarSetRange(mSliderBar *p,int min,int max);
mBool mSliderBarSetPos(mSliderBar *p,int pos);

#ifdef __cplusplus
}
#endif

#endif
