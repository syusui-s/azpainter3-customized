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

#ifndef MLK_IMGBUTTON_H
#define MLK_IMGBUTTON_H

#include "mlk_button.h"

#define MLK_IMGBUTTON(p)  ((mImgButton *)(p))
#define MLK_IMGBUTTON_DEF MLK_BUTTON_DEF mImgButtonData bib;

typedef struct
{
	const uint8_t *imgbuf;
	int type,
		imgw,
		imgh;
}mImgButtonData;

struct _mImgButton
{
	MLK_BUTTON_DEF
	mImgButtonData bib;
};


enum MIMGBUTTON_TYPE
{
	MIMGBUTTON_TYPE_1BIT_TP_TEXT,
	MIMGBUTTON_TYPE_1BIT_WHITE_BLACK,
	MIMGBUTTON_TYPE_2BIT_BLACK_TP_WHITE
};


#ifdef __cplusplus
extern "C" {
#endif

mImgButton *mImgButtonNew(mWidget *parent,int size,uint32_t fstyle);
mImgButton *mImgButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mImgButtonDestroy(mWidget *p);
void mImgButtonHandle_calcHint(mWidget *p);
void mImgButtonHandle_draw(mWidget *p,mPixbuf *pixbuf);

void mImgButton_setBitImage(mImgButton *p,int type,const uint8_t *img,int width,int height);

#ifdef __cplusplus
}
#endif

#endif
