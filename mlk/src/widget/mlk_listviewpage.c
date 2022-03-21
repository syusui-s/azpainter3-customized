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
 * mListViewPage
 * (mListView の内容)
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_scrollview.h"
#include "mlk_listview.h"
#include "mlk_listviewpage.h"
#include "mlk_scrollbar.h"
#include "mlk_listheader.h"
#include "mlk_imagelist.h"
#include "mlk_list.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_key.h"

#include "mlk_columnitem_manager.h"


//--------------------

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//--------------------


/*-------------------

 [!] フォーカスアイテムが変更されたら、MLISTVIEW_N_CHANGE_FOCUS を通知すること。

 - (複数列時) mListViewPage の子として、mListHeader が上端に配置される。

 - アイテムの高さが変更されたら、mListViewPage の item_height も変更すること。

 - キーフォーカスは親ウィジェットにある。


manager, imglist, item_height:
	mListView の値をそのままコピー。

fstyle_listview:
	親が mListView でない場合があるため、リストビューに関するスタイルはここでセットする。

widget_send:
	アイテムの draw ハンドラ時に指定されるウィジェット。
	mListView なら、mListView。
	mComboBox なら、mComboBox。

header_height:
	mListHeader の高さ

---------------------*/


//=========================
// sub
//=========================


/* 通知を送る
 *
 * 通知元 = 親、通知先 = 親の通知先。
 * MLISTVIEW_N_CHANGE_FOCUS 時の param1 は自動セットされる。 */

static void _notify(mListViewPage *p,int type,intptr_t param1,intptr_t param2)
{
	if(type == MLISTVIEW_N_CHANGE_FOCUS)
		param1 = (intptr_t)(p->lvp.manager->item_focus);

	mWidgetEventAdd_notify(p->wg.parent, NULL, type, param1, param2);
}

/* ポインタ位置からアイテム取得 */

static mColumnItem *_get_item_point(mListViewPage *p,int x,int y)
{
	mPoint pt;
	
	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(p), &pt);

	y += pt.y - p->lvp.header_height;
	
	if(y < 0)
		return NULL; 
	else
		return mCIManagerGetItem_atIndex(p->lvp.manager, y / p->lvp.item_height);
}

/* layout ハンドラ */

static void _layout_handle(mWidget *wg)
{
	mListViewPage *p = MLK_LISTVIEWPAGE(wg);

	//mListHeader を上端へ

	if(p->lvp.header)
		mWidgetMoveResize(MLK_WIDGET(p->lvp.header), 0, 0, wg->w, p->lvp.header_height);
}

/* resize ハンドラ */

static void _resize_handle(mWidget *wg)
{
	mListViewPage *p = MLK_LISTVIEWPAGE(wg);

	//水平

	if(mScrollViewPage_setScrollPage_horz(MLK_SCROLLVIEWPAGE(p), wg->w))
	{
		//スクロール位置が変わった時、ヘッダスクロール位置セット
		
		if(p->lvp.header)
		{
			mListHeaderSetScrollPos(p->lvp.header,
				mScrollViewPage_getScrollPos_horz(MLK_SCROLLVIEWPAGE(p)));
		}
	}

	//垂直

	mScrollViewPage_setScrollPage_vert(MLK_SCROLLVIEWPAGE(p), wg->h - p->lvp.header_height);
}

/* mScrollViewPage ハンドラ */

static void _getscrollpage_handle(mWidget *wg,int horz,int vert,mSize *dst)
{
	dst->w = horz;
	dst->h = vert - MLK_LISTVIEWPAGE(wg)->lvp.header_height;
}


//=========================
// main
//=========================


/**@ mListViewPage データ解放 */

void mListViewPageDestroy(mWidget *wg)
{

}

/**@ 作成 */

