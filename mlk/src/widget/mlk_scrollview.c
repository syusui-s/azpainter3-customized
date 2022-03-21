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

/*************************************************
 * mScrollView/mScrollViewPage
 * スクロールビュー
 *************************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_scrollview.h"
#include "mlk_scrollbar.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_util.h"


//=============================
// sub
//=============================


/* 全体のレイアウト範囲取得 */

static void _get_layout_page(mScrollView *p,mBox *box)
{
	if(p->sv.fstyle & MSCROLLVIEW_S_FRAME)
	{
		//枠あり
		
		box->x = box->y = 1;
		box->w = p->wg.w - 2;
		box->h = p->wg.h - 2;
	}
	else
	{
		box->x = box->y = 0;
		box->w = p->wg.w;
		box->h = p->wg.h;
	}
}

/* スクロールバーの操作ハンドラ
 * 
 * page にイベントを送る */

static void _scrollbar_action_handle(mWidget *wg,int pos,uint32_t flags)
{
	mScrollView *p = MLK_SCROLLVIEW(wg->param1);

	mWidgetEventAdd_notify(MLK_WIDGET(p), MLK_WIDGET(p->sv.page),
		(wg->param2)? MSCROLLVIEWPAGE_N_SCROLL_ACTION_VERT: MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ,
		pos, flags);
}

/* スクロールバー作成
 *
 * mWidget::param1 に mScrollView のポインタ。
 * mWidget::param2 に水平 (0)/垂直 (1)。 */

static mScrollBar *_create_scrollbar(mScrollView *p,int vert)
{
	mScrollBar *scr;

	scr = mScrollBarNew(MLK_WIDGET(p), 0,
		(vert)? MSCROLLBAR_S_VERT: MSCROLLBAR_S_HORZ);

	scr->wg.notify_to = MWIDGET_NOTIFYTO_PARENT;
	scr->wg.param1 = (intptr_t)p;
	scr->wg.param2 = vert;
	
	if(!(p->sv.fstyle & MSCROLLVIEW_S_SCROLL_NOTIFY_SELF))
		mScrollBarSetHandle_action(scr, _scrollbar_action_handle);
	
	return scr;
}

/* 現在のサイズから、スクロールバーの表示状態をセット
 * 
 * return: TRUE でスクロールバー表示の変更あり */

static mlkbool _scrollbar_show(mScrollView *p)
{
	mScrollViewPage *page;
	mScrollBar *scrh,*scrv;
	mBox box;
	mSize size_page;
	int flags,n;
	mlkbool horz,vert,ret;
	
	page = p->sv.page;
	if(!page) return FALSE;

	scrh = p->sv.scrh;
	scrv = p->sv.scrv;

	//レイアウト範囲
	
	_get_layout_page(p, &box);

	//ハンドラがある場合、スクロールのページ数取得

	size_page.w = box.w;
	size_page.h = box.h;

	if(page->svp.getscrollpage)
		(page->svp.getscrollpage)(MLK_WIDGET(page), box.w, box.h, &size_page);

	//スクロールバーの有無の各状態でスクロールバーを表示するかどうか
	//[flags]
	// 1 = 水平:垂直バーなしの状態で水平バーが必要なら ON
	// 2 = 水平:垂直バーありの状態
	// 4 = 垂直:水平バーなしの状態
	// 8 = 垂直:水平バーありの状態

	flags = 0;

	if(scrh && size_page.w > 0)
	{
		n = scrh->sb.max - scrh->sb.min;

		if(size_page.w < n) flags |= 1;
		if(size_page.w - MLK_SCROLLBAR_WIDTH < n) flags |= 2;
	}

	if(scrv && size_page.h > 0)
	{
		n = scrv->sb.max - scrv->sb.min;
		
		if(size_page.h < n) flags |= 4;
		if(size_page.h - MLK_SCROLLBAR_WIDTH < n) flags |= 8;
	}

	//固定

	if(p->sv.fstyle & MSCROLLVIEW_S_FIX_HORZ)
		flags |= 3;

	if(p->sv.fstyle & MSCROLLVIEW_S_FIX_VERT)
		flags |= 12;
	
	//--------- 判定
	
	horz = vert = 0;
	
	if(scrh && !scrv)
		//水平バーのみ
		horz = ((flags & 1) != 0);
	else if(!scrh && scrv)
		//垂直バーのみ
		vert = ((flags & 4) != 0);
	else
	{
		//----- 両方あり
		
		//水平

		if((flags & 3) == 3)
			//どちらの状態でも表示
			horz = TRUE;
		else if(!(flags & 3))
			//どちらの状態でも表示しない
			horz = FALSE;
		else
		{
			//表示/非表示の境目
		
			if((flags & 12) == 12)
				//垂直バーは常にあり
				horz = ((flags & 2) != 0);
			else
				//垂直バーなし
				horz = ((flags & 1) != 0);
		}

		//垂直

		if((flags & 12) == 12)
			vert = TRUE;
		else if(!(flags & 12))
			vert = FALSE;
		else
		{
			if((flags & 3) == 3)
				vert = ((flags & 8) != 0);
			else
				vert = ((flags & 4) != 0);
		}
	}

	//------- バーの表示切り替え
	
	ret = FALSE;
	
	if(scrh && mWidgetShow(MLK_WIDGET(scrh), horz))
		ret = TRUE;

	if(scrv && mWidgetShow(MLK_WIDGET(scrv), vert))
		ret = TRUE;

	return ret;
}


