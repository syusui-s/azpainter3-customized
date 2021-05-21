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
 * mListHeader
 * mListView のヘッダ部分
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_listheader.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_list.h"
#include "mlk_cursor.h"

#include "mlk_pv_widget.h"

//----------------------

#define _ITEM_TOP(p)  ((mListHeaderItem *)p->lh.list.top)

#define _PADDING_Y  2		//テキストの上下余白
#define _ITEM_PADDING_X  3	//アイテムの左右余白
#define _ARROW_W    7		//ソートの矢印幅
#define _ITEM_MINW  (_ITEM_PADDING_X * 2)	//アイテムの最小幅

//fpress
#define _PRESSF_RESIZE  1
#define _PRESSF_CLICK   2

static void _draw_handle(mWidget *p,mPixbuf *pixbuf);

//----------------------



//===========================
// sub
//===========================


/* ポインタ位置からアイテム取得
 *
 * dst_left: NULL 以外で、アイテムの左位置が入る */

static mListHeaderItem *_get_item_point(mListHeader *p,int x,int *dst_left)
{
	mListHeaderItem *pi;
	int xx;

	xx = -(p->lh.scrx);

	for(pi = _ITEM_TOP(p); pi; pi = MLK_LISTHEADER_ITEM(pi->i.next))
	{
		if(xx <= x && x < xx + pi->width)
		{
			if(dst_left) *dst_left = xx;
			return pi;
		}

		xx += pi->width;
	}

	return NULL;
}

/* アイテムの幅変更 */

static void _set_item_width(mListHeader *p,mListHeaderItem *pi,int w)
{
	if(w < _ITEM_MINW) w = _ITEM_MINW;

	if(pi->width != w)
	{
		pi->width = w;

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
			MLISTHEADER_N_RESIZE, (intptr_t)pi, w);

		mWidgetRedraw(MLK_WIDGET(p));
	}
}


//===========================
// ハンドラ
//===========================


/**@ calc_hint ハンドラ関数 */

void mListHeaderHandle_calcHint(mWidget *p)
{
	p->hintH = mWidgetGetFontHeight(p) + _PADDING_Y * 2;
}

/* resize ハンドラ */

static void _resize_handle(mWidget *wg)
{
	mListHeaderSetExpandItemWidth(MLK_LISTHEADER(wg));
}


//===========================
// main
//===========================


/* アイテム破棄ハンドラ */

static void _listitem_destroy(mList *list,mListItem *item)
{
	mListHeaderItem *pi = MLK_LISTHEADER_ITEM(item);

	if(pi->flags & MLISTHEADER_ITEM_F_COPYTEXT)
		mFree(pi->text);
}

/**@ mListHeader データ解放 */

void mListHeaderDestroy(mWidget *p)
{
	mListDeleteAll(&MLK_LISTHEADER(p)->lh.list);
}

/**@ 作成 */

