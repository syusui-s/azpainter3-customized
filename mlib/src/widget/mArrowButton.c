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
 * mArrowButton
 *****************************************/

#include "mDef.h"

#include "mArrowButton.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mSysCol.h"


/*****************//**

@defgroup arrowbutton mArrowButton
@brief 矢印ボタン
@ingroup group_widget

<h3>継承</h3>
mWidget \> mButton \> mArrowButton

@{
@file mArrowButton.h
@struct mArrowButtonData
@struct mArrowButton
@enum MARROWBUTTON_STYLE

@def M_ARROWBUTTON(p)

@val MARROWBUTTON_STYLE::MARROWBUTTON_S_FONTSIZE
フォントサイズに合わせたサイズにする

***********************/


/** 作成 */

mArrowButton *mArrowButtonNew(int size,mWidget *parent,uint32_t style)
{
	mArrowButton *p;
	
	if(size < sizeof(mArrowButton)) size = sizeof(mArrowButton);
	
	p = (mArrowButton *)mButtonNew(size, parent, 0);
	if(!p) return NULL;
	
	p->wg.draw = mArrowButtonDrawHandle;
	p->wg.calcHint = mArrowButtonCalcHintHandle;
	
	p->abtt.style = style;
	
	return p;
}

/** 作成 */

mArrowButton *mArrowButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4)
{
	mArrowButton *p;

	p = mArrowButtonNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 推奨サイズ計算 */

void mArrowButtonCalcHintHandle(mWidget *wg)
{
	mArrowButton *p = M_ARROWBUTTON(wg);
	int n;

	if(!(p->abtt.style & MARROWBUTTON_S_FONTSIZE))
		n = 19;
	else
	{
		n = mWidgetGetFontHeight(wg) + 8;
		if(!(n & 1)) n++;
	}

	wg->hintW = wg->hintH = n;
}

/** 描画 */

void mArrowButtonDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mArrowButton *p = M_ARROWBUTTON(wg);
	uint32_t type,col;
	int x,y;
	mBool press;

	press = mButtonIsPressed(M_BUTTON(p));

	//ボタン
	
	mButtonDrawBase(M_BUTTON(p), pixbuf, press);

	//矢印

	x = wg->w / 2 + press;
	y = wg->h / 2 + press;
	col = (wg->fState & MWIDGET_STATE_ENABLED)? MSYSCOL(TEXT): MSYSCOL(TEXT_DISABLE);

	type = (p->abtt.style & (MARROWBUTTON_S_LEFT | MARROWBUTTON_S_RIGHT | MARROWBUTTON_S_UP));

	if(wg->w > 21)
	{
		//大きい

		switch(type)
		{
			case MARROWBUTTON_S_UP:
				mPixbufDrawArrowUp_13x7(pixbuf, x, y, col);
				break;
			case MARROWBUTTON_S_LEFT:
				mPixbufDrawArrowLeft_7x13(pixbuf, x, y, col);
				break;
			case MARROWBUTTON_S_RIGHT:
				mPixbufDrawArrowRight_7x13(pixbuf, x, y, col);
				break;
			default:
				mPixbufDrawArrowDown_13x7(pixbuf, x, y, col);
				break;
		}
	}
	else
	{
		//通常
		
		switch(type)
		{
			case MARROWBUTTON_S_UP:
				mPixbufDrawArrowUp(pixbuf, x, y, col);
				break;
			case MARROWBUTTON_S_LEFT:
				mPixbufDrawArrowLeft(pixbuf, x, y, col);
				break;
			case MARROWBUTTON_S_RIGHT:
				mPixbufDrawArrowRight(pixbuf, x, y, col);
				break;
			default:
				mPixbufDrawArrowDown(pixbuf, x, y, col);
				break;
		}
	}
}

/** @} */
