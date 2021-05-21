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
 * テキスト入力ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_str.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit;
	uint32_t flags;
}_dialog;

//----------------------


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	//OK 時
	
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == MLK_WID_OK)
	{
		_dialog *p = (_dialog *)wg;
	
		//空を受け付けない場合
		
		if((p->flags & MSYSDLG_INPUTTEXT_F_NOT_EMPTY)
			&& mLineEditIsEmpty(p->edit))
			return 1;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/** ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,
	const char *title,const char *message,const char *text,uint32_t flags,int *numstate)
{
	_dialog *p;
	mLineEdit *le;
	
	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->flags = flags;

	//

	mContainerSetType_vert(MLK_CONTAINER(p), 6);
	mContainerSetPadding_same(MLK_CONTAINER(p), 10);

	mToplevelSetTitle(MLK_TOPLEVEL(p), title);

	//ウィジェット

	if(message)
		mLabelCreate(MLK_WIDGET(p), 0, 0, 0, message);

	p->edit = le = mLineEditCreate(MLK_WIDGET(p), 0,
		MLF_EXPAND_W | MLF_EXPAND_Y, 0, (numstate)? MLINEEDIT_S_SPIN: 0);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(le),
		(numstate)? 12: 20, 0);

	if(numstate)
	{
		//数値

		mLineEditSetNumStatus(le, numstate[0], numstate[1], 0);

		if(flags & MSYSDLG_INPUTTEXT_F_SET_DEFAULT)
			mLineEditSetNum(le, numstate[2]);
	}
	else
	{
		if(flags & MSYSDLG_INPUTTEXT_F_SET_DEFAULT)
			mLineEditSetText(le, text);
	}

	mWidgetSetFocus(MLK_WIDGET(le));

	//OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,10,0,0));
	
	return p;
}


//******************************
// 関数
//******************************


/**@ テキスト入力ダイアログ
 *
 * @p:message メッセージのラベルテキスト (NULL でなし) */

mlkbool mSysDlg_inputText(mWindow *parent,
	const char *title,const char *message,uint32_t flags,mStr *strdst)
{
	_dialog *p;
	mlkbool ret;

	p = _create_dialog(parent, title, message, strdst->buf, flags, NULL);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		mLineEditGetTextStr(p->edit, strdst);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

/**@ 数値入力ダイアログ */

mlkbool mSysDlg_inputTextNum(mWindow *parent,
	const char *title,const char *message,uint32_t flags,int min,int max,int *dst)
{
	_dialog *p;
	int state[3];
	mlkbool ret;

	state[0] = min;
	state[1] = max;
	state[2] = *dst;

	//ダイアログ

	p = _create_dialog(parent, title, message, NULL, flags, state);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		*dst = mLineEditGetNum(p->edit);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

