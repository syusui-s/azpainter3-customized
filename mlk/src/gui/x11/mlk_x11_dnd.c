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
 * <X11> ドラッグ＆ドロップ処理
 *****************************************/

#define MLKX11_INC_ATOM
#include "mlk_x11.h"
#include "mlk_x11_event.h"

#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_event.h"
#include "mlk_string.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_window.h"
#include "mlk_pv_event.h"


//デバッグ用、Atom 名表示

#ifdef MLK_DEBUG_PUT_EVENT
#define _PUT_ATOM_NAME(a); mX11PutAtomName(a);
#else
#define _PUT_ATOM_NAME(a);
#endif

/*-------

[mX11DND]
 win_enter: enter 状態のウィンドウ (leave 時は NULL)
 wg_drop: 現在のカーソル下のドロップ先ウィジェット (NULL でなし)
 have_urilist: enter 時、MIME タイプに "text/uri-list" が存在するか

--------*/


//======================
// sub
//======================


/** XdndEnter 時、MIME タイプチェック */

static void _check_mimetype(mAppX11 *p,XClientMessageEvent *ev)
{
	Atom *buf,urilist;
	int i,num;

	urilist = p->atoms[MLKX11_ATOM_text_uri_list];

#ifdef MLK_DEBUG_PUT_EVENT
	mDebug(">>> MIME type >>>\n");
#endif

	//"text/uri-list" が存在するか

	if(ev->data.l[1] & 1)
	{
		//4つ以上の場合: プロパティから読み込み

		buf = (Atom *)mX11GetProperty32((Window)ev->data.l[0],
			mX11GetAtom("XdndTypeList"), XA_ATOM, &num);

		if(buf)
		{
			for(i = 0; i < num; i++)
			{
				_PUT_ATOM_NAME(buf[i]);

				if(buf[i] == urilist)
					p->dnd.have_urilist = TRUE;
			}
			
			mFree(buf);
		}
	}
	else
	{
		//3つ以下の場合: イベントのデータから

		for(i = 2; i <= 4; i++)
		{
			_PUT_ATOM_NAME((Atom)ev->data.l[i]);

			if((Atom)ev->data.l[i] == urilist)
				p->dnd.have_urilist = TRUE;
		}
	}

#ifdef MLK_DEBUG_PUT_EVENT
	mDebug("<<< MIME type <<<\n");
#endif
}

/** XdndStatus 送信 */

static void _send_status(mAppX11 *p,Window srcid,int flags)
{
	XEvent ev;

	mX11SetEventClientMessage(&ev, srcid, p->atoms[MLKX11_ATOM_XdndStatus]);

	ev.xclient.data.l[0] = MLKX11_WINDATA(p->dnd.win_enter)->winid;
	ev.xclient.data.l[1] = flags;
	//[2][3] は矩形範囲。0 = 移動するたびに XdndPosition を送信させる
	//[4] 受け付けるアクション (0 で受け付けない)
	ev.xclient.data.l[4] = (flags & 1)? p->atoms[MLKX11_ATOM_XdndActionCopy]: 0;

	//[!] 受け付けない場合、フラグは 0 にしないと、
	//    bit0 が OFF でも、受け付けるという状態になる。

	XSendEvent(p->display, srcid, True, NoEventMask, &ev);
}

/** XdndFinished 送信 */

static void _send_finished(mAppX11 *p,Window srcid)
{
	XEvent ev;

	mX11SetEventClientMessage(&ev, srcid, mX11GetAtom("XdndFinished"));

	ev.xclient.data.l[0] = MLKX11_WINDATA(p->dnd.win_enter)->winid;
	ev.xclient.data.l[1] = 1;
	ev.xclient.data.l[2] = p->atoms[MLKX11_ATOM_XdndActionCopy];

	XSendEvent(p->display, srcid, True, NoEventMask, &ev);
}

/** uri-list から、ローカルファイル名のリスト取得 */

