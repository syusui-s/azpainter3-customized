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
 * mWidget (ウィジェット)
 *****************************************/
 
#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_rectbox.h"
#include "mlk_font.h"
#include "mlk_cursor.h"
#include "mlk_util.h"
#include "mlk_string.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"
#include "mlk_pv_event.h"
#include "mlk_pv_pixbuf.h"


//==========================
// sub
//==========================


/** デフォルトイベントハンドラ */

static int _handle_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_CLOSE)
	{
		mGuiQuit();
		return 1;
	}
	
	return 0;
}

/** ウィジェット削除処理 */

static void _delete_widget(mWidget *p)
{
	mWindow *win;

	//自身のフォーカスを取り除く
	//- FOCUS [out] イベントは追加される
	
	__mWidgetRemoveSelfFocus(p);

	//タイマー削除
	
	mGuiTimerDelete_widget(p);
	
	//ウィンドウ破棄

	if(p->ftype & MWIDGET_TYPE_WINDOW)
	{
		win = MLK_WINDOW(p);
	
		mPixbufFree(win->win.pixbuf);
		
		(MLKAPP->bkend.window_destroy)(win);

		//バックエンドデータ解放

		mFree(win->win.bkend);
		win->win.bkend = NULL;

		//フォーカスウィンドウの場合

		if(win == MLKAPP->window_focus)
			MLKAPP->window_focus = NULL;
	}

	//残っているイベント削除
	
	mEventListDelete_widget(p);

	//このウィジェットが他のところで使用されている場合、解除

	if(p == MLKAPP->widget_grabpt)
		MLKAPP->widget_grabpt = NULL;
	
	if(p == MLKAPP->widget_enter)
		MLKAPP->widget_enter = NULL;

	if(p == p->toplevel->win.focus_ready)
		p->toplevel->win.focus_ready = NULL;

	//構成変更リストから削除

	if(p->fui & MWIDGET_UI_CONSTRUCT)
		__mAppDelConstructWidget(p);

	//カーソルキャッシュのカーソルを解放

	mCursorCache_release(p->cursor_cache);
	
	//リンクをはずす
	
	mWidgetTreeRemove(p);
	
	//解放
	
	mFree(p);
}

/** 通知先ウィジェット取得
 *
 * enable_replace: 置き換えの値を適用する */

static mWidget *_get_notify_widget(mWidget *p,mlkbool enable_replace)
{
	while(1)
	{
		//置き換え

		if(enable_replace)
		{
			if(p->notify_to_rep == MWIDGET_NOTIFYTOREP_SELF)
				return p;
		}

		//通常値

		switch(p->notify_to)
		{
			//親の値を使う
			case MWIDGET_NOTIFYTO_PARENT_NOTIFY:
				//ルートの子 (ウィンドウ) を最上位とする
				if(!p->parent->parent) return p;

				//親で続ける
				p = p->parent;
				continue;

			//自身
			case MWIDGET_NOTIFYTO_SELF:
				return p;
			//指定ウィジェット
			case MWIDGET_NOTIFYTO_WIDGET:
				return p->notify_widget;
			//１つ上の親
			case MWIDGET_NOTIFYTO_PARENT:
				return (!p->parent->parent)? p: p->parent;
			//２つ上の親
			case MWIDGET_NOTIFYTO_PARENT2:
				if(!p->parent->parent)
					return p;
				else
				{
					p = p->parent;
					return (!p->parent->parent)? p: p->parent;
				}
			//トップレベル
			case MWIDGET_NOTIFYTO_TOPLEVEL:
				return MLK_WIDGET(p->toplevel);
		}
	}

	return NULL;
}


//==========================
// main
//==========================


/**@ ウィジェット削除 */

/* [!] destroy ハンドラ内で、自身以外のウィジェットを削除する場合があるので、
 *     再帰的に直接ウィジェットの削除を行わないようにする。
 * 
 * まず、削除するすべてのウィジェットの destroy ハンドラを実行し、フラグを ON にする。
 * destroy ハンドラ内で mWidgetDestroy() が呼ばれた場合、フラグだけ ON にして戻る。
 * フラグが処理できたら、トップレベルの mWidgetDestroy() 上で、
 * フラグが ON のウィジェットを順番に削除する。 */

void mWidgetDestroy(mWidget *p)
{
	mWidget *pw,*next;
	mlkbool is_run;

	if(!p) return;

	//現時点で mWidgetDestroy() 実行中か
	// :FALSE でトップレベル

	is_run = ((MLKAPP->flags & MAPPBASE_FLAGS_IN_WIDGET_DESTROY) != 0);

	//mWidgetDestroy() 実行中フラグ ON

	if(!is_run)
		MLKAPP->flags |= MAPPBASE_FLAGS_IN_WIDGET_DESTROY;

	//親方向の辿るフラグセット

	__mWidgetSetUIFlags_upper(p, MWIDGET_UI_FOLLOW_DESTROY);

	//下位すべての destroy ハンドラ実行 & フラグを ON
	// :ハンドラ内で mWidgetDestroy() が呼ばれた場合は、フラグのみ ON にする。

	for(pw = p; pw; pw = __mWidgetGetTreeNext_root(pw, p))
	{
		if(!(pw->fui & MWIDGET_UI_DESTROY))
		{
			if(pw->destroy)
				(pw->destroy)(pw);

			pw->fui |= MWIDGET_UI_FOLLOW_DESTROY | MWIDGET_UI_DESTROY;
		}
	}

	//トップレベルの mWidgetDestroy() でなければ戻る

	if(is_run) return;

	//------ 削除処理 (フラグが ON のウィジェットを対象に、下位から順に処理)

	MLKAPP->flags &= ~MAPPBASE_FLAGS_IN_WIDGET_DESTROY;

	//p がウィジェットツリーから取り外されている時は、p がルート。
	//通常は NULL で、ルートウィジェットから。

	pw = __mWidgetGetTreeBottom_uiflag((p->parent)? NULL: p, MWIDGET_UI_FOLLOW_DESTROY);

	for(; pw; pw = next)
	{
		next = __mWidgetGetTreePrev_uiflag(pw,
			MWIDGET_UI_FOLLOW_DESTROY, MWIDGET_UI_DESTROY);

		_delete_widget(pw);
	}
}

