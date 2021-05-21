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
 * mCheckButton [チェックボタン]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_checkbutton.h"
#include "mlk_font.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_key.h"

#include "mlk_pv_widget.h"


//-----------------------

#define _CHECKBOXSIZE 13	//チェック部分のボックスサイズ
#define _MARGIN_TEXT  5		//チェックボックスとテキストの間隔
#define _PADDING_X    2
#define _PADDING_Y    2
#define _BUTTON_PADX  5
#define _BUTTON_PADY  4

//-----------------------



//==========================
// sub
//==========================


/* グラブ状態解除
 *
 * ボタン/キー押し時にチェック状態を変化させているため、
 * キャンセル動作を行う場合でも、常に通知を送る必要がある。 */

static void _grab_release(mCheckButton *p)
{
	if(p->ckbtt.flags & MCHECKBUTTON_F_GRABBED)
	{
		mWidgetUngrabPointer();

		p->ckbtt.flags &= ~MCHECKBUTTON_F_GRABBED;
		p->ckbtt.grabbed_key = 0;

		//通知 (状態変化時のみ)
		
		if(p->ckbtt.flags & MCHECKBUTTON_F_CHANGED)
		{
			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
				MCHECKBUTTON_N_CHANGE, mCheckButtonIsChecked(p), 0);
		}
	}
}

/* ボタン/キー押し時
 *
 * key: -1 でポインタ操作 */

static void _on_press(mCheckButton *p,int key)
{
	p->ckbtt.flags |= MCHECKBUTTON_F_GRABBED;
	p->ckbtt.grabbed_key = key;

	if(key == -1)
	{
		mWidgetSetFocus_redraw(MLK_WIDGET(p), FALSE);
		mWidgetGrabPointer(MLK_WIDGET(p));
	}

	if(mCheckButtonSetState(p, -1))
		p->ckbtt.flags |= MCHECKBUTTON_F_CHANGED;
	else
		p->ckbtt.flags &= ~MCHECKBUTTON_F_CHANGED;
}


//==========================
// main
//==========================


/**@ データ削除 */

void mCheckButtonDestroy(mWidget *wg)
{
	mWidgetLabelText_free(&MLK_CHECKBUTTON(wg)->ckbtt.txt);
}

/**@ 作成 */

mCheckButton *mCheckButtonNew(mWidget *parent,int size,uint32_t fstyle)
{
	mCheckButton *p;
	
	if(size < sizeof(mCheckButton))
		size = sizeof(mCheckButton);
	
	p = (mCheckButton *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mCheckButtonDestroy;
	p->wg.calc_hint = mCheckButtonHandle_calcHint;
	p->wg.draw = mCheckButtonHandle_draw;
	p->wg.event = mCheckButtonHandle_event;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.ftype |= MWIDGET_TYPE_CHECKBUTTON;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY;

	p->ckbtt.fstyle = fstyle;
	
	return p;
}

/**@ 作成
 *
 * @p:checked 0 以外で、デフォルトでチェック状態にする */

mCheckButton *mCheckButtonCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text,int checked)
{
	mCheckButton *p;

	p = mCheckButtonNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	if(checked)
		p->ckbtt.flags |= MCHECKBUTTON_F_CHECKED;

	mWidgetLabelText_set(MLK_WIDGET(p), &p->ckbtt.txt, text,
		fstyle & MCHECKBUTTON_S_COPYTEXT);

	return p;
}

/**@ ラジオタイプかどうか
 *
 * @d:p が実際には mCheckButton でない場合でも判定可。 */

mlkbool mCheckButtonIsRadioType(mCheckButton *p)
{
	return ((p->wg.ftype & MWIDGET_TYPE_CHECKBUTTON)
			&& (p->ckbtt.fstyle & MCHECKBUTTON_S_RADIO));
}

/**@ チェックボックスのサイズを取得 */

