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
 * [panel] キャンバスビュー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_panel.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_menu.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"

#include "panel.h"
#include "appresource.h"

#include "pv_panel_canvview.h"

#include "trid.h"


//-------------------

enum
{
	//TRID と同じ
	CMDID_ZOOM,
	CMDID_FIT,
	CMDID_FLIP_HORZ,
	CMDID_TOOLBAR_ALWAYS,
	CMDID_OPTION,

	CMDID_MENU
};

enum
{
	//ツールチップ
	TRID_TB_MENU = 100,
	TRID_TB_ZOOM,
	TRID_TB_FIT,
	TRID_TB_FLIP_HORZ
};

//メニューデータ

static const uint16_t g_menudat[] = {
	CMDID_ZOOM, MMENU_ARRAY16_SEP,
	CMDID_FIT, CMDID_FLIP_HORZ, CMDID_TOOLBAR_ALWAYS, MMENU_ARRAY16_SEP,
	CMDID_OPTION,
	MMENU_ARRAY16_END
};

//-------------------

int PopupZoomSlider_run(mWidget *parent,mBox *box,
	int max,int current,void (*handle)(int,void *),void *param);

void PanelViewer_optionDlg_run(mWindow *parent,uint8_t *dstbuf,mlkbool is_imageviewer);

//-------------------



//==============================
// sub
//==============================


/* ページ更新 */

static void _update_page(CanvView *p)
{
	CanvView_setZoom_fit(p);
	
	mWidgetRedraw(MLK_WIDGET(p->page));
}

/* mIconBar ボタンの有効/無効セット */

static void _iconbar_enable(CanvView *p)
{
	//表示倍率 (全体表示の時無効)
	
	mIconBarSetEnable(p->iconbar, CMDID_ZOOM, !_IS_ZOOM_FIT);
}

/* (EX) スクロール位置をリセット */

static void _scroll_reset(CanvViewEx *ex)
{
	ex->ptimgct.x = APPDRAW->imgw * 0.5;
	ex->ptimgct.y = APPDRAW->imgh * 0.5;
	ex->pt_scroll.x = ex->pt_scroll.y = 0;
}


//==============================
// コマンド
//==============================


/* ツールバー切り替え */

static void _toggle_toolbar(CanvView *p)
{
	APPCONF->canvview.flags ^= CONFIG_CANVASVIEW_F_TOOLBAR_VISIBLE;

	mWidgetShow(MLK_WIDGET(p->iconbar), _IS_TOOLBAR_VISIBLE);

	mWidgetReLayout(MLK_WIDGET(p));
}

/* 全体表示切り替え */

static void _cmd_toggle_fit(CanvView *p)
{
	APPCONF->canvview.flags ^= CONFIG_CANVASVIEW_F_FIT;

	CanvView_setImageCenter(p, TRUE);

	_iconbar_enable(p);
	_update_page(p);
}

/* 左右反転切り替え */

static void _cmd_toggle_mirror(CanvView *p)
{
	CanvView_setImageCenter(p, FALSE);

	p->ex->mirror ^= 1;
	
	_update_page(p);
}

/* ツールバーは常に表示 */

static void _cmd_toggle_toolbar_always(CanvView *p)
{
	int f;

	APPCONF->canvview.flags ^= CONFIG_CANVASVIEW_F_TOOLBAR_ALWAYS;

	f = _IS_TOOLBAR_VISIBLE;

	if(_IS_TOOLBAR_ALWAYS)
	{
		//[常に表示] 表示時はバーを消去。非表示時は、表示
		
		if(f)
			mWidgetRedraw(MLK_WIDGET(p->page));
		else
			_toggle_toolbar(p);
	}
	else
	{
		//[切り替え可] バーを表示

		mWidgetRedraw(MLK_WIDGET(p->page));
	}
}

/* メニュー表示
 *
 * x: -1 でツールバーから。それ以外で、ページ上の位置 */

