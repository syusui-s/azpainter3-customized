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

#ifndef MLIB_ARROWBUTTON_H
#define MLIB_ARROWBUTTON_H

#include "mButton.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_ARROWBUTTON(p)  ((mArrowButton *)(p))

typedef struct
{
	uint32_t style;
}mArrowButtonData;

typedef struct _mArrowButton
{
	mWidget wg;
	mButtonData btt;
	mArrowButtonData abtt;
}mArrowButton;

enum MARROWBUTTON_STYLE
{
	MARROWBUTTON_S_DOWN  = 0,
	MARROWBUTTON_S_UP    = 1<<0,
	MARROWBUTTON_S_LEFT  = 1<<1,
	MARROWBUTTON_S_RIGHT = 1<<2,
	MARROWBUTTON_S_FONTSIZE = 1<<3
};


void mArrowButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);
void mArrowButtonCalcHintHandle(mWidget *p);

mArrowButton *mArrowButtonNew(int size,mWidget *parent,uint32_t style);
mArrowButton *mArrowButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4);

#ifdef __cplusplus
}
#endif

#endif
