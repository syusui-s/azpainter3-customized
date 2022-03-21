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
 * [panel]レイヤのレイヤリスト
 *****************************************/

#include <stdlib.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_scrollview.h"
#include "mlk_scrollbar.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_rectbox.h"
#include "mlk_font.h"
#include "mlk_string.h"
#include "mlk_menu.h"

#include "def_widget.h"
#include "def_draw.h"

#include "tileimage.h"
#include "layerlist.h"
#include "layeritem.h"
#include "appresource.h"
#include "appcursor.h"
#include "apphelp.h"

#include "mainwindow.h"
#include "panel_func.h"

#include "draw_main.h"
#include "draw_layer.h"

#include "trid.h"
#include "trid_mainmenu.h"

#include "pv_panel_layer.h"
#include "pv_panel_layer_list.h"


//----------------------

static void _draw_one(PanelLayerList *area,mPixbuf *pixbuf,
	LayerItem *pi,int xtop,int ytop,uint8_t flags);
static void _draw_dnd(PanelLayerList *p,mlkbool erase);

//----------------------



//==============================
// sub
//==============================


/* 先頭レイヤの情報取得
 *
 * xtop,ytop: スクロール位置を適用した、ウィジェット座標での表示位置 (一覧上で一番上に来るレイヤ)
 * return: 先頭レイヤアイテム */

static LayerItem *_get_topitem_info(PanelLayerList *p,int *xtop,int *ytop)
{
	mPoint pt;

	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(p), &pt);

	*xtop = -pt.x;
	*ytop = -pt.y;

	return LayerList_getTopItem(APPDRAW->layerlist);
}

/* 指定レイヤの表示X位置と幅を取得
 *
 * item: NULL で x = 0, w = 全体
 * return: 表示 X 位置 (フォルダの余白分) */

static int _get_item_left_width(PanelLayerList *p,LayerItem *item,int *ret_width)
{
	int depw,w;

	depw = (item)? LayerItem_getTreeDepth(item) * _DEPTH_W: 0;

	w = p->wg.w - depw;
	if(w < _MIN_WIDTH) w = _MIN_WIDTH;

	*ret_width = w;

	return depw;
}

/* 指定レイヤのボックスの左上位置取得 (ウィジェット座標)
 *
 * return: 閉じられているか、Y位置がウィジェット範囲外の場合は FALSE */

static mlkbool _get_item_boxpos(PanelLayerList *p,LayerItem *item,int *xret,int *yret)
{
	LayerItem *pi;
	int xtop,y;

	pi = _get_topitem_info(p, &xtop, &y);

	for(; pi; pi = LayerItem_getNextOpened(pi), y += _EACH_H)
	{
		//ウィジェット下端を超えた
		
		if(y >= p->wg.h) return FALSE;

		//

		if(pi == item)
		{
			*xret = xtop;
			*yret = y;

			//上方向に隠れている場合は FALSE

			return (y > -_EACH_H);
		}
	}

	//一覧上に表示されないレイヤ
	return FALSE;
}

/* 指定レイヤの表示Y位置取得 (先頭レイヤを0とした絶対位置)
 *
 * return: item のレイヤ全体が一覧上に見えているか */

static mlkbool _get_item_absY(PanelLayerList *p,LayerItem *item,int *yret)
{
	LayerItem *pi;
	int xtop,y,scry;

	pi = _get_topitem_info(p, &xtop, &y);

	scry = -y;

	for(; pi && pi != item; pi = LayerItem_getNextOpened(pi), y += _EACH_H);

	*yret = scry + y;

	return (y >= 0 && y + _EACH_H <= p->wg.h);
}

/* ポインタ Y 位置からレイヤアイテム取得
 *
 * pttop: NULL でなければ、ボックスの左上位置が入る */

static LayerItem *_get_item_at_pt(PanelLayerList *p,int y,mPoint *pttop)
{
	LayerItem *pi;
	int ytop,xtop;

	pi = _get_topitem_info(p, &xtop, &ytop);

	for(; pi; pi = LayerItem_getNextOpened(pi), ytop += _EACH_H)
	{
		if(ytop <= y && y < ytop + _EACH_H)
		{
			if(pttop)
			{
				pttop->x = xtop;
				pttop->y = ytop;
			}
			return pi;
		}
	}

	return NULL;
}

/* 指定アイテムのみ直接描画 (内部用) */

static void _draw_one_direct(PanelLayerList *p,
	LayerItem *pi,int xtop,int ytop,uint8_t flags)
{
	mPixbuf *pixbuf;

	if((pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p))))
	{
		_draw_one(p, pixbuf, pi, xtop, ytop, flags);
	
		mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);
	}
}

