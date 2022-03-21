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
 * <X11> ウィンドウ
 *****************************************/

#include <string.h>	//strlen

#define MLKX11_INC_ATOM
#define MLKX11_INC_UTIL
#include "mlk_x11.h"

#include "mlk_rectbox.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_window_deco.h"
#include "mlk_imagebuf.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_window.h"


//-----------------------------------------
/* _MOTIF_WM_HINTS のデータフラグ */

#define MWM_HINTS_FUNCTIONS     (1<<0)
#define MWM_HINTS_DECORATIONS   (1<<1)
#define MWM_HINTS_INPUT_MODE    (1<<2)

#define MWM_FUNC_ALL            (1<<0)
#define MWM_FUNC_RESIZE         (1<<1)
#define MWM_FUNC_MOVE           (1<<2)
#define MWM_FUNC_MINIMIZE       (1<<3)
#define MWM_FUNC_MAXIMIZE       (1<<4)
#define MWM_FUNC_CLOSE          (1<<5)

#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define MWM_DECOR_ALL           (1<<0)
#define MWM_DECOR_BORDER        (1<<1)
#define MWM_DECOR_RESIZEH       (1<<2)
#define MWM_DECOR_TITLE         (1<<3)
#define MWM_DECOR_MENU          (1<<4)
#define MWM_DECOR_MINIMIZE      (1<<5)
#define MWM_DECOR_MAXIMIZE      (1<<6)

//-----------------------------------------


//=========================
// sub
//=========================


/** _NET_WM_STATE の状態を変更する
 *
 * type: 0 で取り外す。0 以外でセット。
 * return: 実際にセットされたか */

static mlkbool _change_NET_WM_STATE(mToplevel *p,int type,int atomno,int reqflag)
{
	if(type && MLK_WINDOW_IS_UNMAP(p))
	{
		//ウィンドウが非表示の状態の場合、表示された時に実行させる

		MLKX11_WINDATA(p)->fmap_request |= reqflag;
		return FALSE;
	}
	else
	{
		mX11Send_NET_WM_STATE(MLKX11_WINDATA(p)->winid, (type != 0), MLKAPPX11->atoms[atomno], 0);
		return TRUE;
	}
}


//=======================================
// backend 関数 (mWindow)
//=======================================


/** ウィンドウデータ確保 */

mlkbool __mX11WindowAlloc(void **ppbuf)
{
	*ppbuf = mMalloc0(sizeof(mWindowDataX11));

	return (*ppbuf != NULL);
}

/** ウィンドウ破棄 */

void __mX11WindowDestroy(mWindow *p)
{
	if(MLKX11_WINDATA(p)->winid)
	{
		//IM
		
		mX11IM_context_destroy(p);

		//
	
		XDestroyWindow(MLKX11_DISPLAY, MLKX11_WINDATA(p)->winid);

		XSync(MLKX11_DISPLAY, False);
	}
}

/** ウィンドウ表示/非表示 */

void __mX11WindowShow(mWindow *p,int f)
{
	mAppX11 *app = MLKAPPX11;
	Window id = MLKX11_WINDATA(p)->winid;

	if(f)
	{
		mX11WindowSetUserTime(p, app->evtime_user);

		//表示

		XMapWindow(app->display, id);
		XRaiseWindow(app->display, id);

		//MAP イベントが来るまで待つ

		mX11EventTranslate_wait(p, MLKX11_WAITEVENT_MAP);
	}
	else
	{
		XWithdrawWindow(app->display, id, app->screen);
		XFlush(app->display);
	}
}

/** サイズ変更 */

void __mX11WindowResize(mWindow *p,int w,int h)
{
	XResizeWindow(MLKX11_DISPLAY, MLKX11_WINDATA(p)->winid, w, h);
}

/** カーソルセット
 *
 * cur: 0 でデフォルトカーソルに戻す */

void __mX11WindowSetCursor(mWindow *p,mCursor cur)
{
	if(MLKX11_WINDATA(p)->cursor != cur)
	{
		MLKX11_WINDATA(p)->cursor = cur;
	
		XDefineCursor(MLKX11_DISPLAY, MLKX11_WINDATA(p)->winid, cur);
		XFlush(MLKX11_DISPLAY);
	}
}


