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
 * mButton  [ボタン]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_button.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_key.h"

#include "mlk_pv_widget.h"


//-----------------------

//ボタンの最小サイズ
#define _BTT_DEFW  64
#define _BTT_DEFH  22

//-----------------------


//==========================
// sub
//==========================


/** ボタン/キー押し時
 *
 * key: 押されたキー。-1 でボタンによる押し時 */

static void _on_pressed(mButton *p,int key)
{
	//ボタン押し時、フォーカスをセット

	if(key == -1)
		mWidgetSetFocus_redraw(MLK_WIDGET(p), FALSE);

	//

	if(p->btt.fstyle & MBUTTON_S_DIRECT_PRESS)
	{
		//押し動作なしの場合

		if(p->btt.pressed)
			(p->btt.pressed)(MLK_WIDGET(p));
	}
	else
	{
		//押し動作ありの場合、グラブ

		if(key == -1)
			mWidgetGrabPointer(MLK_WIDGET(p));
		
		p->btt.flags |= MBUTTON_F_GRABBED;
		p->btt.grabbed_key = key;
		
		mButtonSetState(p, TRUE);
	}
}

/** キー押し時、対象キーか判定 */

static mlkbool _is_key_press(uint32_t key)
{
	return (key == MKEY_SPACE || key == MKEY_KP_SPACE
		|| key == MKEY_ENTER || key == MKEY_KP_ENTER);
}

/** グラブ状態を解除
 *
 * run_pressed: pressed ハンドラを実行 */

static void _grab_release(mButton *p,mlkbool run_pressed)
{
	//グラブを解除

	if(p->btt.flags & MBUTTON_F_GRABBED)
	{
		mWidgetUngrabPointer();
	
		p->btt.flags &= ~MBUTTON_F_GRABBED;
		p->btt.grabbed_key = 0;

		mButtonSetState(p, FALSE);
	}
	
	//pressed ハンドラ実行
	
	if(run_pressed && p->btt.pressed)
		(p->btt.pressed)(MLK_WIDGET(p));
}

/** デフォルトの pressed ハンドラ関数 */

static void _pressed_handle(mWidget *p)
{
	mWidgetEventAdd_notify(p, NULL, MBUTTON_N_PRESSED, 0, 0);
}


//==========================
// main
//==========================


/**@ ボタンデータの解放 */

void mButtonDestroy(mWidget *p)
{
	mWidgetLabelText_free(&MLK_BUTTON(p)->btt.txt);
}

/**@ ボタン作成 */

mButton *mButtonNew(mWidget *parent,int size,uint32_t fstyle)
{
	mButton *p;
	
	if(size < sizeof(mButton))
		size = sizeof(mButton);
	
	p = (mButton *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mButtonDestroy;
	p->wg.calc_hint = mButtonHandle_calcHint;
	p->wg.draw = mButtonHandle_draw;
	p->wg.event = mButtonHandle_event;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY;
	p->wg.facceptkey = MWIDGET_ACCEPTKEY_ENTER;

	p->btt.fstyle = fstyle;
	p->btt.pressed = _pressed_handle;
	
	return p;
}

/**@ ボタン作成  */

mButton *mButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,
	uint32_t fstyle,const char *text)
{
	mButton *p;

	p = mButtonNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	mWidgetLabelText_set(MLK_WIDGET(p), &p->btt.txt, text,
		fstyle & MBUTTON_S_COPYTEXT);

	return p;
}

/**@ テキストセット
 *
 * @d:テキストを複製するかは、スタイルフラグによる */

void mButtonSetText(mButton *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->btt.txt, text,
		p->btt.fstyle & MBUTTON_S_COPYTEXT);
}

/**@ テキストをセット (複製)
 *
 * @d:スタイルフラグは変更しない。 */

void mButtonSetText_copy(mButton *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->btt.txt, text, TRUE);
}

/**@ ボタン押し状態 (描画) を変更 */

