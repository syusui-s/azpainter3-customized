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
 * mPanel
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_panel.h"
#include "mlk_panelheader.h"
#include "mlk_splitter.h"
#include "mlk_event.h"
#include "mlk_util.h"


//------------------------

struct _mPanel
{
	int id,				//パネルの ID
		height;			//[格納時] store_topwg のウィジェット高さ (ヘッダの高さ含む。サイズ変更時は常に記録)
	uint32_t fstyle,	//パネルのスタイル
		stflags,		//現在の状態フラグ
		win_style;		//[ウィンドウモード時] ウィンドウスタイル
	intptr_t param1,	//パネルのパラメータ値
		param2;

	mToplevelSaveState winstate;	//[ウィンドウモード時] 位置とサイズの状態

	char *title;	//タイトル文字列 (複製)
	mFont *font;	//パネル用フォント (NULL で親のフォント)

	mWindow *winmode_parent;	//[ウィンドウモード時] ウィンドウの親
	mToplevel *winmode_win;		//[ウィンドウモード時] ウィンドウウィジェット
								//(格納時 or 閉じている時は NULL)
		
	mWidget *store_parent,	//[格納時] 格納先の親ウィジェット
		*store_topwg,		//[格納時] トップコンテナのウィジェット
		*notify_wg,			//通知先ウィジェット
		*wg_contents;		//[共通] 内容のトップウィジェット

	mSplitter *splitter;		//[格納時] スプリッター
	mPanelHeader *header;

	mFuncPanelCreate func_create;	//内容作成関数
	mFuncPanelDestroy func_destroy;	//破棄関数
	mFuncPanelSort func_sort;		//パネルのソート関数
};

//------------------------

static int _handle_event(mWidget *wg,mEvent *ev);

//------------------------

/*
 * ==== ウィンドウモード時 ====
 * 
 * mToplevel / (param1 = mPanel *)
 *   |- mPanelHeader
 *   |- 内容 (wg_contents)
 *
 * [!] store_topct は NULL
 * 
 * ==== 格納時 ====
 *
 * 格納先 (store_parent)
 *   |- mContainer (store_topwg) / (param1 = mPanel *)
 *       |- mPanelHeader (header)
 *       |- 内容 (wg_contents)
 *   |- mSplitter (splitter) / 常に作成。状況に応じて表示/非表示。param1 = NULL にすること。
 *   ...
 *
 */


//==============================
// sub
//==============================


/* PANEL イベント追加 */

static void _send_event(mPanel *p,int act)
{
	mEventPanel *ev;

	if(p->notify_wg)
	{
		ev = (mEventPanel *)mWidgetEventAdd(p->notify_wg, MEVENT_PANEL, sizeof(mEventPanel));
		if(ev)
		{
			ev->panel = p;
			ev->id = p->id;
			ev->act = act;
			ev->param1 = p->param1;
			ev->param2 = p->param2;
		}
	}
}

/* 内容が表示されている状態か
 * (非表示、折りたたみ状態の場合 FALSE) */

static mlkbool _is_contents_visible(mPanel *p)
{
	return (p && (p->stflags & MPANEL_F_VISIBLE) && !(p->stflags & MPANEL_F_SHUT_CLOSED));
}

/* [格納時] 格納先の挿入位置取得
 *
 * return: NULL で終端 */

static mWidget *_get_store_insert_pos(mPanel *p)
{
	mWidget *wg;
	mPanel *panel;

	for(wg = p->store_parent->first; wg; wg = wg->next)
	{
		panel = (mPanel *)wg->param1; //NULL の場合、mSplitter

		//panel の方が下なら、この位置に挿入

		if(panel && (p->func_sort)(p, panel->id, p->id) > 0)
			return wg;
	}

	return NULL;
}

/* [格納時] 格納先に EXPAND_HEIGHT のパネルがあり、かつその内容が表示されているか */

static mlkbool _is_store_have_expand(mPanel *p)
{
	mWidget *wg;
	mPanel *panel;

	for(wg = p->store_parent->first; wg; wg = wg->next)
	{
		panel = (mPanel *)wg->param1;

		if(_is_contents_visible(panel) && (panel->fstyle & MPANEL_S_EXPAND_HEIGHT))
			return TRUE;
	}

	return FALSE;
}