//=======================================
// backend 関数 (mToplevel)
//=======================================


/** トップレベルウィンドウの実体を作成 */

mlkbool __mX11ToplevelCreate(mToplevel *p)
{
	mAppBase *app = MLKAPP;
	mAppX11 *appx = MLKAPPX11;
	mWindowDataX11 *data;
	XSetWindowAttributes attr;
	XClassHint chint;
	Window id;
	Atom atom[3];

	//ウィンドウ属性

	attr.bit_gravity = ForgetGravity;
	attr.override_redirect = 0;
	attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask| ButtonMotionMask;

	attr.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask
		| FocusChangeMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask | PointerMotionMask | PropertyChangeMask;
	
	//ウィンドウ作成
	
	id = XCreateWindow(appx->display, appx->root_window,
		0, 0, 1, 1, 0,
		CopyFromParent, CopyFromParent, CopyFromParent,
		CWBitGravity | CWDontPropagate | CWOverrideRedirect | CWEventMask,
		&attr);

	if(!id) return FALSE;

	data = MLKX11_WINDATA(p);

	data->winid = id;
	data->eventmask = attr.event_mask;
	
	//_NET_WM_PID (プロセスID) セット

	mX11SetProperty_wm_pid(id);

	//WM_CLIENT_LEADER

	mX11SetProperty_wm_client_leader(id);

	//class

	chint.res_name = (app->opt_wmname)? app->opt_wmname: app->app_wmname;
	chint.res_class = (app->opt_wmclass)? app->opt_wmclass: app->app_wmclass;

	XSetClassHint(appx->display, id, &chint);

	//ウィンドウタイプセット
	
	if(p->top.fstyle & MTOPLEVEL_S_DIALOG)
	{
		data->fmap_request |= MLKX11_WIN_MAP_REQUEST_MODAL;

		mX11SetPropertyWindowType(id, "_NET_WM_WINDOW_TYPE_DIALOG");
	}
	else
		mX11SetPropertyWindowType(id, "_NET_WM_WINDOW_TYPE_NORMAL");

	//ユーザータイム
	
	if(appx->fsupport & MLKX11_SUPPORT_USERTIME)
	{
		//_NET_WM_USER_TIME 用の子ウィンドウ

		data->usertime_winid = XCreateSimpleWindow(appx->display, id, -1, -1, 1, 1, 0, 0, 0);

		//_NET_WM_USER_TIME_WINDOW セット
		
		mX11SetProperty32(id,
			mX11GetAtom("_NET_WM_USER_TIME_WINDOW"),
			XA_WINDOW, &data->usertime_winid, 1);

		//0 = マップ時にフォーカスを与えない
		
		//mX11WindowSetUserTime(MLK_WINDOW(p), 0);
	}
	
	//装飾
	
	mX11ToplevelSetDecoration(p);
	
	//親ウィンドウ
	
	if((p->top.fstyle & MTOPLEVEL_S_PARENT)
		&& p->win.parent)
	{
		XSetTransientForHint(appx->display, id,
			MLKX11_WINDATA((p->win.parent)->toplevel)->winid);
	}

	//WM_PROTOCOLS

	atom[0] = appx->atoms[MLKX11_ATOM_WM_DELETE_WINDOW];
	atom[1] = appx->atoms[MLKX11_ATOM_WM_TAKE_FOCUS];
	atom[2] = appx->atoms[MLKX11_ATOM_NET_WM_PING];
	
	XSetWMProtocols(appx->display, id, atom, 3);
	
	//D&D 操作を有効にする
	//("XdndAware" プロパティに DND バージョンセット)

	atom[0] = 5;

	mX11SetPropertyAtom(id, mX11GetAtom("XdndAware"), atom, 1);

	//入力コンテキスト
	
#if !defined(MLK_NO_INPUT_METHOD)
	
	if(!(p->top.fstyle & MTOPLEVEL_S_NO_INPUT_METHOD))
		mX11IM_context_create(MLK_WINDOW(p));
	
#endif

	//XI2

#if defined(MLK_X11_HAVE_XINPUT2)

	if((p->top.fstyle & MTOPLEVEL_S_ENABLE_PENTABLET)
		&& appx->xi2_opcode != -1)
		mX11XI2_selectEvent(id);

#endif

	return TRUE;
}

