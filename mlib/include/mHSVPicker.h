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

#ifndef MLIB_HSVPICKER_H
#define MLIB_HSVPICKER_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_HSVPICKER(p)  ((mHSVPicker *)(p))

typedef struct
{
	mPixbuf *img;
	mRgbCol col;
	int curX,curY,curH,
		fbtt;
}mHSVPickerData;

typedef struct _mHSVPicker
{
	mWidget wg;
	mHSVPickerData hsv;
}mHSVPicker;


enum MHSVPICKER_NOTIFY
{
	MHSVPICKER_N_CHANGE_H,
	MHSVPICKER_N_CHANGE_SV
};


void mHSVPickerDestroyHandle(mWidget *p);
void mHSVPickerDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mHSVPickerEventHandle(mWidget *wg,mEvent *ev);

mHSVPicker *mHSVPickerCreate(mWidget *parent,int id,int height,uint32_t marginB4);

mHSVPicker *mHSVPickerNew(int size,mWidget *parent,uint32_t style,int height);

mRgbCol mHSVPickerGetRGBColor(mHSVPicker *p);
void mHSVPickerGetHSVColor(mHSVPicker *p,double *dst);

void mHSVPickerSetHue(mHSVPicker *p,int hue);
void mHSVPickerSetSV(mHSVPicker *p,double s,double v);
void mHSVPickerSetHSVColor(mHSVPicker *p,double h,double s,double v);
void mHSVPickerSetRGBColor(mHSVPicker *p,mRgbCol col);

void mHSVPickerRedraw(mHSVPicker *p);

#ifdef __cplusplus
}
#endif

#endif
