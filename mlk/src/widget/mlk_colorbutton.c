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
 * mColorButton
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_button.h"
#include "mlk_colorbutton.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_sysdlg.h"

#include "mlk_pv_widget.h"



/** mButton:pressed ハンドラ */

static void _pressed_handle(mWidget *wg)
{
	mColorButton *p = MLK_COLORBUTTON(wg);

	//色選択ダイアログ

	if(p->colbtt.fstyle & MCOLORBUTTON_S_DIALOG)
	{
		if(!mSysDlg_selectColor(wg->toplevel, &p->colbtt.col))
			return;
	}

	//通知
	
	mWidgetEventAdd_notify(wg, NULL,
		MCOLORBUTTON_N_PRESS, p->colbtt.col, 0);
}


//=========================


/**@ データ解放 */

void mColorButtonDestroy(mWidget *p)
{
	mButtonDestroy(p);
}

/**@ 作成 */

mColorButton *mColorButtonNew(mWidget *parent,int size,uint32_t fstyle)
{
	mColorButton *p;
	
	if(size < sizeof(mColorButton))
		size = sizeof(mColorButton);
	
	p = (mColorButton *)mButtonNew(parent, size, 0);
	if(!p) return NULL;
	
	p->wg.draw = mColorButtonHandle_draw;
	p->wg.calc_hint = NULL;
	p->wg.hintW = 60;
	p->wg.hintH = 25;

	p->btt.pressed = _pressed_handle;

	p->colbtt.fstyle = fstyle;
	
	return p;
}

/**@ 作成 */

mColorButton *mColorButtonCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,mRgbCol col)
{
	mColorButton *p;

	p = mColorButtonNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	p->colbtt.col = col & 0xffffff;

	return p;
}

/**@ 色を取得 */

mRgbCol mColorButtonGetColor(mColorButton *p)
{
	return p->colbtt.col;
}

/**@ 色をセット */

void mColorButtonSetColor(mColorButton *p,mRgbCol col)
{
	col &= 0xffffff;

	if(p->colbtt.col != col)
	{
		p->colbtt.col = col;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ draw ハンドラ関数 */

void mColorButtonHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mColorButton *p = MLK_COLORBUTTON(wg);
	int press;

	//ボタン

	mButtonDrawBase(MLK_BUTTON(p), pixbuf);

	//色

	if(!mWidgetIsEnable(wg))
		//無効時は枠のみ
		mPixbufBox(pixbuf, 4, 4, wg->w - 8, wg->h - 8, MGUICOL_PIX(FRAME_DISABLE));
	else
	{
		press = mButtonIsPressed(MLK_BUTTON(p));

		//枠
		mPixbufBox(pixbuf, 4 + press, 4 + press, wg->w - 8, wg->h - 8, 0);

		//色
		mPixbufFillBox(pixbuf, 5 + press, 5 + press, wg->w - 10, wg->h - 10,
			mRGBtoPix(p->colbtt.col));
	}
}

