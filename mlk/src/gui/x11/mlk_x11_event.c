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
 * <X11> イベント処理
 *****************************************/

#include <string.h>

#define MLKX11_INC_ATOM
#define MLKX11_INC_UTIL
#define MLKX11_INC_XI2
#include "mlk_x11.h"
#include "mlk_x11_event.h"

#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_event.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_window.h"
#include "mlk_pv_event.h"

//-----------

//mlk_x11_dnd.c
mlkbool mX11DND_client_message(XClientMessageEvent *ev);

//-----------


//==========================
// 各イベント処理
//==========================


/** フォーカス (FocusIn/FocusOut) */

static void _event_focus(_X11_EVENT *p,mlkbool focusin)
{
	int mode,detail;

	mode = p->xev->xfocus.mode;
	detail = p->xev->xfocus.detail;

	_X11_DEBUG("Focus%s: %p mode(%d) detail(%d)\n",
		(focusin)? "In":"Out", p->win, mode, detail);

	//<ポインタ/キーボードのグラブによるイベントは除外>

	//- Alt+Tab 押し直後に mode=NotifyGrab が来るが、
	//  実際に他のウィンドウにフォーカスが移った時には mode=NotifyNormal が来るので、
	//  他のウィンドウにフォーカスが移った時のみ処理。
    //
	//- バックグラウンドで動作していて、ホットキーが設定されているアプリでは、
	//  キー押し/離し時に NotifyGrab/NotifyUngrab が来る。
	//  ホットキーを処理するアプリ側において、内部でキーの処理だけ行って、
	//  ウィンドウを表示することがない場合、フォーカスウィンドウは変わらないので、
	//  これらのフォーカスイベントは処理しない。
	//
	//- ウィンドウの移動などが開始/終了した時、
	//  detail=NotifyNonlinear で、FocusOut/In が来る。
	//  IM 前編集中などに移動すると、そのまま移動できてしまうので、処理を行う。

	if((mode == NotifyGrab || mode == NotifyUngrab)
		&& detail != NotifyNonlinear)
		return;

	//

	if(focusin)
		__mEventProcFocusIn(p->win);
	else
		__mEventProcFocusOut(p->win);
}

/* ConfigureNotify イベントチェック関数 */

static Bool _check_configure_event(Display *disp,XEvent *ev,XPointer arg)
{
	return (ev->type == ConfigureNotify
		&& ev->xany.window == ((XAnyEvent *)arg)->window);
}

/** ConfigureNotify イベント */

static void _event_configure(_X11_EVENT *p)
{
	XConfigureEvent *xev = (XConfigureEvent *)p->xev;
	XEvent evtmp;
	mWindow *win = p->win;
	mWindowDataX11 *data;
	mRect rc;
	int x,y,w,h,sendevent;

	//表示されていない状態では無効

	if(MLK_WINDOW_IS_UNMAP(win))
		return;

	data = MLKX11_WINDATA(win);

	//---- 同じウィンドウの ConfigureNotify はまとめて処理する
	// [!] キューの状態によっては、後にイベントが続いていても同時に処理できない。

	//サイズのみの変更時、send_event = 0。ウィンドウ位置の変更時、send_event = 1。
	//send_event = 1 の場合のみ、ウィンドウ位置として使用する。
	// send_event = 0: ウィンドウ (枠含む) の左上位置からの相対位置
	// send_event = 1: ルート座標での位置 (枠含まない)

	x = xev->x;
	y = xev->y;
	w = xev->width;
	h = xev->height;
	sendevent = xev->send_event;

	while(XCheckIfEvent(MLKX11_DISPLAY, &evtmp, _check_configure_event, (XPointer)xev))
	{
		w = evtmp.xconfigure.width;
		h = evtmp.xconfigure.height;

		if(evtmp.xconfigure.send_event)
		{
			x = evtmp.xconfigure.x;
			y = evtmp.xconfigure.y;
			sendevent = 1;
		}
	}

	_X11_DEBUG("Configure: %p: %d (%d,%d)-(%dx%d) serial(%d)\n",
		win, sendevent, x, y, w, h, xev->serial);

	//---- 通常ウィンドウ時は、枠を含む位置を常に記録
	//(最大化/フルスクリーン状態で終了した場合の、通常ウィンドウ位置保存用)

	if(sendevent && MLK_WINDOW_IS_NORMAL_SIZE(win))
	{
		mX11WindowGetFrameWidth(win, &rc);
		
		data->norm_x = x - rc.x1;
		data->norm_y = y - rc.y1;
	}

	//---- CONFIGURE イベント (サイズ)

	if(w != win->win.win_width || h != win->win.win_height)
	{
		__mEventProcConfigure(win, w, h, 0, 0, MEVENT_CONFIGURE_F_SIZE);

		//最大化などの状態変化 -> サイズ変化の場合、イベントをまとめる

		mEventListCombineConfigure();
	}
}