/* [格納時] 現在の配置状態から、p のパネルの状態を変更
 *
 * 表示状態、高さ、レイアウト、スプリッターの表示/非表示 */

static void _set_store_state(mPanel *p)
{
	mWidget *wgtop,*wg;
	mPanel *panel;
	mlkbool is_last,is_last_visible,is_show,is_shut_open,flag;

	//is_last : FALSE で、以降に表示&展開状態のパネルがある。
	//is_last_visible : FALSE で、以降に折りたたみ or 展開状態の表示パネルがある。
	
	is_last = TRUE;
	is_last_visible = TRUE;

	for(wg = (mWidget *)p->splitter; wg; wg = wg->next)
	{
		panel = (mPanel *)wg->param1;
		if(panel)
		{
			if(panel->stflags & MPANEL_F_VISIBLE)
			{
				is_last_visible = FALSE;

				if(!(panel->stflags & MPANEL_F_SHUT_CLOSED))
					is_last = FALSE;
			}
		}
	}

	//

	wgtop = p->store_topwg;
	is_show = ((p->stflags & MPANEL_F_VISIBLE) != 0);
	is_shut_open = !(p->stflags & MPANEL_F_SHUT_CLOSED);

	//高さ

	wgtop->h = p->height;

	//内容の表示/非表示

	mWidgetShow(p->wg_contents, is_shut_open);

	//全体の表示/非表示

	mWidgetShow(wgtop, is_show);

	//表示時、レイアウト変更

	flag = FALSE;

	if(is_show)
	{
		wgtop->flayout &= ~(MLF_FIX_H | MLF_EXPAND_H);

		//展開状態時
		// :折りたたみ時は推奨サイズ (ヘッダのみ表示)

		if(is_shut_open)
		{
			if(p->fstyle & MPANEL_S_EXPAND_HEIGHT)
				//常に拡張
				flag = TRUE;
			else if(is_last && !_is_store_have_expand(p))
			{
				//格納先に拡張対象がなく、p が最後の表示&展開パネル

				if(!is_last_visible && (p->fstyle & MPANEL_S_NO_EXPAND_LAST))
					//以降に折りたたみのパネルがあり、拡張しない場合は固定
					flag = FALSE;
				else
					//最後のパネルなら常に拡張
					flag = TRUE;
			}

			if(flag)
				wgtop->flayout |= MLF_EXPAND_H;
			else
				wgtop->flayout |= MLF_FIX_H;
		}
	}

	//スプリッター

	mWidgetShow(MLK_WIDGET(p->splitter),
		(is_show && is_shut_open && !flag && !(p->stflags & MPANEL_F_RESIZE_DISABLED)) );
}

/* [格納時] 格納先のすべてのパネルの状態をセット
 *
 * parent: NULL で store_parent。親が変更された時は直接指定。 */

static void _set_store_state_all(mPanel *p,mWidget *parent)
{
	mWidget *wg;

	if(!parent) parent = p->store_parent;

	for(wg = parent->first; wg; wg = wg->next)
	{
		if(wg->param1)
			_set_store_state((mPanel *)wg->param1);
	}
}

/* [格納時] 現在の高さを記録 */

static void _save_store_height(mPanel *p)
{
	if(p->store_topwg
		&& !(p->stflags & MPANEL_F_SHUT_CLOSED)
		&& (p->store_topwg->flayout & MLF_FIX_H))
		p->height = p->store_topwg->h;
}

/* [格納時] 格納先のすべてのパネルの高さを記録 */

static void _save_store_height_all(mPanel *p)
{
	mWidget *wg;

	for(wg = p->store_parent->first; wg; wg = wg->next)
	{
		if(wg->param1)
			_save_store_height((mPanel *)wg->param1);
	}
}

/* [格納時] 格納先を再レイアウト
 *
 * parent: NULL で store_parent。親が変更された時は直接指定。
 * run: 再レイアウトを実行するか。-1 で、MPANEL_S_NO_RELAYOUT を適用して実行。 */

