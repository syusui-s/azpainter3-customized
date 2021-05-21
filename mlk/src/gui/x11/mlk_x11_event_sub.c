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
 * <X11> イベント関連のサブ関数
 *****************************************/

#include <stdlib.h>	//abs

#define MLKX11_INC_ATOM
#include "mlk_x11.h"
#include "mlk_x11_event.h"

#include "mlk_widget_def.h"
#include "mlk_event.h"
#include "mlk_charset.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_event.h"



/** ウィンドウ ID から mWindow 検索 */

mWindow *mX11Event_getWindow(mAppX11 *p,Window id)
{
	mWindow *win;

	for(win = MLK_WINDOW(MLKAPP->widget_root->first); win; win = MLK_WINDOW(win->wg.next))
	{
		if(MLKX11_WINDATA(win)->winid == id)
			return win;
	}
	
	return NULL;
}

/** X イベントの時間を記録する
 *
 * ClientMessage などの場合は、それぞれで処理すること。 */

void mX11Event_setTime(mAppX11 *p,XEvent *ev)
{
	Time user = 0, last = 0;

	switch(ev->type)
	{
		case MotionNotify:
			last = ev->xmotion.time;
			break;

		case ButtonPress:
			user = ev->xbutton.time;
		case ButtonRelease:
			last = ev->xbutton.time;
			break;

		case KeyPress:
			user = ev->xkey.time;
		case KeyRelease:
			last = ev->xkey.time;
			break;

		case PropertyNotify:
			last = ev->xproperty.time;
			break;
		case EnterNotify:
		case LeaveNotify:
			last = ev->xcrossing.time;
			break;
		case SelectionClear:
			last = ev->xselectionclear.time;
			break;
	}
	
	if(user) p->evtime_user = user;
	if(last) p->evtime_last = last;
}

/** ポインタ/キーの装飾フラグ取得 */

uint32_t mX11Event_convertState(unsigned int state)
{
	uint32_t s = 0;

	if(state & ShiftMask) s |= MLK_STATE_SHIFT;
	if(state & ControlMask) s |= MLK_STATE_CTRL;
	if(state & Mod1Mask) s |= MLK_STATE_ALT;
	if(state & Mod4Mask) s |= MLK_STATE_LOGO;

	if(state & LockMask) s |= MLK_STATE_CAPS_LOCK;
	if(state & Mod2Mask) s |= MLK_STATE_NUM_LOCK;
	
	return s;
}

/** キーイベント時、IM 用の文字列取得
 *
 * keysym: KEY イベントのキーコード
 * dst_char: CHAR イベントで渡す表示可能文字。NULL でなし
 * return: 2文字以上の文字列の場合、ポインタ。NULL でなし */

char *mX11Event_key_getIMString(_X11_EVENT *p,XKeyEvent *xev,
	KeySym *keysym,char *dst_char,int *dst_len)
{
	XIC xic;
	char m[32],*pc = NULL,*buf = NULL;
	Status ret;
	int len;

	xic = MLKX11_WINDATA(p->win)->xic;
	
	//文字列と KeyShm 取得
	
	len = XmbLookupString(xic, xev, m, 32, keysym, &ret);
	
	if(ret == XBufferOverflow)
	{
		//バッファを超える場合、確保して再取得

		pc = (char *)mMalloc(len);
		if(!pc) return NULL;
		
		len = XmbLookupString(xic, xev, pc, len, keysym, &ret);
	}
	
	//文字列がある場合
	// ret = XLookupKeySym の場合は、KeySym のみ。
	// 文字列は、32文字以下なら m に、それ以上なら pc にセットされている。
	
	if(ret == XLookupChars || ret == XLookupBoth)
	{
		if(len == 1)
		{
			//ASCII 文字が1文字の場合、表示可能文字の場合のみセット。
			//それ以外は KEY イベントとしてのみ処理。

			if(m[0] >= 32 && m[0] < 127)
				*dst_char = m[0];
		}
		else
		{
			//UTF-8 に変換
		
			buf = mLocaletoUTF8((pc)? pc: m, len, &len);
		}
	}
	
	if(pc) mFree(pc);

	*dst_len = len;

	return buf;
}

/** ダブルクリック判定 (ButtonPress 時) */

