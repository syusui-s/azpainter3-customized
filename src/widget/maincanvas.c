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

/********************************************
 * MainCanvas/MainCanvasPage
 *
 * メインウィンドウのキャンバス部分ウィジェット
 ********************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_scrollbar.h"
#include "mlk_scrollview.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_key.h"
#include "mlk_rectbox.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_maincanvas.h"

#include "mainwindow.h"
#include "maincanvas.h"

#include "layeritem.h"
#include "appcursor.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_main.h"


//----------------

void FilterDlg_onCanvasClick(double x,double y);

//----------------


/***************************************
 * [MainCanvasPage] 領域部分
 ***************************************/


enum
{
	_TIMERID_UPDATE_PAGE = 1,
	_TIMERID_UPDATE_RECT,
	_TIMERID_UPDATE_MOVE,
	_TIMERID_UPDATE_MOVE_SELECT,
	_TIMERID_UPDATE_MOVE_SELECT_IMAGE,
	_TIMERID_UPDATE_PASTE_MOVE,
	_TIMERID_SCROLL,
	_TIMERID_LAYERNAME
};


/* グラブする */

static void _page_grab(MainCanvasPage *p)
{
	//グラブ
	
	mWidgetGrabPointer(MLK_WIDGET(p));

	//ドラッグ中のカーソル指定がある場合

	if(APPDRAW->w.drag_cursor_type != -1)
	{
		p->cursor_drag_restore = p->wg.cursor;
		p->is_have_drag_cursor = TRUE;
		
		mWidgetSetCursor(MLK_WIDGET(p), AppCursor_getForDrag(APPDRAW->w.drag_cursor_type));
	}
}

/* グラブ解放 */

static void _page_ungrab(MainCanvasPage *p)
{
	mWidgetUngrabPointer();

	//ドラッグ中のカーソル元に戻す

	if(p->is_have_drag_cursor)
	{
		p->is_have_drag_cursor = FALSE;
		
		mWidgetSetCursor(MLK_WIDGET(p), p->cursor_drag_restore);
	}
}


//======================


/* タイマー処理 */

static void _page_event_timer(MainCanvasPage *p,int id)
{
	//タイマー削除
	
	mWidgetTimerDelete(MLK_WIDGET(p), id);

	//

	switch(id)
	{
		//範囲更新
		case _TIMERID_UPDATE_RECT:
			drawUpdateBox_canvas(APPDRAW, &p->box_update);
			p->box_update.x = -1;
			break;

		//全体更新
		case _TIMERID_UPDATE_PAGE:
			mWidgetRedraw(MLK_WIDGET(p));
			break;

		//イメージ移動
		// :AppDraw::w.rcdraw の範囲を更新して、範囲をクリア
		case _TIMERID_UPDATE_MOVE:
			drawUpdateRect_canvas(APPDRAW, &APPDRAW->w.rcdraw);
			mRectEmpty(&APPDRAW->w.rcdraw);
			break;

		//選択範囲の位置移動
		case _TIMERID_UPDATE_MOVE_SELECT:
			drawUpdateRect_canvaswg_forSelect(APPDRAW, &APPDRAW->w.rcdraw);
			mRectEmpty(&APPDRAW->w.rcdraw);
			break;

		//選択範囲イメージ移動
		case _TIMERID_UPDATE_MOVE_SELECT_IMAGE:
			drawUpdateRect_canvas_forSelect(APPDRAW, &APPDRAW->w.rcdraw);
			mRectEmpty(&APPDRAW->w.rcdraw);
			break;

		//矩形選択の貼り付け移動
		case _TIMERID_UPDATE_PASTE_MOVE:
			drawUpdateRect_canvas_forBoxSel(APPDRAW, &APPDRAW->w.rcdraw);
			mRectEmpty(&APPDRAW->w.rcdraw);
			break;

		//レイヤ名
		case _TIMERID_LAYERNAME:
			mWidgetDestroy(MLK_WIDGET(p->ttip_layername));
			p->ttip_layername = NULL;
			break;
	}
}

