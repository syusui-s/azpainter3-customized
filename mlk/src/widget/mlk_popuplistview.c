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
 * mPopupListView
 * (ポップアップでリストビュー表示)
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_scrollview.h"
#include "mlk_listviewpage.h"
#include "mlk_scrollbar.h"  //MLK_SCROLLBAR_WIDTH
#include "mlk_event.h"
#include "mlk_key.h"

#include "mlk_columnitem_manager.h"


//-----------------

#define MLK_POPUPLISTVIEW(p)  ((mPopupListView *)(p))

typedef struct
{
	MLK_POPUP_DEF

	mCIManager *manager;
	mWidget *page;	//mListViewPage
	mColumnItem *item_sel_start;	//ポップアップ開始時の選択アイテム
}mPopupListView;

//-----------------



//======================
// sub
//======================


/** ポップアップ終了ハンドラ */

static void _popup_quit_handle(mWidget *p,mlkbool is_cancel)
{
	//キャンセル時は、ポップアップ開始時の選択に戻す

	if(is_cancel)
	{
		mCIManagerSetFocusItem(MLK_POPUPLISTVIEW(p)->manager,
			MLK_POPUPLISTVIEW(p)->item_sel_start);
	}
}

/** キー押し */

static void _key_event(mPopupListView *p,mEvent *ev)
{
	switch(ev->key.key)
	{
		//Enter/Space : 決定
		case MKEY_ENTER:
		case MKEY_KP_ENTER:
		case MKEY_SPACE:
		case MKEY_KP_SPACE:
			mPopupQuit(MLK_POPUP(p), FALSE);
			break;

		//ほかは mListViewPage へ送る
		default:
			(p->page->event)(p->page, ev);
			break;
	}
}

/** イベント */

static int _popup_event_handle(mWidget *wg,mEvent *ev)
{
	mPopupListView *p = MLK_POPUPLISTVIEW(wg);

	switch(ev->type)
	{
		//キー押し
		case MEVENT_KEYDOWN:
			_key_event(p, ev);
			break;

		//通知
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == p->page
				&& ev->notify.notify_type == MLISTVIEWPAGE_N_POPUP_QUIT)
			{
				//mListViewPage でボタンが押された時

				mPopupQuit(MLK_POPUP(p), FALSE);
			}
			break;

		//表示時
		case MEVENT_MAP:
			//選択アイテムが表示されるようにスクロール
			mListViewPage_scrollToFocus(MLK_LISTVIEWPAGE(p->page), 0, 0);
			break;
	}

	return mPopupEventDefault(wg, ev);
}

/** ポップアップ作成
 *
 * height: 枠は含まない高さ */

static mPopupListView *_create_popup(mWidget *parent,
	mCIManager *manager,int itemh,int height)
{
	mPopupListView *p;
	mScrollView *view;
	mListViewPage *page;

	//mPopupListView 作成
	
	p = (mPopupListView *)mPopupNew(sizeof(mPopupListView), 0);
	if(!p) return NULL;

	p->wg.fstate |= MWIDGET_STATE_ENABLE_KEY_REPEAT;
	p->wg.fevent |= MWIDGET_EVENT_KEY;
	p->wg.event = _popup_event_handle;
	p->wg.font = mWidgetGetFont(parent);  //フォントは親と同じにする

	p->pop.quit = _popup_quit_handle;

	p->manager = manager;
	p->item_sel_start = manager->item_focus;

	//mScrollView

	view = mScrollViewNew(MLK_WIDGET(p), 0, MSCROLLVIEW_S_FRAME | MSCROLLVIEW_S_VERT);

	view->wg.flayout = MLF_EXPAND_WH;

	//mListViewPage

	page = mListViewPageNew(MLK_WIDGET(view), 0,
		MLISTVIEWPAGE_S_POPUP, 0, manager, parent);

	page->lvp.item_height = itemh;

	//

	mScrollViewSetPage(view, MLK_SCROLLVIEWPAGE(page));

	p->page = MLK_WIDGET(page);

	//スクロール

	mScrollViewSetScrollStatus_vert(view, 0, height, 1);

	return p;
}


//==========================
// main
//==========================


/**@ ポップアップでリストビューを表示し、アイテム選択
 *
 * @g:mPopupListView
 *
 * @d:mComboBox で使われる。\
 *  現在のフォーカスアイテムが選択された状態で表示され、
 *  ポップアップ中はポインタ移動で選択が変わる。\
 *  ボタンが押されるか、ENTER/SPACE キーが押されると終了。\
 *  戻り値が TRUE で、フォーカスアイテムが変更されている。\
 *  (キャンセル時は元のフォーカスアイテムに戻る)\
 *  ※単一列であること。\
 *  ※アイテムが一つもない場合は、実行されない。
 *
 * @p:parent アイテムの draw ハンドラ時のウィジェットとしても指定される
 * @p:x,y ポップアップ位置
 * @p:box ポップアップの基準矩形
 * @p:flags ポップアップフラグ
 * @p:itemh 一つのアイテムの高さ (余白など含む)
 * @p:width ウィンドウの最小幅。\
 *  アイテムの幅の方が大きい場合は、アイテム幅からセットされる。\
 *  0 以下で、アイテムの幅から自動計算。
 * @p:maxh ウィンドウの最大高さ (px)。0 以下で、アイテム数から計算。
 * @r:アイテムの選択が変更されたか */

mlkbool mPopupListView_run(mWidget *parent,
	int x,int y,mBox *box,uint32_t flags,
	mCIManager *manager,int itemh,int width,int maxh)
{
	mPopupListView *p;
	int w,h;
	mlkbool ret;

	if(manager->list.num == 0) return FALSE;

	//幅計算

	w = mCIManagerGetMaxWidth(manager)
		+ MLK_SCROLLBAR_WIDTH + MLK_LISTVIEWPAGE_ITEM_PADDING_X * 2 + 2;

	if(width <= 0)
		width = w;
	else if(w > width)
		width = w;

	//高さ

	h = itemh * manager->list.num + 2;

	if(maxh > 0 && h > maxh)
		h = maxh;

	//作成
	//[!] height は、枠を含まない高さ

	p = _create_popup(parent, manager, itemh, h - 2);
	if(!p) return FALSE;

	//

	mPopupRun(MLK_POPUP(p), parent, x, y, width, h, box, flags);

	//終了

	ret = (p->item_sel_start != manager->item_focus);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

