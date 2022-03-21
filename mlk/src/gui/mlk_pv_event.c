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
 * イベント関連の共通関数
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_key.h"
#include "mlk_str.h"
#include "mlk_string.h"
#include "mlk_window_deco.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"
#include "mlk_pv_event.h"


//---------------------

mlkbool __mAcceleratorProcKeyEvent(mAccelerator *accel,uint32_t key,uint32_t state);
mlkbool __mMenuBarShowHotkey(mMenuBar *p,uint32_t key,uint32_t state);

//---------------------


//==========================
// sub
//==========================


/** ポップアップ時のウィンドウ取得 */

static mWindow *_get_popup(void)
{
	mAppRunData *run = MLKAPP->run_current;

	if(run && run->type == MAPPRUN_TYPE_POPUP)
		return run->window;
	else
		return NULL;
}

/** ポップアップ中で、かつ、除外対象であるウィンドウ/ウィジェットかどうか
 *
 * ウィンドウがポップアップでない場合は除外対象。
 * ただし、wg がポップアップ以外で操作対象とするウィジェットの場合は除く。
 * (メニューバーなど)
 *
 * win: イベントのウィンドウ
 * wg: 対象ウィジェット (NULL でなし) */

static mlkbool _is_popup_exclude(mWindow *win,mWidget *wg)
{
	mAppRunData *run = MLKAPP->run_current;

	return (run
		&& run->type == MAPPRUN_TYPE_POPUP
		&& !(win->wg.ftype & MWIDGET_TYPE_WINDOW_POPUP)
		&& (!run->widget_sub || wg != run->widget_sub));
}

/** [Leave 用] ポップアップ時除外 */

static mlkbool _is_popup_exclude_leave(mWindow *win,mWidget *wg)
{
	if(!_is_popup_exclude(win, wg))
		return FALSE;
	else
	{
		//除外対象のうち、現在の enter ウィジェットのウィンドウであれば除外しない
		//(ポップアップ実行時に enter 状態のウィジェット)

		return !(wg && wg->toplevel == win);
	}
}

/** ポップアップ中に通常ボタンが押された時、ポップアップを終了するか */

static mlkbool _is_popup_press_quit(mWindow *win,int x,int y)
{
	mAppRunData *run = MLKAPP->run_current;

	if(run && run->type == MAPPRUN_TYPE_POPUP)
	{
		if(!(win->wg.ftype & MWIDGET_TYPE_WINDOW_POPUP))
			//ポップアップのウィンドウ以外の上で押されたので、終了
			return TRUE;
		else
		{
			//ポップアップウィンドウ上の場合、位置がウィンドウ外なら終了

			//[X11] 同プログラムのウィンドウ以外の上で押された場合は、
			//  グラブされたウィンドウに対して、範囲外の位置で送られてくる。

			return !mWidgetIsPointIn(MLK_WIDGET(win), x, y);
		}
	}

	return FALSE;
}

/** ポップアップ中、motion 時の判定
 *
 * dst: NULL 以外の場合、win 内にポップアップ以外での操作対象のウィジェットが含まれる場合、
 *      そのウィジェットが入る。
 * return: TRUE でイベントを処理しない */

static mlkbool _is_popup_exclude_motion(mWindow *win,mWidget **dst)
{
	mAppRunData *run = MLKAPP->run_current;

	*dst = NULL;

	if(run && run->type == MAPPRUN_TYPE_POPUP)
	{
		if(win->wg.ftype & MWIDGET_TYPE_WINDOW_POPUP)
			//ポップアップウィンドウなら処理
			return FALSE;
		else
		{
			//win がポップアップウィンドウ以外の場合
			
			if(!run->widget_sub)
				//ポップアップ以外の操作対象ウィジェットがなければ、対象外
				return TRUE;
			else if(run->widget_sub->toplevel == win)
			{
				//win がポップアップ以外の操作対象ウィジェットの親である場合
				
				*dst = run->widget_sub;
				return FALSE;
			}
			else
				return TRUE;
		}
	}

	return FALSE;
}

/** ENTER イベント追加＆カーソル変更
 *
 * fx,fy: ウィジェット座標、固定少数点数 */

static void _widget_enter(mWidget *wg,int fx,int fy)
{
	mEventEnter *ev;

	if(wg && mWidgetIsEnable(wg))
	{
		ev = (mEventEnter *)mEventListAdd(wg, MEVENT_ENTER, sizeof(mEventEnter));
		if(ev)
		{
			ev->x = fx >> 8;
			ev->y = fy >> 8;
			ev->fixed_x = fx;
			ev->fixed_y = fy;
		}

		//カーソル変更

		(MLKAPP->bkend.window_setcursor)(wg->toplevel, wg->cursor);
	}
}

/** LEAVE イベント追加 */

static void _widget_leave(mWidget *wg)
{
	if(wg && mWidgetIsEnable(wg))
		mEventListAdd_base(wg, MEVENT_LEAVE);
}

/** キー処理時、ウィンドウ内の現在のフォーカスウィジェット取得
 * 
 * return: NULL でフォーカスウィジェットなし (win で処理する) */

