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
 * mListView [リストビュー]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_scrollview.h"
#include "mlk_listview.h"
#include "mlk_listviewpage.h"
#include "mlk_listheader.h"
#include "mlk_scrollbar.h"
#include "mlk_imagelist.h"
#include "mlk_event.h"
#include "mlk_list.h"
#include "mlk_font.h"
#include "mlk_str.h"

#include "mlk_pv_widget.h"
#include "mlk_columnitem_manager.h"


//----------------

#define _ITEM_TOP(p)  MLK_COLUMNITEM((p)->lv.manager.list.top)

//----------------

/*----------------

mListView (mScrollView) [focus]
  |- mScrollBar
  |- mListViewPage (mScrollViewPage)
    |- mListHeader
    (mListViewPage の領域内に mListHeader を配置し、残りの部分でリストを描画)


item_height:
	余白などを含む、アイテム一つの高さ。

item_height_min:
	アイテムの最小高さ (余白含まない)。

horz_width:
	mListHeader の幅 (水平スクロールの最大幅)

------------------*/


//============================
// sub
//============================


/* 一つのアイテムの高さ取得 (余白なども含む) */

static int _get_item_height(mListView *p)
{
	int h;

	//アイテム高さ

	h = mWidgetGetFontHeight(MLK_WIDGET(p));

	if(h < p->lv.item_height_min)
		h = p->lv.item_height_min;

	//チェックボックスがある場合
	
	if((p->lv.fstyle & MLISTVIEW_S_CHECKBOX)
		&& h < MLK_LISTVIEWPAGE_CHECKBOX_SIZE)
		h = MLK_LISTVIEWPAGE_CHECKBOX_SIZE;

	//アイコン

	if(p->lv.imglist && h < p->lv.imglist->h)
		h = p->lv.imglist->h;

	//余白

	h += MLK_LISTVIEWPAGE_ITEM_PADDING_Y * 2;
	
	return h;
}

/* ページを再描画 */

static void _redraw(mListView *p)
{
	mWidgetRedraw(MLK_WIDGET(p->sv.page));
}

/* アイテム挿入 */

static mColumnItem *_insert_item(mListView *p,
	mColumnItem *ins,int size,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	mColumnItem *pi;
	
	pi = mCIManagerInsertItem(&p->lv.manager, ins, size,
		text, icon, flags, param, mWidgetGetFont(MLK_WIDGET(p)),
		((p->lv.fstyle & MLISTVIEW_S_MULTI_COLUMN) != 0));

	if(pi)
		mWidgetSetConstruct(MLK_WIDGET(p));

	return pi;
}

/* アイテムのテキストセット
 *
 * pi: NULL で現在のフォーカスアイテム */

static void _set_item_text(mListView *p,mColumnItem *pi,const char *text,mlkbool copy)
{
	if(!pi)
	{
		pi = p->lv.manager.item_focus;
		if(!pi) return;
	}

	__mColumnItemSetText(pi, text, copy,
		((p->lv.fstyle & MLISTVIEW_S_MULTI_COLUMN) != 0),
		mWidgetGetFont(MLK_WIDGET(p)));

	if(p->lv.fstyle & MLISTVIEW_S_AUTO_WIDTH)
		mWidgetSetConstruct(MLK_WIDGET(p));
	else
		_redraw(p);
}

/* construct ハンドラ
 *
 * アイテム増減やテキストの変更時、また、
 * mListHeader の列幅サイズ変更時に実行される。
 * 
 * [!] ヘッダに拡張アイテムがあって、垂直スクロールバーの ON/OFF で幅が変わる場合、
 *   レイアウト前にスクロール幅がセットされるため、この関数実行時は正しいスクロール情報ではない。
 *   construct → レイアウト実行 → resize ハンドラが処理されて、正しい幅にセット → スクロール表示状態変更となる。 */

