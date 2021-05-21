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
 * ColorWheel
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_color.h"
#include "mlk_menu.h"

#include "def_config.h"

#include "colorwheel.h"
#include "changecol.h"
#include "appresource.h"
#include "trid.h"


//三角形は、0度(赤色)に向かうような形になる。

//------------------

struct _ColorWheel
{
	mWidget wg;

	mPixbuf *img;
	int type,
		size,	//画像のサイズ
		fdrag;
	double radius,	//Hの円の半径
		radius_sv;	//SVの形の半径
	mPoint pt_sv;	//SV カーソル位置 (イメージ位置)
	mHSVd hsv;
};

//------------------

#define _HUE_RADIUS_IN    0.8
#define _HUE_RADIUS_CURSOR  (_HUE_RADIUS_IN + (1.0 - _HUE_RADIUS_IN) / 2)

#define _SIZE_MIN  130
#define _SIZE_MAX  300
#define _BTTSIZE   17

enum
{
	_DRAG_NONE,
	_DRAG_HUE,
	_DRAG_SV
};

enum
{
	TRID_MENU_HSV_TRIANGLE,
	TRID_MENU_HSV_RECTANGLE
};

//------------------

static const uint8_t g_cursor_img[] = {
0x38,0x00,0x44,0x00,0x82,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x82,0x00,0x44,0x00,
0x38,0x00 };

//------------------



//=========================
// 描画
//=========================


/* SV (三角形) 描画 */

static void _draw_sv_triangle(ColorWheel *p)
{
	mPixbuf *img = p->img;
	uint8_t *pd;
	int x,y,bpp,pitch,top,bottom;
	double dx,dy,d,sqrt3,radius,tri_radius;
	mRGB8 rgb;
	mFuncPixbufSetBuf setpix;

	sqrt3 = sqrt(3);
	radius = p->radius;
	tri_radius = p->radius_sv;

	top = (int)(radius - radius * _HUE_RADIUS_IN);
	bottom = (int)(radius + radius * _HUE_RADIUS_IN);

	//

	pd = mPixbufGetBufPtFast(img, top, top);
	bpp = img->pixel_bytes;
	pitch = img->line_bytes - bpp * (bottom - top + 1);

	mPixbufGetFunc_setbuf(img, &setpix);

	for(y = top; y <= bottom; y++)
	{
		//半径 x ルート3で、三角形のY方向の辺の長さ
		dy = (y - radius) / tri_radius * sqrt3;
	
		for(x = top; x <= bottom; x++, pd += bpp)
		{
			//三角形の外接円の半径を 1 とするので、-1.0〜1.0 となる
			dx = (x - radius) / tri_radius;

			if(dx <= 1				//中心-右方向は、半径 1.0 が最大
				&& dx >= -0.5		//中心-左方向は、半径 -0.5 が最大
				&& dx + dy <= 1
				&& dx - dy <= 1)
			{
				d = dx - dy + 2;
				
				mHSV_to_RGB8(&rgb, p->hsv.h, (1 + dx * 2) / d, d / 3);

				(setpix)(pd, mRGBtoPix_sep(rgb.r, rgb.g, rgb.b));
			}
		}

		pd += pitch;
	}
}

/* SV (四角形) 描画 */

static void _draw_sv_rectangle(ColorWheel *p)
{
	mPixbuf *img = p->img;
	uint8_t *pd;
	int x,y,bpp,pitch,top,bottom,size;
	double dy,div;
	mRGB8 rgb;
	mFuncPixbufSetBuf setpix;

	top = (int)(p->radius - p->radius_sv);
	bottom = (int)(p->radius + p->radius_sv);
	size = bottom - top + 1;

	pd = mPixbufGetBufPtFast(img, top, top);
	bpp = img->pixel_bytes;
	pitch = img->line_bytes - bpp * size;

	mPixbufGetFunc_setbuf(img, &setpix);

	div = 1.0 / (size - 1);

	for(y = 0; y < size; y++)
	{
		dy = (size - 1 - y) * div;
	
		for(x = 0; x < size; x++, pd += bpp)
		{
			mHSV_to_RGB8(&rgb, p->hsv.h, x * div, dy);

			(setpix)(pd, mRGBtoPix_sep(rgb.r, rgb.g, rgb.b));
		}

		pd += pitch;
	}
}

/* SV 描画 */

static void _draw_sv(ColorWheel *p)
{
	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
		_draw_sv_triangle(p);
	else
		_draw_sv_rectangle(p);
}

