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

#ifndef MLK_X11_H
#define MLK_X11_H

/*---- include ----*/

#include "mlk_config.h"

#include <X11/Xlib.h>

#ifdef MLKX11_INC_ATOM
#include <X11/Xatom.h>
#endif

#ifdef MLKX11_INC_UTIL
#include <X11/Xutil.h>
#endif

#ifdef MLKX11_INC_LOCALE
#include <X11/Xlocale.h>
#endif

#ifdef MLKX11_INC_XKB
#include <X11/XKBlib.h>
#endif

#ifdef MLKX11_INC_KEYSYM
#include <X11/keysymdef.h>
#endif

#ifdef MLKX11_INC_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif

#if defined(MLKX11_INC_XSHM) && !defined(MLK_X11_NO_XSHM)
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#if defined(MLKX11_INC_XI2) && defined(MLK_X11_HAVE_XINPUT2)
#include <X11/extensions/XInput2.h>
#endif

#include "mlk_gui.h"
#include "mlk_clipboard_unix.h"

#if !defined(MLK_NO_THREAD)
#include "mlk_thread.h"
#endif

/*---- macro ----*/

#define MLKAPPX11           g_mlk_appx11
#define MLKX11_DISPLAY      (g_mlk_appx11->display)
#define MLKX11_ROOTWINDOW   (g_mlk_appx11->root_window)
#define MLKX11_WINDATA(p)   ((mWindowDataX11 *)((p)->win.bkend))

#ifdef MLK_DEBUG_PUT_EVENT
#define _X11_DEBUG(...);  mDebug("-x11: " __VA_ARGS__)
#else
#define _X11_DEBUG(...);
#endif

enum MLKX11_ATOM
{
	MLKX11_ATOM_MLK_SELECTION,
	MLKX11_ATOM_WM_PROTOCOLS,
	MLKX11_ATOM_WM_DELETE_WINDOW,
	MLKX11_ATOM_WM_TAKE_FOCUS,
	MLKX11_ATOM_NET_WM_PING,
	MLKX11_ATOM_NET_WM_USER_TIME,
	MLKX11_ATOM_NET_WM_STATE,
	MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT,
	MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ,
	MLKX11_ATOM_NET_WM_STATE_HIDDEN,
	MLKX11_ATOM_NET_WM_STATE_MODAL,
	MLKX11_ATOM_NET_WM_STATE_ABOVE,
	MLKX11_ATOM_NET_WM_STATE_FULLSCREEN,
	MLKX11_ATOM_UTF8_STRING,
	MLKX11_ATOM_COMPOUND_TEXT,
	MLKX11_ATOM_CLIPBOARD,
	MLKX11_ATOM_TARGETS,
	MLKX11_ATOM_XdndEnter,
	MLKX11_ATOM_XdndPosition,
	MLKX11_ATOM_XdndLeave,
	MLKX11_ATOM_XdndDrop,
	MLKX11_ATOM_XdndStatus,
	MLKX11_ATOM_XdndActionCopy,
	MLKX11_ATOM_text_uri_list,
	MLKX11_ATOM_text_utf8,

	MLKX11_ATOM_NUM
};


/*---- struct ----*/

/** DND */

typedef struct
{
	mWindow *win_enter;
	mWidget *wg_drop;
	uint8_t have_urilist;
}mX11DND;

/** mAppX11 */

typedef struct _mAppX11
{
	mX11DND dnd;
	mClipboardUnix clipb;

	Display	*display;
	Window root_window,	//ルートウィンドウ
		leader_window;	//クライアントリーダー
	Visual *visual;
	Colormap colormap;

	char *startup_id;

	int screen,
		depth,
		display_fd,
		xi2_opcode;

	uint32_t maskR,		//RGB マスク
		maskG,
		maskB;

	uint8_t is_rgb888,	//マスクが XXRRGGBB か
		r_shift_left,
		g_shift_left,
		b_shift_left,
		r_shift_right,	//8 - colbit
		g_shift_right,
		b_shift_right,
		fsupport,		//サーバーがサポートしている機能のフラグ
		flags;

	GC		gc_def;			//イメージ転送などで使う
	Time	evtime_last,	//Xイベントの時間 (最終)
			evtime_user;	//ユーザーが操作した時間
	
	XIM      im_xim;		//input method
	XIMStyle im_style;
	mWindow *im_preeditwin;
	
	Window dbc_window;		//ダブルクリック判定用
	Time dbc_time;
	int dbc_x,dbc_y,dbc_btt;

	int ptpress_rx,		//最後の ButtonPress イベント時のルート位置とボタン
		ptpress_ry,		//(ウィンドウのドラッグ移動/リサイズ時に使う)	
		ptpress_btt;
		
	mList list_xidev;		//XI2 デバイスリスト
	
	Atom atoms[MLKX11_ATOM_NUM];

#if !defined(MLK_NO_THREAD)
	mThreadMutex mutex;
	int thread_fd[2];
#endif
}mAppX11;

extern mAppX11 *g_mlk_appx11;

/** mWindowDataX11 */

typedef struct
{
	Window winid,		//ウィンドウ XID
		usertime_winid;
	XIC xic;			//IM
	mCursor cursor;		//ウィンドウにセットされている現在のカーソル
	uint32_t eventmask;	//現在のイベントマスク

	int norm_x, norm_y,		//通常ウィンドウ時のウィンドウ位置
		move_x, move_y;		//MAP 時に移動する位置
	uint8_t fmap_request;	//ウィンドウ表示時に実行する処理のフラグ
}mWindowDataX11;


/*---- enum ----*/