/** Enter/Leave イベント */

static void _event_enterleave(_X11_EVENT *p,int enter)
{
	XCrossingEvent *xev = (XCrossingEvent *)p->xev;
	int mode;

	mode = xev->mode;

	_X11_DEBUG("%s: %p: mode(%d) detail(%d)\n",
		(enter)? "Enter": "Leave", p->win, mode, xev->detail);

	//通常の Enter/Leave 時、または Enter + NotifyUngrab 時
	
	//[Enter + NotifyUngrab]
	//  ボタンを押したまま移動している状態で、
	//  押したウィンドウ以外のウィンドウ (同プログラム内) 上でボタンを離した時。

	if(mode == NotifyNormal
		|| (enter && mode == NotifyUngrab))
	{
		if(enter)
			__mEventProcEnter(p->win, xev->x << 8, xev->y << 8);
		else
			__mEventProcLeave(p->win);
	}

	//[Leave + NotifyGrab]
	//  ポップアップグラブ開始時

	if(!enter && mode == NotifyGrab)
	{
		//基本的に、ポップアップウィンドウに対する Enter は来ないので、
		//ポップアップ開始時にポインタがウィンドウ内にあれば、手動で Leave/Enter を送る

		mAppRunData *run = MLKAPP->run_current;

		if(run && run->type == MAPPRUN_TYPE_POPUP)
		{
			mWindow *popup = run->window;
			mPoint pt;

			mX11WindowGetCursorPos(popup, &pt);

			if(mWidgetIsPointIn(MLK_WIDGET(popup), pt.x, pt.y))
			{
				__mEventProcLeave(MLKAPP->window_enter);
				__mEventProcEnter(popup, pt.x << 8, pt.y << 8);
			}
		}
	}
}

/** キーイベント */

static void _event_key(_X11_EVENT *p,int press)
{
	XKeyEvent *xev = (XKeyEvent *)p->xev;
	XEvent evtmp;
	KeySym keysym;
	int str_len,key_repeat;
	char str_char,*str_utf8;
	mWindow *win;
	mWidget *send_wg;
	mEvent *ev;

	win = p->win;

	_X11_DEBUG("Key%s: %p: code(%d) time(%lu)\n",
		(press)?"Press":"Release", win, xev->keycode, xev->time);

	//_NET_WM_USER_TIME

	if(press)
		mX11WindowSetUserTime(win, xev->time);

	//ユーザーアクションブロック

	if(MLKAPP->flags & MAPPBASE_FLAGS_BLOCK_USER_ACTION)
		return;

	//キーリピート処理
	// キーリピートの場合、
	// 同じ keycode & time で KeyRelease -> KeyPress が連続して来る。

	key_repeat = FALSE;

	if(!press && XPending(MLKX11_DISPLAY))
	{
		XPeekEvent(MLKX11_DISPLAY, &evtmp);
		
		if(evtmp.xkey.type == KeyPress
			&& evtmp.xkey.keycode == xev->keycode
			&& evtmp.xkey.time == xev->time)
		{
			//次の KeyPress を取り出し、現在の KeyRelease と置き換える
		
			XNextEvent(MLKX11_DISPLAY, &evtmp);
			xev = &evtmp.xkey;

			press = 1;
		
			key_repeat = TRUE;
		}
	}

	//KeyShm と文字列を取得
	
	keysym = 0;
	str_char = 0;		//ASCII 文字の場合
	str_utf8 = NULL;	//UTF-8 文字列
	str_len = 0;		//文字長さ
	 
	if(press && MLKX11_WINDATA(win)->xic)
	{
		//IM 有効 & 押し時

		str_utf8 = mX11Event_key_getIMString(p, xev, &keysym, &str_char, &str_len);
	}
	else
	{
		//通常時
		//[!] NULL 文字はセットされない場合がある
	
		char m[16];

		str_len = XLookupString(xev, m, 16, &keysym, NULL);
		
		if(str_len == 1 && *m >= 32 && *m < 127)
			str_char = *m;
	}
	
	//イベント処理
	
	send_wg = __mEventProcKey(win, keysym, xev->keycode,
		mX11Event_convertState(xev->state), press, key_repeat);
	
	if(send_wg)
	{
		//CHAR (表示可能な文字のみ)
		
		if(press && str_char
			&& (send_wg->fevent & MWIDGET_EVENT_CHAR))
		{
			ev = mEventListAdd(send_wg, MEVENT_CHAR, sizeof(mEventChar));

			if(ev)
				ev->ch.ch = str_char;
		}
		
		//STRING
		
		if(str_utf8 && (send_wg->fevent & MWIDGET_EVENT_STRING))
		{
			//1byte 余分に確保され、ゼロクリアされるので、終端がヌル文字となる
		
			ev = mEventListAdd(send_wg, MEVENT_STRING, sizeof(mEventString) + str_len);
			if(ev)
			{
				ev->str.len = str_len;

				memcpy(ev->str.text, str_utf8, str_len);
			}
		}
	}
	
	//
	
	mFree(str_utf8);
}