/* H カーソル描画 */

static void _draw_cursor_hue(ColorWheel *p)
{
	double d,r;
	int x,y;

	r = p->radius * _HUE_RADIUS_CURSOR;
	d = -(p->hsv.h / 3.0 * MLK_MATH_PI);

	x = (int)(r * cos(d) + p->radius + 0.5);
	y = (int)(r * sin(d) + p->radius + 0.5);

	mPixbufDraw1bitPattern(p->img, x - 4, y - 4,
		g_cursor_img, 9, 9, MPIXBUF_TPCOL, 1);
}

/* SV カーソル描画 */

static void _draw_cursor_sv(ColorWheel *p)
{
	mPixbufDraw1bitPattern(p->img,
		p->pt_sv.x - 4, p->pt_sv.y - 4,
		g_cursor_img, 9, 9, MPIXBUF_TPCOL, 1);
}

/* H/SV カーソル描画 */

static void _draw_cursor_all(ColorWheel *p)
{
	mPixbufSetPixelModeXor(p->img);
	
	_draw_cursor_hue(p);
	_draw_cursor_sv(p);

	mPixbufSetPixelModeCol(p->img);
}

/* 色相の円描画 */

static void _draw_hue_circle(ColorWheel *p,mPixbuf *img)
{
	int ix,iy,bpp,pitch,size;
	double dx,dy,dyy,r,rd,radius,radius_in;
	uint8_t *pd;
	mFuncPixbufSetBuf setpix;
	mRGB8 rgb;

	size = p->size;
	radius = p->radius;
	radius_in = radius * _HUE_RADIUS_IN;

	pd = img->buf;
	bpp = img->pixel_bytes;
	pitch = img->line_bytes - bpp * size;

	mPixbufGetFunc_setbuf(img, &setpix);

	for(iy = 0; iy < size; iy++)
	{
		dy = iy - radius;
		dyy = dy * dy;
	
		for(ix = 0; ix < size; ix++)
		{
			//半径
			
			dx = ix - radius;
			
			r = sqrt(dx * dx + dyy);

			//描画

			if(r >= radius_in && r < radius)
			{
				//角度 (半時計回り)

				rd = -atan2(dy, dx);

				if(rd < 0)
					rd += MLK_MATH_PI * 2;

				//色

				mHSV_to_RGB8(&rgb, rd * 6 / (MLK_MATH_PI * 2), 1, 1);

				(setpix)(pd, mRGBtoPix_sep(rgb.r, rgb.g, rgb.b));
			}

			pd += bpp;
		}

		pd += pitch;
	}
}

/* 初期描画 */

static void _init_draw(ColorWheel *p)
{
	mPixbuf *img = p->img;

	//背景

	mPixbufFillBox(img, 0, 0, p->size, p->size, MGUICOL_PIX(FACE));

	//ボタン

	mPixbufDrawButton(img, 0, 0, _BTTSIZE, _BTTSIZE, 0);

	mPixbufDraw1bitPattern(img, (_BTTSIZE - 7) / 2, (_BTTSIZE - 7) / 2,
		AppResource_get1bitImg_menubtt(), APPRES_MENUBTT_SIZE, APPRES_MENUBTT_SIZE,
		MPIXBUF_TPCOL, MGUICOL_PIX(TEXT));

	//色相

	_draw_hue_circle(p, img);

	//SV

	_draw_sv(p);

	//カーソル

	_draw_cursor_all(p);
}


//=========================
// sub
//=========================


/* SV の半径をセット */

static void _set_sv_radius(ColorWheel *p)
{
	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
		p->radius_sv = p->radius * _HUE_RADIUS_IN - 2;
	else
		p->radius_sv = p->radius * _HUE_RADIUS_IN * cos(45 / 180.0 * MLK_MATH_PI) - 2;
}

/* 色変更の通知 */

static void _notify(ColorWheel *p)
{
	int h,s,v;

	h = (int)(p->hsv.h * 60 + 0.5) % 360;
	s = (int)(p->hsv.s * 255 + 0.5);
	v = (int)(p->hsv.v * 255 + 0.5);

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
		COLORWHEEL_N_CHANGE_COL,
		mHSV_to_RGB8pac(p->hsv.h, p->hsv.s, p->hsv.v),
		CHANGECOL_MAKE(CHANGECOL_TYPE_HSV, h, s, v));
}

/* 現在の色から、SV カーソル位置セット */

