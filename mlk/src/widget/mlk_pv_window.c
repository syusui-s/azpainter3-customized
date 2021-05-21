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
 * mWindow 内部関数
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_rectbox.h"
#include "mlk_window_deco.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"


/** mWindow データ作成
 *
 * ウィンドウの実体は作成しない。 */

mWindow *__mWindowNew(mWidget *parent,int size)
{
	mWindow *p;
	
	if(size < sizeof(mWindow))
		size = sizeof(mWindow);
	
	p = (mWindow *)mContainerNew(NULL, size);
	if(!p) return NULL;

	p->wg.draw_bkgnd = mWidgetDrawBkgndHandle_fillFace;
	p->wg.ftype |= MWIDGET_TYPE_WINDOW;
	p->wg.fstate &= ~MWIDGET_STATE_VISIBLE;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	p->win.parent = parent;

	//バックエンドデータ確保

	if(!(MLKAPP->bkend.window_alloc)(&p->win.bkend))
	{
		mWidgetDestroy(MLK_WIDGET(p));
		return NULL;
	}
	
	return p;
}


//====================
// 更新範囲
//====================


/** 更新範囲追加 (装飾を含む座標) */

void __mWindowUpdateRootRect(mWindow *p,mRect *rc)
{
	if(p->wg.fui & MWIDGET_UI_UPDATE)
		mRectUnion(&p->win.update_rc, rc);
	else
	{
		p->wg.fui |= MWIDGET_UI_UPDATE;
		p->win.update_rc = *rc;
	}
}

/** 更新範囲追加 (装飾含む座標) */

void __mWindowUpdateRoot(mWindow *p,int x,int y,int w,int h)
{
	mRect rc;

	rc.x1 = x;
	rc.y1 = y;
	rc.x2 = x + w - 1;
	rc.y2 = y + h - 1;

	__mWindowUpdateRootRect(p, &rc);
}

/** 更新範囲追加 (装飾を含まない座標) */

void __mWindowUpdateRect(mWindow *p,mRect *rc)
{
	rc->x1 += p->win.deco.w.left;
	rc->x2 += p->win.deco.w.left;
	rc->y1 += p->win.deco.w.top;
	rc->y2 += p->win.deco.w.top;

	__mWindowUpdateRootRect(p, rc);
}

/** 更新範囲追加 (装飾を含まない座標) */

void __mWindowUpdate(mWindow *p,int x,int y,int w,int h)
{
	mRect rc;

	rc.x1 = p->win.deco.w.left + x;
	rc.y1 = p->win.deco.w.top + y;
	rc.x2 = rc.x1 + w - 1;
	rc.y2 = rc.y1 + h - 1;

	__mWindowUpdateRootRect(p, &rc);
}


//===========================
// ウィンドウサイズ・状態
//===========================


/** ウィンドウサイズから、装飾を除くサイズを取得 */

void __mWindowGetSize_excludeDecoration(mWindow *p,int w,int h,mSize *size)
{
	w -= p->win.deco.w.left + p->win.deco.w.right;
	h -= p->win.deco.w.top + p->win.deco.w.bottom;

	if(w < 1) w = 1;
	if(h < 1) h = 1;

	size->w = w;
	size->h = h;
}

/** ウィンドウサイズを明示的に変更する際のリサイズ処理
 *
 * 通常ウィンドウ時のみ。最大化/フルスクリーン化の状態では実行されない。
 *
 * w,h: 装飾を含まないサイズ */

