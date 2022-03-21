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
 * mSplitter
 *****************************************/

#include <stdlib.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_splitter.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_cursor.h"

/*
  press_pos : ボタン押し時のカーソル位置 (x or y)
  drag_diff : ドラッグ中の変化幅 (+ で右/下方向へ移動)
*/

//--------------

#define _SPL_WIDTH  6	//バーの厚さ
#define _SPL_MINW   9	//最小幅
#define _SPL_KNOB_JUDGE 8  //つまみの操作の左右幅
#define _IS_VERT(p)  (p->spl.fstyle & MSPLITTER_S_VERT)

//fpointer
#define _POINTERF_NONE   0
#define _POINTERF_ONCTRL 1	//つまみ上にカーソルがある
#define _POINTERF_DRAG   2	//ドラッグ中

//--------------



/* コールバック関数デフォルト (前後のウィジェットを操作) */

static int _gettarget_default(mSplitter *p,mSplitterTarget *info)
{
	mWidget *prev,*next;
	mSize sizep,sizen;

	prev = p->wg.prev;
	next = p->wg.next;

	//前と次のいずれかがない
	
	if(!prev || !next) return 0;

	//

	info->wgprev = prev;
	info->wgnext = next;

	//現在のサイズ

	mWidgetGetSize_visible(prev, &sizep);
	mWidgetGetSize_visible(next, &sizen);

	if(_IS_VERT(p))
	{
		info->prev_cur = sizep.w;
		info->next_cur = sizen.w;
	}
	else
	{
		info->prev_cur = sizep.h;
		info->next_cur = sizen.h;
	}

	//最小値

	info->prev_min = info->next_min = 0;

	//最大値
	
	info->prev_max = info->next_max = info->prev_cur + info->next_cur;

	return 1;
}


//=========================
// main
//=========================


/**@ mSplitter データ解放 */

void mSplitterDestroy(mWidget *wg)
{

}

/**@ 作成
 *
 * @d:レイアウトフラグは、常に (水平時) MLF_EXPAND_W、(垂直時) MLF_EXPAND_H がセットされる。 */

mSplitter *mSplitterNew(mWidget *parent,int size,uint32_t fstyle)
{
	mSplitter *p;
	
	if(size < sizeof(mSplitter))
		size = sizeof(mSplitter);
	
	p = (mSplitter *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = mSplitterHandle_draw;
	p->wg.event = mSplitterHandle_event;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->spl.fstyle = fstyle;
	p->spl.get_target = _gettarget_default;

	//

	if(fstyle & MSPLITTER_S_VERT)
	{
		//垂直
		p->wg.flayout = MLF_EXPAND_H;
		p->wg.hintW = _SPL_WIDTH;
		p->wg.hintH = _SPL_MINW;
	}
	else
	{
		//水平
		p->wg.flayout = MLF_EXPAND_W;
		p->wg.hintW = _SPL_MINW;
		p->wg.hintH = _SPL_WIDTH;
	}
	
	return p;
}

/**@ コールバック関数セット (対象の情報取得)
 *
 * @p:func  NULL で、デフォルトの関数にセット
 * @p:param mSplitterTarget::param にセットされる値 */

void mSplitterSetFunc_getTarget(mSplitter *p,mFuncSplitterGetTarget func,intptr_t param)
{
	p->spl.get_target = (func)? func: _gettarget_default;
	p->spl.info.param = param;
}


//========================
// sub
//========================


/* ポインタ位置が、つまみの操作範囲内か */

static int _is_in_ctrl(mSplitter *p,int x,int y)
{
	int ct,pos;

	if(_IS_VERT(p))
	{
		ct = p->wg.h >> 1;
		pos = y;
	}
	else
	{
		ct = p->wg.w >> 1;
		pos = x;
	}

	return (pos >= ct - _SPL_KNOB_JUDGE && pos <= ct + _SPL_KNOB_JUDGE);
}

/* ドラッグ中のバー描画 */

static void _draw_dragbar(mSplitter *p)
{
	mPixbuf *pixbuf;
	mPoint pt;
	int x,y;

	x = y = 0;

	if(_IS_VERT(p))
		x = p->spl.drag_diff;
	else
		y = p->spl.drag_diff;

	//描画位置

	mWidgetGetDrawPos_abs(MLK_WIDGET(p), &pt);

	pt.x += x;
	pt.y += y;

	//mPixbuf に直接描画

	pixbuf = (p->wg.toplevel)->win.pixbuf;

	mPixbufSetPixelModeXor(pixbuf);

	mPixbufFillBox_pat2x2(pixbuf, pt.x, pt.y, p->wg.w, p->wg.h, 0);

	mPixbufSetPixelModeCol(pixbuf);

	//更新

	mWidgetUpdateBox_d(MLK_WIDGET(p), x, y, p->wg.w, p->wg.h);
}

/* 結果を適用する */

static void _set_split(mSplitter *p)
{
	mSplitterTarget *info = &p->spl.info;
	mWidget *prev,*next,*wg;
	mBox box1,box2;
	int diff,diffx,diffy,is_resize_next;

	prev = info->wgprev;
	next = info->wgnext;

	mWidgetGetBox_parent(prev, &box1);
	mWidgetGetBox_parent(next, &box2);

	diff = p->spl.drag_diff;

	is_resize_next = (info->next_min != info->next_max);

	//

	if(_IS_VERT(p))
	{
		//------ 垂直

		if(!mWidgetIsVisible(prev)) box1.w = 0;
		if(!mWidgetIsVisible(next)) box2.w = 0;

		box1.w += diff;
		box2.x += diff;
		if(is_resize_next) box2.w -= diff;
		
		diffx = diff;
		diffy = 0;

		mWidgetShow(prev, (box1.w != 0));
		mWidgetShow(next, (box2.w != 0));
	}
	else
	{
		//------- 水平

		if(!mWidgetIsVisible(prev)) box1.h = 0;
		if(!mWidgetIsVisible(next)) box2.h = 0;

		box1.h += diff;
		box2.y += diff;
		if(is_resize_next) box2.h -= diff;

		diffx = 0;
		diffy = diff;

		mWidgetShow(prev, (box1.h != 0));
		mWidgetShow(next, (box2.h != 0));
	}

	//移動

	mWidgetMoveResize(prev, box1.x, box1.y, box1.w, box1.h);
	mWidgetMoveResize(next, box2.x, box2.y, box2.w, box2.h);

	mWidgetMove(MLK_WIDGET(p), p->wg.x + diffx, p->wg.y + diffy);

	//splitter と prev/next との間に1つ以上のウィジェットがある場合

	for(wg = p->wg.prev; wg != prev; wg = wg->prev)
		mWidgetMove(wg, wg->x + diffx, wg->y + diffy);

	for(wg = p->wg.next; wg != next; wg = wg->next)
		mWidgetMove(wg, wg->x + diffx, wg->y + diffy);

	//通知

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, MSPLITTER_N_MOVED, diff, 0);
}


