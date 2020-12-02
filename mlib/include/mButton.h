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

#ifndef MLIB_BUTTON_H
#define MLIB_BUTTON_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_BUTTON(p)  ((mButton *)(p))

typedef struct _mButton mButton;

typedef struct
{
	char *text;
	int textW;
	uint32_t style,flags,press_key;
	void (*onPressed)(mButton *);
}mButtonData;

struct _mButton
{
	mWidget wg;
	mButtonData btt;
};

enum MBUTTON_STYLE
{
	MBUTTON_S_REAL_W = 1<<0,
	MBUTTON_S_REAL_H = 1<<1,
	
	MBUTTON_S_REAL_WH = MBUTTON_S_REAL_W | MBUTTON_S_REAL_H
};

enum MBUTTON_NOTIFY
{
	MBUTTON_N_PRESS
};

/*--------*/

void mButtonDestroyHandle(mWidget *p);
void mButtonCalcHintHandle(mWidget *p);
void mButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mButtonEventHandle(mWidget *wg,mEvent *ev);

mButton *mButtonCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *text);

mButton *mButtonNew(int size,mWidget *parent,uint32_t style);
void mButtonSetText(mButton *p,const char *text);
void mButtonSetPress(mButton *p,mBool press);
mBool mButtonIsPressed(mButton *p);

void mButtonDrawBase(mButton *p,mPixbuf *pixbuf,mBool pressed);

#ifdef __cplusplus
}
#endif

#endif
