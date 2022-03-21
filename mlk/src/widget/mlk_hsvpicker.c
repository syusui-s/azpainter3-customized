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
 * mHSVPicker
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_hsvpicker.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_color.h"


//-------------------

#define _SPACE_W   6	//SV と H の間の幅
#define _HUE_W     13 	//色相バーの幅

#define _GRABF_HUE  1
#define _GRABF_SV   2

#define _HUE_TO_Y(hue,size)  ((hue) * (size) / 360)
#define _Y_TO_HUE(y,size)    ((y) * 360 / (size))

//-------------------



//============================
// 描画
//============================


/** SV 部分描画 */

static void _draw_sv(mHSVPicker *p)
{
	mPixbuf *img = p->hsv.img;
	uint8_t *pd;
	mFuncPixbufSetBuf setbuf;
	int ix,iy,h,hue,div,v,bytes,pitch;
	uint32_t c;

	h = p->wg.h;
	hue = p->hsv.cur_hue;
	div = h - 1;

	pd = img->buf;
	bytes = img->pixel_bytes;
	pitch = img->line_bytes - h * bytes;

	mPixbufGetFunc_setbuf(img, &setbuf);

	//

	for(iy = 0; iy < h; iy++, pd += pitch)
	{
		v = 255 - iy * 255 / div;
	
		for(ix = 0; ix < h; ix++, pd += bytes)
		{
			c = mHSVi_to_RGB8pac(hue, ix * 255 / div, v);
			
			(setbuf)(pd, mRGBtoPix(c));
		}
	}
}

/** H カーソル描画 */

static void _draw_hue_cursor(mHSVPicker *p)
{
	mPixbuf *img = p->hsv.img;
	int x,y;
	mPoint pt[3];

	x = p->wg.h + _SPACE_W;
	y = p->hsv.cur_hue_y;

	pt[0].x = x, pt[0].y = y;
	pt[1].x = x + 5, pt[1].y = y - 5;
	pt[2].x = x + 5, pt[2].y = y + 5;

	mPixbufLines_close(img, pt, 3, 0);
}

/** SV カーソル描画 */

static void _draw_sv_cursor(mHSVPicker *p)
{
	mPixbufBox_pixel(p->hsv.img, p->hsv.cur_sv_x - 4, p->hsv.cur_sv_y - 4, 9, 9, 0);
}

/** HUE 変更時の描画 */

static void _draw_change_hue(mHSVPicker *p,int hue,int hue_y)
{
	mPixbuf *img = p->hsv.img;

	//カーソル消去

	mPixbufSetPixelModeXor(img);

	_draw_hue_cursor(p);
	_draw_sv_cursor(p);

	//位置変更

	p->hsv.cur_hue = hue;
	p->hsv.cur_hue_y = hue_y;

	//SV 描画

	mPixbufSetPixelModeCol(img);

	_draw_sv(p);

	//カーソル描画

	mPixbufSetPixelModeXor(img);

	_draw_hue_cursor(p);
	_draw_sv_cursor(p);

	mWidgetRedraw(MLK_WIDGET(p));
}


//============================
// sub
//============================


/** イメージ初期化 */

static void _init_image(mHSVPicker *p)
{
	uint8_t *pd;
	mPixbuf *img;
	int i,h,pitch;
	uint32_t col;

	img = p->hsv.img;
	h = p->wg.h;

	mPixbufSetPixelModeCol(img);

	//SV

	_draw_sv(p);

	//間の空白

	mPixbufFillBox(img, h, 0, _SPACE_W, h, MGUICOL_PIX(FACE));

	//色相 (H)

	pd = mPixbufGetBufPt(img, h + _SPACE_W, 0);
	pitch = img->line_bytes;

	for(i = 0; i < h; i++, pd += pitch)
	{
		col = mHSVi_to_RGB8pac(i * 360 / h, 255, 255);
		mPixbufBufLineH(img, pd, _HUE_W, mRGBtoPix(col));
	}

	//カーソル

	mPixbufSetPixelModeXor(img);

	_draw_hue_cursor(p);
	_draw_sv_cursor(p);
}

