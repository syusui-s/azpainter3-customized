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
 * ValueBar
 * スピンと数値表示付きのバー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_string.h"

#include "valuebar.h"
#include "appresource.h"


/*------------------

 - [Ctrl+左右ドラッグ] で相対移動。
 - ホイールで +1/-1。
 
 <通知>
 param1: 位置。
 param2: ボタンが離された/スピン時、1。押し時やドラッグ中は0。
 
-----------------*/

//-------------------------

struct _ValueBar
{
	mWidget wg;

	int min,max,pos,dig,
		fpress,lastx;
};

#define _SPIN_W  8  //枠1px含む
#define _SPIN_ALL_W  (_SPIN_W * 2) //右端の枠は含まない

enum
{
	_PRESS_F_DRAG = 1,
	_PRESS_F_DRAG_REL
};

//-------------------------


//========================
// sub
//========================


/* 位置変更 & 通知
 *
 * only_change: 位置が変更したときのみ通知 */

static void _change_pos(ValueBar *p,int pos,int flags,mlkbool only_change)
{
	if(ValueBar_setPos(p, pos))
		flags |= VALUEBAR_ACT_F_CHANGE_POS;
	else
	{
		if(only_change) return;
	}

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, p->pos, flags);
}

/* バーのポインタ位置から、位置取得 */

static int _get_pos(ValueBar *p,int x)
{
	return (int)((double)x / (p->wg.w - 2 - _SPIN_ALL_W) * (p->max - p->min) + 0.5) + p->min;
}

/* 押し時 */

static void _event_press(ValueBar *p,mEvent *ev)
{
	if(ev->pt.x >= p->wg.w - 1 - _SPIN_ALL_W)
	{
		//スピン

		_change_pos(p, p->pos + ((ev->pt.x >= p->wg.w - 1 - _SPIN_W)? 1: -1),
			VALUEBAR_ACT_F_SPIN, TRUE);
	}
	else
	{
		//バー部分

		p->fpress = (ev->pt.state & MLK_STATE_CTRL)? _PRESS_F_DRAG_REL: _PRESS_F_DRAG;
		p->lastx = ev->pt.x;

		mWidgetGrabPointer(MLK_WIDGET(p));

		if(p->fpress == _PRESS_F_DRAG)
			_change_pos(p, _get_pos(p, ev->pt.x), VALUEBAR_ACT_F_PRESS, FALSE);
	}
}

/* グラブ解除 */

static void _release_grab(ValueBar *p)
{
	if(p->fpress)
	{
		p->fpress = 0;
		mWidgetUngrabPointer();

		_change_pos(p, p->pos, VALUEBAR_ACT_F_RELEASE, FALSE);
	}
}