/* 各フラグの変更
 *
 * fstate: TRUE で、ダブルクリック or +Shift */

static void _toggle_flag(PanelLayerList *p,LayerItem *pi,int no,mPoint *ptpos,int fstate)
{
	int fparam = 1;

	switch(no)
	{
		//塗りつぶし判定元
		case _FLAGBOX_NO_FILLREF:
			pi->flags ^= LAYERITEM_F_FILLREF;
			break;
		//マスクレイヤ
		case _FLAGBOX_NO_MASKLAYER:
			drawLayer_setLayerMask(APPDRAW, pi, fstate);
			fparam = 0;
			break;
		//ロック
		case _FLAGBOX_NO_LOCK:
			drawLayer_toggle_lock(APPDRAW, pi);
			break;
		//チェック
		case _FLAGBOX_NO_CHECK:
			pi->flags ^= LAYERITEM_F_CHECKED;
			break;
		//アルファマスク
		case _FLAGBOX_NO_ALPHAMASK:
			pi->alphamask = (pi->alphamask + 1) & 3;
			break;
	}

	//レイヤ一覧更新

	if(no != _FLAGBOX_NO_MASKLAYER)
		_draw_one_direct(p, pi, ptpos->x, ptpos->y, _DRAWONE_F_UPDATE | _DRAWONE_F_ONLY_INFO);

	//カレントレイヤの場合、パラメータ部分更新

	if(fparam && pi == APPDRAW->curlayer)
		PanelLayer_update_curparam();
}

/* 右クリックメニュー */

static void _runmenu_rbtt(PanelLayerList *p,LayerItem *pi,int x,int y)
{
	mMenu *menu;
	mMenuItem *mi;
	int id,fimg;

	//----- メニュー

	menu = mMenuNew();

	MLK_TRGROUP(TRGROUP_MAINMENU);

	mMenuAppendTrText_array16(menu, g_menudat_rbtt);

	mMenuAppendText(menu, -2, MLK_TR2(TRGROUP_PANEL_LAYER, TRID_LAYER_MENU_HELP));

	//無効

	fimg = LAYERITEM_IS_IMAGE(pi);

	mMenuSetItemEnable(menu, TRMENU_LAYER_OPT_TYPE, fimg);
	mMenuSetItemEnable(menu, TRMENU_LAYER_OPT_COLOR, LayerItem_isNeedColor(pi));
	mMenuSetItemEnable(menu, TRMENU_LAYER_FOLDER_CHECKED_MOVE, !fimg);

	//

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//------- コマンド

	if(id == -1) return;

	if(id >= 0)
	{
		//メインメニュー
		mWidgetEventAdd_command(MLK_WIDGET(APPWIDGET->mainwin), id, 0, MEVENT_COMMAND_FROM_MENU, 0);
	}
	else
	{
		//-2: help
		AppHelp_message(p->wg.toplevel, HELP_TRGROUP_SINGLE, HELP_TRID_LAYER_LIST);
	}
}


//==============================
// イベント
//==============================


/* 左ボタン押し時の処理
 *
 * retproc: 処理タイプが入る
 * return: 押した位置のレイヤアイテム */

