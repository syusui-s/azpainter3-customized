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
 * ウィンドウ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_key.h"
#include "mlk_guicol.h"
#include "mlk_window_deco.h"
#include "mlk_menubar.h"
#include "mlk_accelerator.h"

#include "mlk_str.h"
#include "mlk_pixbuf.h"
#include "mlk_imagebuf.h"
#include "mlk_loadimage.h"
#include "mlk_util.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"



//===============================
// mWindow
//===============================


/**@ フォーカスウィジェットを取得 */

mWidget *mWindowGetFocus(mWindow *p)
{
	return p->win.focus;
}

/**@ トップレベルウィンドウか */

mlkbool mWindowIsToplevel(mWindow *p)
{
	return ((p->wg.ftype & MWIDGET_TYPE_WINDOW_TOPLEVEL) != 0);
}

/**@ アクティブな状態か
 *
 * @d:通常ウィンドウの場合は、ウィンドウにフォーカスがあるか。\
 * ポップアップの場合は、現在ポップアップされているウィンドウか。\
 * (ポップアップの場合、ウィンドウにフォーカスはセットされないため) */

mlkbool mWindowIsActivate(mWindow *p)
{
	mAppRunData *run = MLKAPP->run_current;

	return (p == MLKAPP->window_focus
		|| (run && run->type == MAPPRUN_TYPE_POPUP && run->window == p));
}

/**@ モーダル実行中で、p がモーダルのウィンドウ以外かどうか */

mlkbool mWindowIsOtherThanModal(mWindow *p)
{
	mAppRunData *run = MLKAPP->run_current;

	return (run && run->type == MAPPRUN_TYPE_MODAL && run->window != p);
}

/**@ レイアウトの初期サイズでウィンドウサイズを変更し、ウィンドウ表示 */

void mWindowResizeShow_initSize(mWindow *p)
{
	mGuiCalcHintSize();

	mWidgetResize(MLK_WIDGET(p), p->wg.initW, p->wg.initH);
	mWidgetShow(MLK_WIDGET(p), 1);
}

/**@ ウィンドウにおけるカーソルの変更を行う
 *
 * @d:通常はウィジェット単位でカーソルが管理されるため、
 *  カーソルがウィンドウ下にある時、ウィジェットに関係なく、
 *  一時的にカーソルを変更したいときに使う。 */

void mWindowSetCursor(mWindow *p,mCursor cursor)
{
	(MLKAPP->bkend.window_setcursor)(p, cursor);
}

/**@ カーソルをウィジェットの状態からリセット
 *
 * @d:カーソルが指定ウィンドウ下にある場合、ENTER 状態のウィジェットのカーソルを適用する。 */

void mWindowResetCursor(mWindow *p)
{
	mWidget *wg;

	if(MLKAPP->window_enter == p)
	{
		wg = MLKAPP->widget_enter;

		if(wg)
			(MLKAPP->bkend.window_setcursor)(p, wg->cursor);
	}
}


//===============================
// mWindowDeco
//===============================


/**@ ウィンドウ装飾の各幅をセット */

void mWindowDeco_setWidth(mWindow *p,int left,int top,int right,int bottom)
{
	p->win.deco.w.left = left;
	p->win.deco.w.top = top;
	p->win.deco.w.right = right;
	p->win.deco.w.bottom = bottom;
}

/**@ ウィンドウ装飾の更新範囲を追加
 *
 * @d:update ハンドラが来た時、更新したい範囲を追加するために実行する。 */

void mWindowDeco_updateBox(mWindow *p,int x,int y,int w,int h)
{
	p->wg.fui |= MWIDGET_UI_DECO_UPDATE;

	__mWindowUpdateRoot(p, x, y, w, h);
}


//===============================
// mToplevel
//===============================


/**@ トップレベルウィンドウ作成
 *
 * @g:mToplevel */

