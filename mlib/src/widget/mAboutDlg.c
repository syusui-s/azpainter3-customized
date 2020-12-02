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
 * mAboutDlg
 *****************************************/

#include "mDef.h"
#include "mDialog.h"
#include "mWidget.h"
#include "mContainer.h"
#include "mWindow.h"
#include "mLabel.h"
#include "mButton.h"
#include "mEvent.h"
#include "mTrans.h"
#include "mMessageBox.h"



//******************************
// mAboutDlg
//******************************

enum
{
	WID_BTT_LICENSE = 100
};


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//ライセンス
			case WID_BTT_LICENSE:
				mMessageBox(M_WINDOW(wg), "License", (const char *)wg->param, MMESBOX_OK, MMESBOX_OK);
				break;
		}
	}

	return mDialogEventHandle_okcancel(wg, ev);
}

/** ダイアログ作成 */

static mDialog *_create_dialog(mWindow *owner,const char *copying,const char *license)
{
	mDialog *p;
	mButton *btt;
	mWidget *ct;
	
	p = (mDialog *)mDialogNew(0, owner, MWINDOW_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;
	p->wg.param = (intptr_t)license;

	mContainerSetPadding_one(M_CONTAINER(p), 10);
	p->ct.sepW = 16;

	mWindowSetTitle(M_WINDOW(p), "about");

	//ラベル

	mLabelCreate(M_WIDGET(p), MLABEL_S_CENTER, MLF_EXPAND_W, 0, copying);

	//----- ボタン

	ct = mContainerCreate(M_WIDGET(p), MCONTAINER_TYPE_HORZ, 0, 3, MLF_CENTER | MLF_EXPAND_X);

	//OK

	btt = mButtonCreate(ct, M_WID_OK, 0, 0, 0, M_TR_T2(M_TRGROUP_SYS, M_TRSYS_OK));

	btt->wg.fState |= MWIDGET_STATE_ENTER_DEFAULT;

	//ライセンス

	if(license)
		mButtonCreate(ct, WID_BTT_LICENSE, 0, 0, 0, "License");
	
	return p;
}


//******************************
// 関数
//******************************


/** @addtogroup sysdialog
 * @{ */

/** ソフト情報ダイアログ */

void mSysDlgAbout(mWindow *owner,const char *label)
{
	mDialog *p;

	p = _create_dialog(owner, label, NULL);
	if(!p) return;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	mDialogRun(M_DIALOG(p), TRUE);
}

/** ソフト情報ダイアログ (著作権とライセンスボタン) */

void mSysDlgAbout_license(mWindow *owner,const char *copying,const char *license)
{
	mDialog *p;

	p = _create_dialog(owner, copying, license);
	if(!p) return;

	mWindowMoveResizeShow_hintSize(M_WINDOW(p));

	mDialogRun(M_DIALOG(p), TRUE);
}

/* @} */
