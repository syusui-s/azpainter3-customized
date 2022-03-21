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
 * (カラーパレット) HSL パレット
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_guicol.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_color.h"

#include "def_draw.h"

#include "changecol.h"


//---------------------

typedef struct
{
	mWidget wg;

	mPixbuf *img;
	int fdrag;
}HSLPal;

//---------------------

#define _TOP_H    10	//H 部分の高さ
#define _TOP_W    7		//H 部分の一つの幅 (枠1px除く)
#define _BETWEEN  8		//H と LS の間の高さ
#define _PAL_W    14	//LS の一つの幅
#define _PAL_H    9
#define _PAL_XNUM 15	//LS 横の数
#define _PAL_YNUM 10
#define _PAL_TOPY  (_TOP_H + _BETWEEN) //H 部分(余白含む)の高さ

#define _HUE_NUM   30
#define _HUE_STEP  (6.0 / _HUE_NUM)

#define _GET_L(x)  (0.08 + (x) * 0.06) //真ん中が0.5になるように
#define _GET_S(y)  (1 - (y) * 0.1)

enum
{
	_DRAGF_HUE = 1,
	_DRAGF_PALETTE
};

//---------------------


//========================
// 描画
//========================


/* H カーソル描画 */

static void _draw_cursor_hue(HSLPal *p,mlkbool erase)
{
	mPixbufDrawArrowUp(p->img,
		APPDRAW->col.hslpal_hue * _TOP_W + (_TOP_W - 5) / 2, _TOP_H + 1,
		3, (erase)? MGUICOL_PIX(FACE): MGUICOL_PIX(TEXT));
}

/* パレットカーソル描画 */

static void _draw_cursor_pal(HSLPal *p,mlkbool erase)
{
	mPixbufBox(p->img,
		APPDRAW->col.hslpal_x * _PAL_W,
		_PAL_TOPY + APPDRAW->col.hslpal_y * _PAL_H,
		_PAL_W + 1, _PAL_H + 1,
		(erase)? 0: MGUICOL_PIX(WHITE));
}

/* パレット色の描画 */

static void _draw_palette(HSLPal *p)
{
	mPixbuf *img = p->img;
	int ix,iy,x,y;
	double hue,s;
	mRGB8 col;

	hue = APPDRAW->col.hslpal_hue * _HUE_STEP;

	for(iy = 0, y = _PAL_TOPY + 1; iy < _PAL_YNUM; iy++, y += _PAL_H)
	{
		s = _GET_S(iy);
		
		for(ix = 0, x = 1; ix < _PAL_XNUM; ix++, x += _PAL_W)
		{
			mHSL_to_RGB8(&col, hue, s, _GET_L(ix));
			
			mPixbufFillBox(img, x, y, _PAL_W - 1, _PAL_H - 1,
				mRGBtoPix_sep(col.r, col.g, col.b));
		}
	}
}

/* 最初の描画 */

static void _draw_first(HSLPal *p)
{
	mPixbuf *img = p->img;
	int w,i,n;
	mRGB8 col;

	w = p->wg.w;

	//H 部分の枠

	mPixbufBox(img, 0, 0, w, _TOP_H, 0);

	for(i = _HUE_STEP - 1, n = _TOP_W; i; i--, n += _TOP_W)
		mPixbufLineV(img, n, 1, _TOP_H - 2, 0);

	//H 色

	for(i = 0, n = 1; i < _HUE_NUM; i++, n += _TOP_W)
	{
		mHSL_to_RGB8(&col, i * _HUE_STEP, 1.0, 0.5);
		mPixbufFillBox(img, n, 1, _TOP_W - 1, _TOP_H - 2, mRGBtoPix_sep(col.r, col.g, col.b));
	}

	//H とパレットの間

	mPixbufFillBox(img, 0, _TOP_H, w, _BETWEEN, MGUICOL_PIX(FACE));

	//パレットの枠

	for(i = _PAL_XNUM + 1, n = 0; i; i--, n += _PAL_W)
		mPixbufLineV(img, n, _PAL_TOPY, _PAL_H * _PAL_YNUM + 1, 0);

	for(i = _PAL_YNUM + 1, n = _PAL_TOPY; i; i--, n += _PAL_H)
		mPixbufLineH(img, 0, n, _PAL_W * _PAL_XNUM + 1, 0);

	//パレット色、カーソル

	_draw_palette(p);
	_draw_cursor_hue(p, FALSE);
	_draw_cursor_pal(p, FALSE);
}


