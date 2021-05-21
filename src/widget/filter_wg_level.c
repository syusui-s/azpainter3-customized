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
 * フィルタ用、レベル補正ウィジェット
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"


//-------------------------

typedef struct _FilterWgLevel
{
	mWidget wg;

	mPixbuf *img;
	int val[5],  //[0] in-min [1] in-mid [2] in-max [3] out-min [4] out-max
		bits,
		fdrag;
}FilterWgLevel;

//-------------------------

#define _WIDTH			256
#define _PADDING_X		3
#define _CURSOR_H		8
#define _HISTOGRAM_H	150
#define _OUTPUTGRAD_H	12
#define _SPACE_CENTER	8

#define _OUTPUT_GRAD_Y   (_HISTOGRAM_H + _CURSOR_H + _SPACE_CENTER)
#define _OUTPUT_CURSOR_Y (_HISTOGRAM_H + _CURSOR_H + _SPACE_CENTER + _OUTPUTGRAD_H)

#define DRAGF_INPUT		1
#define DRAGF_OUTPUT	2

//-------------------------



//====================
// sub
//====================

static const uint8_t g_pat_arrow[] = { 0x08,0x14,0x14,0x22,0x22,0x41,0x41,0x7f };


/* カーソル描画 */

static void _draw_cursor(FilterWgLevel *p)
{
	mPixbuf *img = p->img;
	int y,i,n;
	mPixCol col;

	//消去

	col = MGUICOL_PIX(FACE);

	mPixbufFillBox(img, 0, _HISTOGRAM_H, img->width, _CURSOR_H, col);

	mPixbufFillBox(img, 0, _OUTPUT_CURSOR_Y, img->width, _CURSOR_H, col);

	//各カーソル

	y = _HISTOGRAM_H;
	col = MGUICOL_PIX(TEXT);

	for(i = 0; i < 5; i++)
	{
		if(i == 3) y = _OUTPUT_CURSOR_Y;

		n = p->val[i];

		if(p->bits == 16)
			n = n * 255 >> 15;

		mPixbufDraw1bitPattern(img,
			n + _PADDING_X - 3, y,
			g_pat_arrow, 7, 8, MPIXBUF_TPCOL, col);
	}
}

/* 初期イメージ描画 (作成時に一度のみ) */

static void _draw_image(FilterWgLevel *p,uint32_t *histogram)
{
	mPixbuf *img = p->img;
	int i,n;
	uint32_t peek,peek_last,val;

	//背景

	mPixbufFillBox(img, 0, 0, img->width, img->height, MGUICOL_PIX(FACE));

	//--------- ヒストグラム

	//ピーク値 (2番目に大きい値の 1.2 倍)

	peek = peek_last = 0;

	for(i = 0; i < _WIDTH; i++)
	{
		val = histogram[i];

		if(peek < val)
		{
			peek_last = peek;
			peek = val;
		}

		if(peek_last < val && val < peek)
			peek_last = val;
	}

	if(peek_last)
		peek = (uint32_t)(peek_last * 1.2 + 0.5);

	//ヒストグラム背景

	mPixbufFillBox(img, _PADDING_X, 0, _WIDTH, _HISTOGRAM_H, MGUICOL_PIX(WHITE));

	//中央線

	mPixbufLineV(img, _PADDING_X + _WIDTH / 2, 0, _HISTOGRAM_H, mRGBtoPix(0xdddddd));

	//グラフ

	for(i = 0; i < _WIDTH; i++)
	{
		if(histogram[i])
		{
			n = (int)((double)histogram[i] / peek * _HISTOGRAM_H);
			if(n > _HISTOGRAM_H) n = _HISTOGRAM_H;

			if(n)
				mPixbufLineV(img, _PADDING_X + i, _HISTOGRAM_H - n, n, 0);
		}
	}

	//-------- 出力値のグラデーション

	for(i = 0; i < _WIDTH; i++)
	{
		mPixbufLineV(img, _PADDING_X + i, _HISTOGRAM_H + _CURSOR_H + _SPACE_CENTER,
			_OUTPUTGRAD_H, mRGBtoPix_sep(i,i,i));
	}

	//------- カーソル

	_draw_cursor(p);
}

/* カーソル位置変更 */

static void _change_pos(FilterWgLevel *p,int x)
{
	int pos,valno,valnum,i,n,len[3],curno;

	//値のインデックスと個数

	if(p->fdrag == DRAGF_INPUT)
		valno = 0, valnum = 3;
	else
		valno = 3, valnum = 2;

	//位置

	pos = x - _PADDING_X;
	
	if(pos < 0) pos = 0;
	else if(pos > _WIDTH - 1) pos = _WIDTH - 1;

	if(p->bits == 16)
		pos = (pos << 15) / 255;

	//他のカーソルとの距離

	for(i = 0; i < valnum; i++)
	{
		n = pos - p->val[valno + i];
		if(n < 0) n = -n;

		len[i] = n;
	}

	//一番距離の近いカーソル

	for(i = 0, n = 0xffff, curno = 0; i < valnum; i++)
	{
		if(len[i] < n)
		{
			n = len[i];
			curno = i;
		}
	}

	//カーソル位置変更

	p->val[valno + curno] = pos;

	//更新

	_draw_cursor(p);

	mWidgetRedraw(MLK_WIDGET(p));
}


//====================
// ハンドラ
//====================


/* 押し時 */

static void _event_press(FilterWgLevel *p,mEvent *ev)
{
	//入力 or 出力

	if(ev->pt.y < _OUTPUT_GRAD_Y)
		p->fdrag = DRAGF_INPUT;
	else
		p->fdrag = DRAGF_OUTPUT;

	//

	mWidgetGrabPointer(MLK_WIDGET(p));

	_change_pos(p, ev->pt.x);
}

/* グラブ解除 */

static void _release_grab(FilterWgLevel *p)
{
	if(p->fdrag)
	{
		p->fdrag = 0;
		mWidgetUngrabPointer();

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, 0, 0);
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	FilterWgLevel *p = (FilterWgLevel *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fdrag)
					_change_pos(p, ev->pt.x);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && !p->fdrag)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBlt(pixbuf, 0, 0, ((FilterWgLevel *)wg)->img, 0, 0, -1, -1);
}


//====================
// main
//====================


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((FilterWgLevel *)wg)->img);
}

/** 作成
 *
 * histgram: 256個。初期イメージの作成時のみ使用するため、作成後は解放して良い。 */

FilterWgLevel *FilterWgLevel_new(mWidget *parent,int id,int bits,uint32_t *histogram)
{
	FilterWgLevel *p;

	p = (FilterWgLevel *)mWidgetNew(parent, sizeof(FilterWgLevel));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.flayout = MLF_FIX_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.w = _WIDTH + _PADDING_X * 2;
	p->wg.h = _HISTOGRAM_H + _CURSOR_H * 2 + _SPACE_CENTER + _OUTPUTGRAD_H;

	p->bits = bits;

	if(bits == 8)
	{
		p->val[1] = 128;
		p->val[2] = p->val[4] = 255;
	}
	else
	{
		p->val[1] = 0x4000;
		p->val[2] = p->val[4] = 0x8000;
	}

	//イメージ

	p->img = mPixbufCreate(p->wg.w, p->wg.h, 0);

	_draw_image(p, histogram);

	return p;
}

/** 値を取得 */

void FilterWgLevel_getValue(FilterWgLevel *p,int *buf)
{
	int i;

	for(i = 0; i < 5; i++)
		buf[i] = p->val[i];
}

