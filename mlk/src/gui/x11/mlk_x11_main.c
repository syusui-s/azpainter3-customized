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
 * <X11> メイン
 *****************************************/

#include <stdio.h>   //snprintf,fprintf
#include <string.h>
#include <unistd.h>  //pipe
#include <fcntl.h>   //fcntl
#include <poll.h>
#include <errno.h>

#define MLKX11_INC_ATOM
#define MLKX11_INC_UTIL
#define MLKX11_INC_LOCALE
#define MLKX11_INC_XSHM
#define MLKX11_INC_XKB
#define MLKX11_INC_KEYSYM
#include "mlk_x11.h"
#include "mlk_pv_gui.h"

#include "mlk_widget_def.h"
#include "mlk_event.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_util.h"

//-------------------

mAppX11 *g_mlk_appx11 = NULL;

static const char *g_atom_names[] = {
	"_MLK_SELECTION",
	"WM_PROTOCOLS",
	"WM_DELETE_WINDOW",
	"WM_TAKE_FOCUS",
	"_NET_WM_PING",
	"_NET_WM_USER_TIME",
	"_NET_WM_STATE",
	"_NET_WM_STATE_MAXIMIZED_HORZ",
	"_NET_WM_STATE_MAXIMIZED_VERT",
	"_NET_WM_STATE_HIDDEN",
	"_NET_WM_STATE_MODAL",
	"_NET_WM_STATE_ABOVE",
	"_NET_WM_STATE_FULLSCREEN",
	"UTF8_STRING",
	"COMPOUND_TEXT",
	"CLIPBOARD",
	"TARGETS",
	"XdndEnter",
	"XdndPosition",
	"XdndLeave",
	"XdndDrop",
	"XdndStatus",
	"XdndActionCopy",
	"text/uri-list",
	"text/plain;charset=utf-8"
};

//--------------------

//mlk_x11_clipboard.c

void __mX11ClipboardInit(void);

//mlk_x11_cursor.c

void __mX11CursorFree(mCursor cur);
mCursor __mX11CursorLoad(const char *name);
mCursor __mX11CursorCreate1bit(const uint8_t *buf);
mCursor __mX11CursorCreateRGBA(int width,int height,int hotspot_x,int hotspot_y,const uint8_t *buf);

//mlk_x11_pixbuf.c

mPixbuf *__mX11PixbufAlloc(void);
void __mX11PixbufDeleteImage(mPixbuf *pixbuf);
mlkbool __mX11PixbufCreate(mPixbuf *pixbuf,int w,int h);
mlkbool __mX11PixbufResize(mPixbuf *pixbuf,int w,int h,int neww,int newh);
void __mX11PixbufRender(mWindow *win,int x,int y,int w,int h);

//mlk_x11_window.c

mlkbool __mX11WindowAlloc(void **ppbuf);
void __mX11WindowDestroy(mWindow *p);
void __mX11WindowShow(mWindow *p,int f);
void __mX11WindowResize(mWindow *p,int w,int h);
void __mX11WindowSetCursor(mWindow *p,mCursor cur);

mlkbool __mX11ToplevelCreate(mToplevel *p);
void __mX11ToplevelSetTitle(mToplevel *p,const char *title);
void __mX11ToplevelSetIcon32(mToplevel *p,mImageBuf *img);
void __mX11ToplevelMaximize(mToplevel *p,int type);
void __mX11ToplevelFullscreen(mToplevel *p,int type);
void __mX11ToplevelMinimize(mToplevel *p);
void __mX11ToplevelStartDragMove(mToplevel *p);
void __mX11ToplevelStartDragResize(mToplevel *p,int type);
void __mX11ToplevelGetSaveState(mToplevel *p,mToplevelSaveState *st);
void __mX11ToplevelSetSaveState(mToplevel *p,mToplevelSaveState *st);

mlkbool __mX11PopupShow(mPopup *p,int x,int y,int w,int h,mBox *box,uint32_t flags);

//-------------------


//=================================
// sub
//=================================


/** X エラーハンドラ */