mlkbool mX11Event_checkDblClk(mWindow *win,
	int x,int y,int btt,Time time)
{
	mAppX11 *app = MLKAPPX11;
	Window id;

	id = MLKX11_WINDATA(win)->winid;

	if(id == app->dbc_window
		&& btt == app->dbc_btt
		&& time < app->dbc_time + 350
		&& abs(x - app->dbc_x) < 3
		&& abs(y - app->dbc_y) < 3)
	{
		//ダブルクリック
		
		app->dbc_window = 0;
		return TRUE;
	}
	else
	{
		//通常クリック
	
		app->dbc_window = id;
		app->dbc_time   = time;
		app->dbc_x      = x;
		app->dbc_y      = y;
		app->dbc_btt    = btt;

		return FALSE;
	}
}

/** ButtonPress/Release 時のボタン処理
 *
 * return: mlk イベント用のボタン番号。-1 で、以降は処理しない */

int mX11Event_proc_button(_X11_EVENT *p,int btt,uint32_t state,int press)
{
	int h,v;

	if(btt >= 4 && btt <= 7)
	{
		//垂直/水平スクロール時

		if(press)
		{
			h = v = 0;

			if(btt == 4)
				v = -1;
			else if(btt == 5)
				v = 1;
			else if(btt == 6)
				h = -1;
			else
				h = 1;

			__mEventProcScroll(p->win, h * 10, v * 10, h, v, state);
		}

		return -1;
	}
	else
	{
		//通常ボタン時

		if(btt == 1)
			return MLK_BTT_LEFT;
		else if(btt == 2)
			return MLK_BTT_MIDDLE;
		else if(btt == 3)
			return MLK_BTT_RIGHT;
		else
			return MLK_BTT_NONE;
	}
}


//==============================
// イベント処理
//==============================


/** ClientMessage : WM_PROTOCOLS */

void mX11Event_clientmessage_wmprotocols(_X11_EVENT *p)
{
	mAppX11 *app = MLKAPPX11;
	XClientMessageEvent *xev,evtmp;
	Atom mestype;
	mWindow *win;

	xev = (XClientMessageEvent *)p->xev;
	mestype = (Atom)xev->data.l[0];
	win = p->win;

	//last 時間更新

	if((Time)xev->data.l[1] > app->evtime_last)
		app->evtime_last = (Time)xev->data.l[1];
	
	//
		
	if(mestype == app->atoms[MLKX11_ATOM_WM_TAKE_FOCUS])
	{
		//[WM_TAKE_FOCUS]
		// 指定ウィンドウにフォーカスをセットする。
		//
		// [!] XSetInputFocus() を行う場合、
		//    一つのダイアログ終了後に続けてダイアログを表示しようとすると
		//    X エラーが出るので、エラーは無効にしている。
		//
		// ダイアログ中でも、常に送られてきたウィンドウにフォーカスをセットしている。
		// ダイアログ以外のウィンドウにフォーカスがある時は、キーやポインタ操作を無効とする。

		if(app->evtime_user == 0)
			app->evtime_user = app->evtime_last;

		_X11_DEBUG("WM_TAKE_FOCUS: %p: serial(%lu)\n", win, xev->serial);

		mX11WindowSetUserTime(win, app->evtime_user);
		
		XSetInputFocus(app->display, MLKX11_WINDATA(win)->winid, RevertToParent, xev->data.l[1]);
		XFlush(app->display);
	}
	else if(mestype == app->atoms[MLKX11_ATOM_NET_WM_PING])
	{
		//[_NET_WM_PING]
		// アプリが応答可能かどうかを判断するため、
		// ウィンドウマネージャから送られてくるので、返答

		evtmp = *xev;
		evtmp.window = app->root_window;
		
		XSendEvent(app->display, evtmp.window, False,
			SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&evtmp);
	}
	else if(mestype == app->atoms[MLKX11_ATOM_WM_DELETE_WINDOW])
	{
		//[WM_DELETE_WINDOW]
		// ウィンドウの閉じるボタンが押された

		mX11WindowSetUserTime(win, app->evtime_last);

		__mEventProcClose(win);
	}
}

/** PropertyNotify : _NET_WM_STATE
 *
 * ウィンドウの状態変更時 */