/** 現在のカーソル位置からRGB色セット */

static void _calc_color(mHSVPicker *p)
{
	int h = p->wg.h;

	p->hsv.col = mHSV_to_RGB8pac(
		(double)p->hsv.cur_hue / 60,
		(double)p->hsv.cur_sv_x / (h - 1),
		1.0 - (double)p->hsv.cur_sv_y / (h - 1));
}

/** HUE 変更時 */

static void _change_hue(mHSVPicker *p,int y)
{
	int size = p->wg.h;

	if(y < 0) y = 0;
	else if(y >= size) y = size - 1;

	if(p->hsv.cur_hue_y == y) return;

	_draw_change_hue(p, _Y_TO_HUE(y, size), y);

	_calc_color(p);

	//通知

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
		MHSVPICKER_N_CHANGE_HUE, p->hsv.col, p->hsv.cur_hue);
}

/** SV 位置変更時 */

static void _change_sv(mHSVPicker *p,int x,int y,mlkbool notify)
{
	int size = p->wg.h;

	if(x < 0) x = 0;
	else if(x >= size) x = size - 1;

	if(y < 0) y = 0;
	else if(y >= size) y = size - 1;

	if(x == p->hsv.cur_sv_x && y == p->hsv.cur_sv_y)
		return;

	//SV

	mPixbufSetPixelModeXor(p->hsv.img);

	_draw_sv_cursor(p);

	p->hsv.cur_sv_x = x;
	p->hsv.cur_sv_y = y;

	_draw_sv_cursor(p);

	mWidgetRedraw(MLK_WIDGET(p));

	//色

	_calc_color(p);

	//通知

	if(notify)
	{
		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
			MHSVPICKER_N_CHANGE_SV, p->hsv.col, 0);
	}
}

/** HSV (double) 値から色をセット
 *
 * h: 0.0-6.0 */

static void _set_hsv_color(mHSVPicker *p,double h,double s,double v)
{
	mPixbuf *img = p->hsv.img;
	int i,size;

	size = p->wg.h;

	//カーソル消去

	mPixbufSetPixelModeXor(img);

	_draw_hue_cursor(p);
	_draw_sv_cursor(p);

	//HUE

	i = (int)(h * 60 + 0.5);
	if(i > 359) i %= 360;

	p->hsv.cur_hue = i;

	//HUE: Y 位置

	i = (int)((h / 6.0) * size + 0.5);

	if(i < 0) i = 0;
	else if(i >= size) i = size - 1;

	p->hsv.cur_hue_y = i;

	//SV 位置
	
	p->hsv.cur_sv_x = (int)(s * (size - 1) + 0.5);
	p->hsv.cur_sv_y = (int)((1.0 - v) * (size - 1) + 0.5);

	//

	mPixbufSetPixelModeCol(img);

	_draw_sv(p);

	mPixbufSetPixelModeXor(img);

	_draw_hue_cursor(p);
	_draw_sv_cursor(p);

	mWidgetRedraw(MLK_WIDGET(p));
}


//============================
// main
//============================


/**@ データ削除 */

void mHSVPickerDestroy(mWidget *p)
{
	mPixbufFree(MLK_HSVPICKER(p)->hsv.img);
}

/**@ 作成 */