int mCheckButtonGetCheckBoxSize(void)
{
	return _CHECKBOXSIZE;
}

/**@ テキストセット
 *
 * @d:テキストを複製するかは、スタイルフラグによる。 */

void mCheckButtonSetText(mCheckButton *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->ckbtt.txt, text,
		p->ckbtt.fstyle & MCHECKBUTTON_S_COPYTEXT);
}

/**@ テキストをセット (複製)
 *
 * @d:スタイルフラグは変更しない。 */

void mCheckButtonSetText_copy(mCheckButton *p,const char *text)
{
	mWidgetLabelText_set(MLK_WIDGET(p), &p->ckbtt.txt, text, TRUE);
}

/**@ チェック状態の変更
 *
 * @d:ラジオボタン時、p が ON の状態で type != 0 の場合、変化しない。\
 * p 以外が ON の状態で p を ON にする場合、ON 状態のボタンを OFF にした上で p を ON にする。\
 * p が ON の状態で type = 0 の場合は、選択なしの状態になる。
 * 
 * @p:type [0] OFF [正] ON [負] 反転 (ラジオの場合は ON)
 * @r:チェック状態が変わったか */

mlkbool mCheckButtonSetState(mCheckButton *p,int type)
{
	mCheckButton *sel;

	//type: 0=OFF、それ以外=ON
	
	if(p->ckbtt.fstyle & MCHECKBUTTON_S_RADIO)
	{
		//ラジオ時、p 以外のラジオが選択されている状態で p が ON になる場合、
		//現在の選択を OFF にする。

		if(type)
		{
			sel = mCheckButtonGetRadioInfo(p, NULL, NULL);

			if(sel && p != sel)
			{
				sel->ckbtt.flags &= ~MCHECKBUTTON_F_CHECKED;
				mWidgetRedraw(MLK_WIDGET(sel));
			}
		}
	}
	else
	{
		//チェックボタン時、反転の場合

		if(type < 0)
			type = !(p->ckbtt.flags & MCHECKBUTTON_F_CHECKED);
	}

	//状態が変わる場合、変更

	if((!type) != !(p->ckbtt.flags & MCHECKBUTTON_F_CHECKED))
	{
		p->ckbtt.flags ^= MCHECKBUTTON_F_CHECKED;
		mWidgetRedraw(MLK_WIDGET(p));

		return TRUE;
	}

	return FALSE;
}

/**@ ラジオボタンの選択を変更
 *
 * @p:p ラジオグループ内のウィジェットであれば、どれでもよい。
 * @p:no 選択を ON にするボタンのインデックス位置 (グループの先頭を 0 とする)
 * @r:対象のボタンの状態が変化したか */

mlkbool mCheckButtonSetGroupSel(mCheckButton *p,int no)
{
	mCheckButton *top,*end;
	int i;

	mCheckButtonGetRadioInfo(p, &top, &end);

	if(top && end)
	{
		for(i = 0, p = top; p; p = MLK_CHECKBUTTON(p->wg.next))
		{
			if(mCheckButtonIsRadioType(p))
			{
				if(i == no)
					return mCheckButtonSetState(p, 1);
			
				if(p == end) break;

				i++;
			}
		}
	}

	return FALSE;
}

/**@ ラジオグループ内で、現在選択されているボタンのインデックス位置を取得
 * 
 * @r:グループの先頭を 0 としたインデックス位置。\
 * -1 で選択なし。 */

int mCheckButtonGetGroupSel(mCheckButton *p)
{
	mCheckButton *sel,*top;
	int i;
	
	if(p->ckbtt.fstyle & MCHECKBUTTON_S_RADIO)
	{
		sel = mCheckButtonGetRadioInfo(p, &top, NULL);
		
		if(sel)
		{
			for(i = 0; top && top != sel; top = MLK_CHECKBUTTON(top->wg.next))
			{
				if(mCheckButtonIsRadioType(p))
					i++;
			}

			return i;
		}
	}
	
	return -1;
}

