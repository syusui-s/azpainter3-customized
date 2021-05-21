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
 * mLabel [ラベル]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_label.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_string.h"

#include "mlk_pv_widget.h"


//--------------------

//枠線ありの場合の余白
#define _BORDER_SPACE_X  3
#define _BORDER_SPACE_Y  2

//--------------------


/* テキスト変更時 */

static void _change_text(mLabel *p)
{
	if(p->wg.fui & MWIDGET_UI_LAYOUTED)
		mWidgetSetRecalcHint(MLK_WIDGET(p));
}


/**@ ラベルのデータ解放 */

void mLabelDestroy(mWidget *p)
{
	mWidgetLabelText_free(&MLK_LABEL(p)->lb.txt);
}

/**@ ラベルウィジェット作成 */

mLabel *mLabelNew(mWidget *parent,int size,uint32_t fstyle)
{
	mLabel *p;

	if(size < sizeof(mLabel))
		size = sizeof(mLabel);
	
	p = (mLabel *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mLabelDestroy;
	p->wg.calc_hint = mLabelHandle_calcHint;
	p->wg.draw = mLabelHandle_draw;

	p->lb.fstyle = fstyle;
	
	return p;
}

/**@ ラベル作成
 *
 * @p:margin_pack レイアウトの余白を、4つパックで指定する。*/

mLabel *mLabelCreate(mWidget *parent,uint32_t flayout,uint32_t margin_pack,
	uint32_t fstyle,const char *text)
{
	mLabel *p;

	p = mLabelNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), 0, flayout, margin_pack);

	mWidgetLabelText_set(MLK_WIDGET(p), &p->lb.txt, text,
		fstyle & MLABEL_S_COPYTEXT);

	return p;
}

/**@ テキストセット
 *
 * @d:テキストを複製するかは、スタイルフラグによる。\
 *  {em:※レイアウトが推奨サイズの幅・高さの場合、再レイアウトが必要。\
 *  また、右寄せ・中央寄せの場合も、推奨サイズの再計算が必要となる。:em} */

void mLabelSetText(mLabel *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->lb.txt, text,
		p->lb.fstyle & MLABEL_S_COPYTEXT);

	_change_text(p);
}

/**@ テキストをセット (複製)
 *
 * @d:スタイルフラグは変更しない。 */

void mLabelSetText_copy(mLabel *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->lb.txt, text, TRUE);

	_change_text(p);
}

/**@ int 値からテキストセット
 *
 * @d:テキストは常に複製。 */

void mLabelSetText_int(mLabel *p,int val)
{
	char m[32];

	mIntToStr(m, val);
	mWidgetLabelText_set(MLK_WIDGET(p), &p->lb.txt, m, TRUE);

	_change_text(p);
}

/**@ int 値から浮動小数点数テキストセット
 *
 * @d:テキストは常に複製。
 * 
 * @p:dig 小数点以下の桁数。\
 *  1 の場合、10 = 1.0 となる。 */

void mLabelSetText_floatint(mLabel *p,int val,int dig)
{
	char m[32];

	mIntToStr_float(m, val, dig);
	mWidgetLabelText_set(MLK_WIDGET(p), &p->lb.txt, m, TRUE);

	_change_text(p);
}


//==========================
// ハンドラ
//==========================


/**@ calc_hint ハンドラ関数 */

void mLabelHandle_calcHint(mWidget *wg)
{
	mLabel *p = MLK_LABEL(wg);
	mSize size;

	mWidgetLabelText_onCalcHint(wg, &p->lb.txt, &size);
		
	if(p->lb.fstyle & MLABEL_S_BORDER)
	{
		size.w += _BORDER_SPACE_X * 2;
		size.h += _BORDER_SPACE_Y * 2;
	}

	wg->hintW = size.w;
	wg->hintH = size.h;
}

/**@ draw ハンドラ関数 */

void mLabelHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mLabel *p = MLK_LABEL(wg);
	int spx,spy,y,w,h,flags;

	//背景
	
	mWidgetDrawBkgnd(wg, NULL);

	//余白
	
	if(p->lb.fstyle & MLABEL_S_BORDER)
		spx = _BORDER_SPACE_X, spy = _BORDER_SPACE_Y;
	else
		spx = spy = 0;

	//テキスト領域のサイズ

	w = wg->w - spx * 2;
	h = wg->h - spy * 2;

	//テキスト Y 位置
	
	if(p->lb.fstyle & MLABEL_S_BOTTOM)
		y = h - p->lb.txt.szfull.h;
	else if(p->lb.fstyle & MLABEL_S_MIDDLE)
		y = (h - p->lb.txt.szfull.h) / 2;
	else
		y = 0;
	
	//テキスト

	if(p->lb.fstyle & MLABEL_S_CENTER)
		flags = MWIDGETLABELTEXT_DRAW_F_CENTER;
	else if(p->lb.fstyle & MLABEL_S_RIGHT)
		flags = MWIDGETLABELTEXT_DRAW_F_RIGHT;
	else
		flags = 0;
	
	mWidgetLabelText_draw(&p->lb.txt, pixbuf, mWidgetGetFont(wg),
		spx, spy + y, w, MGUICOL_RGB(TEXT), flags);
	
	//枠
	
	if(p->lb.fstyle & MLABEL_S_BORDER)
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FRAME_BOX));
}