static void _construct_handle(mWidget *wg)
{
	mListView *p = MLK_LISTVIEW(wg);
	mListViewPage *page;
	mListHeader *ph;

	page = MLK_LISTVIEWPAGE(p->sv.page);
	ph = page->lvp.header;

	//[単一列] アイテムから幅計算

	if((p->lv.fstyle & MLISTVIEW_S_AUTO_WIDTH)
		&& !(p->lv.fstyle & MLISTVIEW_S_MULTI_COLUMN))
		mListViewSetAutoWidth(p, FALSE);

	//[複数列] ヘッダの全体幅

	if(ph)
		p->lv.horz_width = mListHeaderGetFullWidth(ph);

	//水平スクロール情報

	mScrollViewSetScrollStatus_horz(MLK_SCROLLVIEW(p),
		0, p->lv.horz_width, page->wg.w);

	//垂直スクロール情報セット

	mScrollViewSetScrollStatus_vert(MLK_SCROLLVIEW(p),
		0, p->lv.item_height * p->lv.manager.list.num,
		page->wg.h - page->lvp.header_height);

	//mListHeader のスクロール位置

	if(ph)
	{
		mListHeaderSetScrollPos(ph,
			mScrollViewPage_getScrollPos_horz(MLK_SCROLLVIEWPAGE(page)));
	}

	//レイアウト済みの時

	if(wg->fui & MWIDGET_UI_LAYOUTED)
	{
		//レイアウト実行
	
		mScrollViewLayout(MLK_SCROLLVIEW(p));

		//

		_redraw(p);
	}
}


//============================
// main
//============================


/**@ mListView データ解放 */

void mListViewDestroy(mWidget *wg)
{
	mListView *p = MLK_LISTVIEW(wg);

	mCIManagerFree(&p->lv.manager);

	//イメージリスト破棄

	if(p->lv.fstyle & MLISTVIEW_S_DESTROY_IMAGELIST)
		mImageListFree(p->lv.imglist);
}

/**@ 作成 */

mListView *mListViewNew(mWidget *parent,int size,
	uint32_t fstyle,uint32_t fstyle_scrollview)
{
	mListView *p;
	mListViewPage *page;

	//mListView
	
	if(size < sizeof(mListView))
		size = sizeof(mListView);
	
	p = (mListView *)mScrollViewNew(parent, size, fstyle_scrollview);
	if(!p) return NULL;
	
	p->wg.destroy = mListViewDestroy;
	p->wg.calc_hint = mListViewHandle_calcHint;
	p->wg.event = mListViewHandle_event;
	p->wg.construct = _construct_handle;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_KEY_REPEAT;
	p->wg.fevent |= MWIDGET_EVENT_KEY;

	p->lv.fstyle = fstyle;

	mCIManagerInit(&p->lv.manager, ((fstyle & MLISTVIEW_S_MULTI_SEL) != 0));

	//mListViewPage
	
	page = mListViewPageNew(MLK_WIDGET(p), 0, 0, fstyle, &p->lv.manager, MLK_WIDGET(p));

	mScrollViewSetPage(MLK_SCROLLVIEW(p), MLK_SCROLLVIEWPAGE(page));

	return p;
}

/**@ 作成 */

