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

/*****************************************
 * ImgMatPrev
 * (ImageMaterial のプレビュー)
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"

#include "imagematerial.h"


//------------------------

typedef struct _ImgMatPrev
{
	mWidget wg;

	ImageMaterial *img;
}ImgMatPrev;

//------------------------


/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ImgMatPrev *p = (ImgMatPrev *)wg;
	int w,h;

	w = wg->w;
	h = wg->h;

	//枠
	
	mPixbufBox(pixbuf, 0, 0, w, h, 0);

	w -= 2;
	h -= 2;

	//イメージ

	if(!p->img)
		//なし
		mPixbufFillBox(pixbuf, 1, 1, w, h, mRGBtoPix(0xcccccc));
	else
		ImageMaterial_drawPreview(p->img, pixbuf, 1, 1, w, h);
}

/** 作成
 *
 * w,h: 推奨サイズ */

ImgMatPrev *ImgMatPrev_new(mWidget *parent,int w,int h,uint32_t flayout)
{
	ImgMatPrev *p;
	
	p = (ImgMatPrev *)mWidgetNew(parent, sizeof(ImgMatPrev));
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;

	p->wg.flayout = flayout;
	p->wg.hintW = w;
	p->wg.hintH = h;
	
	return p;
}

/** イメージをセット
 *
 * img: NULL でなし (灰色で塗りつぶされる) */

void ImgMatPrev_setImage(ImgMatPrev *p,ImageMaterial *img)
{
	if(!img && !p->img) return;
	
	p->img = img;

	mWidgetRedraw(MLK_WIDGET(p));
}