enum MLKX11_SUPPORT_FLAGS
{
	MLKX11_SUPPORT_XSHM     = 1<<0,
	MLKX11_SUPPORT_USERTIME = 1<<1
};

enum MLKX11_FLAGS
{
	MLKX11_FLAGS_STARTUP_NOTIFY = 1<<0,	//起動時に StartupNotify を通知したら ON
	MLKX11_FLAGS_HAVE_SELECTION = 1<<1	//セレクションの所有権を持っている
};

enum MLKX11_WIN_MAP_REQUEST_FLAGS
{
	MLKX11_WIN_MAP_REQUEST_MOVE     = 1<<0,
	MLKX11_WIN_MAP_REQUEST_MAXIMIZE = 1<<1,
	MLKX11_WIN_MAP_REQUEST_HIDDEN   = 1<<2,
	MLKX11_WIN_MAP_REQUEST_FULLSCREEN = 1<<3,
	MLKX11_WIN_MAP_REQUEST_MODAL    = 1<<4,
	MLKX11_WIN_MAP_REQUEST_ABOVE    = 1<<5
};

//待ちイベントタイプ
enum
{
	MLKX11_WAITEVENT_NONE,
	MLKX11_WAITEVENT_MAP,
	MLKX11_WAITEVENT_MAXIMIZE,
	MLKX11_WAITEVENT_FULLSCREEN,
	MLKX11_WAITEVENT_SELECTION_NOTIFY
};


/*------------ func --------------*/

//mlk_x11_main.c

int mX11WaitEvent(mAppX11 *p);
void mX11EventTranslate_wait(mWindow *wait_win,int wait_evtype);
void mX11EventTranslate_waitEvent(int wait_evtype,XEvent *evdst);
void mX11GetDesktopBox(mBox *box);
void mX11GetDesktopWorkArea(mBox *box);

//mlk_x11_event.c

mlkbool mX11EventTranslate(mAppX11 *p,mWindow *wait_win,int wait_evtype,XEvent *evdst);

//mlk_x11_window.c

void mX11WidgetGetRootPos(mWidget *p,mPoint *pt);

void mX11ToplevelSetDecoration(mToplevel *p);

void mX11WindowSetEventMask(mWindow *p,long mask,mlkbool append);
void mX11WindowSetUserTime(mWindow *p,unsigned long time);

void mX11WindowGetCursorPos(mWindow *p,mPoint *pt);
void mX11WindowGetRootPos(mWindow *p,mPoint *pt);
void mX11WindowRootToWinPos(mWindow *p,mPoint *pt);
void mX11WindowGetFramePos(mWindow *p,mPoint *pt);
void mX11WindowGetFrameWidth(mWindow *p,mRect *rc);
void mX11WindowGetFullSize(mWindow *p,mSize *size);
void mX11WindowAdjustPosDesktop(mWindow *p,mPoint *pt,mlkbool workarea);

//mlk_x11_dnd.c

mlkbool mX11DND_client_message(XClientMessageEvent *ev);

//mlk_x11_clipboard.c

void mX11Clipboard_selection_request(XSelectionRequestEvent *ev);

//mlk_x11_xinput2.c

int mX11XI2_init(void);
void mX11XI2_selectEvent(Window id);
uint32_t mX11XI2_procEvent(void *event,double *dst_pressure);

//mlk_x11_inputmethod.c

void mX11IM_init(mAppX11 *p);
void mX11IM_close(mAppX11 *p);

void mX11IM_context_destroy(mWindow *win);
void mX11IM_context_create(mWindow *win);
void mX11IM_context_onFocus(mWidget *wg,mlkbool focusout);

//mlk_x11_util.c

Atom mX11GetAtom(const char *name);
void mX11PutAtomName(Atom atom);

void mX11SetProperty32(Window id,Atom prop,Atom proptype,void *val,int num);
void mX11SetPropertyCARDINAL(Window id,Atom prop,void *val,int num);
void mX11SetPropertyAtom(Window id,Atom prop,Atom *atoms,int num);
void mX11SetProperty8(Window id,Atom prop,Atom type,const void *buf,long size,mlkbool append);
void mX11SetPropertyCompoundText(Window id,Atom prop,const char *text,int len);
void mX11SetPropertyWindowType(Window id,const char *name);
void mX11SetProperty_wm_pid(Window id);
void mX11SetProperty_wm_client_leader(Window id);

mlkbool mX11GetProperty32Array(Window id,Atom prop,Atom proptype,void *buf,int num);
void *mX11GetProperty32(Window id,Atom prop,Atom proptype,int *pnum);
void *mX11GetProperty8(Window id,Atom prop,mlkbool append_null,uint32_t *psize);

void mX11SetEventClientMessage(XEvent *ev,Window win,Atom mestype);
void mX11SendClientMessageToRoot(Window id,Atom mestype,void *data,int num);
void mX11SendClientMessageToRoot_string(const char *mestype,const char *mestype_begin,const char *sendstr);

void mX11StartupGetID(mAppX11 *p);
void mX11SendStartupNotify_complete(mAppX11 *p);

int mX11GetEventTimeout(int evtype,int timeout_ms,XEvent *ev);

void mX11Send_NET_WM_STATE(Window id,int action,Atom data1,Atom data2);

int mX11SelectionConvert(Window id,Atom selection,Atom target,Time timestamp,XEvent *dstev);
int mX11GetSelectionTargetAtoms(Window id,Atom selection,Atom **dst);
int mX11GetSelectionData(Window id,Atom selection,Atom target,Time timestamp,mlkbool append_null,void **ppbuf,uint32_t *psize);
int mX11GetSelectionCompoundText(Window id,Atom selection,char **dst,int *dstlen);

#endif
