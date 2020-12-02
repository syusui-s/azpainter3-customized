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
 * タイマー実装
 * (OS にタイマー機能がない場合)
 *****************************************/


#include "mDef.h"

#include "mNanoTime.h"
#include "mList.h"
#include "mEvent.h"
#include "mEventList.h"


//-------------------

#define _TIMER_ITEM(p)  ((_apptimer_item *)(p))

typedef struct
{
	mListItem b;

	mWidget *widget;
	uint32_t timerid;
	uint64_t interval;	//nanosec
	intptr_t param;
	mNanoTime nt_end;
}_apptimer_item;

//-------------------


/** タイマーの最小待ち時間取得
 * 
 * @retval FALSE タイマーによる待ち時間なし */

mBool mAppTimerGetMinTime(mList *list,mNanoTime *nt_min)
{
	_apptimer_item *p;
	mNanoTime nt_now;
	
	p = _TIMER_ITEM(list->top);
	
	if(!p)
		return FALSE;
	else
	{
		mNanoTimeGet(&nt_now);
		
		if(!mNanoTimeSub(nt_min, &p->nt_end, &nt_now))
		{
			nt_min->sec  = 0;
			nt_min->nsec = 0;
		}
		
		return TRUE;
	}
}

/** タイマー処理 */

void mAppTimerProc(mList *list)
{
	mNanoTime nt_now;
	_apptimer_item *p,*next,*pmove;
	mEvent *ev;
	
	p = _TIMER_ITEM(list->top);
	if(!p) return;
	
	mNanoTimeGet(&nt_now);
	
	for(; p; p = next)
	{
		next = _TIMER_ITEM(p->b.next);
		
		//現在時刻より後
		
		if(mNanoTimeCompare(&nt_now, &p->nt_end) < 0)
			break;
		
		//次回の時間
		
		p->nt_end = nt_now;
		mNanoTimeAdd(&p->nt_end, p->interval);
		
		//リスト順を移動 (時間の小さい順)
		
		for(pmove = next
			; pmove && mNanoTimeCompare(&pmove->nt_end, &p->nt_end) < 0
			; pmove = _TIMER_ITEM(pmove->b.next));
		
		mListMove(list, M_LISTITEM(p), M_LISTITEM(pmove));
		
		//イベント
		
		ev = mEventListAppend_widget(p->widget, MEVENT_TIMER);
		
		if(ev)
		{
			ev->timer.id = p->timerid;
			ev->timer.param = p->param;
		}
	}
}

/** 追加 */

void mAppTimerAppend(mList *list,mWidget *wg,
	uint32_t timerid,uint32_t msec,intptr_t param)
{
	uint64_t itv;
	mNanoTime endt;
	_apptimer_item *p,*next,*ins;

	if(msec == 0) msec = 1;
	
	itv = (uint64_t)msec * 1000 * 1000;
	
	mNanoTimeGet(&endt);
	mNanoTimeAdd(&endt, itv);
	
	/* 同じウィジェット・ID がある場合は削除。
	 * 時間を小さい順に並べるための挿入位置も得る。 */

	for(p = _TIMER_ITEM(list->top), ins = NULL; p; p = next)
	{
		next = _TIMER_ITEM(p->b.next);
		
		if(p->widget == wg && p->timerid == timerid)
			mListDelete(list, M_LISTITEM(p));
		else if(!ins && mNanoTimeCompare(&endt, &p->nt_end) < 0)
			ins = p;
	}
	
	//新規挿入
	
	p = (_apptimer_item *)mListInsertNew(list,
			M_LISTITEM(ins), sizeof(_apptimer_item), NULL);

	if(!p) return;
	
	p->widget   = wg;
	p->timerid  = timerid;
	p->param    = param;
	p->interval = itv;
	p->nt_end   = endt;
}

/** タイマー削除
 *
 * @return タイマーを削除したか  */

mBool mAppTimerDelete(mList *list,mWidget *wg,uint32_t timerid)
{
	_apptimer_item *p;
	
	for(p = _TIMER_ITEM(list->top); p; p = _TIMER_ITEM(p->b.next))
	{
		if(p->widget == wg && p->timerid == timerid)
		{
			mListDelete(list, M_LISTITEM(p));
			return TRUE;
		}
	}
	
	return FALSE;
}

/** ウィジェットのタイマーすべて削除 */

void mAppTimerDeleteWidget(mList *list,mWidget *wg)
{
	_apptimer_item *p,*next;
	
	for(p = _TIMER_ITEM(list->top); p; p = next)
	{
		next = _TIMER_ITEM(p->b.next);
		
		if(p->widget == wg)
			mListDelete(list, M_LISTITEM(p));
	}
}

/** 指定タイマーが存在するか */

mBool mAppTimerIsExist(mList *list,mWidget *wg,uint32_t timerid)
{
	_apptimer_item *p;
	
	for(p = _TIMER_ITEM(list->top); p; p = _TIMER_ITEM(p->b.next))
	{
		if(p->widget == wg && p->timerid == timerid)
			return TRUE;
	}
	
	return FALSE;
}
