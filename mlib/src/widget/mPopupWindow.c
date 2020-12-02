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
 * mPopupWindow
 *****************************************/

#include "mDef.h"

#include "mPopupWindow.h"

#include "mWidget.h"
#include "mWindow.h"
#include "mGui.h"
#include "mEvent.h"
#include "mKeyDef.h"


/*********************//**

@defgroup popupwindow mPopupWindow
@brief ポップアップウィンドウ

<h3>動作について</h3>
- カーソルがウィンドウ外にある場合は、ウィンドウ外でのボタン押しを検知するため、ポインタをグラブする。
- カーソルがウィンドウ内にある場合は、内部ウィジェットを動作させるために、グラブは解除する。
- ウィンドウ外がクリックされた時、または ESC キーで終了。

<h3>quit ハンドラ</h3>
ウィンドウ外でクリックされたり ESC キーが押された時に、独自の処理を行いたい場合は、mPopupWindowData::quit にハンドラ関数をセットする。\n
ハンドラの戻り値は TRUE で終了、FALSE で終了しない。\n
quit が NULL の場合は常に終了 (デフォルト)。

<h3>継承</h3>
mWidget \> mContainer \> mWindow \> mPopupWindow

@ingroup group_window
@{

@file mPopupWindow.h
@def M_POPUPWINDOW(p)
@struct _mPopupWindow
@struct mPopupWindowData

*************************/



/** 作成
 *
 * @param style ウィンドウスタイル。MWINDOW_S_POPUP,MWINDOW_S_NO_IM は常に ON。 */

mPopupWindow *mPopupWindowNew(int size,mWindow *owner,uint32_t style)
{
	mPopupWindow *p;
	
	if(size < sizeof(mPopupWindow)) size = sizeof(mPopupWindow);
	
	p = (mPopupWindow *)mWindowNew(size, owner,
		style | MWINDOW_S_POPUP | MWINDOW_S_NO_IM);
	if(!p) return NULL;
	
	p->wg.event = mPopupWindowEventHandle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_POINTER | MWIDGET_EVENTFILTER_KEY;

	return p;
}

/** 実行
 *
 * @attention 表示と移動は行うが、サイズ変更は行わない。*/

void mPopupWindowRun(mPopupWindow *p,int rootx,int rooty)
{
	mWidgetMove(M_WIDGET(p), rootx, rooty);
	mWidgetShow(M_WIDGET(p), 1);

	mAppRunPopup(M_WINDOW(p));
}

/** 実行 (移動は行わず、表示のみ) */

void mPopupWindowRun_show(mPopupWindow *p)
{
	mWidgetShow(M_WIDGET(p), 1);

	mAppRunPopup(M_WINDOW(p));
}

/** ポップアップを終了
 *
 * quit() ハンドラがある場合はその戻り値で終了を判定する。\n
 * ハンドラが NULL の場合は常に終了。 */

void mPopupWindowQuit(mPopupWindow *p,mBool bCancel)
{
	if(!p->pop.quit || (p->pop.quit)(p, bCancel))
	{
		/* ungrab 時に LEAVE イベントが起こるので、
		 * 再びグラブされるのを防ぐため、フラグをセット */
	
		p->pop.bEnd = TRUE;
	
		mWidgetUngrabPointer(M_WIDGET(p));
		mAppQuit();
	}
}


//========================
// ハンドラ
//========================


/** グラブ */

static void _grab_onoff(mWidget *p,mBool on)
{
	if(M_POPUPWINDOW(p)->pop.bEnd) return;

	if(on)
		mWidgetGrabPointer(p);
	else
		mWidgetUngrabPointer(p);
}

/** カーソル位置によってグラブをON/OFF */

static void _grab_cur(mWidget *wg)
{
	_grab_onoff(wg, !mWidgetIsCursorIn(wg));
}

/** イベント */

int mPopupWindowEventHandle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.type == MEVENT_POINTER_TYPE_MOTION)
			{
				_grab_onoff(wg, !mWidgetIsContain(wg, ev->pt.x, ev->pt.y));
			}
			else if(ev->pt.type == MEVENT_POINTER_TYPE_PRESS)
			{
				//ウィンドウ外でクリックされた時は終了

				if(!mWidgetIsContain(wg, ev->pt.x, ev->pt.y))
					mPopupWindowQuit(M_POPUPWINDOW(wg), TRUE);
			}
			break;

		case MEVENT_ENTER:
			_grab_cur(wg);
			break;
		case MEVENT_LEAVE:
			_grab_cur(wg);
			break;
		
		case MEVENT_KEYDOWN:
			//ESC キーで終了
			if(ev->key.code == MKEY_ESCAPE)
				mPopupWindowQuit(M_POPUPWINDOW(wg), TRUE);
			break;

		//表示時
		case MEVENT_MAP:
			_grab_cur(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}

/** @} */