static void _store_relayout(mPanel *p,mWidget *parent,int run)
{
	if(!parent) parent = p->store_parent;

	_set_store_state_all(p, parent);

	//

	if(run < 0)
		run = !(p->fstyle & MPANEL_S_NO_RELAYOUT);

	if(run) mWidgetReLayout(parent);
}


//==============================
// mSplitter
//==============================


/* 移動対象ウィジェットを取得
 * (折りたたみ状態のパネルは除く)
 *
 * current: mSplitter
 * return: 0 で操作を行わない */

static int _splitter_get_target(mWidget *current,mWidget **ret_prev,mWidget **ret_next)
{
	mWidget *wg,*wgprev,*wgnext;
	mPanel *panel;

	wgprev = wgnext = NULL;

	//上方向の対象を検索
	// :表示されている & 折り畳まれていない最初のウィジェット

	for(wg = current->prev; wg; wg = wg->prev)
	{
		if(_is_contents_visible((mPanel *)wg->param1))
		{
			wgprev = wg;
			break;
		}
	}

	if(!wgprev) return 0;

	//下方向の対象を検索
	// :下方向に高さ拡張のウィジェットがある場合は、それを対象にする (間のウィジェットはそのまま)。

	for(wg = current->next; wg; wg = wg->next)
	{
		panel = (mPanel *)wg->param1;

		if(panel)
		{
			//対象となる最初のウィジェット
			
			if(!wgnext && _is_contents_visible(panel))
				wgnext = wg;

			//EXPAND の場合は決定

			if(wg->flayout & MLF_EXPAND_H)
			{
				wgnext = wg;
				break;
			}
		}
	}

	//下方向の対象がない場合、最後の表示ウィジェット (折りたたみ含む)
	// :wgprev が固定で、以降が折りたたみの場合

	if(!wgnext)
	{
		for(wg = (current->parent)->last; wg != current; wg = wg->prev)
		{
			panel = (mPanel *)wg->param1;
			if(panel && (panel->stflags & MPANEL_F_VISIBLE))
			{
				wgnext = wg;
				break;
			}
		}

		if(!wgnext) return 0;
	}

	//

	*ret_prev = wgprev;
	*ret_next = wgnext;

	return 1;
}

/* ドラッグ時の情報取得関数 */

static int _func_splitter_target(mSplitter *p,mSplitterTarget *dst)
{
	mWidget *wgprev,*wgnext,*wg;
	int h;

	if(!_splitter_get_target(MLK_WIDGET(p), &wgprev, &wgnext))
		return 0;

	dst->wgprev = wgprev;
	dst->wgnext = wgnext;

	dst->prev_cur = wgprev->h;
	dst->next_cur = wgnext->h;

	//最小高さは mPanelHeader 高さ分

	dst->prev_min = dst->next_min = MLK_WIDGET(dst->param)->h;

	//

	if(!(MLK_PANEL(wgnext->param1)->stflags & MPANEL_F_SHUT_CLOSED))
		//next が折りたたみでない時 (prev,next のサイズを変更する時)
		dst->prev_max = dst->next_max = dst->prev_cur + dst->next_cur;
	else
	{
		//---- next が折りたたみ状態の場合 (next は位置のみ変更)

		//prev の最大高さ (格納先高さから、他のウィジェットの高さを引く)
		
		h = p->wg.parent->h;
		
		for(wg = (mWidget *)wgprev->prev; wg; wg = wg->prev)
		{
			if(wg->fstate & MWIDGET_STATE_VISIBLE)
				h -= wg->h;
		}

		for(wg = wgnext; wg != (mWidget *)p; wg = wg->prev)
		{
			if(wg->fstate & MWIDGET_STATE_VISIBLE)
				h -= wg->h;
		}

		h -= p->wg.h;
		if(h < 0) h = dst->prev_min;

		//

		dst->prev_max = h;
		dst->next_max = wgnext->h;
	}

	return 1;
}


//==============================
// 作成など
//==============================


/* ヘッダ (mPanelHeader) を作成
 *
 * [!] ヘッダの通知は parent へ送られる。 */

