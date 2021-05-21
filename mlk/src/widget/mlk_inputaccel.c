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
 * mInputAccel : アクセラレータキーの入力
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_inputaccel.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_key.h"
#include "mlk_str.h"
#include "mlk_accelerator.h"

#include "mlk_pv_widget.h"


//---------------

#define _PADDING_X  3
#define _PADDING_Y  3

//---------------



/**@ mInputAccel データ解放 */

void mInputAccelDestroy(mWidget *p)
{
	mStrFree(&MLK_INPUTACCEL(p)->ia.str);
}

/**@ 作成 */

mInputAccel *mInputAccelNew(mWidget *parent,int size,uint32_t fstyle)
{
	mInputAccel *p;
	
	if(size < sizeof(mInputAccel))
		size = sizeof(mInputAccel);
	
	p = (mInputAccel *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mInputAccelDestroy;
	p->wg.calc_hint = mInputAccelHandle_calcHint;
	p->wg.draw = mInputAccelHandle_draw;
	p->wg.event = mInputAccelHandle_event;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY;
	p->wg.facceptkey = MWIDGET_ACCEPTKEY_ALL;

	p->ia.fstyle = fstyle;

	return p;
}

/**@ 作成 */

mInputAccel *mInputAccelCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mInputAccel *p;

	p = mInputAccelNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ 現在のキーを取得 */

uint32_t mInputAccelGetKey(mInputAccel *p)
{
	return p->ia.key;
}

/**@ キーをセット
 *
 * @p:key 0 でクリア */

void mInputAccelSetKey(mInputAccel *p,uint32_t key)
{
	if(p->ia.key != key)
	{
		p->ia.key = key;

		mAcceleratorGetKeyText(&p->ia.str, key);

		mWidgetRedraw(MLK_WIDGET(p));
	}
}


//========================
// ハンドラ
//========================


/**@ calc_hint ハンドラ関数 */

void mInputAccelHandle_calcHint(mWidget *wg)
{
	wg->hintW = 10;
	wg->hintH = mWidgetGetFontHeight(wg) + _PADDING_Y * 2;
}

/* キー押し */

static void _event_keydown(mInputAccel *p,mEvent *ev)
{
	uint32_t k;

	//ENTER を通知する

	if((p->ia.fstyle & MINPUTACCEL_S_NOTIFY_ENTER)
		&& (ev->key.key == MKEY_ENTER || ev->key.key == MKEY_KP_ENTER))
	{
		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, MINPUTACCEL_N_ENTER, 0, 0);
		return;
	}

	//

	k = mAcceleratorGetAccelKey_keyevent(ev);

	if(k != p->ia.key)
	{
		//セット

		mInputAccelSetKey(p, k);

		//通知

		if(p->ia.fstyle & MINPUTACCEL_S_NOTIFY_CHANGE)
		{
			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
				MINPUTACCEL_N_CHANGE, k, 0);
		}
	}
}

/**@ event ハンドラ関数 */

int mInputAccelHandle_event(mWidget *wg,mEvent *ev)
{
	mInputAccel *p = MLK_INPUTACCEL(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			//ボタン押しでフォーカスセット
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
				mWidgetSetFocus_redraw(wg, FALSE);
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			_event_keydown(p, ev);
			break;

		//フォーカス
		case MEVENT_FOCUS:
			mWidgetRedraw(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/**@ 描画 */

void mInputAccelHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mInputAccel *p = MLK_INPUTACCEL(wg);
	mFont *font;
	int w,h,tx,tw,n;
	mlkbool is_disable;

	w = wg->w;
	h = wg->h;

	is_disable = !mWidgetIsEnable(wg);
	
	font = mWidgetGetFont(wg);

	//背景

	n = (is_disable)? MGUICOL_FACE_DISABLE: MGUICOL_FACE_TEXTBOX;

	mPixbufFillBox(pixbuf, 0, 0, w, h, mGuiCol_getPix(n));

	//枠

	if(is_disable)
		n = MGUICOL_FRAME_DISABLE;
	else if(wg->fstate & MWIDGET_STATE_FOCUSED)
		n = MGUICOL_FRAME_FOCUS;
	else
		n = MGUICOL_FRAME;

	mPixbufBox(pixbuf, 0, 0, w, h, mGuiCol_getPix(n));

	//キーテキスト

	if(mStrIsEmpty(&p->ia.str))
		//空の時、カーソル位置は中央
		tx = w >> 1, tw = 0;
	else
	{
		if(mPixbufClip_setBox_d(pixbuf, _PADDING_X, 0, w - _PADDING_X * 2, h))
		{
			tw = mFontGetTextWidth(font, p->ia.str.buf, -1);
			tx = (w - tw) >> 1;

			n = (is_disable)? MGUICOL_TEXT_DISABLE: MGUICOL_TEXT;

			mFontDrawText_pixbuf(font, pixbuf,
				tx, _PADDING_Y,
				p->ia.str.buf, -1, mGuiCol_getRGB(n));
		}
	}

	//カーソル

	if(wg->fstate & MWIDGET_STATE_FOCUSED)
	{
		mPixbufSetPixelModeXor(pixbuf);
		mPixbufLineV(pixbuf, tx + tw, _PADDING_Y, mFontGetHeight(font), 0);
		mPixbufSetPixelModeCol(pixbuf);
	}
}

