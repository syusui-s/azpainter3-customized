/*$
 Copyright (C) 2013-2020 Azel.

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
 * mContainerView
 *****************************************/

#include "mDef.h"
#include "mWidget.h"
#include "mEvent.h"
#include "mScrollBar.h"
#include "mWidget_pv.h"

#include "mContainerView.h"


/*********************//**

@defgroup containerview mContainerView
@brief コンテナを垂直スクロールできるビュー

- 内容ウィジェットはこのウィジェットの子として作成し、mContainerViewData::area にウィジェットポインタをセットする。

<h2>継承</h2>
mWidget \> mContainerView

@ingroup group_widget
@{

@file mContainerView.h

@struct mContainerView
@struct mContainerViewData
@enum MCONTAINERVIEW_STYLE

*************************/



/** スクロールバー、イベントハンドラ */

static int _scrollbar_evhandle(mWidget *wg,mEvent *ev)
{
	//ホイールでスクロール
	
	if(ev->type == MEVENT_SCROLL
		&& (ev->scr.dir == MEVENT_SCROLL_DIR_DOWN || ev->scr.dir == MEVENT_SCROLL_DIR_UP))
	{
		mScrollBar *scr = M_SCROLLBAR(wg);
		mContainerView *p = M_CONTAINERVIEW(scr->wg.param);

		if(mScrollBarMovePos(scr, (ev->scr.dir == MEVENT_SCROLL_DIR_UP)? -30: 30))
			mWidgetMove(M_WIDGET(p->cv.area), 0, -(scr->sb.pos));
		
		return 1;
	}

	return mScrollBarEventHandle(wg, ev);
}

/** スクロールバー操作時 */

static void _scrollbar_handle(mScrollBar *scrb,int pos,int flags)
{
	mContainerView *p = M_CONTAINERVIEW(scrb->wg.param);

	if(flags & MSCROLLBAR_N_HANDLE_F_CHANGE)
		mWidgetMove(M_WIDGET(p->cv.area), 0, -pos);
}

/** スクロール情報セット */

static void _set_scrollinfo(mContainerView *p)
{
	mScrollBarSetStatus(p->cv.scrv, 0, (p->cv.area)? p->cv.area->hintH: 0, p->wg.h);
}

/** スクロールバーの表示/非表示
 *
 * @return 表示の有無が変化したか */

static mBool _showScrollBar(mContainerView *p)
{
	int fscr;
	mBool ret = FALSE;

	if(p->cv.style & MCONTAINERVIEW_S_FIX_SCROLL)
		fscr = 1;
	else
		fscr = (p->cv.area && p->cv.area->hintH > p->wg.h);

	if(mWidgetIsVisible(M_WIDGET(p->cv.scrv)) != fscr)
	{
		mWidgetShow(M_WIDGET(p->cv.scrv), fscr);
		ret = TRUE;
	}

	return ret;
}

/** レイアウト */

static void _layout_handle(mWidget *wg)
{
	mContainerView *p = M_CONTAINERVIEW(wg);
	int fscr,h;
	
	fscr = mWidgetIsVisible(M_WIDGET(p->cv.scrv));

	//内容

	if(p->cv.area)
	{
		/* 内容の高さの方が長ければ、内容の最大高さのサイズにする。
		 * ウィジェットの高さの方が大きければ、そのまま */

		h = (p->cv.area)->hintH;
	
		mWidgetMoveResize(p->cv.area,
			0, -(p->cv.scrv)->sb.pos,
			wg->w - (fscr? MSCROLLBAR_WIDTH: 0),
			(h > wg->h)? h: wg->h);
	}

	//スクロールバー

	if(fscr)
	{
		mWidgetMoveResize(M_WIDGET(p->cv.scrv),
			wg->w - MSCROLLBAR_WIDTH, 0, MSCROLLBAR_WIDTH, wg->h); 
	}
}

/** サイズ変更時 */

static void _onsize_handle(mWidget *wg)
{
	mContainerView *p = M_CONTAINERVIEW(wg);

	_set_scrollinfo(p);

	_showScrollBar(p);
}

/** 推奨サイズ計算 */

static void _calchint_handle(mWidget *wg)
{
	mWidget *area = M_CONTAINERVIEW(wg)->cv.area;

	//[!] 高さはセットしない

	if(area)
	{
		__mWidgetCalcHint(area);

		wg->hintW = area->hintW + MSCROLLBAR_WIDTH;
	}
}


//========================
// main
//========================


/** 作成 */

mContainerView *mContainerViewNew(int size,mWidget *parent,uint32_t style)
{
	mContainerView *p;
	mScrollBar *scr;
	
	if(size < sizeof(mContainerView)) size = sizeof(mContainerView);
	
	p = (mContainerView *)mWidgetNew(size, parent);
	if(!p) return NULL;

	p->wg.layout = _layout_handle;
	p->wg.onSize = _onsize_handle;
	p->wg.calcHint = _calchint_handle;
	p->wg.draw = mWidgetHandleFunc_draw_drawBkgnd;

	p->cv.style = style;

	//スクロールバー

	p->cv.scrv = scr = mScrollBarNew(0, M_WIDGET(p), MSCROLLBAR_S_VERT);

	scr->wg.notifyTarget = MWIDGET_NOTIFYTARGET_PARENT;
	scr->wg.param = (intptr_t)p;
	scr->wg.fEventFilter |= MWIDGET_EVENTFILTER_SCROLL;
	scr->wg.event = _scrollbar_evhandle;
	scr->sb.handle = _scrollbar_handle;
	
	return p;
}

/** 構成を行う
 *
 * 中身の状態が変わった時に手動で更新したい場合。\n
 * mWidgetReLayout() を使う場合は実行しなくて良い。\n
 * スクロールバーの有無の再判定と再レイアウトを行う。 */

void mContainerViewConstruct(mContainerView *p)
{
	/* 中身の高さが変わった場合があるので、常に再レイアウトを行う */

	_showScrollBar(p);
	_set_scrollinfo(p);

	_layout_handle(M_WIDGET(p));
}

/* @} */
