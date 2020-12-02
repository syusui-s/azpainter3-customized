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

#ifndef MLIB_COLORBUTTON_H
#define MLIB_COLORBUTTON_H

#include "mButton.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_COLORBUTTON(p)  ((mColorButton *)(p))

typedef struct
{
	uint32_t style;
	mRgbCol col;
}mColorButtonData;

typedef struct _mColorButton
{
	mWidget wg;
	mButtonData btt;
	mColorButtonData cb;
}mColorButton;

enum MCOLORBUTTON_STYLE
{
	MCOLORBUTTON_S_DIALOG = 1<<0
};

enum MCOLORBUTTON_NOTIFY
{
	MCOLORBUTTON_N_PRESS
};


void mColorButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);

mColorButton *mColorButtonNew(int size,mWidget *parent,uint32_t style);
mColorButton *mColorButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4,mRgbCol col);

mRgbCol mColorButtonGetColor(mColorButton *p);
void mColorButtonSetColor(mColorButton *p,mRgbCol col);

#ifdef __cplusplus
}
#endif

#endif