/** ウィンドウタイトルセット */

void __mX11ToplevelSetTitle(mToplevel *p,const char *title)
{
	mAppX11 *app = MLKAPPX11;
	Window wid;
	int len;

	wid = MLKX11_WINDATA(p)->winid;
	len = (title)? strlen(title): 0;

	//タイトルバー

	XChangeProperty(app->display, wid, mX11GetAtom("_NET_WM_NAME"),
		app->atoms[MLKX11_ATOM_UTF8_STRING], 8, PropModeReplace, (unsigned char *)title, len);

	mX11SetPropertyCompoundText(wid, mX11GetAtom("WM_NAME"), title, len);

	//タスクバー

	XChangeProperty(app->display, wid, mX11GetAtom("_NET_WM_ICON_NAME"),
		app->atoms[MLKX11_ATOM_UTF8_STRING], 8, PropModeReplace, (unsigned char *)title, len);

	mX11SetPropertyCompoundText(wid, mX11GetAtom("WM_ICON_NAME"), title, len);
}

/** アイコンイメージをセット
 *
 * img: 32bit イメージ */

void __mX11ToplevelSetIcon32(mToplevel *p,mImageBuf *img)
{
	unsigned long *buf,*pd;
	uint8_t *ps;
	int pixcnt,i;

	pixcnt = img->width * img->height;

	buf = (unsigned long *)mMalloc((2 + pixcnt) * sizeof(long));
	if(!buf) return;

	//[0] 幅 [1] 高さ [2-] 32bitイメージ(long)

	pd = buf;
	ps = img->buf;

	*(pd++) = img->width;
	*(pd++) = img->height;

	for(i = pixcnt; i > 0; i--, ps += 4)
		*(pd++) = MLK_ARGB(ps[0], ps[1], ps[2], ps[3]);
	
	//セット
	
	XChangeProperty(MLKX11_DISPLAY, MLKX11_WINDATA(p)->winid,
		mX11GetAtom("_NET_WM_ICON"), XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)buf, 2 + pixcnt);
 
	mFree(buf);
}

/** 最大化 */

void __mX11ToplevelMaximize(mToplevel *p,int type)
{
	if(type && MLK_WINDOW_IS_UNMAP(p))
		//最大化 & UNMAP 時 (初期表示時など)
		MLKX11_WINDATA(p)->fmap_request |= MLKX11_WIN_MAP_REQUEST_MAXIMIZE;
	else
	{
		mX11Send_NET_WM_STATE(MLKX11_WINDATA(p)->winid, type,
			MLKAPPX11->atoms[MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_HORZ],
			MLKAPPX11->atoms[MLKX11_ATOM_NET_WM_STATE_MAXIMIZED_VERT]);

		//実際に状態が変わるまで待つ

		mX11EventTranslate_wait(MLK_WINDOW(p), MLKX11_WAITEVENT_MAXIMIZE);
	}
}

/** 全画面 */

void __mX11ToplevelFullscreen(mToplevel *p,int type)
{
	if(_change_NET_WM_STATE(p, type,
		MLKX11_ATOM_NET_WM_STATE_FULLSCREEN,
		MLKX11_WIN_MAP_REQUEST_FULLSCREEN))
	{
		mX11EventTranslate_wait(MLK_WINDOW(p), MLKX11_WAITEVENT_FULLSCREEN);
	}
}

/** 最小化 */

void __mX11ToplevelMinimize(mToplevel *p)
{
	_change_NET_WM_STATE(p, 1,
		MLKX11_ATOM_NET_WM_STATE_HIDDEN,
		MLKX11_WIN_MAP_REQUEST_HIDDEN);
}

/** ドラッグによるウィンドウの移動を開始させる */

void __mX11ToplevelStartDragMove(mToplevel *p)
{
	mAppX11 *app = MLKAPPX11;
	long d[4];

	XUngrabPointer(app->display, CurrentTime);

	d[0] = app->ptpress_rx;
	d[1] = app->ptpress_ry;
	d[2] = 8; //_NET_WM_MOVERESIZE_MOVE
	d[3] = app->ptpress_btt;

	mX11SendClientMessageToRoot(MLKX11_WINDATA(p)->winid,
		mX11GetAtom("_NET_WM_MOVERESIZE"), d, 4);
}

