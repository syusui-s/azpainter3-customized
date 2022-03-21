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
 * ImgViewerPage
 * 
 * [panel] イメージビューアの画像表示部分
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_menu.h"

#include "def_config.h"

#include "image32.h"
#include "draw_main.h"
#include "panel_func.h"
#include "trid.h"

#include "pv_panel_imgviewer.h"


/*
 - ドラッグ中はニアレストネイバーで描画される。
*/

//-------------------

struct _ImgViewerPage
{
	mWidget wg;

	ImgViewerEx *ex;
	int dragbtt,	//ドラッグ中の場合、ボタン番号
		dragcmd,	//ドラッグ中の場合、操作コマンド
		lastx,lasty;
};

//-------------------

#define _IS_ZOOM_FIT  (APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_FIT)

#define _PAGE(p)    ((ImgViewerPage *)(p))
#define _BKGND_COL  0xb0b0b0
#define TRID_SPOITMENU_TOP  200

//操作コマンド
enum
{
	_BTTCMD_SCROLL,
	_BTTCMD_ZOOM,
	_BTTCMD_SPOIT_DRAWCOL,
	_BTTCMD_SPOIT_BKGNDCOL,
	_BTTCMD_SPOIT_MENU
};

//-------------------


//==========================
// sub
//==========================


/* ImgViewer 取得 */

static ImgViewer *_get_imgviewer(ImgViewerPage *p)
{
	return (ImgViewer *)p->wg.parent;
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

	no = APPCONF->imgviewer.bttcmd[no];

	//全体表示時:スクロールと倍率変更は無効

	if(_IS_ZOOM_FIT
		&& (no == _BTTCMD_SCROLL || no == _BTTCMD_ZOOM))
		return -1;

	return no;
}

/* スクロール処理 */

static void _proc_scroll(ImgViewerPage *p,mEventPointer *ev)
{
	ImgViewerEx *ex = p->ex;
	int sx,sy;

	sx = ex->scrx;
	sy = ex->scry;

	ex->scrx += p->lastx - ev->x;
	ex->scry += p->lasty - ev->y;

	ImgViewer_adjustScroll(_get_imgviewer(p));

	if(sx != ex->scrx || sy != ex->scry)
		mWidgetRedraw(MLK_WIDGET(p));
}

/* ドラッグで表示倍率変更 */

static void _proc_drag_zoom(ImgViewerPage *p,mEventPointer *ev)
{
	int n,dir;

	//上移動で拡大、下移動で縮小
	
	dir = p->lasty - ev->y;
	if(dir == 0) return;

	//

	n = p->ex->zoom;

	if(dir > 0)
		n = (int)(n * 1.1 + 0.5);
	else
		n = (int)(n * 0.95 + 0.5);

	if(n < 1) n = 1;
	else if(n > IMGVIEWER_ZOOM_MAX) n = IMGVIEWER_ZOOM_MAX;

	ImgViewer_setZoom(_get_imgviewer(p), n);
}

/* 指定位置の色を取得
 *
 * return: -1 で範囲外 */

static int32_t _get_color(ImgViewerPage *p,int x,int y)
{
	uint8_t *buf;
	mDoublePoint dpt;

	//キャンバス -> イメージ位置

	ImgViewer_canvas_to_image(_get_imgviewer(p), &dpt, x, y);

	//色取得

	buf = Image32_getPixelBuf(p->ex->img, dpt.x, dpt.y);
	if(buf)
		return MLK_RGB(buf[0], buf[1], buf[2]);
	else
		return -1;
}

/* スポイトして色をセット */

static void _spoit_setcolor(ImgViewerPage *p,int x,int y,mlkbool bkgnd)
{
	int32_t col;

	col = _get_color(p, x, y);
	if(col == -1) return;

	if(bkgnd)
		drawColor_setBkgndColor(col);
	else
		drawColor_setDrawColor(col);

	PanelColor_changeDrawColor();
}

/* スポイトメニュー実行 */

static void _run_spoitmenu(ImgViewerPage *p,int x,int y)
{
	mMenu *menu;
	mMenuItem *mi;
	int i;
	int32_t col;

	col = _get_color(p, x, y);
	if(col == -1) return;

	//メニュー

	MLK_TRGROUP(TRGROUP_PANEL_IMAGEVIEWER);

	menu = mMenuNew();

	for(i = 0; i < 2; i++)
		mMenuAppendText(menu, i, MLK_TR(TRID_SPOITMENU_TOP + i));

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP, NULL);

	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(i == -1) return;

	//セット

	_spoit_setcolor(p, x, y, (i == 1));
}

