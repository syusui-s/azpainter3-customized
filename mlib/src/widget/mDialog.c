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
 * mDialog [ダイアログ]
 *****************************************/

#include "mDef.h"

#include "mDialog.h"

#include "mWindow.h"
#include "mWidget.h"
#include "mGui.h"
#include "mEvent.h"
#include "mKeyDef.h"


/*****************//**

@defgroup dialog mDialog
@brief ダイアログ

- \b MWINDOW_S_DIALOG スタイルは常に付く。
- デフォルトで、ESC キーが押されたら閉じる。 \n
- 戻り値のデフォルトは 0。

<h3>継承</h3>
mWidget \> mContainer \> mWindow \> mDialog

@ingroup group_window
@{

@file mDialog.h
@def M_DIALOG(p)
@struct mDialog
@struct mDialogData

*******************/


/** ダイアログ作成 */

mWidget *mDialogNew(int size,mWindow *owner,uint32_t style)
{
	mDialog *p;
	
	if(size < sizeof(mDialog)) size = sizeof(mDialog);
	
	p = (mDialog *)mWindowNew(size, owner, style | MWINDOW_S_DIALOG);
	if(!p) return NULL;
	
	p->wg.event = mDialogEventHandle;
	p->wg.fEventFilter |= MWIDGET_EVENTFILTER_KEY;
	
	return M_WIDGET(p);
}

/** ダイアログ実行
 * 
 * ウィンドウの表示などはあらかじめ行っておくこと。
 * 
 * @param destroy TRUE で終了後自身を削除する */

intptr_t mDialogRun(mDialog *p,mBool destroy)
{
	intptr_t ret;
	
	mAppRunModal(M_WINDOW(p));
	
	ret = p->dlg.retval;

	//削除
	
	if(destroy)
		mWidgetDestroy(M_WIDGET(p));
	
	mAppSync();

	return ret;
}

/** ダイアログ終了 */

void mDialogEnd(mDialog *p,intptr_t ret)
{
	p->dlg.retval = ret;
	
	mAppQuit();
}

/** イベントハンドラ */

int mDialogEventHandle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_CLOSE:
			mAppQuit();
			return TRUE;
		
		case MEVENT_KEYDOWN:
			//ESC で閉じる
			if(!ev->key.bGrab && ev->key.code == MKEY_ESCAPE)
			{
				mWidgetAppendEvent(wg, MEVENT_CLOSE);
				return TRUE;
			}
			break;
	}

	return FALSE;
}

/** OK/キャンセルボタンを処理する汎用イベントハンドラ
 *
 * 通知イベントで M_WID_OK が来た時は TRUE、
 * M_WID_CANCEL が来た時は FALSE を返して終了する。*/

int mDialogEventHandle_okcancel(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.id == M_WID_OK)
				mDialogEnd(M_DIALOG(wg), TRUE);
			else if(ev->notify.id == M_WID_CANCEL)
				mDialogEnd(M_DIALOG(wg), FALSE);
			break;
		default:
			return mDialogEventHandle(wg, ev);
	}

	return TRUE;
}

/** @} */