mToplevel *mToplevelNew(mWindow *parent,int size,uint32_t fstyle)
{
	mToplevel *p;

	//タイトル付きの場合は、常に枠あり

	if(fstyle & MTOPLEVEL_S_TITLE)
		fstyle |= MTOPLEVEL_S_BORDER;

	//

	if(size < sizeof(mToplevel))
		size = sizeof(mToplevel);

	p = (mToplevel *)__mWindowNew(MLK_WIDGET(parent), size);
	if(!p) return NULL;

	p->wg.ftype |= MWIDGET_TYPE_WINDOW_TOPLEVEL;
	p->top.fstyle = fstyle;

	//実体を作成

	if(!(MLKAPP->bkend.toplevel_create)(p))
	{
		mWidgetDestroy(MLK_WIDGET(p));
		return NULL;
	}

	return p;
}

/**@ メニューバーを関連付ける
 *
 * @d:これにより、ウィンドウ上で Alt+指定キーが押された時、メニューが選択される。 */

void mToplevelAttachMenuBar(mToplevel *p,mMenuBar *menubar)
{
	if(p->wg.ftype & MWIDGET_TYPE_WINDOW_TOPLEVEL)
		p->top.menubar = menubar;
}

/**@ メニューバーのメニューデータ (mMenu) を取得 */

mMenu *mToplevelGetMenu_menubar(mToplevel *p)
{
	if(p->top.menubar)
		return (p->top.menubar)->menubar.menu;
	else
		return NULL;
}

/**@ アクセラレータを関連付ける
 *
 * @d:※ウィジェット破棄時には自動で削除されない。 */

void mToplevelAttachAccelerator(mToplevel *p,mAccelerator *accel)
{
	p->top.accelerator = accel;
}

/**@ 関連付けられているアクセラレータを削除する */

void mToplevelDestroyAccelerator(mToplevel *p)
{
	mAcceleratorDestroy(p->top.accelerator);

	p->top.accelerator = NULL;
}

/**@ 関連付けられているアクセラレータを取得 */

mAccelerator *mToplevelGetAccelerator(mToplevel *p)
{
	return p->top.accelerator;
}

/**@ ウィンドウタイトルセット */

void mToplevelSetTitle(mToplevel *p,const char *title)
{
	(MLKAPP->bkend.toplevel_settitle)(p, title);

	//装飾

	__mWindowDecoRunUpdate(MLK_WINDOW(p), MWINDOW_DECO_UPDATE_TYPE_TITLE);
}

/**@ PNG ファイルからアイコンセット
 *
 * @p:filename "!/" で始まる場合、データディレクトリからの相対パスとなる。 */

void mToplevelSetIconPNG_file(mToplevel *p,const char *filename)
{
	mImageBuf *img;
	mLoadImageOpen open;
	mLoadImageType type;
	char *buf;

	buf = mGuiGetPath_data_sp(filename);
	if(!buf) return;

	open.type = MLOADIMAGE_OPEN_FILENAME;
	open.filename = buf;

	mLoadImage_checkPNG(&type, 0, 0);

	img = mImageBuf_loadImage(&open, &type, 32, 0);
	if(img)
	{
		(MLKAPP->bkend.toplevel_seticon32)(p, img);

		mImageBuf_free(img);
	}

	mFree(buf);
}

/**@ PNG データバッファからアイコンセット */

void mToplevelSetIconPNG_buf(mToplevel *p,const void *buf,uint32_t bufsize)
{
	mImageBuf *img;
	mLoadImageOpen open;
	mLoadImageType type;

	open.type = MLOADIMAGE_OPEN_BUF;
	open.buf = buf;
	open.size = bufsize;

	mLoadImage_checkPNG(&type, 0, 0);

	img = mImageBuf_loadImage(&open, &type, 32, 0);
	if(img)
	{
		(MLKAPP->bkend.toplevel_seticon32)(p, img);

		mImageBuf_free(img);
	}
}

/**@ 最大化されているか */

mlkbool mToplevelIsMaximized(mToplevel *p)
{
	return ((p->win.fstate & MWINDOW_STATE_MAXIMIZED) != 0);
}

/**@ フルスクリーン状態か */

mlkbool mToplevelIsFullscreen(mToplevel *p)
{
	return ((p->win.fstate & MWINDOW_STATE_FULLSCREEN) != 0);
}

/**@ 最小化 */

void mToplevelMinimize(mToplevel *p)
{
	(MLKAPP->bkend.toplevel_minimize)(p);
}

/**@ 最大化
 *
 * @p:type 0=最大化解除、1以上=最大化、負の値=反転
 * @r:最大化が行われたか */