static mWidget *_get_key_focus_widget(mWindow *win,uint32_t key)
{
	mWidget *wg;

	wg = win->win.focus;
	if(!wg) return NULL;

	if(wg->foption & MWIDGET_OPTION_KEYEVENT_ALL_TO_WINDOW)
		//すべてのキーをウィンドウ側で処理
		wg = NULL;
	else if(key == MKEY_TAB || key == MKEY_KP_TAB)
	{
		//TAB キー: フォーカスウィジェットが対応しないならウィンドウで処理
		if(!(wg->facceptkey & MWIDGET_ACCEPTKEY_TAB))
			wg = NULL;
	}
	else if(key == MKEY_ENTER || key == MKEY_KP_ENTER)
	{
		if(!(wg->facceptkey & MWIDGET_ACCEPTKEY_ENTER))
			wg = NULL;
	}
	else if(key == MKEY_ESCAPE)
	{
		if(!(wg->facceptkey & MWIDGET_ACCEPTKEY_ESCAPE))
			wg = NULL;
	}
	else if(wg->foption & MWIDGET_OPTION_KEYEVENT_NORM_TO_WINDOW)
		//特殊キーを除く通常キーは、ウィンドウ側で処理
		wg = NULL;
	
	return wg;
}

/** ボタン/スクロール時の最初の処理
 *
 * ブロックと、送り先ウィジェット取得
 *
 * is_scroll: TRUE でスクロールイベント、FALSE で通常ボタンイベント
 * return: 送り先のウィジェット (NULL でなし) */

static mWidget *_button_first(mWindow *win,mlkbool is_scroll)
{
	mWidget *wg;

	//ユーザーアクションブロック

	if(MLKAPP->flags & MAPPBASE_FLAGS_BLOCK_USER_ACTION)
		return NULL;

	//スクロールボタン時、モーダル中なら、モーダルウィンドウ以外は対象外

	if(is_scroll && __mEventIsModalSkip(win))
		return NULL;

	//送り先のウィジェット

	if(MLKAPP->widget_grabpt)
		//グラブ中の場合、常にグラブウィジェットへ
		wg = MLKAPP->widget_grabpt;
	else if(MLKAPP->widget_enter)
		//Enter 状態のウィジェットへ
		wg = MLKAPP->widget_enter;
	else
	{
		//非グラブ中で、Enter 状態のウィジェットもない場合

		//[X11]
		// 非グラブ中に、ボタンを押したままウィンドウ外に出た場合は、
		// Leave イベントによって widget_enter が NULL になっている。

		return NULL;
	}

	//対象ウィジェットが有効状態か

	if(!mWidgetIsEnable(wg)) wg = NULL;

	return wg;
}


//================================
// ウィンドウ装飾イベント
//================================
/*
 * - グラブ中は、ポインタが装飾範囲内にあっても MAPPBASE_FLAGS_WINDOW_ENTER_DECO を ON にしない。
 * - 範囲内の場合、window_enter はセットされた状態だが、widget_enter は NULL。
 */


/** イベント追加 */

static void _windeco_send_event(mWindow *win,int type,int x,int y,int btt)
{
	mEventWinDeco *ev;

	ev = (mEventWinDeco *)mEventListAdd(MLK_WIDGET(win), MEVENT_WINDECO,
		sizeof(mEventWinDeco));

	if(ev)
	{
		ev->decotype = type;
		ev->x = x;
		ev->y = y;
		ev->btt = btt;
	}
}

/** 装飾内の位置かどうか判定 */

static mlkbool _windeco_is_in(mWindow *p,int x,int y)
{
	int left,top;

	left = p->win.deco.w.left;
	top  = p->win.deco.w.top;

	return ((x >= 0 && x < left)
		|| (y >= 0 && y < top)
		|| (x >= left + p->wg.w && x < left + p->wg.w + p->win.deco.w.right)
		|| (y >= top + p->wg.h && y < top + p->wg.h + p->win.deco.w.bottom));
}

/** ウィンドウの Enter 時
 *
 * return: TRUE で装飾の範囲内 */

static mlkbool _windeco_on_enter(mWindow *win,int x,int y)
{
	if(!MLKAPP->widget_grabpt
		&& _windeco_is_in(win, x, y))
	{
		MLKAPP->flags |= MAPPBASE_FLAGS_WINDOW_ENTER_DECO;

		_windeco_send_event(win, MEVENT_WINDECO_TYPE_ENTER, x, y, 0);

		return TRUE;
	}

	return FALSE;
}

/** ウィンドウの Leave 時 */

static void _windeco_on_leave(mWindow *win)
{
	if(MLKAPP->flags & MAPPBASE_FLAGS_WINDOW_ENTER_DECO)
	{
		_windeco_send_event(win, MEVENT_WINDECO_TYPE_LEAVE, 0, 0, 0);
	}
}

/** press
 *
 * return: TRUE で処理した */

static mlkbool _windeco_on_press(mWindow *win,int x,int y,int btt)
{
	if(MLKAPP->flags & MAPPBASE_FLAGS_WINDOW_ENTER_DECO)
	{
		_windeco_send_event(win, MEVENT_WINDECO_TYPE_PRESS, x, y, btt);
		return TRUE;
	}

	return FALSE;
}