/** ドラッグによるウィンドウサイズ変更を開始させる */

void __mX11ToplevelStartDragResize(mToplevel *p,int type)
{
	mAppX11 *app = MLKAPPX11;
	long d[4];
	int n;

	if(type == MTOPLEVEL_RESIZE_TYPE_TOP_LEFT)
		n = 0;
	else if(type == MTOPLEVEL_RESIZE_TYPE_TOP)
		n = 1;
	else if(type == MTOPLEVEL_RESIZE_TYPE_TOP_RIGHT)
		n = 2;
	else if(type == MTOPLEVEL_RESIZE_TYPE_RIGHT)
		n = 3;
	else if(type == MTOPLEVEL_RESIZE_TYPE_BOTTOM_RIGHT)
		n = 4;
	else if(type == MTOPLEVEL_RESIZE_TYPE_BOTTOM)
		n = 5;
	else if(type == MTOPLEVEL_RESIZE_TYPE_BOTTOM_LEFT)
		n = 6;
	else if(type == MTOPLEVEL_RESIZE_TYPE_LEFT)
		n = 7;
	else
		return;

	//

	XUngrabPointer(app->display, CurrentTime);

	d[0] = app->ptpress_rx;
	d[1] = app->ptpress_ry;
	d[2] = n;
	d[3] = app->ptpress_btt;

	mX11SendClientMessageToRoot(MLKX11_WINDATA(p)->winid,
		mX11GetAtom("_NET_WM_MOVERESIZE"), d, 4);
}

/** 保存用の状態データ取得 */

void __mX11ToplevelGetSaveState(mToplevel *p,mToplevelSaveState *st)
{
	mWindowDataX11 *data = MLKX11_WINDATA(p);
	mPoint pt;

	st->flags = MTOPLEVEL_SAVESTATE_F_HAVE_POS;

	if(p->win.fstate & (MWINDOW_STATE_MAXIMIZED | MWINDOW_STATE_FULLSCREEN))
	{
		//最大化/フルスクリーン

		if(p->win.fstate & MWINDOW_STATE_FULLSCREEN)
			st->flags |= MTOPLEVEL_SAVESTATE_F_FULLSCREEN;
		else
			st->flags |= MTOPLEVEL_SAVESTATE_F_MAXIMIZED;

		st->x = st->y = 0;
		st->w = p->win.norm_width;
		st->h = p->win.norm_height;
		st->norm_x = data->norm_x;
		st->norm_y = data->norm_y;
	}
	else
	{
		//位置は (Xサーバーによる) ウィンドウ枠を含む

		mX11WindowGetFramePos(MLK_WINDOW(p), &pt);

		st->x = pt.x;
		st->y = pt.y;
		st->w = p->win.win_width;
		st->h = p->win.win_height;
		st->norm_x = 0;
		st->norm_y = 0;
	}
}

/** 保存用の状態データを適用 */

