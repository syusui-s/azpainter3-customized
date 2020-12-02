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

/*******************************************
 * [イベントリスト]
 *******************************************/

#include "mDef.h"

#include "mAppDef.h"
#include "mAppPrivate.h"
#include "mList.h"
#include "mEvent.h"


//------------------------

typedef struct _mEventListItem
{
	mListItem i;
	mEvent event;
}mEventListItem;

#define EVLIST         (&(MAPP_PV->listEvent))
#define EVLISTITEM(p)  ((mEventListItem *)(p))

//------------------------


//======================


/** アイテム破棄ハンドラ */

static void _item_destroy(mListItem *p)
{
	if(EVLISTITEM(p)->event.data)
		mFree(EVLISTITEM(p)->event.data);
}

/** 指定イベントを後ろから検索 */

static mEventListItem *_search_bottom(mWidget *widget,int type)
{
	mEventListItem *pi;

	for(pi = EVLISTITEM(EVLIST->bottom); pi; pi = EVLISTITEM(pi->i.prev))
	{
		if(pi->event.widget == widget
			&& pi->event.type == type)
			return pi;
	}

	return NULL;
}

//======================


/** イベントをすべて削除 */

void mEventListEmpty(void)
{
	mListDeleteAll(EVLIST);
}

/** イベントがあるか */

mBool mEventListPending(void)
{
	return (EVLIST->top != NULL);
}

/** イベント追加 */

void mEventListAppend(mEvent *ev)
{
	mEventListItem *p;

	p = (mEventListItem *)mListAppendNew(EVLIST, sizeof(mEventListItem), _item_destroy);
	if(!p) return;
	
	p->event = *ev;
}

/** ウィジェットとイベントタイプを指定して追加 */

mEvent *mEventListAppend_widget(mWidget *widget,int type)
{
	mEventListItem *p;

	p = (mEventListItem *)mListAppendNew(EVLIST, sizeof(mEventListItem), _item_destroy);
	if(!p) return NULL;
	
	p->event.type   = type;
	p->event.widget = widget;
	
	return &p->event;
}

/** リスト内に同じイベントがあった場合はそれを返す。なければ追加 */

mEvent *mEventListAppend_only(mWidget *widget,int type)
{
	mEventListItem *pi;

	pi = _search_bottom(widget, type);

	if(pi)
		return &pi->event;
	else
	{
		//見つからないので追加
		return mEventListAppend_widget(widget, type);
	}
}

/** イベント取り出し */

mBool mEventListGetEvent(mEvent *ev)
{
	if(!EVLIST->top)
		return FALSE;
	else
	{
		*ev = EVLISTITEM(EVLIST->top)->event;

		/* アイテムの destroy() ハンドラを呼び出さずに
		 * アイテムを切り離す。
		 * (ev->data のメモリを解放しない) */
		
		mListDeleteNoDestroy(EVLIST, EVLIST->top);
		
		return TRUE;
	}
}

/** 指定ウィジェットのイベントをすべて削除 */

void mEventListDeleteWidget(mWidget *widget)
{
	mList *list = EVLIST;
	mListItem *p,*next;
	
	for(p = list->top; p; p = next)
	{
		next = p->next;
	
		if(EVLISTITEM(p)->event.widget == widget)
			mListDelete(list, p);
	}
}

/** 指定ウィジェット＆イベントタイプの一番新しいイベントを一つ削除
 *
 * @return イベントがあったか */

mBool mEventListDeleteLastEvent(mWidget *wg,int type)
{
	mEventListItem *pi;

	pi = _search_bottom(wg, type);
	if(!pi)
		return FALSE;
	else
	{
		mListDelete(EVLIST, M_LISTITEM(pi));
		return TRUE;
	}
}