static void _create_header(mPanel *p,mWidget *parent)
{
	mPanelHeader *ph;
	uint32_t s = 0;

	//ボタン

	if(p->fstyle & MPANEL_S_HAVE_CLOSE) s |= MPANELHEADER_S_HAVE_CLOSE;
	if(p->fstyle & MPANEL_S_HAVE_STORE) s |= MPANELHEADER_S_HAVE_STORE;
	if(p->fstyle & MPANEL_S_HAVE_SHUT) s |= MPANELHEADER_S_HAVE_SHUT;
	if(p->fstyle & MPANEL_S_HAVE_RESIZE) s |= MPANELHEADER_S_HAVE_RESIZE;

	//ウィンドウモード時

	if(p->winmode_win)
	{
		s |= MPANELHEADER_S_STORE_LEAVED;
		s &= ~(MPANELHEADER_S_HAVE_SHUT | MPANELHEADER_S_HAVE_CLOSE | MPANELHEADER_S_HAVE_RESIZE);
	}

	//折りたたみ状態
	
	if(p->stflags & MPANEL_F_SHUT_CLOSED)
		s |= MPANELHEADER_S_SHUT_CLOSED;

	//リサイズ無効時

	if((s & MPANELHEADER_S_HAVE_RESIZE)
		&& (p->stflags & MPANEL_F_RESIZE_DISABLED))
		s |= MPANELHEADER_S_RESIZE_DISABLED;

	//作成

	p->header = ph = mPanelHeaderNew(parent, 0, s);

	ph->wg.notify_to = MWIDGET_NOTIFYTO_PARENT;

	mPanelHeaderSetTitle(ph, p->title);
}

/* (ウィンドウモード) ウィジェット作成 */

static mlkbool _create_window(mPanel *p)
{
	mToplevel *win;

	win = mToplevelNew(p->winmode_parent, 0, p->win_style);
	if(!win) return FALSE;

	p->winmode_win = win;

	//

	win->wg.param1 = (intptr_t)p;
	win->wg.event = _handle_event;
	win->wg.font = p->font;

	mToplevelSetTitle(win, p->title);

	//ウィジェット

	_create_header(p, MLK_WIDGET(win));

	p->wg_contents = (p->func_create)(p, p->id, MLK_WIDGET(win));

	//位置・サイズ

	mToplevelSetSaveState(win, &p->winstate);

	return TRUE;
}

/* (格納時) ウィジェット作成 */

static mlkbool _create_store_widget(mPanel *p)
{
	mSplitter *sp;
	mWidget *parent,*ins,*wgtop,*wgct;

	parent = p->store_parent;

	if(!parent) return FALSE;

	//作成前に、挿入位置取得

	ins = _get_store_insert_pos(p);

	//トップコンテナ

	p->store_topwg = wgtop = mContainerNew(parent, 0);

	wgtop->param1 = (intptr_t)p;
	wgtop->event = _handle_event;
	wgtop->font = p->font;
	wgtop->flayout = MLF_EXPAND_W;

	//ヘッダ

	_create_header(p, wgtop);

	//内容

	p->wg_contents = wgct = (p->func_create)(p, p->id, wgtop);

	if(p->stflags & MPANEL_F_SHUT_CLOSED)
		wgct->fstate &= ~MWIDGET_STATE_VISIBLE;

	//----- スプリッター
	// [!] param1 は NULL にしておく。
	//     親コンテナからの検索時に内容ウィジェットと区別できるようにする。

	p->splitter = sp = mSplitterNew(parent, 0, MSPLITTER_S_HORZ);

	sp->wg.flayout = MLF_EXPAND_W;
	sp->wg.notify_to = MWIDGET_NOTIFYTO_WIDGET;
	sp->wg.notify_widget = wgtop;

	mSplitterSetFunc_getTarget(sp, _func_splitter_target, (intptr_t)p->header);

	//----- 挿入位置へ移動

	if(ins)
	{
		mWidgetTreeMove(wgtop, parent, ins);
		mWidgetTreeMove(MLK_WIDGET(sp), parent, ins);
	}

	//----- 状態セット

	_set_store_state_all(p, NULL);

	return TRUE;
}

/* パネルのウィジェット作成 */

