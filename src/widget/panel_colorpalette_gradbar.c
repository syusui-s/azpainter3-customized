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
 * 中間色グラデーションバー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_guicol.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"

#include "def_draw.h"

/*
 [通知]
 type  : 0=色取得, 1=左の色変更, 2=右の色変更
 param1: RGB色
*/

//---------------------

typedef struct
{
	mWidget wg;

	uint32_t col[2];		//左右の色
	uint8_t rgb[2][3];		//左右の RGB 値
	int16_t rgb_diff[3],	//RGB 色差
		rgb_diff_step[3];	//RGB 色差 (段階有効時)
	int step,			//段階数
		fdrag,
		curx,			//ドラッグ時のカーソル位置
		last_gradx;		//ドラッグ時、前回のグラデーション部分X位置 (-1 で最初)
}GradBar;

//---------------------

#define _BAR_H    13
#define _BETWEEN  4		//端の色とグラデーションの間

//---------------------


//========================
// sub
//========================


/* 左右の色から、作業用の値をセット */

static void _set_rgb(GradBar *p)
{
	int i,j,sf;
	uint32_t c;

	//R,G,B

	for(i = 0; i < 2; i++)
	{
		c = p->col[i];
		
		for(j = 0, sf = 16; j < 3; j++, sf -= 8)
			p->rgb[i][j] = (c >> sf) & 255;
	}

	//色差

	for(i = 0; i < 3; i++)
	{
		j = p->rgb[1][i] - p->rgb[0][i];

		p->rgb_diff[i] = j;

		//差が 0 より大きくて奇数の場合は、+1
		if((j & 1) && j > 0) j++;

		p->rgb_diff_step[i] = j;
	}
}

/* ドラッグ移動: 色を取得して通知 */

static void _on_motion(GradBar *p,int curx)
{
	int i,x,w,c[3],step,stepno;
	double d;

	x = curx - (_BAR_H + _BETWEEN + 1);  //グラデーション部分を先頭とした位置
	w = p->wg.w - (_BAR_H + _BETWEEN) * 2 - 2;

	if(x < 0) x = 0;
	else if(x >= w) x = w - 1;

	//前回位置と比較

	if(p->last_gradx == x) return;

	p->last_gradx = x;
	p->curx = x + _BAR_H + _BETWEEN + 1;

	//色取得

	step = p->step;

	if(step < 3)
	{
		//段階なし
		
		d = (double)x / (w - 1);

		for(i = 0; i < 3; i++)
			c[i] = (int)(p->rgb_diff[i] * d + p->rgb[0][i] + 0.5);
	}
	else
	{
		//段階あり

		stepno = x * step / w;

		if(stepno == step - 1)
		{
			//終端の色
			
			for(i = 0; i < 3; i++)
				c[i] = p->rgb[1][i];
		}
		else
		{
			for(i = 0; i < 3; i++)
				c[i] = stepno * p->rgb_diff_step[i] / (step - 1) + p->rgb[0][i];
		}
	}

	//通知

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, MLK_RGB(c[0], c[1], c[2]), 0);

	mWidgetRedraw(MLK_WIDGET(p));
}

/* 押し時 */

static void _event_press(GradBar *p,int x)
{
	int w = p->wg.w,no;

	if(x < _BAR_H || x >= w - _BAR_H)
	{
		//左右の色: 描画色セット

		no = (x >= w - _BAR_H);

		p->col[no] = RGBcombo_to_32bit(&APPDRAW->col.drawcol);

		_set_rgb(p);

		mWidgetRedraw(MLK_WIDGET(p));

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 1 + no, p->col[no], 0);
	}
	else if(x >= _BAR_H + _BETWEEN + 1 && x < w - _BAR_H - _BETWEEN)
	{
		//グラデーション部分 (枠は含まない)

		p->fdrag = 1;
		p->last_gradx = -1;

		_on_motion(p, x);

		mWidgetGrabPointer(MLK_WIDGET(p));
	}
}

