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
 * GUI 用タイマー実装
 *****************************************/

/* リストは、常にタイマーの起動時間が早い順に並ぶ。 */


#include "mlk_gui.h"
#include "mlk_nanotime.h"
#include "mlk_list.h"
#include "mlk_event.h"

#include "mlk_pv_gui.h"


//-------------------

typedef struct
{
	mListItem i;

	mWidget *widget;	//NULL で内部処理用。id を type として使う。
	int id;
	uint64_t interval;	//間隔。nanosec
	intptr_t param;
	mNanoTime nt_end;
}_item;

#define _TIMERLIST_PTR  (&MLKAPP->list_timer)
#define _TIMERLIST_TOPITEM ((_item *)MLKAPP->list_timer.top)
#define _ITEM(p)        ((_item *)(p))

//-------------------


/** イベント待ち時の最大時間を取得
 *
 * タイマーのうち、一番残り時間が少ないものの時間。
 * 時間をチェックするだけ。
 * 
 * return: ミリ秒単位。
 *   -1 = タイマーがない。無限に待つ。
 *   -2 = タイマーの残り時間が大きすぎる。
 *   0  = 一番近いタイマーの時間がすでに過ぎている。 */

int mGuiTimerGetWaitTime_ms(void)
{
	_item *p;
	mNanoTime tnow,tmin;
	uint64_t n;

	//先頭アイテムが一番時間が近い

	p = _TIMERLIST_TOPITEM;
	if(!p) return -1;

	//タイマーあり

	mNanoTimeGet(&tnow);
	
	if(!mNanoTimeSub(&tmin, &p->nt_end, &tnow))
		//時間を過ぎている
		return 0;
	else
	{
		//残り時間 (ミリ秒)
		
		n = (uint64_t)tmin.sec * 1000 + tmin.ns / (1000 * 1000);

		if(n > INT32_MAX)
			return -2;
		else
			return (int)n;
	}
}

/** タイマー処理
 *
 * - 時間を過ぎていれば、イベント追加。
 * - 次回の時間をセット。
 * - リスト順を時間の早い順に入れ替え。 */

void mGuiTimerProc(void)
{
	mList *list;
	_item *p,*next,*pmove;
	mEventTimer *ev;
	mNanoTime nt_now;

	list = _TIMERLIST_PTR;
	
	p = _ITEM(list->top);
	if(!p) return;

	//現在時間

	mNanoTimeGet(&nt_now);

	//(現時点で時間の早い順に並んでいる)
	
	for(; p; p = next)
	{
		next = _ITEM(p->i.next);
		
		//現在時刻より後の場合、終了
		
		if(mNanoTimeCompare(&nt_now, &p->nt_end) < 0)
			break;
		
		//TIMER イベント追加

		ev = (mEventTimer *)mEventListAdd(p->widget,
			(p->widget)? MEVENT_TIMER: p->id,
			sizeof(mEventTimer));

		if(ev)
		{
			ev->id = p->id;
			ev->param = p->param;
		}

		//次回の起動時間

		p->nt_end = nt_now;
		mNanoTimeAdd(&p->nt_end, p->interval);
		
		//位置移動 (次回の時間を超える位置に挿入)
		
		for(pmove = next;
			pmove && mNanoTimeCompare(&pmove->nt_end, &p->nt_end) < 0;
			pmove = _ITEM(pmove->i.next));
		
		mListMove(list, MLISTITEM(p), MLISTITEM(pmove));
	}
}


//========================


/** タイマー追加 (ウィジェット用)
 *
 * wg と id が同じタイマーが存在する場合は、置き換える。 */