/* キー押し時 */

static void _page_event_keydown(MainCanvasPage *p,mEventKey *ev)
{
	//スペースキー
	
	if(ev->key == MKEY_SPACE || ev->key == MKEY_KP_SPACE)
		p->is_pressed_space = TRUE;

	//現在押されているキーを記録 (キャンバスキー用)

	if(ev->raw_code < 256)
		p->pressed_rawkey = ev->raw_code;

	//

	if(APPDRAW->w.optype)
	{
		//操作中時、各ツールに対応したキー処理

		if(drawOp_onKeyDown(APPDRAW, ev->key))
			_page_ungrab(p);
	}
	else if(!ev->is_grab_pointer && ev->raw_code < 256)
	{
		//キャンバスキーのコマンド実行 (非操作時)

		MainWindow_runCanvasKeyCmd(APPCONF->canvaskey[ev->raw_code]);
	}
}

/* イベントハンドラ */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	MainCanvasPage *p = (MainCanvasPage *)wg;

	switch(ev->type)
	{
		//ポインタデバイス
		case MEVENT_PENTABLET:
			switch(ev->pentab.act)
			{
				//移動
				case MEVENT_POINTER_ACT_MOTION:
					drawOp_onMotion(APPDRAW, ev);
					break;
				//ダブルクリック
				case MEVENT_POINTER_ACT_DBLCLK:
					//戻り値
					//  TRUE  => ダブルクリックとして処理し、グラブ解除
					//  FALSE => クリック扱いにするため、このまま PRESS の処理へ
					if(ev->pentab.btt == MLK_BTT_LEFT
						&& drawOp_onLBttDblClk(APPDRAW, ev))
					{
						_page_ungrab(p);
						break;
					}
				//押し
				case MEVENT_POINTER_ACT_PRESS:
					if(drawOp_onPress(APPDRAW, ev))
						_page_grab(p);
					break;
				//離し
				case MEVENT_POINTER_ACT_RELEASE:
					if(drawOp_onRelease(APPDRAW, ev))
						_page_ungrab(p);
					break;
			}
			break;

		//ポインタデバイス (ダイアログ表示中)
		case MEVENT_PENTABLET_MODAL:
			//左クリック
			if(ev->pentab.act == MEVENT_POINTER_ACT_PRESS && ev->pentab.btt == MLK_BTT_LEFT)
			{
				if(APPDRAW->text.in_dialog)
				{
					//テキストダイアログ
					drawText_setPoint_inDialog(APPDRAW, ev->pentab.x, ev->pentab.y);
				}
				else if(APPDRAW->in_filter_dialog)
				{
					//フィルタダイアログ
					FilterDlg_onCanvasClick(ev->pentab.x, ev->pentab.y);
				}
			}
			break;

		//タイマー
		case MEVENT_TIMER:
			_page_event_timer(p, ev->timer.id);
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			_page_event_keydown(p, (mEventKey *)ev);
			break;

		//キー離し
		case MEVENT_KEYUP:
			if(ev->key.key == MKEY_SPACE || ev->key.key == MKEY_KP_SPACE)
				p->is_pressed_space = FALSE;

			if(ev->key.raw_code == p->pressed_rawkey)
				p->pressed_rawkey = -1;
			break;

		//フォーカスが別のウィンドウに移った or 予期せぬグラブ解除
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
			{
				drawOp_onUngrab(APPDRAW);

				_page_ungrab(p);

				p->is_pressed_space = FALSE;
				p->pressed_rawkey = -1;
			}
			break;
		
		//カーソルが離れた
		// :矩形範囲でリサイズカーソル状態になっている場合、元に戻す
		case MEVENT_LEAVE:
			drawBoxSel_restoreCursor(APPDRAW);
			break;

		//ファイルドロップ
		case MEVENT_DROP_FILES:
			if(MainWindow_confirmSave(APPWIDGET->mainwin))
				MainWindow_loadImage(APPWIDGET->mainwin, *(ev->dropfiles.files), NULL);
			break;
	}

	return 1;
}