mlkbool mToplevelMaximize(mToplevel *p,int type)
{
	if(!(p->win.fstate & MWINDOW_STATE_FULLSCREEN))
	{
		if(mGetChangeFlagState(type,
			p->win.fstate & MWINDOW_STATE_MAXIMIZED, &type))
		{
			//バックエンドごとに、実際に状態が変化したら fstate を変更する
		
			(MLKAPP->bkend.toplevel_maximize)(p, type);
			return TRUE;
		}
	}

	return FALSE;
}

/**@ フルスクリーン
 *
 * @p:type 0=フルスクリーン解除、1以上=フルスクリーン化、負の値=反転 */

mlkbool mToplevelFullscreen(mToplevel *p,int type)
{
	if(!(p->win.fstate & MWINDOW_STATE_MAXIMIZED))
	{
		if(mGetChangeFlagState(type,
			p->win.fstate & MWINDOW_STATE_FULLSCREEN, &type))
		{
			(MLKAPP->bkend.toplevel_fullscreen)(p, type);
			return TRUE;
		}
	}
	
	return FALSE;
}

/**@ ユーザーのドラッグによるウィンドウ位置の移動を開始 */

void mToplevelStartDragMove(mToplevel *p)
{
	(MLKAPP->bkend.toplevel_start_drag_move)(p);
}

/**@ ユーザーのドラッグによるウィンドウサイズの変更を開始
 *
 * @p:type 操作する箇所 */

void mToplevelStartDragResize(mToplevel *p,int type)
{
	(MLKAPP->bkend.toplevel_start_drag_resize)(p, type);
}

/**@ ウィンドウ状態の情報を取得 */

void mToplevelGetSaveState(mToplevel *p,mToplevelSaveState *st)
{
	(MLKAPP->bkend.toplevel_get_save_state)(p, st);
}

/**@ ウィンドウ状態を復元する */

void mToplevelSetSaveState(mToplevel *p,mToplevelSaveState *st)
{
	(MLKAPP->bkend.toplevel_set_save_state)(p, st);
}


//===============================
// mDialog
//===============================


/**@ ダイアログ作成
 *
 * @g:mDialog
 *
 * @p:fstyle トップレベルウィンドウスタイル。\
 * MTOPLEVEL_S_DIALOG、MTOPLEVEL_S_PARENT は常に ON。 */

mDialog *mDialogNew(mWindow *parent,int size,uint32_t fstyle)
{
	mDialog *p;
	
	if(size < sizeof(mDialog))
		size = sizeof(mDialog);
	
	p = (mDialog *)mToplevelNew(parent, size,
		fstyle | MTOPLEVEL_S_DIALOG | MTOPLEVEL_S_PARENT);

	if(!p) return NULL;

	p->wg.ftype |= MWIDGET_TYPE_DIALOG;
	p->wg.event = mDialogEventDefault;
	p->wg.fevent |= MWIDGET_EVENT_KEY;
	
	return p;
}

/**@ ダイアログ実行
 * 
 * @d:ウィンドウの表示などはあらかじめ行っておくこと。\
 * destroy を TRUE にして実行した後に、mDialog * を参照しないよう注意。
 * 
 * @p:destroy TRUE で、終了後に自身を削除する */

intptr_t mDialogRun(mDialog *p,mlkbool destroy)
{
	intptr_t ret;
	
	mGuiRunModal(MLK_WINDOW(p));
	
	ret = p->dlg.retval;

	//削除
	
	if(destroy)
		mWidgetDestroy(MLK_WIDGET(p));
	
	return ret;
}

/**@ ダイアログを終了してループを抜ける
 *
 * @p:ret ダイアログの戻り値 */

void mDialogEnd(mDialog *p,intptr_t ret)
{
	p->dlg.retval = ret;
	
	mGuiQuit();
}

/**@ ダイアログ用のデフォルトイベント処理
 *
 * @d:ダイアログごとのイベントハンドラ処理の後、最後にこの関数を実行すること。\
 * ウィンドウの閉じるボタン、または ESC キーが押された場合に、
 * ダイアログを終了する処理が行われる。\
 * (ダイアログの戻り値は、現在値のまま変更されない)
 *
 * @r:イベントを処理した場合、TRUE */