void __mWindowOnResize_setNormal(mWindow *p,int w,int h)
{
	//装飾幅をセット

	__mWindowDecoRunSetWidth(p, MWINDOW_DECO_SETWIDTH_TYPE_NORMAL);
	
	//ウィンドウサイズ (装飾分を追加)

	w += p->win.deco.w.left + p->win.deco.w.right;
	h += p->win.deco.w.top  + p->win.deco.w.bottom;

	p->win.win_width  = w;
	p->win.win_height = h;

	//通常ウィンドウサイズがセットされていない場合はセット

	if(p->win.norm_width == 0 || p->win.norm_height == 0)
	{
		p->win.norm_width  = w;
		p->win.norm_height = h;
	}

	//mPixbuf 作成 or リサイズ

	if(p->win.pixbuf)
		mPixbufResizeStep(p->win.pixbuf, w, h, 0, 0);
	else
		p->win.pixbuf = mPixbufCreate(w, h, 0);

	//ウィンドウサイズ変更

	(MLKAPP->bkend.window_resize)(p, w, h);

	//レイアウト関連

	__mWidgetOnResize(MLK_WIDGET(p));

	//装飾更新範囲セット

	__mWindowDecoRunUpdate(p, MWINDOW_DECO_UPDATE_TYPE_RESIZE);
}

/** イベントによってウィンドウサイズの変更が通知された時
 *
 * CONFIGURE が追加される時、または状態変更時。
 *
 * w,h: ウィンドウのサイズ (装飾含む)。
 *      w が負の値で、状態変更時。 */

void __mWindowOnResize_configure(mWindow *p,int w,int h)
{
	mSize size;

	if(w < 0)
	{
		//最大化などの状態変更時

		w = p->win.win_width;
		h = p->win.win_height;
	}
	else
	{
		p->win.win_width  = w;
		p->win.win_height = h;

		//通常ウィンドウ時は、最大化/フルスクリーンを戻す時のサイズとして記録

		if(MLK_WINDOW_IS_NORMAL_SIZE(p))
		{
			p->win.norm_width  = w;
			p->win.norm_height = h;
		}
	}

	//装飾を含まないサイズ

	__mWindowGetSize_excludeDecoration(p, w, h, &size);

	p->wg.w = size.w;
	p->wg.h = size.h;

	//mPixbuf リサイズ

	if(!p->win.pixbuf)
		p->win.pixbuf = mPixbufCreate(w, h, 0);
	else
		mPixbufResizeStep(p->win.pixbuf, w, h, 32, 32);

	//再レイアウトと再描画

	__mWidgetOnResize(MLK_WIDGET(p));

	//装飾更新範囲セット

	__mWindowDecoRunUpdate(p, MWINDOW_DECO_UPDATE_TYPE_RESIZE);
}

/** CONFIGURE イベントでの状態変更時 */

void __mWindowOnConfigure_state(mWindow *p,uint32_t mask)
{
	int n;

	//最大化/フルスクリーン状態変更時

	if(mask & (MEVENT_CONFIGURE_STATE_MAXIMIZED | MEVENT_CONFIGURE_STATE_FULLSCREEN))
	{
		//装飾幅セット

		if(p->win.fstate & MWINDOW_STATE_MAXIMIZED)
			n = MWINDOW_DECO_SETWIDTH_TYPE_MAXIMIZED;
		else if(p->win.fstate & MWINDOW_STATE_FULLSCREEN)
			n = MWINDOW_DECO_SETWIDTH_TYPE_FULLSCREEN;
		else
			n = MWINDOW_DECO_SETWIDTH_TYPE_NORMAL;

		__mWindowDecoRunSetWidth(p, n);

		//装飾幅変更によるサイズ変更
		//(状態変化後、ウィンドウサイズの変化がない場合に、必要になる)

		__mWindowOnResize_configure(p, -1, -1);
	}
}


//==========================
// フォーカスなど
//==========================


/** ウィンドウ内のフォーカスウィジェット変更
 *
 * フォーカスを変更できる状態の場合、現在のフォーカスを解除し、新しいフォーカスをセット。
 * 現時点でウィンドウにフォーカスがない場合は、focus_ready にセットしておく。
 *
 * window: フォーカスをセットするウィンドウ
 * focus:  新しいフォーカスウィジェット (window の子)。NULL で解除。
 * from:   フォーカスイベントの from に設定する値
 * return: 実際にフォーカスが変更されたか */