/**@ ウィジェット新規作成
 * 
 * @p:parent 親ウィジェット。NULL でルート (=トップレベルウィンドウ)。
 * @p:size  mWidget 構造体の確保サイズ。最小で mWidget のサイズになる。 */

mWidget *mWidgetNew(mWidget *parent,int size)
{
	mWidget *p;
	
	if(size < sizeof(mWidget))
		size = sizeof(mWidget);

	//親 (parent == 1 でルートウィジェットを作成)

	if(parent == (mWidget *)1)
		parent = NULL;
	else if(!parent)
		parent = MLKAPP->widget_root;

	//作成

	p = (mWidget *)mMalloc0(size);
	if(!p) return NULL;

	p->parent = parent;
	p->fstate = MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED;
	p->event  = _handle_event;
	
	p->w = p->h = 1;
	p->hintW = p->hintH = 1;

	//親がある場合
	
	if(parent)
	{
		//最上位のウィンドウをセット

		if(parent == MLKAPP->widget_root)
			p->toplevel = MLK_WINDOW(p);
		else
			p->toplevel = parent->toplevel;

		//推奨サイズ計算フラグ ON

		__mWidgetSetUIFlags_upper(p, MWIDGET_UI_FOLLOW_CALC | MWIDGET_UI_CALC);
	}
	
	//リンクを追加
	
	mWidgetTreeAdd(p, parent);
	
	return p;
}

/**@ draw ハンドラ用関数
 *
 * @d:全体を背景描画する */

void mWidgetDrawHandle_bkgnd(mWidget *p,mPixbuf *pixbuf)
{
	__mWidgetDrawBkgnd(p, NULL, FALSE);
}

/**@ draw ハンドラ用関数
 *
 * @d:背景と FRAME 色の枠を描画 */

void mWidgetDrawHandle_bkgndAndFrame(mWidget *p,mPixbuf *pixbuf)
{
	__mWidgetDrawBkgnd(p, NULL, FALSE);

	mPixbufBox(pixbuf, 0, 0, p->w, p->h, MGUICOL_PIX(FRAME));
}

/**@ draw ハンドラ用関数
 *
 * @d:FRAME 色の枠を描画する */

void mWidgetDrawHandle_frame(mWidget *p,mPixbuf *pixbuf)
{
	mPixbufBox(pixbuf, 0, 0, p->w, p->h, MGUICOL_PIX(FRAME));
}

/**@ draw ハンドラ用関数
 *
 * @d:黒い 1px 枠を描画する */

void mWidgetDrawHandle_frameBlack(mWidget *p,mPixbuf *pixbuf)
{
	mPixbufBox(pixbuf, 0, 0, p->w, p->h, 0);
}

/**@ draw_bkgnd ハンドラ用関数
 *
 * @d:FACE 色で全体を塗りつぶす */

void mWidgetDrawBkgndHandle_fillFace(mWidget *p,mPixbuf *pixbuf,mBox *box)
{
	mPixbufFillBox(pixbuf, box->x, box->y, box->w, box->h, MGUICOL_PIX(FACE));
}


//============================
// ウィジェットツリー
//============================


/**@ p が root 以下のウィジェットかどうか
 *
 * @d:root の子かどうか。p == root の場合も含む。 */

mlkbool mWidgetTreeIsUnder(mWidget *p,mWidget *root)
{
	for(; p; p = p->parent)
	{
		if(p == root) return TRUE;
	}

	return FALSE;
}

/**@ ウィジェットツリー: p を parent の最後に追加 */

void mWidgetTreeAdd(mWidget *p,mWidget *parent)
{
	if(p && parent)
	{
		p->parent = parent;
		p->prev = parent->last;
		p->next = NULL;
		
		parent->last = p;
		
		if(p->prev)
			(p->prev)->next = p;
		else
			parent->first = p;
	}
}

/**@ ウィジェットツリー: p を parent の指定位置に挿入
 *
 * @p:parent 親
 * @p:ins    NULL で最後に追加 */

void mWidgetTreeInsert(mWidget *p,mWidget *parent,mWidget *ins)
{
	if(!ins)
		mWidgetTreeAdd(p, parent);
	else
	{
		p->parent = parent;

		if(ins->prev)
			ins->prev->next = p;
		else
			parent->first = p;

		p->prev = ins->prev;
		p->next = ins;
		ins->prev = p;
	}
}

/**@ ウィジェットツリー: ツリーから取り外す */

void mWidgetTreeRemove(mWidget *p)
{
	if(p)
	{
		if(p->prev)
			(p->prev)->next = p->next;
		else if(p->parent)
			(p->parent)->first = p->next;
		
		if(p->next)
			(p->next)->prev = p->prev;
		else if(p->parent)
			(p->parent)->last = p->prev;
		
		p->parent = p->prev = p->next = NULL;
	}
}

/**@ ウィジェットツリー:子ウィジェットをすべて取り外す
 *
 * @d:子ウィジェット以下はそのまま。 */

void mWidgetTreeRemove_child(mWidget *p)
{
	while(p->first)
		mWidgetTreeRemove(p->first);
}

/**@ ウィジェットツリー: 指定位置の前に移動
 *
 * @p:parent 移動先の親
 * @p:ins    挿入位置。NULL で親の最後に移動。 */

void mWidgetTreeMove(mWidget *p,mWidget *parent,mWidget *ins)
{
	if(p && p != ins)
	{
		mWidgetTreeRemove(p);
		mWidgetTreeInsert(p, parent, ins);
	}
}

/**@ ウィジェットツリー: 指定親の先頭に移動
 *
 * @p:parent 移動先の親。NULL で自分の親。 */

void mWidgetTreeMove_toFirst(mWidget *p,mWidget *parent)
{
	if(!parent) parent = p->parent;

	mWidgetTreeMove(p, parent, parent->first);
}


//============================
// 情報取得
//============================


/**@ フォント取得
 * 
 * @d:font が NULL の場合は、親のフォントを使う (再帰的)。
 * 親のフォントが全て NULL なら、デフォルトフォント。 */

mFont *mWidgetGetFont(mWidget *p)
{
	for(; p; p = p->parent)
	{
		if(p->font) return p->font;
	}

	return MLKAPP->font_default;
}

/**@ フォントの高さ取得 */

int mWidgetGetFontHeight(mWidget *p)
{
	mFont *font = mWidgetGetFont(p);
	
	return mFontGetHeight(font);
}

/**@ フォントの行高さ取得 */

