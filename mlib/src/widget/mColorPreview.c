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

/*****************************************
 * mColorPreview
 *****************************************/

#include "mDef.h"

#include "mColorPreview.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"


/**
@defgroup colorpreview mColorPreview
@brief カラープレビュー
@ingroup group_widget

<h3>継承</h3>
mWidget \> mColorPreview

@{

@file mColorPreview.h
@def M_COLORPREVIEW(p)
@struct mColorPreviewData
@struct mColorPreview
@enum MCOLORPREVIEW_STYLE

@var MCOLORPREVIEW_STYLE::MCOLORPREVIEW_S_FRAME
枠を付ける
*/


/** 作成 */

mColorPreview *mColorPreviewCreate(mWidget *parent,uint32_t style,int w,int h,uint32_t marginB4)
{
	mColorPreview *p;

	p = mColorPreviewNew(0, parent, style);
	if(!p) return NULL;

	p->wg.hintW = w;
	p->wg.hintH = h;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mColorPreview *mColorPreviewNew(int size,mWidget *parent,uint32_t style)
{
	mColorPreview *p;
	
	if(size < sizeof(mColorPreview)) size = sizeof(mColorPreview);
	
	p = (mColorPreview *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.draw = mColorPreviewDrawHandle;
	
	p->cp.style = style;
	p->cp.colrgb = 0xffffff;
	
	return p;
}

/** 色セット */

void mColorPreviewSetColor(mColorPreview *p,mRgbCol col)
{
	col &= 0xffffff;

	if(col != p->cp.colrgb)
	{
		p->cp.colrgb = col;

		mWidgetUpdate(M_WIDGET(p));
	}
}

/** 描画 */

void mColorPreviewDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mColorPreview *p = M_COLORPREVIEW(wg);
	uint32_t col;

	col = mRGBtoPix(p->cp.colrgb);

	if(p->cp.style & MCOLORPREVIEW_S_FRAME)
	{
		//枠つき
		
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, 0);
		mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, col);
	}
	else
		mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, col);
}

/** @} */