/** Motion
 *
 * return: 0 で以降の処理は行わない。1,2 で続ける。2 は enter widget を NULL にする */

static int _windeco_on_motion(mWindow *win,int x,int y)
{
	mAppBase *app = MLKAPP;

	if(_windeco_is_in(win, x, y))
	{
		//------- 現在、ポインタが装飾内にある

		if(MLKAPP->widget_grabpt)
		{
			//グラブ中は widget_enter のみ変更
			
			app->widget_enter = NULL;
			return 2;
		}
		else
		{
			//ウィジェット部分から装飾内に入った時

			if(!(app->flags & MAPPBASE_FLAGS_WINDOW_ENTER_DECO))
			{
				//現在の Enter ウィジェットを leave

				if(app->widget_enter)
				{
					_widget_leave(app->widget_enter);

					app->widget_enter = NULL;
				}
			
				//装飾内に Enter

				app->flags |= MAPPBASE_FLAGS_WINDOW_ENTER_DECO;

				_windeco_send_event(win, MEVENT_WINDECO_TYPE_ENTER, x, y, 0);
			}

			//motion

			_windeco_send_event(win, MEVENT_WINDECO_TYPE_MOTION, x, y, 0);

			return 0;
		}
	}
	else
	{
		//------ ポインタが装飾外の位置

		if(!(app->widget_grabpt)
			&& (app->flags & MAPPBASE_FLAGS_WINDOW_ENTER_DECO))
		{
			//装飾内からウィジェット部分へ移動した時、装飾の Leave

			app->flags &= ~MAPPBASE_FLAGS_WINDOW_ENTER_DECO;

			_windeco_send_event(win, MEVENT_WINDECO_TYPE_LEAVE, 0, 0, 0);
		}
	}

	return 1;
}


//================================
// 
//================================


/** モーダルウィンドウ以外のため、イベントをスキップするか
 *
 * モーダル中でないなら常に FALSE。
 * モーダル中の場合、win がモーダルウィンドウなら FALSE、それ以外なら TRUE。 */

mlkbool __mEventIsModalSkip(mWindow *win)
{
	mAppRunData *run = MLKAPP->run_current;

	return (run && run->type == MAPPRUN_TYPE_MODAL && win != run->window);
}

/** ウィジェットのグラブ解除時
 *
 * ボタンが離れた時に、現在のカーソル位置から、Leave/Enter イベントを作成。 */

void __mEventOnUngrab(void)
{
	mWidget *last,*now,*last_win,*now_win;
	int fx,fy;

	//last : グラブ開始時の Enter ウィジェット
	//now  : 現在の Enter ウィジェット

	last = MLKAPP->widget_enter_last;
	now  = MLKAPP->widget_enter;

	//グラブ開始時のウィジェットの上にいるなら、何もしない

	if(last == now) return;

	//各ウィンドウ

	last_win = (last)? MLK_WIDGET(last->toplevel): NULL;
	now_win  = (now)? MLK_WIDGET(now->toplevel): NULL;

	//------ LEAVE

	if(last)
	{
		//ウィジェット

		_widget_leave(last);

		//ウィンドウから離れた
		// last がウィンドウ自身の場合は重複しないようにする

		if(last_win != now_win && last != last_win)
			_widget_leave(last_win);
	}

	//------ ENTER

	if(now)
	{
		fx = MLKAPP->pointer_last_win_fx;
		fy = MLKAPP->pointer_last_win_fy;
		
		//ウィンドウ
		//now がウィンドウ自身の場合は重複しないようにする

		if(last_win != now_win && now != now_win)
			_widget_enter(now_win, fx, fy);

		//ウィジェット

		_widget_enter(now, fx - (now->absX << 8), fy - (now->absY << 8));
	}
}

/** text/uri-list のデータから char* 配列作成 */

char **__mEventGetURIList_ptr(uint8_t *datbuf)
{
	mStr str = MSTR_INIT;
	char **buf,**pp;
	const char *top,*end;
	int num;

	//mStr にデコード

	num = mStrSetDecode_urilist(&str, (const char *)datbuf, TRUE);
	if(num == 0) return NULL;

	//char* の配列を作成

	buf = (char **)mMalloc(sizeof(char *) * (num + 1));
	if(buf)
	{
		top = str.buf;
	
		for(pp = buf; 1; pp++)
		{
			if(!mStringGetNextSplit(&top, &end, '\t')) break;

			*pp = mStrndup(top, end - top);
			if(!(*pp)) break;

			top = end;
		}

		*pp = 0;
	}

	//

	mStrFree(&str);

	return buf;
}


//==========================
// イベント処理
//==========================


/** 閉じるボタンが押された時 */

void __mEventProcClose(mWindow *win)
{
	//モーダル中でモーダルウィンドウ以外の場合や、
	//ユーザーアクションブロック時は除外

	if(__mEventIsModalSkip(win)
		|| (MLKAPP->flags & MAPPBASE_FLAGS_BLOCK_USER_ACTION))
		return;

	//CLOSE イベント

	mEventListAdd_base(MLK_WIDGET(win), MEVENT_CLOSE);
}