int mWidgetGetFontLineHeight(mWidget *p)
{
	mFont *font = mWidgetGetFont(p);
	
	return mFontGetLineHeight(font);
}

/**@ ウィジェットの位置とサイズを取得 (親からの位置)
 *
 * @d:位置は、親からの位置。 */

void mWidgetGetBox_parent(mWidget *p,mBox *box)
{
	box->x = p->x;
	box->y = p->y;
	box->w = p->w;
	box->h = p->h;
}

/**@ ウィジェットの位置とサイズを取得 (相対位置)
 *
 * @d:位置は、このウィジェットからの相対位置。 */

void mWidgetGetBox_rel(mWidget *p,mBox *box)
{
	box->x = 0;
	box->y = 0;
	box->w = p->w;
	box->h = p->h;
}

/**@ ウィジェットのサイズを取得
 *
 * @d:非表示状態の時は、0 になる。 */

void mWidgetGetSize_visible(mWidget *p,mSize *size)
{
	if(mWidgetIsVisible(p))
		size->w = p->w, size->h = p->h;
	else
		size->w = size->h = 0;
}

/**@ ウィジェットの幅を取得
 *
 * @d:非表示状態の時は、0 になる。 */

int mWidgetGetWidth_visible(mWidget *p)
{
	return (mWidgetIsVisible(p))? p->w: 0;
}

/**@ ウィジェットの高さを取得
 *
 * @d:非表示状態の時は、0 になる。 */

int mWidgetGetHeight_visible(mWidget *p)
{
	return (mWidgetIsVisible(p))? p->h: 0;
}

/**@ 画面上に存在し、かつ指定位置の一番下層にあるウィジェットを取得
 *
 * @d:現在画面上に表示されているウィジェットが対象。\
 * 非表示の状態や、ウィンドウの表示範囲外上に位置するものなどは除く。
 *
 * @p:root 検索のルート。この下位のウィジェットのみ対象とする。
 * @p:x,y  ポイント位置。root からの相対位置。
 * @r:該当するウィジェットがない場合は root が返る */

mWidget *mWidgetGetWidget_atPoint(mWidget *root,int x,int y)
{
	mWidget *p;
	mRect rc,rcroot;
	
	x += root->absX;
	y += root->absY;

	//root が画面上に表示されていないか、x,y が範囲外ならそのまま返す

	if(!mWidgetIsVisible(root)
		|| !__mWidgetGetWindowRect(root, &rcroot, NULL, FALSE)
		|| x < rcroot.x1 || y < rcroot.y1 || x > rcroot.x2 || y > rcroot.y2)
		return root;

	//root 内に x,y が含まれるため、子を検索

	while(1)
	{
		//子の中で、表示状態で、かつ x,y が範囲内にあるものを取得

		for(p = root->first; p; p = p->next)
		{
			if(!(p->fstate & MWIDGET_STATE_VISIBLE))
				continue;

			rc = rcroot;
			if(!mRectClipBox_d(&rc, p->absX, p->absY, p->w, p->h))
				continue;

			if(rc.x1 <= x && x <= rc.x2 && rc.y1 <= y && y <= rc.y2)
				break;
		}

		//見つかった場合、さらにその子を検索。
		//見つからなければ終了 (最後の親を返す)

		if(!p)
			break;
		else
		{
			root = p;
			rcroot = rc;
		}
	}
	
	return root;
}

/**@ ウィジェットの位置を、親ウィジェットからの相対値に変換
 *
 * @p:dst 変換先の親ウィジェット。
 * @p:pt src からの相対位置を入れておく。結果が入る。 */

void mWidgetMapPoint(mWidget *src,mWidget *dst,mPoint *pt)
{
	mPoint pt1;

	pt1 = *pt;

	for(; src && src->parent && src != dst; src = src->parent)
	{
		pt1.x += src->x;
		pt1.y += src->y;
	}

	*pt = pt1;
}

/**@ 指定位置がウィジェットの範囲内にあるか
 *
 * @p:x,y ウィジェットからの相対位置 */

mlkbool mWidgetIsPointIn(mWidget *p,int x,int y)
{
	return (x >= 0 && x < p->w && y >= 0 && y < p->h);
}

/**@ 下位ウィジェットの中から id を検索
 *
 * @p:root このウィジェットの下位のみ対象 */

mWidget *mWidgetFindFromID(mWidget *root,int id)
{
	mWidget *p;
	
	for(p = root; p && p->id != id; p = __mWidgetGetTreeNext_root(p, root));
	
	return p;
}


//============================
// 状態
//============================


/**@ 表示されているか
 *
 * @g:状態
 *
 * @d:親の表示状態も含む。\
 * ウィンドウの範囲外にある場合など、画面上では見えないが表示状態である場合も含める。 */

mlkbool mWidgetIsVisible(mWidget *p)
{
	for(; p->parent; p = p->parent)
	{
		if(!(p->fstate & MWIDGET_STATE_VISIBLE))
			return FALSE;
	}

	return TRUE;
}

/**@ 表示されているか (自身のみ)
 *
 * @d:自身の表示状態のみ判定する。*/

mlkbool mWidgetIsVisible_self(mWidget *p)
{
	if(p)
		return ((p->fstate & MWIDGET_STATE_VISIBLE) != 0);
	else
		return FALSE;
}

/**@ ウィジェットが使用可能な状態か
 *
 * @d:表示されていて、かつ有効状態であるかどうか。\
 * 親の状態も含む。 */

mlkbool mWidgetIsEnable(mWidget *p)
{
	for(; p->parent; p = p->parent)
	{
		if((p->fstate & (MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED))
				!= (MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED))
			return FALSE;
	}

	return TRUE;
}

/**@ 有効/無効状態の変更
 *
 * @p:type 0:無効、正:有効、負:反転
 * @r:状態が変化したか */

mlkbool mWidgetEnable(mWidget *p,int type)
{
	if(!p) return FALSE;

	if(!mGetChangeFlagState(type, p->fstate & MWIDGET_STATE_ENABLED, &type))
		//変化なし
		return FALSE;
	else
	{
		p->fstate ^= MWIDGET_STATE_ENABLED;

		//無効状態になった場合

		__mWidgetLeave_forDisable(p);
		__mWidgetRemoveFocus_forDisabled(p);

		mWidgetRedraw(p);

		return TRUE;
	}
}

