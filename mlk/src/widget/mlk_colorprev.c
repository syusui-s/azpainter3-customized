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

/*****************************************
 * mColorPrev
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_colorprev.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"


/**@ データ削除 */

void mColorPrevDestroy(mWidget *p)
{

}

/**@ 作成 */

mColorPrev *mColorPrevNew(mWidget *parent,int size,uint32_t fstyle)
{
	mColorPrev *p;
	
	if(size < sizeof(mColorPrev))
		size = sizeof(mColorPrev);
	
	p = (mColorPrev *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = mColorPrevHandle_draw;
	
	p->cp.fstyle = fstyle;
	p->cp.rgbcol = 0xffffff;
	
	return p;
}

/**@ 作成
 *
 * @p:w,h 推奨サイズにセットされる */

mColorPrev *mColorPrevCreate(mWidget *parent,
	int w,int h,uint32_t margin_pack,uint32_t fstyle,mRgbCol col)
{
	mColorPrev *p;

	p = mColorPrevNew(parent, 0, fstyle);
	if(!p) return NULL;

	p->wg.hintW = w;
	p->wg.hintH = h;

	mWidgetSetMargin_pack4(MLK_WIDGET(p), margin_pack);

	p->cp.rgbcol = col;

	return p;
}

/**@ RGB 色セット */

void mColorPrevSetColor(mColorPrev *p,mRgbCol col)
{
	col &= 0xffffff;

	if(col != p->cp.rgbcol)
	{
		p->cp.rgbcol = col;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ draw ハンドラ関数 */

void mColorPrevHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mColorPrev *p = MLK_COLORPREV(wg);
	mPixCol col;
	int w,h;

	w = wg->w;
	h = wg->h;
	col = mRGBtoPix(p->cp.rgbcol);

	if(p->cp.fstyle & MCOLORPREV_S_FRAME)
	{
		//枠付き
		
		mPixbufBox(pixbuf, 0, 0, w, h, MGUICOL_PIX(FRAME));
		mPixbufDraw3DFrame(pixbuf, 1, 1, w - 2, h - 2, mRGBtoPix(0x404040), MGUICOL_PIX(WHITE));
		mPixbufFillBox(pixbuf, 2, 2, w - 4, h - 4, col);
	}
	else
		mPixbufFillBox(pixbuf, 0, 0, w, h, col);
}

