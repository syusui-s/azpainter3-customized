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
 * [X11] 入力メソッド
 *****************************************/

#include "mlk_x11.h"

#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_guicol.h"
#include "mlk_charset.h"

#include "mlk_pv_window.h"


/*--------------

 - XFilterEvent() では、KeyRelease はフィルタリングされないので、
   前編集中のキー離しイベントは、トップウィンドウに送られてくる。

---------------*/


//************************************
// XIM
// (アプリケーションに一つ)
//************************************


/* callback: IM が何らかの理由で停止した時
 * 
 * [!] このコールバックが来た時は、XCloseIM() や XDestroyIC() を呼んではならない */

static void _cb_im_destroy(XIM *xim,XPointer client,XPointer empty)
{
	MLKAPPX11->im_xim = NULL;
}

/* 対応しているスタイルの中で使用可能なものを取得
 *
 * return: 0 でエラー、または使用可能なものがない */

static XIMStyle _get_im_style(XIM xim)
{
	int i,j,k;
	XIMStyles *styles;
	XIMStyle s,result,
		style_pe[] = {XIMPreeditCallbacks, XIMPreeditNothing, XIMPreeditNone, 0},
		style_st[] = {XIMStatusNothing, XIMStatusNone, 0};

	//サポートされている XIMStyle のリスト取得

	if(XGetIMValues(xim, XNQueryInputStyle, &styles, (void *)0) != NULL)
		return 0;

	//対応しているスタイルの組み合わせがあるか確認

	result = 0;
	
	for(i = 0; style_pe[i]; i++)
	{
		for(j = 0; style_st[j]; j++)
		{
			s = style_pe[i] | style_st[j];
			
			for(k = 0; k < styles->count_styles; k++)
			{
				if(styles->supported_styles[k] == s)
				{
					result = s;
					goto END;
				}
			}
		}
	}

END:
	XFree(styles);

	return result;
}


/** IM 閉じる */

void mX11IM_close(mAppX11 *p)
{
	if(p->im_xim)
		XCloseIM(p->im_xim);
}

/** IM 初期化 */

void mX11IM_init(mAppX11 *p)
{
	XIM xim;
	XIMStyle style;
	XIMCallback cb;

	//IM 開く

	xim = XOpenIM(p->display, NULL, NULL, NULL);
	if(!xim) return;

	//スタイル

	style = _get_im_style(xim);

	if(style == 0)
	{
		XCloseIM(xim);
		return;
	}
	
	//destroy コールバック

	cb.callback = (XIMProc)_cb_im_destroy;
	cb.client_data = NULL;

	XSetIMValues(xim, XNDestroyCallback, &cb, (void *)0);

	//

	p->im_xim = xim;
	p->im_style = style;
}


//************************************
// 前編集用のポップアップウィンドウ
//************************************


/* 前編集用ウィンドウ作成
 *
 * top: 呼び出し元のトップレベルウィンドウ */

static void _preeditwin_create(mWindow *top)
{
	mWindow *p;
	XSetWindowAttributes attr;
	Window xid;

	mWidgetDestroy(MLK_WIDGET(MLKAPPX11->im_preeditwin));

	//X ウィンドウ作成
	
	attr.bit_gravity = ForgetGravity;
	attr.override_redirect = 1;
	attr.event_mask = ExposureMask;
	
	xid = XCreateWindow(MLKX11_DISPLAY, MLKX11_ROOTWINDOW,
		0, 0, 1, 1, 0,
		CopyFromParent, CopyFromParent, CopyFromParent,
		CWBitGravity | CWOverrideRedirect | CWEventMask,
		&attr);

	if(!xid) return;

	//mWindow

	p = __mWindowNew(NULL, 0);
	if(!p) return;

	MLKX11_WINDATA(p)->winid = xid;
	
	p->wg.draw = NULL;
	p->wg.font = mWidgetGetFont((top->win.focus)? top->win.focus: MLK_WIDGET(top));

	//

	MLKAPPX11->im_preeditwin = p;
}

/* 前編集終了 */