//=============================
// ハンドラ
//=============================


/**@ resize ハンドラ関数
 *
 * @d:スクロールバーの表示状態をセット */

void mScrollViewHandle_resize(mWidget *wg)
{
	_scrollbar_show(MLK_SCROLLVIEW(wg));
}

/**@ layout ハンドラ関数 */

void mScrollViewHandle_layout(mWidget *wg)
{
	mScrollView *p = MLK_SCROLLVIEW(wg);
	mBox box;
	int edge,scrb = 0;

	//レイアウト範囲
	
	_get_layout_page(p, &box);

	//スクロールバーが表示されているか
	
	if(mWidgetIsVisible_self(MLK_WIDGET(p->sv.scrh))) scrb |= 1;
	if(mWidgetIsVisible_self(MLK_WIDGET(p->sv.scrv))) scrb |= 2;
	
	//page
	
	if(p->sv.page)
	{
		mWidgetMoveResize(MLK_WIDGET(p->sv.page),
			box.x, box.y,
			box.w - ((scrb & 2)? MLK_SCROLLBAR_WIDTH: 0),
			box.h - ((scrb & 1)? MLK_SCROLLBAR_WIDTH: 0));
	}
	
	//スクロールバー
	
	edge = (scrb == 3)? MLK_SCROLLBAR_WIDTH: 0;
	
	if(p->sv.scrh && (scrb & 1))
	{
		mWidgetMoveResize(MLK_WIDGET(p->sv.scrh),
			box.x, box.y + box.h - MLK_SCROLLBAR_WIDTH,
			box.w - edge, MLK_SCROLLBAR_WIDTH); 
	}

	if(p->sv.scrv && (scrb & 2))
	{
		mWidgetMoveResize(MLK_WIDGET(p->sv.scrv),
			box.x + box.w - MLK_SCROLLBAR_WIDTH, box.y,
			MLK_SCROLLBAR_WIDTH, box.h - edge); 
	}
}

/**@ draw ハンドラ関数 */

void mScrollViewHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mScrollView *p = MLK_SCROLLVIEW(wg);
	int frame;
	
	frame = ((p->sv.fstyle & MSCROLLVIEW_S_FRAME) != 0);

	//枠

	if(frame)
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FRAME));
	
	//右下の余白
	
	if(mWidgetIsVisible_self(MLK_WIDGET(p->sv.scrh))
		&& mWidgetIsVisible_self(MLK_WIDGET(p->sv.scrv)))
	{
		mPixbufFillBox(pixbuf,
			wg->w - frame - MLK_SCROLLBAR_WIDTH,
			wg->h - frame - MLK_SCROLLBAR_WIDTH,
			MLK_SCROLLBAR_WIDTH, MLK_SCROLLBAR_WIDTH,
			MGUICOL_PIX(FACE));
	}
}


//=============================
// メイン
//=============================


/**@ mScrollView データ解放 */

void mScrollViewDestroy(mWidget *wg)
{

}

/**@ 作成
 *
 * @g:mScrollView */

