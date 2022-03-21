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
 * mScrollBar [スクロールバー]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_scrollbar.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"

#include "mlk_pv_widget.h"


/* [!] max の値は、実際の最大値より +1 された値。 */


//-----------------------

#define _IS_VERT(p)   ((p)->sb.fstyle & MSCROLLBAR_S_VERT)

#define _TIMERID_NOTIFY    0
#define _TIMER_TIME_NOTIFY 20

//-----------------------



//=================================
// sub
//=================================


/** バーの情報取得
 * 
 * gripw: つまみ部分の幅
 * grippos: バーの表示位置。NULL でなし。
 * [!] 値の範囲がない場合、両方 0 になる。 */

static void _get_bar_state(mScrollBar *p,int *dst_gripw,int *dst_grippos)
{
	int pos,gw,w;

	gw = pos = 0;
	
	if(p->sb.range)
	{
		//バー全体の幅
	
		w = (_IS_VERT(p))? p->wg.h: p->wg.w;
		
		//つまみバーの幅
		
		gw = (int)((double)w / (p->sb.max - p->sb.min) * p->sb.page + 0.5);
		if(gw < 8) gw = 8;
		
		//バー位置

		if(dst_grippos)
		{
			pos = (int)((double)(w - gw) / p->sb.range * (p->sb.pos - p->sb.min) + 0.5);
			
			if(pos < 0)
				pos = 0;
			else if(pos > w - gw)
				pos = w - gw;
		}
	}
	
	*dst_gripw = gw;
	if(dst_grippos) *dst_grippos = pos;
}

/** 内部での位置変更時 */

static void _change_pos(mScrollBar *p,int pos,uint32_t flags)
{
	//位置変更

	if(mScrollBarSetPos(p, pos))
		flags |= MSCROLLBAR_ACTION_F_CHANGE_POS;

	//通知

	if(p->sb.action)
		(p->sb.action)(MLK_WIDGET(p), p->sb.pos, flags);
}

/** action ハンドラデフォルト */

static void _action_handle(mWidget *p,int pos,uint32_t flags)
{
	mWidgetEventAdd_notify(p, NULL, MSCROLLBAR_N_ACTION, pos, flags);
}


//=================================
// メイン
//=================================


/**@ mScrollBar データ解放 */

void mScrollBarDestroy(mWidget *p)
{

}

/**@ 作成 */

