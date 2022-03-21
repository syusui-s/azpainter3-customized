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
 * パレットカラービュー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_scrollbar.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_rectbox.h"

#include "palettelist.h"
#include "changecol.h"
#include "draw_main.h"

#include "trid.h"
#include "trid_colorpalette.h"


/*
 - 色変更の通知は、直接送られる。
 - 選択されているパレットがない場合、item = NULL。
*/

//---------------------

typedef struct
{
	mWidget wg;

	mScrollBar *scr;
	PaletteListItem *item;
	
	int fpress,
		xnum,			//横方向の表示個数
		press_no;		//押し時のパレット番号
	mPoint pt_press,	//押し時のパレット描画位置/[グラデーション時] 始点
		pt_last;		//グラデーション時、前回の位置
}_palpage;

typedef struct
{
	MLK_CONTAINER_DEF

	_palpage *page;
	mScrollBar *scr;
}_palview;

//---------------------

#define _COL_BKGND  0x999999
#define _COL_SEL    0xff0000

enum
{
	_PRESS_SPOIT = 1,
	_PRESS_SETCOL,
	_PRESS_GRADATION
};

static const uint16_t g_menudat[] = {
	TRID_PALVIEW_SETCOL, TRID_PALVIEW_GETCOL,
	MMENU_ARRAY16_END
};

//---------------------


/************************************
 * _palpage (表示部分)
 ************************************/


/* 横の表示数セット */

static void _page_set_xnum(_palpage *p)
{
	int n,max;

	if(!p->item) return;

	max = p->item->xmaxnum;

	n = p->wg.w / p->item->cellw;

	if(max && n > max)
		n = max;
	
	if(n < 1) n = 1;

	p->xnum = n;
}

/* スクロール情報セット */

static void _page_set_scroll(_palpage *p)
{
	int page;

	if(!p->item)
		mScrollBarSetStatus(p->scr, 0, 1, -1);
	else
	{
		page = p->wg.h / p->item->cellh;
		if(page <= 0) page = 1;

		mScrollBarSetStatus(p->scr, 0, (p->item->palnum + p->xnum - 1) / p->xnum, page);
	}
}

/* カーソル位置からパレット番号取得
 *
 * ptdst: NULL 以外で、パレットの左上位置がセットされる
 * return: -1 で範囲外 */

static int _get_palno_at_pt(_palpage *p,int x,int y,mPoint *ptdst)
{
	PaletteListItem *item = p->item;
	int no,scrpos;

	if(!item || x < 0 || y < 0) return -1;

	scrpos = mScrollBarGetPos(p->scr);

	x = x / item->cellw;
	y = y / item->cellh + scrpos;

	if(x >= p->xnum) return -1;

	no = x + y * p->xnum;

	if(no >= item->palnum) return -1;

	if(ptdst)
	{
		ptdst->x = x * item->cellw;
		ptdst->y = (y - scrpos) * item->cellh;
	}

	return no;
}


//========================
// 直接描画
//========================


/* カーソルを直接描画 */

static void _draw_cursor(_palpage *p,mPoint *pt,mlkbool erase)
{
	mPixbuf *pixbuf;
	mBox box;

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p));
	if(!pixbuf) return;

	box.x = pt->x;
	box.y = pt->y;
	box.w = p->item->cellw + 1;
	box.h = p->item->cellh + 1;

	mPixbufBox(pixbuf, box.x, box.y, box.w, box.h,
		mRGBtoPix(erase? _COL_BKGND: _COL_SEL));

	mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);

	mWidgetUpdateBox(MLK_WIDGET(p), &box);
}

/* 色とカーソルを直接描画
 *
 * only_color: 色のみ描画 */

static void _draw_color_and_cursor(_palpage *p,mPoint *pt,uint32_t col,mlkbool only_color)
{
	mPixbuf *pixbuf;
	mBox box;

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p));
	if(!pixbuf) return;

	box.x = pt->x;
	box.y = pt->y;
	box.w = p->item->cellw + 1;
	box.h = p->item->cellh + 1;

	if(!only_color)
		mPixbufBox(pixbuf, box.x, box.y, box.w, box.h, mRGBtoPix(_COL_SEL));

	mPixbufFillBox(pixbuf, box.x + 1, box.y + 1, box.w - 2, box.h - 2,
		mRGBtoPix(col));

	mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);

	mWidgetUpdateBox(MLK_WIDGET(p), &box);
}