mScrollView *mScrollViewNew(mWidget *parent,int size,uint32_t fstyle)
{
	mScrollView *p;
	
	if(size < sizeof(mScrollView))
		size = sizeof(mScrollView);
	
	p = (mScrollView *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = mScrollViewHandle_draw;
	p->wg.layout = mScrollViewHandle_layout;
	p->wg.resize = mScrollViewHandle_resize;
	
	p->wg.hintW = (fstyle & MSCROLLVIEW_S_VERT)? MLK_SCROLLBAR_WIDTH + 4: 4;
	p->wg.hintH = (fstyle & MSCROLLVIEW_S_HORZ)? MLK_SCROLLBAR_WIDTH + 4: 4;
	
	p->sv.fstyle = fstyle;
	
	//スクロールバー

	if(fstyle & MSCROLLVIEW_S_HORZ)
		p->sv.scrh = _create_scrollbar(p, FALSE);

	if(fstyle & MSCROLLVIEW_S_VERT)
		p->sv.scrv = _create_scrollbar(p, TRUE);
	
	return p;
}

/**@ 中身のウィジェットをセット
 *
 * @d:page は mScrollView の子として作成しておくこと。 */

void mScrollViewSetPage(mScrollView *p,mScrollViewPage *page)
{
	p->sv.page = page;
}

/**@ バーの固定表示を変更
 *
 * @d:水平/垂直ともに変更される。
 *
 * @p:fix [正] 常に表示 [0] 状態によって切り替え [負] 反転 */

void mScrollViewSetFixBar(mScrollView *p,int type)
{
	//水平
	
	if(mIsChangeFlagState(type, p->sv.fstyle & MSCROLLVIEW_S_FIX_HORZ))
		p->sv.fstyle ^= MSCROLLVIEW_S_FIX_HORZ;

	//垂直

	if(mIsChangeFlagState(type, p->sv.fstyle & MSCROLLVIEW_S_FIX_VERT))
		p->sv.fstyle ^= MSCROLLVIEW_S_FIX_VERT;
}

/**@ レイアウト構成を行う
 *
 * @d:中身の状態が変わった時に手動で行う。\
 * スクロールバーを表示するかどうかを再判定し、レイアウトし直す。\
 * (mScrollView のサイズ変更時は自動で行われる) */

void mScrollViewLayout(mScrollView *p)
{
	if(_scrollbar_show(p))
		mWidgetLayout(MLK_WIDGET(p));
}

/**@ 水平スクロールの情報セット */

void mScrollViewSetScrollStatus_horz(mScrollView *p,int min,int max,int page)
{
	if(p->sv.scrh)
		mScrollBarSetStatus(p->sv.scrh, min, max, page);
}

/**@ 垂直スクロールの情報セット */

void mScrollViewSetScrollStatus_vert(mScrollView *p,int min,int max,int page)
{
	if(p->sv.scrv)
		mScrollBarSetStatus(p->sv.scrv, min, max, page);
}

/**@ スクロールバーの表示状態を取得
 *
 * @r:[bit0] 水平スクロールバー [bit1] 垂直スクロールバー。\
 *  ビットが ON で表示されている。 */

int mScrollViewGetScrollShowStatus(mScrollView *p)
{
	int f = 0;

	if(p->sv.scrh && mWidgetIsVisible_self(MLK_WIDGET(p->sv.scrh)))
		f |= 1;

	if(p->sv.scrv && mWidgetIsVisible_self(MLK_WIDGET(p->sv.scrv)))
		f |= 2;

	return f;
}

/**@ スクロールバーの有効状態を変更
 *
 * @p:type [0] 無効 [正] 有効 [負] 反転 */

void mScrollViewEnableScrollBar(mScrollView *p,int type)
{
	if(p->sv.scrh)
		mWidgetEnable(MLK_WIDGET(p->sv.scrh), type);

	if(p->sv.scrv)
		mWidgetEnable(MLK_WIDGET(p->sv.scrv), type);
}


//*********************************
// mScrollViewPage
//*********************************


/**@ resize ハンドラ関数
 *
 * @d:ウィジェットサイズから、スクロールのページ値をセット。\
 *  派生側が独自で行うなら、関数を置き換えても良い。 */

void mScrollViewPageHandle_resize(mWidget *wg)
{
	mScrollViewPage *p = MLK_SCROLLVIEWPAGE(wg);
	mScrollBar *scrh,*scrv;
	mSize size;

	//ページ値取得

	size.w = wg->w;
	size.h = wg->h;

	if(p->svp.getscrollpage)
		(p->svp.getscrollpage)(wg, size.w, size.h, &size);

	//ページ値セット

	mScrollViewPage_getScrollBar(p, &scrh, &scrv);

	if(scrh) mScrollBarSetPage(scrh, size.w);
	if(scrv) mScrollBarSetPage(scrv, size.h);
}


/**@ 作成
 *
 * @g:mScrollViewPage */

mScrollViewPage *mScrollViewPageNew(mWidget *parent,int size)
{
	mWidget *p;

	if(size < sizeof(mScrollViewPage))
		size = sizeof(mScrollViewPage);
	
	p = mWidgetNew(parent, size);
	if(!p) return NULL;

	p->resize = mScrollViewPageHandle_resize;

	return (mScrollViewPage *)p;
}

/**@ ハンドラセット
 *
 * @d:スクロールバーのページ値が、中身のウィジェットのサイズと異なる場合は必要。\
 * (項目の高さ単位でスクロールする場合など)\
 * このハンドラは、mScrollViewPage のサイズが変わった時や、スクロールバーの表示の有無を判定する時に使われる。\
 * 関数内で、中身のサイズから、スクロールバーでのページ値に変換させる。\
 * ページ値に 0 以下を指定した場合は、スクロールバー非表示 (固定表示の場合はスクロール無効) となる。 */

void mScrollViewPage_setHandle_getScrollPage(mScrollViewPage *p,
	mFuncScrollViewPage_getScrollPage handle)
{
	p->svp.getscrollpage = handle;
}

/**@ 水平スクロールのページ値のセット
 *
 * @r:スクロール位置が変更されたか */

mlkbool mScrollViewPage_setScrollPage_horz(mScrollViewPage *p,int page)
{
	mScrollBar *scrh;

	mScrollViewPage_getScrollBar(p, &scrh, NULL);

	if(!scrh)
		return FALSE;
	else
		return (mScrollBarSetPage(scrh, page) == 2);
}

/**@ 垂直スクロールのページ値のセット
 *
 * @r:スクロール位置が変更されたか */

mlkbool mScrollViewPage_setScrollPage_vert(mScrollViewPage *p,int page)
{
	mScrollBar *scrv;

	mScrollViewPage_getScrollBar(p, NULL, &scrv);

	if(!scrv)
		return FALSE;
	else
		return (mScrollBarSetPage(scrv, page) == 2);
}

/**@ スクロール位置取得 */

void mScrollViewPage_getScrollPos(mScrollViewPage *p,mPoint *pt)
{
	mScrollView *sv = MLK_SCROLLVIEW(p->wg.parent);
	
	pt->x = (sv->sv.scrh)? (sv->sv.scrh)->sb.pos: 0;
	pt->y = (sv->sv.scrv)? (sv->sv.scrv)->sb.pos: 0;
}

/**@ 水平スクロール位置取得 */

int mScrollViewPage_getScrollPos_horz(mScrollViewPage *p)
{
	mScrollBar *sb = MLK_SCROLLVIEW(p->wg.parent)->sv.scrh;

	return (sb)? sb->sb.pos: 0;
}

/**@ 垂直スクロール位置取得 */

int mScrollViewPage_getScrollPos_vert(mScrollViewPage *p)
{
	mScrollBar *sb = MLK_SCROLLVIEW(p->wg.parent)->sv.scrv;

	return (sb)? sb->sb.pos: 0;
}

/**@ スクロールバーウィジェット取得
 *
 * @p:scrh,scrv  NULL で取得しない */

void mScrollViewPage_getScrollBar(mScrollViewPage *p,mScrollBar **scrh,mScrollBar **scrv)
{
	mScrollView *sv = MLK_SCROLLVIEW(p->wg.parent);

	if(scrh) *scrh = sv->sv.scrh;
	if(scrv) *scrv = sv->sv.scrv;
}

/**@ 水平スクロールバー取得 */

mScrollBar *mScrollViewPage_getScrollBar_horz(mScrollViewPage *p)
{
	return MLK_SCROLLVIEW(p->wg.parent)->sv.scrh;
}

/**@ 垂直スクロールバー取得 */

mScrollBar *mScrollViewPage_getScrollBar_vert(mScrollViewPage *p)
{
	return MLK_SCROLLVIEW(p->wg.parent)->sv.scrv;
}