static void _create_widget(mPanel *p)
{
	if(p->stflags & MPANEL_F_WINDOW_MODE)
	{
		//ウィンドウモード
		
		if(!(p->winmode_win) && _create_window(p))
			p->stflags |= MPANEL_F_CREATED;
	}
	else
	{
		//格納時
	
		if(!(p->store_topwg) && _create_store_widget(p))
		{
			p->stflags |= MPANEL_F_CREATED;

			//自動再レイアウト
			
			if(!(p->fstyle & MPANEL_S_NO_RELAYOUT)
				&& (p->store_parent->fui & MWIDGET_UI_LAYOUTED))
				mWidgetReLayout(p->store_parent);
		}
	}
}

/* パネルウィジェットを閉じる (破棄)
 *
 * user_action: [0] 手動による動作 [1] ユーザーアクションによる動作
 *  [-1] モード切り替え時など内部で実行される時 */

static void _close_panel(mPanel *p,int user_action)
{
	mlkbool closed = FALSE;

	//閉じる (closed = 実際に破棄された)

	if(p->stflags & MPANEL_F_WINDOW_MODE)
	{
		//---- ウィンドウモード

		if(p->winmode_win)
		{
			mToplevelGetSaveState(p->winmode_win, &p->winstate);
		
			mWidgetDestroy(MLK_WIDGET(p->winmode_win));
			p->winmode_win = NULL;

			closed = TRUE;
		}
	}
	else
	{
		//---- 格納時
		
		if(p->store_topwg)
		{
			//高さ記録

			_save_store_height_all(p);

			//破棄
		
			mWidgetDestroy(p->store_topwg);
			mWidgetDestroy(MLK_WIDGET(p->splitter));

			p->store_topwg = NULL;
			p->splitter = NULL;

			//再レイアウト

			_store_relayout(p, NULL, -1);

			closed = TRUE;
		}
	}

	//

	p->stflags &= ~MPANEL_F_CREATED;
	p->header = NULL;
	p->wg_contents = NULL;

	//イベント (ユーザーアクション時のみ)

	if(user_action >= 1 && closed)
		_send_event(p, MPANEL_ACT_CLOSE);
}

/* パネル表示状態変更 (stflags はあらかじめ変更しておく)
 *
 * type: 0 で非表示、1 で表示 */

static void _set_visible(mPanel *p,int type)
{
	if(p->stflags & MPANEL_F_WINDOW_MODE)
	{
		//ウィンドウ

		if(p->winmode_win)
		{
			if(type == 0)
				mToplevelGetSaveState(p->winmode_win, &p->winstate);
			else
				mToplevelSetSaveState(p->winmode_win, &p->winstate);

			mWidgetShow(MLK_WIDGET(p->winmode_win), type);
		}
	}
	else
	{
		//格納時

		if(p->store_topwg)
		{
			mWidgetShow(p->store_topwg, type);
			mWidgetShow(MLK_WIDGET(p->splitter), type);

			_store_relayout(p, NULL, -1);
		}
	}
}


//==============================
// ハンドラ
//==============================


/* モード切り替え */

static void _toggle_window_mode(mPanel *p,mlkbool user_action)
{
	_close_panel(p, -1);

	p->stflags ^= MPANEL_F_WINDOW_MODE;

	_create_widget(p);

	mPanelShowWindow(p);

	//イベント

	if(user_action)
		_send_event(p, MPANEL_ACT_TOGGLE_STORE);
}

/* 折りたたむ */

static void _toggle_shut(mPanel *p,mlkbool user_action)
{
	p->stflags ^= MPANEL_F_SHUT_CLOSED;

	mPanelHeaderSetShut(p->header, -1);
	mWidgetShow(p->wg_contents, -1);

	_store_relayout(p, NULL, -1);

	//通知

	if(user_action)
		_send_event(p, MPANEL_ACT_TOGGLE_SHUT);
}

/* リサイズの有効/無効 */

static void _toggle_resize(mPanel *p,mlkbool user_action)
{
	p->stflags ^= MPANEL_F_RESIZE_DISABLED;

	mPanelHeaderSetResize(p->header, -1);

	_store_relayout(p, NULL, -1);

	//通知

	if(user_action)
		_send_event(p, MPANEL_ACT_TOGGLE_RESIZE);
}