mListView *mListViewCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,
	uint32_t fstyle,uint32_t fstyle_scrollview)
{
	mListView *p;

	p = mListViewNew(parent, 0, fstyle, fstyle_scrollview);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ アイコンのイメージリストをセット */

void mListViewSetImageList(mListView *p,mImageList *img)
{
	p->lv.imglist = img;

	mListViewPage_setImageList(MLK_LISTVIEWPAGE(p->sv.page), img);
}

/**@ アイテムの最小高さをセット
 *
 * @d:デフォルトではフォントの高さが使われるため、フォント高さより大きい場合は、この値が使われる。\
 *  なお、項目のテキストは、垂直方向に中央寄せされるため、余白を付けたい時にも使う。
 *
 * @p:h アイテムの最小高さ。0 でフォントの高さを使う。 */

void mListViewSetItemHeight_min(mListView *p,int h)
{
	p->lv.item_height_min = h;
}

/**@ アイテムの破棄ハンドラをセット
 *
 * @d:ハンドラ関数内では、最後に mColumnItemHandle_destroyItem() を行うこと。 */

void mListViewSetItemDestroyHandle(mListView *p,void (*func)(mList *,mListItem *))
{
	(p->lv.manager).list.item_destroy = func;
}

/**@ ヘッダのウィジェットを取得 */

mListHeader *mListViewGetHeaderWidget(mListView *p)
{
	return MLK_LISTVIEWPAGE(p->sv.page)->lvp.header;
}

/**@ 一つのアイテムの高さを取得 (余白なども含む) */

int mListViewGetItemHeight(mListView *p)
{
	return _get_item_height(p);
}

/**@ 指定したアイテムの幅から、アイテム全体の幅を計算
 *
 * @d:水平スクロールサイズを計算したい時などに使う。
 *
 * @p:itemw テキスト部分などのアイテムの幅。\
 *  余白、アイコン、チェックボックスの幅は含まない。 */

int mListViewCalcItemWidth(mListView *p,int itemw)
{
	int w;

	w = itemw + MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2;

	//チェックボックス

	if(p->lv.fstyle & MLISTVIEW_S_CHECKBOX)
		w += MLK_LISTVIEWPAGE_CHECKBOX_SIZE + MLK_LISTVIEWPAGE_CHECKBOX_SPACE_RIGHT;

	//アイコン

	if(p->lv.imglist)
		w += p->lv.imglist->eachw + MLK_LISTVIEWPAGE_ICON_SPACE_RIGHT;

	return w;
}

/**@ 指定したアイテム幅から、ウィジェット全体の幅を計算
 *
 * @d:アイテム幅を元に、固定サイズにしたい場合などに使う。\
 *  垂直スクロールは常にあるものとみなす。
 * 
 * @p:itemw テキスト部分などのアイテム幅 */

int mListViewCalcWidgetWidth(mListView *p,int itemw)
{
	int w;

	w = mListViewCalcItemWidth(p, itemw);

	//枠

	if(p->sv.fstyle & MSCROLLVIEW_S_FRAME)
		w += 2;

	//垂直スクロール

	if(p->sv.fstyle & MSCROLLVIEW_S_VERT)
		w += MLK_SCROLLBAR_WIDTH;

	return w;
}


//=====================
// 取得
//=====================


/**@ アイテム数取得 */

int mListViewGetItemNum(mListView *p)
{
	return p->lv.manager.list.num;
}

/**@ 先頭アイテム取得 */

mColumnItem *mListViewGetTopItem(mListView *p)
{
	return (mColumnItem *)(p->lv.manager.list.top);
}

/**@ フォーカスアイテム取得 */

mColumnItem *mListViewGetFocusItem(mListView *p)
{
	return p->lv.manager.item_focus;
}

/**@ 位置からアイテム取得
 *
 * @p:index 負の値で、現在のフォーカスアイテム */

mColumnItem *mListViewGetItem_atIndex(mListView *p,int index)
{
	return mCIManagerGetItem_atIndex(&p->lv.manager, index);
}

/**@ パラメータ値からアイテム検索 */

mColumnItem *mListViewGetItem_fromParam(mListView *p,intptr_t param)
{
	return mCIManagerGetItem_fromParam(&p->lv.manager, param);
}

/**@ テキストからアイテム検索 */

mColumnItem *mListViewGetItem_fromText(mListView *p,const char *text)
{
	return mCIManagerGetItem_fromText(&p->lv.manager, text);
}


//=====================
// 削除
//=====================


/**@ アイテムをすべて削除 */

void mListViewDeleteAllItem(mListView *p)
{
	mCIManagerDeleteAllItem(&p->lv.manager);

	mWidgetSetConstruct(MLK_WIDGET(p));
}

/**@ アイテム削除 */

void mListViewDeleteItem(mListView *p,mColumnItem *item)
{
	if(item)
	{
		mCIManagerDeleteItem(&p->lv.manager, item);
		
		mWidgetSetConstruct(MLK_WIDGET(p));
	}
}

/**@ フォーカスアイテムを削除後、その上下のアイテムを選択する
 *
 * @r:選択されたアイテム */

mColumnItem *mListViewDeleteItem_focus(mListView *p)
{
	mColumnItem *pi,*sel;

	pi = p->lv.manager.item_focus;
	if(!pi) return NULL;
	
	sel = (mColumnItem *)((pi->i.next)? pi->i.next: pi->i.prev);

	mListViewDeleteItem(p, pi);

	if(sel) mListViewSetFocusItem(p, sel);

	return sel;
}


//======================
// アイテム追加
//======================


/**@ アイテム挿入 */

mColumnItem *mListViewInsertItem(mListView *p,mColumnItem *ins,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	return _insert_item(p, ins, 0, text, icon, flags, param);
}

/**@ アイテム追加 */

mColumnItem *mListViewAddItem(mListView *p,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	return _insert_item(p, NULL, 0, text, icon, flags, param);
}

/**@ アイテム追加: アイテムの確保サイズを指定 */

mColumnItem *mListViewAddItem_size(mListView *p,int size,
	const char *text,int icon,uint32_t flags,intptr_t param)
{
	return _insert_item(p, NULL, size, text, icon, flags, param);
}

/**@ アイテム追加: 静的テキストのみ */

mColumnItem *mListViewAddItem_text_static(mListView *p,const char *text)
{
	return _insert_item(p, NULL, 0, text, -1, 0, 0);
}

/**@ アイテム追加: 静的テキストとパラメータ値 */

mColumnItem *mListViewAddItem_text_static_param(mListView *p,const char *text,intptr_t param)
{
	return _insert_item(p, NULL, 0, text, -1, 0, param);
}

/**@ アイテム追加: テキスト(複製) */

mColumnItem *mListViewAddItem_text_copy(mListView *p,const char *text)
{
	return _insert_item(p, NULL, 0, text, -1, MCOLUMNITEM_F_COPYTEXT, 0);
}

/**@ アイテム追加: テキスト(複製)とパラメータ値 */

mColumnItem *mListViewAddItem_text_copy_param(mListView *p,const char *text,intptr_t param)
{
	return _insert_item(p, NULL, 0, text, -1, MCOLUMNITEM_F_COPYTEXT, param);
}


//======================
// フォーカス
//======================


/**@ フォーカスアイテム変更 */

void mListViewSetFocusItem(mListView *p,mColumnItem *item)
{
	if(mCIManagerSetFocusItem(&p->lv.manager, item))
		_redraw(p);
}

/**@ フォーカスアイテムを変更し、スクロール位置セット
 *
 * @d:アイテムが上端に来るようにする。 */

void mListViewSetFocusItem_scroll(mListView *p,mColumnItem *item)
{
	if(mCIManagerSetFocusItem(&p->lv.manager, item))
		_redraw(p);

	mListViewScrollToItem(p, item, 0);
}

/**@ フォーカスアイテム変更 (インデックス番号から)
 *
 * @r:フォーカスアイテム */

mColumnItem *mListViewSetFocusItem_index(mListView *p,int index)
{
	if(mCIManagerSetFocusItem_atIndex(&p->lv.manager, index))
		_redraw(p);

	return p->lv.manager.item_focus;
}

/**@ フォーカスアイテム変更 (パラメータ値から検索) */

mColumnItem *mListViewSetFocusItem_param(mListView *p,intptr_t param)
{
	mColumnItem *pi;

	pi = mCIManagerGetItem_fromParam(&p->lv.manager, param);
	if(pi)
	{
		if(mCIManagerSetFocusItem(&p->lv.manager, pi))
			_redraw(p);
	}

	return pi;
}

/**@ フォーカスアイテム変更 (テキストから検索) */

mColumnItem *mListViewSetFocusItem_text(mListView *p,const char *text)
{
	mColumnItem *pi;

	pi = mCIManagerGetItem_fromText(&p->lv.manager, text);
	if(pi)
	{
		if(mCIManagerSetFocusItem(&p->lv.manager, pi))
			_redraw(p);
	}

	return pi;
}


//==================
// アイテム
//==================


/**@ アイテムポインタからインデックス番号取得
 *
 * @p:item  NULL で現在のフォーカス
 * @r:アイテムポインタが NULL の場合、-1 */

int mListViewGetItemIndex(mListView *p,mColumnItem *item)
{
	return mCIManagerGetItemIndex(&p->lv.manager, item);
}

/**@ アイテムのパラメータ値取得
 *
 * @p:index 負の値で、現在のフォーカスアイテム
 * @r:指定位置にアイテムがない場合は、0 */

intptr_t mListViewGetItemParam(mListView *p,int index)
{
	mColumnItem *pi = mListViewGetItem_atIndex(p, index);

	return (pi)? pi->param: 0;
}

/**@ 複数列時、アイテムの指定列のテキスト取得 */

void mListViewGetItemColumnText(mListView *p,mColumnItem *pi,int index,mStr *str)
{
	char *pc;
	int len;

	len = mColumnItem_getColText(pi, index, &pc);

	mStrSetText_len(str, pc, len);
}

/**@ アイテムのテキストをセット (静的文字列)
 *
 * @d:複数列の場合は、静的/複製どちらでも同じ扱いになる。
 *
 * @p:pi  NULL でフォーカスアイテム */

void mListViewSetItemText_static(mListView *p,mColumnItem *pi,const char *text)
{
	_set_item_text(p, pi, text, FALSE);
}

/**@ アイテムのテキストをセット (複製)
 *
 * @p:pi  NULL でフォーカスアイテム */

void mListViewSetItemText_copy(mListView *p,mColumnItem *pi,const char *text)
{
	_set_item_text(p, pi, text, TRUE);
}

/**@ 複数列の指定位置のテキストをセット
 *
 * @p:text NULL で空文字列 */

void mListViewSetItemColumnText(mListView *p,mColumnItem *pi,int col,const char *text)
{
	mColumnItem_setColText(pi, col, text);

	_redraw(p);
}


//==================
// アイテム操作
//==================


/**@ アイテムを上下に移動
 *
 * @p:item  NULL でフォーカスアイテム
 * @r:移動したか */

mlkbool mListViewMoveItem_updown(mListView *p,mColumnItem *item,mlkbool down)
{
	if(mCIManagerMoveItem_updown(&p->lv.manager, item, down))
	{
		_redraw(p);
		return TRUE;
	}
	else
		return FALSE;
}

/**@ アイテムの位置を移動
 *
 * @p:insert 挿入位置。NULL で終端へ。
 * @r:移動したか */

mlkbool mListViewMoveItem(mListView *p,mColumnItem *item,mColumnItem *insert)
{
	if(mListMove(&p->lv.manager.list, MLISTITEM(item), MLISTITEM(insert)))
	{
		_redraw(p);
		return TRUE;
	}
	else
		return FALSE;
}

/**@ アイテムをソート */

void mListViewSortItem(mListView *p,
	int (*comp)(mListItem *,mListItem *,void *),void *param)
{
	mListSort(&p->lv.manager.list, comp, param);

	_redraw(p);
}

/**@ アイテムの最大幅から、水平スクロールの幅をセット (単一列時)
 *
 * @p:hintsize TRUE で、アイテム幅から計算したウィジェット幅を、hintRepW にセットする。\
 *   固定サイズにしたい時に使う。 */

void mListViewSetAutoWidth(mListView *p,mlkbool hintsize)
{
	mColumnItem *pi;
	int maxw = 0;

	//スクロール幅セット

	for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		if(pi->text_width > maxw)
			maxw = pi->text_width;
	}

	p->lv.horz_width = mListViewCalcItemWidth(p, maxw);

	//ウィジェット幅を推奨サイズにセット

	if(hintsize)
		p->wg.hintRepW = mListViewCalcWidgetWidth(p, maxw);

	//再構成

	if(!(p->lv.fstyle & MLISTVIEW_S_AUTO_WIDTH))
		mWidgetSetConstruct(MLK_WIDGET(p));
}