mlkbool mGuiTimerAdd(mWidget *wg,
	int id,uint32_t msec,intptr_t param)
{
	uint64_t itv;
	mNanoTime endt;
	mList *list;
	_item *p,*next,*ins;

	list = _TIMERLIST_PTR;

	if(msec == 0) msec = 1;
	
	itv = (uint64_t)msec * 1000 * 1000;
	
	mNanoTimeGet(&endt);
	mNanoTimeAdd(&endt, itv);
	
	//同じウィジェット・ID がある場合は削除。
	//時間を小さい順に並べるための挿入位置も得る。

	for(p = _ITEM(list->top), ins = NULL; p; p = next)
	{
		next = _ITEM(p->i.next);
		
		if(p->widget == wg && p->id == id)
			mListDelete(list, MLISTITEM(p));
		else if(!ins && mNanoTimeCompare(&endt, &p->nt_end) < 0)
			ins = p;
	}
	
	//新規挿入
	
	p = (_item *)mListInsertNew(list, MLISTITEM(ins), sizeof(_item));
	if(!p) return FALSE;
	
	p->widget   = wg;
	p->id       = id;
	p->param    = param;
	p->interval = itv;
	p->nt_end   = endt;

	return TRUE;
}

/** タイマー追加 (内部用)
 *
 * 重複判定は行わない。
 *
 * type: イベントタイプ */

mlkbool mGuiTimerAdd_app(int type,uint32_t msec,intptr_t param)
{
	uint64_t itv;
	mNanoTime endt;
	mList *list;
	_item *p,*ins;

	list = _TIMERLIST_PTR;

	if(msec == 0) msec = 1;
	
	itv = (uint64_t)msec * 1000 * 1000;
	
	mNanoTimeGet(&endt);
	mNanoTimeAdd(&endt, itv);
	
	//時間を小さい順に並べるための挿入位置を取得

	for(p = _ITEM(list->top), ins = NULL; p; p = _ITEM(p->i.next))
	{
		if(!ins && mNanoTimeCompare(&endt, &p->nt_end) < 0)
			ins = p;
	}
	
	//新規挿入
	
	p = (_item *)mListInsertNew(list, MLISTITEM(ins), sizeof(_item));
	if(!p) return FALSE;
	
	p->id       = type;
	p->param    = param;
	p->interval = itv;
	p->nt_end   = endt;

	return TRUE;
}

/** タイマーをすべて削除 */

void mGuiTimerDeleteAll(void)
{
	mListDeleteAll(_TIMERLIST_PTR);
}

/** タイマー削除
 *
 * return: タイマーを削除したか  */

mlkbool mGuiTimerDelete(mWidget *wg,int id)
{
	_item *p;
	
	for(p = _TIMERLIST_TOPITEM; p; p = _ITEM(p->i.next))
	{
		if(p->widget == wg && p->id == id)
		{
			mListDelete(_TIMERLIST_PTR, MLISTITEM(p));
			return TRUE;
		}
	}
	
	return FALSE;
}

/** ウィジェットのタイマーをすべて削除 */

void mGuiTimerDelete_widget(mWidget *wg)
{
	mList *list = _TIMERLIST_PTR;
	_item *p,*next;
	
	for(p = _ITEM(list->top); p; p = next)
	{
		next = _ITEM(p->i.next);
		
		if(p->widget == wg)
			mListDelete(list, MLISTITEM(p));
	}
}

/** 指定タイプのタイマーをすべて削除 (内部用) */

void mGuiTimerDelete_app(int type)
{
	mList *list = _TIMERLIST_PTR;
	_item *p,*next;
	
	for(p = _ITEM(list->top); p; p = next)
	{
		next = _ITEM(p->i.next);
		
		if(!p->widget && p->id == type)
			mListDelete(list, MLISTITEM(p));
	}
}

/** 指定タイマーが存在するか */

mlkbool mGuiTimerFind(mWidget *wg,int id)
{
	_item *p;
	
	for(p = _TIMERLIST_TOPITEM; p; p = _ITEM(p->i.next))
	{
		if(p->widget == wg && p->id == id)
			return TRUE;
	}
	
	return FALSE;
}
