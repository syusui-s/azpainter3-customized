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

#ifndef MLIB_BITIMGBUTTON_H
#define MLIB_BITIMGBUTTON_H

#include "mButton.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_BITIMGBUTTON(p)  ((mBitImgButton *)(p))

typedef struct
{
	const uint8_t *imgpat;
	int type,width,height;
}mBitImgButtonData;

typedef struct _mBitImgButton
{
	mWidget wg;
	mButtonData btt;
	mBitImgButtonData bib;
}mBitImgButton;


enum MBITIMGBUTTON_TYPE
{
	MBITIMGBUTTON_TYPE_1BIT_BLACK_WHITE,
	MBITIMGBUTTON_TYPE_1BIT_TP_BLACK,
	MBITIMGBUTTON_TYPE_2BIT_TP_BLACK_WHITE
};


void mBitImgButtonCalcHintHandle(mWidget *p);
void mBitImgButtonDrawHandle(mWidget *p,mPixbuf *pixbuf);

mBitImgButton *mBitImgButtonNew(int size,mWidget *parent,uint32_t style);
mBitImgButton *mBitImgButtonCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

void mBitImgButtonSetImage(mBitImgButton *p,int type,const uint8_t *imgpat,int width,int height);

#ifdef __cplusplus
}
#endif

#endif
