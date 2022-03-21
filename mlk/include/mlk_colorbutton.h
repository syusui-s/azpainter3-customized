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

#ifndef MLK_COLORBUTTON_H
#define MLK_COLORBUTTON_H

#include "mlk_button.h"

#define MLK_COLORBUTTON(p)  ((mColorButton *)(p))
#define MLK_COLORBUTTON_DEF MLK_BUTTON_DEF mColorButtonData colbtt;

typedef struct
{
	uint32_t fstyle;
	mRgbCol col;
}mColorButtonData;

struct _mColorButton
{
	MLK_BUTTON_DEF
	mColorButtonData colbtt;
};

enum MCOLORBUTTON_STYLE
{
	MCOLORBUTTON_S_DIALOG = 1<<0
};

enum MCOLORBUTTON_NOTIFY
{
	MCOLORBUTTON_N_PRESS
};


#ifdef __cplusplus
extern "C" {
#endif

mColorButton *mColorButtonNew(mWidget *parent,int size,uint32_t fstyle);
mColorButton *mColorButtonCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,mRgbCol col);
void mColorButtonDestroy(mWidget *p);

void mColorButtonHandle_draw(mWidget *p,mPixbuf *pixbuf);

mRgbCol mColorButtonGetColor(mColorButton *p);
void mColorButtonSetColor(mColorButton *p,mRgbCol col);

#ifdef __cplusplus
}
#endif

#endif
