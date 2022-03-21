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
 * CanvasSlider
 *
 * 表示倍率・回転角度のスライダーとボタン。
 * キャンバス操作/キャンバスビュー/イメージビューアで使う。
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_string.h"

#include "canvas_slider.h"
#include "appresource.h"


/*------------------
 <通知>
  
 - CANVASSLIDER_N_BAR_* (バーが操作された時)
    param1 : 現在値

 - CANVASSLIDER_N_PRESSED_BUTTON (ボタンが押された時)
    param1 : ボタン番号
 
---------------------*/

//----------------------

struct _CanvasSlider
{
	mWidget wg;

	int type,min,max,pos,fpress;
};

//----------------------

#define CANVASSLIDER(p)  ((CanvasSlider *)(p))

#define _HEIGHT        14	//ボタンの高さでもある
#define _PAD_BAR_LEFT  2	//左の余白
#define _PAD_BAR_RIGHT 4	//バーとボタンの間の余白
#define _BAR_HEIGHT    3
#define _BTT_ALL_W     ((_HEIGHT + 1) * 3 - 1)
#define _BTT_IMGW	   8
#define _BTT_PADDING   ((_HEIGHT - _BTT_IMGW) / 2)

#define _PRESS_F_BAR 1
#define _PRESS_F_BTT 2

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//ボタンイメージ
static const uint8_t g_img1bit_btt[3][8] = {
	{0x20,0x30,0x38,0x3c,0x3c,0x38,0x30,0x20},
	{0x04,0x0c,0x1c,0x3c,0x3c,0x1c,0x0c,0x04},
	{0x00,0x7e,0x7e,0x7e,0x7e,0x7e,0x7e,0x00}
};

//----------------------


/** 作成
 *
 * zoom_max: 表示倍率時の最大値 (1=0.1%) */

CanvasSlider *CanvasSlider_new(mWidget *parent,int type,int zoom_max)
{
	CanvasSlider *p;
	
	p = (CanvasSlider *)mWidgetNew(parent, sizeof(CanvasSlider));
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.flayout = MLF_EXPAND_W;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.hintW = _PAD_BAR_LEFT + _PAD_BAR_RIGHT + _BTT_ALL_W + 5;
	p->wg.hintH = _HEIGHT;

	p->type = type;

	if(type == CANVASSLIDER_TYPE_ZOOM)
	{
		p->min = 1;
		p->max = zoom_max;
		p->pos = 1000;
	}
	else
	{
		p->min = -1800;
		p->max = 1800;
	}
	
	return p;
}

/** 値をセット
 *
 * [!] 最小値・最大値の調整は行わない。 */

void CanvasSlider_setValue(CanvasSlider *p,int val)
{
	if(val != p->pos)
	{
		p->pos = val;
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/** 最大値を取得 */

int CanvasSlider_getMax(CanvasSlider *p)
{
	return p->max;
}


//=============================
// イベント
//=============================


/* 通知 */

static void _send_notify(CanvasSlider *p,int type,int param1)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, type, param1, 0);
}

/* ポインタ位置変更 */

static mlkbool _motion_x(CanvasSlider *p,int x)
{
	int barw,ct,pos;

	x -= _PAD_BAR_LEFT;

	barw = p->wg.w - (_PAD_BAR_LEFT + _PAD_BAR_RIGHT + _BTT_ALL_W);
	ct = barw >> 1;

	if(x < 0) x = 0;
	else if(x >= barw) x = barw - 1;

	//位置

	if(p->type == CANVASSLIDER_TYPE_ANGLE)
	{
		//角度
		
		if(x >= ct)
			pos = (int)((x - ct) * 1800.0 / (barw - 1 - ct) + 0.5);
		else
			pos = (int)((x - ct) * 1800.0 / ct - 0.5);
	}
	else
	{
		//表示倍率

		if(x >= ct)
		{
			//100% 以上 (10% 単位)
			pos = (int)((double)(x - ct) * ((p->max - 1000) / 100) / (barw - 1 - ct) + 0.5);
			pos = pos * 100 + 1000;
		}
		else
			pos = (int)((double)x * (1000 - p->min) / ct + 0.5) + p->min;
	}

	//セット

	if(pos != p->pos)
	{
		p->pos = pos;

		mWidgetRedraw(MLK_WIDGET(p));

		return TRUE;
	}

	return FALSE;
}

/* ポインタボタン押し時 */

static void _event_press(CanvasSlider *p,mEvent *ev)
{
	int i,x;

	x = p->wg.w - _BTT_ALL_W;

	if(ev->pt.x >= x)
	{
		//ボタン

		for(i = 0; i < 3; i++, x += _HEIGHT + 1)
		{
			if(x <= ev->pt.x && ev->pt.x < x + _HEIGHT)
			{
				p->fpress = _PRESS_F_BTT + i;

				mWidgetGrabPointer(MLK_WIDGET(p));
				mWidgetRedraw(MLK_WIDGET(p));
			
				break;
			}
		}
	}
	else
	{
		//バー

		p->fpress = _PRESS_F_BAR;

		mWidgetGrabPointer(MLK_WIDGET(p));

		_motion_x(p, ev->pt.x);
		_send_notify(p, CANVASSLIDER_N_BAR_PRESS, p->pos);
	}
}

