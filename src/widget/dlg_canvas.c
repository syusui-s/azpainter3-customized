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
 * キャンバス関連ダイアログ
 *
 * [キャンバスサイズ変更]
 * [キャンバス拡大縮小]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_groupbox.h"
#include "mlk_label.h"
#include "mlk_checkbutton.h"
#include "mlk_lineedit.h"
#include "mlk_combobox.h"
#include "mlk_event.h"

#include "def_macro.h"
#include "def_config.h"

#include "dialogs.h"
#include "widget_func.h"

#include "trid.h"


//--------------------

enum
{
	TRID_TITLE_RESIZE,
	TRID_TITLE_SCALE,

	TRID_ALIGN = 100,
	TRID_CROP,
	TRID_RATIO,
	TRID_KEEP_ASPECT,
	TRID_CHANGE_DPI,
	TRID_METHOD
};

//--------------------



//*****************************************
// キャンバスサイズ変更
//*****************************************


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[2];
	mCheckButton *ck_align_top,
		*ck_crop;
}_dlg_size;

//----------------------


/* ダイアログ作成 */

static _dlg_size *_resize_create(mWindow *parent,int initw,int inith)
{
	_dlg_size *p;
	mWidget *ct;
	mCheckButton *ckbtt;
	int i;

	MLK_TRGROUP(TRGROUP_DLG_CANVAS);

	p = (_dlg_size *)widget_createDialog(parent, sizeof(_dlg_size),
		MLK_TR(TRID_TITLE_RESIZE), mDialogEventDefault_okcancel);

	if(!p) return NULL;

	//------ 幅・高さ

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 10, 8, 0, 0);

	//幅

	widget_createLabel_width(ct);

	p->edit[0] = widget_createEdit_num(ct, 8, 1, IMAGE_SIZE_MAX, 0, initw);

	//高さ

	widget_createLabel_height(ct);

	p->edit[1] = widget_createEdit_num(ct, 8, 1, IMAGE_SIZE_MAX, 0, inith);

	//----- 配置

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,12,0,0), 0, MLK_TR(TRID_ALIGN));

	mContainerSetType_grid(MLK_CONTAINER(ct), 3, 2, 2);

	for(i = 0; i < 9; i++)
	{
		ckbtt = mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO, NULL, (i == 0));

		if(i == 0) p->ck_align_top = ckbtt;
	}

	//範囲外を切り取る

	p->ck_crop = mCheckButtonCreate(MLK_WIDGET(p), 0, 0, MLK_MAKE32_4(0,10,0,0), 0, MLK_TR(TRID_CROP), FALSE);

	//OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** キャンバスサイズ変更ダイアログ
 *
 * dst_size: 現在の値を入れておく。
 * dst_option: [下位4bit:配置][bit4:切り取り] */

mlkbool CanvasDialog_resize(mWindow *parent,mSize *dst_size,int *dst_option)
{
	_dlg_size *p;
	mlkbool ret;
	int opt;

	p = _resize_create(parent, dst_size->w, dst_size->h);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		dst_size->w = mLineEditGetNum(p->edit[0]);
		dst_size->h = mLineEditGetNum(p->edit[1]);

		opt = mCheckButtonGetGroupSel(p->ck_align_top);

		if(mCheckButtonIsChecked(p->ck_crop))
			opt |= 1<<4;

		*dst_option = opt;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//*****************************************
// キャンバス拡大縮小
//*****************************************


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[4],
		*edit_dpi;
	mCheckButton *ck_aspect,
		*ck_dpi;
	mComboBox *cb_type;

	CanvasScaleInfo *info;

	int sizeval[4],		//[0]幅 [1]高さ [2]%幅 [3]%高さ
		keep_aspect;
}_dlg_scale;

enum
{
	WID_SCALE_EDIT_W = 100,
	WID_SCALE_EDIT_H,
	WID_SCALE_EDIT_PERS_W,
	WID_SCALE_EDIT_PERS_H,
	WID_SCALE_CK_KEEP_ASPECT,
	WID_SCALE_CK_CHANGE_DPI,
	WID_SCALE_EDIT_DPI,
	WID_SCALE_CB_TYPE
};