/**@ (複数列時) 列の幅を、列のテキストと全アイテムのテキスト幅を元にセット
 *
 * @p:index 列の位置 */

void mListViewSetColumnWidth_auto(mListView *p,int index)
{
	mListHeader *lh;
	mListHeaderItem *lhi;
	mColumnItem *pi;
	mFont *font;
	char *pc;
	int w,maxw,len;

	lh = mListViewGetHeaderWidget(p);
	if(!lh) return;

	font = mWidgetGetFont(MLK_WIDGET(p));

	//列テキストの幅

	lhi = mListHeaderGetItem_atIndex(lh, index);

	maxw = mFontGetTextWidth(font, lhi->text, -1);

	//各アイテムの最大幅

	for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		len = mColumnItem_getColText(pi, index, &pc);

		if(len)
		{
			w = mFontGetTextWidth(font, pc, len);
			if(w > maxw) maxw = w;
		}
	}

	//アイテムの幅をセット

	mListHeaderSetItemWidth(lh, index, maxw, TRUE);
}

/**@ 指定アイテムの位置を基準として、垂直スクロール位置変更
 *
 * @d:レイアウトが行われていない初期状態では、現在のアイテム数からスクロール情報がセットされる。\
 * ※未レイアウト状態では、ページ値が確定していないため、中央位置を正しくセットできない。
 * 
 * @p:align [0] アイテムが上端に来るように\
 *  [1] アイテムが中央に来るように */