void mButtonSetState(mButton *p,mlkbool pressed)
{
	int now;
	
	now = p->btt.flags & MBUTTON_F_PRESSED;

	//状態が変化する時
	
	if((!now) != (!pressed))
	{
		p->btt.flags ^= MBUTTON_F_PRESSED;
		
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ ボタンが押された状態か */

mlkbool mButtonIsPressed(mButton *p)
{
	return ((p->btt.flags & MBUTTON_F_PRESSED) != 0);
}

/**@ ボタンのベースを描画 */

void mButtonDrawBase(mButton *p,mPixbuf *pixbuf)
{
	uint8_t flags = 0;

	if(p->btt.flags & MBUTTON_F_PRESSED)
		flags |= MPIXBUF_DRAWBTT_PRESSED;

	if(p->wg.fstate & MWIDGET_STATE_FOCUSED)
		flags |= MPIXBUF_DRAWBTT_FOCUSED;

	if(!mWidgetIsEnable(MLK_WIDGET(p)))
		flags |= MPIXBUF_DRAWBTT_DISABLED;

	if(p->wg.fstate & MWIDGET_STATE_ENTER_SEND)
		flags |= MPIXBUF_DRAWBTT_DEFAULT_BUTTON;
	
	mPixbufDrawButton(pixbuf, 0, 0, p->wg.w, p->wg.h, flags);
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mButtonHandle_calcHint(mWidget *wg)
{
	mButton *p = MLK_BUTTON(wg);
	mSize size;
	int w,h;

	mWidgetLabelText_onCalcHint(wg, &p->btt.txt, &size);

	w = size.w;
	h = size.h;

	//幅

	if(p->btt.fstyle & MBUTTON_S_REAL_W)
		w += 8;
	else
	{
		w += 10;

		if(w < _BTT_DEFW)
		{
			w = _BTT_DEFW;
			//テキスト以外の余白は偶数にする
			if((w - size.w) & 1) w++;
		}
	}

	//高さ
	
	if(p->btt.fstyle & MBUTTON_S_REAL_H)
		h += 6;
	else
	{
		h += 8;
	
		if(h < _BTT_DEFH)
		{
			h = _BTT_DEFH;
			if((h - size.h) & 1) h++;
		}
	}

	//REAL_W 時、幅が高さより小さければ、高さに合わせる

	if((p->btt.fstyle & MBUTTON_S_REAL_W) && w < h)
		w = h;
	
	wg->hintW = w;
	wg->hintH = h;
}

/**@ event ハンドラ関数 */

int mButtonHandle_event(mWidget *wg,mEvent *ev)
{
	mButton *p = MLK_BUTTON(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し
			
				if(ev->pt.btt == MLK_BTT_LEFT
					&& !(p->btt.flags & MBUTTON_F_GRABBED))
					_on_pressed(p, -1);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
			
				if(ev->pt.btt == MLK_BTT_LEFT
					&& (p->btt.flags & MBUTTON_F_GRABBED)
					&& p->btt.grabbed_key == -1)
					_grab_release(p, TRUE);
			}
			return TRUE;

		//キー押し
		case MEVENT_KEYDOWN:
			if(_is_key_press(ev->key.key))
			{
				//ボタン押し動作
				// MWIDGET_STATE_ENTER_SEND が ON の場合、
				// フォーカスがない状態でキーイベントが来る場合あり。
				
				if(!(p->btt.flags & MBUTTON_F_GRABBED))
				{
					_on_pressed(p, ev->key.raw_code);
					return TRUE;
				}
			}
			else
				//矢印キーで移動
				__mWidgetMoveFocus_arrowkey(wg, ev->key.key);
			break;

		//キー離し
		case MEVENT_KEYUP:
			if((p->btt.flags & MBUTTON_F_GRABBED)
				&& p->btt.grabbed_key == ev->key.raw_code)
			{
				_grab_release(p, TRUE);
				return TRUE;
			}
			break;

		//フォーカス
		case MEVENT_FOCUS:
			//フォーカスアウト時、グラブ状態解除
			if(ev->focus.is_out)
				_grab_release(p, FALSE);
					
			mWidgetRedraw(wg);
			return TRUE;
	}

	return FALSE;
}

/**@ draw ハンドラ関数 */

void mButtonHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mButton *p = MLK_BUTTON(wg);
	int add;

	add = ((p->btt.flags & MBUTTON_F_PRESSED) != 0);
	
	//ボタン
	
	mButtonDrawBase(p, pixbuf);
	
	//テキスト

	mWidgetLabelText_draw(&p->btt.txt, pixbuf, mWidgetGetFont(wg),
		add, add + (wg->h - p->btt.txt.szfull.h) / 2, wg->w,
		(mWidgetIsEnable(wg))? MGUICOL_RGB(TEXT): MGUICOL_RGB(TEXT_DISABLE),
		MWIDGETLABELTEXT_DRAW_F_CENTER);
}