//========================
// ハンドラ
//========================


/* ENTER/MOTION 時 */

static void _event_motion(mSplitter *p,int x,int y)
{
	int f;

	if(_is_in_ctrl(p, x,  y))
		f = _POINTERF_ONCTRL;
	else
		f = 0;

	if(f != p->spl.fpointer)
	{
		p->spl.fpointer = f;

		//カーソル変更

		if(!f)
			mWidgetSetCursor(MLK_WIDGET(p), 0);
		else
		{
			mWidgetSetCursor_cache_type(MLK_WIDGET(p),
				(_IS_VERT(p))? MCURSOR_TYPE_RESIZE_COL: MCURSOR_TYPE_RESIZE_ROW);
		}
	}
}

/* ボタン押し時 (ONCTRL 時) */

static void _event_press(mSplitter *p,mEventPointer *ev)
{
	//ターゲットの情報取得

	if(!(p->spl.get_target)(p, &p->spl.info)) return;

	//

	p->spl.fpointer = _POINTERF_DRAG;
	p->spl.drag_diff = 0;

	if(_IS_VERT(p))
		p->spl.press_pos = ev->x;
	else
		p->spl.press_pos = ev->y;

	//ドラッグ中は状態を直接描画するため、
	//現在のウィジェットをすべて mPixbuf に描画

	mGuiDrawWidgets();

	_draw_dragbar(p);

	mWidgetGrabPointer(MLK_WIDGET(p));
}

/* ドラッグ時 */

static void _event_drag(mSplitter *p,mEventPointer *ev)
{
	mSplitterTarget *info = &p->spl.info;
	int diff,size;

	//開始時からの変化幅

	diff = ((_IS_VERT(p))? ev->x: ev->y) - p->spl.press_pos;

	//prev

	size = info->prev_cur + diff;

	if(size < info->prev_min)
		//最小値に調整
		diff = info->prev_min - info->prev_cur;
	else if(size > info->prev_max)
		//最大値に調整
		diff = info->prev_max - info->prev_cur;

	//next (min,max が同じ場合、next のサイズは変更しない)

	if(info->next_min != info->next_max)
	{
		size = info->next_cur - diff;

		if(size < info->next_min)
			diff = info->next_cur - info->next_min;
		else if(size > info->next_max)
			diff = info->next_cur - info->next_max;
	}

	//バー描画

	if(diff != p->spl.drag_diff)
	{
		_draw_dragbar(p);

		p->spl.drag_diff = diff;

		_draw_dragbar(p);
	}
}

/* ドラッグ解除
 *
 * ev: NULL 以外で、RELEASE イベント */

static void _release_grab(mSplitter *p,mEvent *ev)
{
	if(p->spl.fpointer == _POINTERF_DRAG)
	{
		_draw_dragbar(p);

		if(p->spl.drag_diff)
			_set_split(p);

		mWidgetUngrabPointer();

		//離した位置で再判定

		p->spl.fpointer = _POINTERF_ONCTRL;

		if(ev)
			_event_motion(p, ev->pt.x, ev->pt.y);
	}
}

/**@ event ハンドラ関数 */

int mSplitterHandle_event(mWidget *wg,mEvent *ev)
{
	mSplitter *p = MLK_SPLITTER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->spl.fpointer == _POINTERF_DRAG)
					_event_drag(p, (mEventPointer *)ev);
				else
					_event_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT
					&& p->spl.fpointer == _POINTERF_ONCTRL)
					_event_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p, ev);
			}
			break;

		case MEVENT_ENTER:
			if(p->spl.fpointer != _POINTERF_DRAG)
				_event_motion(p, ev->enter.x, ev->enter.y);
			break;
	
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p, NULL);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mSplitterHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mSplitter *p = MLK_SPLITTER(wg);
	uint32_t col;
	int i,n;

	//背景

	mWidgetDrawBkgnd_force(wg, NULL);

	//つまみ

	col = MGUICOL_PIX(FRAME);

	if(_IS_VERT(p))
	{
		n = (wg->h - 7) >> 1;

		for(i = 0; i < 3; i++)
			mPixbufLineH(pixbuf, 1, n + i * 3, wg->w - 2, col); 
	}
	else
	{
		n = (wg->w - 7) >> 1;

		for(i = 0; i < 3; i++)
			mPixbufLineV(pixbuf, n + i * 3, 1, wg->h - 2, col);
	}
}