int mDialogEventDefault(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_CLOSE:
			mGuiQuit();
			return TRUE;
		
		case MEVENT_KEYDOWN:
			//ESC で閉じる
			if(!ev->key.is_grab_pointer && ev->key.key == MKEY_ESCAPE)
			{
				mGuiQuit();
				return TRUE;
			}
			break;
	}

	return FALSE;
}

/**@ デフォルトイベント処理 (OK/キャンセルボタン処理)
 *
 * @d:イベント処理後に実行するデフォルトのイベントハンドラ。\
 * ダイアログ終了処理に加え、OK/キャンセルボタンが押された時の処理も行う。\
 * \
 * 通知イベントで MLK_WID_OK or MLK_WID_CANCEL が来た場合、mDialogEnd() で終了させる。\
 * この時、ダイアログの戻り値は、OK 時は TRUE、CANCEL 時は FALSE となる。 */

int mDialogEventDefault_okcancel(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& (ev->notify.id == MLK_WID_OK || ev->notify.id == MLK_WID_CANCEL))
	{
		mDialogEnd(MLK_DIALOG(wg), (ev->notify.id == MLK_WID_OK));
		return TRUE;
	}

	return mDialogEventDefault(wg, ev);
}


//===============================
// mPopup
//===============================


/**@ 作成
 *
 * @g:mPopup
 *
 * @p:fstyle ポップアップのスタイル */

mPopup *mPopupNew(int size,uint32_t fstyle)
{
	mPopup *p;
	
	if(size < sizeof(mPopup))
		size = sizeof(mPopup);
	
	p = (mPopup *)__mWindowNew(NULL, size);
	if(!p) return NULL;

	p->wg.ftype |= MWIDGET_TYPE_WINDOW_POPUP;
	p->pop.fstyle = fstyle;

	//イベント

	if(fstyle & MPOPUP_S_NO_EVENT)
	{
		p->wg.event = NULL;
		p->wg.fevent = 0;
	}
	else
	{
		p->wg.event = mPopupEventDefault;
		p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY;
	}

	return p;
}

/**@ ポップアップを表示
 *
 * @d:メインループを実行せず、表示のみ行う場合に使う。\
 *  不要になったら、手動で破棄すること。
 *
 * @r:ウィンドウが作成＆表示されたか */

mlkbool mPopupShow(mPopup *p,mWidget *parent,int x,int y,int w,int h,mBox *box,uint32_t flags)
{
	p->win.parent = parent;

	return (MLKAPP->bkend.popup_show)(p, x, y, w, h, box, flags);
}

/**@ ポップアップの表示と実行
 *
 * @d:ポップアップを表示して、メインループを開始する。\
 *  関数を抜けた時点で、ポップアップが終了しているため、mPopup は破棄できる (自動で破棄は行われない)。
 *
 * @p:parent ポップアップの親 (通知先)
 * @p:x,y ポップアップの位置。\
 *  box != NULL の場合、基準位置からの相対位置。\
 *  box = NULL の場合、parent からの相対位置。
 * @p:w,h ポップアップのデフォルトの幅と高さ
 * @p:box ポップアップを表示する基準の矩形 (parent からの相対位置)。\
 *  NULL で x,y の位置。
 * @p:flags フラグ (MPOPUP_FLAGS)
 * @r:ポップアップを実行したか */

mlkbool mPopupRun(mPopup *p,mWidget *parent,int x,int y,int w,int h,mBox *box,uint32_t flags)
{
	p->win.parent = parent;

	if(!(MLKAPP->bkend.popup_show)(p, x, y, w, h, box, flags))
		return FALSE;
	else
	{
		mGuiRunPopup(p, NULL);
		return TRUE;
	}
}

/**@ ポップアップを終了してループを抜ける
 *
 * @d:ポップアップの表示中に、明示的に終了したい時に使う。\
 *  quit ハンドラがセットされている場合、quit ハンドラを実行した後、ポップアップを終了する。
 *
 * @p:is_cancel TRUE で、範囲外のクリックや ESC キーなどの、キャンセル動作による終了 */

void mPopupQuit(mPopup *p,mlkbool is_cancel)
{
	//quit ハンドラ

	if(p->pop.quit)
		(p->pop.quit)(MLK_WIDGET(p), is_cancel);

	//終了

	mGuiQuit();
}

