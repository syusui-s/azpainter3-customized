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
 * mLineEdit [一行テキスト入力]
 *****************************************/

#include <stdlib.h> //strtod
#include <stdio.h>	//snprintf

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_lineedit.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_key.h"
#include "mlk_str.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_string.h"
#include "mlk_unicode.h"
#include "mlk_util.h"

#include "mlk_pv_widget.h"
#include "mlk_widgettextedit.h"


//-------------------------

#define _SPIN_W    32
#define _PADDING   2  		//テキストの余白 (枠線含まない)
#define _LABEL_PADDING 3	//単位ラベルの余白

//fpress
#define _PRESS_TEXT       1
#define _PRESS_SPIN_UP    2
#define _PRESS_SPIN_DOWN  3

//timer id
#define _TIMERID_SPIN_START   0
#define _TIMERID_SPIN_REPEAT  1

//--------------------------



//=====================================
// sub
//=====================================


/** 枠部分の余白を取得*/

static int _get_padding(mLineEdit *p)
{
	return (p->le.fstyle & MLINEEDIT_S_NO_FRAME)? 0: _PADDING + 1;
}

/** テキスト部分の右端 X 位置取得 */

static int _get_right_x(mLineEdit *p)
{
	int x;

	x = p->wg.w - _get_padding(p);

	if(p->le.fstyle & MLINEEDIT_S_SPIN)
		x -= _SPIN_W;

	if(p->le.label.text)
		x -= p->le.label.szfull.w + _LABEL_PADDING * 2;

	return x;
}

/** スクロール位置調整 (カーソルが見えるように)
 *
 * カーソル位置がウィジェット範囲外の場合、調整 */

static void _adjust_scroll_cursor(mLineEdit *p)
{
	int x,w;
	
	x = p->le.dat.curx;
	w = _get_right_x(p) - _get_padding(p);
	
	if(x - p->le.scrx < 0)
		//カーソルが左に隠れている
		p->le.scrx = x;
	else if(x - p->le.scrx >= w)
		//カーソルが右に隠れている
		p->le.scrx = x - w + 1;
}

/** スクロール最大位置調整
 *
 * 現在のスクロール位置と最大位置を比較 */

static void _adjust_scroll_max(mLineEdit *p)
{
	int maxw,w;

	maxw = p->le.dat.maxwidth;
	w = _get_right_x(p) - _get_padding(p);

	if(maxw <= w)
		//ウィジェット幅の方が大きい
		p->le.scrx = 0;
	else if(p->le.scrx > maxw + 1 - w)
		//テキスト最大位置が右端になるように
		p->le.scrx = maxw + 1 - w;
}

/** カーソル位置変更時
 *
 * adjust_scrmax: スクロール最大値調整
 *    (カーソル位置を任意の位置に移動した時など) */

static void _change_curpos(mLineEdit *p,mlkbool adjust_scrmax)
{
	//スクロール位置調整

	if(adjust_scrmax)
		_adjust_scroll_max(p);

	_adjust_scroll_cursor(p);
}

/** 文字列変更時 */

static void _change_text(mLineEdit *p)
{
	_change_curpos(p, TRUE);

	//CHANGE 通知

	if(p->le.fstyle & MLINEEDIT_S_NOTIFY_CHANGE)
	{
		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
			MLINEEDIT_N_CHANGE, 0, 0);
	}
}

/** ポインタ位置から文字位置取得 */

static int _get_txtpos_point(mLineEdit *p,int ptx)
{
	uint32_t *pt;
	uint16_t *pw;
	int x,w,i;

	//文字先頭を 0 とした位置
	
	ptx = ptx - _get_padding(p) + p->le.scrx;

	if(ptx <= 0)
		return 0;
	else if(ptx >= p->le.dat.maxwidth)
		return p->le.dat.textlen;

	//テキスト範囲内

	pt = p->le.dat.unibuf;
	pw = p->le.dat.wbuf;
	if(!pt) return 0;
	
	for(i = x = 0; *pt; pt++, i++)
	{
		w = *(pw++);
		
		//文字幅の半分以上なら次の文字
		
		if(x <= ptx && ptx < x + w)
			return i + (ptx - x >= w / 2);
		
		x += w;
	}
	
	return i;
}