mListViewPage *mListViewPageNew(mWidget *parent,int size,
	uint32_t fstyle,uint32_t fstyle_listview,
	mCIManager *manager,mWidget *send)
{
	mListViewPage *p;
	mListHeader *header;

	//mScrollViewPage
	
	if(size < sizeof(mListViewPage))
		size = sizeof(mListViewPage);
	
	p = (mListViewPage *)mScrollViewPageNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.event = mListViewPageHandle_event;
	p->wg.layout = _layout_handle;
	p->wg.resize = _resize_handle;
	p->wg.fstate |= MWIDGET_STATE_ENABLE_KEY_REPEAT;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;

	p->svp.getscrollpage = _getscrollpage_handle;

	p->lvp.manager = manager;
	p->lvp.fstyle = fstyle;
	p->lvp.fstyle_listview = fstyle_listview;
	p->lvp.widget_send = send;

	//複数列の場合、mListHeader 作成
	//[!] ヘッダを表示しない場合も、各列の幅を決めるために必要

	if(fstyle_listview & MLISTVIEW_S_MULTI_COLUMN)
	{
		p->lvp.header = header = (mListHeader *)mListHeaderNew(
			MLK_WIDGET(p), 0,
			(fstyle_listview & MLISTVIEW_S_HEADER_SORT)? MLISTHEADER_S_SORT: 0);

		//通知は親 (mListView) へ送る

		header->wg.notify_to = MWIDGET_NOTIFYTO_PARENT;

		//ヘッダの高さを計算

		mListHeaderHandle_calcHint(MLK_WIDGET(header));

		//ヘッダを表示しない場合は、非表示

		if(fstyle_listview & MLISTVIEW_S_HAVE_HEADER)
			p->lvp.header_height = header->wg.hintH;
		else
			header->wg.fstate &= ~MWIDGET_STATE_VISIBLE;
	}
	
	return p;
}

/**@ アイコンイメージセット */

void mListViewPage_setImageList(mListViewPage *p,mImageList *img)
{
	p->lvp.imglist = img;
}

/**@ フォーカスアイテムが画面上に表示されない範囲にある場合、垂直スクロール位置を調整
 *
 * @d:現在のスクロール位置で問題ない場合は、そのまま。\
 *  上下キーでの移動時と、ポップアップ表示の開始時に使われる。
 *
 * @p:dir  [0] 方向なし [負] 上方向移動時 [正] 下方向移動時
 * @p:margin_num  上下に余白を付けたい場合、余白のアイテム数 */

void mListViewPage_scrollToFocus(mListViewPage *p,int dir,int margin_num)
{
	mScrollBar *scrv;
	int pos,y,itemh;
	
	mScrollViewPage_getScrollBar(MLK_SCROLLVIEWPAGE(p), NULL, &scrv);
	if(!scrv) return;

	itemh = p->lvp.item_height;
	pos = scrv->sb.pos;
	y = mCIManagerGetItemIndex(p->lvp.manager, NULL) * itemh;

	margin_num *= itemh;

	if(dir == 0)
	{
		//スクロールせずに表示できる場合はそのまま。
		//スクロールする場合は、アイテムが先頭に来るように。
	
		if(y < pos + margin_num
			|| y > pos + scrv->sb.page - itemh - margin_num)
			mScrollBarSetPos(scrv, y - margin_num);
	}
	else if(dir < 0)
	{
		//上移動 (上方向に見えない場合)

		if(y < pos + margin_num)
			mScrollBarSetPos(scrv, y - margin_num);
	}
	else if(dir > 0)
	{
		//下移動 (下方向に見えない場合)

		if(y > pos + scrv->sb.page - itemh - margin_num)
			mScrollBarSetPos(scrv, y - scrv->sb.page + itemh + margin_num);
	}
}

/**@ アイテムの Y 位置を取得
 *
 * @r:mListViewPage の左上を基準とした Y 位置。 */

int mListViewPage_getItemY(mListViewPage *p,mColumnItem *pi)
{
	return mCIManagerGetItemIndex(p->lvp.manager, pi) * p->lvp.item_height
		+ p->lvp.header_height - mScrollViewPage_getScrollPos_vert(MLK_SCROLLVIEWPAGE(p));
}


//========================
// sub - イベント
//========================


/** カーソルが item のアイテム上にある時、チェックボタンの処理を行う
 *
 * 通常クリックと、ダブルクリック時に呼ばれる。
 * 
 * return: TRUE で、チェクボックス内の処理を行った */

