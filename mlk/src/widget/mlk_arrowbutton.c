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
 * mArrowButton
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_button.h"
#include "mlk_arrowbutton.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "mlk_pv_widget.h"


/**@ データの解放 */

void mArrowButtonDestroy(mWidget *p)
{
	mButtonDestroy(p);
}

/**@ 作成 */

mArrowButton *mArrowButtonNew(mWidget *parent,int size,uint32_t fstyle)
{
	mArrowButton *p;
	
	if(size < sizeof(mArrowButton))
		size = sizeof(mArrowButton);
	
	p = (mArrowButton *)mButtonNew(parent, size,
		(fstyle & MARROWBUTTON_S_DIRECT_PRESS)? MBUTTON_S_DIRECT_PRESS: 0);

	if(!p) return NULL;
	
	p->wg.draw = mArrowButtonHandle_draw;
	p->wg.calc_hint = mArrowButtonHandle_calcHint;
	
	p->arrbtt.fstyle = fstyle;
	
	return p;
}

/**@ 作成 */

mArrowButton *mArrowButtonCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mArrowButton *p;

	p = mArrowButtonNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ 作成 (最小サイズ指定) */

mArrowButton *mArrowButtonCreate_minsize(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,int size)
{
	mArrowButton *p;

	p = mArrowButtonCreate(parent, id, flayout, margin_pack, fstyle);

	if(p)
		p->wg.hintMinW = p->wg.hintMinH = size;

	return p;
}

/**@ calc_hint ハンドラ関数 */

void mArrowButtonHandle_calcHint(mWidget *wg)
{
	mArrowButton *p = MLK_ARROWBUTTON(wg);
	int n;

	if(p->arrbtt.fstyle & MARROWBUTTON_S_FONTSIZE)
		//フォントサイズに合わせる
		n = mWidgetGetFontHeight(wg) + 6;
	else
		//固定
		n = 19;

	wg->hintW = wg->hintH = n;
}

/**@ draw ハンドラ関数 */

void mArrowButtonHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mArrowButton *p = MLK_ARROWBUTTON(wg);
	int w,h,size,lsize,add;
	uint32_t style,col;

	//ボタンのベースを描画
	
	mButtonDrawBase(MLK_BUTTON(p), pixbuf);

	//矢印

	w = wg->w;
	h = wg->h;

	if(w < 19)
		size = 3;
	else if(w < 21)
		size = 4;
	else
		size = 5;

	lsize = size * 2 - 1;

	add = ((p->btt.flags & MBUTTON_F_PRESSED) != 0);
	style = p->arrbtt.fstyle;
	col = (mWidgetIsEnable(wg))? MGUICOL_PIX(TEXT): MGUICOL_PIX(TEXT_DISABLE);

	if(style & MARROWBUTTON_S_DOWN)
		//down
		mPixbufDrawArrowDown(pixbuf, (w - lsize) / 2 + add, (h - size) / 2 + add, size, col);
	else if(style & MARROWBUTTON_S_UP)
		//up
		mPixbufDrawArrowUp(pixbuf, (w - lsize) / 2 + add, (h - size) / 2 + add, size, col);
	else if(style & MARROWBUTTON_S_LEFT)
		//left
		mPixbufDrawArrowLeft(pixbuf, (w - size) / 2 + add, (h - lsize) / 2 + add, size, col);
	else
		//right
		mPixbufDrawArrowRight(pixbuf, (w - size) / 2 + add, (h - lsize) / 2 + add, size, col);
}