/* イベントハンドラ
 * 
 * wg = [ウィンドウモード] winmode_win [格納時] store_topwg
 * 
 * mPanelHeader/mSplitter のイベントが来る。 */

int _handle_event(mWidget *wg,mEvent *ev)
{
	mPanel *p = (mPanel *)wg->param1;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == (mWidget *)p->splitter)
			{
				//---- スプリッター
				// サイズ変更後、格納先のすべての高さ保存

				if(ev->notify.notify_type == MSPLITTER_N_MOVED)
					_save_store_height_all(p);
			}
			else if(ev->notify.widget_from == (mWidget *)p->header)
			{
				//---- mPanelHeader

				if(ev->notify.notify_type == MPANELHEADER_N_PRESS_BUTTON)
				{
					switch(ev->notify.param1)
					{
						//閉じる
						case MPANELHEADER_BTT_CLOSE:
							_close_panel(p, TRUE);
							break;
						//モード切り替え
						case MPANELHEADER_BTT_STORE:
							_toggle_window_mode(p, TRUE);
							break;
						//折りたたむ
						case MPANELHEADER_BTT_SHUT:
							_toggle_shut(p, TRUE);
							break;
						//リサイズ
						case MPANELHEADER_BTT_RESIZE:
							_toggle_resize(p, TRUE);
							break;
					}
				}
			}
			break;

		//[ウィンドウモード時] ウィンドウの閉じるボタン
		case MEVENT_CLOSE:
			_close_panel(p, TRUE);
			break;
	}

	return 1;
}


//============================
// main
//============================


/**@ 削除 */

void mPanelDestroy(mPanel *p)
{
	if(p)
	{
		if(p->func_destroy)
			(p->func_destroy)(p, mPanelGetExPtr(p));
	
		mFree(p->title);
		mFree(p);
	}
}

/**@ 作成
 *
 * @d:この時点では、ウィジェットはまだ作成されない。\
 *  {em:※mPanel は自動で解放されないので、明示的に削除すること。:em}
 *
 * @p:id パネルの ID
 * @p:exsize mPanel のバッファに、拡張データとして追加するサイズ
 * @p:fstyle mPanel のスタイル */

mPanel *mPanelNew(int id,int exsize,uint32_t fstyle)
{
	mPanel *p;

	p = (mPanel *)mMalloc0(sizeof(mPanel) + exsize);
	if(!p) return NULL;

	p->id = id;
	p->fstyle = fstyle;

	p->height = 100;
	p->winstate.w = p->winstate.h = 200;

	return p;
}

/**@ 内容の作成関数をセット
 *
 * @d:{em:作成関数内で、mWidgetReLayout() は実行しないこと。:em}
 *
 * @p:func id でパネルの ID、parent で親のウィジェットが渡される。\
 *  parent の子として内容のウィジェットを作成し、そのトップのウィジェットを返す。 */

void mPanelSetFunc_create(mPanel *p,mFuncPanelCreate func)
{
	p->func_create = func;
}

/**@ 破棄時の関数をセット
 *
 * @d:mPanelDestroy() 時に呼ばれる。\
 *  主に拡張データに関する解放を行うために使う。\
 *  exdat は、拡張データのポインタ。 */

void mPanelSetFunc_destroy(mPanel *p,mFuncPanelDestroy func)
{
	p->func_destroy = func;
}

/**@ パネルのソート関数をセット
 *
 * @d:格納時、各パネルの配置順を決める時に使われる。
 *
 * @p:func 並び替え対象の２つのパネルの ID が引数として渡されるため、
 *  id1(上) < id2(下) なら 0 以下を返し、id1 > id2 なら正の値を返す。*/

void mPanelSetFunc_sort(mPanel *p,mFuncPanelSort func)
{
	p->func_sort = func;
}

/**@ ウィンドウモード時の情報をセット
 *
 * @p:parent 親のウィンドウ
 * @p:winstyle ウィンドウスタイル */

void mPanelSetWindowInfo(mPanel *p,mWindow *parent,uint32_t winstyle)
{
	p->winmode_parent = parent;
	p->win_style = winstyle;
}

