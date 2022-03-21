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
 * PrevTileImage
 *
 * TileImage プレビューウィジェット
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "tileimage.h"


//----------------------

typedef struct _PrevTileImage
{
	mWidget wg;

	TileImage *img;
	mSize size;
}PrevTileImage;

//----------------------


/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	PrevTileImage *p = (PrevTileImage *)wg;

	//枠

	mPixbufDraw3DFrame(pixbuf, 0, 0, wg->w, wg->h, mRGBtoPix(0x828282), MGUICOL_PIX(WHITE));

	//プレビュー

	if(p->img)
		TileImage_drawPreview(p->img, pixbuf, 1, 1, wg->w - 2, wg->h - 2, p->size.w, p->size.h);
	else
		mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, mRGBtoPix(0xd2d2d2));
}

/** 作成 */

PrevTileImage *PrevTileImage_new(mWidget *parent,int w,int h)
{
	PrevTileImage *p;

	p = (PrevTileImage *)mWidgetNew(parent, sizeof(PrevTileImage));

	p->wg.flayout = MLF_FIX_WH;
	p->wg.w = w;
	p->wg.h = h;
	p->wg.draw = _draw_handle;

	return p;
}

/** 画像をセット */

void PrevTileImage_setImage(PrevTileImage *p,TileImage *img,int w,int h)
{
	p->img = img;
	p->size.w = w;
	p->size.h = h;
	
	mWidgetRedraw(MLK_WIDGET(p));
}