static void _set_sv_curpos(ColorWheel *p)
{
	double d,svr,radius;

	radius = p->radius;
	svr = p->radius_sv;

	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
	{
		d = (p->hsv.s * p->hsv.v * 3 - 1) * 0.5;
	
		p->pt_sv.x = (int)(d * svr + radius + 0.5);
		p->pt_sv.y = (int)((d + 2 - p->hsv.v * 3) / sqrt(3) * svr + radius + 0.5);
	}
	else
	{
		p->pt_sv.x = (int)((p->hsv.s - 0.5) * 2 * svr + radius + 0.5);
		p->pt_sv.y = (int)((0.5 - p->hsv.v) * 2 * svr + radius + 0.5);
	}
}

/* イメージ部分を再描画 */

static void _redraw_image(ColorWheel *p)
{
	mBox box;

	box.x = box.y = 0;
	box.w = box.h = p->size;

	mWidgetRedrawBox(MLK_WIDGET(p), &box);
}


//=========================
// ポインタ操作
//=========================


/* 色相操作 */

static void _pointer_hue(ColorWheel *p,double dx,double dy)
{
	double rd;

	//角度

	rd = -atan2(dy, dx);

	if(rd < 0)
		rd += MLK_MATH_PI * 2;

	//カーソル消去

	_draw_cursor_all(p);

	//H

	p->hsv.h = rd / MLK_MATH_PI * 3;

	//

	_draw_sv(p);
	_draw_cursor_all(p);
	_redraw_image(p);

	_notify(p);
}

/* SV 位置変更
 *
 * x,y: SV 位置 */

static void _set_svpos(ColorWheel *p,double s,double v,int x,int y)
{
	//SV 値
	
	if(s != p->hsv.s || v != p->hsv.v)
	{
		p->hsv.s = s;
		p->hsv.v = v;

		_notify(p);
	}

	//カーソル位置

	if(p->pt_sv.x != x || p->pt_sv.y != y)
	{
		mPixbufSetPixelModeXor(p->img);
		
		_draw_cursor_sv(p);

		p->pt_sv.x = x;
		p->pt_sv.y = y;

		_draw_cursor_sv(p);

		mPixbufSetPixelModeCol(p->img);

		_redraw_image(p);
	}
}

/* SV 操作 (三角形) */

static void _pointer_sv_triangle(ColorWheel *p,double dx,double dy)
{
	double d,radius_sv,sqrt3,s,v;
	int ix,iy;

	radius_sv = p->radius_sv;
	sqrt3 = sqrt(3);

	dx = dx / radius_sv;
	dy = dy / radius_sv * sqrt3;

	if(dx < -0.5)
		dx = -0.5;
	else if(dx > 1)
		dx = 1;

	if(dx + dy > 1)
		dy = 1 - dx;

	if(dx - dy > 1)
		dy = dx - 1;

	//

	d = dx - dy + 2;

	s = (1 + dx * 2) / d;
	v = d / 3;

	ix = (int)(dx * radius_sv + p->radius + 0.5);
	iy = (int)(dy / sqrt3 * radius_sv + p->radius + 0.5);

	_set_svpos(p, s, v, ix, iy);
}

/* SV 操作 (四角形) */

static void _pointer_sv_rectangle(ColorWheel *p,double dx,double dy)
{
	double r,s,v;
	int ix,iy;

	r = p->radius_sv;

	if(dx < -r) dx = -r;
	else if(dx > r) dx = r;

	if(dy < -r) dy = -r;
	else if(dy > r) dy = r;

	ix = (int)(dx + p->radius + 0.5);
	iy = (int)(dy + p->radius + 0.5);

	dx += r;
	dy += r;

	s = dx / (r * 2);
	v = 1 - dy / (r * 2);

	_set_svpos(p, s, v, ix, iy);
}

/* SV 操作 */

static void _pointer_sv(ColorWheel *p,double dx,double dy)
{
	if(p->type == COLORWHEEL_TYPE_HSV_TRIANGLE)
		_pointer_sv_triangle(p, dx, dy);
	else
		_pointer_sv_rectangle(p, dx, dy);
}

/* メニュー */

