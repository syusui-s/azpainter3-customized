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

/****************************************
 * <X11> ユーティリティ関数
 ****************************************/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#define MLKX11_INC_ATOM
#define MLKX11_INC_UTIL
#include "mlk_x11.h"

#include "mlk_nanotime.h"
#include "mlk_charset.h"
#include "mlk_str.h"



/** 名前からアトム識別子取得 */

Atom mX11GetAtom(const char *name)
{
	return XInternAtom(MLKX11_DISPLAY, name, False);
}

/** アトム値から名前を取得してデバッグ表示 */

void mX11PutAtomName(Atom atom)
{
	char *name;

	name = XGetAtomName(MLKX11_DISPLAY, atom);
	if(name)
	{
		mDebug("%s\n", name);
		XFree(name);
	}
}


//===================================
// プロパティにデータセット
//===================================


/** プロパティに複数の long 値をセット (タイプ指定) */

void mX11SetProperty32(Window id,Atom prop,Atom proptype,void *val,int num)
{
	XChangeProperty(MLKX11_DISPLAY, id, prop, proptype, 32,
		PropModeReplace, (unsigned char *)val, num);
}

/** プロパティに複数の long 値をセット (type=CARDINAL) */

void mX11SetPropertyCARDINAL(Window id,Atom prop,void *val,int num)
{
	XChangeProperty(MLKX11_DISPLAY, id, prop, XA_CARDINAL, 32,
		PropModeReplace, (unsigned char *)val, num);
}

/** プロパティに複数の Atom 値をセット */

void mX11SetPropertyAtom(Window id,Atom prop,Atom *atoms,int num)
{
	XChangeProperty(MLKX11_DISPLAY, id, prop, XA_ATOM, 32,
		PropModeReplace, (unsigned char *)atoms, num);
}

/** プロパティに format=8 のデータをセット
 *
 * append: TRUE で追加。FALSE で置換。 */

void mX11SetProperty8(Window id,Atom prop,Atom type,
	const void *buf,long size,mlkbool append)
{
	Display *disp = MLKX11_DISPLAY;
	int mode;
	uint8_t *ps = (uint8_t *)buf;
	long maxsize,send;

	maxsize = XMaxRequestSize(disp) * 4;
	mode = (append)? PropModeAppend: PropModeReplace;

	while(size)
	{
		send = (size < maxsize)? size: maxsize;

		XChangeProperty(disp, id, prop, type, 8, mode, ps, send);

		mode = PropModeAppend;
		ps += send;
		size -= send;
	}
}

/** プロパティに COMPOUND_TEXT 文字列セット
 *
 * text: UTF-8 文字列 (NULL で空文字列)
 * len : 文字数 (0 以下で空文字列扱い。常に指定すること) */

void mX11SetPropertyCompoundText(Window id,Atom prop,const char *text,int len)
{
	mAppX11 *p = MLKAPPX11;
	XTextProperty tp;
	char *buf;

	if(!text || len <= 0)
	{
		//空にする
		
		XChangeProperty(p->display, id, prop,
			p->atoms[MLKX11_ATOM_COMPOUND_TEXT], 8, PropModeReplace, NULL, 0);
	}
	else
	{
		buf = mUTF8toLocale(text, len, NULL);
		if(!buf) return;

		if(XmbTextListToTextProperty(p->display, &buf, 1, XCompoundTextStyle, &tp) == Success)
		{
			XSetTextProperty(p->display, id, &tp, prop);

			XFree(tp.value);
		}

		mFree(buf);
	}
}

/** ウィンドウタイプセット */

void mX11SetPropertyWindowType(Window id,const char *name)
{
	Atom type = mX11GetAtom(name);
	
	mX11SetProperty32(id, mX11GetAtom("_NET_WM_WINDOW_TYPE"), XA_ATOM, &type, 1);
}

/** _NET_WM_PID セット */

void mX11SetProperty_wm_pid(Window id)
{
	long pid = getpid();
	
	mX11SetPropertyCARDINAL(id, mX11GetAtom("_NET_WM_PID"), &pid, 1);
}

/** WM_CLIENT_LEADER セット */

void mX11SetProperty_wm_client_leader(Window id)
{
	Window leader = MLKAPPX11->leader_window;

	mX11SetProperty32(id, mX11GetAtom("WM_CLIENT_LEADER"), XA_WINDOW, &leader, 1);
}


//===================================
// プロパティデータ読み込み
//===================================


/** プロパティから long の配列を指定数読み込み */