/**@ 格納先のウィジェットをセット
 *
 * @d:ウィジェット作成後に変更する場合は、再配置と再レイアウトが必要になる。 */

void mPanelSetStoreParent(mPanel *p,mWidget *parent)
{
	p->store_parent = parent;
}

/**@ パネル状態をセット */

void mPanelSetState(mPanel *p,mPanelState *info)
{
	p->stflags = info->flags;
	p->height = info->height;
	p->winstate = info->winstate;
}

/**@ タイトルをセット
 *
 * @d:ヘッダ部分に表示される。 */

void mPanelSetTitle(mPanel *p,const char *title)
{
	mFree(p->title);

	p->title = mStrdup(title);
}

/**@ パラメータ値1をセット */

void mPanelSetParam1(mPanel *p,intptr_t param)
{
	p->param1 = param;
}

/**@ パラメータ値2をセット */

void mPanelSetParam2(mPanel *p,intptr_t param)
{
	p->param2 = param;
}

/**@ パネル用のフォントをセット
 *
 * @d:指定しない場合は、親ウィジェットのフォントが使われる。 */

void mPanelSetFont(mPanel *p,mFont *font)
{
	p->font = font;
}

/**@ 通知先のウィジェットをセット
 *
 * @d:パネルに関する操作は、MEVENT_PANEL で通知される。 */
 
void mPanelSetNotifyWidget(mPanel *p,mWidget *wg)
{
	p->notify_wg = wg;
}

//===================


/**@ 現在の状態を取得
 *
 * @g:情報の取得 */

void mPanelGetState(mPanel *p,mPanelState *state)
{
	if(p->winmode_win)
		mToplevelGetSaveState(p->winmode_win, &p->winstate);
	else if(p->store_topwg)
		_save_store_height(p);

	//

	state->winstate = p->winstate;
	state->height = p->height;
	state->flags = p->stflags;
}

/**@ ウィジェットが作成されているか
 *
 * @d:閉じている状態では、ウィジェットは存在していない。 */

mlkbool mPanelIsCreated(mPanel *p)
{
	return (p && (p->stflags & MPANEL_F_CREATED));
}

/**@ 内容のウィジェットが実際に表示されている状態か
 *
 * @d:閉じている or 畳まれている or 非表示時は、FALSE */

mlkbool mPanelIsVisibleContents(mPanel *p)
{
	return (p && p->wg_contents && mWidgetIsVisible(p->wg_contents));
}

/**@ 現在、ウィンドウモードになっているか */

mlkbool mPanelIsWindowMode(mPanel *p)
{
	return (p && (p->stflags & MPANEL_F_WINDOW_MODE));
}

/**@ 格納状態で、存在＆表示状態であるか
 *
 * @d:格納先の親に表示される状態であるかどうか。 */

mlkbool mPanelIsStoreVisible(mPanel *p)
{
	return (p && !(p->stflags & MPANEL_F_WINDOW_MODE)
		&& (p->stflags & MPANEL_F_CREATED)
		&& (p->stflags & MPANEL_F_VISIBLE));
}

/**@ ウィンドウモード時、ウィンドウを取得
 *
 * @r:格納時、または閉じている時は NULL */

mToplevel *mPanelGetWindow(mPanel *p)
{
	return (p && (p->stflags & MPANEL_F_WINDOW_MODE) && p->winmode_win)?
		p->winmode_win: NULL;
}

/**@ 内容のウィジェットを取得
 *
 * @r:閉じている時は NULL */

mWidget *mPanelGetContents(mPanel *p)
{
	return (p)? p->wg_contents: NULL;
}

/**@ 拡張部分の先頭ポインタを取得
 *
 * @d:パネル作成時に指定した拡張サイズ分が、mPanel データの末尾に追加されているので、
 * その位置を取得する。 */

void *mPanelGetExPtr(mPanel *p)
{
	if(p)
		return (void *)((uint8_t *)p + sizeof(mPanel));
	else
		return NULL;
}

/**@ パラメータ値1を取得 */

intptr_t mPanelGetParam1(mPanel *p)
{
	return p->param1;
}

/**@ パラメータ値2を取得 */