/**@ ボタンがチェックされている状態か */

mlkbool mCheckButtonIsChecked(mCheckButton *p)
{
	return ((p->ckbtt.flags & MCHECKBUTTON_F_CHECKED) != 0);
}

/**@ ラジオグループの範囲と現在の選択を取得
 *
 * @d:同じウィジェット階層内が対象。\
 *  RADIO_GROUP がある場合、このグループの先頭 or 次のグループの先頭となる。\
 *  ラジオタイプ以外のすべてのウィジェットはスキップ対象となり、
 *  次のラジオタイプへグループが継続するため、範囲内では mCheckButton 以外も存在する。
 *
 * @p:p ラジオグループ内のいずれかのウィジェット
 * @p:pptop NULL 以外で、グループの先頭ウィジェットが入る
 * @p:ppend NULL 以外で、グループの終端ウィジェットが入る
 * @r:現在選択されているウィジェットが返る。NULL でなし。 */

mCheckButton *mCheckButtonGetRadioInfo(mCheckButton *p,
	mCheckButton **pptop,mCheckButton **ppend)
{
	mCheckButton *p2,*last,*sel = NULL;

	//先頭を検索

	last = p;

	while(1)
	{
		if(mCheckButtonIsRadioType(p))
			last = p;

		if((p->wg.ftype & MWIDGET_TYPE_CHECKBUTTON) && (p->ckbtt.fstyle & MCHECKBUTTON_S_RADIO_GROUP))
			break;

		//前

		p2 = MLK_CHECKBUTTON(p->wg.prev);
		if(!p2) break;

		p = p2;
	}
	
	if(pptop) *pptop = last;

	//先頭から、終端と現在の選択を検索

	p = last;

	while(1)
	{
		if(mCheckButtonIsRadioType(p))
		{
			last = p;

			//現在の選択

			if(p->ckbtt.flags & MCHECKBUTTON_F_CHECKED)
				sel = p;
		}
	
		//次
		
		p2 = MLK_CHECKBUTTON(p->wg.next);

		if(!p2
			|| ((p2->wg.ftype & MWIDGET_TYPE_CHECKBUTTON) && (p2->ckbtt.fstyle & MCHECKBUTTON_S_RADIO_GROUP)))
			break;

		p = p2;
	}
	
	if(ppend) *ppend = last;
		
	return sel;
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mCheckButtonHandle_calcHint(mWidget *wg)
{
	mCheckButton *p = MLK_CHECKBUTTON(wg);
	mSize size;

	//ラベルのサイズ

	mWidgetLabelText_onCalcHint(wg, &p->ckbtt.txt, &size);

	//

	if(p->ckbtt.fstyle & MCHECKBUTTON_S_BUTTON)
	{
		//ボタンタイプ

		size.w += _BUTTON_PADX * 2;
		size.h += _BUTTON_PADY * 2;

		if(size.w < size.h) size.w = size.h;
	}
	else
	{
		//チェックタイプ
		
		if(size.w) size.w += _MARGIN_TEXT;
		size.w += _CHECKBOXSIZE + _PADDING_X * 2;

		if(size.h < _CHECKBOXSIZE) size.h = _CHECKBOXSIZE;
		size.h += _PADDING_Y * 2;
	}
	
	wg->hintW = size.w;
	wg->hintH = size.h;
}

/**@ event ハンドラ関数 */

int mCheckButtonHandle_event(mWidget *wg,mEvent *ev)
{
	mCheckButton *p = MLK_CHECKBUTTON(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//左押し: グラブ開始
			
				if(ev->pt.btt == MLK_BTT_LEFT
					&& !(p->ckbtt.flags & MCHECKBUTTON_F_GRABBED))
					_on_press(p, -1);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//左離し: グラブ解放
			
				if(ev->pt.btt == MLK_BTT_LEFT
					&& (p->ckbtt.flags & MCHECKBUTTON_F_GRABBED)
					&& p->ckbtt.grabbed_key == -1)
					_grab_release(p);
			}
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			if(!(p->ckbtt.flags & MCHECKBUTTON_F_GRABBED))
			{
				if(ev->key.key == MKEY_SPACE || ev->key.key == MKEY_KP_SPACE)
					//SPACE
					_on_press(p, ev->key.raw_code);
				else
					//矢印キーによるフォーカス移動
					__mWidgetMoveFocus_arrowkey(wg, ev->key.key);
			}
			break;

		//キー離し
		case MEVENT_KEYUP:
			if((p->ckbtt.flags & MCHECKBUTTON_F_GRABBED)
				&& p->ckbtt.grabbed_key == ev->key.raw_code)
				_grab_release(p);
			break;
		
		case MEVENT_FOCUS:
			//フォーカスアウト時、グラブ状態解除
			if(ev->focus.is_out)
				_grab_release(p);
					
			mWidgetRedraw(wg);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mCheckButtonHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mCheckButton *p = MLK_CHECKBUTTON(wg);
	mFont *font;
	int disable,pressed,y;
	uint32_t flags;

	font = mWidgetGetFont(wg);

	disable = !mWidgetIsEnable(wg);
	pressed = ((p->ckbtt.flags & MCHECKBUTTON_F_CHECKED) != 0);

	if(p->ckbtt.fstyle & MCHECKBUTTON_S_BUTTON)
	{
		//---- ボタンタイプ
	
		flags = 0;

		if(pressed)
			flags |= MPIXBUF_DRAWBTT_PRESSED;

		if(p->wg.fstate & MWIDGET_STATE_FOCUSED)
			flags |= MPIXBUF_DRAWBTT_FOCUSED;

		if(disable)
			flags |= MPIXBUF_DRAWBTT_DISABLED;

		mPixbufDrawButton(pixbuf, 0, 0, wg->w, wg->h, flags);

		//

		mWidgetLabelText_draw(&p->ckbtt.txt, pixbuf, font,
			pressed, (wg->h - p->ckbtt.txt.szfull.h) / 2 + pressed, wg->w,
			mGuiCol_getRGB((disable)? MGUICOL_TEXT_DISABLE: MGUICOL_TEXT),
			MWIDGETLABELTEXT_DRAW_F_CENTER);
	}
	else
	{
		//---- チェックタイプ
	
		//背景
		
		mWidgetDrawBkgnd(wg, NULL);
		
		//フォーカス枠
		
		if(wg->fstate & MWIDGET_STATE_FOCUSED)
			mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FRAME_FOCUS));
		
		//チェックボックス
		
		flags = 0;
		if(pressed) flags |= MPIXBUF_DRAWCKBOX_CHECKED;
		if(p->ckbtt.fstyle & MCHECKBUTTON_S_RADIO) flags |= MPIXBUF_DRAWCKBOX_RADIO;
		if(disable) flags |= MPIXBUF_DRAWCKBOX_DISABLE;

		y = (mFontGetHeight(font) - _CHECKBOXSIZE) >> 1;
		if(y < 0) y = 0;
		
		mPixbufDrawCheckBox(pixbuf, _PADDING_X, _PADDING_Y + y, flags);
		
		//ラベル

		y = (_CHECKBOXSIZE - p->ckbtt.txt.szfull.h) >> 1;
		if(y < 0) y = 0;

		mWidgetLabelText_draw(&p->ckbtt.txt, pixbuf, font,
			_PADDING_X + _CHECKBOXSIZE + _MARGIN_TEXT,
			_PADDING_Y + y,
			p->ckbtt.txt.szfull.w,
			mGuiCol_getRGB((disable)? MGUICOL_TEXT_DISABLE: MGUICOL_TEXT), 0);
	}
}

