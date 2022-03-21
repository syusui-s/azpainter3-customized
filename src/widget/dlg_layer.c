/*$
 Copyright (C) 2013-2022 Azel.

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
 * レイヤ関連ダイアログ
 *
 * [線数一括変換] [レイヤタイプ変更] [複数レイヤ結合]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_groupbox.h"
#include "mlk_checkbutton.h"
#include "mlk_lineedit.h"
#include "mlk_combobox.h"
#include "mlk_event.h"

#include "widget_func.h"

#include "trid.h"
#include "trid_layerdlg.h"


//************************************
// 線数一括変換
//************************************


typedef struct
{
	MLK_DIALOG_DEF

	mCheckButton *ck_all;
	mLineEdit *edit_src,
		*edit_dst;
}_dlg_tonelines;


/* ダイアログ作成 */

static _dlg_tonelines *_tonelines_create(mWindow *parent)
{
	_dlg_tonelines *p;
	mWidget *ct;

	p = (_dlg_tonelines *)mDialogNew(parent, sizeof(_dlg_tonelines), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventDefault_okcancel;

	//

	MLK_TRGROUP(TRGROUP_DLG_LAYER);

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR(TRID_LAYERDLG_TITLE_TONE_LINES));

	//---- 対象

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, 0, 0, MLK_TR(TRID_LAYERDLG_TARGET));

	mContainerSetType_vert(MLK_CONTAINER(ct), 5);

	//すべてのレイヤ

	p->ck_all = mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO, MLK_TR(TRID_LAYERDLG_ALL_LAYER), TRUE);

	//指定線数のレイヤ

	mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO, MLK_TR(TRID_LAYERDLG_SPEC_LINES_LAYER), FALSE);

	//線数

	p->edit_src = widget_createEdit_num(ct, 7, 50, 2000, 1, 600);

	mWidgetSetMargin_pack4(MLK_WIDGET(p->edit_src), MLK_MAKE32_4(15,4,0,0));

	//---- 置き換え値

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, 0, MLK_MAKE32_4(0,15,0,0));

	widget_createLabel(ct, MLK_TR(TRID_LAYERDLG_REPLACE_VALUE));

	p->edit_dst = widget_createEdit_num(ct, 7, 50, 2000, 1, 600);

	//-----

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** 線数一括変換ダイアログ実行
 *
 * return: 0 でキャンセル。下位16bit=対象の指定線数(0ですべて)、上位16bit=置き換える線数 */