static char **_get_filelist(mAppX11 *p,Time timestamp)
{
	void *uribuf;
	char **buf;

	if(mX11GetSelectionData(MLKX11_WINDATA(p->dnd.win_enter)->winid,
		mX11GetAtom("XdndSelection"), p->atoms[MLKX11_ATOM_text_uri_list],
		timestamp, TRUE, &uribuf, NULL))
		return NULL;

	buf = __mEventGetURIList_ptr(uribuf);

	mFree(uribuf);

	return buf;
}


//======================
// イベント処理
//======================


/** [XdndEnter] D&D のカーソルがウィンドウ内に入った */

static void _on_enter(mAppX11 *p,XClientMessageEvent *ev)
{
	_X11_DEBUG("XdndEnter: ver(%d)\n", (uint8_t)(ev->data.l[1] >> 24));

	//

	p->dnd.win_enter = mX11Event_getWindow(p, ev->window);
	p->dnd.have_urilist = 0;

	//MIME タイプチェック

	_check_mimetype(p, ev);
}

/** [XdndPosition]
 * 
 * カーソル位置を元に、その下のターゲットに対して、ドロップが可能かを返す。 */

static void _on_position(mAppX11 *p,XClientMessageEvent *ev)
{
	mWindow *win;
	mWidget *wg;
	mPoint pt;
	int flags;

	_X11_DEBUG("XdndPosition\n");

	//[flags]
	// 0bit: ドロップを受け付ける
	// 1bit: 矩形内移動中も XdndPosition の送信が必要

	flags = 0;
	wg = NULL;
	win = p->dnd.win_enter;

	//モーダル中は、現在のモーダルウィンドウのみ対象

	if(p->dnd.have_urilist
		&& !__mEventIsModalSkip(win))
	{
		//ルート位置から、win における相対位置取得

		pt.x = ((uint32_t)ev->data.l[2] >> 16) & 0xffff;
		pt.y = ev->data.l[2] & 0xffff;

		mX11WindowRootToWinPos(win, &pt);

		//ドロップ先ウィジェット

		wg = __mWindowGetDNDDropTarget(win, pt.x, pt.y);

		flags = (wg)? 3: 0;
	}

	p->dnd.wg_drop = wg;

	//XdndStatus 送信

	_send_status(p, (Window)ev->data.l[0], flags);
}

/** [XdndDrop] ドロップされた時  */

static void _on_drop(mAppX11 *p,XClientMessageEvent *ev)
{
	char **files;
	mEventDropFiles *pev;

	_X11_DEBUG("XdndDrop\n");

	if(p->dnd.wg_drop)
	{
		//urilist から、ローカルファイル名取得

		files = _get_filelist(p, (Time)ev->data.l[2]);

		//イベント

		if(files)
		{
			pev = (mEventDropFiles *)mEventListAdd(p->dnd.wg_drop,
				MEVENT_DROP_FILES, sizeof(mEventDropFiles));

			if(pev)
				pev->files = files;
			else
				mStringFreeArray_tonull(files);
		}
	}

	//終了

	_send_finished(p, (Window)ev->data.l[0]);

	p->dnd.win_enter = NULL;
}


//======================
// メイン
//======================


/** ClientMessage イベント処理
 *
 * return: D&D イベントを処理したか */

mlkbool mX11DND_client_message(XClientMessageEvent *ev)
{
	mAppX11 *p = MLKAPPX11;
	Atom type;

	type = ev->message_type;

	if(type == p->atoms[MLKX11_ATOM_XdndEnter])
	{
		//enter

		_on_enter(p, ev);
	}
	else if(type == p->atoms[MLKX11_ATOM_XdndPosition])
	{
		//position

		if(p->dnd.win_enter)
			_on_position(p, ev);
	}
	else if(type == p->atoms[MLKX11_ATOM_XdndDrop])
	{
		//drop

		if(p->dnd.win_enter)
			_on_drop(p, ev);
	}
	else if(type == p->atoms[MLKX11_ATOM_XdndLeave])
	{
		//leave
		// カーソルがウィンドウから離れた時、
		// または、ドロップを受け付けない状態でボタンが離された時

		_X11_DEBUG("XdndLeave\n");

		p->dnd.win_enter = NULL;
	}
	else
		return FALSE;

	return TRUE;
}