/* グラデーション線描画
 *
 * pt1: 始点
 * pt2: 終点。NULL で始点のみ描画 */

static void _draw_grad_line(_palpage *p,mPoint *pt1,mPoint *pt2)
{
	mPixbuf *pixbuf;
	mBox box,box2;

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p));
	if(!pixbuf) return;

	mPixbufSetPixelModeXor(pixbuf);
	
	if(!pt2)
	{
		mPixbufSetPixel(pixbuf, pt1->x, pt1->y, 0);

		box.x = pt1->x, box.y = pt1->y;
		box.w = box.h = 1;
	}
	else
	{
		mPixbufLine(pixbuf, pt1->x, pt1->y, p->pt_last.x, p->pt_last.y, 0);
		mPixbufLine(pixbuf, pt1->x, pt1->y, pt2->x, pt2->y, 0);

		mBoxSetPoint_minmax(&box, pt1, &p->pt_last);
		mBoxSetPoint_minmax(&box2, pt1, pt2);
		mBoxUnion(&box, &box2);
	}

	mPixbufSetPixelModeCol(pixbuf);

	mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);

	mWidgetUpdateBox(MLK_WIDGET(p), &box);
}


//========================
// イベント
//========================


/* メニュー */

static void _run_menu(_palpage *p,int palno,int x,int y,mPoint *pt)
{
	mMenu *menu;
	mMenuItem *mi;
	int id;

	//カーソル描画

	_draw_cursor(p, pt, FALSE);

	//メニュー

	MLK_TRGROUP(TRGROUP_PANEL_COLOR_PALETTE);

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menudat);

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//カーソル消去

	_draw_cursor(p, pt, TRUE);

	//コマンド

	if(id == -1) return;

	switch(id)
	{
		//描画色をセット
		case TRID_PALVIEW_SETCOL:
			PaletteListItem_setColor(p->item, palno, drawColor_getDrawColor());

			_draw_color_and_cursor(p, pt, drawColor_getDrawColor(), TRUE);
			break;
		//色を取得
		case TRID_PALVIEW_GETCOL:
			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0,
				PaletteListItem_getColor(p->item, palno), CHANGECOL_RGB);
			break;
	}
}

/* 押し時 */

static void _event_press(_palpage *p,mEvent *ev)
{
	int no,type = -1;
	mPoint pt;

	//パレット位置

	no = _get_palno_at_pt(p, ev->pt.x, ev->pt.y, &pt);
	if(no == -1) return;

	//ボタン

	if(ev->pt.btt == MLK_BTT_RIGHT)
	{
		//右ボタン: メニュー

		_run_menu(p, no, ev->pt.x, ev->pt.y, &pt);
	}
	else if(ev->pt.btt == MLK_BTT_LEFT)
	{
		//------ 左ボタン

		if(ev->pt.state & MLK_STATE_SHIFT)
		{
			//+Shift: 色登録

			PaletteListItem_setColor(p->item, no, drawColor_getDrawColor());

			type = _PRESS_SETCOL;

			_draw_color_and_cursor(p, &pt, drawColor_getDrawColor(), FALSE);
		}
		else if(ev->pt.state & MLK_STATE_CTRL)
		{
			//+Ctrl: グラデーション

			type = _PRESS_GRADATION;

			pt.x = ev->pt.x;
			pt.y = ev->pt.y;
			p->pt_last = pt;

			_draw_grad_line(p, &pt, NULL);
		}
		else
		{
			//色取得

			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0,
				PaletteListItem_getColor(p->item, no), CHANGECOL_RGB);

			type = _PRESS_SPOIT;

			_draw_cursor(p, &pt, FALSE);
		}
	}

	//グラブ

	if(type != -1)
	{
		p->fpress = type;
		p->press_no = no;
		p->pt_press = pt;

		mWidgetGrabPointer(MLK_WIDGET(p));
	}
}

/* 移動時: グラデーション */

static void _event_motion_grad(_palpage *p,mEvent *ev)
{
	mPoint pt;

	pt.x = ev->pt.x;
	pt.y = ev->pt.y;

	_draw_grad_line(p, &p->pt_press, &pt);

	p->pt_last = pt;
}

/* グラブ解除
 *
 * ev: NULL でフォーカスアウト時 */