static void _preeditwin_done(void)
{
	mAppX11 *p = MLKAPPX11;

	if(p->im_preeditwin)
	{
		mWidgetDestroy(MLK_WIDGET(p->im_preeditwin));
		
		p->im_preeditwin = NULL;
	}
}

/* 前編集ウィンドウの位置を移動 */

static void _preeditwin_move(mWindow *top)
{
	mWindow *win;
	mWidget *wg;
	mPoint pt1,pt2;

	//フォーカスウィジェットでの表示位置取得

	wg = (top->win.focus)? top->win.focus: MLK_WIDGET(top);
	
	mX11WidgetGetRootPos(wg, &pt1);

	if(wg->get_inputmethod_pos)
		(wg->get_inputmethod_pos)(wg, &pt2);
	else
		pt2.x = 0, pt2.y = wg->h;

	//移動

	win = MLKAPPX11->im_preeditwin;

	XMoveWindow(MLKX11_DISPLAY, MLKX11_WINDATA(win)->winid,
		pt1.x + pt2.x, pt1.y + pt2.y);
}

/* 属性対応のテキスト描画
 *
 * return: 次の描画位置 */

static int _preeditwin_drawtext(mPixbuf *pixbuf,mFont *font,
	int x,uint32_t *text,int len,int fonth,XIMFeedback fb)
{
	int tw;
	mRgbCol col;
	
	tw = mFontGetTextWidth_utf32(font, text, len);

	//反転

	if(fb & XIMReverse)
	{
		mPixbufFillBox(pixbuf, x, 0, tw, fonth, MGUICOL_PIX(FACE_SELECT));
		col = MGUICOL_RGB(TEXT_SELECT);
	}
	else
		col = MGUICOL_RGB(TEXT);

	//下線
	
	if(fb & XIMUnderline)
		mPixbufLineH(pixbuf, x, fonth, tw, MGUICOL_PIX(TEXT));

	//テキスト
	
	mFontDrawText_pixbuf_utf32(font, pixbuf, x, 0, text, len, col);
	
	return x + tw;
}

/* 描画 */

static void _preeditwin_draw(mWindow *p,mFont *font,
	uint32_t *text,int len,XIMFeedback *feedback)
{
	mPixbuf *pixbuf;
	int i,pos_top,pos_cur,x,fonth;
	XIMFeedback fb,fb_prev;
	
	pixbuf = p->win.pixbuf;

	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, p->wg.w, p->wg.h, MGUICOL_PIX(FACE_DARK));
		
	//テキスト

	if(!feedback)
		//属性なし: すべて通常テキスト
		mFontDrawText_pixbuf_utf32(font, pixbuf, 0, 0, text, len, MGUICOL_RGB(TEXT));
	else
	{
		//------ 属性あり
	
		fonth = mFontGetHeight(font);
		x = 0;
		pos_cur = 0;
		pos_top = -1;
		fb_prev = 0;
	
		for(i = len; i > 0; i--, pos_cur++)
		{
			fb = *(feedback++) & (XIMReverse | XIMUnderline);
			
			if(fb != fb_prev)
			{
				//前の属性と異なる場合、pos_top〜pos_cur - 1 までを指定属性で描画
				
				if(pos_top >= 0)
				{
					x = _preeditwin_drawtext(pixbuf, font, x,
							text + pos_top, pos_cur - pos_top, fonth, fb_prev);
				}
				
				fb_prev = fb;
				pos_top = pos_cur;
			}
		}

		//残り

		if(pos_top >= 0)
		{
			_preeditwin_drawtext(pixbuf, font, x,
				text + pos_top, pos_cur - pos_top, fonth, fb_prev);
		}
	}
}


//************************************
// XIC : コンテキスト
//************************************