/** ボタン押し/離し (XI2 でない場合) */

static void _event_button(_X11_EVENT *p,mlkbool press)
{
	XButtonEvent *xev = (XButtonEvent *)p->xev;
	mWindow *win = p->win;
	int btt;
	uint32_t state;
	uint8_t flags = 0;

	_X11_DEBUG("Button%s: %p: x(%d) y(%d) btt(%d)\n",
		(press)?"Press":"Release", win, xev->x, xev->y, xev->button);

	//押し時

	if(press)
	{
		mX11WindowSetUserTime(win, xev->time);

		//最後の ButtonPress の状態を記録

		MLKAPPX11->ptpress_rx = xev->x_root;
		MLKAPPX11->ptpress_ry = xev->y_root;
		MLKAPPX11->ptpress_btt = xev->button;

		//ダブルクリック判定

		if(mX11Event_checkDblClk(win, xev->x, xev->y, xev->button, xev->time))
			flags |= MEVENTPROC_BUTTON_F_DBLCLK;
	}

	//装飾キー

	state = mX11Event_convertState(xev->state);

	//スクロールイベント処理
	// + 通常ボタンの場合は、イベントのボタン番号取得

	btt = mX11Event_proc_button(p, xev->button, state, press);

	//通常ボタン処理

	if(btt >= 0)
	{
		if(press)
			flags |= MEVENTPROC_BUTTON_F_PRESS;

		//XI2 が無効の場合は、PENTAB イベントを生成

		if(MLKAPPX11->xi2_opcode == -1)
			flags |= MEVENTPROC_BUTTON_F_ADD_PENTAB;

		//

		__mEventProcButton(win,
			xev->x << 8, xev->y << 8, btt, xev->button,
			state, flags);
	}
}

/** ポインタ移動 */

static void _event_motion(_X11_EVENT *p)
{
	XMotionEvent *xev = (XMotionEvent *)p->xev;

	_X11_DEBUG("motion: %p: x(%d) y(%d)\n",
		p->win, xev->x, xev->y);

	//X11 のグラブを行っていない状態で、ボタンを押したまま移動している場合、Enter/Leave は来る。
	//(その場合、対象は最初にボタンを押したウィンドウのみ。ほかウィンドウ上での Enter は来ない)
	//この時、ボタンを押している間は、ウィンドウ間を移動しても Motion がずっと送られてくる。
	//ポインタが、最初に押されたウィンドウ上を離れて、Leave が来た時、
	// widget_enter が NULL になっている場合は、MOTION イベントは送らないものとする。

	__mEventProcMotion(p->win, xev->x << 8, xev->y << 8,
		mX11Event_convertState(xev->state),
		(MLKAPPX11->xi2_opcode == -1));
}

/** ウィンドウ表示 */