static int _xerrhandle(Display *disp,XErrorEvent *ev)
{
	char buf[256],m[32],reqbuf[256];

	//エラーを無効にする

	if(ev->request_code == 42) return 0;  //WM_TAKE_FOCUS

	if(ev->error_code == BadAtom && ev->request_code == 17) return 0; //XGetAtomName() の BadAtom

	//エラー表示

	XGetErrorText(disp, ev->error_code, buf, 256);

	snprintf(m, 32, "%d", ev->request_code);
	XGetErrorDatabaseText(disp, "XRequest", m, "", reqbuf, 256);

	fprintf(stderr, "X error: %d %s\n  req:%d(%s) minor:%d\n",
			ev->error_code, buf, ev->request_code, reqbuf, ev->minor_code);

	return 1;
}

/** 各機能がサポートされているかチェック & 初期化 */

static void _check_support(mAppX11 *p)
{
	int i,cnt,major,minor,op,ev,err;
	Atom *atom;

	//XKB (キーボード)

	major = XkbMajorVersion;
	minor = XkbMinorVersion;

	if(XkbLibraryVersion(&major, &minor))
	{
		major = XkbMajorVersion;
		minor = XkbMinorVersion;

		XkbQueryExtension(p->display, &op, &ev, &err, &major, &minor);
	}

	//XShm (共有メモリ)

#if !defined(MLK_X11_NO_XSHM)

	if(XQueryExtension(p->display, "MIT-SHM", &i, &i, &i))
	{
		if(XShmQueryVersion(p->display, &i, &i, &i))
			p->fsupport |= MLKX11_SUPPORT_XSHM;
	}

#endif

	//ウィンドウマネージャの有効プロパティ

	atom = (Atom *)mX11GetProperty32(p->root_window,
				mX11GetAtom("_NET_SUPPORTED"), XA_ATOM, &cnt);
	
	if(atom)
	{
		for(i = 0; i < cnt; i++)
		{
			if(atom[i] == p->atoms[MLKX11_ATOM_NET_WM_USER_TIME])
				p->fsupport |= MLKX11_SUPPORT_USERTIME;
		}
		
		mFree(atom);
	}
}

/** スレッド関連初期化 */

static int _init_thread(mAppX11 *p)
{
#if !defined(MLK_NO_THREAD)

	int fd[2];

	if(pipe(fd) != 0) return 1;

	p->thread_fd[0] = fd[0]; //read
	p->thread_fd[1] = fd[1]; //write

	fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
	fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK);

	p->mutex = mThreadMutexNew();

	mThreadMutexLock(p->mutex);

#endif

	return 0;
}

/** クライアントリーダーウィンドウ作成 */

static void _create_client_leader(mAppX11 *p)
{
	mAppBase *app = MLKAPP;
	XSetWindowAttributes attr;
	Window id;
	XClassHint chint;

	//ウィンドウ作成

	attr.override_redirect = 0;
	attr.event_mask = 0;
	
	id = XCreateWindow(p->display, p->root_window,
		10, 10, 10, 10, 0,
		CopyFromParent, InputOnly, CopyFromParent,
		CWOverrideRedirect | CWEventMask,
		&attr);

	p->leader_window = id;

	//

	mX11SetProperty_wm_pid(id);
	mX11SetProperty_wm_client_leader(id);

	//WM_COMMAND

	XSetCommand(p->display, id, &app->opt_arg0, 1);

	//WM_CLASS

	chint.res_name = (app->opt_wmname)? app->opt_wmname: app->app_wmname;
	chint.res_class = (app->opt_wmclass)? app->opt_wmclass: app->app_wmclass;

	XSetClassHint(p->display, id, &chint);
}


//=================================
// mAppBackend 関数
//=================================


/** 終了処理 */

