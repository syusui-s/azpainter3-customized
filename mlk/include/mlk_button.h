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

#ifndef MLK_BUTTON_H
#define MLK_BUTTON_H

#define MLK_BUTTON(p)  ((mButton *)(p))
#define MLK_BUTTON_DEF mWidget wg; mButtonData btt;

typedef void (*mFuncButtonHandle_pressed)(mWidget *p);

typedef struct
{
	mWidgetLabelText txt;
	uint32_t fstyle,
		flags;
	int grabbed_key;
	mFuncButtonHandle_pressed pressed;
}mButtonData;

struct _mButton
{
	mWidget wg;
	mButtonData btt;
};

enum MBUTTON_STYLE
{
	MBUTTON_S_COPYTEXT = 1<<0,
	MBUTTON_S_DIRECT_PRESS = 1<<1,
	MBUTTON_S_REAL_W = 1<<2,
	MBUTTON_S_REAL_H = 1<<3,
	
	MBUTTON_S_REAL_WH = MBUTTON_S_REAL_W | MBUTTON_S_REAL_H
};

enum MBUTTON_FLAGS
{
	MBUTTON_F_PRESSED = 1<<0,
	MBUTTON_F_GRABBED = 1<<1
};

enum MBUTTON_NOTIFY
{
	MBUTTON_N_PRESSED
};


#ifdef __cplusplus
extern "C" {
#endif

mButton *mButtonNew(mWidget *parent,int size,uint32_t fstyle);
mButton *mButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text);
void mButtonDestroy(mWidget *p);

void mButtonHandle_calcHint(mWidget *p);
void mButtonHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mButtonHandle_event(mWidget *wg,mEvent *ev);

void mButtonSetText(mButton *p,const char *text);
void mButtonSetText_copy(mButton *p,const char *text);
void mButtonSetState(mButton *p,mlkbool pressed);
mlkbool mButtonIsPressed(mButton *p);

void mButtonDrawBase(mButton *p,mPixbuf *pixbuf);

#ifdef __cplusplus
}
#endif

#endif