static void _run_menu(CanvView *p,int x,int y)
{
	mMenu *menu;
	mBox box;
	int zoom_fit;

	zoom_fit = _IS_ZOOM_FIT;

	MLK_TRGROUP(TRGROUP_PANEL_CANVAS_VIEW);

	//メニュー作成

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menudat);

	mMenuSetItemEnable(menu, CMDID_ZOOM, !zoom_fit);

	mMenuSetItemCheck(menu, CMDID_FIT, zoom_fit);
	mMenuSetItemCheck(menu, CMDID_FLIP_HORZ, p->ex->mirror);
	mMenuSetItemCheck(menu, CMDID_TOOLBAR_ALWAYS, _IS_TOOLBAR_ALWAYS);

	//実行

	if(x == -1)
	{
		//ツールバーから
		
		mIconBarGetItemBox(p->iconbar, CMDID_MENU, &box);

		mMenuPopup(menu, MLK_WIDGET(p->iconbar), 0, 0, &box,
			MPOPUP_F_RIGHT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY | MPOPUP_F_MENU_SEND_COMMAND,
			MLK_WIDGET(p));
	}
	else
	{
		//ページのボタン操作から
		
		mMenuPopup(menu, MLK_WIDGET(p->page), x, y, NULL,
			MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY | MPOPUP_F_MENU_SEND_COMMAND,
			MLK_WIDGET(p));
	}

	mMenuDestroy(menu);
}


//==============================
// 表示倍率スライダー
//==============================


/* 表示倍率スライダー:倍率変更時 */

static void _popupslider_handle(int zoom,void *param)
{
	CanvView_setZoom((CanvView *)param, zoom, 0);
}

/* 表示倍率スライダー実行 */

static void _popupslider_run(CanvView *p,mlkbool toolbar)
{
	mWidget *wg;
	mBox box;

	//全体表示時は無効

	if(_IS_ZOOM_FIT) return;

	CanvView_setImageCenter(p, FALSE);

	//

	if(toolbar)
	{
		wg = MLK_WIDGET(p->iconbar);
		mIconBarGetItemBox(p->iconbar, CMDID_ZOOM, &box);
	}
	else
	{
		wg = MLK_WIDGET(p->page);
		mWidgetGetBox_rel(wg, &box);
	}
	
	PopupZoomSlider_run(wg, &box,
		CANVVIEW_ZOOM_MAX, p->ex->zoom, _popupslider_handle, p);
}


//==============================
// ハンドラ
//==============================


/* コマンド */

static void _event_command(CanvView *p,mEventCommand *ev)
{
	switch(ev->id)
	{
		//メニュー
		case CMDID_MENU:
			_run_menu(p, -1, -1);
			break;
		//表示倍率
		case CMDID_ZOOM:
			_popupslider_run(p, (ev->from == MEVENT_COMMAND_FROM_ICONBAR_BUTTON));
			break;
		//全体表示
		case CMDID_FIT:
			_cmd_toggle_fit(p);

			if(ev->from != MEVENT_COMMAND_FROM_ICONBAR_BUTTON)
				mIconBarSetCheck(p->iconbar, CMDID_FIT, -1);
			break;
		//左右反転表示
		case CMDID_FLIP_HORZ:
			_cmd_toggle_mirror(p);

			if(ev->from != MEVENT_COMMAND_FROM_ICONBAR_BUTTON)
				mIconBarSetCheck(p->iconbar, CMDID_FLIP_HORZ, -1);
			break;
		//ツールバーは常に表示
		case CMDID_TOOLBAR_ALWAYS:
			_cmd_toggle_toolbar_always(p);
			break;
		//設定
		case CMDID_OPTION:
			PanelViewer_optionDlg_run(MLK_WINDOW(p->wg.toplevel),
				APPCONF->canvview.bttcmd, FALSE);
			break;
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	CanvView *p = (CanvView *)wg;

	switch(ev->type)
	{
		//page からの通知
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == (mWidget *)p->page)
			{
				switch(ev->notify.notify_type)
				{
					//ツールバー切り替え
					case PAGE_NOTIFY_TOGGLE_TOOLBAR:
						_toggle_toolbar(p);
						break;
					//メニュー
					case PAGE_NOTIFY_MENU:
						_run_menu(p, ev->notify.param1, ev->notify.param2);
						break;
				}
			}
			break;
	
		//ツールバー/メニュー
		case MEVENT_COMMAND:
			_event_command(p, (mEventCommand *)ev);
			break;
	}

	return 1;
}


//==============================
// main
//==============================