/** CONFIGURE イベント処理
 *
 * w,h: 装飾を含むサイズ */

void __mEventProcConfigure(mWindow *win,
	int w,int h,uint32_t state,uint32_t state_mask,uint32_t flags)
{
	//状態変更時 (先に実行)

	if(flags & MEVENT_CONFIGURE_F_STATE)
		__mWindowOnConfigure_state(win, state_mask);

	//サイズ変更時
	
	if(flags & MEVENT_CONFIGURE_F_SIZE)
		__mWindowOnResize_configure(win, w, h);

	//CONFIGURE イベント

	if(flags)
	{
		mEventConfigure *ev;

		ev = (mEventConfigure *)mEventListAdd(MLK_WIDGET(win),
			MEVENT_CONFIGURE, sizeof(mEventConfigure));
		if(ev)
		{
			ev->width = win->wg.w;	//装飾を含まないサイズ
			ev->height = win->wg.h;
			ev->state = state;
			ev->state_mask = state_mask;
			ev->flags = flags;
		}
	}
}

/** フォーカスイン イベント処理 */

void __mEventProcFocusIn(mWindow *win)
{
	mWidget *wg;
	int from;

	//ウィンドウにフォーカスをセット

	MLKAPP->window_focus = win;

	win->wg.fstate |= MWIDGET_STATE_FOCUSED;

	//ウィンドウのフォーカスイベント

	mEventListAdd_focus(MLK_WIDGET(win), FALSE, MEVENT_FOCUS_FROM_WINDOW_FOCUS);

	//ウィジェットにフォーカスをセット

	if(!win->win.focus)
	{
		wg = __mWindowGetDefaultFocusWidget(win, &from);
		if(wg)
			__mWindowSetFocus(win, wg, from);
	}

	//装飾更新 (アクティブ状態変更)

	__mWindowDecoRunUpdate(win, MWINDOW_DECO_UPDATE_TYPE_ACTIVE);
}

/** フォーカスアウト イベント処理 */

void __mEventProcFocusOut(mWindow *win)
{
	mWidget *wg_grab;
	mWindow *popup;
	
	wg_grab = MLKAPP->widget_grabpt;

	//グラブ中に FocusOut した場合、グラブ解除
	//(Alt+Tab などでの強制グラブ解除時)

	if(wg_grab)
		mWidgetUngrabPointer();

	//<ポップアップウィンドウ時>

	//- ポップアップウィンドウの表示開始時、フォーカスは変更されない。
	//- ポップアップ中、Alt+Tab などで強制的にフォーカスが変わった時は、
	//  現在のフォーカスウィンドウに対して FocusOut が来るので、
	//  フォーカスウィンドウがフォーカスアウトしたら、
	//  トップのポップアップもフォーカスアウトしたとみなす。

	popup = _get_popup();

	if(popup && win == MLKAPP->window_focus)
		mEventListAdd_focus(MLK_WIDGET(popup), TRUE, MEVENT_FOCUS_FROM_WINDOW_FOCUS);

	//フォーカスウィジェットのフォーカスを解除

	if(!wg_grab)
		//通常時
		__mWindowSetFocus(win, NULL, MEVENT_FOCUS_FROM_WINDOW_FOCUS);
	else
	{
		//フォーカス変更による強制グラブ解除時
	
		if(wg_grab == win->win.focus)
			//フォーカスウィジェットのグラブ解除時
			//(フォーカスイベントが重複しないようにする)
			__mWindowSetFocus(win, NULL, MEVENT_FOCUS_FROM_FORCE_UNGRAB);
		else
		{
			//フォーカスウィジェットではない場合、
			//グラブウィジェットとフォーカスウィジェットを別々に処理

			mEventListAdd_focus(wg_grab, TRUE, MEVENT_FOCUS_FROM_FORCE_UNGRAB);
			
			__mWindowSetFocus(win, NULL, MEVENT_FOCUS_FROM_WINDOW_FOCUS);
		}
	}
	
	//ウィンドウのフォーカス解除

	MLKAPP->window_focus = NULL;

	win->wg.fstate &= ~MWIDGET_STATE_FOCUSED;

	//ウィンドウのフォーカスアウトイベント

	mEventListAdd_focus(MLK_WIDGET(win), TRUE, MEVENT_FOCUS_FROM_WINDOW_FOCUS);

	//装飾更新 (アクティブ状態変更)

	__mWindowDecoRunUpdate(win, MWINDOW_DECO_UPDATE_TYPE_ACTIVE);
}

/** ウィンドウの Enter イベント処理
 *
 * fx,fy: ウィンドウ座標、固定少数点数 (24:8) */

/* >> ボタンを押しながらポインタを移動した場合
 * 
 * [X11]
 *  X11 のグラブを行っていない場合、
 *  最初にボタンを押したウィンドウに対してのみ Enter/Leave は来るが、
 *  他のウィンドウに対しては Enter/Leave イベントは送信されない。
 *  ボタンが離された時に、他のウィンドウ上にカーソルがある場合、Enter は送られるので、処理する。
 *
 * [Wayland]
 *  完全にグラブ状態となっているので、押している間は Enter/Leave は全く来ない。 */

