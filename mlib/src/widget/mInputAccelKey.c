/*$
 Copyright (C) 2013-2020 Azel.

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
 * mInputAccelKey
 *****************************************/

#include "mDef.h"

#include "mInputAccelKey.h"

#include "mWidget.h"
#include "mPixbuf.h"
#include "mFont.h"
#include "mEvent.h"
#include "mSysCol.h"
#include "mKeyDef.h"
#include "mAccelerator.h"


#define _PADDING_X  3
#define _PADDING_Y  3


/******************//**

@defgroup inputaccelkey mInputAccelKey
@brief アクセラレータキーの入力
@ingroup group_widget

<h3>継承</h3>
mWidget \> mInputAccelKey

@{

@file mInputAccelKey.h
@def M_INPUTACCELKEY(p)
@struct mInputAccelKeyData
@struct mInputAccelKey
@enum MINPUTACCELKEY_STYLE
@enum MINPUTACCELKEY_NOTIFY

@var MINPUTACCELKEY_STYLE::MINPUTACCELKEY_S_NOTIFY_CHANGE
キーが変更された時、MINPUTACCELKEY_N_CHANGE_KEY 通知を送るようにする。

@var MINPUTACCELKEY_NOTIFY::MINPUTACCELKEY_N_CHANGE_KEY
キーが変更された時。\n
装飾キーのみの場合は 0 となる。\n
param1 : キーコード

***********************/


/** 解放処理 */

void mInputAccelKeyDestroyHandle(mWidget *p)
{
	mFree(M_INPUTACCELKEY(p)->ak.text);
}

/** 作成 */

mInputAccelKey *mInputAccelKeyNew(int size,mWidget *parent,uint32_t style)
{
	mInputAccelKey *p;
	
	if(size < sizeof(mInputAccelKey)) size = sizeof(mInputAccelKey);
	
	p = (mInputAccelKey *)mWidgetNew(size, parent);
	if(!p) return NULL;
	
	p->wg.destroy = mInputAccelKeyDestroyHandle;
	p->wg.calcHint = mInputAccelKeyCalcHintHandle;
	p->wg.draw = mInputAccelKeyDrawHandle;
	p->wg.event = mInputAccelKeyEventHandle;
	
	p->wg.fState |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;
	p->wg.fAcceptKeys = MWIDGET_ACCEPTKEY_ALL;

	p->ak.style = style;

	return p;
}

/** 作成 */

mInputAccelKey *mInputAccelKeyCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginb4)
{
	mInputAccelKey *p;

	p = mInputAccelKeyNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginb4);

	return p;
}

/** キーを取得 */

uint32_t mInputAccelKey_getKey(mInputAccelKey *p)
{
	return p->ak.key;
}

/** キーをセット
 *
 * @param key 0 でクリア */

void mInputAccelKey_setKey(mInputAccelKey *p,uint32_t key)
{
	if(p->ak.key != key)
	{
		M_FREE_NULL(p->ak.text);

		p->ak.key = key;
		if(key) p->ak.text = mAcceleratorGetKeyText(key);

		mWidgetUpdate(M_WIDGET(p));
	}
}


//========================
// ハンドラ
//========================


/** サイズ計算 */

void mInputAccelKeyCalcHintHandle(mWidget *wg)
{
	wg->hintW = 10;
	wg->hintH = mWidgetGetFontHeight(wg) + _PADDING_Y * 2;
}

/** キー離し時
 *
 * 押し時にセットすると、キーリピートで何度も実行されるので、
 * 押し時にフラグ ON -> 離した時に確定処理。 */

static void _event_keyup(mInputAccelKey *p,mEvent *ev)
{
	uint32_t k;

	if(p->ak.bKeyDown)
	{
		//装飾キーのみの場合はクリア

		k = p->ak.key & MACCKEY_KEYMASK;

		if(k == MKEY_CONTROL || k == MKEY_SHIFT || k == MKEY_ALT)
			mInputAccelKey_setKey(p, 0);

		//通知

		if(p->ak.keyprev != p->ak.key
			&& (p->ak.style & MINPUTACCELKEY_S_NOTIFY_CHANGE))
		{
			mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
				MINPUTACCELKEY_N_CHANGE_KEY, p->ak.key, 0);
		}

		//

		p->ak.bKeyDown = FALSE;
	}
}

/** イベント */

int mInputAccelKeyEventHandle(mWidget *wg,mEvent *ev)
{
	mInputAccelKey *p = M_INPUTACCELKEY(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			//左ボタン押しでフォーカスセット
			if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS
				&& ev->pt.btt == M_BTT_LEFT)
				mWidgetSetFocus_update(wg, FALSE);
			break;

		//キー押し
		case MEVENT_KEYDOWN:
			p->ak.keyprev = p->ak.key;
			p->ak.bKeyDown = TRUE;
			
			mInputAccelKey_setKey(p, mAcceleratorGetKeyFromEvent(ev));
			break;
		//キー離し
		case MEVENT_KEYUP:
			_event_keyup(p, ev);
			break;
		
		case MEVENT_FOCUS:
			//キー押し中にフォーカスアウトした時
			if(ev->focus.bOut)
				_event_keyup(p, ev);
		
			mWidgetUpdate(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void mInputAccelKeyDrawHandle(mWidget *wg,mPixbuf *pixbuf)
{
	mInputAccelKey *p = M_INPUTACCELKEY(wg);
	mFont *font;
	int tx,tw;
	
	font = mWidgetGetFont(wg);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MSYSCOL(FACE_LIGHTEST));

	//枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h,
		(wg->fState & MWIDGET_STATE_FOCUSED)? MSYSCOL(FRAME_FOCUS): MSYSCOL(FRAME_DARK));

	//キーテキスト

	if(!p->ak.text)
		tx = wg->w >> 1, tw = 0;
	else
	{
		mPixbufSetClipBox_d(pixbuf, _PADDING_X, 0, wg->w - _PADDING_X * 2, wg->h);

		tw = mFontGetTextWidth(font, p->ak.text, -1);
		tx = (wg->w - tw) >> 1;

		mFontDrawText(font, pixbuf, tx, _PADDING_Y,
			p->ak.text, -1, MSYSCOL_RGB(TEXT));
	}

	//カーソル

	if(wg->fState & MWIDGET_STATE_FOCUSED)
		mPixbufLineV(pixbuf, tx + tw, _PADDING_Y, font->height, MSYSCOL(TEXT)); 
}

/** @} */