/* パネル内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	CanvView *p;
	CanvViewEx *ex;
	mIconBar *ib;

	ex = (CanvViewEx *)mPanelGetExPtr(panel);

	//トップコンテナ

	p = (CanvView *)mContainerNew(parent, sizeof(CanvView));

	mContainerSetType_horz(MLK_CONTAINER(p), 0);

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;
	p->wg.draw = NULL;

	p->ex = ex;

	//mIconBar

	p->iconbar = ib = mIconBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0, MICONBAR_S_TOOLTIP | MICONBAR_S_VERT);

	mIconBarSetImageList(ib, APPRES->imglist_icon_other);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_PANEL_CANVAS_VIEW);

	mIconBarAdd(ib, CMDID_MENU, APPRES_OTHERICON_HOME, TRID_TB_MENU, 0);
	mIconBarAdd(ib, CMDID_ZOOM, APPRES_OTHERICON_ZOOM, TRID_TB_ZOOM, 0);
	mIconBarAdd(ib, CMDID_FIT, APPRES_OTHERICON_FIT, TRID_TB_FIT, MICONBAR_ITEMF_CHECKBUTTON);
	mIconBarAdd(ib, CMDID_FLIP_HORZ, APPRES_OTHERICON_FLIP_HORZ, TRID_TB_FLIP_HORZ, MICONBAR_ITEMF_CHECKBUTTON);

	mIconBarSetCheck(ib, CMDID_FIT, _IS_ZOOM_FIT);
	mIconBarSetCheck(ib, CMDID_FLIP_HORZ, ex->mirror);

	_iconbar_enable(p);

	mWidgetShow(MLK_WIDGET(ib), _IS_TOOLBAR_VISIBLE);

	//表示エリア

	p->page = CanvViewPage_new(MLK_WIDGET(p), ex);

	return MLK_WIDGET(p);
}

/** 初期作成 */

void PanelCanvasView_new(mPanelState *state)
{
	mPanel *p;
	CanvViewEx *ex;

	//CanvasView 作成

	p = Panel_new(PANEL_CANVAS_VIEW, sizeof(CanvViewEx), 0, 0,
		state, _panel_create_handle);

	//初期化

	ex = (CanvViewEx *)mPanelGetExPtr(p);

	ex->zoom = APPCONF->canvview.zoom;
	ex->dscale = ex->zoom / 1000.0;
	ex->dscalediv = 1.0 / ex->dscale;

	//

	mPanelCreateWidget(p);
}

/** 表示倍率セット (全体表示でない時)
 *
 * zoom: 0 で dscale を使う */

void CanvView_setZoom(CanvView *p,int zoom,double dscale)
{
	CanvViewEx *ex = p->ex;

	if(zoom)
	{
		//整数値
		
		if(zoom == ex->zoom) return;

		dscale = (double)zoom / 1000;
	}
	else
	{
		//dscale

		if(dscale < 0.001) dscale = 0.001;
		else if(dscale > 20) dscale = 20;

		zoom = (int)(dscale * 1000 + 0.5);
		if(zoom < 1) zoom = 1;
	}

	//

	ex->zoom = zoom;
	ex->dscale = dscale;
	ex->dscalediv = 1.0 / ex->dscale;

	APPCONF->canvview.zoom = zoom;

	CanvView_adjustScroll(p);

	mWidgetRedraw(MLK_WIDGET(p->page));
}

/** 表示倍率をウィンドウに合わせる */

void CanvView_setZoom_fit(CanvView *p)
{
	CanvViewEx *ex = p->ex;
	double h,v;

	if(!_IS_ZOOM_FIT) return;

	//倍率が低い方 (拡大もあり)

	h = (double)MLK_WIDGET(p->page)->w / APPDRAW->imgw;
	v = (double)MLK_WIDGET(p->page)->h / APPDRAW->imgh;

	if(h > v) h = v;

	if(h < 0.001) h = 0.001;
	else if(h > 16) h = 16;

	ex->zoom = (int)(h * 1000 + 0.5);
	ex->dscale = h;
	ex->dscalediv = 1.0 / h;

	ex->pt_scroll.x = ex->pt_scroll.y = 0;
}

/** イメージ -> キャンバス位置 */

void CanvView_image_to_canvas(CanvView *p,mPoint *dst,double x,double y)
{
	CanvViewEx *ex = p->ex;

	x = (x - ex->ptimgct.x) * ex->dscale;
	y = (y - ex->ptimgct.y) * ex->dscale;

	if(ex->mirror) x = -x;

	dst->x = x + MLK_WIDGET(p->page)->w / 2 - ex->pt_scroll.x;
	dst->y = y + MLK_WIDGET(p->page)->h / 2 - ex->pt_scroll.y;
}

/** キャンバス -> イメージ位置 */

void CanvView_canvas_to_image(CanvView *p,mDoublePoint *dst,int x,int y)
{
	CanvViewEx *ex = p->ex;

	x = x - MLK_WIDGET(p->page)->w / 2 + ex->pt_scroll.x;
	y = y - MLK_WIDGET(p->page)->h / 2 + ex->pt_scroll.y;

	if(ex->mirror) x = -x;

	dst->x = x * ex->dscalediv + ex->ptimgct.x;
	dst->y = y * ex->dscalediv + ex->ptimgct.y;
}