static LayerItem *_press_left(PanelLayerList *p,int x,int y,
	mlkbool is_dblclk,uint32_t state,int *retproc)
{
	LayerItem *pi;
	mPoint ptpos;
	int proc,n;
	mlkbool is_folder;

	//押した位置のレイヤアイテム

	pi = _get_item_at_pt(p, y, &ptpos);
	if(!pi) return NULL;

	//ボックス内の相対位置

	x -= ptpos.x + LayerItem_getTreeDepth(pi) * _DEPTH_W;
	y -= ptpos.y;

	is_folder = LAYERITEM_IS_FOLDER(pi);

	proc = _PRESSLEFT_SETCURRENT;

	//

	if(_UPPER_Y <= y && y < _UPPER_Y + _VISIBLE_BOX_SIZE)
	{
		//------ 上段
	
		if(_INFO_X <= x && x < _INFO_X + _VISIBLE_BOX_SIZE)
		{
			//[表示/非表示ボックス]

			drawLayer_toggle_visible(APPDRAW, pi);

			_draw_one_direct(p, pi, ptpos.x, ptpos.y, _DRAWONE_F_UPDATE | _DRAWONE_F_ONLY_INFO);

			proc = _PRESSLEFT_DONE;
		}
		else if(is_folder && _FOLDER_ARROW_X <= x && x < _FOLDER_ARROW_X + _FOLDER_ARROW_W)
		{
			//[フォルダ開く/閉じる]

			//[!] この時、カレントレイヤは常に変更しなければならない。
			// フォルダを閉じた時、
			// 現在のカレントレイヤがそのフォルダの子である場合、
			// 一覧上にカレントレイヤが表示されなくなるため。

			drawLayer_toggle_folderOpened(APPDRAW, pi);

			proc = _PRESSLEFT_SETCURRENT_DONE;
		}
	}
	else if(_LOWER_Y <= y && y < _LOWER_Y + _FLAGS_BOX_SIZE)
	{
		//---- 下段

		if(x >= _LAYERTYPE_X && x < _LAYERTYPE_X + _FLAGS_BOX_SIZE)
		{
			//レイヤタイプ
			// :ダブルクリックで、線の色変更

			if(is_dblclk && LayerItem_isNeedColor(pi))
				proc = _PRESSLEFT_SETCOLOR;
		}
		else if(x >= _FLAGS_X && x < _FLAGS_X + _FLAGS_ALLW)
		{
			//フラグボックス

			n = (x - _FLAGS_X) / (_FLAGS_BOX_SIZE + _FLAGS_PAD);

			if(x - _FLAGS_X - n * (_FLAGS_BOX_SIZE + _FLAGS_PAD) >= _FLAGS_BOX_SIZE)
				//ボタン間の余白
				proc = _PRESSLEFT_SETCURRENT;
			else if(is_folder && n != _FLAGBOX_NO_LOCK)
				//フォルダでロック以外の場合
				proc = _PRESSLEFT_SETCURRENT;
			else
			{
				//処理 (カレントレイヤの変更はしない)
				
				_toggle_flag(p, pi, n, &ptpos, (is_dblclk || (state & MLK_STATE_SHIFT)) );

				proc = _PRESSLEFT_DONE;
			}
		}
	}

	//

	if(proc == _PRESSLEFT_SETCURRENT)
	{
		/*
		if(x >= _PREV_X && x < _PREV_X + _PREV_W
			&& y >= _PREV_Y && y < _PREV_Y + _PREV_H)
		{
			//プレビュー部分 (A/A1 時)
			// :ダブルクリックで色設定ダイアログ。

			if(LayerItem_isNeedColor(pi) && is_dblclk)
				proc = _PRESSLEFT_SETCOLOR;
		}
		else
		*/
		if(is_dblclk)
		{
			//ダブルクリック => レイヤ設定

			proc = _PRESSLEFT_OPTION;
		}
	}

	//

	*retproc = proc;

	return pi;
}

/* ポインタ押し時 */

static void _event_press(PanelLayerList *p,mEventPointer *ev)
{
	LayerItem *pi;
	int proc;

	if(ev->btt == MLK_BTT_RIGHT)
	{
		//----- 右ボタン (メニュー)

		pi = _get_item_at_pt(p, ev->y, NULL);

		if(pi)
		{
			//カレント変更
			drawLayer_setCurrent(APPDRAW, pi);
			
			_runmenu_rbtt(p, pi, ev->x, ev->y);
		}
	}
	else if(ev->btt == MLK_BTT_LEFT)
	{
		//----- 左ボタン

		pi = _press_left(p, ev->x, ev->y,
				(ev->act == MEVENT_POINTER_ACT_DBLCLK), ev->state, &proc);

		//範囲外、または処理済みでカレント変更しない場合

		if(!pi || proc == _PRESSLEFT_DONE)
			return;

		//

		if(proc == _PRESSLEFT_OPTION)
		{
			//レイヤ設定
			
			MainWindow_layer_setOption(APPWIDGET->mainwin, pi);
		}
		else if(proc == _PRESSLEFT_SETCOLOR)
		{
			//線の色変更

			MainWindow_layer_setColor(APPWIDGET->mainwin, pi);
		}
		else
		{
			//カレントレイヤ変更
			// :カレントレイヤ上をクリックで、
			// :カレントレイヤが変更されない場合は、プレビュー更新

			if(!drawLayer_setCurrent(APPDRAW, pi))
				PanelLayer_update_curlayer(FALSE);

			//

			if(proc == _PRESSLEFT_SETCURRENT)
			{
				if(ev->state & MLK_STATE_CTRL)
				{
					//+Ctrl でカレントレイヤのみ表示

					drawLayer_visibleAll(APPDRAW, 2);
				}
				else
				{
					//レイヤ移動 D&D 判定開始

					p->fpress = _PRESS_MOVE_BEGIN;
					p->dnd_pos = ev->y;

					mWidgetGrabPointer(MLK_WIDGET(p));
				}
			}
		}
	}
}