static void _run_menu(ColorWheel *p)
{
	mMenu *menu;
	mMenuItem *mi;
	mBox box;
	int i,id;

	MLK_TRGROUP(TRGROUP_PANEL_COLOR_WHEEL);

	menu = mMenuNew();

	for(i = 0; i < 2; i++)
		mMenuAppendRadio(menu, i, MLK_TR(TRID_MENU_HSV_TRIANGLE + i));

	mMenuSetItemCheck(menu, p->type, 1);

	box.x = box.y = 0;
	box.w = box.h = _BTTSIZE;
	
	mi = mMenuPopup(menu, MLK_WIDGET(p), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_Y, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//

	if(id != -1)
	{
		p->type = id;

		APPCONF->panel.colwheel_type = id;

		_set_sv_radius(p);
		_set_sv_curpos(p);
		_init_draw(p);

		_redraw_image(p);
	}
}

/* 押し時 */

static void _event_press(ColorWheel *p,int x,int y)
{
	double r,dx,dy,radius;

	if(x < _BTTSIZE && y < _BTTSIZE)
	{
		//メニューボタン

		_run_menu(p);
	}
	else
	{
		//半径

		radius = p->radius;

		dx = x - radius;
		dy = y - radius;
		
		r = sqrt(dx * dx + dy * dy);

		//

		if(r < radius)
		{
			if(r >= radius * _HUE_RADIUS_IN)
			{
				//色相内

				_pointer_hue(p, dx, dy);

				p->fdrag = _DRAG_HUE;
				mWidgetGrabPointer(MLK_WIDGET(p));
			}
			else
			{
				//SV

				_pointer_sv(p, dx, dy);

				p->fdrag = _DRAG_SV;
				mWidgetGrabPointer(MLK_WIDGET(p));
			}
		}
	}
}

/* 移動時 */

static void _event_motion(ColorWheel *p,double dx,double dy)
{
	dx -= p->radius;
	dy -= p->radius;

	if(p->fdrag == _DRAG_HUE)
		_pointer_hue(p, dx, dy);
	else
		_pointer_sv(p, dx, dy);
}

/* グラブ解放 */

static void _release_grab(ColorWheel *p)
{
	if(p->fdrag)
	{
		p->fdrag = _DRAG_NONE;
		mWidgetUngrabPointer();
	}
}


//=========================
//
//=========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	ColorWheel *p = (ColorWheel *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				if(p->fdrag)
					_event_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				if(ev->pt.btt == MLK_BTT_LEFT && p->fdrag == _DRAG_NONE)
					_event_press(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
	}

	return 1;
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ColorWheel *p = (ColorWheel *)wg;

	//ウィジェットサイズがイメージより大きく、かつ全体描画の場合のみ、背景描画
	// : イメージ部分のみ更新の場合、mWidgetRedrawBox() で 1 が返る

	if((wg->w > p->size || wg->h > p->size)
		&& mWidgetGetDrawBox(wg, NULL) == 0)
	{
		mWidgetDrawBkgnd(wg, NULL);
	}

	mPixbufBlt(pixbuf, 0, 0, p->img, 0, 0, p->size, p->size);
}

/* リサイズ */

static void _resize_handle(mWidget *wg)
{
	ColorWheel *p = (ColorWheel *)wg;
	int size;

	size = wg->w;
	if(size > wg->h) size = wg->h;
	
	if(size < _SIZE_MIN) size = _SIZE_MIN;
	else if(size > _SIZE_MAX) size = _SIZE_MAX;

	if(p->size != size)
	{
		p->size = size;
		p->radius = size * 0.5;

		_set_sv_radius(p);
		_set_sv_curpos(p);

		//画像

		if(!p->img)
			p->img = mPixbufCreate(size, size, 0);
		else
			mPixbufResizeStep(p->img, size, size, 32, 32);

		_init_draw(p);
	}
}


//===========================
// main
//===========================


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((ColorWheel *)wg)->img);
}

/* 作成 */

ColorWheel *ColorWheel_new(mWidget *parent,int id,uint32_t col)
{
	ColorWheel *p;

	p = (ColorWheel *)mWidgetNew(parent, sizeof(ColorWheel));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.destroy = _destroy_handle;
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.resize = _resize_handle;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.flayout = MLF_EXPAND_WH;

	p->type = APPCONF->panel.colwheel_type;

	//初期色

	mRGB8pac_to_HSV(&p->hsv, col);

	return p;
}

/** 色をセット
 *
 * col: CHANGECOL */

void ColorWheel_setColor(ColorWheel *p,uint32_t col)
{
	_draw_cursor_all(p);

	ChangeColor_to_HSV(&p->hsv, col);

	_set_sv_curpos(p);

	//

	_draw_sv(p);
	_draw_cursor_all(p);

	_redraw_image(p);
}