static void _backend_close(void)
{
	mAppX11 *p = MLKAPPX11;

	if(!p) return;

	//スレッド関連

#if !defined(MLK_NO_THREAD)

	if(p->thread_fd[0] != -1)
	{
		close(p->thread_fd[0]);
		close(p->thread_fd[1]);
	}

	if(p->mutex)
	{
		mThreadMutexUnlock(p->mutex);
		mThreadMutexDestroy(p->mutex);
	}

#endif

	//input method
	
	mX11IM_close(p);

	//クリップボード

	mClipboardUnix_destroy(&p->clipb);

	//XI2

#if defined(MLK_X11_HAVE_XINPUT2)
	mListDeleteAll(&p->list_xidev);
#endif

	//GC

	if(p->gc_def)
		XFreeGC(p->display, p->gc_def);

	//クライアントリーダー

	if(p->leader_window)
		XDestroyWindow(p->display, p->leader_window);

	//

	mFree(p->startup_id);

	//ディスプレイ閉じる

	if(p->display)
	{
		XSync(p->display, False);
		XCloseDisplay(p->display);
	}

	mFree(p);

	MLKAPPX11 = NULL;
}

/** データ確保 */

static void *_backend_alloc_data(void)
{
	mAppX11 *p;

	//確保
	
	p = (mAppX11 *)mMalloc0(sizeof(mAppX11));
	if(!p) return NULL;

	MLKAPPX11 = p;

	//値初期化

	p->xi2_opcode = -1;

#if !defined(MLK_NO_THREAD)

	p->thread_fd[0] = p->thread_fd[1] = -1;

#endif

	return (void *)p;
}

/** 初期化 */

static mlkbool _backend_init(void)
{
	mAppX11 *p = MLKAPPX11;
	Display *disp;

	//Xエラーハンドラ

	XSetErrorHandler(_xerrhandle);
	
	//ディスプレイ開く
	
	p->display = disp = XOpenDisplay(NULL);
	if(!disp) return FALSE;
	
	//ロケール＆入力メソッドセット

	if(XSupportsLocale())
	{
#if !defined(MLK_NO_INPUT_METHOD)

		if(XSetLocaleModifiers(""))
			mX11IM_init(p);

#endif
	}
	
	//X 情報

	p->root_window = DefaultRootWindow(disp);
	p->screen      = DefaultScreen(disp);
	p->display_fd  = ConnectionNumber(disp);
	p->visual      = DefaultVisual(disp, p->screen);
	p->colormap    = DefaultColormap(disp, p->screen);
	p->depth       = DefaultDepth(disp, p->screen);
	
	if(p->depth <= 8) return FALSE;

	//カラー情報
	
	p->maskR = p->visual->red_mask;
	p->maskG = p->visual->green_mask;
	p->maskB = p->visual->blue_mask;
	
	p->r_shift_left = mGetOnBitPosL(p->maskR);
	p->g_shift_left = mGetOnBitPosL(p->maskG);
	p->b_shift_left = mGetOnBitPosL(p->maskB);

	p->r_shift_right = 8 - mGetOffBitPosL(p->maskR >> p->r_shift_left);
	p->g_shift_right = 8 - mGetOffBitPosL(p->maskG >> p->g_shift_left);
	p->b_shift_right = 8 - mGetOffBitPosL(p->maskB >> p->b_shift_left);

	p->is_rgb888 = (p->maskR == 0xff0000 && p->maskG == 0x00ff00 && p->maskB == 0x0000ff);

	//各アトム取得
	
	XInternAtoms(disp, (char **)g_atom_names, MLKX11_ATOM_NUM, False, p->atoms);

	//

	_check_support(p);

	if(_init_thread(p)) return FALSE;

	//クリップボード

	__mX11ClipboardInit();

	//クライアントリーダー

	_create_client_leader(p);

	//startup (クライアントリーダー作成後)

	mX11StartupGetID(p);

	//GC
		
	p->gc_def = XCreateGC(disp, p->root_window, 0, NULL);

	//XI2
	
#if defined(MLK_X11_HAVE_XINPUT2)

	if(MLKAPP->flags & MAPPBASE_FLAGS_ENABLE_PENTABLET)
		p->xi2_opcode = mX11XI2_init();

#endif

	return TRUE;
}

/** メインループ
 * 
 * wait: TRUE で、イベントがない場合は来るまで待つ
 * return: 0 でループ終了。それ以外で続ける */

