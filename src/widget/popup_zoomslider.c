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

/**************************************************
 * PopupZoomSlider
 * 
 * キャンバスビュー/イメージビューアの倍率変更用。
 * ポップアップのスライダー
 **************************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"

#include "canvas_slider.h"

/*
 - ハンドラは、値が変更したときのみ呼ばれ、
   バーのドラッグ中は、タイマーによって指定間隔で更新される。
*/

//--------------------------

typedef struct
{
	MLK_POPUP_DEF

	CanvasSlider *slider;
	int curval;

	void *param;
	void (*handle)(int,void*);
}PopupZoomSlider;

#define _POPUP_WIDTH  300
#define _ZOOMUP_STEP  50  //100%以上で拡大時の一段階 (1=1%)

//--------------------------


/* 値変更 */

static mlkbool _change_val(PopupZoomSlider *p,int val,mlkbool fdrag)
{
	if(p->curval == val)
		return FALSE;
	else
	{
		p->curval = val;

		if(fdrag)
			//ドラッグ中の場合は、20ms ごとに更新
			mWidgetTimerAdd_ifnothave(MLK_WIDGET(p), 0, 20, 0);
		else
			(p->handle)(val, p->param);

		return TRUE;
	}
}

/* 一段階変更 */

static int _get_step(PopupZoomSlider *p,mlkbool fadd)
{
	int val,n,max;

	val = p->curval;
	n = val / 10;
	max = CanvasSlider_getMax(p->slider);

	//n : 1 = 1%

	if((!fadd && n <= 100) || (fadd && n < 100))
	{
		//100% 以下
		// :拡大時は現在値の 115%、縮小時は 85%

		if(fadd)
		{
			val = (int)(val * 1.15 + 0.5);
			if(val > 1000) val = 1000;
		}
		else
		{
			//値が小さくて変化しない場合は -1
			
			n = (int)(val * 0.85 + 0.5);
			if(n == val) n--;

			val = n;
		}
	}
	else
	{
		//100% 以上

		if(fadd)
			n = (n + _ZOOMUP_STEP) / _ZOOMUP_STEP * _ZOOMUP_STEP;
		else
		{
			n = (n - 1) / _ZOOMUP_STEP * _ZOOMUP_STEP;
			if(n < 100) n = 100;
		}

		val = n * 10;
	}

	if(val < 1) val = 1;
	else if(val > max) val = max;

	return val;
}

/* ボタン操作 */

static void _pressed_button(PopupZoomSlider *p,int btt)
{
	int n;

	switch(btt)
	{
		//リセット
		case CANVASSLIDER_BTT_RESET:
			n = 1000;
			break;
		//+
		case CANVASSLIDER_BTT_ADD:
			n = _get_step(p, TRUE);
			break;
		//-
		case CANVASSLIDER_BTT_SUB:
			n = _get_step(p, FALSE);
			break;
	}

	if(_change_val(p, n, FALSE))
		CanvasSlider_setValue(p->slider, n);
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	PopupZoomSlider *p = (PopupZoomSlider *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == MLK_WIDGET(p->slider))
			{
				switch(ev->notify.notify_type)
				{
					//移動
					case CANVASSLIDER_N_BAR_MOTION:
						_change_val(p, ev->notify.param1, TRUE);
						break;
					//押し
					case CANVASSLIDER_N_BAR_PRESS:
						_change_val(p, ev->notify.param1, FALSE);
						break;
					//離し
					case CANVASSLIDER_N_BAR_RELEASE:
						//タイマーが残っている場合は、ハンドラ実行
						
						if(mWidgetTimerDelete(wg, 0))
							(p->handle)(p->curval, p->param);
						break;
					//ボタン
					case CANVASSLIDER_N_PRESSED_BUTTON:
						_pressed_button(p, ev->notify.param1);
						break;
				}
			}
			break;

		//タイマー
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			(p->handle)(p->curval, p->param);
			break;
	}

	return mPopupEventDefault(wg, ev);
}

/** ポップアップ実行
 *
 * 表示倍率の値は、1=0.1%
 *
 * max : 表示倍率最大値
 * handle : (int zoom,void *param)
 * return : 結果の表示倍率が返る */

int PopupZoomSlider_run(mWidget *parent,mBox *box,
	int max,int current,void (*handle)(int,void *),void *param)
{
	PopupZoomSlider *p;
	int ret;

	//作成

	p = (PopupZoomSlider *)mPopupNew(sizeof(PopupZoomSlider), MPOPUP_S_NO_GRAB);
	if(!p) return current;

	p->wg.draw = mWidgetDrawHandle_bkgndAndFrame;
	p->wg.event = _event_handle;

	p->handle = handle;
	p->param = param;
	p->curval = current;

	mContainerSetPadding_same(MLK_CONTAINER(p), 4);

	//CanvasSlider

	p->slider = CanvasSlider_new(MLK_WIDGET(p), CANVASSLIDER_TYPE_ZOOM, max);

	CanvasSlider_setValue(p->slider, current);

	//実行

	mPopupRun(MLK_POPUP(p), parent, 0, 0,
		_POPUP_WIDTH, MLK_WIDGET(p->slider)->hintH + 8, box,
		MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_Y);

	//終了

	ret = p->curval;

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}
