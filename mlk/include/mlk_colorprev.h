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

#ifndef MLK_COLORPREV_H
#define MLK_COLORPREV_H

#define MLK_COLORPREV(p)  ((mColorPrev *)(p))
#define MLK_COLORPREV_DEF mWidget wg; mColorPrevData cp;

typedef struct
{
	uint32_t fstyle,rgbcol;
}mColorPrevData;

struct _mColorPrev
{
	mWidget wg;
	mColorPrevData cp;
};

enum MCOLORPREV_STYLE
{
	MCOLORPREV_S_FRAME = 1<<0
};


#ifdef __cplusplus
extern "C" {
#endif

mColorPrev *mColorPrevNew(mWidget *parent,int size,uint32_t fstyle);
mColorPrev *mColorPrevCreate(mWidget *parent,int w,int h,uint32_t margin_pack,uint32_t fstyle,mRgbCol col);
void mColorPrevDestroy(mWidget *p);

void mColorPrevHandle_draw(mWidget *p,mPixbuf *pixbuf);

void mColorPrevSetColor(mColorPrev *p,mRgbCol col);

#ifdef __cplusplus
}
#endif

#endif