/* ポインタ移動時 (レイヤ移動) */

static void _event_motion_movelayer(PanelLayerList *p,int y)
{
	LayerItem *pi;
	int type;
	mPoint ptpos;

	//----- ドラッグ開始判定
	// (押し位置から一定距離離れたらドラッグ開始)

	if(p->fpress == _PRESS_MOVE_BEGIN)
	{
		if(abs(y - p->dnd_pos) <= _EACH_H / 2)
			return;

		//ドラッグ開始

		mWidgetSetCursor(MLK_WIDGET(p), AppCursor_getForDrag(APPCURSOR_DND_ITEM));

		p->fpress = _PRESS_MOVE;
		p->item = NULL;
		p->dnd_type = _MOVEDEST_NONE;
	}

	//------ ドラッグ中

	//ウィジェット範囲外の場合

	if(y < 0 || y >= p->wg.h)
	{
		//スクロール開始
	
		if(p->dnd_type != _MOVEDEST_SCROLL)
		{
			mWidgetTimerAdd(MLK_WIDGET(p), 0, _TIMER_SCROLL_TIME, (y >= p->wg.h));

			_draw_dnd(p, TRUE);
			
			p->dnd_type = _MOVEDEST_SCROLL;
		}

		return;
	}

	//スクロール解除

	if(p->dnd_type == _MOVEDEST_SCROLL)
		mWidgetTimerDelete(MLK_WIDGET(p), 0);

	//pi = 移動先

	pi = _get_item_at_pt(p, y, &ptpos);

	type = _MOVEDEST_INSERT;

	if(pi == APPDRAW->curlayer)
	{
		//カレントの場合は移動なし

		type = _MOVEDEST_NONE;
	}
	else if(!pi)
	{
		//範囲外なら終端へ

		mScrollBar *sb = mScrollViewPage_getScrollBar_vert(MLK_SCROLLVIEWPAGE(p));

		ptpos.y = sb->sb.max;
	}
	else if(LayerItem_isInParent(pi, APPDRAW->curlayer))
	{
		//移動元(カレントレイヤ)がフォルダで、移動先がそのフォルダ下の場合は無効

		type = _MOVEDEST_NONE;
	}
	else if(LAYERITEM_IS_FOLDER(pi))
	{
		//移動先がフォルダの場合、境界から一定範囲内は挿入、それ以外はフォルダ内への移動

		type = (y - ptpos.y >= 8)? _MOVEDEST_FOLDER: _MOVEDEST_INSERT;
	}

	//移動先変更

	if(pi != p->item || type != p->dnd_type)
	{
		_draw_dnd(p, TRUE);
	
		p->item = pi;
		p->dnd_type = type;
		p->dnd_pos = ptpos.y;

		_draw_dnd(p, FALSE);
	}
}

/* グラブ解放 */

static void _release_grab(PanelLayerList *p)
{
	if(p->fpress)
	{
		//D&D 移動

		if(p->fpress == _PRESS_MOVE)
		{
			mWidgetTimerDelete(MLK_WIDGET(p), 0);

			mWidgetSetCursor(MLK_WIDGET(p), 0);

			_draw_dnd(p, TRUE);

			if(p->dnd_type >= 0)
				drawLayer_moveDND(APPDRAW, p->item, p->dnd_type);
		}

		//
	
		p->fpress = 0;
		mWidgetUngrabPointer();
	}
}


//==============================
// ハンドラ
//==============================


/* ファイルドロップ */

static void _page_dropfiles(AppDraw *p,char **files)
{
	mRect rc;

	mRectEmpty(&rc);

	//読み込み
	// :各ファイルごとにスレッド

	for(; *files; files++)
		drawLayer_newLayer_file(p, *files, &rc);

	//まとめて更新

	drawUpdateRect_canvas_canvasview(p, &rc);
	PanelLayer_update_all();
}

