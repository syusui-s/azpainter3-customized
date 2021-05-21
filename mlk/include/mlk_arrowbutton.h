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

#ifndef MLK_ARROWBUTTON_H
#define MLK_ARROWBUTTON_H

#include "mlk_button.h"

#define MLK_ARROWBUTTON(p)  ((mArrowButton *)(p))
#define MLK_ARROWBUTTON_DEF MLK_BUTTON_DEF mArrowButtonData arrbtt;

typedef struct
{
	uint32_t fstyle;
}mArrowButtonData;

struct _mArrowButton
{
	MLK_BUTTON_DEF
	mArrowButtonData arrbtt;
};

enum MARROWBUTTON_STYLE
{
	MARROWBUTTON_S_DOWN  = 1<<0,
	MARROWBUTTON_S_UP    = 1<<1,
	MARROWBUTTON_S_LEFT  = 1<<2,
	MARROWBUTTON_S_RIGHT = 1<<3,
	MARROWBUTTON_S_FONTSIZE = 1<<4,
	MARROWBUTTON_S_DIRECT_PRESS = 1<<5
};


#ifdef __cplusplus
extern "C" {
#endif

mArrowButton *mArrowButtonNew(mWidget *parent,int size,uint32_t fstyle);
mArrowButton *mArrowButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);
mArrowButton *mArrowButtonCreate_minsize(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,int size);

void mArrowButtonDestroy(mWidget *p);

void mArrowButtonHandle_calcHint(mWidget *p);
void mArrowButtonHandle_draw(mWidget *p,mPixbuf *pixbuf);

#ifdef __cplusplus
}
#endif

#endif