static void _release_grab(_palpage *p,mEvent *ev)
{
	int no;

	if(p->fpress)
	{
		switch(p->fpress)
		{
			case _PRESS_SPOIT:
			case _PRESS_SETCOL:
				_draw_cursor(p, &p->pt_press, TRUE);
				break;
			//グラデーション
			case _PRESS_GRADATION:
				if(ev)
				{
					no = _get_palno_at_pt(p, ev->pt.x, ev->pt.y, NULL);
					if(no != -1)
						PaletteListItem_gradation(p->item, p->press_no, no);
				}
				
				mWidgetRedraw(MLK_WIDGET(p));
				break;
		}
	
		p->fpress = 0;
		mWidgetUngrabPointer();
	}
}

/* ファイルドロップ (パレットファイル読み込み) */

static void _page_dropfiles(_palpage *p,char **files)
{
	if(p->item)
	{
		if(PaletteListItem_loadFile(p->item, *files, FALSE))
		{
			_page_set_scroll(p);

			mWidgetRedraw(MLK_WIDGET(p));
		}
	}
}

/* イベント */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	_palpage *p = (_palpage *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fpress == _PRESS_GRADATION)
					_event_motion_grad(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し
				
				if(!p->fpress)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p, ev);
			}
			break;

		//ホイール: 垂直スクロール
		case MEVENT_SCROLL:
			if(ev->scroll.vert_step)
			{
				mScrollBarAddPos(p->scr, ev->scroll.vert_step * 3);

				mWidgetRedraw(wg);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p, NULL);
			break;

		//ファイルドロップ
		case MEVENT_DROP_FILES:
			_page_dropfiles(p, ev->dropfiles.files);
			break;
	}

	return 1;
}


//========================


/* サイズ変更時 */

static void _page_resize(mWidget *wg)
{
	_palpage *p = (_palpage *)wg;

	_page_set_xnum(p);
	_page_set_scroll(p);
}

/* 描画 */

static void _page_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	_palpage *p = (_palpage *)wg;
	PaletteListItem *item = p->item;
	uint8_t *buf;
	int no,num,ix,x,y,h,cellw,cellh;

	h = wg->h;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, h, mRGBtoPix(_COL_BKGND));

	if(!item) return;

	//パレット

	no = mScrollBarGetPos(p->scr) * p->xnum;
	buf = item->palbuf + no * 3;
	num = item->palnum;
	cellw = item->cellw;
	cellh = item->cellh;

	for(y = 1; y < h; y += cellh)
	{
		for(ix = p->xnum, x = 1; ix > 0; ix--, x += cellw, buf += 3, no++)
		{
			if(no >= num) return;

			mPixbufFillBox(pixbuf, x, y, cellw - 1, cellh - 1,
				mRGBtoPix_sep(buf[0], buf[1], buf[2]));
		}
	}
}

/* _palpage 作成 */

static _palpage *_create_page(mWidget *parent)
{
	_palpage *p;

	p = (_palpage *)mWidgetNew(parent, sizeof(_palpage));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;
	p->wg.fstate |= MWIDGET_STATE_ENABLE_DROP;
	p->wg.resize = _page_resize;
	p->wg.draw = _page_draw_handle;
	p->wg.event = _page_event_handle;

	return p;
}


/************************************
 * _palview (コンテナ)
 ************************************/


/* スクロールバー ハンドラ */

static void _view_scroll_handle(mWidget *p,int pos,uint32_t flags)
{
	if(flags & MSCROLLBAR_ACTION_F_CHANGE_POS)
		mWidgetRedraw((mWidget *)p->param1);
}

/** 作成 */

mWidget *ColorPaletteView_new(mWidget *parent,int id,mListItem *item)
{
	_palview *p;
	mScrollBar *sb;

	p = (_palview *)mContainerNew(parent, sizeof(_palview));

	mContainerSetType_horz(MLK_CONTAINER(p), 0);

	p->wg.id = id;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.draw = NULL;

	//page

	p->page = _create_page(MLK_WIDGET(p));

	p->page->item = (PaletteListItem *)item;

	//mScollBar

	sb = p->scr = mScrollBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0, MSCROLLBAR_S_VERT);

	sb->wg.param1 = (intptr_t)p->page;

	mScrollBarSetHandle_action(sb, _view_scroll_handle);

	//

	p->page->scr = sb;
	
	return (mWidget *)p;
}

/** 更新 */

void ColorPaletteView_update(mWidget *wg,mListItem *item)
{
	_palpage *p = ((_palview *)wg)->page;

	p->item = (PaletteListItem *)item;

	_page_set_xnum(p);
	_page_set_scroll(p);

	mWidgetRedraw(MLK_WIDGET(p));
}