/**@ 表示/非表示状態の変更
 *
 * @d:- 非表示状態の時は、描画＆更新は行われない。\
 * - ウィンドウが非表示→表示に変更された時は、ウィンドウ全体を再描画する。\
 * - p の表示状態の変化によって、上位のウィジェットのサイズが変化する場合、
 * その一番上位のウィジェットを再描画すること。
 *
 * @p:p  NULL で何もしない
 * @p:type 0:非表示、正:表示、負:反転
 * @r:状態が変化したか */

mlkbool mWidgetShow(mWidget *p,int type)
{
	if(!p) return FALSE;

	if(!mGetChangeFlagState(type, p->fstate & MWIDGET_STATE_VISIBLE, &type))
		return FALSE;
	else
	{
		p->fstate ^= MWIDGET_STATE_VISIBLE;

		if(p->ftype & MWIDGET_TYPE_WINDOW)
		{
			//ウィンドウ

			if(type)
				mWidgetRedraw(p);

			(MLKAPP->bkend.window_show)(MLK_WINDOW(p), type);
		}
		else
		{
			//ウィジェット
			//- 上位ウィジェットの推奨サイズを再計算する必要がある。
			//- 上位ウィジェットのサイズが変わった場合、その一番上位のウィジェットを再描画する必要がある。
			//  (とりあえず一つ上の親は常に再描画する)

			mWidgetSetRecalcHint(p->parent);
			
			mWidgetRedraw(p->parent);
		}

		//非表示になった時、ENTER 状態の場合は LEAVE を送る
		// :mIconBar が非表示になった時、ツールチップが残ったりする場合がある

		__mWidgetLeave_forDisable(p);

		//非表示になった時、フォーカス除去

		__mWidgetRemoveFocus_forDisabled(p);

		return TRUE;
	}
}

/**@ 位置移動
 *
 * @d:ウィジェットの相対位置を移動する。\
 * ウィンドウの場合は何もしない。
 *
 * @p:x,y 親からの相対位置 */

void mWidgetMove(mWidget *p,int x,int y)
{
	mWidget *p2 = NULL;
	
	if(!p || (p->ftype & MWIDGET_TYPE_WINDOW))
		return;
		
	p->x = x, p->y = y;

	//絶対位置

	p->absX = p->parent->absX + x;
	p->absY = p->parent->absY + y;

	//下位の絶対位置変更

	while(1)
	{
		p2 = __mWidgetGetTreeNext_root(p2, p);
		if(!p2) break;

		p2->absX = p2->parent->absX + p2->x;
		p2->absY = p2->parent->absY + p2->y;
	}

	mWidgetRedraw(p->parent);
}

/**@ サイズ変更
 *
 * @d:ウィンドウの場合、最大化/フルスクリーン化されていない時のみ有効。
 * 
 * @p:w,h ウィンドウ時は、装飾を含まないサイズ
 * @r:FALSE でサイズの変化がなかった */

mlkbool mWidgetResize(mWidget *p,int w,int h)
{
	if(w < 1) w = 1;
	if(h < 1) h = 1;

	if(!p || (w == p->w && h == p->h)) return FALSE;

	p->w = w;
	p->h = h;

	if(p->ftype & MWIDGET_TYPE_WINDOW)
	{
		//ウィンドウ

		if(MLK_WINDOW_IS_NORMAL_SIZE((mWindow *)p))
			__mWindowOnResize_setNormal(MLK_WINDOW(p), w, h);
	}
	else
		//ウィジェット (レイアウト)
		__mWidgetOnResize(p);

	return TRUE;
}

/**@ 位置とサイズを変更
 *
 * @d:{em:layout ハンドラ内で、子ウィジェットの位置とサイズを変更する場合は、
 * 常にこの関数を使うこと。:em}
 *
 * @r:サイズが変更されたか */

mlkbool mWidgetMoveResize(mWidget *p,int x,int y,int w,int h)
{
	if(!p) return FALSE;

	if(MLKAPP->flags & MAPPBASE_FLAGS_IN_WIDGET_LAYOUT)
	{
		//layout ハンドラ内で呼ばれた場合

		return __mWidgetLayoutMoveResize(p, x, y, w, h);
	}
	else
	{
		mWidgetMove(p, x, y);

		return mWidgetResize(p, w, h);
	}
}


//============================
// フォーカス関連
//============================


/**@ フォーカスウィジェットを変更
 *
 * @g:フォーカス
 *
 * @r:フォーカス状態が変更されたか */

mlkbool mWidgetSetFocus(mWidget *p)
{
	return __mWindowSetFocus(p->toplevel, p, MEVENT_FOCUS_FROM_UNKNOWN);
}

/**@ フォーカスのセットと再描画を行う
 *
 * @d:フォーカスが変更された時は、再描画を行う。
 * 
 * @p:force TRUE で常にウィジェットを再描画。\
 * FALSE の場合、フォーカスが変更された時のみ再描画を行う。 */

void mWidgetSetFocus_redraw(mWidget *p,mlkbool force)
{
	mlkbool ret;

	ret = mWidgetSetFocus(p);

	if(force || ret)
		mWidgetRedraw(p);
}

/**@ root を含む下位全てのウィジェットで、フォーカスを受け付けないようにする
 *
 * @d:fstate の MWIDGET_STATE_TAKE_FOCUS を OFF にする。 */

void mWidgetSetNoTakeFocus_under(mWidget *root)
{
	mWidget *p;

	for(p = root; p; p = __mWidgetGetTreeNext_root(p, root))
		p->fstate &= ~MWIDGET_STATE_TAKE_FOCUS;
}


//============================
// レイアウト関連
//============================


/**@ レイアウトサイズを再計算させる
 *
 * @g:レイアウト
 *
 * @d:上位の推奨サイズ計算フラグを ON にする。\
 * ここではフラグを ON にするだけなので、実際に計算させる場合は mGuiCalcHintSize() を実行する。 */

void mWidgetSetRecalcHint(mWidget *p)
{
	__mWidgetSetUIFlags_upper(p, MWIDGET_UI_FOLLOW_CALC | MWIDGET_UI_CALC);
}

/**@ レイアウト実行
 *
 * @d:推奨サイズの計算の実行と、layout ハンドラによるレイアウトを行う (下位すべてが対象)。 */

