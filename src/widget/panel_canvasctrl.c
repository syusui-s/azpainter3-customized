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
 * [panel] キャンバス操作
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_event.h"

#include "def_macro.h"
#include "def_widget.h"
#include "def_draw.h"

#include "draw_main.h"

#include "panel.h"
#include "canvas_slider.h"


//------------------------

typedef struct
{
	MLK_CONTAINER_DEF

	CanvasSlider *bar_zoom,
		*bar_angle;
}_topct;

//------------------------


// drawCanvas_update() 時に、バーの値はセットされる。


/* 表示倍率バー */

static void _notify_zoom(_topct *p,mEventNotify *ev)
{
	switch(ev->notify_type)
	{
		//ボタン押し
		case CANVASSLIDER_N_PRESSED_BUTTON:
			if(ev->param1 == CANVASSLIDER_BTT_RESET)
			{
				//リセット
				drawCanvas_update(APPDRAW, 1000, 0,
					DRAWCANVAS_UPDATE_ZOOM | DRAWCANVAS_UPDATE_RESET_SCROLL);
			}
			else
				//一段階変更
				drawCanvas_zoomStep(APPDRAW, (ev->param1 == CANVASSLIDER_BTT_ADD));
			break;
		//バー操作
		default:
			if(ev->notify_type == CANVASSLIDER_N_BAR_PRESS)
				drawCanvas_lowQuality();
			else if(ev->notify_type == CANVASSLIDER_N_BAR_RELEASE)
				drawCanvas_normalQuality();

			//スクロール位置がすでにリセットされている場合は、リセット処理は行われない。

			drawCanvas_update(APPDRAW, ev->param1, 0,
				DRAWCANVAS_UPDATE_ZOOM | DRAWCANVAS_UPDATE_RESET_SCROLL);
			break;
	}
}

/* 回転バー */

static void _notify_angle(_topct *p,mEventNotify *ev)
{
	switch(ev->notify_type)
	{
		//ボタン押し
		case CANVASSLIDER_N_PRESSED_BUTTON:
			if(ev->param1 == CANVASSLIDER_BTT_RESET)
			{
				//リセット
				drawCanvas_update(APPDRAW, 0, 0,
					DRAWCANVAS_UPDATE_ANGLE | DRAWCANVAS_UPDATE_RESET_SCROLL);
			}
			else
				//一段階変更
				drawCanvas_rotateStep(APPDRAW, (ev->param1 == CANVASSLIDER_BTT_SUB));
			break;
		//バー操作
		default:
			if(ev->notify_type == CANVASSLIDER_N_BAR_PRESS)
				drawCanvas_lowQuality();
			else if(ev->notify_type == CANVASSLIDER_N_BAR_RELEASE)
				drawCanvas_normalQuality();

			drawCanvas_update(APPDRAW, 0, ev->param1 * 10,
				DRAWCANVAS_UPDATE_ANGLE | DRAWCANVAS_UPDATE_RESET_SCROLL);
			break;
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_topct *p = (_topct *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.widget_from == MLK_WIDGET(p->bar_zoom))
			_notify_zoom(p, (mEventNotify *)ev);
		else if(ev->notify.widget_from == MLK_WIDGET(p->bar_angle))
			_notify_angle(p, (mEventNotify *)ev);
	}

	return 1;
}

/* mPanel 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	_topct *p;

	//コンテナ

	p = (_topct *)mContainerNew(parent, sizeof(_topct));

	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(2,4,2,0));

	p->ct.sep = 6;

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	//ウィジェット

	p->bar_zoom = CanvasSlider_new(MLK_WIDGET(p), CANVASSLIDER_TYPE_ZOOM, CANVAS_ZOOM_MAX);
	p->bar_angle = CanvasSlider_new(MLK_WIDGET(p), CANVASSLIDER_TYPE_ANGLE, 0);

	CanvasSlider_setValue(p->bar_zoom, APPDRAW->canvas_zoom);
	CanvasSlider_setValue(p->bar_angle, APPDRAW->canvas_angle / 10);

	return (mWidget *)p;
}


//========================
// main
//========================


/** 作成 */

void PanelCanvasCtrl_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_CANVAS_CTRL, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}

/** 倍率/回転の値をセット */

void PanelCanvasCtrl_setValue(void)
{
	_topct *p = (_topct *)Panel_getContents(PANEL_CANVAS_CTRL);

	if(p)
	{
		CanvasSlider_setValue(p->bar_zoom, APPDRAW->canvas_zoom);
		CanvasSlider_setValue(p->bar_angle, APPDRAW->canvas_angle / 10);
	}
}