//=======================
// コールバック
//=======================
/*
 * - IM が ON の状態で文字が入力されると、最初に start が来る。
 * 
 * - フォーカスアウト/終了/キャンセル/削除で文字がなくなった時は、
 *   draw [text = NULL] -> done が来る。
 * 
 * - IM 側で状態を共有しない場合は、XUnsetICFocus() 時に done コールバックが来る。
 *   状態を共有する場合、XUnsetICFocus() 時は来ないが、XSetICFocus() 時に来る。
 * 
 * - 変換後、確定キーを押さずに新しい文字を入力した場合は、done は来なくて draw が続く。
 */


/* 前編集開始
 *
 * return: 前編集文字列の最大長 (-1 で無限) */

static int _cb_ic_start(XIC *xic,XPointer client,XPointer empty)
{
	_X11_DEBUG("IM [preedit_start]: %p\n", client);
	
	_preeditwin_create(MLK_WINDOW(client));

	return -1;
}

/* 前編集終了 */

static void _cb_ic_done(XIC *xic,XPointer client,XPointer empty)
{
	_X11_DEBUG("IM [preedit_done]: %p\n", client);

	_preeditwin_done();
}

/*-------------

[XIMPreeditDrawCallbackStruct]
	int caret;      //文字列内でのカーソル位置 (文字数)
	int chg_first;  //変換対象部分の開始位置
	int chg_length; //変換対象部分の文字数
	XIMText *text;  //テキスト情報 (NULL で消去)

[XIMText]
	unsigned short length;  //テキスト文字数 (文字単位)
	XIMFeedback * feedback; //各文字における表示タイプの配列
	Bool encoding_is_wchar; //ワイド文字か
	union {
		char * multi_byte;
		wchar_t * wide_char;
	} string;

---------------*/

/* 描画 */

static void _cb_ic_draw(XIC *xic,XPointer client,XIMPreeditDrawCallbackStruct *dat)
{
	mWindow *top,*prewin;
	mFont *font;
	uint32_t *text;
	int len;

	//消去の場合、基本的にこの後に done が来るので、何もしない

	if(!dat->text || !dat->text->string.multi_byte)
	{
		_X11_DEBUG("IM [draw]: clear\n");
		return;
	}

	prewin = MLKAPPX11->im_preeditwin;
	if(!prewin) return;

	//

	_X11_DEBUG("IM [draw]: len(%d) is_wide(%d)\n",
		dat->text->length, (int)dat->text->encoding_is_wchar);

	top = MLK_WINDOW(client);
	
	//UTF-32 文字列取得
	
	if(dat->text->encoding_is_wchar)
		//wchar_t
		text = mWidetoUTF32(dat->text->string.wide_char, -1, &len);
	else
		//ロケール文字列
		text = mLocaletoUTF32(dat->text->string.multi_byte, -1, &len);

	if(!text)
	{
		mWidgetShow(MLK_WIDGET(prewin), 0);
		return;
	}

	if(len > dat->text->length)
		len = dat->text->length;

	//ウィンドウサイズ

	font = mWidgetGetFont(MLK_WIDGET(prewin));
	
	mWidgetResize(MLK_WIDGET(prewin),
		mFontGetTextWidth_utf32(font, text, len),
		mFontGetHeight(font) + 1);

	//ウィンドウ位置

	_preeditwin_move(top);
	
	//描画
	//(mPixbuf に直接描画し、ウィンドウ転送させる)

	_preeditwin_draw(prewin, font, text, len, dat->text->feedback);
	
	mWidgetUpdateBox(MLK_WIDGET(prewin), NULL);

	//表示

	if(!(prewin->wg.fstate & MWIDGET_STATE_VISIBLE))
	{
		XMapWindow(MLKX11_DISPLAY, MLKX11_WINDATA(prewin)->winid);
		XRaiseWindow(MLKX11_DISPLAY, MLKX11_WINDATA(prewin)->winid);

		prewin->wg.fstate |= MWIDGET_STATE_VISIBLE;
	}

	//

	mFree(text);
}

/* テキスト挿入位置移動 */

static void _cb_ic_caret(XIC *xic,XPointer client,XIMPreeditCaretCallbackStruct *calldat)
{

}


//======================


/* コンテキスト作成 (XIMPreeditCallbacks 用) */