static int _backend_mainloop(mAppRunData *run,mlkbool wait)
{
	mAppX11 *p = MLKAPPX11;
	Display *disp;

	disp = p->display;

	//最初の実行時、StartupNotify 完了通知を送る
	
	if(!(p->flags & MLKX11_FLAGS_STARTUP_NOTIFY))
	{
		p->flags |= MLKX11_FLAGS_STARTUP_NOTIFY;

		mX11SendStartupNotify_complete(p);
	}

	//

	XFlush(disp);
	
	while(1)
	{
		//タイマー処理
		
		mGuiTimerProc();

		//X イベント処理
	
		mX11EventTranslate(p, NULL, MLKX11_WAITEVENT_NONE, NULL);
		
		//イベント追加後の処理
		
		if(__mAppAfterEvent())
			XSync(disp, False);
		
		//mGuiQuit() が呼ばれたため、ループ終了

		if(!run->loop)
			return 0;

		//イベントが起こるまで待つ
		
		if(!wait)
			break;
		else
		{
			//イベントが残っている場合は続ける

			XFlush(disp);

			if(XPending(disp)) continue;

			//何かが起こるまで待つ
			
			mX11WaitEvent(p);
		}
	}
	
	return 1;
}

/** 内部イベントを実行 */

static void _backend_run_event(void)
{
	mAppX11 *p = MLKAPPX11;
	mAppRunData *run;
	mEvent *ev;
	void *evitem;

	run = MLKAPP->run_current;

	while(run->loop)
	{
		ev = mEventListGetEvent(&evitem);
		if(!ev) break;

	#ifdef MLK_DEBUG_PUT_EVENT
		__mAppPutEventDebug(ev);
	#endif

		//(IM) フォーカスイベント時、XIC のフォーカスをセット
		
		if(ev->type == MEVENT_FOCUS && p->im_xim)
			mX11IM_context_onFocus(ev->widget, ev->focus.is_out);

		//イベント関数

		if(ev->widget->event)
			(ev->widget->event)(ev->widget, ev);

		//イベントアイテム削除

		mEventFreeItem(evitem);
	}
}

/** スレッドロック */

void _backend_thread_lock(mlkbool lock)
{
#if !defined(MLK_NO_THREAD)

	mAppX11 *p = MLKAPPX11;

	if(p->mutex)
	{
		if(lock)
			mThreadMutexLock(p->mutex);
		else
			mThreadMutexUnlock(p->mutex);
	}

#endif
}

/** スレッド中に、イベント待ちを抜けさせる */

void _backend_thread_wakeup(void)
{
#if !defined(MLK_NO_THREAD)

	uint8_t dat = 1;

	if(MLKAPPX11->thread_fd[1])
		write(MLKAPPX11->thread_fd[1], &dat, 1);

#endif
}

/** RGB -> pix */

static mPixCol _backend_rgb_to_pix(uint8_t r,uint8_t g,uint8_t b)
{
	mAppX11 *p = MLKAPPX11;

	if(p->is_rgb888)
		return MLK_RGB(r, g, b);
	else if(p->depth >= 24)
	{
		return ((uint32_t)r << p->r_shift_left)
			| (g << p->g_shift_left)
			| (b << p->b_shift_left);
	}
	else
	{
		//16bit
		return (((uint32_t)r >> p->r_shift_right) << p->r_shift_left)
			| ((g >> p->g_shift_right) << p->g_shift_left)
			| ((b >> p->b_shift_right) << p->b_shift_left);
	}
}

/** PIX -> RGB  */

static mRgbCol _backend_pix_to_rgb(mPixCol c)
{
	mAppX11 *p = MLKAPPX11;
	uint32_t r,g,b;

	if(p->is_rgb888)
		return c & 0xffffff;
	else
	{
		r = (c & p->maskR) >> p->r_shift_left;
		g = (c & p->maskG) >> p->g_shift_left;
		b = (c & p->maskB) >> p->b_shift_left;

		//16bit の場合、0-255 に変換
		if(p->depth <= 16)
		{
			r = (r << p->r_shift_right) + (1 << p->r_shift_right) - 1;
			g = (g << p->g_shift_right) + (1 << p->g_shift_right) - 1;
			b = (b << p->b_shift_right) + (1 << p->b_shift_right) - 1;
		}
	}
	
	return (r << 16) | (g << 8) | b;
}