void __mX11ToplevelSetSaveState(mToplevel *p,mToplevelSaveState *st)
{
	mWindowDataX11 *data = MLKX11_WINDATA(p);
	mPoint pt;
	int x,y;
	mSize size;

	//サイズ

	p->win.norm_width  = st->w;
	p->win.norm_height = st->h;

	if(st->flags
		& (MTOPLEVEL_SAVESTATE_F_MAXIMIZED | MTOPLEVEL_SAVESTATE_F_FULLSCREEN))
	{
		//最大化/フルスクリーン
		//(戻る時用のサイズとしてセット)
		
		p->win.win_width  = st->w;
		p->win.win_height = st->h;
		
		XResizeWindow(MLKX11_DISPLAY, data->winid, st->w, st->h);
	}
	else
	{
		//通常ウィンドウ状態
		
		__mWindowDecoRunSetWidth(MLK_WINDOW(p), MWINDOW_DECO_SETWIDTH_TYPE_NORMAL);

		__mWindowGetSize_excludeDecoration(MLK_WINDOW(p), st->w, st->h, &size);

		mWidgetResize(MLK_WIDGET(p), size.w, size.h);
	}

	//位置

	if(st->flags & MTOPLEVEL_SAVESTATE_F_HAVE_POS)
	{
		if(st->flags
			& (MTOPLEVEL_SAVESTATE_F_MAXIMIZED | MTOPLEVEL_SAVESTATE_F_FULLSCREEN))
		{
			x = st->norm_x;
			y = st->norm_y;
		}
		else
		{
			x = st->x;
			y = st->y;
		}

		//作業領域内に調整

		pt.x = x;
		pt.y = y;
		mX11WindowAdjustPosDesktop(MLK_WINDOW(p), &pt, TRUE);

		//非表示時は MAP 時に位置をセット

		if(MLK_WINDOW_IS_UNMAP(p))
		{
			data->fmap_request |= MLKX11_WIN_MAP_REQUEST_MOVE;
			data->move_x = pt.x;
			data->move_y = pt.y;
		}

		//セット

		XMoveWindow(MLKX11_DISPLAY, data->winid, pt.x, pt.y);
	}

	//最大化/フルスクリーン

	if(st->flags & MTOPLEVEL_SAVESTATE_F_FULLSCREEN)
		mToplevelFullscreen(p, 1);
	else if(st->flags & MTOPLEVEL_SAVESTATE_F_MAXIMIZED)
		mToplevelMaximize(p, 1);
}


//======================================
// backend 関数 (ポップアップ)
//======================================


/* ウィンドウ位置調整 */

static void _popup_adjust_pos(mPopup *p,mPoint *ptdst,
	int x,int y,int w,int h,mBox *box,uint32_t flags,mBox *boxwork)
{
	mPoint pt;
	mRect rc,rc2,rca,rcwork;

	//作業領域の rect

	mRectSetBox(&rcwork, boxwork);

	//基準の矩形 (ルート位置)

	mX11WidgetGetRootPos(p->win.parent, &pt);

	if(box)
	{
		rca.x1 = pt.x + box->x + x;
		rca.y1 = pt.y + box->y + y;
		rca.x2 = rca.x1 + box->w;
		rca.y2 = rca.y1 + box->h;
	}
	else
	{
		rca.x1 = rca.x2 = pt.x + x;
		rca.y1 = rca.y2 = pt.y + y;
	}

	//rc = デフォルト位置

	__mPopupGetRect(&rc, &rca, w, h, flags, 0);

	ptdst->x = rc.x1;
	ptdst->y = rc.y1;

	//X 方向がはみ出している場合、X 反転

	if((flags & MPOPUP_F_FLIP_X)
		&& (rc.x1 < rcwork.x1 || rc.x2 > rcwork.x2))
	{
		__mPopupGetRect(&rc2, &rca, w, h, flags, 1);

		ptdst->x = rc2.x1;
	}

	//Y 方向がはみ出している場合、Y 反転

	if((flags & MPOPUP_F_FLIP_Y)
		&& (rc.y1 < rcwork.y1 || rc.y2 > rcwork.y2))
	{
		__mPopupGetRect(&rc2, &rca, w, h, flags, 2);

		ptdst->y = rc2.y1;
	}

	//[!] 領域内のずらし調整はこの後で行われる
}

/** ポップアップウィンドウ表示 */

