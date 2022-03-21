/*$
 Copyright (C) 2013-2022 Azel.

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

#ifndef MLK_HSVPICKER_H
#define MLK_HSVPICKER_H

#define MLK_HSVPICKER(p)  ((mHSVPicker *)(p))
#define MLK_HSVPICKER_DEF mWidget wg; mHSVPickerData hsv;

typedef struct
{
	mPixbuf *img;
	mRgbCol col;
	int cur_sv_x,
		cur_sv_y,
		cur_hue,
		cur_hue_y,
		fgrab;
}mHSVPickerData;

struct _mHSVPicker
{
	mWidget wg;
	mHSVPickerData hsv;
};

enum MHSVPICKER_NOTIFY
{
	MHSVPICKER_N_CHANGE_HUE,
	MHSVPICKER_N_CHANGE_SV
};


#ifdef __cplusplus
extern "C" {
#endif

mHSVPicker *mHSVPickerNew(mWidget *parent,int size,uint32_t fstyle,int height);
mHSVPicker *mHSVPickerCreate(mWidget *parent,int id,uint32_t margin_pack,uint32_t fstyle,int height);

void mHSVPickerDestroy(mWidget *p);
void mHSVPickerHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mHSVPickerHandle_event(mWidget *wg,mEvent *ev);

mRgbCol mHSVPickerGetRGBColor(mHSVPicker *p);
void mHSVPickerGetHSVColor(mHSVPicker *p,double *dst);

void mHSVPickerSetHue(mHSVPicker *p,int hue);
void mHSVPickerSetSV(mHSVPicker *p,double s,double v);
void mHSVPickerSetHSVColor(mHSVPicker *p,double h,double s,double v);
void mHSVPickerSetRGBColor(mHSVPicker *p,mRgbCol col);

#ifdef __cplusplus
}
#endif

#endif