static void _event_map(_X11_EVENT *p)
{
	mWindow *win = p->win;

	_X11_DEBUG("Map: %p: serial(%lu)\n", win, p->xev->xany.serial);
	
	//表示処理
	
	win->win.fstate |= MWINDOW_STATE_MAP;

	mX11Event_map_setRequest(MLKAPPX11, win);

	mEventListAdd_base(MLK_WIDGET(win), MEVENT_MAP);

	//イベント待ち時

	if(p->waitev_check && p->waitev_type == MLKX11_WAITEVENT_MAP)
		p->waitev_flag = TRUE;
}

/** XInput2 イベント
 *
 * ポインタとキーボードのイベントが来る。
 * XI2 が有効なウィンドウでは、通常のポインタイベントは来ない。 */

#if defined(MLK_X11_HAVE_XINPUT2)

static void _event_xinput2(_X11_EVENT *p,XIDeviceEvent *xev)
{
	mAppX11 *app = MLKAPPX11;
	mWindow *win;
	int type,btt;
	uint32_t state,evflags;
	uint8_t flags;
	double pressure;

	type = xev->evtype;

	//時間

	app->evtime_last = xev->time;

	//対象ウィンドウ

	win = mX11Event_getWindow(MLKAPPX11, xev->event);
	if(!win) return;

	//_NET_WM_USER_TIME

	if(type == XI_ButtonPress || type == XI_KeyPress)
		mX11WindowSetUserTime(win, xev->time);

	//キーボードイベントなどは除外

	if(type != XI_Motion
		&& type != XI_ButtonPress && type != XI_ButtonRelease)
		return;

	//押し時の位置とボタンを記録

	if(type == XI_ButtonPress)
	{
		app->ptpress_rx = xev->root_x;
		app->ptpress_ry = xev->root_y;
		app->ptpress_btt = xev->detail;
	}

	//------- ポインタイベント処理

	//筆圧などの情報取得

	evflags = mX11XI2_procEvent(xev, &pressure);

	//装飾フラグ (Lock キーの状態はなし)

	state = mX11Event_convertState(xev->mods.base);

	//

	if(type == XI_Motion)
	{
		//[Motion]

		__mEventProcMotion_pentab(win, xev->event_x, xev->event_y,
			pressure, state, evflags);
	}
	else
	{
		//[Press/Release]

		flags = 0;

		//押し時

		if(type == XI_ButtonPress)
		{
			flags |= MEVENTPROC_BUTTON_F_PRESS;

			//ダブルクリック判定

			if(mX11Event_checkDblClk(win, xev->event_x, xev->event_y, xev->detail, xev->time))
				flags |= MEVENTPROC_BUTTON_F_DBLCLK;
		}

		//スクロールボタンとボタン変換

		btt = mX11Event_proc_button(p, xev->detail, state, (type == XI_ButtonPress));

		//通常ボタン処理

		if(btt >= 0)
		{
			__mEventProcButton_pentab(win, xev->event_x, xev->event_y,
				pressure, btt, xev->detail, state, evflags, flags);
		}
	}
}

#endif


//==========================
//
//==========================


/** イベントを一つ処理 */

static void _event_translate_sub(_X11_EVENT *p)
{
	XEvent *xev = p->xev;
	int type;

	type = xev->xany.type;

	switch(type)
	{
		//更新 (ウィンドウの移動による重なり状態の変更時など)
		case Expose:
		case GraphicsExpose:
			__mWindowUpdateRoot(p->win,
				xev->xexpose.x, xev->xexpose.y,
				xev->xexpose.width, xev->xexpose.height);
			break;

		//ポインタ移動
		case MotionNotify:
			_event_motion(p);
			break;

		//ボタン押し/離し
		case ButtonPress:
		case ButtonRelease:
			_event_button(p, (type == ButtonPress));
			break;
	
		//キー
		case KeyPress:
		case KeyRelease:
			_event_key(p, (type == KeyPress));
			break;

		//ウィンドウの構成変更
		case ConfigureNotify:
			_event_configure(p);
			break;

		//Enter/Leave
		case EnterNotify:
		case LeaveNotify:
			_event_enterleave(p, (type == EnterNotify));
			break;
	
		//フォーカス
		case FocusIn:
		case FocusOut:
			_event_focus(p, (type == FocusIn));
			break;
	
		//プロパティ変更
		case PropertyNotify:
			if(xev->xproperty.atom == MLKAPPX11->atoms[MLKX11_ATOM_NET_WM_STATE])
				mX11Event_property_netwmstate(p);
			break;

		//クライアントメッセージ
		case ClientMessage:
			if(xev->xclient.message_type == MLKAPPX11->atoms[MLKX11_ATOM_WM_PROTOCOLS])
				mX11Event_clientmessage_wmprotocols(p);
			else
				//他に処理するタイプがあれば、戻り値が FALSE の時に判定
				mX11DND_client_message((XClientMessageEvent *)xev);
			break;

		//ウィンドウ表示
		case MapNotify:
			_event_map(p);
			break;
		
		//ウィンドウ非表示
		case UnmapNotify:
			_X11_DEBUG("Unmap: %p\n", p->win);

			p->win->win.fstate &= ~MWINDOW_STATE_MAP;

			mEventListAdd_base(MLK_WIDGET(p->win), MEVENT_UNMAP);
			break;

		//キーボードマッピング
		case MappingNotify:
			if(xev->xmapping.request != MappingPointer)
				XRefreshKeyboardMapping((XMappingEvent *)xev);
			break;
	}
}