/** スピン操作による値の上下 */

static void _spin_updown(mLineEdit *p,mlkbool up)
{
	int n;
	
	n = mLineEditGetNum(p);
	n += (up)? p->le.spin_val: -(p->le.spin_val);
	
	mLineEditSetNum(p, n);
	
	_change_text(p);
}

/** resize ハンドラ関数 */

static void _resize_handle(mWidget *p)
{
	//スクロール調整
	
	_adjust_scroll_max(MLK_LINEEDIT(p));
}

/* IM 表示位置取得ハンドラ */

static void _get_inputmethod_pos_handle(mWidget *wg,mPoint *pt)
{
	mLineEdit *p = MLK_LINEEDIT(wg);
	int pad;

	pad = _get_padding(p);

	pt->x = p->le.dat.curx - p->le.scrx + pad;
	pt->y = pad;
}


//=====================================
// メイン
//=====================================


/**@ mLineEdit のデータ解放 */

void mLineEditDestroy(mWidget *p)
{
	mWidgetTextEdit_free(&MLK_LINEEDIT(p)->le.dat);
	mWidgetLabelText_free(&MLK_LINEEDIT(p)->le.label);
}

/**@ 作成 */

mLineEdit *mLineEditNew(mWidget *parent,int size,uint32_t fstyle)
{
	mLineEdit *p;
	
	if(size < sizeof(mLineEdit))
		size = sizeof(mLineEdit);
	
	p = (mLineEdit *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mLineEditDestroy;
	p->wg.calc_hint = mLineEditHandle_calcHint;
	p->wg.draw = mLineEditHandle_draw;
	p->wg.event = mLineEditHandle_event;
	p->wg.resize = _resize_handle;
	p->wg.get_inputmethod_pos = _get_inputmethod_pos_handle;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_KEY_REPEAT
		| MWIDGET_STATE_ENABLE_INPUT_METHOD;

	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY
		| MWIDGET_EVENT_CHAR | MWIDGET_EVENT_STRING | MWIDGET_EVENT_SCROLL;

	if(fstyle & MLINEEDIT_S_NOTIFY_ENTER)
		p->wg.facceptkey |= MWIDGET_ACCEPTKEY_ENTER;

	//

	p->le.fstyle = fstyle;
	p->le.spin_val = 1;

	mWidgetTextEdit_init(&p->le.dat, MLK_WIDGET(p), FALSE);
	
	return p;
}

/**@ 作成 */

mLineEdit *mLineEditCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mLineEdit *p;

	p = mLineEditNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ calc_hint ハンドラ関数 */

void mLineEditHandle_calcHint(mWidget *wg)
{
	mLineEdit *p = MLK_LINEEDIT(wg);
	mSize size;
	int pad;

	//ラベル

	mWidgetLabelText_onCalcHint(wg, &p->le.label, &size);
	if(size.w) size.w += _LABEL_PADDING * 2;

	//

	pad = _get_padding(p);

	wg->hintW = pad * 2 + 4 + size.w;
	wg->hintH = pad * 2 + mWidgetGetFontHeight(wg);
	
	if(p->le.fstyle & MLINEEDIT_S_SPIN)
	{
		wg->hintW += _SPIN_W;
		if(wg->hintH < 13) wg->hintH = 13;
	}
}

/**@ 単位用などのラベルテキストをセット
 *
 * @p:copy 0 で静的文字列、それ以外で文字列をコピーする */

void mLineEditSetLabelText(mLineEdit *p,const char *text,int copy)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->le.label, text, copy);
}

/**@ 数値情報セット */

void mLineEditSetNumStatus(mLineEdit *p,int min,int max,int dig)
{
	p->le.num_min = min;
	p->le.num_max = max;
	p->le.num_dig = dig;
}

/**@ 半角数字の幅を元に、文字長さから幅をセット
 *
 * @d:hintRepW にセットされる。\
 *  ※単位ラベルの幅は計算に含まれない。 */

void mLineEditSetWidth_textlen(mLineEdit *p,int len)
{
	int w;

	w = mFontGetTextWidth(mWidgetGetFont(MLK_WIDGET(p)), "9", 1);
	if(w < 8) w = 8;

	//+1 はカーソル分
	p->wg.hintRepW = w * len + _get_padding(p) * 2 + 1;

	if(p->le.fstyle & MLINEEDIT_S_SPIN)
		p->wg.hintRepW += _SPIN_W;
}