void mWidgetLayout(mWidget *root)
{
	mWidget *p;

	if(MLKAPP->flags & MAPPBASE_FLAGS_IN_WIDGET_LAYOUT)
		return;

	mGuiCalcHintSize();

	//calchint の後に行う

	__mAppRunConstruct();

	//レイアウトハンドラ

	MLKAPP->flags |= MAPPBASE_FLAGS_IN_WIDGET_LAYOUT;

	for(p = root; p; p = __mWidgetGetTreeNext_root(p, root))
	{
		if(p->layout)
			(p->layout)(p);
	}

	MLKAPP->flags &= ~MAPPBASE_FLAGS_IN_WIDGET_LAYOUT;
}

/**@ 再レイアウトを行う
 *
 * @d:p の推奨サイズを再計算するようにセット + レイアウト + p を再描画。 */

void mWidgetReLayout(mWidget *p)
{
	mWidgetSetRecalcHint(p);
	mWidgetLayout(p);
	mWidgetRedraw(p);
}

/**@ レイアウトと再描画を行う
 *
 * @d:推奨サイズが計算済みで、サイズの変化に関わらず再描画したい時に。 */

void mWidgetLayout_redraw(mWidget *p)
{
	mWidgetLayout(p);
	mWidgetRedraw(p);
}

/**@ 推奨サイズを再計算し、サイズが大きければリサイズ
 *
 * @d:p の推奨サイズを再計算し、結果が現在のウィジェットサイズより大きくなった場合、
 *  p のサイズを変更する。
 *
 * @r:リサイズを行ったか */

mlkbool mWidgetResize_calchint_larger(mWidget *p)
{
	int w,h;

	//推奨サイズを再計算

	mWidgetSetRecalcHint(p);

	mGuiCalcHintSize();

	//サイズが大きくなった時

	w = p->w, h = p->h;

	if(w < p->hintW || h < p->hintH)
	{
		if(w < p->hintW) w = p->hintW;
		if(h < p->hintH) h = p->hintH;
	
		mWidgetResize(p, w, h);
		return TRUE;
	}

	return FALSE;
}

/**@ 余白幅を上下左右、同じ値にセット */

void mWidgetSetMargin_same(mWidget *p,int val)
{
	__mWidgetRectSetSame(&p->margin, val);
}

/**@ 余白幅を、４つの値を一つの値にパックした値からセット
 * 
 * @p:val 上位バイトから順に left,top,right,bottom */

void mWidgetSetMargin_pack4(mWidget *p,uint32_t val)
{
	__mWidgetRectSetPack4(&p->margin, val);
}

/**@ 初期サイズを、フォント高さの倍数でセット
 *
 * @p:wmul,hmul  フォント高さの n 倍 (0 で指定しない) */

void mWidgetSetInitSize_fontHeightUnit(mWidget *p,int wmul,int hmul)
{
	int h = mWidgetGetFontHeight(p);

	p->initW = h * wmul;
	p->initH = h * hmul;
}

/**@ 構成変更状態を ON にする
 *
 * @d:アイテムが増えるなどの複数の変更が行われた後に、まとめて一度だけ処理をしたい時に使う。\
 * ON の状態の場合、メインループのあるタイミングで、各ウィジェットの construct ハンドラが呼ばれる。 */

void mWidgetSetConstruct(mWidget *p)
{
	if(!(p->fui & MWIDGET_UI_CONSTRUCT))
	{
		p->fui |= MWIDGET_UI_CONSTRUCT;

		//

		p->construct_next = MLKAPP->widget_construct_top;

		MLKAPP->widget_construct_top = p;
	}
}

/**@ 自身の構成変更状態が ON の場合、すぐにハンドラを実行する
 *
 * @d:通常は自動的にハンドラが呼ばれるが、明示的なタイミングで実行したい場合に使う。 */

void mWidgetRunConstruct(mWidget *p)
{
	if(p->fui & MWIDGET_UI_CONSTRUCT)
	{
		__mAppDelConstructWidget(p);

		p->fui &= ~MWIDGET_UI_CONSTRUCT;

		//

		if(p->construct)
			(p->construct)(p);
	}
}


//============================
// 描画・更新
//============================


/**@ draw ハンドラ時の描画範囲を取得
 *
 * @g:描画・更新
 *
 * @d:draw ハンドラ内で、全体を描画せず、
 *  事前に指定した範囲のみを描画したい時に使う。\
 *  mWidgetRedrawBox() で描画する範囲を指定し、draw ハンドラ内でこの関数を実行して、範囲を取得する。\
 *  範囲指定後に、全体を更新するような状態になった時は、0 が返るため、その時は全体を描画する。
 *
 * @p:box NULL で範囲を取得しない。戻り値のみ取得したい場合は NULL。
 * @r:描画する範囲。\
 *  1 = ウィジェットの一部が描画範囲。\
 *  0 = ウィジェット全体が描画範囲。\
 *  -1 = 描画範囲は、すべてウィジェットの範囲外。ウィジェットの範囲は (0,0)-WxH。 */

int mWidgetGetDrawBox(mWidget *p,mBox *box)
{
	mRect rc;

	rc = p->draw_rect;

	//draw_rect をウィジェット内にクリッピング

	if(!mRectClipBox_d(&rc, 0, 0, p->w, p->h))
		return -1;
	else
	{
		//rc -> box
		if(box)
			mBoxSetRect(box, &rc);
	
		if(rc.x1 == 0 && rc.y1 == 0
			&& rc.x2 == p->w - 1 && rc.y2 == p->w - 1)
			//全体
			return 0;
		else
			return 1;
	}
}

/**@ ウィジェットの左上の描画位置を、絶対値で取得
 *
 * @d:mPixbuf に直接描画する際の左上位置となる。 */

void mWidgetGetDrawPos_abs(mWidget *p,mPoint *pt)
{
	mWindow *win = p->toplevel;

	pt->x = p->absX + win->win.deco.w.left;
	pt->y = p->absY + win->win.deco.w.top;
}

/**@ ウィジェット全体を描画＆更新させる
 *
 * @d:ここでは更新情報をセットするだけで、実際には描画・更新は行われない。 */

void mWidgetRedraw(mWidget *p)
{
	__mWidgetSetUIFlags_draw(p);

	mRectSetBox_d(&p->draw_rect, 0, 0, p->w, p->h);

	mWidgetUpdateBox_d(p, 0, 0, p->w, p->h);
}