void __mEventProcEnter(mWindow *win,int fx,int fy)
{
	mAppBase *app = MLKAPP;
	mWidget *wg;
	int wfx,wfy;

	if(win == app->window_enter) return;

	//記録

	app->window_enter = win;
	app->pointer_last_win_fx = fx;
	app->pointer_last_win_fy = fy;

	wfx = fx - (win->win.deco.w.left << 8);
	wfy = fy - (win->win.deco.w.top << 8);

	//カーソル下のウィジェット取得

	wg = mWidgetGetWidget_atPoint(MLK_WIDGET(win), wfx >> 8, wfy >> 8);

	//ポップアップ中の場合は、処理する対象を絞る

	if(!_is_popup_exclude(win, wg))
	{
		//ウィンドウ装飾

		if(_windeco_on_enter(win, fx >> 8, fy >> 8))
			return;

		//ENTER/LEAVE イベント (グラブ中は送らない)

		if(!app->widget_grabpt)
		{
			//ウィンドウの ENTER

			_widget_enter(MLK_WIDGET(win), wfx, wfy);

			//ウィジェットの ENTER
			//wg == win なら重複させない

			if(wg != MLK_WIDGET(win))
				_widget_enter(wg, wfx - (wg->absX << 8), wfy - (wg->absY << 8));
		}
	}

	//Enter 状態のウィジェット (グラブ中も変更する)

	app->widget_enter = wg;
}

/** ウィンドウの Leave イベント処理 */

void __mEventProcLeave(mWindow *win)
{
	mWidget *wg;

	//win が Enter 状態でないなら無視

	if(!win || win != MLKAPP->window_enter)
		return;

	//ENTER/LEAVE イベント
	// :グラブ中は無効。

	if(!MLKAPP->widget_grabpt
		&& !_is_popup_exclude_leave(win, MLKAPP->widget_enter))
	{
		wg = MLKAPP->widget_enter;

		//ウィジェットの LEAVE

		_widget_leave(wg);

		//ウィンドウの LEAVE
		//wg == win の場合は重複させない

		if(wg != MLK_WIDGET(win))
			_widget_leave(MLK_WIDGET(win));

		//ウィンドウ装飾の LEAVE

		_windeco_on_leave(win);
	}

	//

	MLKAPP->window_enter = NULL;
	MLKAPP->widget_enter = NULL;
	MLKAPP->flags &= ~MAPPBASE_FLAGS_WINDOW_ENTER_DECO;
}

/** 現在のポインタ位置から、ENTER を判定
 *
 * [!] グラブやポップアップ中でないこと。
 * 新しく作成したウィジェット上にカーソルがある場合、カーソルを動かさないと ENTER が来ないため、
 * ホイール操作が行えない。 */

void __mEventReEnter(void)
{
	mAppBase *p = MLKAPP;
	mWindow *win;
	mWidget *wg;
	int wfx,wfy;

	win = p->window_enter;
	if(!win) return;

	wfx = p->pointer_last_win_fx - (win->win.deco.w.left << 8);
	wfy = p->pointer_last_win_fy - (win->win.deco.w.top << 8);

	//カーソル下のウィジェット取得

	wg = mWidgetGetWidget_atPoint(MLK_WIDGET(win), wfx >> 8, wfy >> 8);

	//ウィジェットが異なる場合

	if(wg != p->widget_enter)
	{
		_widget_leave(p->widget_enter);
		_widget_enter(wg, wfx - (wg->absX << 8), wfy - (wg->absY << 8));

		p->widget_enter = wg;
	}
}

/** キーイベント処理
 *
 * [!] ユーザーアクションブロックは、この処理の前にあらかじめ行っておくこと。
 *
 * - ウィンドウにアクセラレータやメニューバーが関連付けられている場合、そのキーを処理する。
 * - TAB/Enter/ESC など、ウィンドウ側で処理するキーの場合、処理する。
 * - KEYDOWN/KEYUP イベントを追加。
 * - CHAR/STRING イベントを送る場合は、戻り値のウィジェットに送る。
 *
 * key: XKB のキーコード
 * rawcode: 生のキーコード
 * key_repeat: キーリピートによる押しイベントか
 * return: イベントの送り先ウィジェット。
 *  NULL の場合、イベントは送らない。
 *  返ったウィジェットは常に有効な状態である。 */