uint32_t LayerDialog_batchToneLines(mWindow *parent)
{
	_dlg_tonelines *p;
	uint32_t ret;

	p = _tonelines_create(parent);
	if(!p) return 0;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		if(mCheckButtonIsChecked(p->ck_all))
			ret = 0;
		else
			ret = mLineEditGetNum(p->edit_src);

		ret |= mLineEditGetNum(p->edit_dst) << 16;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//************************************
// レイヤタイプ変更
//************************************


typedef struct
{
	MLK_DIALOG_DEF

	int is_have_color;	//現在のカラータイプが色を持つタイプか

	mCheckButton *ck_type,
		*ck_lumtoa;
}_dlg_type;

#define WID_TYPE_CK_TYPE_TOP 100


/* イベント */

static int _type_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_type *p = (_dlg_type *)wg;

	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id >= WID_TYPE_CK_TYPE_TOP && ev->notify.id < WID_TYPE_CK_TYPE_TOP + 4)
	{
		//タイプ選択変更時
		// :フルカラー/GRAY => A16/A1 へ変更する場合のみ有効

		mWidgetEnable(MLK_WIDGET(p->ck_lumtoa),
			(p->is_have_color && ev->notify.id - WID_TYPE_CK_TYPE_TOP >= 2) );
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* 作成 */

static _dlg_type *_create_dlg_type(mWindow *parent,int curtype,mlkbool is_text)
{
	_dlg_type *p;
	mCheckButton *ckbtt;
	int i;

	p = (_dlg_type *)widget_createDialog(parent, sizeof(_dlg_type),
		MLK_TR2(TRGROUP_DLG_LAYER, TRID_LAYERDLG_TITLE_CHANGE_TYPE), _type_event_handle);

	if(!p) return NULL;

	p->is_have_color = (curtype < 2);

	//---- タイプ

	MLK_TRGROUP(TRGROUP_LAYER_TYPE);

	for(i = 0; i < 4; i++)
	{
		ckbtt = mCheckButtonCreate(MLK_WIDGET(p),
			WID_TYPE_CK_TYPE_TOP + i, 0, MLK_MAKE32_4(0,0,0,3),
			MCHECKBUTTON_S_RADIO, MLK_TR(i), FALSE);

		if(i == 0) p->ck_type = ckbtt;

		//通常レイヤ時、現在のタイプは選択不可
		
		if(!is_text && i == curtype)
			mWidgetEnable(MLK_WIDGET(ckbtt), 0);
	}

	//初期選択
	// :[通常レイヤ] 現在がフルカラーならGRAY、それ以外はフルカラー
	// :[テキストレイヤ] 現在のタイプ

	if(is_text)
		i = curtype;
	else
		i = (curtype == 0);
	
	mCheckButtonSetGroupSel(p->ck_type, i);

	//---- 輝度をアルファ値に

	p->ck_lumtoa = mCheckButtonCreate(MLK_WIDGET(p), 0,
		0, MLK_MAKE32_4(0,6,0,0), 0,
		MLK_TR2(TRGROUP_DLG_LAYER, TRID_LAYERDLG_LUM_TO_ALPHA), FALSE);

	mWidgetEnable(MLK_WIDGET(p->ck_lumtoa), 0);

	//---- OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** レイヤタイプ変更ダイアログ
 *
 * return: -1 でキャンセル。[下位8bit] タイプ [bit9] 輝度からアルファ値変換 */

int LayerDialog_layerType(mWindow *parent,int curtype,mlkbool is_text)
{
	_dlg_type *p;
	int ret = -1;

	p = _create_dlg_type(parent, curtype, is_text);
	if(!p) return -1;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	if(mDialogRun(MLK_DIALOG(p), FALSE))
	{
		ret = mCheckButtonGetGroupSel(p->ck_type);

		if(mCheckButtonIsChecked(p->ck_lumtoa))
			ret |= 1<<9;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//************************************
// 複数レイヤ結合
//************************************


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mlkbool have_checked;

	mCheckButton *ck_target[3],
		*ck_new;
	mComboBox *cb_type;
}_dlg_combine;

enum
{
	WID_COMBINE_PROC_NORMAL = 100,
	WID_COMBINE_PROC_NEW
};

//----------------------


/* イベント */

static int _combin_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_combine *p = (_dlg_combine *)wg;
	int f;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//処理
			case WID_COMBINE_PROC_NORMAL:
			case WID_COMBINE_PROC_NEW:
				if(p->have_checked)
				{
					f = mCheckButtonIsChecked(p->ck_new);

					//チェックされたレイヤ
					
					mWidgetEnable(MLK_WIDGET(p->ck_target[2]), f);

					//チェックされたレイヤが ON の場合、対象を変更
					
					if(!f && mCheckButtonIsChecked(p->ck_target[2]))
						mCheckButtonSetState(p->ck_target[0], 1);
				}
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dlg_combine *_combine_create(mWindow *parent,mlkbool cur_folder)
{
	_dlg_combine *p;
	mWidget *ct;
	int i;
	const uint16_t target_trid[] = {TRID_LAYERDLG_ALL_LAYER, TRID_LAYERDLG_FOLDER_LAYER, TRID_LAYERDLG_CHECKED_LAYER};

	MLK_TRGROUP(TRGROUP_DLG_LAYER);

	p = (_dlg_combine *)widget_createDialog(parent, sizeof(_dlg_combine),
		MLK_TR(TRID_LAYERDLG_TITLE_COMBINE_MULTI), _combin_event_handle);

	if(!p) return NULL;

	//----- 処理

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, 0, 0, MLK_TR(TRID_LAYERDLG_PROC));

	mContainerSetType_vert(MLK_CONTAINER(ct), 5);

	mCheckButtonCreate(ct, WID_COMBINE_PROC_NORMAL, 0, 0,
		MCHECKBUTTON_S_RADIO, MLK_TR(TRID_LAYERDLG_COMBINE_NORMAL), 1);

	p->ck_new = mCheckButtonCreate(ct, WID_COMBINE_PROC_NEW, 0, 0,
		MCHECKBUTTON_S_RADIO, MLK_TR(TRID_LAYERDLG_COMBINE_NEW), 0);

	//---- 対象

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0), 0, MLK_TR(TRID_LAYERDLG_TARGET));

	mContainerSetType_vert(MLK_CONTAINER(ct), 5);

	for(i = 0; i < 3; i++)
	{
		p->ck_target[i] = mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO,
			MLK_TR(target_trid[i]), (i == 0));
	}

	if(!cur_folder)
		mWidgetEnable(MLK_WIDGET(p->ck_target[1]), 0);

	mWidgetEnable(MLK_WIDGET(p->ck_target[2]), 0);

	//結合後のタイプ

	ct = mContainerCreateHorz(MLK_WIDGET(p), 7, 0, MLK_MAKE32_4(0,12,0,0));
	
	p->cb_type = widget_createLabelCombo(ct, MLK_TR(TRID_LAYERDLG_TYPE_AFTER_COMBINE), 0);

	//ヘルプ

	mLabelCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0), MLABEL_S_BORDER, MLK_TR(TRID_LAYERDLG_COMBINE_HELP));

	//OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	//--- レイヤタイプ

	MLK_TRGROUP(TRGROUP_LAYER_TYPE);

	mComboBoxAddItems_tr(p->cb_type, 0, 4, 0);
	mComboBoxSetAutoWidth(p->cb_type);
	mComboBoxSetSelItem_atIndex(p->cb_type, 0);

	return p;
}

/** 複数レイヤ結合ダイアログ
 *
 * have_checked: チェックされたレイヤがあるか
 * return: -1 でキャンセル。[下位2bit:対象] [bit2:新規レイヤ] [bit3-:レイヤタイプ] */

int LayerDialog_combineMulti(mWindow *parent,mlkbool cur_folder,mlkbool have_checked)
{
	_dlg_combine *p;
	int ret;

	p = _combine_create(parent, cur_folder);
	if(!p) return -1;

	p->have_checked = have_checked;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	if(!mDialogRun(MLK_DIALOG(p), FALSE))
		ret = -1;
	else
	{
		ret = mCheckButtonGetGroupSel(p->ck_target[0]);

		if(mCheckButtonIsChecked(p->ck_new)) ret |= 1<<2;

		ret |= mComboBoxGetItemParam(p->cb_type, -1) << 3;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