mHSVPicker *mHSVPickerNew(mWidget *parent,int size,uint32_t fstyle,int height)
{
	mHSVPicker *p;
	
	if(size < sizeof(mHSVPicker))
		size = sizeof(mHSVPicker);
	
	p = (mHSVPicker *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mHSVPickerDestroy;
	p->wg.draw = mHSVPickerHandle_draw;
	p->wg.event = mHSVPickerHandle_event;

	p->wg.flayout |= MLF_FIX_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->wg.w = height + _SPACE_W + _HUE_W;
	p->wg.h = height;

	p->hsv.col = 0xffffff;
	p->hsv.img = mPixbufCreate(p->wg.w, p->wg.h, 0);

	_init_image(p);

	return p;
}

/**@ 作成 */

mHSVPicker *mHSVPickerCreate(mWidget *parent,int id,uint32_t margin_pack,uint32_t fstyle,int height)
{
	mHSVPicker *p;

	p = mHSVPickerNew(parent, 0, fstyle, height);
	if(!p) return NULL;

	p->wg.id = id;

	mWidgetSetMargin_pack4(MLK_WIDGET(p), margin_pack);

	return p;
}

/**@ RGB 色取得 */

mRgbCol mHSVPickerGetRGBColor(mHSVPicker *p)
{
	return p->hsv.col;
}

/**@ 現在の HSV 値取得
 *
 * @p:dst H,S,V の順で3つの値が入る。\
 * H = 0.0-6.0、S,V = 0.0-1.0 */

void mHSVPickerGetHSVColor(mHSVPicker *p,double *dst)
{
	int h = p->wg.h;

	dst[0] = (double)p->hsv.cur_hue / 60;
	dst[1] = (double)p->hsv.cur_sv_x / (h - 1);
	dst[2] = 1.0 - (double)p->hsv.cur_sv_y / (h - 1);
}

/**@ 色相値変更
 *
 * @p:hue 0-359 */

void mHSVPickerSetHue(mHSVPicker *p,int hue)
{
	if(hue < 0)
		hue = 0;
	else if(hue > 359)
		hue %= 360;

	if(hue != p->hsv.cur_hue)
	{
		_draw_change_hue(p, hue, _HUE_TO_Y(hue, p->wg.h));
		_calc_color(p);
	}
}

/**@ SV 値変更
 *
 * @p:s,v 0.0-1.0 */

void mHSVPickerSetSV(mHSVPicker *p,double s,double v)
{
	_change_sv(p,
		(int)(s * (p->wg.h - 1) + 0.5),
		(int)((1.0 - v) * (p->wg.h - 1) + 0.5),
		FALSE);
}

/**@ HSV 値をセット
 *
 * @p:h 0.0-6.0
 * @p:s,v 0.0-1.0 */

void mHSVPickerSetHSVColor(mHSVPicker *p,double h,double s,double v)
{
	p->hsv.col = mHSV_to_RGB8pac(h, s, v);

	_set_hsv_color(p, h, s, v);
}

/**@ RGB 値をセット */

void mHSVPickerSetRGBColor(mHSVPicker *p,mRgbCol col)
{
	mHSVd hsv;

	col &= 0xffffff;

	p->hsv.col = col;

	mRGB8pac_to_HSV(&hsv, col);

	_set_hsv_color(p, hsv.h, hsv.s, hsv.v);
}


//========================
// ハンドラ
//========================


/* グラブ解除 */

static void _release_grab(mHSVPicker *p)
{
	if(p->hsv.fgrab)
	{
		p->hsv.fgrab = 0;
		mWidgetUngrabPointer();
	}
}

/**@ event ハンドラ関数 */

int mHSVPickerHandle_event(mWidget *wg,mEvent *ev)
{
	mHSVPicker *p = MLK_HSVPICKER(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動
				
				if(p->hsv.fgrab == _GRABF_HUE)
					_change_hue(p, ev->pt.y);
				else if(p->hsv.fgrab == _GRABF_SV)
					_change_sv(p, ev->pt.x, ev->pt.y, TRUE);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && p->hsv.fgrab == 0)
				{
					if(ev->pt.x < p->wg.h)
					{
						//SV

						p->hsv.fgrab = _GRABF_SV;
						_change_sv(p, ev->pt.x, ev->pt.y, TRUE);
					}
					else if(ev->pt.x >= p->wg.h + _SPACE_W)
					{
						//H

						p->hsv.fgrab = _GRABF_HUE;
						_change_hue(p, ev->pt.y);
					}

					//grab

					if(p->hsv.fgrab)
						mWidgetGrabPointer(wg);
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//フォーカス
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mHSVPickerHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBlt(pixbuf, 0, 0, MLK_HSVPICKER(wg)->hsv.img, 0, 0, -1, -1);
}