mWidget *__mEventProcKey(mWindow *win,
	uint32_t key,uint32_t rawcode,uint32_t state,int press,mlkbool key_repeat)
{
	mWindow *popup;
	mWidget *send_wg,*wg;
	mEventKey *ev;
	uint32_t code;

	//ダイアログ時、ダイアログ以外のウィンドウにフォーカスがある場合、スキップ
	
	if(__mEventIsModalSkip(win)) return NULL;

	//ポップアップ時は常に、ループデータで指定された
	//トップのポップアップウィンドウへ送る

	popup = _get_popup();
	if(popup) win = popup;

	//

	code = (key <= 0xffff)? key: 0;
	
	//----- トップレベル時のキー処理 (押し時 & 非グラブ中のみ)

	if(MLK_WIDGET_IS_TOPLEVEL(win) && press && !MLKAPP->widget_grabpt)
	{
		mToplevel *top = MLK_TOPLEVEL(win);

		//ハンドラ

		if(top->top.keydown
			&& (top->top.keydown)(MLK_WIDGET(top), code, state))
			return NULL;

		//アクセラレータ

		if(top->top.accelerator
			&& __mAcceleratorProcKeyEvent(top->top.accelerator, code, state))
			return NULL;

		//メニューバーのホットキー (Alt+?)

		if(top->top.menubar
			&& __mMenuBarShowHotkey(top->top.menubar, code, state))
			return NULL;
	}

	//-------

	//イベントの送り先ウィジェット取得
	//(フォーカスウィジェット、もしくは NULL)

	send_wg = _get_key_focus_widget(win, key);

	//NULL の場合は、ウィンドウ側で処理

	if(!send_wg)
	{
		send_wg = MLK_WIDGET(win);

		//ウィンドウ側で特殊キー処理 (グラブ中は除く)

		if(!MLKAPP->widget_grabpt)
		{
			if(key == MKEY_TAB || key == MKEY_KP_TAB)
			{
				//[TAB] タブキー有効時、フォーカス移動

				if(MLK_WIDGET_IS_TOPLEVEL(win)
					&& (MLK_TOPLEVEL(win)->top.fstyle & MTOPLEVEL_S_TAB_MOVE))
				{
					if(press)
						__mWindowMoveFocusNext(win);
					
					return NULL;
				}
			}
			else if(key == MKEY_ENTER || key == MKEY_KP_ENTER)
			{
				//[ENTER]
				// 最初に MWIDGET_STATE_ENTER_SEND が ON になっているウィジェットに送る。
				// なければ、ウィンドウ
				
				wg = __mWindowGetStateEnable_first(win, MWIDGET_STATE_ENTER_SEND, FALSE);

				if(wg) send_wg = wg;
			}
		}
	}

	//キーリピート無効時

	if(key_repeat && !(send_wg->fstate & MWIDGET_STATE_ENABLE_KEY_REPEAT))
		return NULL;
	
	//KEYDOWN/UP イベント

	if(send_wg->fevent & MWIDGET_EVENT_KEY)
	{
		ev = (mEventKey *)mEventListAdd(send_wg,
			(press)? MEVENT_KEYDOWN: MEVENT_KEYUP, sizeof(mEventKey));

		if(ev)
		{
			ev->key = code;
			ev->state = state;
			ev->raw_keysym = key;
			ev->raw_code = rawcode;
			ev->is_grab_pointer = (MLKAPP->widget_grabpt != NULL);
		}
	}
		
	return send_wg;
}

/* POINTER/PENTABLET イベント追加 (PRESS/RELEASE) */

static void _addevent_pointer(mWidget *wg,int act,int fx,int fy,
	int btt,int rawbtt,uint32_t state,mlkbool modal_skip,mlkbool add_pentab)
{
	mEvent *ev;

	//POINTER イベント

	if(wg->fevent & MWIDGET_EVENT_POINTER)
	{
		ev = mEventListAdd(wg,
				(modal_skip)? MEVENT_POINTER_MODAL: MEVENT_POINTER,
				sizeof(mEventPointer));

		if(ev)
		{
			
			ev->pt.act = act;
			ev->pt.x = fx >> 8;
			ev->pt.y = fy >> 8;
			ev->pt.fixed_x = fx;
			ev->pt.fixed_y = fy;
			ev->pt.btt = btt;
			ev->pt.raw_btt = rawbtt;
			ev->pt.state = state;
		}
	}

	//PENTABLET イベント
	// :ウィジェットは PENTABLET イベントを受け取るが、
	// :ペンタブレットが使用できない状態の場合、疑似的に追加する。
	
	if(add_pentab && (wg->fevent & MWIDGET_EVENT_PENTABLET))
	{
		ev = mEventListAdd(wg,
				(modal_skip)? MEVENT_PENTABLET_MODAL: MEVENT_PENTABLET,
				sizeof(mEventPenTablet));

		if(ev)
		{
			ev->pentab.act = act;
			ev->pentab.x = fx / 256.0;
			ev->pentab.y = fy / 256.0;
			ev->pentab.btt = btt;
			ev->pentab.raw_btt = rawbtt;
			ev->pentab.state = state;
			ev->pentab.pressure = 1;
		}
	}
}
	
/** スクロールイベント処理 */