//========================
// sub
//========================


/* 通知 */

static void _notify(HSLPal *p)
{
	double h,s,l;
	uint32_t rgb,col;

	h = APPDRAW->col.hslpal_hue * _HUE_STEP;
	s = _GET_S(APPDRAW->col.hslpal_y);
	l = _GET_L(APPDRAW->col.hslpal_x);

	rgb = mHSL_to_RGB8pac(h, s, l);

	col = CHANGECOL_MAKE(CHANGECOL_TYPE_HSL,
			(int)(h * 60 + 0.5), (int)(s * 255 + 0.5), (int)(l * 255 + 0.5));

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, rgb, col);
}

/* HUE 選択 */

static void _on_motion_hue(HSLPal *p,int x)
{
	x = (x - 1) / _TOP_W;

	if(x < 0) x = 0;
	else if(x >= _HUE_NUM) x = _HUE_NUM - 1;

	if(x != APPDRAW->col.hslpal_hue)
	{
		_draw_cursor_hue(p, TRUE);
		
		APPDRAW->col.hslpal_hue = x;

		_draw_cursor_hue(p, FALSE);
		_draw_palette(p);

		mWidgetRedraw(MLK_WIDGET(p));

		_notify(p);
	}
}

/* パレット選択 */

static void _on_motion_palette(HSLPal *p,int x,int y)
{
	//パレット位置

	x = (x - 1) / _PAL_W;
	y = (y - _PAL_TOPY - 1) / _PAL_H;

	if(x < 0) x = 0; else if(x >= _PAL_XNUM) x = _PAL_XNUM - 1;
	if(y < 0) y = 0; else if(y >= _PAL_YNUM) y = _PAL_YNUM - 1;

	//変更

	if(x != APPDRAW->col.hslpal_x || y != APPDRAW->col.hslpal_y)
	{
		_draw_cursor_pal(p, TRUE);

		APPDRAW->col.hslpal_x = x;
		APPDRAW->col.hslpal_y = y;

		_draw_cursor_pal(p, FALSE);

		mWidgetRedraw(MLK_WIDGET(p));

		_notify(p);
	}
}


//========================
// ハンドラ
//========================


/* 押し時 */

static void _event_press(HSLPal *p,int x,int y)
{
	if(y < _PAL_TOPY)
	{
		//HUE

		p->fdrag = _DRAGF_HUE;

		_on_motion_hue(p, x);
	}
	else
	{
		//パレット

		p->fdrag = _DRAGF_PALETTE;

		_on_motion_palette(p, x, y);
	}

	mWidgetGrabPointer(MLK_WIDGET(p));
}

/* グラブ解除 */

static void _grab_release(HSLPal *p)
{
	if(p->fdrag)
	{
		p->fdrag = 0;
		mWidgetUngrabPointer();
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	HSLPal *p = (HSLPal *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fdrag == _DRAGF_PALETTE)
					_on_motion_palette(p, ev->pt.x, ev->pt.y);
				else if(p->fdrag == _DRAGF_HUE)
					_on_motion_hue(p, ev->pt.x);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し
				
				if(ev->pt.btt == MLK_BTT_LEFT && !p->fdrag)
					_event_press(p, ev->pt.x, ev->pt.y);
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
	mPixbufBlt(pixbuf, 0, 0, ((HSLPal *)wg)->img, 0, 0, -1, -1);
}

/* 破棄 */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((HSLPal *)wg)->img);
}

/** 作成 */

mWidget *HSLPalette_new(mWidget *parent)
{
	HSLPal *p;

	p = (HSLPal *)mWidgetNew(parent, sizeof(HSLPal));

	p->wg.destroy = _destroy_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.flayout = MLF_FIX_WH;
	p->wg.w = _TOP_W * _HUE_NUM + 1;
	p->wg.h = _TOP_H + _BETWEEN + _PAL_H * _PAL_YNUM + 1;

	//イメージ作成

	p->img = mPixbufCreate(p->wg.w, p->wg.h, 0);

	_draw_first(p);

	return (mWidget *)p;
}