/** 画像の中心位置をセット
 *
 * reset: TRUE でリセット、FALSE で現在の表示中央 */

void CanvView_setImageCenter(CanvView *p,mlkbool reset)
{
	CanvViewEx *ex = p->ex;

	if(reset)
		_scroll_reset(ex);
	else
	{
		CanvView_canvas_to_image(p, &ex->ptimgct,
			MLK_WIDGET(p->page)->w / 2, MLK_WIDGET(p->page)->h / 2);

		ex->pt_scroll.x = ex->pt_scroll.y = 0;
	}
}

/** スクロール位置調整 */

void CanvView_adjustScroll(CanvView *p)
{
	CanvViewEx *ex = p->ex;
	mDoublePoint dpt;
	mPoint pt1,pt2;
	int imgw,imgh,cw,ch;

	if(_IS_ZOOM_FIT) return;

	imgw = APPDRAW->imgw;
	imgh = APPDRAW->imgh;
	cw = MLK_WIDGET(p->page)->w / 2;
	ch = MLK_WIDGET(p->page)->h / 2;

	//キャンバス中央のイメージ位置

	CanvView_canvas_to_image(p, &dpt, cw, ch);

	//イメージ端のキャンバス位置

	CanvView_image_to_canvas(p, &pt1, 0, 0);
	CanvView_image_to_canvas(p, &pt2, imgw, imgh);

	//調整

	if(dpt.x < 0)
		ex->pt_scroll.x = pt1.x + ex->pt_scroll.x - cw;
	else if(dpt.x > imgw)
		ex->pt_scroll.x = pt2.x + ex->pt_scroll.x - cw;

	if(dpt.y < 0)
		ex->pt_scroll.y = pt1.y + ex->pt_scroll.y - ch;
	else if(dpt.y > imgh)
		ex->pt_scroll.y = pt2.y + ex->pt_scroll.y - ch;
}


//==============================
// 外部からの呼び出し
//==============================


/* CanvView 取得 */

static CanvView *_get_canvview(void)
{
	return (CanvView *)Panel_getContents(PANEL_CANVAS_VIEW);
}

/** イメージサイズが変わった時 */

void PanelCanvasView_changeImageSize(void)
{
	CanvViewEx *ex = Panel_getExPtr(PANEL_CANVAS_VIEW);
	CanvView *p = _get_canvview();

	//スクロール位置は常にリセット
	// :閉じた状態でイメージが変わった後に表示した時、正しい位置になるように

	_scroll_reset(ex);

	//ウィジェットが作成されている時

	if(p)
		_update_page(p);
}

/** イメージ全体を更新 */

void PanelCanvasView_update(void)
{
	CanvView *p;

	if(Panel_isVisible(PANEL_CANVAS_VIEW))
	{
		p = _get_canvview();
		
		mWidgetRedraw(MLK_WIDGET(p->page));
	}
}

/* 指定イメージの範囲のみ更新 */

static void _update_imgbox(CanvView *p,const mBox *boximg)
{
	mPoint pt1,pt2;
	mBox box;
	int cw,ch;

	cw = MLK_WIDGET(p->page)->w;
	ch = MLK_WIDGET(p->page)->h;

	//イメージ -> キャンバス座標

	CanvView_image_to_canvas(p, &pt1, boximg->x, boximg->y);
	CanvView_image_to_canvas(p, &pt2, boximg->x + boximg->w, boximg->y + boximg->h);

	pt1.x--, pt1.y--;
	pt2.x++, pt2.y++;

	//キャンバス範囲外判定

	if(pt2.x < 0 || pt2.y < 0 || pt1.x >= cw || pt1.y >= ch)
		return;

	//調整

	if(pt1.x < 0) pt1.x = 0;
	if(pt1.y < 0) pt1.y = 0;
	if(pt2.x >= cw) pt2.x = cw - 1;
	if(pt2.y >= ch) pt2.y = ch - 1;

	//更新

	box.x = pt1.x, box.y = pt1.y;
	box.w = pt2.x - pt1.x + 1;
	box.h = pt2.y - pt1.y + 1;

	mWidgetRedrawBox(MLK_WIDGET(p->page), &box);	
}

/** 指定範囲更新
 *
 * boximg: イメージの範囲 */

void PanelCanvasView_updateBox(const mBox *boximg)
{
	if(Panel_isVisible(PANEL_CANVAS_VIEW))
	{
		CanvView *p = _get_canvview();
		
		_update_imgbox(p, boximg);
	}
}