mScrollBar *mScrollBarNew(mWidget *parent,int size,uint32_t fstyle)
{
	mScrollBar *p;
	
	if(size < sizeof(mScrollBar))
		size = sizeof(mScrollBar);
	
	p = (mScrollBar *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = mScrollBarHandle_draw;
	p->wg.event = mScrollBarHandle_event;
	
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.hintW = p->wg.hintH = MLK_SCROLLBAR_WIDTH;
	
	p->sb.fstyle = fstyle;
	p->sb.action = _action_handle;

	return p;
}

/**@ 作成 */

mScrollBar *mScrollBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mScrollBar *p;

	p = mScrollBarNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ 現在位置が先頭か */

mlkbool mScrollBarIsPos_top(mScrollBar *p)
{
	return (p->sb.pos == p->sb.min);
}

/**@ 現在位置が終端か */

mlkbool mScrollBarIsPos_bottom(mScrollBar *p)
{
	return (p->sb.pos == p->sb.max - p->sb.page);
}

/**@ 位置取得 */

int mScrollBarGetPos(mScrollBar *p)
{
	return p->sb.pos;
}

/**@ ページ値取得 */

int mScrollBarGetPage(mScrollBar *p)
{
	return p->sb.page;
}

/**@ 位置の最大値を取得
 *
 * @d:最大値からページ値を引いた値 */

int mScrollBarGetMaxPos(mScrollBar *p)
{
	return p->sb.max - p->sb.page;
}

/**@ 操作ハンドラをセット
 *
 * @d:ハンドラがセットされている場合、ACTION 通知の代わりにハンドラ関数が呼ばれる。 */

void mScrollBarSetHandle_action(mScrollBar *p,mFuncScrollBarHandle_action action)
{
	p->sb.action = action;
}

/**@ 範囲のステータス変更
 *
 * @d:max は、最小で min + 1 となる。\
 * page は最小で 1、最大で max - min となる。\
 * page >= max - min の場合、位置は常に min となる。
 *
 * @p:max バー位置の実際の範囲は、0〜max - 1 となる。
 * @p:page 0 以下の場合、page は max - min となる。 */

void mScrollBarSetStatus(mScrollBar *p,int min,int max,int page)
{
	if(min >= max) max = min + 1;

	if(page <= 0 || page > max - min)
		page = max - min;

	p->sb.min   = min;
	p->sb.max   = max;
	p->sb.page  = page;
	p->sb.range = max - min - page;

	if(p->sb.pos < min)
		p->sb.pos = min;
	else if(p->sb.pos > max - page)
		p->sb.pos = max - page;

	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ ページ値の変更
 * 
 * @d:※ページ値の変更により、現在位置が調整される場合がある。
 *
 * @p:page 0 以下で、max - min となる。
 * @r:[0] ページ値変更なし\
 * [1] ページ値が変わった (現在位置は変更なし)\
 * [2] ページ値が変わった (現在位置が調整された) */

int mScrollBarSetPage(mScrollBar *p,int page)
{
	int min,max,ret;

	min = p->sb.min;
	max = p->sb.max;

	if(page <= 0 || page > max - min)
		page = max - min;
	
	if(page == p->sb.page)
		//変化なし
		return 0;
	else
	{
		p->sb.page = page;
		p->sb.range = max - min - page;
		
		if(p->sb.pos < min)
		{
			p->sb.pos = min;
			ret = 2;
		}
		else if(p->sb.pos > max - p->sb.page)
		{
			p->sb.pos = max - p->sb.page;
			ret = 2;
		}
		else
			ret = 1;
		
		mWidgetRedraw(MLK_WIDGET(p));

		return ret;
	}
}

/**@ 現在位置をセット
 *
 * @r:位置が変わったか */

mlkbool mScrollBarSetPos(mScrollBar *p,int pos)
{
	if(pos < p->sb.min)
		pos = p->sb.min;
	else if(pos > p->sb.max - p->sb.page)
		pos = p->sb.max - p->sb.page;
	
	if(pos == p->sb.pos)
		return FALSE;
	else
	{
		p->sb.pos = pos;
		mWidgetRedraw(MLK_WIDGET(p));
		return TRUE;
	}
}

/**@ 現在位置を終端にセット */

mlkbool mScrollBarSetPos_bottom(mScrollBar *p)
{
	return mScrollBarSetPos(p, p->sb.max - p->sb.page);
}

/**@ 現在位置に指定値を追加 */

mlkbool mScrollBarAddPos(mScrollBar *p,int val)
{
	return mScrollBarSetPos(p, p->sb.pos + val);
}


//========================
// ハンドラ
//========================


/* 左ボタン押し時 */

static void _event_press_left(mScrollBar *p,int x,int y)
{
	int gripw,grippos,pos,w;
	
	_get_bar_state(p, &gripw, &grippos);
	
	if(gripw == 0) return;
	
	//
	
	if(_IS_VERT(p))
		pos = y, w = p->wg.h;
	else
		pos = x, w = p->wg.w;

	//

	if(grippos <= pos && pos < grippos + gripw)
	{
		//つまみ内 (現在位置からスタート)
		
		p->sb.drag_diff = pos - grippos;
		pos = p->sb.pos;
	}
	else
	{
		//つまみ外 (押された位置からスタート)
	
		p->sb.drag_diff = gripw / 2;
		pos = (int)((double)(pos - gripw / 2) / (w - gripw) * p->sb.range + p->sb.min + 0.5);
	}

	//ドラッグ開始

	p->sb.fgrab = TRUE;
	
	mWidgetGrabPointer(MLK_WIDGET(p));
	
	_change_pos(p, pos, MSCROLLBAR_ACTION_F_PRESS);
}

/* ドラッグ時 */

static void _event_motion(mScrollBar *p,int x,int y)
{
	int gripw,pos,w;
	
	_get_bar_state(p, &gripw, NULL);
		
	if(_IS_VERT(p))
		w = p->wg.h, pos = y;
	else
		w = p->wg.w, pos = x;
	
	pos = (int)((double)(pos - p->sb.drag_diff) / (w - gripw) * p->sb.range + p->sb.min + 0.5);
	
	//位置変更時の通知は、タイマーで指定時間ごとに送る
	//(スクロール時に連動させる描画などの処理は重くなるため、通知自体を減らす)
	
	if(mScrollBarSetPos(p, pos))
		mWidgetTimerAdd_ifnothave(MLK_WIDGET(p), _TIMERID_NOTIFY, _TIMER_TIME_NOTIFY, 0);
}

/* グラブ解放 */

static void _release_grab(mScrollBar *p)
{
	if(p->sb.fgrab)
	{
		uint32_t flags = MSCROLLBAR_ACTION_F_RELEASE;
	
		p->sb.fgrab = 0;
		mWidgetUngrabPointer();
		
		//タイマー削除
		//(タイマーが残っている場合、位置変更フラグ ON)
		
		if(mWidgetTimerDelete(MLK_WIDGET(p), _TIMERID_NOTIFY))
			flags |= MSCROLLBAR_ACTION_F_CHANGE_POS;
		
		_change_pos(p, p->sb.pos, flags);
	}
}

/**@ event ハンドラ関数 */

int mScrollBarHandle_event(mWidget *wg,mEvent *ev)
{
	mScrollBar *p = MLK_SCROLLBAR(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動
				
				if(p->sb.fgrab)
					_event_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し
				
				if(ev->pt.btt == MLK_BTT_LEFT && !p->sb.fgrab)
					_event_press_left(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;
		
		//タイマー (ドラッグ中)
		// 位置が変更された時に遅延して送られる
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, ev->timer.id);
			
			_change_pos(p, p->sb.pos,
				MSCROLLBAR_ACTION_F_DRAG | MSCROLLBAR_ACTION_F_CHANGE_POS);
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mScrollBarHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mScrollBar *p = MLK_SCROLLBAR(wg);
	int gripw,grippos;
		
	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(SCROLLBAR_FACE));

	//バー
	
	_get_bar_state(p, &gripw, &grippos);
	
	if(gripw)
	{
		if(_IS_VERT(p))
		{
			mPixbufLineV(pixbuf, 0, 0, wg->h, MGUICOL_PIX(FRAME));
			mPixbufFillBox(pixbuf, 1, grippos, wg->w - 1, gripw, MGUICOL_PIX(SCROLLBAR_GRIP));
		}
		else
		{
			mPixbufLineH(pixbuf, 0, 0, wg->w, MGUICOL_PIX(FRAME));
			mPixbufFillBox(pixbuf, grippos, 1, gripw, wg->h - 1, MGUICOL_PIX(SCROLLBAR_GRIP));
		}
	}
}