mlkbool __mWindowSetFocus(mWindow *win,mWidget *focus,int from)
{
	mWidget *old = win->win.focus,*wg;

	//変更なし

	if(focus == old) return FALSE;

	//focus が、フォーカスをセットできる状態にあるか

	if(focus)
	{
		//フォーカスを受け取れない
		
		if(!(focus->fstate & MWIDGET_STATE_TAKE_FOCUS))
			return FALSE;

		//親で MWIDGET_OPTION_NO_TAKE_FOCUS がセットされている

		for(wg = focus; wg->parent; wg = wg->parent)
		{
			if(wg->foption & MWIDGET_OPTION_NO_TAKE_FOCUS)
				return FALSE;
		}

		//<無効状態>
		// ウィンドウが非表示の状態でフォーカスを変更したい場合があるので、
		// 無効判定は、現在フォーカスがあるウィンドウ上のみ

		if(MLKAPP->window_focus == win
			&& !mWidgetIsEnable(focus))
			return FALSE;
	}

	//現在のフォーカスを解除

	if(old)
	{
		//イベントによるウィンドウの FocusOut の場合、
		//再びフォーカスが戻ってきた時のために、現在のフォーカスを記録しておく

		if(from == MEVENT_FOCUS_FROM_WINDOW_FOCUS)
		{
			win->win.focus_ready = old;
			win->win.focus_ready_type = MWINDOW_FOCUS_READY_TYPE_FOCUSOUT_WINDOW;
		}

		//解除

		if(old->fstate & MWIDGET_STATE_FOCUSED)
		{
			old->fstate &= ~MWIDGET_STATE_FOCUSED;

			mEventListAdd_focus(old, TRUE, from);
		}
	}

	//フォーカスセット

	if(!focus)
		win->win.focus = NULL;
	else
	{
		//現時点でウィンドウにフォーカスがない状態でセットしようとした時
		//(初期表示時に最初のフォーカスを指定したい時など) は、
		//focus_ready にセットしておくと、ウィンドウにフォーカスが来た時にセットさせる。
		
		//[!] ポップアップウィンドウにはフォーカスは付加しない
	
		if(MLKAPP->window_focus != win)
		{
			//ウィンドウにフォーカスがない: 記録しておく

			win->win.focus_ready = focus;
			win->win.focus_ready_type = MWINDOW_FOCUS_READY_TYPE_NO_FOCUS;
		}
		else
		{
			//フォーカスセット

			win->win.focus = focus;

			focus->fstate |= MWIDGET_STATE_FOCUSED;

			mEventListAdd_focus(focus, FALSE, from);
		}
	}

	return TRUE;
}

/** ウィンドウから、有効 & 表示 & fstate の指定フラグが ON の最初のウィジェット取得
 *
 * is_focus: TRUE でフォーカス対象を取得
 * return: 一つもなければ NULL */

mWidget *__mWindowGetStateEnable_first(mWindow *win,uint32_t flags,mlkbool is_focus)
{
	mWidget *p;
	mlkbool enable;

	p = win->wg.first;

	while(p)
	{
		//フォーカス時:NO_TAKE_FOCUS がある場合、子を検索しない

		if(is_focus && (p->foption & MWIDGET_OPTION_NO_TAKE_FOCUS))
		{
			p = __mWidgetGetTreeNextPass_root(p, MLK_WIDGET(win));
			continue;
		}

		//有効状態か
		//(上位から順に判定するので、p のウィジェットのフラグだけで問題ない)
		
		enable = ((p->fstate & (MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED))
					== (MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED));
	
		if(enable && (p->fstate & flags))
			return p;

		//次へ

		if(p->first && enable)
			//親が有効で、子があるなら、子を検索
			p = p->first;
		else
			p = __mWidgetGetTreeNextPass_root(p, MLK_WIDGET(win));
	}
	
	return NULL;
}