/* イベントハンドラ */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	PanelLayerList *p = _PAGE(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動
				
				if(p->fpress == _PRESS_MOVE_BEGIN || p->fpress == _PRESS_MOVE)
					_event_motion_movelayer(p, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し

				if(!p->fpress)
					_event_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;
	
		//スクロールバー
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == wg->parent
				&& (ev->notify.notify_type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_VERT
					|| ev->notify.notify_type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ)
				&& (ev->notify.param2 & MSCROLLBAR_ACTION_F_CHANGE_POS))
			{
				mWidgetRedraw(wg);
			}
			break;

		//タイマー (D&D 時のスクロール)
		case MEVENT_TIMER:
			if(mScrollBarAddPos(
				mScrollViewPage_getScrollBar_vert(MLK_SCROLLVIEWPAGE(wg)),
				(ev->timer.param)? _EACH_H: -_EACH_H))
			{
				mWidgetRedraw(wg);
			}
			break;

		//ホイールによる垂直スクロール
		case MEVENT_SCROLL:
			if(p->fpress == 0 && ev->scroll.vert_step)
			{
				if(mScrollBarAddPos(
					mScrollViewPage_getScrollBar_vert(MLK_SCROLLVIEWPAGE(wg)),
					ev->scroll.vert_step * _EACH_H))
				{
					mWidgetRedraw(wg);
				}
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;

		//ファイルドロップ
		case MEVENT_DROP_FILES:
			_page_dropfiles(APPDRAW, ev->dropfiles.files);
			break;
	}
	
	return 1;
}

/* 描画 */

static void _page_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	LayerItem *pi;
	mPoint ptscr;
	int y;

	//レイヤ

	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(wg), &ptscr);

	pi = LayerList_getTopItem(APPDRAW->layerlist);

	for(y = -ptscr.y; pi && y < wg->h; pi = LayerItem_getNextOpened(pi), y += _EACH_H)
	{
		if(y > -_EACH_H)
			_draw_one((PanelLayerList *)wg, pixbuf, pi, -ptscr.x, y, 0);
	}

	//残りの背景

	if(y < wg->h)
		mPixbufFillBox(pixbuf, 0, y, wg->w, wg->h - y, MGUICOL_PIX(FACE_DARK));
}

/* サイズ変更時 */

static void _page_resize_handle(mWidget *wg)
{
	PanelLayerList_setScrollInfo((PanelLayerList *)wg);
}


//==============================
// 作成
//==============================


/* ページ作成 */

static mScrollViewPage *_page_new(mWidget *parent)
{
	PanelLayerList *p;

	p = (PanelLayerList *)mScrollViewPageNew(parent, sizeof(PanelLayerList));

	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;
	p->wg.fstate |= MWIDGET_STATE_ENABLE_DROP;
	p->wg.draw = _page_draw_handle;
	p->wg.event = _page_event_handle;
	p->wg.resize = _page_resize_handle;

	return (mScrollViewPage *)p;
}

/** レイヤリスト作成
 *
 * return: ページ部分のウィジェットを返す */

PanelLayerList *PanelLayerList_new(mWidget *parent)
{
	mScrollView *p;

	p = mScrollViewNew(parent, 0, MSCROLLVIEW_S_HORZVERT_FRAME | MSCROLLVIEW_S_FIX_HORZVERT);

	p->wg.flayout = MLF_EXPAND_WH;

	p->sv.page = _page_new(MLK_WIDGET(p));

	return (PanelLayerList *)p->sv.page;
}

/** スクロール情報セット */

void PanelLayerList_setScrollInfo(PanelLayerList *p)
{
	int num,depth,w;

	num = LayerList_getScrollInfo(APPDRAW->layerlist, &depth);

	//水平

	w = depth * _DEPTH_W + _MIN_WIDTH;

	mScrollBarSetStatus(
		mScrollViewPage_getScrollBar_horz(MLK_SCROLLVIEWPAGE(p)),
		0, (p->wg.w > w)? p->wg.w: w, p->wg.w);

	//垂直
	
	mScrollBarSetStatus(
		mScrollViewPage_getScrollBar_vert(MLK_SCROLLVIEWPAGE(p)),
		0, num * _EACH_H, p->wg.h); 
}

/** 指定アイテムのみを直接描画 */

void PanelLayerList_drawLayer(PanelLayerList *p,LayerItem *pi)
{
	int x,y;

	if(_get_item_boxpos(p, pi, &x, &y))
		_draw_one_direct(p, pi, x, y, _DRAWONE_F_UPDATE);
}

/** 任意のレイヤをカレントに変更した時の更新
 *
 * 一覧上で見えるようにスクロール。 */

void PanelLayerList_changeCurrent_visible(PanelLayerList *p,LayerItem *lastitem,mlkbool update_all)
{
	int y;

	//カレントレイヤが一覧の表示範囲外なら、スクロール

	if(!_get_item_absY(p, APPDRAW->curlayer, &y))
	{
		y -= p->wg.h / 2 - _EACH_H / 2;

		mScrollBarSetPos(mScrollViewPage_getScrollBar_vert(MLK_SCROLLVIEWPAGE(p)), y);

		update_all = TRUE;
	}

	//更新

	if(update_all)
		mWidgetRedraw(MLK_WIDGET(p));
	else
	{
		//スクロール or 全体更新でない場合、各レイヤを個別更新

		if(lastitem) PanelLayerList_drawLayer(p, lastitem);
		
		PanelLayerList_drawLayer(p, APPDRAW->curlayer);
	}
}