mlkbool __mX11PopupShow(mPopup *p,int x,int y,int w,int h,mBox *box,uint32_t flags)
{
	XSetWindowAttributes attr;
	Window xid;
	mBox boxw;
	mPoint pt;

	//ウィンドウ作成

	attr.bit_gravity = ForgetGravity;
	attr.override_redirect = 1;
	attr.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask| ButtonMotionMask;

	attr.event_mask = ExposureMask | EnterWindowMask | LeaveWindowMask
		| StructureNotifyMask | KeyPressMask | KeyReleaseMask
		| ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
	
	xid = XCreateWindow(MLKX11_DISPLAY, MLKX11_ROOTWINDOW,
		0, 0, 1, 1, 0,
		CopyFromParent, CopyFromParent, CopyFromParent,
		CWBitGravity | CWDontPropagate | CWOverrideRedirect | CWEventMask,
		&attr);

	if(!xid) return FALSE;

	MLKX11_WINDATA(p)->winid = xid;
	MLKX11_WINDATA(p)->eventmask = attr.event_mask;

	mX11GetDesktopWorkArea(&boxw);

	//作業領域内のサイズに収める

	if(w > boxw.w) w = boxw.w;
	if(h > boxw.h) h = boxw.h;
	
	//サイズ変更
	
	mWidgetResize(MLK_WIDGET(p), w, h);

	//ルート位置

	_popup_adjust_pos(p, &pt, x, y, w, h, box, flags, &boxw);

	mX11WindowAdjustPosDesktop(MLK_WINDOW(p), &pt, TRUE);

	//移動

	XMoveWindow(MLKX11_DISPLAY, xid, pt.x, pt.y);

	//表示

	mWidgetShow(MLK_WIDGET(p), 1);

	//ポインタのグラブ
	// :イベントは通常通り各ウィンドウに送る。
	// :同プログラムのウィンドウ以外でイベントが起こった時は、グラブウィンドウに送られる。

	if(!(p->pop.fstyle & MPOPUP_S_NO_GRAB)
		&& !(MLKAPP->flags & MAPPBASE_FLAGS_DEBUG_MODE))
	{
		XGrabPointer(MLKX11_DISPLAY, xid, True,
			ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask | LeaveWindowMask,
			GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	}

	return TRUE;
}


//======================================
// X11用関数
// (X11 内部でのみ使われるもの)
//======================================


/** ウィジェットのルート位置取得 */

void mX11WidgetGetRootPos(mWidget *p,mPoint *pt)
{
	mWindow *win = p->toplevel;

	mX11WindowGetRootPos(win, pt);

	pt->x += win->win.deco.w.left + p->absX;
	pt->y += win->win.deco.w.top  + p->absY;
}

/** ウィンドウ装飾セット */

void mX11ToplevelSetDecoration(mToplevel *p)
{
	//[0] flags [1] functions [2] decorations [3] inputmode
	long val[4];
	uint32_t style;
	Atom prop;
	
	style = p->top.fstyle;

	val[0] = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS | MWM_HINTS_INPUT_MODE;
	val[1] = MWM_FUNC_MOVE;
	val[2] = 0;
	val[3] = MWM_INPUT_MODELESS;

	if(!(style & MTOPLEVEL_S_NO_RESIZE))
		val[1] |= MWM_FUNC_RESIZE;

	if(style & MTOPLEVEL_S_TITLE)
		val[2] |= MWM_DECOR_TITLE;

	if(style & MTOPLEVEL_S_CLOSE)
		val[1] |= MWM_FUNC_CLOSE;

	if(style & MTOPLEVEL_S_BORDER)
		val[2] |= MWM_DECOR_BORDER;

	if(style & MTOPLEVEL_S_SYSMENU)
		val[2] |= MWM_DECOR_MENU;

	if(style & MTOPLEVEL_S_MINIMIZE)
	{
		val[2] |= MWM_DECOR_MINIMIZE;
		val[1] |= MWM_FUNC_MINIMIZE;
	}

	if((style & MTOPLEVEL_S_MAXIMIZE) &&
		!(style & MTOPLEVEL_S_NO_RESIZE))
	{
		val[2] |= MWM_DECOR_MAXIMIZE;
		val[1] |= MWM_FUNC_MAXIMIZE;
	}

	//

	prop = mX11GetAtom("_MOTIF_WM_HINTS");

	XChangeProperty(MLKX11_DISPLAY, MLKX11_WINDATA(p)->winid, prop, prop, 32,
		PropModeReplace, (unsigned char *)val, 4);
}

/** X イベントマスクセット
 * 
 * append: TRUE で現在のマスクに追加。FALSE で置換え。 */

void mX11WindowSetEventMask(mWindow *p,long mask,mlkbool append)
{
	mWindowDataX11 *data = MLKX11_WINDATA(p);

	if(append)
		data->eventmask |= mask;
	else
		data->eventmask = mask;
	
	XSelectInput(MLKX11_DISPLAY, data->winid, data->eventmask);
}

/** ユーザータイムをセット */

void mX11WindowSetUserTime(mWindow *p,unsigned long time)
{
	//初期表示時に 0 が設定されている場合、
	//ウィンドウマネージャによって、マップ時にフォーカスがセットされない。
	//そのため、time = 0 の場合はセットしない。

	if(time && MLKX11_WINDATA(p)->usertime_winid)
	{
		mX11SetPropertyCARDINAL(
			MLKX11_WINDATA(p)->usertime_winid,
			MLKAPPX11->atoms[MLKX11_ATOM_NET_WM_USER_TIME], &time, 1);
	}
}

/** 現在のカーソルの位置を、指定ウィンドウの座標で取得 */

void mX11WindowGetCursorPos(mWindow *p,mPoint *pt)
{
	Window win;
	int x,y,rx,ry;
	unsigned int btt;

	XQueryPointer(MLKX11_DISPLAY, MLKX11_WINDATA(p)->winid,
		&win, &win, &rx, &ry, &x, &y, &btt);

	pt->x = x;
	pt->y = y;
}

/** ウィンドウの左上ルート位置を取得 (ウィンドウ枠は含まない) */

void mX11WindowGetRootPos(mWindow *p,mPoint *pt)
{
	int x,y;
	Window id;

	XTranslateCoordinates(MLKX11_DISPLAY,
		MLKX11_WINDATA(p)->winid, MLKX11_ROOTWINDOW,
		0, 0, &x, &y, &id);

	pt->x = x;
	pt->y = y;
}

/** ルート位置をウィンドウからの相対位置に変換 */

void mX11WindowRootToWinPos(mWindow *p,mPoint *pt)
{
	int x,y;
	Window id;

	XTranslateCoordinates(MLKX11_DISPLAY,
		MLKX11_ROOTWINDOW, MLKX11_WINDATA(p)->winid,
		pt->x, pt->y, &x, &y, &id);

	pt->x = x;
	pt->y = y;
}

/** ウィンドウ枠の左上のルート位置を取得 */

void mX11WindowGetFramePos(mWindow *p,mPoint *pt)
{
	mAppX11 *app = MLKAPPX11;
	Window root,child;
	int x,y;
	unsigned int w,h,b,d;
	mRect rc;

	mX11WindowGetFrameWidth(p, &rc);

	XGetGeometry(app->display, MLKX11_WINDATA(p)->winid, &root, &x, &y, &w, &h, &b, &d);
	
	XTranslateCoordinates(app->display, MLKX11_WINDATA(p)->winid,
		app->root_window, 0, 0, &x, &y, &child);

	pt->x = x - rc.x1;
	pt->y = y - rc.y1;
}

/** ウィンドウ枠の各幅を取得 */

void mX11WindowGetFrameWidth(mWindow *p,mRect *rc)
{
	long val[4];
	
	if(mX11GetProperty32Array(MLKX11_WINDATA(p)->winid,
		mX11GetAtom("_NET_FRAME_EXTENTS"), XA_CARDINAL, val, 4))
	{
		rc->x1 = val[0];
		rc->x2 = val[1];
		rc->y1 = val[2];
		rc->y2 = val[3];
	}
	else
		rc->x1 = rc->y1 = rc->x2 = rc->y2 = 0;
}

/** 枠も含めたウィンドウ全体のサイズ取得 */

void mX11WindowGetFullSize(mWindow *p,mSize *size)
{
	mRect rc;

	mX11WindowGetFrameWidth(p, &rc);
	
	size->w = rc.x1 + rc.x2 + p->win.win_width;
	size->h = rc.y1 + rc.y2 + p->win.win_height;
}

/** ウィンドウ位置を、デスクトップ内に収まるように調整
 * 
 * workarea: TRUE で作業領域内に収める */

void mX11WindowAdjustPosDesktop(mWindow *p,mPoint *pt,mlkbool workarea)
{
	mBox box;
	mSize size;
	int x,y,x2,y2;
	
	x = pt->x, y = pt->y;
	
	mX11WindowGetFullSize(p, &size);

	if(workarea)
		mX11GetDesktopWorkArea(&box);
	else
		mX11GetDesktopBox(&box);

	x2 = box.x + box.w;
	y2 = box.y + box.h;
		
	if(x + size.w > x2) x = x2 - size.w;
	if(y + size.h > y2) y = y2 - size.h;
	if(x < box.x) x = box.x;
	if(y < box.y) y = box.y;
	
	pt->x = x, pt->y = y;
}