void mListViewScrollToItem(mListView *p,mColumnItem *pi,int align)
{
	int y;

	if(!p->sv.scrv || !pi) return;

	//アイテム高さ

	if(p->lv.item_height == 0)
		p->lv.item_height = _get_item_height(p);

	//Y 位置

	y = mCIManagerGetItemIndex(&p->lv.manager, pi) * p->lv.item_height;

	if(align == 1)
	{
		mListViewPage *page = MLK_LISTVIEWPAGE(p->sv.page);
		
		y -= (page->wg.h - page->lvp.header_height - p->lv.item_height) / 2;
	}

	//未レイアウト時はステータスセット

	if(!(p->wg.fui & MWIDGET_UI_LAYOUTED))
	{
		mScrollViewSetScrollStatus_vert(MLK_SCROLLVIEW(p),
			0, p->lv.item_height * p->lv.manager.list.num, 1);
	}

	//位置セット

	if(mScrollBarSetPos(p->sv.scrv, y))
		_redraw(p);
}

/**@ フォーカスアイテムが表示されるように、垂直スクロール
 *
 * @d:スクロールの必要がない場合は、何もしない。\
 *  フォーカスが上下に隠れている場合、フォーカスアイテムが上端に来るようにする。\
 *  ※未レイアウトの状態では使わないこと。 */
 