//==============================
// 描画
//==============================


/* 数値用表示用のデータセット */

static uint8_t *_setimgno(uint8_t *pd,int v,int dig)
{
	if(v >= 1000)
		*(pd++) = (v / 1000) % 10;

	if(v >= 100)
		*(pd++) = (v / 100) % 10;

	if(v >= 10)
		*(pd++) = (v / 10) % 10;

	//1の桁

	v = v % 10;

	if(dig == 1)
	{
		//小数点以下が0なら、なし
		if(v)
		{
			*(pd++) = 11;
			*(pd++) = v;
		}
	}
	else
		*(pd++) = v;

	return pd;
}

/* トーン情報の表示データ */

static void _setimgno_tone(uint8_t *pd,int line,int density)
{
	pd = _setimgno(pd, line, 1);

	*(pd++) = 14; //L

	if(density)
	{
		pd = _setimgno(pd, density, 0);
		*(pd++) = 12; //%
	}

	*pd = 255;
}

/* レイヤを一つ描画
 *
 * xtop,ytop: 左上の描画位置 (スクロール適用) */

void _draw_one(PanelLayerList *page,mPixbuf *pixbuf,
	LayerItem *pi,int xtop,int ytop,uint8_t flags)
{
	int width,right,x,y,i,n,n2;
	uint8_t m[16],*pm;
	const char *pc;
	mPixCol pix_bkgnd,coltmp[4],col[5];
	mlkbool is_sel,is_folder;

	//width = レイヤのボックスの幅
	//right = ボックスの右端位置

	//フォルダ深さを適用

	n = LayerItem_getTreeDepth(pi) * _DEPTH_W;

	width = page->wg.w - n;
	if(width < _MIN_WIDTH) width = _MIN_WIDTH;

	xtop += n;

	//右端

	right = xtop + width;

	//

	is_sel = (pi == APPDRAW->curlayer);
	is_folder = LAYERITEM_IS_FOLDER(pi);

	pix_bkgnd = (is_sel)? mRGBtoPix(_COL_SEL_BKGND): MGUICOL_PIX(WHITE);

	col[0] = 0;
	col[1] = MGUICOL_PIX(WHITE);
	col[2] = mRGBtoPix(_COL_VISIBLE_BOX);
	col[3] = mRGBtoPix(0xb0b0b0);	//フラグ OFF 時の図形の色
	col[4] = mRGBtoPix(0xFF42DD);	//フラグ ON 時の背景色

	//----- 背景など基本部分

	if(!(flags & _DRAWONE_F_ONLY_INFO))
	{
		//左の余白 (フォルダの深さ分)
		// :水平スクロールにより隠れている場合は描画しない

		if(xtop > 0)
			mPixbufFillBox(pixbuf, 0, ytop, xtop, _EACH_H, MGUICOL_PIX(FACE_DARK));
	
		//背景色

		mPixbufFillBox(pixbuf, xtop, ytop, right - xtop, _EACH_H - 1, pix_bkgnd);

		//右の余白

		if(right < page->wg.w)
			mPixbufFillBox(pixbuf, right, ytop, page->wg.w - right, _EACH_H, MGUICOL_PIX(FACE_DARK));

		//下の境界線

		mPixbufLineH(pixbuf, 0, ytop + _EACH_H - 1, page->wg.w, 0);

		//選択枠

		if(is_sel)
			mPixbufBox(pixbuf, xtop, ytop, width, _EACH_H - 1, mRGBtoPix(_COL_SEL_FRAME));
	}

	//------- プレビュー

	if(!is_folder && !(flags & _DRAWONE_F_ONLY_INFO))
	{
		x = xtop + _PREV_X;
		y = ytop + _PREV_Y;

		//枠
	
		mPixbufBox(pixbuf, x, y, _PREV_W, _PREV_H, 0);

		//プレビューイメージ

		TileImage_drawPreview(pi->img, pixbuf,
			x + 1, y + 1, _PREV_W - 2, _PREV_H - 2,
			APPDRAW->imgw, APPDRAW->imgh);
	}

	//------ フォルダ

	if(is_folder)
	{
		//フォルダアイコン

		mPixbufDraw1bitPattern(pixbuf,
			xtop + _FOLDER_ICON_X, ytop + _UPPER_Y + 2,
			g_img_folder_icon, 10, 9, MPIXBUF_TPCOL, 0);

		//矢印

		if(LAYERITEM_IS_FOLDER_OPENED(pi))
		{
			mPixbufDraw1bitPattern(pixbuf,
				xtop + _FOLDER_ARROW_X, ytop + _UPPER_Y + 4,
				g_img_arrow_down, 9, 5, MPIXBUF_TPCOL, 0);
		}
		else
		{
			mPixbufDraw1bitPattern(pixbuf,
				xtop + _FOLDER_ARROW_X + 2, ytop + _UPPER_Y + 2,
				g_img_arrow_right, 5, 9, MPIXBUF_TPCOL, 0);
		}
	}

	//------ 情報 (1段目)

	//表示/非表示ボックス

	mPixbufBlt_2bit(pixbuf,
		xtop + _INFO_X, ytop + _UPPER_Y,
		g_img_visible_2bit, (LAYERITEM_IS_VISIBLE(pi))? 13: 0, 0, 13, 13,
		13 * 2, col);

	//レイヤ名

	if(!(flags & _DRAWONE_F_ONLY_INFO))
	{
		mPixbufClip_setBox_d(pixbuf,
			xtop + _INFO_NAME_X, ytop + _UPPER_Y,
			right - (xtop + _INFO_NAME_X) - 4, 15);
	
		mFontDrawText_pixbuf(
			mWidgetGetFont(MLK_WIDGET(page)), pixbuf,
			xtop + _INFO_NAME_X, ytop + _UPPER_Y,
			pi->name, -1,
			(is_sel)? _COL_SEL_NAME: 0);

		mPixbufClip_clear(pixbuf);
	}

	//------ 情報 (2段目)

	y = ytop + _LOWER_Y;

	//レイヤタイプ

	if(!is_folder && pi->type != LAYERTYPE_RGBA)
	{
		x = xtop + _INFO_X;

		//2bit レイヤ色用
		coltmp[0] = 0;
		coltmp[1] = mRGBtoPix(pi->col);
		coltmp[3] = col[1];

		switch(pi->type)
		{
			//グレイスケール
			case LAYERTYPE_GRAY:
				if(LAYERITEM_IS_TONE(pi))
				{
					mPixbufDraw2bitPattern(pixbuf, x, y,
						g_img_coltype_tone_gray, _FLAGS_BOX_SIZE, _FLAGS_BOX_SIZE, coltmp);
				}
				else
					mPixbufBlt_gray8(pixbuf, x, y, g_img_coltype_gray, 0, 0, 14, 14, 14);
				break;
			//アルファ値のみ
			case LAYERTYPE_ALPHA:
				mPixbufDraw1bitPattern(pixbuf, x, y,
					g_img_coltype_alpha, _FLAGS_BOX_SIZE, _FLAGS_BOX_SIZE,
					mRGBtoPix(pi->col), 0);
				break;
			//A1
			case LAYERTYPE_ALPHA1BIT:
				if(LAYERITEM_IS_TONE(pi))
				{
					mPixbufDraw2bitPattern(pixbuf, x, y,
						g_img_coltype_tone_a1, _FLAGS_BOX_SIZE, _FLAGS_BOX_SIZE, coltmp);
				}
				else
				{
					mPixbufDraw2bitPattern(pixbuf, x, y,
						g_img_coltype_a1, _FLAGS_BOX_SIZE, _FLAGS_BOX_SIZE, coltmp);
				}
				break;
		}

		//テキストレイヤ

		if(LAYERITEM_IS_TEXT(pi))
		{
			mPixbufDraw1bitPattern(pixbuf, x + 1, y + _FLAGS_BOX_SIZE - 8,
				g_img_coltype_text, 6, 7, col[1], 0);
		}
	}

	//フラグ

	coltmp[0] = 0; //[0]枠 [1]中の図形の色 [3]背景色

	x = xtop + _FLAGS_X;

	for(i = 0; i < 4; i++, x += _FLAGS_BOX_SIZE + _FLAGS_PAD)
	{
		//フォルダ時はロックのみ
		if(is_folder && i != _FLAGBOX_NO_LOCK) continue;

		if(i == _FLAGBOX_NO_MASKLAYER)
		{
			//マスクレイヤ

			if(pi == APPDRAW->masklayer)
				n = _FLAGBOX_NO_MASKLAYER * 14, n2 = 1;
			else if(pi->flags & LAYERITEM_F_MASK_UNDER)
				//下レイヤでマスク
				n = _FLAGBOX_NO_MASK_UNDER * 14, n2 = 1;
			else
				//OFF
				n = _FLAGBOX_NO_MASKLAYER * 14, n2 = 0;
		}
		else
		{
			n = i * 14;
			n2 = pi->flags & g_flagbox_flags[i];
		}

		//描画

		if(n2)
			//ON
			coltmp[1] = col[1], coltmp[3] = col[4];
		else
			coltmp[1] = col[3], coltmp[3] = col[1];
	
		mPixbufBlt_2bit(pixbuf, x, y, g_img_flags_2bit,
			n, 0, 14, 14, _FLAGBOX_IMG_WIDTH, coltmp);
	}

	//アルファマスク

	if(!is_folder)
	{
		n = pi->alphamask;

		if(!n)
			//OFF
			coltmp[1] = col[3], coltmp[3] = col[1];
		else
		{
			coltmp[1] = col[1];

			if(n == 1)
				coltmp[3] = mRGBtoPix_sep(255,0,220);
			else if(n == 2)
				coltmp[3] = mRGBtoPix_sep(0,218,0);
			else
				coltmp[3] = mRGBtoPix_sep(0,60,255);
		}

		mPixbufBlt_2bit(pixbuf, x, y, g_img_flags_2bit,
			14 * _FLAGBOX_NO_ALPHAMASK, 0, 14, 14, _FLAGBOX_IMG_WIDTH, coltmp);
	}

	//トーン情報・合成モード

	if(!is_folder && LAYERITEM_IS_TONE(pi)
		&& (pi->type == LAYERTYPE_GRAY || pi->type == LAYERTYPE_ALPHA1BIT))
	{
		_setimgno_tone(m, pi->tone_lines, pi->tone_density);

		mPixbufDraw1bitPattern_list(pixbuf,
			xtop + _TONEINFO_X, ytop + _EACH_H - 9 - 6,
			AppResource_get1bitImg_number5x9(), APPRES_NUMBER_5x9_WIDTH, 9, 5,
			m, 0);
	}
	else
	{
		pc = g_blend_name[pi->blendmode];
		pm = m;

		for(; *pc; pc++)
			*(pm++) = *pc - 'A';

		*pm = 255;

		mPixbufDraw1bitPattern_list(pixbuf,
			xtop + _TONEINFO_X + 2, ytop + _EACH_H - 7 - 6,
			g_img_alphabet, 156, 7, 6, m, 0);
	}
	
	//不透明度

	pm = _setimgno(m, (int)(pi->opacity / 128.0 * 100.0 + 0.5), 0);

	n = (pm - m);
	*pm = 255;

	mPixbufDraw1bitPattern_list(pixbuf,
		right - 5 * n - 4, ytop + _EACH_H - 9 - 6,
		AppResource_get1bitImg_number5x9(), APPRES_NUMBER_5x9_WIDTH, 9, 5,
		m, 0);

	//------ ウィンドウの更新範囲 (直接描画時)

	if(flags & _DRAWONE_F_UPDATE)
		mWidgetUpdateBox_d(MLK_WIDGET(page), 0, ytop, page->wg.w, _EACH_H);
}

