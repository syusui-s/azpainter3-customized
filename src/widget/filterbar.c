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
 * FilterBar
 *
 * フィルタ用の数値バー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"

#include "filterbar.h"


/*
 * <通知>
 * param1: 位置。
 * param2: ボタン離し時 1。押し時やドラッグ中の位置変更時は 0。
 * 
 */

//-------------------------

struct _FilterBar
{
	mWidget wg;

	int min,max,pos,center,
		fpress;
};

//-------------------------


//========================
// sub
//========================


/* 通知 */

static void _notify(FilterBar *p,int flag)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, p->pos, flag);
}

/* 位置変更 */

static void _change_pos(FilterBar *p,int x)
{
	int pos,ct,w;

	w = p->wg.w;

	if(x < 0) x = 0;
	else if(x >= w) x = w - 1;

	pos = (int)((double)x / (w - 1) * (p->max - p->min) + 0.5) + p->min;

	//中央線に吸着

	if(p->center > p->min)
	{
		ct = (int)((double)(p->center - p->min) / (p->max - p->min) * (w - 1) + 0.5);

		if(x == ct) pos = p->center;
	}

	//変更

	if(pos != p->pos)
	{
		p->pos = pos;

		mWidgetRedraw(MLK_WIDGET(p));

		_notify(p, 0);
	}
}

/* グラブ解除 */

static void _release_grab(FilterBar *p)
{
	if(p->fpress)
	{
		p->fpress = 0;
		mWidgetUngrabPointer();

		_notify(p, 1);
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	FilterBar *p = FILTERBAR(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fpress == 1)
					_change_pos(p, ev->pt.x);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && !p->fpress)
				{
					p->fpress = 1;
					mWidgetGrabPointer(MLK_WIDGET(p));

					_change_pos(p, ev->pt.x);
				}
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
	FilterBar *p = FILTERBAR(wg);
	int w,h,n;

	w = wg->w;
	h = wg->h;

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//枠

	mPixbufBox(pixbuf, 0, 0, w, h, MGUICOL_PIX(FRAME));

	//中央線

	if(p->center > p->min)
	{
		n = (int)((double)(p->center - p->min) / (p->max - p->min) * (w - 1) + 0.5);

		mPixbufLineV(pixbuf, n, 1, h - 2, MGUICOL_PIX(FRAME));
	}

	//カーソル

	n = (int)((double)(p->pos - p->min) / (p->max - p->min) * (w - 1) + 0.5);

	mPixbufFillBox(pixbuf, n - 1, 1, 3, h - 2, MGUICOL_PIX(TEXT));
}


//====================


/** 作成 */

FilterBar *FilterBar_new(mWidget *parent,int id,uint32_t flayout,int initw,
	int min,int max,int pos,int center)
{
	FilterBar *p;

	p = (FilterBar *)mWidgetNew(parent, sizeof(FilterBar));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.flayout = flayout;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.hintW = 20;
	p->wg.hintH = 16;
	p->wg.initW = initw;

	if(pos < min) pos = min;
	else if(pos > max) pos = max;

	p->min = min;
	p->max = max;
	p->pos = pos;
	p->center = center;

	return p;
}

/** 位置を取得 */

int FilterBar_getPos(FilterBar *p)
{
	return p->pos;
}

/** 位置をセット
 *
 * return: 位置が変更されたか */

mlkbool FilterBar_setPos(FilterBar *p,int pos)
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

/** 情報をセット */

void FilterBar_setStatus(FilterBar *p,int min,int max,int pos)
{
	p->min = min;
	p->max = max;
	p->pos = pos;

	mWidgetRedraw(MLK_WIDGET(p));
}