static XIC _create_context_callback(mAppX11 *p,mWindow *win)
{
	XIC xic;
	XIMCallback cb[4];
	XVaNestedList listp;
	Window xid;
	int i;

	//前編集属性データ

	cb[0].callback = (XIMProc)_cb_ic_start;
	cb[1].callback = (XIMProc)_cb_ic_done;
	cb[2].callback = (XIMProc)_cb_ic_draw;
	cb[3].callback = (XIMProc)_cb_ic_caret;

	for(i = 0; i < 4; i++)
		cb[i].client_data = (XPointer)win;

	listp = XVaCreateNestedList(0,
		XNPreeditStartCallback, cb,
		XNPreeditDoneCallback,  cb + 1,
		XNPreeditDrawCallback,  cb + 2,
		XNPreeditCaretCallback, cb + 3,
		(void *)0);

	//作成

	xid = MLKX11_WINDATA(win)->winid;

	xic = XCreateIC(p->im_xim,
		XNInputStyle,   p->im_style,
		XNClientWindow, xid,
		XNFocusWindow,  xid,
		XNPreeditAttributes, listp,
		(void *)0);

	XFree(listp);

	return xic;
}

/** ウィンドウ削除時 */

void mX11IM_context_destroy(mWindow *win)
{
	mWindowDataX11 *p = MLKX11_WINDATA(win);

	//[!] IM が停止している場合、破棄処理は行なってはいけない

	if(MLKAPPX11->im_xim && p->xic)
		XDestroyIC(p->xic);
}

/** コンテキスト作成 (トップレベルウィンドウ作成時) */

void mX11IM_context_create(mWindow *win)
{
	mAppX11 *app = MLKAPPX11;
	mWindowDataX11 *winx11;
	XIC xic;
	long mask;
	
	//IM 無効時

	if(!app->im_xim) return;

	winx11 = MLKX11_WINDATA(win);

	//コンテキスト作成
	
	if(app->im_style & XIMPreeditCallbacks)
	{
		//コールバック

		xic = _create_context_callback(app, win);
	}
	else
	{
		//前編集なし

		xic = XCreateIC(app->im_xim,
				XNInputStyle,   app->im_style,
				XNClientWindow, winx11->winid,
				XNFocusWindow,  winx11->winid,
				(void *)0);
	}

	if(!xic) return;

	//ウィンドウが必要な X イベントマスク追加
	//[!] 64bit OS の場合、上位 32bit には値がセットされないので、先に mask をクリア

	mask = 0;
	XGetICValues(xic, XNFilterEvents, &mask, (void *)0);

	mX11WindowSetEventMask(win, mask, TRUE);

	//

	winx11->xic = xic;
}

/** ウィジェットのフォーカスIN/OUT 時
 *
 * 内部イベントで MEVENT_FOCUS が来た時、常に実行する。
 * トップレベルウィンドウの入力コンテキストのフォーカスを変更。
 * トップレベルが同じで、ウィジェットのフォーカスのみが変更された時も変更する。 */

void mX11IM_context_onFocus(mWidget *wg,mlkbool focusout)
{
	XIC xic;
	mWindow *top;

	top = MLK_WINDOW(wg->toplevel);

	//最上位がトップレベルでない場合

	if(!(top->wg.ftype & MWIDGET_TYPE_WINDOW_TOPLEVEL))
		return;

	//フォーカスイン時、ウィジェットが IM 無効なら、フォーカスアウト扱い

	if(!focusout
		&& !(wg->fstate & MWIDGET_STATE_ENABLE_INPUT_METHOD))
		focusout = TRUE;

	//XIC フォーカス

	xic = MLKX11_WINDATA(top)->xic;
	if(!xic) return;

	if(focusout)
		XUnsetICFocus(xic);
	else
		XSetICFocus(xic);

	_X11_DEBUG("IM <focus %s>: %p\n", (focusout)? "out": "in", top);

	//フォーカスアウト時、IM が状態共有時はウィンドウが残るので、破棄する

	if(focusout)
		_preeditwin_done();
}