void __mEventProcScroll(mWindow *win,int hval,int vval,int hstep,int vstep,uint32_t state)
{
	mWidget *wg;
	mEventScroll *ev;
	int btt;

	//ポップアップ時
	//(ポップアップ以外の対象ウィジェットは無効にする)

	if(_is_popup_exclude(win, NULL)) return;

	//ブロック処理 & 送り先取得

	wg = _button_first(win, TRUE);
	if(!wg) return;

	//イベント

	if(wg->foption & MWIDGET_OPTION_SCROLL_TO_POINTER)
	{
		//---- POINTER

		//垂直

		if(vstep)
		{
			btt = (vstep < 0)? MLK_BTT_SCR_UP: MLK_BTT_SCR_DOWN;

			_addevent_pointer(wg, MEVENT_POINTER_ACT_PRESS, 0, 0, btt, -1, state, FALSE, TRUE);
			_addevent_pointer(wg, MEVENT_POINTER_ACT_RELEASE, 0, 0, btt, -1, state, FALSE, TRUE);
		}

		//水平

		if(hstep)
		{
			btt = (hstep < 0)? MLK_BTT_SCR_LEFT: MLK_BTT_SCR_RIGHT;

			_addevent_pointer(wg, MEVENT_POINTER_ACT_PRESS, 0, 0, btt, -1, state, FALSE, TRUE);
			_addevent_pointer(wg, MEVENT_POINTER_ACT_RELEASE, 0, 0, btt, -1, state, FALSE, TRUE);
		}
	}
	else
	{
		//---- SCROLL
		
		if(wg->fevent & MWIDGET_EVENT_SCROLL)
		{
			ev = (mEventScroll *)mEventListAdd(wg, MEVENT_SCROLL, sizeof(mEventScroll));
			if(ev)
			{
				ev->horz_val = hval;
				ev->vert_val = vval;
				ev->horz_step = hstep;
				ev->vert_step = vstep;
				ev->state = state;
				ev->is_grab_pointer = (MLKAPP->widget_grabpt != 0);
			}
		}
	}
}

/** ボタン押し/離しイベント処理
 *
 * fx,fy: ウィンドウ座標、固定少数点数 (24:8)
 * flags: 処理フラグ
 * return: 送り先ウィジェット (NULL でなし) */

mWidget *__mEventProcButton(mWindow *win,
	int fx,int fy,int btt,int rawbtt,uint32_t state,uint8_t flags)
{
	mWidget *wg;
	int type,press;
	mlkbool modal_skip;

	press = (flags & MEVENTPROC_BUTTON_F_PRESS);
	modal_skip = __mEventIsModalSkip(win);

	//ポップアップ中に、ポップアップ以外の上で押された場合は、
	//ポップアップを終了させる

	if(press && _is_popup_press_quit(win, fx >> 8, fy >> 8))
	{
		mPopupQuit((mPopup *)MLKAPP->run_current->window, TRUE);
		return NULL;
	}
	
	//ウィンドウ装飾部分

	if(btt != MLK_BTT_NONE
		&& press && !modal_skip
		&& _windeco_on_press(win, fx >> 8, fy >> 8, btt))
		return NULL;

	//ブロック & 送り先取得

	wg = _button_first(win, FALSE);
	if(!wg) return NULL;
	
	//処理タイプ
	
	if(flags & MEVENTPROC_BUTTON_F_DBLCLK)
		type = MEVENT_POINTER_ACT_DBLCLK;
	else
		type = (press)? MEVENT_POINTER_ACT_PRESS: MEVENT_POINTER_ACT_RELEASE;

	//ウィジェット座標

	fx -= (win->win.deco.w.left + wg->absX) << 8;
	fy -= (win->win.deco.w.top + wg->absY) << 8;

	//イベント

	_addevent_pointer(wg, type, fx, fy, btt, rawbtt, state,
		modal_skip, ((flags & MEVENTPROC_BUTTON_F_ADD_PENTAB) != 0));

	return wg;
}

/** ポインタ移動イベント処理
 *
 * fx,fy: ウィンドウ座標、固定少数点数
 * add_pentab: PENTABLET イベントを疑似追加 */

