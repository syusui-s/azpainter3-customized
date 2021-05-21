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
 * キャンバスイメージ上の位置取得ダイアログ
 * (透過色時)
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_combobox.h"
#include "mlk_scrollview.h"
#include "mlk_scrollbar.h"
#include "mlk_event.h"
#include "mlk_str.h"

#include "def_draw.h"

#include "imagecanvas.h"
#include "canvasinfo.h"

#include "widget_func.h"

#include "trid.h"


//-------------------

typedef struct
{
	MLK_DIALOG_DEF

	mPoint ptpos;
	int zoom;
	double dscalediv;

	mScrollView *view;
	mScrollViewPage *page;
	mLabel *label_pos;
	mComboBox *cb_zoom;
}_dialog;

enum
{
	WID_PAGE = 100,
	WID_CB_ZOOM
};

//-------------------



//=============================
// mScrollViewPage
//=============================
/* param1 = (double *) scalediv */


/* ボタン押し時 */

static void _page_on_press(mScrollViewPage *p,mEvent *ev)
{
	mPoint ptscr;
	double scalediv;
	int x,y;

	mScrollViewPage_getScrollPos(p, &ptscr);

	scalediv = *((double *)p->wg.param1);

	x = (int)((ptscr.x + ev->pt.x) * scalediv);
	y = (int)((ptscr.y + ev->pt.y) * scalediv);

	if(x < 0) x = 0;
	if(y < 0) y = 0;

	if(x >= APPDRAW->imgw || y >= APPDRAW->imgh)
		return;

	mWidgetEventAdd_notify(MLK_WIDGET(p), MLK_WIDGET(p->wg.toplevel), 0, x, y);
}

/* イベントハンドラ */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == wg->parent
				&& (ev->notify.notify_type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ
					|| ev->notify.notify_type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_VERT)
				&& (ev->notify.param2 & MSCROLLBAR_ACTION_F_CHANGE_POS))
			{
				mWidgetRedraw(wg);
			}
			break;
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
				_page_on_press(MLK_SCROLLVIEWPAGE(wg), ev);
			break;
	}

	return 1;
}

/* 描画ハンドラ */

static void _page_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mScrollViewPage *p = MLK_SCROLLVIEWPAGE(wg);
	CanvasDrawInfo di;
	CanvasViewParam param;
	mPoint ptscr;

	mScrollViewPage_getScrollPos(p, &ptscr);

	param.scalediv = *((double *)wg->param1);

	di.boxdst.x = 0;
	di.boxdst.y = 0;
	di.boxdst.w = wg->w;
	di.boxdst.h = wg->h;
	
	di.originx = 0;
	di.originy = 0;
	di.scrollx = ptscr.x;
	di.scrolly = ptscr.y;
	di.mirror  = 0;
	di.imgw = APPDRAW->imgw;
	di.imgh = APPDRAW->imgh;
	di.bkgndcol = 0x888888;
	di.param = &param;

	ImageCanvas_drawPixbuf_nearest(APPDRAW->imgcanvas, pixbuf, &di);
}


//=============================
// _dialog
//=============================


/* スクロール情報のセット */

static void _set_scrollinfo(_dialog *p)
{
	mScrollViewSetScrollStatus_horz(p->view, 0, APPDRAW->imgw * p->zoom,
		p->page->wg.w);

	mScrollViewSetScrollStatus_vert(p->view, 0, APPDRAW->imgh * p->zoom,
		p->page->wg.h);
}

/* 位置のラベルセット */

static void _set_pos_label(_dialog *p)
{
	mStr str = MSTR_INIT;

	mStrSetFormat(&str, "%d, %d", p->ptpos.x, p->ptpos.y);

	mLabelSetText(p->label_pos, str.buf);

	mStrFree(&str);
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//位置変更
				case WID_PAGE:
					p->ptpos.x = ev->notify.param1;
					p->ptpos.y = ev->notify.param2;
					
					_set_pos_label(p);
					break;
				//表示倍率
				case WID_CB_ZOOM:
					p->zoom = ev->notify.param2;
					p->dscalediv = 1.0 / ev->notify.param2;

					_set_scrollinfo(p);
					mWidgetRedraw(MLK_WIDGET(p->page));
					break;
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,mPoint *ptdst)
{
	_dialog *p;
	mWidget *ct;
	mScrollView *view;
	mScrollViewPage *page;
	mComboBox *cb;
	int zoom[] = {1,4,8,16};

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR2(TRGROUP_DLG_TITLES, TRID_DLG_TITLE_SET_IMAGE_POINT),
		_event_handle);
		
	if(!p) return NULL;

	p->ptpos = *ptdst;
	p->zoom = 1;
	p->dscalediv = 1;

	//---- scrollview

	//view

	p->view = view = mScrollViewNew(MLK_WIDGET(p), 0,
		MSCROLLVIEW_S_FIX_HORZVERT | MSCROLLVIEW_S_FRAME);

	view->wg.flayout = MLF_EXPAND_WH;
	view->wg.initW = 400;
	view->wg.initH = 400;

	//page

	p->page = page = mScrollViewPageNew(MLK_WIDGET(view), 0);

	page->wg.id = WID_PAGE;
	page->wg.draw = _page_draw_handle;
	page->wg.event = _page_event_handle;
	page->wg.fevent |= MWIDGET_EVENT_POINTER;

	page->wg.param1 = (intptr_t)&p->dscalediv;

	mScrollViewSetPage(view, page);

	//scroll

	_set_scrollinfo(p);

	//----- 下部

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0));

	//倍率

	cb = mComboBoxCreate(ct, WID_CB_ZOOM, MLF_MIDDLE, 0, 0);

	mComboBoxAddItems_sepnull_arrayInt(cb, "x1\0x4\0x8\0x16\0", zoom);
	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, 0);

	//label

	p->label_pos = mLabelCreate(ct, MLF_EXPAND_W | MLF_MIDDLE, MLK_MAKE32_4(5,0,5,0),
		MLABEL_S_COPYTEXT | MLABEL_S_BORDER, NULL);

	_set_pos_label(p);

	//OK/Cancel

	mContainerAddButtons_okcancel(ct);

	return p;
}

/** ダイアログ実行 */

mlkbool CanvasImagePointDlg_run(mWindow *parent,mPoint *ptdst)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent, ptdst);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		*ptdst = p->ptpos;

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