//----------------------


/* sizeval から mLineEdit にセット
 *
 * no: 0-3 で、指定値以外をセット。それ以外ですべてセット。 */

static void _scale_set_edit(_dlg_scale *p,int no)
{
	int i;

	for(i = 0; i < 4; i++)
	{
		if(no != i)
			mLineEditSetNum(p->edit[i], p->sizeval[i]);
	}
}

/* サイズのエディット変更時 */

static void _scale_change_sizeval(_dlg_scale *p,int no)
{
	int n1,n2,sw,rw;

	//sw=変更値の元のサイズ, rw=変更値と逆の辺の元サイズ

	if(no == 0 || no == 2)
		sw = p->info->w, rw = p->info->h;
	else
		sw = p->info->h, rw = p->info->w;

	//

	if(no < 2)
	{
		//px

		n1 = no;
		n2 = 1 - no;

		p->sizeval[n1] = mLineEditGetNum(p->edit[n1]);
		p->sizeval[2 + n1] = (int)((double)p->sizeval[n1] / sw * 1000.0 + 0.5);

		if(p->keep_aspect)
		{
			p->sizeval[n2] = (int)((double)p->sizeval[n1] / sw * rw + 0.5);
			p->sizeval[2 + n2] = p->sizeval[2 + n1];
		}
	}
	else
	{
		//%

		n1 = no;
		n2 = 5 - no;

		p->sizeval[n1] = mLineEditGetNum(p->edit[n1]);
		p->sizeval[n1 - 2] = (int)((double)p->sizeval[n1] / 1000.0 * sw + 0.5);

		if(p->keep_aspect)
		{
			p->sizeval[n2] = p->sizeval[n1];
			p->sizeval[n2 - 2] = (int)((double)p->sizeval[n2] / 1000.0 * rw + 0.5);
		}
	}

	_scale_set_edit(p, no);
}

/* DPI値変更時 */

static void _scale_change_dpival(_dlg_scale *p)
{
	double d;

	d = (double)mLineEditGetNum(p->edit_dpi) / p->info->dpi;

	p->sizeval[0] = (int)(p->info->w * d + 0.5);
	p->sizeval[1] = (int)(p->info->h * d + 0.5);
	p->sizeval[2] = p->sizeval[3] = (int)(d * 1000 + 0.5);

	_scale_set_edit(p, -1);
}

/* DPI変更のチェック変更時 */

static void _scale_toggle_change_dpi(_dlg_scale *p)
{
	int i,f;

	f = mCheckButtonIsChecked(p->ck_dpi);

	for(i = 0; i < 4; i++)
		mWidgetEnable(MLK_WIDGET(p->edit[i]), !f);

	mWidgetEnable(MLK_WIDGET(p->edit_dpi), f);

	if(f) _scale_change_dpival(p);
}

/* イベント */