/* 描画ハンドラ */

static void _page_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mBox box;

	if(!APPDRAW->in_thread_imgcanvas)
	{
		if(mWidgetGetDrawBox(wg, &box) != -1)
			drawUpdate_drawCanvas(APPDRAW, pixbuf, &box);
	}
}

/* サイズ変更ハンドラ */

static void _page_resize_handle(mWidget *wg)
{
	drawCanvas_setCanvasSize(APPDRAW, wg->w, wg->h);
}

/* MainCanvasPage 作成 */

static MainCanvasPage *_create_page(mWidget *parent)
{
	MainCanvasPage *p;

	p = (MainCanvasPage *)mScrollViewPageNew(parent, sizeof(MainCanvasPage));
	if(!p) return NULL;

	APPWIDGET->canvaspage = p;

	p->wg.draw = _page_draw_handle;
	p->wg.event = _page_event_handle;
	p->wg.resize = _page_resize_handle;
	
	p->wg.fevent |= MWIDGET_EVENT_PENTABLET | MWIDGET_EVENT_KEY;
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_DROP;
	p->wg.facceptkey = MWIDGET_ACCEPTKEY_ENTER | MWIDGET_ACCEPTKEY_ESCAPE;
	p->wg.foption |= MWIDGET_OPTION_SCROLL_TO_POINTER;

	p->box_update.x = -1;
	p->pressed_rawkey = -1;

	//カーソル

	MainCanvasPage_setCursor_forTool();

	return p;
}

/** スペースキーが押されているか */

mlkbool MainCanvasPage_isPressed_space(void)
{
	return (APPWIDGET->canvaspage)->is_pressed_space;
}

/** 現在押されているキーの生番号取得
 *
 * return: -1 でなし */

int MainCanvasPage_getPressedRawKey(void)
{
	return (APPWIDGET->canvaspage)->pressed_rawkey;
}

/** レイヤ名のツールチップ表示 */

void MainCanvasPage_showLayerName(void)
{
	MainCanvasPage *p = APPWIDGET->canvaspage;

	//前回表示した時からレイヤが変わっていなければ、表示しない

	if(APPDRAW->ttip_layer != APPDRAW->curlayer)
	{
		mWidgetTimerDelete(MLK_WIDGET(p), _TIMERID_LAYERNAME);

		APPDRAW->ttip_layer = APPDRAW->curlayer;

		p->ttip_layername = mTooltipShow(p->ttip_layername, MLK_WIDGET(p),
			APPDRAW->w.pt_canv_last.x, APPDRAW->w.pt_canv_last.y - 15, NULL,
			MPOPUP_F_GRAVITY_TOP, APPDRAW->curlayer->name, 0);

		mWidgetTimerAdd(MLK_WIDGET(p), _TIMERID_LAYERNAME, 500, 0);
	}
}


//==========================
// カーソル
//==========================


/** ツール用のカーソル形状セット */

void MainCanvasPage_setCursor_forTool(void)
{
	int no = drawCursor_getToolCursor(APPDRAW->tool.no);

	mWidgetSetCursor(MLK_WIDGET(APPWIDGET->canvaspage), AppCursor_getForCanvas(no));
}

/** AppCursor の指定カーソルセット (ドラッグ中の変更時) */

void MainCanvasPage_setCursor(int curno)
{
	mWidgetSetCursor(MLK_WIDGET(APPWIDGET->canvaspage), AppCursor_getForCanvas(curno));
}

/** 環境設定で描画カーソルが変更された時 */