mListHeader *mListHeaderNew(mWidget *parent,int size,uint32_t fstyle)
{
	mListHeader *p;
	
	if(size < sizeof(mListHeader))
		size = sizeof(mListHeader);
	
	p = (mListHeader *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mListHeaderDestroy;
	p->wg.calc_hint = mListHeaderHandle_calcHint;
	p->wg.event = mListHeaderHandle_event;
	p->wg.draw = _draw_handle;
	p->wg.resize = _resize_handle;

	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->lh.fstyle = fstyle;
	p->lh.list.item_destroy = _listitem_destroy;
	
	return p;
}

/**@ 作成 */

mListHeader *mListHeaderCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mListHeader *p;

	p = mListHeaderNew(parent, 0, fstyle);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ アイテム追加 */

void mListHeaderAddItem(mListHeader *p,
	const char *text,int width,uint32_t flags,intptr_t param)
{
	mListHeaderItem *pi;

	pi = (mListHeaderItem *)mListAppendNew(&p->lh.list, sizeof(mListHeaderItem));
	if(pi)
	{
		if(flags & MLISTHEADER_ITEM_F_COPYTEXT)
			pi->text = mStrdup(text);
		else
			pi->text = (char *)text;

		pi->width = width;
		pi->flags = flags;
		pi->param = param;
	}
}

/**@ 先頭アイテム取得 */

mListHeaderItem *mListHeaderGetTopItem(mListHeader *p)
{
	return _ITEM_TOP(p);
}

/**@ 指定位置のアイテム取得 */

mListHeaderItem *mListHeaderGetItem_atIndex(mListHeader *p,int index)
{
	return (mListHeaderItem *)mListGetItemAtIndex(&p->lh.list, index);
}

/**@ 指定位置のアイテムの幅を取得 */

int mListHeaderGetItemWidth(mListHeader *p,int index)
{
	mListHeaderItem *pi;

	pi = mListHeaderGetItem_atIndex(p, index);

	return (pi)? pi->width: 0;
}

/**@ すべてのアイテムの幅取得
 *
 * @d:拡張アイテムの場合は、現在の幅となる。 */

int mListHeaderGetFullWidth(mListHeader *p)
{
	mListHeaderItem *pi;
	int w = 0;

	for(pi = _ITEM_TOP(p); pi; pi = MLK_LISTHEADER_ITEM(pi->i.next))
		w += pi->width;

	return w;
}

/**@ 水平スクロール位置セット */

void mListHeaderSetScrollPos(mListHeader *p,int scrx)
{
	if(p->lh.scrx != scrx)
	{
		p->lh.scrx = scrx;
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ ソートの情報をセット
 *
 * @p:index ソート対象のアイテムの位置
 * @p:rev   ソートを逆順 (降順) にするか */

void mListHeaderSetSort(mListHeader *p,int index,mlkbool rev)
{
	p->lh.item_sort = (mListHeaderItem *)mListGetItemAtIndex(&p->lh.list, index);
	p->lh.is_sort_rev = (rev != 0);

	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 現在のウィジェット幅から、拡張アイテムの幅をセット */

void mListHeaderSetExpandItemWidth(mListHeader *p)
{
	mListHeaderItem *pi,*pi_expand = NULL;
	int w = 0;

	//拡張アイテム & 拡張以外のすべての幅を取得

	for(pi = _ITEM_TOP(p); pi; pi = MLK_LISTHEADER_ITEM(pi->i.next))
	{
		if(!pi_expand && (pi->flags & MLISTHEADER_ITEM_F_EXPAND))
			pi_expand = pi;
		else
			w += pi->width;
	}

	//拡張アイテムの幅セット

	if(pi_expand)
		_set_item_width(p, pi_expand, p->wg.w - w);
}

/**@ 指定位置のアイテムの幅をセットする
 *
 * @p:add_pad TRUE で、幅に、左右の余白分を追加する */

void mListHeaderSetItemWidth(mListHeader *p,int index,int width,mlkbool add_pad)
{
	mListHeaderItem *pi;

	if(add_pad) width += _ITEM_PADDING_X * 2;

	pi = mListHeaderGetItem_atIndex(p, index);
	if(pi) pi->width = width;
}

/**@ ウィジェットの X 位置におけるアイテムの位置を取得
 *
 * @r:先頭アイテムを 0 とする位置。-1 で範囲外。\
 *  p == NULL の場合は -1 となる。 */

int mListHeaderGetPos_atX(mListHeader *p,int x)
{
	mListHeaderItem *pi;
	int xx,pos;

	if(!p) return -1;

	xx = -(p->lh.scrx);
	pos = 0;

	for(pi = _ITEM_TOP(p); pi; pi = MLK_LISTHEADER_ITEM(pi->i.next))
	{
		if(x >= xx && x < xx + pi->width)
			return pos;

		xx += pi->width;
		pos++;
	}

	return -1;
}

/**@ 指定位置のアイテムの X 位置を取得 */

int mListHeaderGetItemX(mListHeader *p,int index)
{
	mListHeaderItem *pi;
	int x,pos = 0;

	x = -(p->lh.scrx);

	for(pi = _ITEM_TOP(p); pi; pi = MLK_LISTHEADER_ITEM(pi->i.next))
	{
		if(pos == index) break;

		x += pi->width;
		pos++;
	}

	return x;
}


//========================
// イベント
//========================


/* ボタン押し時 */

static void _event_press(mListHeader *p,int x)
{
	mListHeaderItem *pi;
	int left;

	pi = _get_item_point(p, x - 4, &left);
	if(!pi) return;

	if(!(pi->flags & MLISTHEADER_ITEM_F_FIX)
		&& x > left + pi->width - 4)
	{
		//境界上ドラッグで、幅をリサイズ
		
		p->lh.fpress = _PRESSF_RESIZE;
		p->lh.item_drag = pi;
		p->lh.drag_left = left;

		mWidgetGrabPointer(MLK_WIDGET(p));
	}
	else if(p->lh.fstyle & MLISTHEADER_S_SORT)
	{
		//ソートありの場合、クリックでソート項目と昇降順変更

		if(p->lh.item_sort == pi)
			p->lh.is_sort_rev = !p->lh.is_sort_rev;
		else
		{
			//アイテムが変わった場合、常に昇順
			p->lh.item_sort = pi;
			p->lh.is_sort_rev = 0;
		}

		p->lh.fpress = _PRESSF_CLICK;

		mWidgetGrabPointer(MLK_WIDGET(p));
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/* カーソル移動時 */

static void _event_motion(mListHeader *p,int x)
{
	mListHeaderItem *pi;
	int left;

	if(p->lh.fpress == _PRESSF_RESIZE)
	{
		//----- リサイズ中

		_set_item_width(p, p->lh.item_drag, x - p->lh.drag_left);
	}
	else if(p->lh.fpress == 0)
	{
		//通常移動時
		// 境界周辺の上に来たらカーソル変更
	
		pi = _get_item_point(p, x - 4, &left);

		if(pi
			&& !(pi->flags & MLISTHEADER_ITEM_F_FIX)
			&& x > left + pi->width - 4)	//右端の上にある時だけ
			mWidgetSetCursor_cache_type(MLK_WIDGET(p), MCURSOR_TYPE_RESIZE_COL);
		else
			mWidgetSetCursor(MLK_WIDGET(p), 0);
	}
}

/* グラブ解放 */

static void _release_grab(mListHeader *p)
{
	if(p->lh.fpress)
	{
		if(p->lh.fpress == _PRESSF_CLICK)
		{
			//ソート変更通知
			
			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
				MLISTHEADER_N_CHANGE_SORT, (intptr_t)p->lh.item_sort, p->lh.is_sort_rev);

			mWidgetRedraw(MLK_WIDGET(p));
		}
	
		p->lh.fpress = 0;
		mWidgetUngrabPointer();
	}
}

/**@ イベントハンドラ関数 */

int mListHeaderHandle_event(mWidget *wg,mEvent *ev)
{
	mListHeader *p = MLK_LISTHEADER(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
				_event_motion(p, ev->pt.x);
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				if(ev->pt.btt == MLK_BTT_LEFT && !p->lh.fpress)
					_event_press(p, ev->pt.x);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//ENTER
		case MEVENT_ENTER:
			_event_motion(p, ev->enter.x);
			break;
		//LEAVE
		case MEVENT_LEAVE:
			mWidgetSetCursor(wg, 0);
			break;

		//フォーカス
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}

/* 描画 ハンドラ */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mListHeader *p = MLK_LISTHEADER(wg);
	mFont *font;
	mListHeaderItem *pi;
	int x,w,wh,xx,dw;
	uint32_t col_frame,colrgb_text;
	mlkbool have_arrow,enable_sort;
	
	font = mWidgetGetFont(wg);
	wh = wg->h;
	enable_sort = ((p->lh.fstyle & MLISTHEADER_S_SORT) != 0);

	col_frame = MGUICOL_PIX(FRAME);
	colrgb_text = MGUICOL_RGB(TEXT);

	//背景

	mPixbufDrawBkgndShadow(pixbuf, 0, 0, wg->w, wh - 1, MGUICOL_BUTTON_FACE);

	//下枠線

	mPixbufLineH(pixbuf, 0, wh - 1, wg->w, col_frame);

	//項目

	x = -(p->lh.scrx);

	for(pi = _ITEM_TOP(p); pi; x += pi->width, pi = MLK_LISTHEADER_ITEM(pi->i.next))
	{
		w = pi->width;
	
		if(x + w <= 0) continue;
		if(x >= wg->w) break;

		//クリック時の背景

		if(p->lh.fpress == _PRESSF_CLICK && p->lh.item_sort == pi)
			mPixbufDrawBkgndShadowRev(pixbuf, x, 0, w - 1, wh - 1, MGUICOL_BUTTON_FACE_PRESS);

		//右境界線

		mPixbufLineV(pixbuf, x + w - 1, 0, wh, col_frame);

		//----------

		dw = w - _ITEM_PADDING_X * 2;

		have_arrow = (enable_sort && pi == p->lh.item_sort);

		//ソート矢印

		if(have_arrow)
		{
			if(mPixbufClip_setBox_d(pixbuf, x + _ITEM_PADDING_X, 0, dw, wh))
			{
				xx = x + w - _ITEM_PADDING_X - _ARROW_W;

				if(p->lh.is_sort_rev)
					mPixbufDrawArrowUp(pixbuf, xx, (wh - 4) >> 1, 4, MGUICOL_PIX(TEXT));
				else
					mPixbufDrawArrowDown(pixbuf, xx, (wh - 4) >> 1, 4, MGUICOL_PIX(TEXT));
			}

			dw -= _ARROW_W + 2;
		}

		//テキスト

		xx = x + _ITEM_PADDING_X;

		if(mPixbufClip_setBox_d(pixbuf, xx, 0, dw, wh))
		{
			//右寄せ

			if(pi->flags & MLISTHEADER_ITEM_F_RIGHT)
			{
				xx = x + w - _ITEM_PADDING_X - mFontGetTextWidth(font, pi->text, -1);
				if(have_arrow) xx -= _ARROW_W + 2;
			}

			mFontDrawText_pixbuf(font, pixbuf, xx, _PADDING_Y, pi->text, -1, colrgb_text);
		}

		//

		mPixbufClip_clear(pixbuf);
	}
}