static mlkbool _proc_checkbox(mListViewPage *p,mColumnItem *item,mEventPointer *ev)
{
	mPoint pt;
	int itemh;

	if(!(p->lvp.fstyle_listview & MLISTVIEW_S_CHECKBOX))
		return FALSE;
	
	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(p), &pt);

	//X 位置

	pt.x += ev->x;

	if(pt.x < MLK_LISTVIEWPAGE_ITEM_PADDING_X
		|| pt.x >= MLK_LISTVIEWPAGE_ITEM_PADDING_X + MLK_LISTVIEWPAGE_CHECKBOX_SIZE)
		return FALSE;

	//Y 位置

	itemh = p->lvp.item_height;

	pt.y += ev->y - p->lvp.header_height;
	
	pt.y = (pt.y - (pt.y / itemh) * itemh) - (itemh - MLK_LISTVIEWPAGE_CHECKBOX_SIZE) / 2;
	
	if(pt.y < 0 || pt.y >= MLK_LISTVIEWPAGE_CHECKBOX_SIZE)
		return FALSE;

	//チェックボックス内
	
	item->flags ^= MCOLUMNITEM_F_CHECKED;
	
	_notify(p, MLISTVIEW_N_CHANGE_CHECK, (intptr_t)item, ((item->flags & MCOLUMNITEM_F_CHECKED) != 0));
	
	return TRUE;
}

/** PageUp/Down キー
 *
 * dir: 移動方向 (-1 or 1) */

static void _page_updown(mListViewPage *p,int dir)
{
	mScrollBar *scrv;
	int pos,itemh;
	
	mScrollViewPage_getScrollBar(MLK_SCROLLVIEWPAGE(p), NULL, &scrv);
	if(!scrv) return;

	//スクロール位置
	
	pos = scrv->sb.pos + dir * scrv->sb.page;
	
	if(mScrollBarSetPos(scrv, pos))
	{
		mWidgetRedraw(MLK_WIDGET(p));
	
		//フォーカスアイテムの変更
		//PageUp は上端、PageDown は下端の位置のアイテム
		
		itemh = p->lvp.item_height;
		pos = scrv->sb.pos;
		
		if(dir < 0)
			pos += itemh - 1;
		else
			pos += scrv->sb.page - itemh;
		
		if(mCIManagerSetFocusItem_atIndex(p->lvp.manager, pos / itemh))
			_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, -1);
	}
}

/** Home/End キー */

static void _key_home_end(mListViewPage *p,mlkbool home)
{
	mScrollBar *scrv;

	//アイテム選択
	
	if(mCIManagerSetFocus_toHomeEnd(p->lvp.manager, home))
		_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, -1);

	//スクロール位置

	mScrollViewPage_getScrollBar(MLK_SCROLLVIEWPAGE(p), NULL, &scrv);
	if(scrv)
	{
		if(home)
			mScrollBarSetPos(scrv, scrv->sb.min);
		else
			mScrollBarSetPos_bottom(scrv);
	}
	
	mWidgetRedraw(MLK_WIDGET(p));
}


//========================
// イベントハンドラ
//========================


/** ボタン押し (左/右) */

static void _event_btt_press(mListViewPage *p,mEventPointer *ev)
{
	mColumnItem *pi;
	int ret;
	mPoint pt;

	//ポップアップ時は、ボタン押しで即終了

	if(p->lvp.fstyle & MLISTVIEWPAGE_S_POPUP)
	{
		//to: mListView の通知先
		//from: mListViewPage
	
		mWidgetEventAdd_notify(MLK_WIDGET(p), mWidgetGetNotifyWidget(p->wg.parent),
			MLISTVIEWPAGE_N_POPUP_QUIT, 0, 0);
		return;
	}

	//親にフォーカスセット

	mWidgetSetFocus_redraw(p->wg.parent, FALSE);

	//アイテム処理
	
	pi = _get_item_point(p, ev->x, ev->y);
	if(pi)
	{
		//選択操作の処理
		
		ret = mCIManagerOnClick(p->lvp.manager, ev->state, pi);

		//フォーカス変更通知

		if(ret & MCIMANAGER_ONCLICK_CHANGE_FOCUS)
		{
			_notify(p, MLISTVIEW_N_CHANGE_FOCUS,
				0, mListHeaderGetPos_atX(p->lvp.header, ev->x));
		}
		else if((ret & MCIMANAGER_ONCLICK_ON_FOCUS) && ev->btt == MLK_BTT_LEFT)
		{
			//フォーカス上を左押し

			_notify(p, MLISTVIEW_N_CLICK_ON_FOCUS,
				(intptr_t)pi, mListHeaderGetPos_atX(p->lvp.header, ev->x));
		}

		//チェックボックス処理
		//(+Ctrl/Shift 操作以外時)
		
		if(ev->btt == MLK_BTT_LEFT
			&& !(ret & MCIMANAGER_ONCLICK_KEY_SEL))
			_proc_checkbox(p, pi, ev);

		mWidgetRedraw(MLK_WIDGET(p));
	}
			
	//右ボタン押し
	
	if(ev->btt == MLK_BTT_RIGHT)
	{
		pt.x = ev->x;
		pt.y = ev->y;
		
		mWidgetMapPoint(MLK_WIDGET(p), p->wg.parent, &pt);
	
		_notify(p, MLISTVIEW_N_ITEM_R_CLICK,
			(intptr_t)pi, (pt.x << 16) | pt.y);
	}
}