/** 物理キー番号から、キー識別子取得 */

static uint32_t _backend_keycode_to_key(uint32_t code)
{
	KeySym ks;

	ks = XkbKeycodeToKeysym(MLKX11_DISPLAY, code, 0, 0);

	if(ks == NoSymbol)
		return 0;
	else
		return (ks <= 0xffff)? ks: 0;
}

/** キー識別子から、キー名の文字列を取得 */

static mlkbool _backend_keycode_getname(mStr *str,uint32_t code)
{
	const char *name = NULL;

	//Prior,Next は置き換え

	switch(code)
	{
		case XK_Prior:
			name = "PageUp";
			break;
		case XK_KP_Prior:
			name = "KP_PageUp";
			break;
		case XK_Next:
			name = "PageDown";
			break;
		case XK_KP_Next:
			name = "KP_PageDown";
			break;
	}

	if(!name)
		name = XKeysymToString(code);

	mStrSetText(str, name);

	return (name != NULL);
}


//=================================
// 内部で使う関数
//=================================


/** イベントなどが起こるまで待つ
 * 
 * return: 1 = X イベントが起きた。0 = タイマーなどが起きた */

int mX11WaitEvent(mAppX11 *p)
{
	struct pollfd fds[2];
	int num,timems,timeout,ret;

	//pollfd

	fds[0].fd = p->display_fd;
	fds[0].events = POLLIN;
	num = 1;

#if !defined(MLK_NO_THREAD)

	fds[1].fd = p->thread_fd[0];
	fds[1].events = POLLIN;
	num++;

#endif

	//待つ

	do
	{
		//タイマーの最大待ち時間
		//ret = 0, -1, -2, or ms

		timems = mGuiTimerGetWaitTime_ms();
		if(timems == 0) return 0;

		timeout = (timems == -2)? INT32_MAX: timems;

		//poll

	#if !defined(MLK_NO_THREAD)
		mThreadMutexUnlock(p->mutex);
	#endif

		ret = poll(fds, num, timeout);

	#if !defined(MLK_NO_THREAD)
		mThreadMutexLock(p->mutex);
	#endif

	//待ち時間最大で何も起こらなかった or エラーで割り込み時は、続ける
	} while((ret == 0 && timems == -2) || (ret == -1 && errno == EINTR));

	//結果

	if(ret >= 1)
	{
		//スレッド
		
#if !defined(MLK_NO_THREAD)

		if(fds[1].revents & POLLIN)
		{
			uint32_t dat;
			
			while(read(p->thread_fd[0], &dat, 4) > 0);
		}

#endif
		//X イベント
	
		if(fds[0].revents & POLLIN)
			return 1;
	}
	
	return 0;
}

/** 指定ウィンドウに指定イベントが来るまで、イベント変換
 *
 * wait_win: 必要なければ NULL */

void mX11EventTranslate_wait(mWindow *wait_win,int wait_evtype)
{
	mAppX11 *p = MLKAPPX11;

	XFlush(p->display);

	while(1)
	{
		//タイマー処理
		
		mGuiTimerProc();

		//X イベント処理
	
		if(mX11EventTranslate(p, wait_win, wait_evtype, NULL)) break;

		//イベントが残っている場合は続ける

		XFlush(p->display);

		if(XPending(p->display)) continue;

		//何かが起こるまで待つ
		
		mX11WaitEvent(p);
	}
}

/** 指定イベントが来るまで待ち、XEvent を取得 */

void mX11EventTranslate_waitEvent(int wait_evtype,XEvent *evdst)
{
	mAppX11 *p = MLKAPPX11;

	XFlush(p->display);

	while(1)
	{
		//タイマー処理
		
		mGuiTimerProc();

		//X イベント処理
	
		if(mX11EventTranslate(p, NULL, wait_evtype, evdst)) break;

		//イベントが残っている場合は続ける

		XFlush(p->display);

		if(XPending(p->display)) continue;

		//何かが起こるまで待つ
		
		mX11WaitEvent(p);
	}
}