/* グラブ解放
 *
 * send: ボタン押し時、イベントを送るか */

static void _release_grab(CanvasSlider *p,mlkbool send)
{
	if(p->fpress)
	{
		mWidgetUngrabPointer();

		if(p->fpress == _PRESS_F_BAR)
			//バー
			_send_notify(p, CANVASSLIDER_N_BAR_RELEASE, p->pos);
		else
		{
			//ボタン
			
			mWidgetRedraw(MLK_WIDGET(p));

			if(send)
				_send_notify(p, CANVASSLIDER_N_PRESSED_BUTTON, p->fpress - _PRESS_F_BTT);
		}

		p->fpress = 0;
	}
}

/* イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	CanvasSlider *p = CANVASSLIDER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動
				
				if(p->fpress == _PRESS_F_BAR)
				{
					if(_motion_x(p, ev->pt.x))
						_send_notify(p, CANVASSLIDER_N_BAR_MOTION, p->pos);
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し
				
				if(ev->pt.btt == MLK_BTT_LEFT && p->fpress == 0)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p, TRUE);
			}
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p, FALSE);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/* 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	CanvasSlider *p = CANVASSLIDER(wg);
	uint8_t m[16],*pm;
	int i,n,barw,center;
	uint32_t col_frame;
	mlkbool fangle,fpress;

	fangle = (p->type == CANVASSLIDER_TYPE_ANGLE);

	barw = wg->w - (_PAD_BAR_LEFT + _PAD_BAR_RIGHT + _BTT_ALL_W);
	center = barw >> 1;

	col_frame = MGUICOL_PIX(FRAME);

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//バー

	mPixbufFillBox(pixbuf, _PAD_BAR_LEFT, 0, barw, _BAR_HEIGHT, col_frame);

	//中央線

	mPixbufLineV(pixbuf, _PAD_BAR_LEFT + center, _BAR_HEIGHT, wg->h - _BAR_HEIGHT, col_frame);

	//カーソル

	if(fangle)
	{
		//角度
		
		n = (p->pos >= 0)? barw - 1 - center: center;
		n = (int)(center + p->pos * n / 1800.0 + 0.5);
	}
	else
	{
		//表示倍率
		
		if(p->pos >= 1000)
			n = (int)(center + (double)(p->pos - 1000) * (barw - 1 - center) / (p->max - 1000) + 0.5);
		else
			n = (int)((double)(p->pos - p->min) * center / (1000 - p->min) + 0.5);
	}

	mPixbufFillBox(pixbuf, _PAD_BAR_LEFT + n - 1, _BAR_HEIGHT + 1,
		3, wg->h - _BAR_HEIGHT - 1, MGUICOL_PIX(TEXT));

	//数値

	mIntToStr_float((char *)m, p->pos, 1);

	for(pm = m; *pm; pm++)
	{
		if(*pm == '-')
			*pm = 10;
		else if(*pm == '.')
			*pm = 11;
		else
			*pm = *pm - '0';
	}

	*(pm++) = 12 + p->type;
	*pm = 255;

	if((fangle && p->pos >= 0) || (!fangle && p->pos >= 1000))
		//左端
		n = _PAD_BAR_LEFT;
	else
		//右端に右寄せ
		n = _PAD_BAR_LEFT + barw - (pm - m) * 5;

	mPixbufDraw1bitPattern_list(pixbuf, n, _HEIGHT - 9,
		AppResource_get1bitImg_number5x9(), APPRES_NUMBER_5x9_WIDTH, 9, 5,
		m, MGUICOL_PIX(TEXT));

	//ボタン

	n = wg->w - _BTT_ALL_W;

	for(i = 0; i < 3; i++)
	{
		fpress = (p->fpress == _PRESS_F_BTT + i);
	
		mPixbufBox(pixbuf, n, 0, _HEIGHT, _HEIGHT, col_frame);
		
		mPixbufFillBox(pixbuf, n + 1, 1, _HEIGHT - 2, _HEIGHT - 2,
			(fpress)? MGUICOL_PIX(BUTTON_FACE_PRESS): MGUICOL_PIX(BUTTON_FACE));
		
		mPixbufDraw1bitPattern(pixbuf,
			n + _BTT_PADDING + fpress, _BTT_PADDING + fpress,
			g_img1bit_btt[i], _BTT_IMGW, _BTT_IMGW,
			MPIXBUF_TPCOL, MGUICOL_PIX(TEXT));

		n += _HEIGHT + 1; 
	}
}