/** 左ダブルクリック */

static void _event_dblclk(mListViewPage *p,mEventPointer *ev)
{
	mColumnItem *pi;
	
	pi = _get_item_point(p, ev->x, ev->y);
	if(pi)
	{
		if(_proc_checkbox(p, pi, ev))
			//チェックボックス
			mWidgetRedraw(MLK_WIDGET(p));
		else
		{
			//チェックボックス以外の部分: 通知

			_notify(p, MLISTVIEW_N_ITEM_L_DBLCLK,
				(intptr_t)pi, mListHeaderGetPos_atX(p->lvp.header, ev->x));
		}
	}
}

/** (ポップアップ時) カーソル移動 */

static void _event_motion_popup(mListViewPage *p,mEventPointer *ev)
{
	mColumnItem *pi;
	int ret;
	
	pi = _get_item_point(p, ev->x, ev->y);
	if(pi)
	{
		ret = mCIManagerOnClick(p->lvp.manager, 0, pi);

		if(ret & MCIMANAGER_ONCLICK_CHANGE_FOCUS)
			mWidgetRedraw(MLK_WIDGET(p));
	}
}

/** スクロール */

static void _event_scroll(mListViewPage *p,mEventScroll *ev)
{
	mScrollBar *scrh,*scrv;
	int pos,n;

	if(ev->is_grab_pointer) return;

	mScrollViewPage_getScrollBar(MLK_SCROLLVIEWPAGE(p), &scrh, &scrv);

	//垂直

	if(scrv)
	{
		pos = scrv->sb.pos + ev->vert_step * p->lvp.item_height * 3;

		if(mScrollBarSetPos(scrv, pos))
			mWidgetRedraw(MLK_WIDGET(p));
	}

	//水平

	if(scrh)
	{
		n = scrh->sb.page >> 1;
		if(n < 10) n = 10;
		
		pos = mScrollBarGetPos(scrh) + ev->horz_step * n;
		
		if(mScrollBarSetPos(scrh, pos))
			mWidgetRedraw(MLK_WIDGET(p));
	}
}

/** キー押し */

static int _event_key_down(mListViewPage *p,mEventKey *ev)
{
	mlkbool update = FALSE;

	if(ev->is_grab_pointer) return FALSE;

	switch(ev->key)
	{
		//上
		case MKEY_UP:
		case MKEY_KP_UP:
			if(mCIManagerFocusItem_updown(p->lvp.manager, FALSE))
			{
				mListViewPage_scrollToFocus(p, -1, 0);
				_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, -1);
				update = TRUE;
			}
			break;

		//下
		case MKEY_DOWN:
		case MKEY_KP_DOWN:
			if(mCIManagerFocusItem_updown(p->lvp.manager, TRUE))
			{
				mListViewPage_scrollToFocus(p, 1, 0);
				_notify(p, MLISTVIEW_N_CHANGE_FOCUS, 0, -1);
				update = TRUE;
			}
			break;

		//PageUp/Down
		case MKEY_PAGE_UP:
		case MKEY_KP_PAGE_UP:
			_page_updown(p, -1);
			break;
		case MKEY_PAGE_DOWN:
		case MKEY_KP_PAGE_DOWN:
			_page_updown(p, 1);
			break;

		//Home/End
		case MKEY_HOME:
		case MKEY_KP_HOME:
			_key_home_end(p, TRUE);
			break;
		case MKEY_END:
		case MKEY_KP_END:
			_key_home_end(p, FALSE);
			break;
		
		//Ctrl+A (すべて選択)
		case 'A':
		case 'a':
			if((ev->state & MLK_STATE_MASK_MODS) == MLK_STATE_CTRL
				&& (p->lvp.fstyle_listview & MLISTVIEW_S_MULTI_SEL))
			{
				mCIManagerSelectAll(p->lvp.manager);
				update = TRUE;
			}
			else
				return FALSE;
			break;
		
		default:
			return FALSE;
	}
	
	//更新
	
	if(update)
		mWidgetRedraw(MLK_WIDGET(p));
	
	return TRUE;
}