/* グラブ解除 */

static void _grab_release(GradBar *p)
{
	if(p->fdrag)
	{
		p->fdrag = 0;

		mWidgetUngrabPointer();
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	GradBar *p = (GradBar *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fdrag)
					_on_motion(p, ev->pt.x);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し
				
				if(ev->pt.btt == MLK_BTT_LEFT && !p->fdrag)
					_event_press(p, ev->pt.x);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_grab_release(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_grab_release(p);
			break;
	}

	return 1;
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	GradBar *p = (GradBar *)wg;
	int i,j,x,w,c[3],step,stepno,pitch,bpp;
	uint32_t col;
	uint8_t *pd,*pd_y;
	mBox box;
	mFuncPixbufSetBuf setpix;

	w = wg->w;

	//背景色

	mPixbufFillBox(pixbuf, 0, 0, w, _BAR_H, MGUICOL_PIX(FACE));

	//端の色

	for(i = 0, x = 0; i < 2; i++)
	{
		mPixbufBox(pixbuf, x, 0, _BAR_H, _BAR_H, 0);
		mPixbufFillBox(pixbuf, x + 1, 1, _BAR_H - 2, _BAR_H - 2, mRGBtoPix(p->col[i]));

		x = w - _BAR_H;
	}

	//グラデーション

	w -= (_BAR_H + _BETWEEN) * 2;

	mPixbufBox(pixbuf, _BAR_H + _BETWEEN, 0, w, _BAR_H, 0);


	if(mPixbufClip_getBox_d(pixbuf, &box, _BAR_H + _BETWEEN + 1, 1, w - 2, _BAR_H - 2))
	{
		pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
		bpp = pixbuf->pixel_bytes;
		pitch = pixbuf->line_bytes;

		mPixbufGetFunc_setbuf(pixbuf, &setpix);

		x = box.x - (_BAR_H + _BETWEEN + 1);
		w -= 2;

		step = p->step;
		if(step < 3) w--;

		for(i = 0; i < box.w; i++, x++, pd += bpp)
		{
			if(step < 3)
			{
				//段階なし

				for(j = 0; j < 3; j++)
					c[j] = p->rgb_diff[j] * x / w + p->rgb[0][j];
			}
			else
			{
				//段階あり
				
				stepno = x * step / w;

				if(stepno == step - 1)
				{
					//終端
					
					for(j = 0; j < 3; j++)
						c[j] = p->rgb[1][j];
				}
				else
				{
					for(j = 0; j < 3; j++)
						c[j] = stepno * p->rgb_diff_step[j] / (step - 1) + p->rgb[0][j];
				}
			}

			col = mRGBtoPix_sep(c[0], c[1], c[2]);

			//Y を同じ色で埋める

			for(j = _BAR_H - 2, pd_y = pd; j; j--, pd_y += pitch)
				(setpix)(pd_y, col);
		}
	}

	//カーソル

	if(p->fdrag)
	{
		mPixbufSetPixelModeXor(pixbuf);
		mPixbufLineV(pixbuf, p->curx, 0, _BAR_H, 0);
		mPixbufSetPixelModeCol(pixbuf);
	}
}


//========================


/** バー作成 */

mWidget *GradationBar_new(mWidget *parent,
	int id,uint32_t col_left,uint32_t col_right,int step)
{
	GradBar *p;

	p = (GradBar *)mWidgetNew(parent, sizeof(GradBar));

	p->col[0] = col_left;
	p->col[1] = col_right;
	p->step = step;

	p->wg.id = id;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.flayout = MLF_EXPAND_W;
	p->wg.hintH = _BAR_H;
	p->wg.hintW = 80;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;

	_set_rgb(p);

	return (mWidget *)p;
}

/** 段階数変更 */

void GradationBar_setStep(mWidget *wg,int step)
{
	((GradBar *)wg)->step = step;

	mWidgetRedraw(wg);
}