void mX11Event_property_netwmstate(_X11_EVENT *p)
{
	mAppX11 *app = MLKAPPX11;
	mWindow *win = p->win;
	Atom *atom;
	int i,num;
	uint32_t fnew,flast,state,mask;

	//アトムリスト取得
	//[!] 値がない場合は空の状態として判定する必要があるため、atom = NULL でも続ける
	
	atom = (Atom *)mX11GetProperty32(p->xev->xproperty.window,
			app->atoms[MLKX11_ATOM_NET_WM_STATE], XA_ATOM, &num);

	//fnew に現在の状態をセット

	flast = win->win.fstate;
	fnew  = flast & ~(MWINDOW_STATE_MAXIMIZED | MWINDOW_STATE_FULLSCREEN);

	for(i = 0; i < num; i++)
	{
		if(atom[i] == app->atoms[MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ]
			|| atom[i] == app->atoms[MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT])
			fnew |= MWINDOW_STATE_MAXIMIZED;

		else if(atom[i] == app->atoms[MLKX11_ATOM_NET_WM_STATE_FULLSCREEN])
			fnew |= MWINDOW_STATE_FULLSCREEN;

	/*
		else if(atom[i] == app->atoms[MLKX11_ATOM_NET_WM_STATE_HIDDEN])
			fnew |= MLKX11_WIN_STATE_HIDDEN;

		else if(atom[i] == app->atoms[MLKX11_ATOM_NET_WM_STATE_ABOVE])
			fnew |= MLKX11_WIN_STATE_ABOVE;
	*/
	}

	mFree(atom);

	win->win.fstate = fnew;
	
	_X11_DEBUG("_NET_WM_STATE: %p: state(0x%x) serial(%lu)\n", win, fnew, p->xev->xproperty.serial);

	//状態の変化フラグ

	mask = 0;

	if((flast & MWINDOW_STATE_MAXIMIZED) != (fnew & MWINDOW_STATE_MAXIMIZED))
		mask |= MEVENT_CONFIGURE_STATE_MAXIMIZED;

	if((flast & MWINDOW_STATE_FULLSCREEN) != (fnew & MWINDOW_STATE_FULLSCREEN))
		mask |= MEVENT_CONFIGURE_STATE_FULLSCREEN;

	//状態変化時

	if(mask)
	{
		//CONFIGURE イベント
		
		state = 0;

		if(fnew & MWINDOW_STATE_MAXIMIZED)
			state |= MEVENT_CONFIGURE_STATE_MAXIMIZED;

		if(fnew & MWINDOW_STATE_FULLSCREEN)
			state |= MEVENT_CONFIGURE_STATE_FULLSCREEN;

		__mEventProcConfigure(win, 0, 0, state, mask, MEVENT_CONFIGURE_F_STATE);

		//イベント待ちON (最大化状態の変化時)

		if(mask & MEVENT_CONFIGURE_STATE_MAXIMIZED)
		{
			if(p->waitev_check
				&& p->waitev_type == MLKX11_WAITEVENT_MAXIMIZE)
				p->waitev_flag = TRUE;
		}

		//イベント待ちON (フルスクリーン状態の変化時)

		if(mask & MEVENT_CONFIGURE_STATE_FULLSCREEN)
		{
			if(p->waitev_check
				&& p->waitev_type == MLKX11_WAITEVENT_FULLSCREEN)
				p->waitev_flag = TRUE;
		}
	}
}

/** ウィンドウ表示時に要求状態をセットする */

void mX11Event_map_setRequest(mAppX11 *p,mWindow *win)
{
	mWindowDataX11 *data = MLKX11_WINDATA(win);
	Window xid;
	int f;

	xid = data->winid;
	f = data->fmap_request;

	//位置

	if(f & MLKX11_WIN_MAP_REQUEST_MOVE)
		XMoveWindow(p->display, xid, data->move_x, data->move_y);

	//最小化

	if(f & MLKX11_WIN_MAP_REQUEST_HIDDEN)
		mX11Send_NET_WM_STATE(xid, 1, p->atoms[MLKX11_ATOM_NET_WM_STATE_HIDDEN], 0);

	//最大化

	if(f & MLKX11_WIN_MAP_REQUEST_MAXIMIZE)
	{
		mX11Send_NET_WM_STATE(xid, 1,
			p->atoms[MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ],
			p->atoms[MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT]);
	}
	
	//全画面
	
	if(f & MLKX11_WIN_MAP_REQUEST_FULLSCREEN)
		mX11Send_NET_WM_STATE(xid, 1, p->atoms[MLKX11_ATOM_NET_WM_STATE_FULLSCREEN], 0);

	//モーダル
	
	if(f & MLKX11_WIN_MAP_REQUEST_MODAL)
		mX11Send_NET_WM_STATE(xid, 1, p->atoms[MLKX11_ATOM_NET_WM_STATE_MODAL], 0);
	
	//常に最前面
	
	if(f & MLKX11_WIN_MAP_REQUEST_ABOVE)
		mX11Send_NET_WM_STATE(xid, 1, p->atoms[MLKX11_ATOM_NET_WM_STATE_ABOVE], 0);
	//

	XFlush(p->display);
	
	data->fmap_request = 0;
}