/** NOTIFY イベント */

static void _event_notify(mListViewPage *p,mEventNotify *ev)
{
	int type = ev->notify_type;

	if(ev->widget_from == p->wg.parent)
	{
		//---- 親から: スクロール操作時
		
		if((type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_VERT
			|| type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ)
			&& (ev->param2 & MSCROLLBAR_ACTION_F_CHANGE_POS))
		{
			//mListHeader の水平スクロール
			
			if(type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ
				&& p->lvp.header)
				mListHeaderSetScrollPos(p->lvp.header, ev->param1);

			//
		
			mWidgetRedraw(MLK_WIDGET(p));
		}
	}
	else if(p->lvp.header
		&& ev->widget_from == (mWidget *)p->lvp.header)
	{
		//---- ヘッダから

		if(type == MLISTHEADER_N_RESIZE)
		{
			//列サイズが変更 -> 親を再構成
			//(水平スクロールの表示/非表示が変わる場合があるため)

			mWidgetSetConstruct(p->wg.parent);
		}
		else if(type == MLISTHEADER_N_CHANGE_SORT)
		{
			//列クリックでソート情報変更 -> 通知

			_notify(p, MLISTVIEW_N_CHANGE_SORT, ev->param1, ev->param2);
		}
	}
}

/**@ event ハンドラ関数 */

int mListViewPageHandle_event(mWidget *wg,mEvent *ev)
{
	mListViewPage *p = MLK_LISTVIEWPAGE(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動 (ポップアップ時)
				
				if(p->lvp.fstyle & MLISTVIEWPAGE_S_POPUP)
					_event_motion_popup(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT || ev->pt.btt == MLK_BTT_RIGHT)
					_event_btt_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//ダブルクリック

				if(ev->pt.btt == MLK_BTT_LEFT)
					_event_dblclk(p, (mEventPointer *)ev);
			}
			break;

		//通知
		case MEVENT_NOTIFY:
			_event_notify(p, (mEventNotify *)ev);
			break;
		
		//スクロール
		case MEVENT_SCROLL:
			_event_scroll(p, (mEventScroll *)ev);
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			return _event_key_down(p, (mEventKey *)ev);
		
		default:
			return FALSE;
	}

	return TRUE;
}


//=======================
// 描画
//=======================


/** チェックボックス・アイコン描画
 *
 * return: 次の x 位置 */

static int _draw_item_left(mListViewPage *p,
	mPixbuf *pixbuf,int x,int y,int itemh,mColumnItem *pi)
{
	//ヘッダアイテムの場合、なし

	if(pi->flags & MCOLUMNITEM_F_HEADER)
		return x;

	//チェックボックス
	
	if(p->lvp.fstyle_listview & MLISTVIEW_S_CHECKBOX)
	{
		mPixbufDrawCheckBox(pixbuf,
			x, y + (itemh - MLK_LISTVIEWPAGE_CHECKBOX_SIZE) / 2,
			(pi->flags & MCOLUMNITEM_F_CHECKED)? MPIXBUF_DRAWCKBOX_CHECKED: 0);
		
		x += MLK_LISTVIEWPAGE_CHECKBOX_SIZE + MLK_LISTVIEWPAGE_CHECKBOX_SPACE_RIGHT;
	}

	//アイコン
	//(アイコンなしの場合は位置だけ進める)

	if(p->lvp.imglist)
	{
		mImageList *img = p->lvp.imglist;

		if(pi->icon >= 0)
			mImageListPutPixbuf(img, pixbuf, x, y + (itemh - img->h) / 2, pi->icon, 0);

		x += img->eachw + MLK_LISTVIEWPAGE_ICON_SPACE_RIGHT;
	}
	
	return x;
}

/** 描画 */

