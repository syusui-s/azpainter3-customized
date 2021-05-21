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

#ifndef MLK_SLIDERBAR_H
#define MLK_SLIDERBAR_H

#define MLK_SLIDERBAR(p)  ((mSliderBar *)(p))
#define MLK_SLIDERBAR_DEF mWidget wg; mSliderBarData sl;

typedef void (*mFuncSliderBarHandle_action)(mWidget *p,int pos,uint32_t flags);

typedef struct
{
	uint32_t fstyle;
	int min,max,pos,fgrab;
	mFuncSliderBarHandle_action action;
}mSliderBarData;

struct _mSliderBar
{
	mWidget wg;
	mSliderBarData sl;
};

enum MSLIDERBAR_STYLE
{
	MSLIDERBAR_S_HORZ  = 0,
	MSLIDERBAR_S_VERT  = 1<<0,
	MSLIDERBAR_S_SMALL = 1<<1
};

enum MSLIDERBAR_NOTIFY
{
	MSLIDERBAR_N_ACTION
};

enum MSLIDERBAR_ACTION_FLAGS
{
	MSLIDERBAR_ACTION_F_CHANGE_POS = 1<<0,
	MSLIDERBAR_ACTION_F_KEY_PRESS  = 1<<1,
	MSLIDERBAR_ACTION_F_BUTTON_PRESS = 1<<2,
	MSLIDERBAR_ACTION_F_DRAG         = 1<<3,
	MSLIDERBAR_ACTION_F_BUTTON_RELEASE = 1<<4,
	MSLIDERBAR_ACTION_F_SCROLL     = 1<<5
};


#ifdef __cplusplus
extern "C" {
#endif

mSliderBar *mSliderBarNew(mWidget *parent,int size,uint32_t fstyle);
mSliderBar *mSliderBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);
void mSliderBarDestroy(mWidget *wg);

void mSliderBarHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mSliderBarHandle_event(mWidget *wg,mEvent *ev);

int mSliderBarGetPos(mSliderBar *p);
void mSliderBarSetStatus(mSliderBar *p,int min,int max,int pos);
void mSliderBarSetRange(mSliderBar *p,int min,int max);
mlkbool mSliderBarSetPos(mSliderBar *p,int pos);

#ifdef __cplusplus
}
#endif

#endif