void MainCanvasPage_changeDrawCursor(void)
{
	mWidget *wg = MLK_WIDGET(APPWIDGET->canvaspage);

	mWidgetSetCursor(wg, 0);

	AppCursor_setDrawCursor(APPCONF->strCursorFile.buf,
		APPCONF->cursor_hotspot[0], APPCONF->cursor_hotspot[1]);

	MainCanvasPage_setCursor_forTool();
}


//============================
// タイマーセット/クリア
//============================


/** 領域更新セット */

void MainCanvasPage_setTimer_updatePage(int time)
{
	mWidgetTimerAdd_ifnothave(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_PAGE, time, 0);
}

/** 領域更新クリア */

void MainCanvasPage_clearTimer_updatePage(void)
{
	mWidgetTimerDelete(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_PAGE);
}

/** タイマーセット:範囲更新 (描画更新用) */

void MainCanvasPage_setTimer_updateRect(mBox *boximg)
{
	MainCanvasPage *p = APPWIDGET->canvaspage;

	//タイマーが存在する場合はそのまま

	if(drawCalc_unionImageBox(&p->box_update, boximg))
		mWidgetTimerAdd_ifnothave(MLK_WIDGET(p), _TIMERID_UPDATE_RECT, 2, 0);
}

/** 範囲更新タイマーをクリア
 *
 * update: 残っている範囲を更新するか */

void MainCanvasPage_clearTimer_updateRect(mlkbool update)
{
	MainCanvasPage *p = APPWIDGET->canvaspage;

	if(mWidgetTimerDelete(MLK_WIDGET(p), _TIMERID_UPDATE_RECT))
	{
		if(update)
			drawUpdateBox_canvas(APPDRAW, &p->box_update);
	}

	p->box_update.x = -1;
}

/** イメージの移動処理の更新タイマーをセット */

void MainCanvasPage_setTimer_updateMove(void)
{
	mWidgetTimerAdd_ifnothave(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_MOVE, 5, 0);
}

/** イメージの移動処理の更新タイマーをクリア */

void MainCanvasPage_clearTimer_updateMove(void)
{
	mWidgetTimerDelete(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_MOVE);
}

/** 選択範囲の位置移動のタイマーセット */

void MainCanvasPage_setTimer_updateSelectMove(void)
{
	mWidgetTimerAdd_ifnothave(MLK_WIDGET(APPWIDGET->canvaspage),
		_TIMERID_UPDATE_MOVE_SELECT, 5, 0);
}

/** 選択範囲の位置移動のタイマークリア */

void MainCanvasPage_clearTimer_updateSelectMove(void)
{
	if(mWidgetTimerDelete(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_MOVE_SELECT))
		drawUpdateRect_canvaswg_forSelect(APPDRAW, &APPDRAW->w.rcdraw);
}

/** 選択範囲イメージ移動のタイマーセット */

void MainCanvasPage_setTimer_updateSelectImageMove(void)
{
	mWidgetTimerAdd_ifnothave(MLK_WIDGET(APPWIDGET->canvaspage),
		_TIMERID_UPDATE_MOVE_SELECT_IMAGE, 5, 0);
}

/** 選択範囲イメージ移動のタイマークリア
 *
 * return: TRUE で更新した */

mlkbool MainCanvasPage_clearTimer_updateSelectImageMove(void)
{
	if(mWidgetTimerDelete(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_MOVE_SELECT_IMAGE))
	{
		drawUpdateRect_canvas_forSelect(APPDRAW, &APPDRAW->w.rcdraw);
		return TRUE;
	}

	return FALSE;
}

/** 矩形範囲貼り付け移動のタイマーセット */

void MainCanvasPage_setTimer_updatePasteMove(void)
{
	mWidgetTimerAdd_ifnothave(MLK_WIDGET(APPWIDGET->canvaspage),
		_TIMERID_UPDATE_PASTE_MOVE, 5, 0);
}