#define _DRAWF_FOCUSED       1  //親ウィジェットにフォーカスあり
#define _DRAWF_SINGLE_COLUMN 2	//単一列
#define _DRAWF_GRID_ROW      4  //水平グリッドあり

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mListViewPage *p = MLK_LISTVIEWPAGE(wg);
	mFont *font;
	mColumnItem *pi;
	mListHeaderItem *hi,*hi_top;
	mPoint scr;
	int wg_w,wg_h,n,x,y,itemh,texty,header_height,hindex,len,xx;
	uint32_t col_grid, drawflags_base;
	uint8_t flags, is_enable, is_sel, is_focus, is_header, *buf;
	mColumnItemDrawInfo dinfo;
	
	font = mWidgetGetFont(wg);

	wg_w = wg->w;
	wg_h = wg->h;
	header_height = p->lvp.header_height;
	itemh = p->lvp.item_height;
	is_enable = mWidgetIsEnable(wg);
	
	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(wg), &scr);

	col_grid = MGUICOL_PIX(GRID);

	//全体のフラグ
	
	flags = 0;

	if((wg->parent->fstate & MWIDGET_STATE_FOCUSED)
		|| (p->lvp.fstyle & MLISTVIEWPAGE_S_POPUP))	//ポップアップ時はフォーカスが来ないため、常にフォーカスありとみなす
		flags |= _DRAWF_FOCUSED;
	
	if(!(p->lvp.fstyle_listview & MLISTVIEW_S_MULTI_COLUMN)) flags |= _DRAWF_SINGLE_COLUMN;
	if(p->lvp.fstyle_listview & MLISTVIEW_S_GRID_ROW) flags |= _DRAWF_GRID_ROW;

	//独自描画時のフラグ

	drawflags_base = 0;
	if(flags & _DRAWF_FOCUSED) drawflags_base |= MCOLUMNITEM_DRAWF_FOCUSED;
	if(!is_enable) drawflags_base |= MCOLUMNITEM_DRAWF_DISABLED;

	//複数列

	if(p->lvp.header)
		hi_top = MLK_LISTHEADER_ITEM(p->lvp.header->lh.list.top);

	//全体の背景
	
	mPixbufFillBox(pixbuf, 0, 0, wg_w, wg_h,
		mGuiCol_getPix(is_enable? MGUICOL_FACE_TEXTBOX: MGUICOL_FACE_DISABLE));

	//垂直グリッド

	if((p->lvp.fstyle_listview & MLISTVIEW_S_GRID_COL) && p->lvp.header)
	{
		x = -scr.x;
		
		for(hi = MLK_LISTHEADER_ITEM(p->lvp.header->lh.list.top)
			; hi && hi->i.next
			; hi = MLK_LISTHEADER_ITEM(hi->i.next))
		{
			x += hi->width;
			mPixbufLineV(pixbuf, x - 1, header_height, wg_h - 1, col_grid);
		}
	}
	
	//--------- アイテム
	
	pi = MLK_COLUMNITEM(p->lvp.manager->list.top);
	y = -scr.y + header_height;
	
	texty = (itemh - mFontGetHeight(font)) >> 1; //テキストは垂直中央寄せ

	//
	
	for(; pi; pi = MLK_COLUMNITEM(pi->i.next), y += itemh)
	{
		if(y + itemh <= header_height) continue;
		if(y >= wg_h) break;

		is_sel = ((pi->flags & MCOLUMNITEM_F_SELECTED) != 0); //アイテム選択状態
		is_focus = (p->lvp.manager->item_focus == pi);  //フォーカスアイテムか
		is_header = ((pi->flags & MCOLUMNITEM_F_HEADER) != 0); //ヘッダアイテム

		//---- 共通描画
		
		//項目背景色

		if(is_sel)
		{
			if(!(flags & _DRAWF_FOCUSED))
				n = MGUICOL_FACE_SELECT_UNFOCUS;
			else
				n = (is_focus)? MGUICOL_FACE_SELECT: MGUICOL_FACE_SELECT_LIGHT;
		}
		else if(is_header)
			n = MGUICOL_FACE_DARK;
		else
			n = -1;

		if(n != -1)
			mPixbufFillBox(pixbuf, 0, y, wg_w, itemh, mGuiCol_getPix(n));

		//水平グリッド線
		
		if(flags & _DRAWF_GRID_ROW)
			mPixbufLineH(pixbuf, 0, y + itemh - 1, wg_w, col_grid);

		//---- アイテム

		if(flags & _DRAWF_SINGLE_COLUMN)
		{
			//------ 単一列
						
			mPixbufClip_setBox_d(pixbuf,
				MLK_LISTVIEWPAGE_ITEM_PADDING_X, y,
				wg_w - MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2, itemh);

			//内容の左端
			
			x = _draw_item_left(p, pixbuf,
				MLK_LISTVIEWPAGE_ITEM_PADDING_X - scr.x, y, itemh, pi);

			//描画関数 (n == 0 で描画した)

			n = 1;

			if(pi->draw)
			{
				dinfo.widget = p->lvp.widget_send;
				dinfo.box.x = x;
				dinfo.box.y = y;
				dinfo.box.w = wg_w - x - MLK_LISTVIEWPAGE_ITEM_PADDING_X;
				dinfo.box.h = itemh;
				dinfo.column = -1;
				dinfo.flags = drawflags_base;

				if(is_sel)
					dinfo.flags |= MCOLUMNITEM_DRAWF_ITEM_SELECTED;

				if(is_focus)
					dinfo.flags |= MCOLUMNITEM_DRAWF_ITEM_FOCUS;

				if(mPixbufClip_setBox(pixbuf, &dinfo.box))
					n = (pi->draw)(pixbuf, pi, &dinfo);
				else
					n = 0;
			}

			//通常テキスト

			if(n)
			{
				if(is_header)
				{
					//ヘッダ項目の場合は中央寄せ
				
					x = (wg_w - pi->text_width) / 2;

					if(x < MLK_LISTVIEWPAGE_ITEM_PADDING_X)
						x = MLK_LISTVIEWPAGE_ITEM_PADDING_X;
				}

				if(is_sel)
					n = MGUICOL_TEXT_SELECT;
				else
					n = MGUICOL_TEXT;
				
				mFontDrawText_pixbuf(font, pixbuf,
					x, y + texty, pi->text, -1, mGuiCol_getRGB(n));
			}
		}
		else if(p->lvp.header)
		{
			//---------- 複数列

			buf = pi->text_col;
			hindex = 0;
			x = -scr.x;

			for(hi = hi_top; hi; hi = MLK_LISTHEADER_ITEM(hi->i.next), hindex++)
			{
				//テキスト長さ

				for(len = 0; buf && *buf != 255;)
				{
					n = *(buf++);
					len += n;
					if(n < 254) break;
				}

				//X 描画範囲内

				if(x + hi->width > 0 && x < wg->w)
				{
					//xx = 描画左位置
					
					xx = x + MLK_LISTVIEWPAGE_ITEM_PADDING_X;

					mPixbufClip_setBox_d(pixbuf,
						xx, y, hi->width - MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2, itemh);

					//(先頭の列時) アイコンなど

					if(hindex == 0)
						xx = _draw_item_left(p, pixbuf, xx, y, itemh, pi);

					//描画関数 (n == 0 で描画した)

					n = 1;

					if(pi->draw)
					{
						dinfo.widget = p->lvp.widget_send;
						dinfo.box.x = xx;
						dinfo.box.y = y;
						dinfo.box.w = hi->width - MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2;
						dinfo.box.h = itemh;
						dinfo.flags = drawflags_base;
						dinfo.column = hindex;

						if(is_sel)
							dinfo.flags |= MCOLUMNITEM_DRAWF_ITEM_SELECTED;

						if(is_focus)
							dinfo.flags |= MCOLUMNITEM_DRAWF_ITEM_FOCUS;

						if(mPixbufClip_setBox(pixbuf, &dinfo.box))
							n = (pi->draw)(pixbuf, pi, &dinfo);
						else
							n = 0;
					}

					//通常テキスト

					if(n && len)
					{
						//右寄せ (2番目の列以降)

						if(hindex && (hi->flags & MLISTHEADER_ITEM_F_RIGHT))
						{
							xx = x + hi->width
								- mFontGetTextWidth(font, (char *)buf, len)
								- MLK_LISTVIEWPAGE_ITEM_PADDING_X;
						}

						//描画

						if(is_sel)
							n = MGUICOL_TEXT_SELECT;
						else
							n = MGUICOL_TEXT;

						mFontDrawText_pixbuf(font, pixbuf,
							xx, y + texty,
							(char *)buf, len, mGuiCol_getRGB(n));
					}
				}

				//次へ

				x += hi->width;

				if(buf && len)
					buf += len + 1;
			}
		}
		
		mPixbufClip_clear(pixbuf);
	}
}