/**@ ウィジェットの指定範囲を描画＆更新させる
 *
 * @d:draw ハンドラ内で、mWidgetGetDrawBox() を使って描画範囲を取得すること。 */

void mWidgetRedrawBox(mWidget *p,mBox *box)
{
	//すでに再描画状態の場合は、範囲を追加

	if(p->fui & MWIDGET_UI_DRAW)
		mRectUnion_box(&p->draw_rect, box);
	else
		mRectSetBox(&p->draw_rect, box);

	//

	__mWidgetSetUIFlags_draw(p);

	mWidgetUpdateBox(p, box);
}

/**@ ウィジェットの更新範囲を追加 (mBox)
 *
 * @d:描画は行われず、画面に転送する範囲を追加するのみ。
 * 
 * @p:box NULL でウィジェット全体 */

void mWidgetUpdateBox(mWidget *p,mBox *box)
{
	mWindow *win = p->toplevel;
	mRect rc;

	rc.x1 = p->absX;
	rc.y1 = p->absY;

	if(box)
	{
		rc.x1 += box->x;
		rc.y1 += box->y;
		rc.x2 = rc.x1 + box->w - 1;
		rc.y2 = rc.y1 + box->h - 1;
	}
	else
	{
		rc.x2 = rc.x1 + p->w - 1;
		rc.y2 = rc.y1 + p->h - 1;
	}

	__mWindowUpdateRect(win, &rc);
}

/**@ ウィジェットの更新範囲を追加 */

void mWidgetUpdateBox_d(mWidget *p,int x,int y,int w,int h)
{
	mWindow *win = p->toplevel;
	mRect rc;
	
	rc.x1 = p->absX + x;
	rc.y1 = p->absY + y;
	rc.x2 = rc.x1 + w - 1;
	rc.y2 = rc.y1 + h - 1;

	__mWindowUpdateRect(win, &rc);
}

/**@ 背景の描画を行う
 *
 * @d:draw ハンドラ内で、背景描画を行う際に使う。\
 * draw_bkgnd が NULL で、親によって全体が背景描画されていた場合は、何も描画しない。
 *
 * @p:box NULL でウィジェット全体 */

void mWidgetDrawBkgnd(mWidget *p,mBox *box)
{
	__mWidgetDrawBkgnd(p, box, FALSE);
}

/**@ 背景の描画を行う (常に描画)
 *
 * @d:mWidgetDrawBkgnd() と同じだが、親によって背景描画されていた場合でも、
 * 強制的に描画を行う。 */

void mWidgetDrawBkgnd_force(mWidget *p,mBox *box)
{
	__mWidgetDrawBkgnd(p, box, TRUE);
}

/**@ mPixbuf への直接描画を開始
 *
 * @d:描画タイミングに関係なく、一部の内容を直接書き換えたい時に使う。
 *
 * @r:描画先の mPixbuf。\
 *  NULL の場合、非表示やクリッピングなどによって、内容を描画する必要はない。 */

mPixbuf *mWidgetDirectDraw_begin(mWidget *p)
{
	mPixbuf *pixbuf;
	mRect rc;
	mPoint pt;

	//全体の描画要求がある、または非表示状態

	if((p->fui & MWIDGET_UI_DRAW) || !mWidgetIsVisible_self(p))
		return NULL;

	//クリッピング

	pixbuf = (p->toplevel)->win.pixbuf;

	if(!__mWidgetGetWindowRect(p, &rc, &pt, TRUE))
		return NULL;

	__mPixbufSetClipRoot(pixbuf, &rc);

	mPixbufSetOffset(pixbuf, pt.x, pt.y, NULL);

	return pixbuf;
}

/**@ 直接描画を終了する */

void mWidgetDirectDraw_end(mWidget *p,mPixbuf *pixbuf)
{
	if(pixbuf)
	{
		__mPixbufSetClipRoot(pixbuf, NULL);
		mPixbufSetOffset(pixbuf, 0, 0, NULL);
	}
}


//============================
// イベント
//============================


/**@ 通知先ウィジェット取得
 *
 * @g:イベント関連
 *
 * @d:通知先の置き換えは有効。 */

mWidget *mWidgetGetNotifyWidget(mWidget *p)
{
	return _get_notify_widget(p, TRUE);
}

/**@ 通知先ウィジェット取得
 *
 * @d:通知先の置き換えは無効。 */

mWidget *mWidgetGetNotifyWidget_raw(mWidget *p)
{
	return _get_notify_widget(p, FALSE);
}

/**@ イベント追加
 *
 * @p:p 送り先のウィジェット
 * @p:size イベント構造体のサイズ */

mEvent *mWidgetEventAdd(mWidget *p,int type,int size)
{
	return mEventListAdd(p, type, size);
}

/**@ イベント追加 (タイプ指定のみ) */

void mWidgetEventAdd_base(mWidget *p,int type)
{
	mEventListAdd_base(p, type);
}

/**@ 通知イベント追加
 *
 * @p:from 送り元のウィジェット (mEventNotify::widget_from にセットされる)
 * @p:send 送り先のウィジェット。\
 *  NULL で、from の通知先ウィジェット。\
 *  MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW (=1) の場合、
 *  mWidgetGetNotifyWidget_raw(from) で取得したウィジェットを使う。
 * @p:type 通知タイプ */
 
void mWidgetEventAdd_notify(mWidget *from,
	mWidget *send,int type,intptr_t param1,intptr_t param2)
{
	mWidgetEventAdd_notify_id(from, send,
		from->id, type, param1, param2);
}

/**@ NOTIFY イベント追加 (id 指定) */

void mWidgetEventAdd_notify_id(mWidget *from,mWidget *send,int id,
	int type,intptr_t param1,intptr_t param2)
{
	mEventNotify *ev;
	
	if(!send)
		send = _get_notify_widget(from, TRUE);
	else if(send == MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW)
		send = _get_notify_widget(from, FALSE);;

	//

	ev = (mEventNotify *)mEventListAdd(send, MEVENT_NOTIFY, sizeof(mEventNotify));
	if(ev)
	{
		ev->widget_from = from;
		ev->notify_type = type;
		ev->id = id;
		ev->param1 = param1;
		ev->param2 = param2;
	}
}

/**@ COMMAND イベント追加 */