/* スポイト処理 */

static void _cmd_spoit(ImgViewerPage *p,int x,int y,int cmd)
{
	switch(cmd)
	{
		//描画色
		case _BTTCMD_SPOIT_DRAWCOL:
			_spoit_setcolor(p, x, y, FALSE);
			break;
		//背景色
		case _BTTCMD_SPOIT_BKGNDCOL:
			_spoit_setcolor(p, x, y, TRUE);
			break;
		//メニュー
		case _BTTCMD_SPOIT_MENU:
			_run_spoitmenu(p, x, y);
			break;
	}
}


//==========================
// イベント
//==========================


/* ボタン押し時 (イメージがある時) */

static void _event_press(ImgViewerPage *p,mEventPointer *ev)
{
	int cmd;

	cmd = _get_cmd_from_button(ev->btt, ev->state);
	if(cmd < 0) return;

	if(cmd == _BTTCMD_SCROLL || cmd == _BTTCMD_ZOOM)
	{
		//スクロール or 表示倍率変更

		p->dragbtt = ev->btt;
		p->dragcmd = cmd;
		p->lastx = ev->x;
		p->lasty = ev->y;

		if(cmd == _BTTCMD_ZOOM)
			ImgViewer_setImageCenter(_get_imgviewer(p), FALSE);

		mWidgetGrabPointer(MLK_WIDGET(p));
	}
	else
		//スポイト
		_cmd_spoit(p, ev->x, ev->y, cmd);
}

/* ポインタ移動時 */

static void _event_motion(ImgViewerPage *p,mEventPointer *ev)
{
	if(p->dragcmd == _BTTCMD_SCROLL)
		//スクロール
		_proc_scroll(p, ev);
	else
		//倍率変更
		_proc_drag_zoom(p, ev);

	p->lastx = ev->x;
	p->lasty = ev->y;
}

/* ドラッグ解除 */

static void _release_drag(ImgViewerPage *p)
{
	if(p->dragbtt)
	{
		mWidgetUngrabPointer();
		p->dragbtt = 0;

		//ドラッグ中はニアレストネイバーのため、再描画
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	ImgViewerPage *p = _PAGE(wg);

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
				//押し (イメージがある時)

				if(p->dragbtt == 0 && p->ex->img)
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

		//ファイルドロップ
		case MEVENT_DROP_FILES:
			ImgViewer_loadImage(_get_imgviewer(p), *(ev->dropfiles.files));
			break;
	}

	return 1;
}


//==========================
// ハンドラ
//==========================


/* サイズ変更時 */

static void _resize_handle(mWidget *wg)
{
	ImgViewer_setZoom_fit(_get_imgviewer(_PAGE(wg)));
}

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ImgViewerPage *p = _PAGE(wg);
	ImgViewerEx *ex = p->ex;

	if(!ex->img)
		mWidgetDrawBkgnd(wg, NULL);
	else
	{
		Image32CanvasInfo info;
		mBox box;

		box.x = box.y = 0;
		box.w = wg->w;
		box.h = wg->h;

		info.origin_x = ex->ptimgct.x;
		info.origin_y = ex->ptimgct.y;
		info.scroll_x = ex->scrx - (wg->w >> 1);
		info.scroll_y = ex->scry - (wg->h >> 1);
		info.scalediv = ex->dscalediv;
		info.mirror = APPCONF->imgviewer.flags & CONFIG_IMAGEVIEWER_F_MIRROR;
		info.bkgndcol = _BKGND_COL;

		if(ex->zoom < 1000 && p->dragbtt == 0)
			//縮小時 (ドラッグ中は除く)
			Image32_drawCanvas_oversamp(ex->img, pixbuf, &box, &info);
		else
			Image32_drawCanvas_nearest(ex->img, pixbuf, &box, &info);
	}
}


//==========================
// main
//==========================


/** ImgViewerPage 作成 */

ImgViewerPage *ImgViewerPage_new(mWidget *parent,ImgViewerEx *dat)
{
	ImgViewerPage *p;

	p = (ImgViewerPage *)mWidgetNew(parent, sizeof(ImgViewerPage));

	p->ex = dat;

	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.resize = _resize_handle;

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fstate |= MWIDGET_STATE_ENABLE_DROP;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	return p;
}