mWidget *__mEventProcMotion(mWindow *win,int fx,int fy,uint32_t state,mlkbool add_pentab)
{
	mWidget *wg,*wg_grab,*wg_enter,*wg_popup_send;
	mEvent *ev;
	int ret_outside = 0;

	//ポインタ位置記録

	MLKAPP->pointer_last_win_fx = fx;
	MLKAPP->pointer_last_win_fy = fy;

	//ポップアップ中
	//wg_popup_send : win 内にポップアップ以外の操作対象ウィジェットがある場合、NULL 以外が入る

	if(_is_popup_exclude_motion(win, &wg_popup_send))
		return NULL;

	//ウィンドウ装飾

	if(!wg_popup_send)
	{
		ret_outside = _windeco_on_motion(win, fx >> 8, fy >> 8);
		if(ret_outside == 0) return NULL;

		//2 = グラブ中で装飾内
	}

	//------ 送り先と ENTER/LEAVE

	wg_grab  = MLKAPP->widget_grabpt;
	wg_enter = MLKAPP->widget_enter;

	fx -= win->win.deco.w.left << 8;
	fy -= win->win.deco.w.top << 8;

	//wg = カーソル下のウィジェット

	if(!MLKAPP->window_enter || ret_outside == 2)
	{
		//グラブ中で、装飾内にポインタがある場合、
		//グラブウィジェットへの MOTION イベントは送るが、
		//Enter 状態のウィジェットはなしとする。

		wg = NULL;
	}
	else
	{
		//カーソル下のウィジェット取得
		//[!] ウィンドウ範囲外の場合は、win が返る

		wg = mWidgetGetWidget_atPoint(MLK_WIDGET(win), fx >> 8, fy >> 8);

		//Enter 状態のウィジェット変更時、ENTER/LEAVE イベント追加
		//- グラブ中に関わらず、widget_enter は常に変更する。
		//- グラブ中は、ENTER/LEAVE イベントは送らない。

		if(wg != wg_enter)
		{
			if(wg_popup_send)
			{
				//[ポップアップ中で、ポップアップ以外の操作対象ウィジェットに対する MOTION の場合]

				if(!wg_grab)
				{
					//wg_popup_send の Leave
				
					if(wg_popup_send == wg_enter)
						_widget_leave(wg_enter);

					//wg_popup_send の Enter

					if(wg_popup_send == wg)
						_widget_enter(wg, fx - (wg->absX << 8), fy - (wg->absY << 8));
				}

				MLKAPP->widget_enter = (wg_popup_send == wg)? wg: NULL;
			}
			else
			{
				//[通常時]

				if(!wg_grab)
				{
					_widget_leave(wg_enter);
					_widget_enter(wg, fx - (wg->absX << 8), fy - (wg->absY << 8));
				}

				MLKAPP->widget_enter = wg;
			}
		}
	}

	//MOTION イベント送り先
	//(ポップアップ時、範囲外の MOTION は必要ない)

	if(wg_grab) wg = wg_grab;

	//ポップアップ

	if(wg_popup_send && wg != wg_popup_send)
		return NULL;

	//イベントが送れるか

	if(!wg || !mWidgetIsEnable(wg))
		return NULL;

	//--------- MOTION イベント追加
	
	//ウィジェット座標

	fx -= wg->absX << 8;
	fy -= wg->absY << 8;

	//POINTER イベント

	if(wg->fevent & MWIDGET_EVENT_POINTER)
	{
		ev = mEventListAdd(wg, MEVENT_POINTER, sizeof(mEventPointer));
		if(ev)
		{
			ev->pt.act = MEVENT_POINTER_ACT_MOTION;
			ev->pt.x = fx >> 8;
			ev->pt.y = fy >> 8;
			ev->pt.fixed_x = fx;
			ev->pt.fixed_y = fy;
			ev->pt.state = state;
		}
	}

	//PENTABLET イベント

	if((wg->fevent & MWIDGET_EVENT_PENTABLET)
		&& add_pentab)
	{
		ev = mEventListAdd(wg, MEVENT_PENTABLET, sizeof(mEventPenTablet));
		if(ev)
		{
			ev->pentab.act = MEVENT_POINTER_ACT_MOTION;
			ev->pentab.x = fx / 256.0;
			ev->pentab.y = fy / 256.0;
			ev->pentab.state = state;
			ev->pentab.pressure = 1;
		}
	}

	return wg;
}

/** ペンタブのボタンイベント */

void __mEventProcButton_pentab(mWindow *win,
	double x,double y,double pressure,int btt,int rawbtt,
	uint32_t state,uint32_t evflags,uint8_t flags)
{
	mWidget *wg;
	mEventPenTablet *ev;

	//共通の処理と POINTER イベント

	wg = __mEventProcButton(win,
		floor(x * 256), floor(y * 256),
		btt, rawbtt, state, flags);

	//PENTABLET イベント

	if(wg && (wg->fevent & MWIDGET_EVENT_PENTABLET))
	{
		ev = (mEventPenTablet *)mEventListAdd(wg,
			(__mEventIsModalSkip(win))? MEVENT_PENTABLET_MODAL: MEVENT_PENTABLET,
			sizeof(mEventPenTablet));

		if(ev)
		{
			if(flags & MEVENTPROC_BUTTON_F_DBLCLK)
				ev->act = MEVENT_POINTER_ACT_DBLCLK;
			else if(flags & MEVENTPROC_BUTTON_F_PRESS)
				ev->act = MEVENT_POINTER_ACT_PRESS;
			else
				ev->act = MEVENT_POINTER_ACT_RELEASE;

			ev->x = x - wg->absX;
			ev->y = y - wg->absY;
			ev->btt = btt;
			ev->raw_btt = rawbtt;
			ev->state = state;
			ev->pressure = pressure;
			ev->flags = evflags;
		}
	}
}

/** ペンタブの MOTION イベント */

void __mEventProcMotion_pentab(mWindow *win,double x,double y,
	double pressure,uint32_t state,uint32_t evflags)
{
	mWidget *wg;
	mEventPenTablet *ev;

	//共通の処理と POINTER イベント

	wg = __mEventProcMotion(win,
		floor(x * 256), floor(y * 256), state, FALSE);

	//PENTABLET イベント

	if(wg && (wg->fevent & MWIDGET_EVENT_PENTABLET))
	{
		ev = (mEventPenTablet *)mEventListAdd(wg, MEVENT_PENTABLET,
			sizeof(mEventPenTablet));

		if(ev)
		{
			ev->act = MEVENT_POINTER_ACT_MOTION;
			ev->x = x - wg->absX;
			ev->y = y - wg->absY;
			ev->state = state;
			ev->pressure = pressure;
			ev->flags = evflags;
		}
	}
}