/* D&D 移動時の描画 */

void _draw_dnd(PanelLayerList *p,mlkbool erase)
{
	mPixbuf *pixbuf;
	uint32_t col;
	int x,y,w;

	//移動先なし

	if(p->dnd_type < 0) return;

	//色

	if(erase)
	{
		if(p->item)
			col = MGUICOL_PIX(WHITE);
		else
			col = MGUICOL_PIX(FACE_DARK);
	}
	else
		col = mRGBtoPix(0xff0000);

	//描画

	if((pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p))))
	{
		x = -mScrollViewPage_getScrollPos_horz(MLK_SCROLLVIEWPAGE(p));
		x += _get_item_left_width(p, p->item, &w);
		y = p->dnd_pos;
	
		if(p->dnd_type == _MOVEDEST_FOLDER)
		{
			//フォルダ

			mPixbufBox(pixbuf, x, y, w, _EACH_H - 1, col);
			mPixbufBox(pixbuf, x + 1, y + 1, w - 2, _EACH_H - 3, col);

			mWidgetUpdateBox_d(MLK_WIDGET(p), x, y, w, _EACH_H - 1);
		}
		else
		{
			//挿入

			mPixbufFillBox(pixbuf, x, y, w, 2, col);

			mWidgetUpdateBox_d(MLK_WIDGET(p), x, y, w, 2);
		}

		mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);
	}
}