/** フォーカスをセット可能なウィジェットを、先頭から検索
 *
 * win 自体は含まない。
 * (フォーカスウィジェットがない場合、キーイベントはウィンドウに送られるため) */

mWidget *__mWindowGetTakeFocus_first(mWindow *win)
{
	return __mWindowGetStateEnable_first(win, MWIDGET_STATE_TAKE_FOCUS, TRUE);
}

/** 現在のフォーカスウィジェットの次に対象となるウィジェットを取得
 *
 * 次のウィジェットがなければ、先頭から検索。 */

mWidget *__mWindowGetTakeFocus_next(mWindow *win)
{
	mWidget *p,*wg;
	mlkbool flag;

	//フォーカスウィジェットがなければ先頭から

	if(!win->win.focus)
		return __mWindowGetTakeFocus_first(win);

	//次を検索

	p = __mWidgetGetTreeNext_root(win->win.focus, MLK_WIDGET(win));

	while(p)
	{
		if((p->fstate & MWIDGET_STATE_TAKE_FOCUS) && mWidgetIsEnable(p))
		{
			//オプションを判定
			
			flag = TRUE;
			
			for(wg = p; wg->parent; wg = wg->parent)
			{
				if(wg->foption & MWIDGET_OPTION_NO_TAKE_FOCUS)
				{
					flag = FALSE;
					break;
				}
			}
		
			if(flag) return p;
		}

		p = __mWidgetGetTreeNext_root(p, MLK_WIDGET(win));
	}

	//見つからなければ、先頭から再度検索

	if(p)
		return p;
	else
		return __mWindowGetTakeFocus_first(win);
}

/** ウィンドウにフォーカスがセットされた時のフォーカス対象ウィジェットを取得
 *
 * (FOCUSIN イベントで、他のウィンドウから win にフォーカスが移った時)
 *
 * dst_from: フォーカスイベントの from 値が入る */

mWidget *__mWindowGetDefaultFocusWidget(mWindow *win,int *dst_from)
{
	mWidget *wg;

	*dst_from = MEVENT_FOCUS_FROM_WINDOW_FOCUS;

	wg = win->win.focus_ready;

	if(!wg)
	{
		//復元するフォーカスがない場合は、フォーカスをセット可能な最初のウィジェット

		return __mWindowGetTakeFocus_first(win);
	}
	else
	{
		//ウィンドウのフォーカスアウト後、再びフォーカスがセットされた場合
	
		if(win->win.focus_ready_type == MWINDOW_FOCUS_READY_TYPE_FOCUSOUT_WINDOW)
			*dst_from = MEVENT_FOCUS_FROM_WINDOW_RESTORE;

		//現時点で wg がフォーカスをセットできない状態の場合は、
		//フォーカスセット時に弾かれるので問題ない。

		return wg;
	}
}

/** ウィンドウ内の次のフォーカスへ移動
 *
 * return: TRUE で移動した */

mlkbool __mWindowMoveFocusNext(mWindow *win)
{
	mWidget *wg;

	wg = __mWindowGetTakeFocus_next(win);
	
	if(!wg)
		return FALSE;
	else
	{
		__mWindowSetFocus(win, wg, MEVENT_FOCUS_FROM_KEY_MOVE);
		return TRUE;
	}
}

/** D&D 時、ドロップ先のウィジェット取得
 *
 * x,y: 装飾含むウィンドウ座標 */

mWidget *__mWindowGetDNDDropTarget(mWindow *win,int x,int y)
{
	mWidget *p;

	x -= win->win.deco.w.left;
	y -= win->win.deco.w.top;

	//指定位置上の最下位ウィジェット

	p = mWidgetGetWidget_atPoint(MLK_WIDGET(win), x, y);

	//ドロップ可能なウィジェットを親方向に検索

	for(; p->parent; p = p->parent)
	{
		if(p->fstate & MWIDGET_STATE_ENABLE_DROP)
			return p;
	}

	return NULL;
}

