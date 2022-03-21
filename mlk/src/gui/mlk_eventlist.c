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

/*******************************************
 * GUI イベントリスト (内部用)
 *******************************************/

/* 先頭 (古い) -> 終端 (新しい) */


#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_event.h"
#include "mlk_widget_def.h"
#include "mlk_string.h"

#include "mlk_pv_gui.h"


//------------------------

#define _LIST     MLKAPP->list_event
#define _LISTPTR  (&(MLKAPP->list_event))

#define _GETEVENT(p)  (mEvent *)((uint8_t *)(p) + sizeof(mListItem))

//------------------------


//======================
// sub
//======================


/** 指定イベントを一番新しいものから検索 */

/*
static mEvent *_search_bottom(mWidget *widget,int type,mListItem **ppitem)
{
	mListItem *pi;
	mEvent *ev;

	for(pi = _LIST.bottom; pi; pi = pi->prev)
	{
		ev = _GETEVENT(pi);
	
		if(ev->widget == widget && ev->type == type)
		{
			if(ppitem) *ppitem = pi;
			return ev;
		}
	}

	return NULL;
}
*/

/* アイテム破棄ハンドラ */

static void _item_destroy(mList *list,mListItem *item)
{
	mEvent *p = _GETEVENT(item);

	switch(p->type)
	{
		case MEVENT_DROP_FILES:
			mStringFreeArray_tonull(((mEventDropFiles *)p)->files);
			break;
	}
}


//======================
// main
//======================


/** イベントリスト初期化 */

void mEventListInit(void)
{
	_LIST.item_destroy = _item_destroy;
}

/** イベントをすべて削除 */

void mEventListEmpty(void)
{
	mListDeleteAll(_LISTPTR);
}

/** イベント取得後のアイテム削除 */

void mEventFreeItem(void *item)
{
	_item_destroy(NULL, MLISTITEM(item));

	mFree(item);
}

/** イベント追加 (データサイズ指定) */

mEvent *mEventListAdd(mWidget *wg,int type,int size)
{
	mListItem *p;
	mEvent *ev;

	p = mListAppendNew(_LISTPTR, sizeof(mListItem) + size);
	if(!p) return NULL;

	ev = _GETEVENT(p);

	ev->type = type;
	ev->widget = wg;

	return ev;
}

/** イベント追加 (mEventBase) */

void mEventListAdd_base(mWidget *widget,int type)
{
	mEventListAdd(widget, type, sizeof(mEventBase));
}

/** イベント追加 (FOCUS) */

void mEventListAdd_focus(mWidget *widget,mlkbool is_out,int from)
{
	mEventFocus *ev;

	ev = (mEventFocus *)mEventListAdd(widget, MEVENT_FOCUS, sizeof(mEventFocus));
	if(ev)
	{
		ev->is_out = is_out;
		ev->from = from;
	}
}

/** 一番古いイベントを取得
 *
 * リストから取り外される。
 * 使用後は *itemptr を mEventFreeItem() で解放すること。
 *
 * itemptr: リストアイテムのポインタが入る。
 * return: NULL でイベントなし */

mEvent *mEventListGetEvent(void **itemptr)
{
	mList *list = _LISTPTR;
	mListItem *pi;

	pi = list->top;
	
	if(!pi)
		return NULL;
	else
	{
		*itemptr = pi;

		mListRemoveItem(list, pi);

		return _GETEVENT(pi);
	}
}

/** 指定ウィジェットのイベントをすべて削除 */

void mEventListDelete_widget(mWidget *widget)
{
	mList *list = _LISTPTR;
	mListItem *p,*next;
	mEvent *ev;
	
	for(p = list->top; p; p = next)
	{
		next = p->next;

		ev = _GETEVENT(p);
	
		if(ev->widget == widget)
			mListDelete(list, p);
	}
}

/** 終端の連続する CONFIGURE イベントを一つにまとめる
 *
 * X11用、最大化などの状態変化後のサイズ変化をまとめて一つにする */

void mEventListCombineConfigure(void)
{
	mListItem *last,*prev;
	mEvent *evlast,*evprev;

	last = _LIST.bottom;
	if(!last) return;

	prev = last->prev;
	if(!prev) return;

	//

	evlast = _GETEVENT(last);
	evprev = _GETEVENT(prev);

	if(evlast->type == MEVENT_CONFIGURE
		&& evprev->type == MEVENT_CONFIGURE
		&& evlast->widget == evprev->widget
		&& evprev->config.flags == MEVENT_CONFIGURE_F_STATE
		&& evlast->config.flags == MEVENT_CONFIGURE_F_SIZE)
	{
		evprev->config.width = evlast->config.width;
		evprev->config.height = evlast->config.height;
		evprev->config.flags |= evlast->config.flags;

		//last 削除
	
		mListDelete(_LISTPTR, last);
	}
}