/**@ スピン操作時の増減幅をセット */

void mLineEditSetSpinValue(mLineEdit *p,int val)
{
	p->le.spin_val = val;
}

/**@ 読み取り専用の状態を変更
 *
 * @p:type [0] OFF [正] ON [負] 反転 */

void mLineEditSetReadOnly(mLineEdit *p,int type)
{
	if(mIsChangeFlagState(type, p->le.fstyle & MLINEEDIT_S_READ_ONLY))
	{
		p->le.fstyle ^= MLINEEDIT_S_READ_ONLY;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}


//========= 


/**@ テキストセット */

void mLineEditSetText(mLineEdit *p,const char *text)
{
	mWidgetTextEdit_setText(&p->le.dat, text);

	_change_curpos(p, TRUE);
	
	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ UTF-32 テキストセット */

void mLineEditSetText_utf32(mLineEdit *p,const mlkuchar *text)
{
	char *utf8;

	if(text)
		utf8 = mUTF32toUTF8_alloc(text, -1, NULL);
	else
		utf8 = NULL;

	mLineEditSetText(p, utf8);

	mFree(utf8);
}

/**@ int 値を文字列にしてセット */

void mLineEditSetInt(mLineEdit *p,int n)
{
	char m[32];
	
	mIntToStr(m, n);
	
	mLineEditSetText(p, m);
}

/**@ 数値セット */

void mLineEditSetNum(mLineEdit *p,int num)
{
	char m[32];

	if(num < p->le.num_min)
		num = p->le.num_min;
	else if(num > p->le.num_max)
		num = p->le.num_max;
	
	mIntToStr_float(m, num, p->le.num_dig);
	
	mLineEditSetText(p, m);
}

/**@ double 値セット
 *
 * @d:小数点以下が 0 の場合は、通常の数値扱いとする。
 * 
 * @p:dig 小数点以下の桁数 */

void mLineEditSetDouble(mLineEdit *p,double d,int dig)
{
	double f;
	char m[64];

	f = d - (int)d;

	if(f == 0)
		mLineEditSetInt(p, (int)d);
	else
	{
		snprintf(m, 64, "%.*f", dig, d);
		mLineEditSetText(p, m);
	}
}


//====================


/**@ テキストが空かどうか */

mlkbool mLineEditIsEmpty(mLineEdit *p)
{
	return mWidgetTextEdit_isEmpty(&p->le.dat);
}

/**@ テキスト取得 */

void mLineEditGetTextStr(mLineEdit *p,mStr *str)
{
	mStrSetText_utf32(str, p->le.dat.unibuf, p->le.dat.textlen);
}

/**@ int 値に変換して取得 */

int mLineEditGetInt(mLineEdit *p)
{
	return mUTF32toInt_float(p->le.dat.unibuf, 0);
}

/**@ double 値に変換して取得 */

double mLineEditGetDouble(mLineEdit *p)
{
	char *buf;
	double d;
	
	buf = mUTF32toUTF8_alloc(p->le.dat.unibuf, p->le.dat.textlen, NULL);
	if(!buf) return 0;

	d = strtod(buf, NULL);
	
	mFree(buf);
	
	return d;
}

/**@ 数値取得 */

int mLineEditGetNum(mLineEdit *p)
{
	int n;

	n = mUTF32toInt_float(p->le.dat.unibuf, p->le.num_dig);
	
	if(n < p->le.num_min)
		n = p->le.num_min;
	else if(n > p->le.num_max)
		n = p->le.num_max;
	
	return n;
}

/**@ すべて選択 */

void mLineEditSelectAll(mLineEdit *p)
{
	if(mWidgetTextEdit_selectAll(&p->le.dat))
		mWidgetRedraw(MLK_WIDGET(p));
}

/**@ カーソルを右端へ移動 */

void mLineEditMoveCursor_toRight(mLineEdit *p)
{
	mWidgetTextEdit_moveCursor_toBottom(&p->le.dat);

	_change_curpos(p, TRUE);

	mWidgetRedraw(MLK_WIDGET(p));
}


//========================
// イベント
//========================


/** キー押し時 */

static void _event_key(mLineEdit *p,mEventKey *ev)
{
	int ret;

	//ENTER (通知)

	if(ev->key == MKEY_ENTER || ev->key == MKEY_KP_ENTER)
	{
		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, MLINEEDIT_N_ENTER, 0, 0);
		return;
	}
	
	//キー処理
	
	ret = mWidgetTextEdit_keyProc(&p->le.dat, ev->key, ev->state,
		!(p->le.fstyle & MLINEEDIT_S_READ_ONLY));

	//
	
	if(ret == MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT)
		_change_text(p);
	else if(ret == MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR)
		_change_curpos(p, FALSE);

	//更新
	
	if(ret)
		mWidgetRedraw(MLK_WIDGET(p));
}

/** 左ボタン押し */

static void _event_btt_left_press(mLineEdit *p,mEventPointer *ev)
{
	int pos,spinx;

	spinx = p->wg.w - 1 - _SPIN_W;

	if((p->le.fstyle & MLINEEDIT_S_SPIN) && ev->x >= spinx)
	{
		//スピン部分
		
		if(!(p->le.fstyle & MLINEEDIT_S_READ_ONLY)
			&& ev->x < p->wg.w - 1)
		{
			mWidgetSetFocus_redraw(MLK_WIDGET(p), TRUE);

			if((ev->x - spinx) / (_SPIN_W / 2) == 0)
				p->le.fpress = _PRESS_SPIN_DOWN;
			else
				p->le.fpress = _PRESS_SPIN_UP;
			
			_spin_updown(p, (p->le.fpress == _PRESS_SPIN_UP));
			
			mWidgetGrabPointer(MLK_WIDGET(p));
			
			mWidgetTimerAdd(MLK_WIDGET(p), _TIMERID_SPIN_START, 500, 0);
		}
	}
	else if(ev->x >= _get_right_x(p))
		//ラベル部分
		return;
	else
	{
		//テキスト部分

		if(ev->act == MEVENT_POINTER_ACT_DBLCLK)
		{
			//ダブルクリック: 全選択

			mLineEditSelectAll(p);
		}
		else
		{
			mWidgetSetFocus_redraw(MLK_WIDGET(p), TRUE);

			pos = _get_txtpos_point(p, ev->x);
			
			mWidgetTextEdit_moveCursorPos(&p->le.dat, pos, ev->state & MLK_STATE_SHIFT);
			
			_change_curpos(p, FALSE);

			p->le.fpress = _PRESS_TEXT;
			p->le.pos_last = pos;
			
			mWidgetGrabPointer(MLK_WIDGET(p));
		}
	}
}

/** グラブ解放 */

static void _release_grab(mLineEdit *p)
{
	if(p->le.fpress)
	{
		//スピン
	
		if(p->le.fpress == _PRESS_SPIN_UP
			|| p->le.fpress == _PRESS_SPIN_DOWN)
		{
			mWidgetRedraw(MLK_WIDGET(p));
			mWidgetTimerDeleteAll(MLK_WIDGET(p));
		}

		//
	
		p->le.fpress = 0;
		mWidgetUngrabPointer();
	}
}

/**@ イベントハンドラ */

int mLineEditHandle_event(mWidget *wg,mEvent *ev)
{
	mLineEdit *p = MLK_LINEEDIT(wg);
	int pos;

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動 (ドラッグで選択範囲拡張)
				
				if(p->le.fpress == _PRESS_TEXT)
				{
					pos = _get_txtpos_point(p, ev->pt.x);
				
					if(pos != p->le.pos_last)
					{
						mWidgetTextEdit_dragExpandSelect(&p->le.dat, pos);
						
						p->le.pos_last = pos;
						
						_change_curpos(p, FALSE);
						
						mWidgetRedraw(wg);
					}
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し
			
				if(!p->le.fpress && ev->pt.btt == MLK_BTT_LEFT)
					_event_btt_left_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
			
				if(p->le.fpress && ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			if(!p->le.fpress)
				_event_key(p, (mEventKey *)ev);
			break;

		//文字
		case MEVENT_CHAR:
		case MEVENT_STRING:
			if(!p->le.fpress && !(p->le.fstyle & MLINEEDIT_S_READ_ONLY))
			{
				if(mWidgetTextEdit_insertText(&p->le.dat,
					(ev->type == MEVENT_CHAR)? NULL: (char *)ev->str.text,
					(ev->type == MEVENT_CHAR)? ev->ch.ch: 0))
				{
					_change_text(p);
					mWidgetRedraw(wg);
				}
			}
			break;

		//ホイール (スピンあり時)
		case MEVENT_SCROLL:
			if(!p->le.fpress
				&& (p->le.fstyle & MLINEEDIT_S_SPIN)
				&& ev->scroll.vert_step)
				_spin_updown(p, (ev->scroll.vert_step < 0));
			break;
		
		//タイマー
		case MEVENT_TIMER:
			if(ev->timer.id == _TIMERID_SPIN_START)
			{
				//スピン開始
				mWidgetTimerDelete(wg, _TIMERID_SPIN_START);
				mWidgetTimerAdd(wg, _TIMERID_SPIN_REPEAT, 60, 0);
			}
			else
				//リピート
				_spin_updown(p, (p->le.fpress == _PRESS_SPIN_UP));
			break;
		
		//フォーカス
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
			{
				//[out] (選択解除)
				//ウィンドウによるフォーカス変更時は選択を残す

				if(ev->focus.from != MEVENT_FOCUS_FROM_WINDOW_FOCUS
					&& ev->focus.from != MEVENT_FOCUS_FROM_FORCE_UNGRAB)
					p->le.dat.seltop = -1;

				_release_grab(p);
			}
			else
			{
				//[in]
				//ウィンドウのフォーカス変更 (復元時は除く)、
				//または、キーによるフォーカス移動の場合は、全選択
				
				if(ev->focus.from == MEVENT_FOCUS_FROM_WINDOW_FOCUS
					|| ev->focus.from == MEVENT_FOCUS_FROM_KEY_MOVE)
					mWidgetTextEdit_selectAll(&p->le.dat);
			}
		
			mWidgetRedraw(wg);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}


//====================
// 描画
//====================


/* スピンの背景描画 */

static void _draw_spin_bkgnd(mLineEdit *p,mPixbuf *pixbuf,int x,int press)
{
	if(press)
		mPixbufDrawBkgndShadowRev(pixbuf, x, 1, _SPIN_W / 2, p->wg.h - 2, MGUICOL_BUTTON_FACE_PRESS);
	else
		mPixbufDrawBkgndShadow(pixbuf, x, 1, _SPIN_W / 2, p->wg.h - 2, MGUICOL_BUTTON_FACE);
}

/* スピン描画 */

static void _draw_spin(mLineEdit *p,mPixbuf *pixbuf,mlkbool disable)
{
	int x,h;
	uint32_t col_frame,col_text;
	
	x = p->wg.w - 1 - _SPIN_W;
	h = p->wg.h;

	col_frame = mGuiCol_getPix(disable? MGUICOL_FRAME_DISABLE: MGUICOL_FRAME);
	col_text = mGuiCol_getPix(disable? MGUICOL_TEXT_DISABLE: MGUICOL_TEXT);

	//下

	_draw_spin_bkgnd(p, pixbuf, x, (p->le.fpress == _PRESS_SPIN_DOWN));

	mPixbufLineV(pixbuf, x, 1, h - 2, col_frame);
	
	mPixbufDrawArrowDown(pixbuf, x + (_SPIN_W / 2 - 7) / 2 + 1, h / 2 - 1, 4, col_text);

	//上

	x += _SPIN_W / 2;

	_draw_spin_bkgnd(p, pixbuf, x, (p->le.fpress == _PRESS_SPIN_UP));

	mPixbufLineV(pixbuf, x, 1, h - 2, col_frame);

	mPixbufDrawArrowUp(pixbuf, x + (_SPIN_W / 2 - 7) / 2 + 1, h / 2 - 2, 4, col_text);
}

/* テキスト描画 */

static void _draw_text(mLineEdit *p,mPixbuf *pixbuf,mlkbool disable)
{
	mFont *font;
	mWidgetTextEdit *te;
	uint32_t *pt,tcol,tselcol,selbkcol;
	uint16_t *pw;
	int fonth,x,pad,right,sel_top,sel_end,w,len,tpos,top;
	uint8_t is_sel;
	
	font = mWidgetGetFont(MLK_WIDGET(p));
	tcol = mGuiCol_getRGB((disable)? MGUICOL_TEXT_DISABLE: MGUICOL_TEXT);
	right = _get_right_x(p);
	pad = _get_padding(p);

	//ラベル

	if(p->le.label.text)
	{
		mWidgetLabelText_draw(&p->le.label, pixbuf, font,
			right + _LABEL_PADDING, pad, 0, tcol, 0);
	}

	//---------
	
	te = &p->le.dat;
	if(!te->unibuf) return;
	
	//クリッピング
	
	if(!mPixbufClip_setBox_d(pixbuf, pad, 0, right - pad, p->wg.h))
		return;

	//

	sel_top = te->seltop;
	sel_end = te->selend;
	fonth = mFontGetHeight(font);

	tselcol = MGUICOL_RGB(TEXT_SELECT);
	selbkcol = mGuiCol_getPix(
		(p->wg.fstate & MWIDGET_STATE_FOCUSED)? MGUICOL_FACE_SELECT: MGUICOL_FACE_SELECT_UNFOCUS);

	//表示上の先頭文字

	pw = te->wbuf;
	x = -(p->le.scrx);
	tpos = 0;

	for(len = te->textlen; len > 0; len--, tpos++)
	{
		w = *(pw++);
		if(x + w > 0) break;

		x += w;
	}

	is_sel = mWidgetTextEdit_isSelected_atPos(te, tpos);
	x += pad;

	//テキスト

	pt = te->unibuf + tpos;
	pw = te->wbuf + tpos;
	w = 0;
	top = tpos;
	
	for(; 1; pt++, pw++, tpos++)
	{
		//各境ごとに、top -> tpos までのテキストを描画

		if((!is_sel && tpos == sel_top)		//tpos = 選択の先頭
			|| (is_sel && tpos == sel_end)	//tpos = 選択の終端
			|| x + w >= right		//右端を越える
			|| *pt == 0)			//テキスト終了
		{
			if(is_sel)
				mPixbufFillBox(pixbuf, x, pad, w, fonth, selbkcol);
			
			mFontDrawText_pixbuf_utf32(font, pixbuf,
				x, pad, te->unibuf + top, tpos - top,
				(is_sel)? tselcol: tcol);

			x += w;
			if(x >= right || *pt == 0) break;
			
			top = tpos;
			w = 0;
			is_sel ^= 1;
		}

		w += *pw;
	}

	//カーソル

	if(p->wg.fstate & MWIDGET_STATE_FOCUSED)
	{
		x = te->curx - p->le.scrx;
		
		if(x >= 0 && x < right - pad)
		{
			mPixbufSetPixelModeXor(pixbuf);
			
			mPixbufLineV(pixbuf, pad + x, pad, p->wg.h - pad * 2, 0);

			mPixbufSetPixelModeCol(pixbuf);
		}
	}
}

/**@ draw ハンドラ */

void mLineEditHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mLineEdit *p = MLK_LINEEDIT(wg);
	int n;
	uint8_t frame,disable;

	disable = !mWidgetIsEnable(wg);
	frame = !(p->le.fstyle & MLINEEDIT_S_NO_FRAME);
	
	//枠

	if(frame)
	{
		if(disable)
			n = MGUICOL_FRAME_DISABLE;
		else if(wg->fstate & MWIDGET_STATE_FOCUSED)
			n = MGUICOL_FRAME_FOCUS;
		else
			n = MGUICOL_FRAME;
	
		mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, mGuiCol_getPix(n));
	}
	
	//背景

	n = (disable || (p->le.fstyle & MLINEEDIT_S_READ_ONLY))
			? MGUICOL_FACE_DISABLE: MGUICOL_FACE_TEXTBOX;
	
	mPixbufFillBox(pixbuf, frame, frame,
		wg->w - frame * 2, wg->h - frame * 2, mGuiCol_getPix(n));

	//スピン
	
	if(p->le.fstyle & MLINEEDIT_S_SPIN)
		_draw_spin(p, pixbuf, disable);
	
	//ラベル・テキスト・カーソル
	
	_draw_text(p, pixbuf, disable);
}

