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
 * キャンバスビューの表示エリア
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "def_config.h"
#include "def_draw.h"
#include "canvasinfo.h"

#include "imagecanvas.h"

#include "draw_main.h"

#include "pv_panel_canvview.h"


//-------------------

typedef struct _CanvViewPage
{
	mWidget wg;

	CanvViewEx *ex;
	
	int dragbtt,	//ドラッグ時の操作ボタン
		cmd,		//現在の操作コマンド
		lastx,lasty;
}CanvViewPage;

//-------------------

#define _PAGE(p)   ((CanvViewPage *)(p))

#define _TOGGLEBAR_WIDTH  12
#define _TOGGLEBAR_HEIGHT 7

enum
{
	_BTTCMD_VIEW_SCROLL,
	_BTTCMD_CANVAS_SCROLL,
	_BTTCMD_VIEW_ZOOM,
	_BTTCMD_MENU
};

//-------------------


//==========================
// sub
//==========================


/* CanvView 取得 */

static CanvView *_get_canvview(CanvViewPage *p)
{
	return (CanvView *)p->wg.parent;
}

/* 押されたボタンから操作コマンド取得
 *
 * return: -1 でなし */

static int _get_cmd_from_button(int btt,uint32_t state)
{
	int no = -1;

	if(btt == MLK_BTT_LEFT)
	{
		//左ボタン
		
		if(state & MLK_STATE_CTRL)
			no = CONFIG_PANELVIEW_BTT_LEFT_CTRL;
		else if(state & MLK_STATE_SHIFT)
			no = CONFIG_PANELVIEW_BTT_LEFT_SHIFT;
		else
			no = CONFIG_PANELVIEW_BTT_LEFT;
	}
	else if(btt == MLK_BTT_RIGHT)
		//右ボタン
		no = CONFIG_PANELVIEW_BTT_RIGHT;
	else if(btt == MLK_BTT_MIDDLE)
		no = CONFIG_PANELVIEW_BTT_MIDDLE;

	if(no < 0) return -1;

	//コマンド

	no = APPCONF->canvview.bttcmd[no];

	//全体表示時:スクロールと倍率変更は無効

	if(_IS_ZOOM_FIT
		&& (no == _BTTCMD_VIEW_SCROLL || no == _BTTCMD_VIEW_ZOOM))
		return -1;

	return no;
}

/* ビュースクロール処理 */

static void _proc_view_scroll(CanvViewPage *p,mEventPointer *ev)
{
	CanvViewEx *ex = p->ex;
	int sx,sy;

	sx = ex->pt_scroll.x;
	sy = ex->pt_scroll.y;

	ex->pt_scroll.x += p->lastx - ev->x;
	ex->pt_scroll.y += p->lasty - ev->y;

	CanvView_adjustScroll(_get_canvview(p));

	if(sx != ex->pt_scroll.x || sy != ex->pt_scroll.y)
		mWidgetRedraw(MLK_WIDGET(p));
}

/* キャンバススクロール */

static void _proc_canvas_scroll(CanvViewPage *p,int x,int y)
{
	mDoublePoint pt;

	CanvView_canvas_to_image(_get_canvview(p), &pt, x, y);

	drawCanvas_scroll_at_imagecenter(APPDRAW, &pt);
}

/* ドラッグ時:倍率変更 */

static void _proc_zoom(CanvViewPage *p,int y)
{
	int dir;
	double d;

	//上移動で拡大、下移動で縮小
	
	dir = p->lasty - y;
	if(dir == 0) return;

	d = p->ex->dscale * ((dir > 0)? 1.1: 0.95);

	CanvView_setZoom(_get_canvview(p), 0, d);
}


//==========================
// ハンドラ
//==========================


/* ボタン押し時 */

static void _event_press(CanvViewPage *p,mEventPointer *ev)
{
	if(!_IS_TOOLBAR_ALWAYS
		&& ev->btt == MLK_BTT_LEFT
		&& ev->x < _TOGGLEBAR_WIDTH && ev->y < _TOGGLEBAR_HEIGHT)
	{
		//ツールバー切り替え

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, PAGE_NOTIFY_TOGGLE_TOOLBAR, 0, 0);
	}
	else
	{
		//ボタン操作
		
		int cmd;

		cmd = _get_cmd_from_button(ev->btt, ev->state);
		if(cmd < 0) return;

		if(cmd == _BTTCMD_MENU)
		{
			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, PAGE_NOTIFY_MENU, ev->x, ev->y);
			return;
		}

		p->dragbtt = ev->btt;
		p->cmd = cmd;
		p->lastx = ev->x;
		p->lasty = ev->y;

		mWidgetGrabPointer(MLK_WIDGET(p));

		switch(cmd)
		{
			//キャンバススクロール
			case _BTTCMD_CANVAS_SCROLL:
				drawCanvas_lowQuality();
				
				_proc_canvas_scroll(p, ev->x, ev->y);
				break;
			//表示倍率
			case _BTTCMD_VIEW_ZOOM:
				CanvView_setImageCenter(_get_canvview(p), FALSE);
				break;
		}
	}
}