intptr_t mPanelGetParam2(mPanel *p)
{
	return p->param2;
}


//=================


/**@ パネルのウィジェットを作成する
 *
 * @g:状態変更
 *
 * @d:ウィンドウモードの場合、表示は行われないので、この後に mPanelShowWindow() を実行すること。 */

void mPanelCreateWidget(mPanel *p)
{
	if(p && (p->stflags & MPANEL_F_CREATED))
		_create_widget(p);
}

/**@ ウィンドウモード時、ウィンドウを表示
 *
 * @d:パネルのウィジェットの作成後は常に実行すること。\
 *  ウィンドウモードで表示状態の場合、ウィンドウが表示される。 */

void mPanelShowWindow(mPanel *p)
{
	if(p && p->winmode_win && (p->stflags & MPANEL_F_VISIBLE))
		mWidgetShow(MLK_WIDGET(p->winmode_win), 1);
}

/**@ 存在状態を変更
 *
 * @d:ウィジェットの作成または破棄を行う。
 * 
 * @p:type [0] 破棄(閉じる) [正] 作成 [負] 反転 */

void mPanelSetCreate(mPanel *p,int type)
{
	if(p && mGetChangeFlagState(type, p->stflags & MPANEL_F_CREATED, &type))
	{
		if(type)
		{
			//作成
			_create_widget(p);
			mPanelShowWindow(p);
		}
		else
			//破棄
			_close_panel(p, FALSE);
	}
}

/**@ 表示状態を変更
 *
 * @d:ウィジェットの作成状態はそのままで、表示状態のみ変更する。 */

void mPanelSetVisible(mPanel *p,int type)
{
	if(p && mGetChangeFlagState(type, p->stflags & MPANEL_F_VISIBLE, &type))
	{
		p->stflags ^= MPANEL_F_VISIBLE;

		//作成済みの場合、表示切り替え

		if(p->stflags & MPANEL_F_CREATED)
			_set_visible(p, type);
	}
}

/**@ ウィンドウモード/格納状態の変更
 *
 * @d:※非表示時は何もしない。
 * 
 * @p:type [0] 格納 [正] ウィンドウモード [負] 切り替え */

void mPanelSetWindowMode(mPanel *p,int type)
{
	if(p && (p->stflags & MPANEL_F_VISIBLE)
		&& mGetChangeFlagState(type, p->stflags & MPANEL_F_WINDOW_MODE, &type))
	{
		if(p->stflags & MPANEL_F_CREATED)
			//切り替え
			_toggle_window_mode(p, FALSE);
		else
			//未作成なら、フラグだけ変更
			p->stflags ^= MPANEL_F_WINDOW_MODE;
	}
}

/**@ 格納状態の時、格納先の再レイアウトを行う */

void mPanelStoreReLayout(mPanel *p)
{
	if(p && p->store_topwg)
		mWidgetReLayout(p->store_parent);
}

/**@ 格納状態の時、格納先のパネルの再配置を行う
 *
 * @d:p のウィジェットのウィジェットツリー位置を移動し、親を再レイアウトする。\
 *  格納先のウィジェットを変更したい場合は、mPanelSetStoreParent() を実行した後、この関数を実行する。
 *
 * @p:relayout 格納先ウィジェットの再レイアウトを実行するか。\
 *  FALSE の場合、手動で mPanelStoreReLayout() する必要がある。\
 *  移動するパネルが複数ある場合は、各パネルを再配置後、最後に再レイアウトを実行する。 */

void mPanelStoreReArrange(mPanel *p,mlkbool relayout)
{
	mWidget *ins,*parent,*top;

	top = p->store_topwg;
	if(!top) return;

	//現在の実際の親

	parent = top->parent;

	//挿入先
	
	ins = _get_store_insert_pos(p);

	if(top == ins) return;

	//移動
	
	mWidgetTreeMove(top, p->store_parent, ins);
	mWidgetTreeMove(MLK_WIDGET(p->splitter), p->store_parent, ins);

	//状態変更

	_store_relayout(p, NULL, relayout);

	//格納先が変更された場合、元の親も

	if(parent != p->store_parent)
		_store_relayout(p, parent, relayout);
}