static int _scale_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dlg_scale *p = (_dlg_scale *)wg;
	
		switch(ev->notify.id)
		{
			//サイズ
			case WID_SCALE_EDIT_W:
			case WID_SCALE_EDIT_H:
			case WID_SCALE_EDIT_PERS_W:
			case WID_SCALE_EDIT_PERS_H:
				if(ev->notify.notify_type == MLINEEDIT_N_CHANGE)
					_scale_change_sizeval(p, ev->notify.id - WID_SCALE_EDIT_W);
				break;
			
			//縦横比維持
			case WID_SCALE_CK_KEEP_ASPECT:
				p->keep_aspect ^= 1;
				break;
			//DPI変更
			case WID_SCALE_CK_CHANGE_DPI:
				_scale_toggle_change_dpi(p);
				break;
			//DPI値
			case WID_SCALE_EDIT_DPI:
				if(ev->notify.notify_type == MLINEEDIT_N_CHANGE)
					_scale_change_dpival(p);
				break;
		}
	}
	
	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dlg_scale *_scale_create(mWindow *parent,CanvasScaleInfo *info)
{
	_dlg_scale *p;
	mWidget *ct,*cth;

	MLK_TRGROUP(TRGROUP_DLG_CANVAS);

	//作成

	p = (_dlg_scale *)widget_createDialog(parent, sizeof(_dlg_scale),
		MLK_TR(TRID_TITLE_SCALE), _scale_event_handle);

	if(!p) return NULL;

	p->info = info;
	p->keep_aspect = 1;

	p->sizeval[0] = info->w;
	p->sizeval[1] = info->h;
	p->sizeval[2] = 1000;
	p->sizeval[3] = 1000;

	//------ サイズ・比率

	cth = mContainerCreateHorz(MLK_WIDGET(p), 10, 0, 0);

	//-- [サイズ]

	ct = (mWidget *)mGroupBoxCreate(cth, 0, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE));

	mContainerSetType_grid(MLK_CONTAINER(ct), 2, 6, 10);

	//幅

	widget_createLabel_width(ct);

	p->edit[0] = widget_createEdit_num_change(ct, WID_SCALE_EDIT_W, 7, 1, IMAGE_SIZE_MAX, 0, info->w);

	//高さ

	widget_createLabel_height(ct);

	p->edit[1] = widget_createEdit_num_change(ct, WID_SCALE_EDIT_H, 7, 1, IMAGE_SIZE_MAX, 0, info->h);

	//-- [比率]

	ct = (mWidget *)mGroupBoxCreate(cth, 0, 0, 0, MLK_TR(TRID_RATIO));

	mContainerSetType_grid(MLK_CONTAINER(ct), 2, 6, 8);

	//幅

	widget_createLabel_width(ct);

	p->edit[2] = widget_createEdit_num_change(ct, WID_SCALE_EDIT_PERS_W, 7, 1, 20000, 1, 1000);

	//高さ

	widget_createLabel_height(ct);

	p->edit[3] = widget_createEdit_num_change(ct, WID_SCALE_EDIT_PERS_H, 7, 1, 20000, 1, 1000);

	//---- 縦横比維持

	p->ck_aspect = mCheckButtonCreate(MLK_WIDGET(p), WID_SCALE_CK_KEEP_ASPECT, 0, MLK_MAKE32_4(0,8,0,0),
		0, MLK_TR(TRID_KEEP_ASPECT), TRUE);

	//---- DPI 変更・補間

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 8, 10, 0, MLK_MAKE32_4(0,12,0,0));

	//DPI変更

	p->ck_dpi = mCheckButtonCreate(ct, WID_SCALE_CK_CHANGE_DPI, 0, 0, 0, MLK_TR(TRID_CHANGE_DPI), FALSE);
	
	p->edit_dpi = widget_createEdit_num_change(ct, WID_SCALE_EDIT_DPI, 6, 1, 10000, 0, info->dpi);

	mWidgetEnable(MLK_WIDGET(p->edit_dpi), FALSE);

	//補間方法

	widget_createLabel(ct, MLK_TR(TRID_METHOD));

	p->cb_type = mComboBoxCreate(ct, WID_SCALE_CB_TYPE, 0, 0, 0);

	mComboBoxAddItems_sepnull(p->cb_type,
		"Nearest neighbor\0mitchell\0Lagrange\0Lanczos2\0Lanczos3\0spline16\0spline36\0blackmansinc2\0blackmansinc3\0", 0);

	mComboBoxSetAutoWidth(p->cb_type);
	mComboBoxSetSelItem_atIndex(p->cb_type, APPCONF->canvas_scale_method);

	//OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** キャンバス拡大縮小ダイアログ
 *
 * w,h,dpi には現在の値を入れておく。 */

mlkbool CanvasDialog_scale(mWindow *parent,CanvasScaleInfo *info)
{
	_dlg_scale *p;
	mlkbool ret;

	p = _scale_create(parent, info);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		info->w = mLineEditGetNum(p->edit[0]);
		info->h = mLineEditGetNum(p->edit[1]);
		info->dpi = (mCheckButtonIsChecked(p->ck_dpi))? mLineEditGetNum(p->edit_dpi): -1;
		info->method = mComboBoxGetItemParam(p->cb_type, -1);

		APPCONF->canvas_scale_method = info->method;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


