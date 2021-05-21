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
 * mContainerView
 * コンテナを垂直スクロールできるビュー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_containerview.h"
#include "mlk_scrollbar.h"
#include "mlk_event.h"


//========================
// sub
//========================


/* スクロール情報セット */

static void _set_scrollinfo(mContainerView *p)
{
	mScrollBarSetStatus(p->cv.scrv, 0, (p->cv.page)? p->cv.page->hintH: 0, p->wg.h);
}

/* スクロールバーの表示/非表示
 *
 * return: 表示の有無が変化したか */

static mlkbool _show_scrollbar(mContainerView *p)
{
	mlkbool fscr;

	//表示するか

	if(p->cv.fstyle & MCONTAINERVIEW_S_FIX_SCROLLBAR)
		fscr = 1;
	else
		fscr = (p->cv.page && p->cv.page->hintH > p->wg.h);

	//表示切り替え

	return mWidgetShow(MLK_WIDGET(p->cv.scrv), fscr);
}


//===========================
// スクロールバー ハンドラ
//===========================


/* event ハンドラ */

static int _scrollbar_event_handle(mWidget *wg,mEvent *ev)
{
	//垂直ホイールでスクロール
	
	if(ev->type == MEVENT_SCROLL
		&& ev->scroll.vert_step
		&& !ev->scroll.is_grab_pointer)
	{
		mScrollBar *scr = MLK_SCROLLBAR(wg);
		mContainerView *p = MLK_CONTAINERVIEW(scr->wg.param1);

		if(mScrollBarAddPos(scr, ev->scroll.vert_step * 30))
			mWidgetMove(MLK_WIDGET(p->cv.page), 0, -(scr->sb.pos));
		
		return 1;
	}

	return mScrollBarHandle_event(wg, ev);
}

/* action ハンドラ */

static void _scrollbar_action_handle(mWidget *wg,int pos,uint32_t flags)
{
	mContainerView *p = MLK_CONTAINERVIEW(wg->param1);

	//スクロール位置変更時、中身の表示位置を変更

	if(flags & MSCROLLBAR_ACTION_F_CHANGE_POS)
		mWidgetMove(MLK_WIDGET(p->cv.page), 0, -pos);
}


//========================
// ハンドラ
//========================


/* calc_hint ハンドラ */

static void _calchint_handle(mWidget *wg)
{
	mWidget *page = MLK_CONTAINERVIEW(wg)->cv.page;

	if(page)
	{
		//幅は、中身の推奨サイズからセット (高さはセットしない)。
		//calc_hint は下位ウィジェットから順に実行されるので、
		//子である page の推奨サイズはすでにセットされている。

		wg->hintW = page->hintW + MLK_SCROLLBAR_WIDTH;
	}
}

/* resize ハンドラ */

static void _resize_handle(mWidget *wg)
{
	_set_scrollinfo(MLK_CONTAINERVIEW(wg));

	_show_scrollbar(MLK_CONTAINERVIEW(wg));
}

/* レイアウト */

static void _layout_handle(mWidget *wg)
{
	mContainerView *p = MLK_CONTAINERVIEW(wg);
	int fscr,h;
	
	fscr = mWidgetIsVisible_self(MLK_WIDGET(p->cv.scrv));

	//中身

	if(p->cv.page)
	{
		h = (p->cv.page)->hintH;
	
		mWidgetMoveResize(p->cv.page,
			0, -(p->cv.scrv)->sb.pos,
			wg->w - (fscr? MLK_SCROLLBAR_WIDTH: 0),
			(h > wg->h)? h: wg->h);
	}

	//スクロールバー

	if(fscr)
	{
		mWidgetMoveResize(MLK_WIDGET(p->cv.scrv),
			wg->w - MLK_SCROLLBAR_WIDTH, 0,
			MLK_SCROLLBAR_WIDTH, wg->h); 
	}
}


//========================
// main
//========================


/**@ 作成 */

mContainerView *mContainerViewNew(mWidget *parent,int size,uint32_t fstyle)
{
	mContainerView *p;
	mScrollBar *scr;
	
	if(size < sizeof(mContainerView))
		size = sizeof(mContainerView);
	
	p = (mContainerView *)mWidgetNew(parent, size);
	if(!p) return NULL;

	p->wg.layout = _layout_handle;
	p->wg.resize = _resize_handle;
	p->wg.calc_hint = _calchint_handle;
	p->wg.draw = mWidgetDrawHandle_bkgnd;

	p->cv.fstyle = fstyle;

	//スクロールバー

	p->cv.scrv = scr = mScrollBarNew(MLK_WIDGET(p), 0, MSCROLLBAR_S_VERT);

	scr->wg.notify_to = MWIDGET_NOTIFYTO_PARENT;
	scr->wg.param1 = (intptr_t)p;
	scr->wg.fevent |= MWIDGET_EVENT_SCROLL;
	scr->wg.event = _scrollbar_event_handle;
	scr->sb.action = _scrollbar_action_handle;
	
	return p;
}

/**@ 中身のウィジェットをセット
 *
 * @p:page NULL で空にする */

void mContainerViewSetPage(mContainerView *p,mWidget *page)
{
	p->cv.page = page;
}

/**@ レイアウトを行う (自身のみ)
 *
 * @d:中身の状態が変わった時、ウィジェットを再構成したい場合に使う。\
 * 中身の推奨サイズから、スクロールバーの有無の判定と、レイアウト (中身とスクロールバーの配置) を行う。\
 * \
 * 中身自体の推奨サイズはあらかじめ計算されていること。 */

void mContainerViewLayout(mContainerView *p)
{
	_show_scrollbar(p);
	_set_scrollinfo(p);

	//中身のウィジェット内容が変わり、hintH が変化した場合があるので、
	//スクロールの切り替え状態にかかわらず、常に再レイアウトを行う。

	mWidgetLayout(MLK_WIDGET(p));
}

/**@ 再レイアウトを行う
 *
 * @d:中身の再レイアウトを行った後、mContainerView のレイアウトを行う。\
 * \
 * mContainerView 自体のサイズが変わらない状態で中身のサイズが変化する場合は、こちらを使う。 */

void mContainerViewReLayout(mContainerView *p)
{
	//中身の再レイアウト
	
	if(p->cv.page)
		mWidgetReLayout(p->cv.page);

	mContainerViewLayout(p);
}