mlkbool mX11GetProperty32Array(Window id,Atom prop,Atom proptype,void *buf,int num)
{
	Atom type;
	int format;
	unsigned long pnum,after;
	unsigned char *pdat;
	mlkbool ret = FALSE;

	if(XGetWindowProperty(MLKX11_DISPLAY, id, prop, 0, 1024, False, proptype,
			&type, &format, &pnum, &after, &pdat) == Success)
	{
		if(type == proptype && format == 32 && (int)pnum >= num)
		{
			memcpy(buf, pdat, sizeof(long) * num);
			ret = TRUE;
		}

		XFree(pdat);
	}

	return ret;
}

/** プロパティから long データをすべて読み込み
 *
 * pnum: 読み込んだ個数が入る
 * return: 確保されたバッファ。mFree() で解放する。 */

void *mX11GetProperty32(Window id,Atom prop,Atom proptype,int *pnum)
{
	Atom type;
	int format;
	unsigned long num,after;
	unsigned char *pdat;
	void *buf = NULL;

	*pnum = 0;

	if(prop == 0) return NULL;

	//ウィンドウの状態が変化した時の値を取得する際などに使われる。
	//データサイズを確認してから読み込むと、
	// サイズ確認時と実際の読み込み時で値が変わる場合がある (個数が変化するなど) ため、
	// 一度の関数で読み込むべきである。

	if(XGetWindowProperty(MLKX11_DISPLAY, id, prop, 0, 1024,
		0, proptype, &type, &format, &num, &after, &pdat) == Success)
	{
		if(format == 32 && type == proptype && num)
		{
			buf = mMalloc(sizeof(long) * num);
			if(buf)
				memcpy(buf, pdat, sizeof(long) * num);

			*pnum = num;
		}

		XFree(pdat);
	}

	return buf;
}

/** プロパティから format=8 データ取得
 * 
 * append_null: データの最後に 0 (1byte) を追加する
 * psize: NULL 以外で、データのサイズが入る (追加した 0 も含む)
 * return: 確保されたバッファ */

void *mX11GetProperty8(Window id,Atom prop,mlkbool append_null,uint32_t *psize)
{
	Atom type;
	int ret,format;
	unsigned long nitems,after,size;
	long offset;
	unsigned char *pdat,*buf;

	if(prop == 0) return NULL;

	//データサイズ取得

	ret = XGetWindowProperty(MLKX11_DISPLAY, id, prop, 0, 0,
			False, AnyPropertyType, &type, &format, &nitems, &after, &pdat);

	if(pdat) XFree(pdat);

	if(ret != Success || type == None || format != 8)
		return NULL;

	if(after == 0 && !append_null) return NULL;

	//確保
	
	size = after;
	if(append_null) size++;
	
	buf = (unsigned char *)mMalloc(size);
	if(!buf) return NULL;
	
	if(psize) *psize = size;
	
	//読み込み
	//オフセットと数は、常に 4byte 単位で指定する。

	for(offset = 0; after > 0; offset += nitems)
	{
		ret = XGetWindowProperty(MLKX11_DISPLAY, id, prop,
				offset / 4, after / 4 + 1,
				False, AnyPropertyType, &type, &format, &nitems, &after, &pdat);

		if(ret != Success)
		{
			mFree(buf);
			return NULL;
		}

		memcpy(buf + offset, pdat, nitems);
		
		XFree(pdat);
	}
	
	//NULL 追加
	
	if(append_null)
		buf[size - 1] = 0;

	return (void *)buf;
}


//==============================
// イベント関連
//==============================


/** ClientMessage 用にイベント構造体をセット */

void mX11SetEventClientMessage(XEvent *ev,Window win,Atom mestype)
{
	memset(ev, 0, sizeof(XEvent));

	ev->xclient.type         = ClientMessage;
	ev->xclient.display      = MLKX11_DISPLAY;
	ev->xclient.message_type = mestype;
	ev->xclient.format       = 32;
	ev->xclient.window       = win;
}

/** ルートウィンドウに ClientMessage イベントを送る (format=32)
 * 
 * data: long 値の配列
 * num : data の個数。最大5 */