/** デスクトップの領域を取得 */

void mX11GetDesktopBox(mBox *box)
{
	mAppX11 *p = MLKAPPX11;

	box->x = box->y = 0;
	box->w = XDisplayWidth(p->display, p->screen);
	box->h = XDisplayHeight(p->display, p->screen);
}

/** デスクトップの作業領域を取得 */

void mX11GetDesktopWorkArea(mBox *box)
{
	long *buf,*pb,current;
	int num;

	//失敗時用、デフォルト

	mX11GetDesktopBox(box);

	//現在のデスクトップ番号

	if(mX11GetProperty32Array(MLKX11_ROOTWINDOW,
		mX11GetAtom("_NET_CURRENT_DESKTOP"), XA_CARDINAL, &current, 1))
	{
		//各デスクトップの作業領域 ([4] x N)
		
		buf = mX11GetProperty32(MLKX11_ROOTWINDOW,
			mX11GetAtom("_NET_WORKAREA"), XA_CARDINAL, &num);

		if(buf)
		{
			if((current + 1) * 4 <= num)
			{
				pb = buf + current * 4;

				box->x = pb[0];
				box->y = pb[1];
				box->w = pb[2];
				box->h = pb[3];
			}
			
			mFree(buf);
		}
	}
}


//================================
// 直接呼ばれる関数
//=================================


/** X11 が使えるかチェック */

mlkbool __mGuiCheckX11(const char *name)
{
	Display *disp;

	disp = XOpenDisplay(name);
	if(!disp)
		return FALSE;
	else
	{
		XCloseDisplay(disp);
		return TRUE;
	}
}

/** mAppBackend にセット */

void __mGuiSetBackendX11(mAppBackend *p)
{
	p->alloc_data = _backend_alloc_data;
	p->close = _backend_close;
	p->init = _backend_init;
	p->mainloop = _backend_mainloop;
	p->run_event = _backend_run_event;
	p->thread_lock = _backend_thread_lock;
	p->thread_wakeup = _backend_thread_wakeup;

	p->rgbtopix = _backend_rgb_to_pix;
	p->pixtorgb = _backend_pix_to_rgb;
	p->keycode_to_code = _backend_keycode_to_key;
	p->keycode_getname = _backend_keycode_getname;

	p->cursor_free = __mX11CursorFree;
	p->cursor_load = __mX11CursorLoad;
	p->cursor_create1bit = __mX11CursorCreate1bit;
	p->cursor_createRGBA = __mX11CursorCreateRGBA;

	p->pixbuf_alloc = __mX11PixbufAlloc;
	p->pixbuf_deleteimg = __mX11PixbufDeleteImage;
	p->pixbuf_create = __mX11PixbufCreate;
	p->pixbuf_resize = __mX11PixbufResize;
	p->pixbuf_render = __mX11PixbufRender;

	p->window_alloc = __mX11WindowAlloc;
	p->window_destroy = __mX11WindowDestroy;
	p->window_show = __mX11WindowShow;
	p->window_resize = __mX11WindowResize;
	p->window_setcursor = __mX11WindowSetCursor;

	p->toplevel_create = __mX11ToplevelCreate;
	p->toplevel_settitle = __mX11ToplevelSetTitle;
	p->toplevel_seticon32 = __mX11ToplevelSetIcon32;
	p->toplevel_maximize = __mX11ToplevelMaximize;
	p->toplevel_fullscreen = __mX11ToplevelFullscreen;
	p->toplevel_minimize = __mX11ToplevelMinimize;
	p->toplevel_start_drag_move = __mX11ToplevelStartDragMove;
	p->toplevel_start_drag_resize = __mX11ToplevelStartDragResize;
	p->toplevel_get_save_state = __mX11ToplevelGetSaveState;
	p->toplevel_set_save_state = __mX11ToplevelSetSaveState;

	p->popup_show = __mX11PopupShow;
}