/**@ quit ハンドラセット
 *
 * @d:キャンセル動作を含む、ポップアップ終了時に呼ばれる。 */

void mPopupSetHandle_quit(mPopup *p,mFuncPopupQuit handle)
{
	p->pop.quit = handle;
}

/**@ ポップアップ用イベントハンドラ
 *
 * @d:イベント処理の最後に実行する。\
 * ESC キーでの終了処理や、フォーカスアウトによる終了処理が行われる。
 *
 * @r:イベントを処理した場合、TRUE */

int mPopupEventDefault(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_KEYDOWN:
			//ESC キーで終了
			if(!ev->key.is_grab_pointer && ev->key.key == MKEY_ESCAPE)
			{
				mPopupQuit(MLK_POPUP(wg), TRUE);
				return TRUE;
			}
			break;

		case MEVENT_FOCUS:
			//Alt+Tab などの操作により、
			//現在のフォーカスウィンドウがフォーカスアウトした場合
			//[!] ポップアップウィンドウ自体にはフォーカスが来ないため、
			//    元々フォーカスがあったウィンドウのフォーカスが離れた時。

			if(ev->focus.is_out)
			{
				mPopupQuit(MLK_POPUP(wg), TRUE);
				return TRUE;
			}
			break;
	}

	return FALSE;
}


//===============================
// mTooltip
//===============================


#define _TOOLTIP_PADDING_X  5
#define _TOOLTIP_PADDING_Y  4


/* destroy ハンドラ */

static void _tooltip_destroy_handle(mWidget *p)
{
	mWidgetLabelText_free(&MLK_TOOLTIP(p)->ttip.txt);
}

/* draw ハンドラ */

static void _tooltip_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	//枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(TOOLTIP_FRAME));

	//背景

	mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, MGUICOL_PIX(TOOLTIP_FACE));

	//テキスト

	mWidgetLabelText_draw(&MLK_TOOLTIP(wg)->ttip.txt,
		pixbuf, mWidgetGetFont(wg),
		_TOOLTIP_PADDING_X, _TOOLTIP_PADDING_Y, wg->w - 2,
		MGUICOL_RGB(TOOLTIP_TEXT), 0);
}

/**@ チールチップウィンドウ表示
 *
 * @g:mTooltip
 *
 * @p:p 新規作成時は NULL。\
 *  現在表示しているツールチップがある場合にそれを指定すると、そのウィンドウを削除した上で、
 *  新しいツールチップウィンドウを作成する。
 * @p:parent 位置の基準となる親ウィジェット
 * @p:x,y,box ポップアップと同じ
 * @p:popup_flags ポップアップのフラグ
 * @r:ツールチップウィンドウのウィジェットポインタ。\
 *  NULL で表示されなかった。 */

mTooltip *mTooltipShow(mTooltip *p,mWidget *parent,int x,int y,mBox *box,uint32_t popup_flags,
	const char *text,uint32_t tooltip_flags)
{
	mSize size;

	//削除

	mWidgetDestroy(MLK_WIDGET(p));

	//空テキスト

	if(!text || *text == 0) return NULL;

	//作成
	
	p = (mTooltip *)mPopupNew(sizeof(mTooltip), MPOPUP_S_NO_GRAB | MPOPUP_S_NO_EVENT);
	if(!p) return NULL;

	p->wg.destroy = _tooltip_destroy_handle;
	p->wg.draw = _tooltip_draw_handle;

	//ラベルテキストセット

	mWidgetLabelText_set(MLK_WIDGET(p), &p->ttip.txt, text,
		((tooltip_flags & MTOOLTIP_F_COPYTEXT) != 0));

	//ウィンドウサイズ

	mWidgetLabelText_getSize(MLK_WIDGET(p), p->ttip.txt.text, p->ttip.txt.linebuf, &size);

	size.w += _TOOLTIP_PADDING_X * 2;
	size.h += _TOOLTIP_PADDING_Y * 2;

	//表示

	if(!mPopupShow(MLK_POPUP(p), parent, x, y, size.w, size.h, box, popup_flags))
	{
		mWidgetDestroy(MLK_WIDGET(p));
		return NULL;
	}

	return p;
}
