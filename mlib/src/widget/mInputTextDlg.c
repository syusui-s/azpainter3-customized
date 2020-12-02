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
 * テキスト入力ダイアログ
 *****************************************/

#include "mDef.h"
#include "mDialog.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mLineEdit.h"
#include "mStr.h"
#include "mEvent.h"
#include "mSysDialog.h"


//----------------------

typedef struct
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;

	mLineEdit *edit;
	uint32_t flags;
}_dialog;

#define _FLAGS_SET_DEFAULT  0x80000000

//----------------------


//******************************
// ダイアログ
//******************************


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.widgetFrom->id == M_WID_OK)
	{
		_dialog *p = (_dialog *)wg;
	
		//空かどうか
		
		if((p->flags & MSYSDLG_INPUTTEXT_F_NOEMPTY)
			&& mLineEditIsEmpty(p->edit))
		{
			return 1;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** 作成 */

static _dialog *_create_dialog(mWindow *owner,
	const char *title,const char *message,const char *text,uint32_t flags,int *numstate)
{
	_dialog *p;
	mWidget *wg;
	mLineEdit *le;
	
	p = (_dialog *)mDialogNew(sizeof(_dialog), owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->flags = flags;

	//

	mContainerSetPadding_one(M_CONTAINER(p), 10);
	p->ct.sepW = 6;

	mWindowSetTitle(M_WINDOW(p), title);

	//ウィジェット

	mLabelCreate(M_WIDGET(p), 0, 0, 0, message);

	p->edit = le = mLineEditCreate(M_WIDGET(p), 0,
		(numstate)? MLINEEDIT_S_SPIN: 0, MLF_EXPAND_W, 0);
	
	le->wg.initW = mWidgetGetFontHeight(M_WIDGET(p)) * 20;

	if(numstate)
	{
		//数値

		mLineEditSetNumStatus(le, numstate[0], numstate[1], 0);

		if(flags & _FLAGS_SET_DEFAULT)
			mLineEditSetNum(le, numstate[2]);
	}
	else
		mLineEditSetText(le, text);

	mWidgetSetFocus(M_WIDGET(le));
	mLineEditSelectAll(le);

	//OK/Cancel

	wg = mContainerCreateOkCancelButton(M_WIDGET(p));
	wg->margin.top = 10;
	
	return p;
}


//******************************
// 関数
//******************************


/** 文字列入力ダイアログ
 *
 * str の内容が初期テキストとして表示される。
 *
 * @param flags \b MSYSDLG_INPUTTEXT_F_NOEMPTY : 空文字列は許可しない
 * 
 * @ingroup sysdialog */

mBool mSysDlgInputText(mWindow *owner,
	const char *title,const char *message,mStr *str,uint32_t flags)
{
	_dialog *p;
	mBool ret;

	p = _create_dialog(owner, title, message, str->buf, flags, NULL);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		mLineEditGetTextStr(p->edit, str);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}

/** 数値入力ダイアログ
 *
 * @param flags \b MSYSDLG_INPUTNUM_F_DEFAULT : *dst の値をデフォルト値としてセット
 * 
 * @ingroup sysdialog */

mBool mSysDlgInputNum(mWindow *owner,
	const char *title,const char *message,int *dst,int min,int max,uint32_t flags)
{
	_dialog *p;
	int state[3];
	mBool ret;

	state[0] = min;
	state[1] = max;
	state[2] = *dst;

	//ダイアログ

	p = _create_dialog(owner, title, message, NULL,
		(flags & MSYSDLG_INPUTNUM_F_DEFAULT)? _FLAGS_SET_DEFAULT: 0, state);
	if(!p) return FALSE;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	ret = mDialogRun(M_DIALOG(p), FALSE);

	if(ret)
		*dst = mLineEditGetNum(p->edit);

	mWidgetDestroy(M_WIDGET(p));

	return ret;
}

