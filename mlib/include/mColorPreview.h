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

#ifndef MLIB_COLORPREVIEW_H
#define MLIB_COLORPREVIEW_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_COLORPREVIEW(p)  ((mColorPreview *)(p))

typedef struct
{
	uint32_t style,colrgb;
}mColorPreviewData;

typedef struct _mColorPreview
{
	mWidget wg;
	mColorPreviewData cp;
}mColorPreview;


enum MCOLORPREVIEW_STYLE
{
	MCOLORPREVIEW_S_FRAME = 1<<0
};


void mColorPreviewDrawHandle(mWidget *p,mPixbuf *pixbuf);

mColorPreview *mColorPreviewCreate(mWidget *parent,uint32_t style,int w,int h,uint32_t marginB4);

mColorPreview *mColorPreviewNew(int size,mWidget *parent,uint32_t style);

void mColorPreviewSetColor(mColorPreview *p,mRgbCol col);

#ifdef __cplusplus
}
#endif

#endif
