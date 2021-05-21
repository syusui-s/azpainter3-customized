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
 * mProgressBar [プログレスバー]
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_progressbar.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_guicol.h"

#include "mlk_pv_widget.h"


//------------------

#define _FRAME_W  2  //枠の幅 (余白含む)

//------------------



/**@ データ削除 */

void mProgressBarDestroy(mWidget *p)
{
	mFree(MLK_PROGRESSBAR(p)->pb.text);
}

/**@ 作成 */

mProgressBar *mProgressBarNew(mWidget *parent,int size,uint32_t fstyle)
{
	mProgressBar *p;
	
	if(size < sizeof(mProgressBar))
		size = sizeof(mProgressBar);
	
	p = (mProgressBar *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mProgressBarDestroy;
	p->wg.calc_hint = mProgressBarHandle_calcHint;
	p->wg.draw = mProgressBarHandle_draw;
	
	p->pb.fstyle = fstyle;
	p->pb.max = 100;
	
	return p;
}

/**@ 作成 */

mProgressBar *mProgressBarCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mProgressBar *p;

	p = mProgressBarNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ 値のステータスセット */

void mProgressBarSetStatus(mProgressBar *p,int min,int max,int pos)
{
	if(min >= max) max = min + 1;
	
	if(pos < min) pos = min;
	else if(pos > max) pos = max;
	
	p->pb.min = min;
	p->pb.max = max;
	p->pb.pos = pos;
	
	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 任意テキストセット */

void mProgressBarSetText(mProgressBar *p,const char *text)
{
	mFree(p->pb.text);
	
	p->pb.text = mStrdup(text);

	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 位置の値をセット
 *
 * @r:位置が変わったか */

mlkbool mProgressBarSetPos(mProgressBar *p,int pos)
{
	if(pos < p->pb.min)
		pos = p->pb.min;
	else if(pos > p->pb.max)
		pos = p->pb.max;
	
	if(pos == p->pb.pos)
		return FALSE;
	else
	{
		p->pb.pos = pos;
		mWidgetRedraw(MLK_WIDGET(p));
		return TRUE;
	}
}

/**@ 位置を+1 */

void mProgressBarIncPos(mProgressBar *p)
{
	if(p->pb.pos < p->pb.max)
	{
		p->pb.pos++;
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ 現在位置に指定数を加算 */

mlkbool mProgressBarAddPos(mProgressBar *p,int add)
{
	return mProgressBarSetPos(p, p->pb.pos + add);
}

/**@ サブステップ開始
 *
 * @d:プログレスバー上で、現在位置から num 分までの範囲を、
 * 0〜maxcnt - 1 までの仮想的な値の範囲に割り当てて、バーの位置を順に進める。
 * 
 * @p:num プログレスバー上での数
 * @p:maxcnt 仮想的な値の最大値 */

void mProgressBarSubStep_begin(mProgressBar *p,int num,int maxcnt)
{
	p->pb.sub_step = num;
	p->pb.sub_max  = maxcnt;
	p->pb.sub_toppos = p->pb.pos;
	p->pb.sub_curcnt = 0;
	p->pb.sub_curstep = 0;

	//次にプログレスを +1 する時の仮想値のカウント数
	// 最大値とステップ数が同じくらいの場合は、その都度計算する (-1 にセット)。
	// 最大値の方が大きい場合は、次ステップのカウント数を求めて比較。

	p->pb.sub_nextcnt = (maxcnt < num * 2)? -1: maxcnt / num;
}

/**@ サブステップ開始 (1ステップのみ)
 *
 * @d:仮想的な範囲を 0-1000、プログレスの範囲を 0-10 にして、バーの段階を小さくする場合など。\
 * 現在位置を 0、プログレスバーの最大値を num にして、サブステップ開始。
 *
 * @p:num プログレスバーの最大値
 * @p:maxnt 仮想的な値の最大値 */

void mProgressBarSubStep_begin_onestep(mProgressBar *p,int num,int maxcnt)
{
	p->pb.min = p->pb.pos = 0;
	p->pb.max = num;
	
	mProgressBarSubStep_begin(p, num, maxcnt);
}

/**@ サブステップのカウントを +1
 *
 * @r:位置が変化したか */

mlkbool mProgressBarSubStep_inc(mProgressBar *p)
{
	//最大値を超えていた場合は終了している (等しい場合は処理)

	if(p->pb.sub_curcnt > p->pb.sub_max)
		return FALSE;

	//+1

	p->pb.sub_curcnt++;

	//

	if(p->pb.sub_curcnt == p->pb.sub_max)
	{
		//最大値まで来たら、位置を終端にセット

		return mProgressBarSetPos(p, p->pb.sub_toppos + p->pb.sub_step);
	}
	else if(p->pb.sub_nextcnt < 0)
	{
		//最大値がステップ数と同じくらいの場合、常に位置を計算
		
		return mProgressBarSetPos(p,
			p->pb.sub_toppos + p->pb.sub_curcnt * p->pb.sub_step / p->pb.sub_max);
	}
	else
	{
		//最大値が大きい場合、次のステップ時のカウント値を求めておいて比較

		if(p->pb.sub_curcnt < p->pb.sub_nextcnt)
			return FALSE;
		else
		{
			p->pb.sub_curstep++;
			//次のカウント値
			p->pb.sub_nextcnt = (p->pb.sub_curstep + 1) * p->pb.sub_max / p->pb.sub_step;

			return mProgressBarSetPos(p, p->pb.sub_toppos + p->pb.sub_curstep);
		}
	}
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mProgressBarHandle_calcHint(mWidget *wg)
{
	uint32_t style = MLK_PROGRESSBAR(wg)->pb.fstyle;

	wg->hintW = 6;

	//高さ
	
	if(style & (MPROGRESSBAR_S_TEXT | MPROGRESSBAR_S_TEXT_PERS))
		wg->hintH = mWidgetGetFontHeight(wg);
	else
		wg->hintH = 13;

	//枠

	if(style & MPROGRESSBAR_S_FRAME)
		wg->hintH += _FRAME_W * 2;
}

/**@ draw ハンドラ関数 */

void mProgressBarHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mProgressBar *p = MLK_PROGRESSBAR(wg);
	mFont *font;
	int fullw,barw,barh,tx,ty,frame;
	double range;
	char m[32],*text;

	range = p->pb.max - p->pb.min;
	frame = (p->pb.fstyle & MPROGRESSBAR_S_FRAME)? _FRAME_W: 0;
	fullw = wg->w - frame * 2;
	barh = wg->h - frame * 2;
		
	//背景

	mWidgetDrawBkgnd(wg, NULL);
	
	//枠

	if(frame)
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FRAME));

	//バー

	barw = (int)(fullw / range * (p->pb.pos - p->pb.min) + 0.5);
	
	if(barw)
		mPixbufFillBox(pixbuf, frame, frame, barw, barh, MGUICOL_PIX(FACE_SELECT));
	
	//テキスト
	
	if(p->pb.fstyle & (MPROGRESSBAR_S_TEXT | MPROGRESSBAR_S_TEXT_PERS))
	{
		//text = テキストポインタ
	
		if(p->pb.fstyle & MPROGRESSBAR_S_TEXT)
			text = p->pb.text;
		else
		{
			snprintf(m, 32, "%d%%", (int)((p->pb.pos - p->pb.min) / range * 100.0 + 0.5));
			text = m;
		}
		
		//
		
		font = mWidgetGetFont(wg);
		tx = (fullw - mFontGetTextWidth(font, text, -1)) / 2;
		ty = (wg->h - mFontGetHeight(font)) / 2;

		//バー部分

		if(mPixbufClip_setBox_d(pixbuf, frame, frame, barw, barh))
			mFontDrawText_pixbuf(font, pixbuf, tx, ty, text, -1, MGUICOL_RGB(TEXT_SELECT));

		//背景部分

		if(mPixbufClip_setBox_d(pixbuf, frame + barw, frame, fullw - barw, barh))
			mFontDrawText_pixbuf(font, pixbuf, tx, ty, text, -1, MGUICOL_RGB(TEXT));
	}
}