/* ポインタ移動時 */

static void _event_motion(CanvViewPage *p,mEventPointer *ev)
{
	switch(p->cmd)
	{
		//ビュースクロール
		case _BTTCMD_VIEW_SCROLL:
			_proc_view_scroll(p, ev);
			break;

		//キャンバススクロール
		case _BTTCMD_CANVAS_SCROLL:
			_proc_canvas_scroll(p, ev->x, ev->y);
			break;

		//倍率変更
		case _BTTCMD_VIEW_ZOOM:
			_proc_zoom(p, ev->y);
			break;
	}

	p->lastx = ev->x;
	p->lasty = ev->y;
}

/* ドラッグ解除 */

static void _release_drag(CanvViewPage *p)
{
	if(p->dragbtt)
	{
		p->dragbtt = 0;

		mWidgetUngrabPointer();

		if(p->cmd == _BTTCMD_CANVAS_SCROLL)
		{
			//キャンバス、元の品質に戻す
			drawCanvas_normalQuality();
			drawUpdate_canvas();
		}
		else
			//スクロール/表示倍率時、元の品質に戻す
			mWidgetRedraw(MLK_WIDGET(p));
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	CanvViewPage *p = _PAGE(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->dragbtt)
					_event_motion(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(!p->dragbtt)
					_event_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == p->dragbtt)
					_release_drag(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_drag(p);
			break;
	}

	return 1;
}

/* 描画ハンドラ */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	CanvViewPage *p = _PAGE(wg);
	CanvViewEx *ex = p->ex;
	CanvasDrawInfo info;
	CanvasViewParam param;
	mBox box;

	if(APPDRAW->in_thread_imgcanvas
		|| mWidgetGetDrawBox(wg, &box) == -1)
		return;

	//imgcanvas を作業用に更新した場合、
	//途中でキャンバスビューが更新されると、作業状態のイメージが残るため、
	//最後に再更新させる。

	APPDRAW->is_canvview_update = TRUE;

	//キャンバス

	info.boxdst = box;
	info.originx = ex->ptimgct.x;
	info.originy = ex->ptimgct.y;
	info.scrollx = ex->pt_scroll.x - wg->w / 2;
	info.scrolly = ex->pt_scroll.y - wg->h / 2;
	info.mirror = ex->mirror;
	info.imgw = APPDRAW->imgw;
	info.imgh = APPDRAW->imgh;
	info.bkgndcol = APPCONF->canvasbkcol;
	info.param = &param;

	param.scalediv = ex->dscalediv;

	if(ex->zoom != 1000 && ex->zoom < 2000 && p->dragbtt == 0)
		//縮小時 (ドラッグ時は除く)
		ImageCanvas_drawPixbuf_oversamp(APPDRAW->imgcanvas, pixbuf, &info);
	else
		ImageCanvas_drawPixbuf_nearest(APPDRAW->imgcanvas, pixbuf, &info);

	//ツールバー切り替えバー

	if(!_IS_TOOLBAR_ALWAYS
		&& box.x < _TOGGLEBAR_WIDTH && box.y < _TOGGLEBAR_HEIGHT)
	{
		mPixbufBox(pixbuf, 0, 0, _TOGGLEBAR_WIDTH, _TOGGLEBAR_HEIGHT, MGUICOL_PIX(WHITE));

		mPixbufFillBox(pixbuf, 1, 1, _TOGGLEBAR_WIDTH - 2, _TOGGLEBAR_HEIGHT - 2, 0);
	}
}


//==========================
// main
//==========================


/* サイズ変更時 */

static void _resize_handle(mWidget *wg)
{
	CanvView_setZoom_fit(_get_canvview(_PAGE(wg)));
}

/** 作成 */

CanvViewPage *CanvViewPage_new(mWidget *parent,CanvViewEx *ex)
{
	CanvViewPage *p;

	p = (CanvViewPage *)mWidgetNew(parent, sizeof(CanvViewPage));

	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.resize = _resize_handle;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->ex = ex;

	return p;
}