void mX11SendClientMessageToRoot(Window id,Atom mestype,void *data,int num)
{
	XEvent ev;
	int i;
	
	if(num > 5) num = 5;

	mX11SetEventClientMessage(&ev, id, mestype);

	for(i = 0; i < num; i++)
		ev.xclient.data.l[i] = *((long *)data + i);

	XSendEvent(MLKX11_DISPLAY, MLKX11_ROOTWINDOW, False,
		SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

/** ルートウィンドウに、ClientMessage を使って文字列を送信 (StartupNotify 用) */

void mX11SendClientMessageToRoot_string(const char *mestype,
	const char *mestype_begin,const char *sendstr)
{
	Display *disp = MLKX11_DISPLAY;
	XSetWindowAttributes attrs;
	XClientMessageEvent ev;
	Window win,rootwin;
	Atom atom_type,atom_type_begin;
	const char *ps;
	uint8_t *dst;
	int len,size;

	if(!sendstr) return;

	rootwin = MLKX11_ROOTWINDOW;

	//送信用ウィンドウ作成

	attrs.override_redirect = True;
	attrs.event_mask = PropertyChangeMask | StructureNotifyMask;

	win = XCreateWindow(disp, rootwin,
		 -100, -100, 1, 1, 0,
		 CopyFromParent, CopyFromParent, CopyFromParent,
		 CWOverrideRedirect | CWEventMask,
		 &attrs);

	if(!win) return;

	//

	atom_type = mX11GetAtom(mestype);
	atom_type_begin = mX11GetAtom(mestype_begin);

	memset(&ev, 0, sizeof(XClientMessageEvent));

	ev.type = ClientMessage;
	ev.message_type = atom_type_begin;
	ev.display = disp;
	ev.window = win;
	ev.format = 8;

	len = strlen(sendstr) + 1;

	//送信
	//data 部分は、32bit OS だと long(4byte) x 5 = 20byte あるので、
	//その部分を使って 20byte ごとにデータを送る。

	ps = sendstr;
	dst = (uint8_t *)ev.data.b;

	while(len)
	{
		size = (len < 20)? len: 20;
		
		memcpy(dst, ps, size);

		if(size < 20)
			memset(dst + size, 0, 20 - size);

		XSendEvent(disp, rootwin, False, PropertyChangeMask, (XEvent *)&ev);

		ev.message_type = atom_type;

		ps += size;
		len -= size;
	}

	//終了

	XDestroyWindow(disp, win);
	XFlush(disp);
}

/** 指定イベントを受け取る (タイムアウト付き)
 *
 * evtype: 受け取るイベント
 * timeout_ms: タイムアウト時間 (ms)
 * return: 0 で成功。1 でタイムアウト。-1 でエラー。 */

int mX11GetEventTimeout(int evtype,int timeout_ms,XEvent *ev)
{
	struct pollfd fds;
	int ret;
	mNanoTime nt_now,nt_end,nt_diff;

	fds.fd = MLKAPPX11->display_fd;
	fds.events = POLLIN;

	//タイムアウト終了時間

	mNanoTimeGet(&nt_end);
	mNanoTimeAdd_ms(&nt_end, timeout_ms);

	//

	while(1)
	{
		//キュー内にあるか

		if(XCheckTypedEvent(MLKX11_DISPLAY, evtype, ev))
			break;

		//タイムアウトまでの時間

		mNanoTimeGet(&nt_now);

		if(!mNanoTimeSub(&nt_diff, &nt_end, &nt_now))
			return 1;

		//イベントを受け取るまで待つ

		ret = poll(&fds, 1, mNanoTimeToMilliSec(&nt_diff));

		if(ret == 0)
			return 1;
		else if(ret == -1 && errno != EINTR)
			return -1;
	}

	return 0;
}


//==============================
// STARTUP
//==============================


/** 初期化時、STARTUP_ID 取得 */

void mX11StartupGetID(mAppX11 *p)
{
	char *env,*pc,*end;
	Time time;

	//環境変数から ID 文字列取得

	env = getenv("DESKTOP_STARTUP_ID");

	if(!env || env[0] == 0) return;

	p->startup_id = mStrdup(env);

	unsetenv("DESKTOP_STARTUP_ID");

	//_TIME から usertime セット

	pc = strstr(env, "_TIME");
	if(pc)
	{
		pc += 5;
		
		time = strtoul(pc, &end, 0);
		if(pc != end)
			p->evtime_user = time;
	}

	//クライアントリーダーに _NET_STARTUP_ID セット

	XChangeProperty(p->display, p->leader_window,
		mX11GetAtom("_NET_STARTUP_ID"),
		p->atoms[MLKX11_ATOM_UTF8_STRING], 8, PropModeReplace,
		(unsigned char *)env, strlen(env));
}

/** StartupNotify 通知 (起動完了を示す) */

void mX11SendStartupNotify_complete(mAppX11 *p)
{
	char *pc;
	mStr str = MSTR_INIT;

	if(!p->startup_id) return;

	//送信文字列

	mStrSetText(&str, "remove: ID=\"");

	for(pc = p->startup_id; *pc; pc++)
	{
		if(*pc == ' ' || *pc == '"' || *pc == '\\')
			mStrAppendChar(&str, '\\');

		mStrAppendChar(&str, *pc);
	}

	mStrAppendChar(&str, '"');

	//ID 文字列解放

	mFree(p->startup_id);
	p->startup_id = NULL;

	//送信
	
	mX11SendClientMessageToRoot_string(
		"_NET_STARTUP_INFO", "_NET_STARTUP_INFO_BEGIN", str.buf);
	
	mStrFree(&str);
}


//==============================
// ウィンドウマネージャ関連
//==============================


/** _NET_WM_STATE を送る
 *
 * action: [0]はずす [1]追加 [2]トグル */

void mX11Send_NET_WM_STATE(Window id,int action,Atom data1,Atom data2)
{
	long dat[3];
	
	dat[0] = action;
	dat[1] = data1;
	dat[2] = data2;

    mX11SendClientMessageToRoot(id,
		MLKAPPX11->atoms[MLKX11_ATOM_NET_WM_STATE], dat, 3);
}


//=================================
// セレクション関連
//=================================


/** セレクションの変換要求を行い、SelectionNotify イベント取得
 * 
 * id のウィンドウのプロパティ "_MLK_SELECTION" 上にデータが変換される。
 * 
 * dstev: NULL でイベントを取得しない
 * return: [0] 成功 [-1] 所有者なし [1] 失敗 */

int mX11SelectionConvert(Window id,Atom selection,Atom target,Time timestamp,XEvent *dstev)
{
	XEvent xev;

	//所有者がいない

	if(XGetSelectionOwner(MLKX11_DISPLAY, selection) == None)
		return -1;

	//変換要求

	XConvertSelection(MLKX11_DISPLAY, selection, target,
		MLKAPPX11->atoms[MLKX11_ATOM_MLK_SELECTION], id, timestamp);

	//SelectionNotify イベントが来るまで処理

	xev.xselection.selection = selection;

	mX11EventTranslate_waitEvent(MLKX11_WAITEVENT_SELECTION_NOTIFY, &xev);

	if(dstev) *dstev = xev;

	//property = 0 で失敗、または所有者なし
	return (xev.xselection.property == 0)? 1: 0;
}

/** セレクションで利用可能なデータのタイプのアトムリスト取得
 *
 * dst: 確保されたバッファポインタが入る
 * return: リスト数 (0 でなし) */

int mX11GetSelectionTargetAtoms(Window id,Atom selection,Atom **dst)
{
	XEvent ev;
	Atom *buf;
	int num;
	
	//変換

	if(mX11SelectionConvert(id, selection, MLKAPPX11->atoms[MLKX11_ATOM_TARGETS],
		CurrentTime, &ev))
		return 0;

	//アトムリスト取得
	
	buf = (Atom *)mX11GetProperty32(ev.xselection.requestor,
		ev.xselection.property, XA_ATOM, &num);

	if(!buf) return 0;

	*dst = buf;

	return num;
}

/** セレクションからデータ取得 (format=8)
 *
 * return: mX11SelectionConvert と同じ。 [2] データ取得エラー */

int mX11GetSelectionData(Window id,Atom selection,Atom target,Time timestamp,
	mlkbool append_null,void **ppbuf,uint32_t *psize)
{
	void *buf;
	Atom prop;
	int ret;

	//変換

	ret = mX11SelectionConvert(id, selection, target, timestamp, NULL);
	if(ret) return ret;
	
	//取得

	prop = MLKAPPX11->atoms[MLKX11_ATOM_MLK_SELECTION];
	
	buf = mX11GetProperty8(id, prop, append_null, psize);
	if(!buf) return 2;

	//プロパティ削除

	XDeleteProperty(MLKX11_DISPLAY, id, prop);

	*ppbuf = buf;

	return 0;
}

/** セレクションから COMPOUND_TEXT 文字列を UTF-8 に変換して取得
 *
 * dstlen: NULL 以外で、文字数が入る
 * return: mX11SelectionConvert と同じ。 [2] データ変換エラー */

int mX11GetSelectionCompoundText(Window id,Atom selection,char **dst,int *dstlen)
{
	mAppX11 *p = MLKAPPX11;
	XTextProperty tp;
	int ret,num;
	char **list,*buf = NULL;
	
	//変換

	ret = mX11SelectionConvert(id, selection, p->atoms[MLKX11_ATOM_COMPOUND_TEXT], CurrentTime, NULL);
	if(ret) return ret;

	//文字列リストデータ取得

	XGetTextProperty(p->display, id, &tp, p->atoms[MLKX11_ATOM_MLK_SELECTION]);

	if(tp.format != 0)
	{
		if(XmbTextPropertyToTextList(p->display, &tp, &list, &num) == Success)
		{
			buf = mLocaletoUTF8(*list, -1, dstlen);

			XFreeStringList(list);
		}

		XFree(tp.value);
	}
	
	//プロパティ削除

	XDeleteProperty(p->display, id, p->atoms[MLKX11_ATOM_MLK_SELECTION]);

	*dst = buf;

	return (buf)? 0: 2;
}