void mWidgetEventAdd_command(mWidget *p,int id,intptr_t param,int from,void *from_ptr)
{
	mEventCommand *ev;

	if(p)
	{
		ev = (mEventCommand *)mEventListAdd(p, MEVENT_COMMAND, sizeof(mEventCommand));
		if(ev)
		{
			ev->id = id;
			ev->param = param;
			ev->from = from;
			ev->from_ptr = from_ptr;
		}
	}
}


//============================
// タイマー
//============================


/**@ タイマー追加
 *
 * @d:一度設定すると、指定間隔でずっと動作する。\
 * 同じ ID のタイマーが存在している場合は、置き換わる。
 *
 * @p:id タイマー ID
 * @p:msec 間隔 (ms) */

void mWidgetTimerAdd(mWidget *p,int id,uint32_t msec,intptr_t param)
{
	mGuiTimerAdd(p, id, msec, param);
}

/**@ 指定 ID のタイマーが存在しない場合のみ追加
 *
 * @r:存在していた場合 FALSE */

mlkbool mWidgetTimerAdd_ifnothave(mWidget *p,
	int id,uint32_t msec,intptr_t param)
{
	if(mGuiTimerFind(p, id))
		return FALSE;
	else
	{
		mWidgetTimerAdd(p, id, msec, param);
		return TRUE;
	}
}

/**@ 指定 ID のタイマーが存在するか */

mlkbool mWidgetTimerIsHave(mWidget *p,int id)
{
	return mGuiTimerFind(p, id);
}

/**@ 指定タイマー削除
 * 
 * @r:タイマーを削除したか */

mlkbool mWidgetTimerDelete(mWidget *p,int id)
{
	return mGuiTimerDelete(p, id);
}

/**@ ウィジェットのタイマーをすべて削除 */

void mWidgetTimerDeleteAll(mWidget *p)
{
	mGuiTimerDelete_widget(p);
}


//===============================
// ほか
//===============================


/**@ ウィジェットにカーソルをセット
 *
 * @g:ほか
 *
 * @d:カーソルポインタがこのウィジェット上にいる間は、指定したカーソルに変更される。\
 * 新規作成したカーソルをセットした場合は、必要なくなった時に、明示的に削除すること。\
 * カーソルキャッシュからセットされていた場合、(他で使われていない場合は) 現在のカーソルは未使用状態となる。
 * 
 * @p:cur 0 で標準カーソルに戻す
 * @r:カーソルが変更されたか */

mlkbool mWidgetSetCursor(mWidget *p,mCursor cur)
{
	if(p->cursor == cur)
		return FALSE;
	else
	{
		//キャッシュから解放

		if(p->cursor && p->cursor == p->cursor_cache)
		{
			mCursorCache_release(p->cursor_cache);

			p->cursor_cache = 0;
			p->cursor_cache_type = MCURSOR_TYPE_NONE;
		}
	
		//セット
	
		p->cursor = cur;

		//現在、ポインタがウィジェットの上にある場合、
		//または、ウィジェットがグラブ中の場合は、ただちに変更

		if(p == MLKAPP->widget_enter
			|| p == MLKAPP->widget_grabpt)
			(MLKAPP->bkend.window_setcursor)(p->toplevel, cur);

		return TRUE;
	}
}

/**@ カーソルキャッシュから、指定タイプのカーソルをセット
 *
 * @d:キャッシュに存在しない場合は新規ロードされ、
 *  存在する場合はそれがセットされる。\
 *  セットしたカーソルの解放は、自動で行われる。
 *
 * @r:カーソルが変更されたか */

mlkbool mWidgetSetCursor_cache_type(mWidget *p,int curtype)
{
	mCursor cur;

	if(p->cursor_cache && p->cursor_cache_type == curtype)
		return FALSE;
	else
	{
		//取得

		cur = mCursorCache_getCursor_type(curtype);
		if(cur == p->cursor) return FALSE;

		//変更

		mWidgetSetCursor(p, cur);

		p->cursor_cache = cur;
		p->cursor_cache_type = curtype;

		return TRUE;
	}
}

/**@ カーソルキャッシュに格納されているカーソルをセット
 *
 * @d:自分で読み込んだカーソルをキャッシュにセットした場合に使う。\
 *  カーソルの解放は自動で行われる。\
 *  セットしたカーソルは、明示的に解放してはならない。
 *
 * @r:カーソルが変更されたか */

mlkbool mWidgetSetCursor_cache_cur(mWidget *p,mCursor cur)
{
	if(p->cursor_cache == cur || p->cursor == cur)
		return FALSE;
	else
	{
		mWidgetSetCursor(p, cur);

		p->cursor_cache = cur;
		p->cursor_cache_type = MCURSOR_TYPE_NONE;

		return TRUE;
	}
}

/**@ ポインタをグラブする
 *
 * @d:グラブ中、ポインタのイベントはすべて、現在のウィジェットに対して送られる。\
 *  なお、ウィジェットのトップレベルウィンドウがアクティブ状態であることが前提。\
 *  ※バックエンドにおけるグラブは行われないため、ボタンを押していない状態でグラブをしても、
 *  ウィンドウ外のポインタイベントは来ない。
 *
 * @r:FALSE ですでにグラブ済み、または、ウィンドウがアクティブ状態ではない */

mlkbool mWidgetGrabPointer(mWidget *p)
{
	mAppBase *app = MLKAPP;

	if(app->widget_grabpt
		|| !mWindowIsActivate(p->toplevel))
		return FALSE;
	else
	{
		app->widget_grabpt = p;

		//グラブ開始時の enter ウィジェットを記録しておく
		app->widget_enter_last = app->widget_enter;

		return TRUE;
	}
}

/**@ ポインタのグラブを解放する
 *
 * @r:TRUE で解放した */

mlkbool mWidgetUngrabPointer(void)
{
	if(!MLKAPP->widget_grabpt)
		return FALSE;
	else
	{
		MLKAPP->widget_grabpt = NULL;

		__mEventOnUngrab();

		return TRUE;
	}
}

/**@ 現在ポインタがグラブされているウィジェットを取得
 *
 * @r:なければ NULL */

mWidget *mWidgetGetGrabPointer(void)
{
	return MLKAPP->widget_grabpt;
}


//===============================
// テキスト
//===============================


/**@ 複数行ラベル文字列から、各行の情報データを作成
 *
 * @g:mWidgetLabelText
 *
 * @d:text の文字列を元に、複数行の場合は、
 * mWidgetLabelTextLineInfo x (行数 + 1) 分のバッファを確保し、各行の位置と長さをセットして返す。\
 * 終端のデータでは、pos が -1 となる。\
 * 文字列が単一行の場合は、データを作成せず、NULL となる。
 *
 * @r:確保されたバッファ。mFree() で解放する。 */