//========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	ValueBar *p = VALUEBAR(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fpress == _PRESS_F_DRAG)
				{
					//ドラッグで位置移動
					
					_change_pos(p, _get_pos(p, ev->pt.x), VALUEBAR_ACT_F_DRAG, TRUE);
				}
				else if(p->fpress == _PRESS_F_DRAG_REL)
				{
					//+Ctrl ドラッグ:相対移動

					_change_pos(p, ev->pt.x - p->lastx + p->pos, VALUEBAR_ACT_F_DRAG, TRUE);

					p->lastx = ev->pt.x;
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && !p->fpress)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//ホイール
		case MEVENT_SCROLL:
			if(ev->scroll.vert_step && !p->fpress)
				_change_pos(p, p->pos - ev->scroll.vert_step, VALUEBAR_ACT_F_WHEEL, TRUE);
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
	}

	return TRUE;
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ValueBar *p = VALUEBAR(wg);
	uint8_t m[16],*pm;
	int w,h,n,n2,n3;
	mPixCol colframe,coltext;
	mlkbool enabled;

	w = wg->w;
	h = wg->h;
	enabled = mWidgetIsEnable(wg);

	colframe = mGuiCol_getPix(enabled? MGUICOL_FRAME: MGUICOL_FRAME_DISABLE);
	coltext = mGuiCol_getPix(enabled? MGUICOL_TEXT: MGUICOL_TEXT_DISABLE);

	//枠

	mPixbufBox(pixbuf, 0, 0, w, h, colframe);

	//---- スピン

	n = w - 1 - _SPIN_ALL_W;
	n2 = h - 2;
	n3 = (h - 3) / 2;

	//背景

	mPixbufFillBox(pixbuf, n, 1, _SPIN_ALL_W, n2, MGUICOL_PIX(FACE_DARK));

	//下

	mPixbufLineV(pixbuf, n, 1, n2, colframe);
	mPixbufDrawArrowDown(pixbuf, n + 2, n3 + 1, 3, coltext);

	//上

	mPixbufLineV(pixbuf, n + _SPIN_W, 1, h - 2, colframe);
	mPixbufDrawArrowUp(pixbuf, n + _SPIN_W + 2, n3, 3, coltext);

	//---- バー

	n2 = w - 2 - _SPIN_ALL_W;

	if(enabled)
		n = (int)((double)(p->pos - p->min) * n2 / (p->max - p->min) + 0.5);
	else
		n = 0; //無効時はバー無し

	if(n)
	{
		mPixbufFillBox(pixbuf, 1, 1, n, h - 2,
			mGuiCol_getPix(enabled? MGUICOL_BUTTON_FACE_DEFAULT: MGUICOL_FACE_DISABLE));
	}

	//バー残り

	n2 -= n;

	if(n2 > 0)
	{
		mPixbufFillBox(pixbuf, 1 + n, 1, n2, h - 2,
			mGuiCol_getPix(enabled? MGUICOL_FACE_TEXTBOX: MGUICOL_FACE));
	}

	//------ 数値

	mIntToStr_float((char *)m, p->pos, p->dig);

	for(pm = m; *pm; pm++)
	{
		if(*pm == '-')
			*pm = 10;
		else if(*pm == '.')
			*pm = 11;
		else
			*pm -= '0';
	}

	*pm = 255;

	mPixbufDraw1bitPattern_list(pixbuf,
		w - 4 - _SPIN_ALL_W - (pm - m) * 5, 3,
		AppResource_get1bitImg_number5x9(), APPRES_NUMBER_5x9_WIDTH, 9, 5,
		m, coltext);
}


//====================
// main
//====================


/** 作成 */

ValueBar *ValueBar_new(mWidget *parent,int id,uint32_t flayout,
	int dig,int min,int max,int pos)
{
	ValueBar *p;

	p = (ValueBar *)mWidgetNew(parent, sizeof(ValueBar));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.flayout = flayout;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.hintW = _SPIN_ALL_W + 5;
	p->wg.hintH = 14;

	if(pos < min) pos = min;
	else if(pos > max) pos = max;

	p->dig = dig;
	p->min = min;
	p->max = max;
	p->pos = pos;

	return p;
}

/** 位置を取得 */

int ValueBar_getPos(ValueBar *p)
{
	return p->pos;
}

/** 位置をセット
 *
 * return: 位置が変更されたか */

mlkbool ValueBar_setPos(ValueBar *p,int pos)
{
	if(pos < p->min)
		pos = p->min;
	else if(pos > p->max)
		pos = p->max;

	if(pos == p->pos)
		return FALSE;
	else
	{
		p->pos = pos;

		mWidgetRedraw(MLK_WIDGET(p));

		return TRUE;
	}
}

/** 範囲と位置をセット */

void ValueBar_setStatus(ValueBar *p,int min,int max,int pos)
{
	p->min = min;
	p->max = max;
	p->pos = pos;

	mWidgetRedraw(MLK_WIDGET(p));
}

/** 小数点以下桁数指定付き */

void ValueBar_setStatus_dig(ValueBar *p,int dig,int min,int max,int pos)
{
	p->dig = dig;
	p->min = min;
	p->max = max;
	p->pos = pos;

	mWidgetRedraw(MLK_WIDGET(p));
}
