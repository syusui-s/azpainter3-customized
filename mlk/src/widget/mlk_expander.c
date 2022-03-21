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
 * mExpander
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_expander.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_util.h"

#include "mlk_pv_widget.h"


//----------------

#define _SEP_HEIGHT 2  //セパレータの高さ
#define _TEXT_LEFT  14 //テキストの左位置

//----------------


/* 展開状態の変更時 */

static void _change_expand(mExpander *p)
{
	if(p->wg.fui & MWIDGET_UI_LAYOUTED)
	{
		//自身の推奨サイズを再計算
	
		mWidgetSetRecalcHint(MLK_WIDGET(p));
	
		//通知
		//[!] 再レイアウトは、サイズ変更による最上位の親で実行する必要があるため、
		//    通知を受けた側が実行する。

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, MEXPANDER_N_TOGGLE, p->exp.expanded, 0);
	}
}


//=============================
// main
//=============================


/**@ mExpander データ解放 */

void mExpanderDestroy(mWidget *wg)
{
	mExpander *p = MLK_EXPANDER(wg);

	if(p->exp.fstyle & MEXPANDER_S_COPYTEXT)
		mFree(MLK_EXPANDER(p)->exp.text);
}

/**@ 作成 */

mExpander *mExpanderNew(mWidget *parent,int size,uint32_t fstyle)
{
	mExpander *p;
	
	if(size < sizeof(mExpander))
		size = sizeof(mExpander);
	
	p = (mExpander *)mContainerNew(parent, size);
	if(!p) return NULL;

	p->wg.destroy = mExpanderDestroy;
	p->wg.calc_hint = mExpanderHandle_calcHint;
	p->wg.draw = mExpanderHandle_draw;
	p->wg.event = mExpanderHandle_event;
	
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->exp.fstyle = fstyle;
	
	return p;
}

/**@ 作成 */

mExpander *mExpanderCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text)
{
	mExpander *p;

	p = mExpanderNew(parent, 0, fstyle);
	if(p)
	{
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

		mExpanderSetText(p, text);
	}

	return p;
}

/**@ ヘッダテキストセット */

void mExpanderSetText(mExpander *p,const char *text)
{
	if(p->exp.fstyle & MEXPANDER_S_COPYTEXT)
	{
		mFree(p->exp.text);

		p->exp.text = mStrdup(text);
	}
	else
		p->exp.text = (char *)text;
}

/**@ 中身の余白をセット
 *
 * @p:pack 上位バイトから、left,top,right,bottom の順 */

void mExpanderSetPadding_pack4(mExpander *p,uint32_t pack)
{
	__mWidgetRectSetPack4(&p->exp.rcpad, pack);
}

/**@ 展開の状態を変更
 *
 * @p:type 0=閉じる、正=展開、負=切り替え */

void mExpanderToggle(mExpander *p,int type)
{
	if(mIsChangeFlagState(type, p->exp.expanded))
	{
		p->exp.expanded ^= 1;

		_change_expand(p);
	}
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mExpanderHandle_calcHint(mWidget *wg)
{
	mExpander *p = MLK_EXPANDER(wg);
	mFont *font = mWidgetGetFont(wg);
	mWidgetRect *pad;
	int w,fonth;

	fonth = mFontGetHeight(font);

	pad = &p->ct.padding;

	//ヘッダ高さ

	p->exp.header_height = (fonth < 7)? 7: fonth;

	//コンテナ余白 (ヘッダ部分は余白としてセット)

	mMemset0(pad, sizeof(mWidgetRect));

	pad->top = p->exp.header_height;

	if(p->exp.fstyle & MEXPANDER_S_SEP_TOP)
		pad->top += _SEP_HEIGHT;

	//hintW/H

	if(p->exp.expanded)
	{
		//展開状態
		
		pad->left   = p->exp.rcpad.left;
		pad->top   += p->exp.rcpad.top;
		pad->right  = p->exp.rcpad.right;
		pad->bottom = p->exp.rcpad.bottom;

		(p->ct.calc_hint)(wg);
	}
	else
	{
		//閉じている時

		wg->hintW = 1;
		wg->hintH = pad->top;
	}

	//ヘッダテキスト幅を適用

	w = mFontGetTextWidth(font, p->exp.text, -1) + _TEXT_LEFT;

	if(w > wg->hintW) wg->hintW = w;
}

/**@ event ハンドラ関数 */

int mExpanderHandle_event(mWidget *wg,mEvent *ev)
{
	mExpander *p = MLK_EXPANDER(wg);

	switch(ev->type)
	{
		//ヘッダ上を左クリック時、展開状態を切り替え
		case MEVENT_POINTER:
			if((ev->pt.act == MEVENT_POINTER_ACT_PRESS || ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
				&& ev->pt.btt == MLK_BTT_LEFT
				&& ev->pt.y < p->exp.header_height)
			{
				p->exp.expanded ^= 1;

				_change_expand(p);
			}
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mExpanderHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mExpander *p = MLK_EXPANDER(wg);
	mFont *font = mWidgetGetFont(wg);
	int y = 0,enable;
	uint32_t col;

	enable = mWidgetIsEnable(wg);

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//上の境界線

	if(p->exp.fstyle & MEXPANDER_S_SEP_TOP)
	{
		mPixbufLineH(pixbuf, 0, 0, wg->w, MGUICOL_PIX(FRAME));
		y = _SEP_HEIGHT;
	}

	//ヘッダ背景

	if(p->exp.fstyle & MEXPANDER_S_DARK)
		mPixbufFillBox(pixbuf, 0, y, wg->w, p->exp.header_height, MGUICOL_PIX(FACE_DARK));

	//ヘッダテキスト

	mFontDrawText_pixbuf(font, pixbuf, _TEXT_LEFT, y, p->exp.text, -1,
		mGuiCol_getRGB(enable? MGUICOL_TEXT: MGUICOL_TEXT_DISABLE));

	//矢印 (7x4)

	y += (p->exp.header_height - 7) >> 1;

	col = mGuiCol_getPix(enable? MGUICOL_TEXT: MGUICOL_TEXT_DISABLE);

	if(p->exp.expanded)
		mPixbufDrawArrowDown(pixbuf, 3, y + 2, 4, col);
	else
		mPixbufDrawArrowRight(pixbuf, 5, y, 4, col);
}