mWidgetLabelTextLineInfo *mWidgetLabelText_createLineInfo(const char *text)
{
	mWidgetLabelTextLineInfo *buf,*pd;
	const char *pc,*pcend;
	int len,lines;
	
	if(!text) return NULL;
	
	//行数 (単一行なら確保しない)
	
	lines = mStringGetLineCnt(text);
	if(lines <= 1) return NULL;

	//確保
	
	buf = (mWidgetLabelTextLineInfo *)mMalloc(
		sizeof(mWidgetLabelTextLineInfo) * (lines + 1));
	
	if(!buf) return NULL;
	
	//各行処理

	pd = buf;
	
	for(pc = text; *pc; pd++)
	{
		pcend = mStringFindChar(pc, '\n');

		len = pcend - pc;

		//'\n' が終端にある場合は無視
		
		if(len == 0 && *pcend == 0) break;
		
		//
		
		pd->pos = pc - text;
		pd->len = len;

		pc = pcend;
		if(*pc) pc++;
	}

	//終端
	pd->pos = -1;

	return buf;
}

/**@ 複数行ラベルテキストの、各行の幅と全体のテキストサイズを取得
 *
 * @d:text を元に、mWidgetLabelTextLineInfo の各行の width に、各行のピクセル幅をセット。\
 * また、テキストの全体サイズを dstsize にセット。
 *
 * @p:text ラベル文字列。NULL で空文字列。
 * @p:buf 各行の情報バッファ。単一行の場合 NULL。
 * @p:dstsize テキストの全体サイズが入る */

void mWidgetLabelText_getSize(mWidget *p,const char *text,mWidgetLabelTextLineInfo *buf,mSize *dstsize)
{
	mFont *font;
	int w,maxw = 0,lines;
	
	font = mWidgetGetFont(p);

	if(!text || !buf)
	{
		//空文字列、または単一行
		
		dstsize->w = (text)? mFontGetTextWidth(font, text, -1): 0;
		dstsize->h = mFontGetHeight(font);
	}
	else
	{
		//複数行

		for(lines = 0; buf->pos >= 0; buf++, lines++)
		{
			w = mFontGetTextWidth(font, text + buf->pos, buf->len);
			
			buf->width = w;
			
			if(w > maxw) maxw = w;
		}
		
		dstsize->w = maxw;
		dstsize->h = mFontGetHeight(font) + mFontGetLineHeight(font) * (lines - 1);
	}
}

/**@ ラベルテキスト解放 */

void mWidgetLabelText_free(mWidgetLabelText *lt)
{
	if(lt->fcopy)
		mFree(lt->text);

	mFree(lt->linebuf);
}

/**@ ラベルテキストセット
 *
 * @d:すでにセットされている場合、元の情報は削除される。\
 * また、再描画の更新も行われる。
 *
 * @p:lt 情報のセット先
 * @p:text ラベル文字列
 * @p:fcopy 文字列をコピーするか。\
 *  FALSE の場合、text のポインタがそのままセットされる。 */

void mWidgetLabelText_set(mWidget *p,mWidgetLabelText *lt,const char *text,int fcopy)
{
	//解放

	if(lt->fcopy)
	{
		mFree(lt->text);
		lt->fcopy = FALSE;
	}

	mFree(lt->linebuf);

	//セット

	if(!text)
	{
		lt->text = NULL;
		lt->linebuf = NULL;
	}
	else
	{
		if(fcopy)
		{
			lt->text = mStrdup(text);
			lt->fcopy = TRUE;
		}
		else
			lt->text = (char *)text;
		
		lt->linebuf = mWidgetLabelText_createLineInfo(text);
	}

	//レイアウト済みなら再描画

	if(p->fui & MWIDGET_UI_LAYOUTED)
		mWidgetRedraw(p);
}

/**@ ラベルテキスト用、calc_hint ハンドラ内で実行する
 *
 * @d:各行の幅と全体のテキストサイズを計算する。
 *
 * @p:size NULL 以外で、テキストの全体サイズが入る。\
 *  これを元に、推奨サイズをセットする。 */

void mWidgetLabelText_onCalcHint(mWidget *p,mWidgetLabelText *lt,mSize *size)
{
	mWidgetLabelText_getSize(p, lt->text, lt->linebuf, &lt->szfull);

	if(size)
	{
		size->w = lt->szfull.w;
		size->h = lt->szfull.h;
	}
}

/**@ ラベルテキストの描画
 *
 * @d:draw ハンドラ内で実行する。
 *
 * @p:x,y テキスト位置
 * @p:w テキスト描画部分の幅。中央位置や、右寄せ時に使われる。\
 *  単一行で左寄せなら、必要ない。
 * @p:col テキスト色 (RGB)
 * @p:flags テキスト描画用フラグ */

void mWidgetLabelText_draw(mWidgetLabelText *lt,mPixbuf *pixbuf,mFont *font,
	int x,int y,int w,mRgbCol col,uint32_t flags)
{
	mWidgetLabelTextLineInfo *buf;
	int tx,ty,lh;

	if(!lt->text) return;

	buf = lt->linebuf;
	
	if(!buf)
	{
		//単一行

		if(flags & MWIDGETLABELTEXT_DRAW_F_CENTER)
			tx = (w - lt->szfull.w) / 2;
		else if(flags & MWIDGETLABELTEXT_DRAW_F_RIGHT)
			tx = w - lt->szfull.w;
		else
			tx = 0;

		mFontDrawText_pixbuf(font, pixbuf, x + tx, y, lt->text, -1, col);
	}
	else
	{
		//複数行

		lh = mFontGetLineHeight(font);
		
		for(ty = 0; buf->pos >= 0; buf++)
		{
			if(flags & MWIDGETLABELTEXT_DRAW_F_CENTER)
				tx = (w - buf->width) / 2;
			else if(flags & MWIDGETLABELTEXT_DRAW_F_RIGHT)
				tx = w - buf->width;
			else
				tx = 0;
			
			mFontDrawText_pixbuf(font, pixbuf, x + tx, y + ty,
				lt->text + buf->pos, buf->len, col);
			
			ty += lh;
		}
	}
}
