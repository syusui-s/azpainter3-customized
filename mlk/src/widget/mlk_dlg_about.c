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
 * About ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_sysdlg.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_event.h"


//=============================
// dialog
//=============================


#define WID_BTT_LICENSE 100


/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == WID_BTT_LICENSE)
	{
		//ライセンスボタン
		
		mMessageBox(MLK_WINDOW(wg), "License", (const char *)wg->param1,
			MMESBOX_OK, MMESBOX_OK);

		return 1;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/** ダイアログ作成 */

static mWidget *_create_dialog(mWindow *parent,const char *copying,const char *license)
{
	mWidget *p,*ct;
	
	p = (mWidget *)mDialogNew(parent, 0,
			MTOPLEVEL_S_DIALOG_NORMAL | MTOPLEVEL_S_NO_INPUT_METHOD);

	if(!p) return NULL;
	
	p->event = _event_handle;
	p->param1 = (intptr_t)license;

	mContainerSetType_vert(MLK_CONTAINER(p), 16);
	mContainerSetPadding_same(MLK_CONTAINER(p), 10);

	mToplevelSetTitle(MLK_TOPLEVEL(p), "about");

	//著作権ラベル

	mLabelCreate(p, MLF_EXPAND_W, 0, MLABEL_S_CENTER, copying);

	//----- ボタン

	ct = mContainerCreateHorz(p, 5, MLF_CENTER | MLF_EXPAND_X, 0);

	//OK

	mButtonCreate(ct, MLK_WID_OK, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));

	//ライセンス

	if(license)
		mButtonCreate(ct, WID_BTT_LICENSE, 0, 0, 0, "License");
	
	return p;
}


//==========================
// 関数
//==========================


/**@ ソフト情報ダイアログ */

void mSysDlg_about(mWindow *parent,const char *label)
{
	mWidget *p;

	p = _create_dialog(parent, label, NULL);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	mDialogRun(MLK_DIALOG(p), TRUE);
}

/**@ ソフト情報ダイアログ (著作権とライセンスボタン) */

void mSysDlg_about_license(mWindow *parent,const char *copying,const char *license)
{
	mWidget *p;

	p = _create_dialog(parent, copying, license);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	mDialogRun(MLK_DIALOG(p), TRUE);
}