void mListViewScrollToFocus(mListView *p)
{
	//アイテム増減などの処理
	mWidgetRunConstruct(MLK_WIDGET(p));
	
	mListViewPage_scrollToFocus(MLK_LISTVIEWPAGE(p->sv.page), 0, 0);
}

/**@ フォーカスアイテムが表示されるように、垂直スクロール (余白付き)
 *
 * @d:フォーカスアイテムが上に隠れている場合は、上に指定アイテム分の余白を付けた位置。\
 * 下に隠れている場合は、下に余白が付く。
 *
 * @p:margin_itemnum 上下に付ける余白のアイテム数 */

void mListViewScrollToFocus_margin(mListView *p,int margin_itemnum)
{
	mWidgetRunConstruct(MLK_WIDGET(p));
	
	mListViewPage_scrollToFocus(MLK_LISTVIEWPAGE(p->sv.page), 0, margin_itemnum);
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mListViewHandle_calcHint(mWidget *wg)
{
	mListView *p = MLK_LISTVIEW(wg);

	//アイテム高さ (mListViewPage にもセット)

	p->lv.item_height = _get_item_height(p);
	
	MLK_LISTVIEWPAGE(p->sv.page)->lvp.item_height = p->lv.item_height;
}

/**@ イベントハンドラ関数 */

int mListViewHandle_event(mWidget *wg,mEvent *ev)
{
	mListView *p = MLK_LISTVIEW(wg);

	switch(ev->type)
	{
		//キー押し: mListViewPage へ送る
		case MEVENT_KEYDOWN:
			return ((p->sv.page)->wg.event)(MLK_WIDGET(p->sv.page), ev);

		//フォーカス
		case MEVENT_FOCUS:
			_redraw(p);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