/** ポップアップ用。
 * 表示時の基準矩形からポップアップの矩形を取得
 *
 * rc: 基準矩形
 * flip: bit0:X反転、bit1:Y反転 */

void __mPopupGetRect(mRect *rcdst,mRect *rc,int w,int h,uint32_t f,int flip)
{
	int x,y;
	uint32_t ftmp;

	//X 反転

	if(flip & 1)
	{
		ftmp = f;
		f &= ~(MPOPUP_F_LEFT | MPOPUP_F_RIGHT | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_GRAVITY_RIGHT);

		if(ftmp & MPOPUP_F_LEFT)
			f |= MPOPUP_F_RIGHT;
		else if(ftmp & MPOPUP_F_RIGHT)
			f |= MPOPUP_F_LEFT;

		if(ftmp & MPOPUP_F_GRAVITY_LEFT)
			f |= MPOPUP_F_GRAVITY_RIGHT;
		else
			f |= MPOPUP_F_GRAVITY_LEFT;
	}

	//Y 反転

	if(flip & 2)
	{
		ftmp = f;
		f &= ~(MPOPUP_F_TOP | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_TOP | MPOPUP_F_GRAVITY_BOTTOM);

		if(ftmp & MPOPUP_F_TOP)
			f |= MPOPUP_F_BOTTOM;
		else if(ftmp & MPOPUP_F_BOTTOM)
			f |= MPOPUP_F_TOP;

		if(ftmp & MPOPUP_F_GRAVITY_TOP)
			f |= MPOPUP_F_GRAVITY_BOTTOM;
		else
			f |= MPOPUP_F_GRAVITY_TOP;
	}

	//基点X
	
	if(f & MPOPUP_F_LEFT)
		x = rc->x1;
	else if(f & MPOPUP_F_RIGHT)
		x = rc->x2;
	else
		x = rc->x1 + (rc->x2 - rc->x1 - w) / 2;

	//基点Y

	if(f & MPOPUP_F_TOP)
		y = rc->y1;
	else if(f & MPOPUP_F_BOTTOM)
		y = rc->y2;
	else
		y = rc->y1 + (rc->y2 - rc->y1 - h) / 2;

	//X方向 (デフォルトで RIGHT)

	if(f & MPOPUP_F_GRAVITY_LEFT)
	{
		rcdst->x1 = x - w;
		rcdst->x2 = x - 1;
	}
	else
	{
		rcdst->x1 = x;
		rcdst->x2 = x + w - 1;
	}

	//Y方向 (デフォルトで BOTTOM)

	if(f & MPOPUP_F_GRAVITY_TOP)
	{
		rcdst->y1 = y - h;
		rcdst->y2 = y - 1;
	}
	else
	{
		rcdst->y1 = y;
		rcdst->y2 = y + h - 1;
	}
}


//==============================
// mWindowDeco
//==============================


/** mWindowDecoInfo に値をセット */

void __mWindowDecoSetInfo(mWindow *p,mWindowDecoInfo *dst)
{
	dst->width = p->win.win_width;
	dst->height = p->win.win_height;
	dst->left = p->win.deco.w.left;
	dst->top = p->win.deco.w.top;
	dst->right = p->win.deco.w.right;
	dst->bottom = p->win.deco.w.bottom;
}

/** 装飾の幅をセットするハンドラを実行 */

void __mWindowDecoRunSetWidth(mWindow *p,int type)
{
	if(p->win.deco.setwidth)
		(p->win.deco.setwidth)(MLK_WIDGET(p), type);
}

/** 装飾の更新範囲追加ハンドラを実行 */

void __mWindowDecoRunUpdate(mWindow *p,int type)
{
	if(p->win.deco.update)
	{
		mWindowDecoInfo info;

		__mWindowDecoSetInfo(p, &info);
		
		(p->win.deco.update)(MLK_WIDGET(p), type, &info);
	}
}