//===============================
// main
//===============================


/** 現在ある X イベントをすべて処理して、内部イベントに変換
 *
 * wait_win : 待ちイベントの対象ウィンドウ (必要なければ NULL)
 * wait_evtype : 待ちイベントのタイプ (0 でなし)
 * evdst : 特定の指定イベントを待つ場合、取得したイベント情報が入る
 * return: TRUE で待ちイベントが来た */

mlkbool mX11EventTranslate(mAppX11 *p,mWindow *wait_win,int wait_evtype,XEvent *evdst)
{
	Display *disp = p->display;
	_X11_EVENT dat;
	XEvent xev;
	mWindow *win;

	//_X11_EVENT データ
	
	dat.xev = &xev;
	dat.waitev_type = wait_evtype;
	dat.waitev_flag = FALSE;

	//
	
	while(XPending(disp))
	{
		//X イベント取得
	
		XNextEvent(disp, &xev);
		
		//イベントの時間を記録
		
		mX11Event_setTime(p, &xev);
		
		//IM 用フィルタ

		if(XFilterEvent(&xev, None))
			continue;

		//各イベント

		switch(xev.xany.type)
		{
			//GenericEvent (XInput2)
			case GenericEvent:
			#if defined(MLK_X11_HAVE_XINPUT2)

				if(xev.xcookie.extension == p->xi2_opcode)
				{
					if(XGetEventData(disp, &xev.xcookie))
					{
						_event_xinput2(&dat, (XIDeviceEvent *)xev.xcookie.data);
					
						XFreeEventData(disp, &xev.xcookie);
					}
				}

			#endif
				break;

			//セレクション: 他クライアントからデータが要求された
			case SelectionRequest:
				if(xev.xselectionrequest.selection == p->atoms[MLKX11_ATOM_CLIPBOARD])
					mX11Clipboard_selection_request((XSelectionRequestEvent *)&xev);
				break;

			//セレクション: 他クライアントに対してデータ送信を要求した時の返信
			// (イベント待ちのみ)
			// evdst->xselection.selection に、対応するセレクションを入れておくこと。
			case SelectionNotify:
				if(wait_evtype == MLKX11_WAITEVENT_SELECTION_NOTIFY
					&& evdst && evdst->xselection.selection == xev.xselection.selection)
				{
				#ifdef MLK_DEBUG_PUT_EVENT
					_X11_DEBUG("SelectionNotify: ");
					mX11PutAtomName(xev.xselection.target);
				#endif
					
					*evdst = xev;
					return TRUE;
				}
				break;

			//セレクション: 所有権が解除された (他のクライアントが所有権を持った場合など)
			case SelectionClear:
				if(xev.xselectionclear.selection == p->atoms[MLKX11_ATOM_CLIPBOARD])
				{
					_X11_DEBUG("SelectionClear\n");
				
					mClipboardUnix_freeData(&p->clipb);

					p->flags &= ~MLKX11_FLAGS_HAVE_SELECTION;
				}
				break;

			//通常イベント
			default:
				//mWindow 取得

				win = mX11Event_getWindow(p, xev.xany.window);
				if(!win) continue;

				dat.win = win;
				dat.waitev_check = (win == wait_win);

				_event_translate_sub(&dat);
				break;
		}
	}

	return dat.waitev_flag;
}
