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
 * mImgButton
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_imgbutton.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "mlk_pv_widget.h"


//-------------

#define _FRAME_W  4

//-------------


/**@ mImgButton データ削除 */

void mImgButtonDestroy(mWidget *p)
{

}

/**@ 作成 */

mImgButton *mImgButtonNew(mWidget *parent,int size,uint32_t fstyle)
{
	mImgButton *p;
	
	if(size < sizeof(mImgButton))
		size = sizeof(mImgButton);
	
	p = (mImgButton *)mButtonNew(parent, size, 0);
	if(!p) return NULL;
	
	p->wg.calc_hint = mImgButtonHandle_calcHint;
	p->wg.draw = mImgButtonHandle_draw;

	return p;
}

/**@ 作成 */

mImgButton *mImgButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mImgButton *p;

	p = mImgButtonNew(parent, 0, fstyle);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ ビットイメージデータをセット
 *
 * @p:img イメージデータ。ポインタ値が直接セットされるため、常に参照可能な状態であること。
 * @p:width イメージの幅
 * @p:height イメージの高さ */

void mImgButton_setBitImage(mImgButton *p,int type,const uint8_t *img,int width,int height)
{
	p->bib.type = type;
	p->bib.imgbuf = img;
	p->bib.imgw = width;
	p->bib.imgh = height;
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mImgButtonHandle_calcHint(mWidget *wg)
{
	mImgButton *p = MLK_IMGBUTTON(wg);

	wg->hintW = p->bib.imgw + _FRAME_W * 2;
	wg->hintH = p->bib.imgh + _FRAME_W * 2;
}

/**@ draw ハンドラ関数 */

void mImgButtonHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mImgButton *p = MLK_IMGBUTTON(wg);
	int x,y,iw,ih,press,fenable;
	uint32_t c,col[4];

	fenable = mWidgetIsEnable(wg);
	press = mButtonIsPressed(MLK_BUTTON(wg));

	//ボタンのベース描画

	mButtonDrawBase(MLK_BUTTON(wg), pixbuf);

	//イメージ描画

	if(p->bib.imgbuf)
	{
		iw = p->bib.imgw;
		ih = p->bib.imgh;
		
		x = ((wg->w - iw) >> 1) + press;
		y = ((wg->h - ih) >> 1) + press;

		//色

		if(p->bib.type != MIMGBUTTON_TYPE_1BIT_TP_TEXT)
		{
			if(fenable)
			{
				//有効時
				col[0] = 0;
				col[3] = MGUICOL_PIX(WHITE);
			}
			else
			{
				//無効時は、背景色とブレンド
				
				c = MGUICOL_RGB(FACE);
				col[0] = mRGBtoPix_blend256(0, c, 64);
				col[3] = mRGBtoPix_blend256(0xffffff, c, 64);
			}
		}

		//

		switch(p->bib.type)
		{
			//1bit (透過,テキスト色)
			case MIMGBUTTON_TYPE_1BIT_TP_TEXT:
				mPixbufDraw1bitPattern(pixbuf, x, y, p->bib.imgbuf, iw, ih, MPIXBUF_TPCOL,
					mGuiCol_getPix((fenable)? MGUICOL_TEXT: MGUICOL_TEXT_DISABLE));
				break;

			//1bit (白,黒)
			case MIMGBUTTON_TYPE_1BIT_WHITE_BLACK:
				mPixbufDraw1bitPattern(pixbuf, x, y, p->bib.imgbuf, iw, ih, col[3], col[0]);
				break;

			//2bit (黒,透過,透過,白)
			case MIMGBUTTON_TYPE_2BIT_BLACK_TP_WHITE:
				col[1] = col[2] = MPIXBUF_TPCOL;
				mPixbufDraw2bitPattern(pixbuf, x, y, p->bib.imgbuf, iw, ih, col);
				break;
		}
	}
}