void MainCanvasPage_clearTimer_updatePasteMove(void)
{
	if(mWidgetTimerDelete(MLK_WIDGET(APPWIDGET->canvaspage), _TIMERID_UPDATE_PASTE_MOVE))
		drawUpdateRect_canvas_forBoxSel(APPDRAW, &APPDRAW->w.rcdraw);
}


/*********************************
 * [MainCanvas] (mScrollView)
 *********************************/


//========================
// sub
//========================


/* スクロール処理 */

static void _view_scroll(MainCanvas *p,int pos,uint32_t flags,mlkbool vert)
{
	if(flags & MSCROLLBAR_ACTION_F_PRESS)
		//ドラッグ開始 (ドラッグ中は低品質表示)
		drawCanvas_lowQuality();
	else if(flags & MSCROLLBAR_ACTION_F_RELEASE)
	{
		//ドラッグ終了
		drawCanvas_normalQuality();
		drawUpdate_canvas();
	}

	//位置変更

	if(flags & MSCROLLBAR_ACTION_F_CHANGE_POS)
	{
		if(vert)
			APPDRAW->canvas_scroll.y = pos - p->pt_scroll_base.y;
		else
			APPDRAW->canvas_scroll.x = pos - p->pt_scroll_base.x;

		drawCanvas_update_scrollpos(APPDRAW, TRUE);
	}
}

/* イベントハンドラ */

static int _view_event_handle(mWidget *wg,mEvent *ev)
{
	MainCanvas *p = (MainCanvas *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == MLK_WIDGET(p->sv.scrh))
			{
				//水平スクロール

				if(ev->notify.notify_type == MSCROLLBAR_N_ACTION)
					_view_scroll(p, ev->notify.param1, ev->notify.param2, FALSE);
			}
			else if(ev->notify.widget_from == MLK_WIDGET(p->sv.scrv))
			{
				//垂直スクロール

				if(ev->notify.notify_type == MSCROLLBAR_N_ACTION)
					_view_scroll(p, ev->notify.param1, ev->notify.param2, TRUE);
			}
			break;
	}

	return 1;
}


/** MainCanvas 作成 */

void MainCanvas_new(mWidget *parent)
{
	MainCanvas *p;

	p = (MainCanvas *)mScrollViewNew(parent, sizeof(MainCanvas),
			MSCROLLVIEW_S_FIX_HORZVERT | MSCROLLVIEW_S_FRAME | MSCROLLVIEW_S_SCROLL_NOTIFY_SELF);
	if(!p) return;

	APPWIDGET->canvas = p;

	p->wg.event = _view_event_handle;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	//page

	p->sv.page = (mScrollViewPage *)_create_page(MLK_WIDGET(p));
}

/** スクロール情報セット */

void MainCanvas_setScrollInfo(void)
{
	MainCanvas *p = APPWIDGET->canvas;
	mSize size;

	drawCalc_getCanvasScrollMax(APPDRAW, &size);

	//スクロールバーの基準位置
	// :スクロールバーが中央に来る位置

	p->pt_scroll_base.x = (size.w - p->sv.page->wg.w) >> 1;
	p->pt_scroll_base.y = (size.h - p->sv.page->wg.h) >> 1;

	//セット

	mScrollBarSetStatus(p->sv.scrh, 0, size.w, p->sv.page->wg.w);
	mScrollBarSetStatus(p->sv.scrv, 0, size.h, p->sv.page->wg.h);

	//位置をリセット

	mScrollBarSetPos(p->sv.scrh, p->pt_scroll_base.x);
	mScrollBarSetPos(p->sv.scrv, p->pt_scroll_base.y);
}

/** スクロール位置セット */

void MainCanvas_setScrollPos(void)
{
	MainCanvas *p = APPWIDGET->canvas;
	
	mScrollBarSetPos(p->sv.scrh, APPDRAW->canvas_scroll.x + p->pt_scroll_base.x);
	mScrollBarSetPos(p->sv.scrv, APPDRAW->canvas_scroll.y + p->pt_scroll_base.y);
}

