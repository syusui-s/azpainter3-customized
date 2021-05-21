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
 * 矩形編集:変形ダイアログ
 *****************************************/

#include <string.h> //memcpy
#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_checkbutton.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_groupbox.h"
#include "mlk_button.h"
#include "mlk_event.h"
#include "mlk_rectbox.h"
#include "mlk_str.h"

#include "def_draw.h"
#include "def_config.h"

#include "canvas_slider.h"
#include "widget_func.h"

#include "trid.h"

#include "pv_transformdlg.h"


//---------------------

typedef struct
{
	MLK_DIALOG_DEF

	TransformView *view;
	CanvasSlider *zoombar;
	mLineEdit *edit[3];
	mWidget *wg_apply;
}_dialog;

//---------------------

enum
{
	TRID_TITLE,
	TRID_TYPE_NORMAL,
	TRID_TYPE_TRAPEZOID,
	TRID_RESET,
	TRID_X_SCALE,
	TRID_Y_SCALE,
	TRID_ANGLE,
	TRID_KEEP_ASPECT,
	TRID_APPLY,
	TRID_HELP
};

enum
{
	WID_VIEW = 100,
	WID_ZOOMBAR,
	WID_BTT_RESET,
	WID_EDIT_SCALEX,
	WID_EDIT_SCALEY,
	WID_EDIT_ROTATE,
	WID_CK_KEEPASPECT,
	WID_BTT_APPLY,

	WID_CK_TYPE_NORMAL = 200,
	WID_CK_TYPE_TRAPEZOID
};

//---------------------



/* mLineEdit にパラメータ値セット */

static void _set_edit_value(_dialog *p)
{
	double d;

	d = -(p->view->angle) / MLK_MATH_PI * 180;
	while(d > 360) d -= 360;
	while(d < 0) d += 360;

	mLineEditSetDouble(p->edit[0], p->view->scalex, 5);
	mLineEditSetDouble(p->edit[1], p->view->scaley, 5);
	mLineEditSetDouble(p->edit[2], d, 3);
}

/* 値を適用 */

static void _apply_value(_dialog *p)
{
	double sx,sy,rd;

	sx = mLineEditGetDouble(p->edit[0]);
	sy = mLineEditGetDouble(p->edit[1]);
	rd = mLineEditGetDouble(p->edit[2]);

	if(sx < 0.001) sx = 0.001;
	if(sy < 0.001) sy = 0.001;

	while(rd > 360) rd -= 360;
	while(rd < -360) rd += 360;

	rd = rd / 180 * MLK_MATH_PI;

	//

	p->view->scalex = sx;
	p->view->scaley = sy;
	p->view->angle = -rd;

	TransformView_update(p->view, TRANSFORM_UPDATE_F_TRANSPARAM);
}

/* XY倍率のエディット編集時、縦横比維持の場合はもう一方にも同じ値をセット */

static void _set_edit_keepaspect(_dialog *p,int no)
{
	mStr str = MSTR_INIT;

	mLineEditGetTextStr(p->edit[no], &str);

	mLineEditSetText(p->edit[no ^ 1], str.buf);

	mStrFree(&str);
}

/* 変形パラメータのリセット */

static void _reset_param(_dialog *p)
{
	TransformView_resetParam(p->view);
	TransformView_update(p->view, TRANSFORM_UPDATE_F_TRANSPARAM);

	_set_edit_value(p);
}

/* 変形タイプ変更 */

static void _set_transform_type(_dialog *p,int type)
{
	int i,f;

	p->view->type = type;

	//パラメータリセット

	_reset_param(p);

	//有効/無効

	f = (type == TRANSFORM_TYPE_NORMAL);

	for(i = 0; i < 3; i++)
		mWidgetEnable(MLK_WIDGET(p->edit[i]), f);

	mWidgetEnable(p->wg_apply, f);
}


//====================


/* 表示倍率バー */

static void _notify_zoombar(CanvasSlider *bar,TransformView *p,mEventNotify *ev)
{
	int val,n;

	val = ev->param1;

	switch(ev->notify_type)
	{
		//ドラッグ開始
		case CANVASSLIDER_N_BAR_PRESS:
			TransformView_startCanvasZoomDrag(p);
			break;
		//ドラッグ終了
		case CANVASSLIDER_N_BAR_RELEASE:
			p->low_quality = 0;
			break;
		//ボタン
		case CANVASSLIDER_N_PRESSED_BUTTON:
			if(val == CANVASSLIDER_BTT_RESET)
				val = 1000;
			else if(val == CANVASSLIDER_BTT_ADD)
			{
				//拡大

				val = p->canv_zoom;

				if(val >= 1000)
					val = (val / 1000 + 1) * 1000;
				else
				{
					n = (int)(val * 1.15 + 0.5);
					if(n == val) n++;
					if(n > 1000) n = 1000;

					val = n;
				}

				if(val >= TRANSFORM_ZOOM_MAX)
					val = TRANSFORM_ZOOM_MAX;
			}
			else
			{
				//縮小

				val = p->canv_zoom;

				if(val > 1000)
					val = (val - 1) / 1000 * 1000;
				else
				{
					n = (int)(val * 0.85 + 0.5);
					if(n == val) n--;
					if(n < 1) n= 1;

					val = n;
				}
			}

			CanvasSlider_setValue(bar, val);
			break;
	}

	//倍率変更

	TransformView_setCanvasZoom(p, val);
}

/* イベント */

static int _dlg_event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;
	int id,ntype;

	if(ev->type == MEVENT_NOTIFY)
	{
		id = ev->notify.id;
		ntype = ev->notify.notify_type;

		switch(id)
		{
			//ビュー
			case WID_VIEW:
				if(ntype == TRANSFORM_NOTIFY_CHANGE_AFFINE_PARAM)
					//アフィン変換のパラメータ変更
					_set_edit_value(p);
				else
					//キャンバス表示倍率変更
					CanvasSlider_setValue(p->zoombar, p->view->canv_zoom);
				break;
		
			//表示倍率バー
			case WID_ZOOMBAR:
				_notify_zoombar(p->zoombar, p->view, (mEventNotify *)ev);
				break;

			//XY倍率エディット
			case WID_EDIT_SCALEX:
			case WID_EDIT_SCALEY:
				if(ntype == MLINEEDIT_N_CHANGE
					&& p->view->keep_aspect)
					_set_edit_keepaspect(p, id - WID_EDIT_SCALEX);
				break;

			//タイプ
			case WID_CK_TYPE_NORMAL:
			case WID_CK_TYPE_TRAPEZOID:
				_set_transform_type(p, id - WID_CK_TYPE_NORMAL);
				break;
		
			//リセット
			case WID_BTT_RESET:
				_reset_param(p);
				break;
			//値の適用
			case WID_BTT_APPLY:
				_apply_value(p);
				break;
			//縦横比維持
			case WID_CK_KEEPASPECT:
				p->view->keep_aspect ^= 1;
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//=======================
// main
//=======================


/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,TileImage *imgcopy)
{
	_dialog *p;
	mWidget *ct,*ct2,*ct3;
	int i;

	MLK_TRGROUP(TRGROUP_DLG_TRANSFORM);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog), MLK_TR(TRID_TITLE), _dlg_event_handle);
	if(!p) return NULL;

	mContainerSetType_horz(MLK_CONTAINER(p), 12);

	//========= [左側]

	ct = mContainerCreateVert(MLK_WIDGET(p), 10, MLF_EXPAND_WH, 0);

	//ビュー

	p->view = TransformView_create(ct, WID_VIEW);

	p->view->imgcopy = imgcopy;

	//倍率バー

	p->zoombar = CanvasSlider_new(ct, CANVASSLIDER_TYPE_ZOOM, TRANSFORM_ZOOM_MAX);

	MLK_WIDGET(p->zoombar)->id = WID_ZOOMBAR;

	//========= [右側]

	ct = mContainerCreateVert(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0);

	//タイプ

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_CK_TYPE_NORMAL + i, 0, MLK_MAKE32_4(0,3,0,0),
			MCHECKBUTTON_S_RADIO,
			MLK_TR(TRID_TYPE_NORMAL + i), (i == 0));
	}

	//リセットボタン

	mButtonCreate(ct, WID_BTT_RESET, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0), 0, MLK_TR(TRID_RESET));

	//---- 通常時の値

	ct2 = (mWidget *)mGroupBoxCreate(ct, MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0), 0, MLK_TR(TRID_TYPE_NORMAL));

	//edit

	ct3 = mContainerCreateGrid(ct2, 2, 6, 7, MLF_EXPAND_W, 0);

	for(i = 0; i < 3; i++)
	{
		mLabelCreate(ct3, MLF_MIDDLE, 0, 0, MLK_TR(TRID_X_SCALE + i));

		p->edit[i] = mLineEditCreate(ct3, WID_EDIT_SCALEX + i, MLF_EXPAND_W, 0,
			(i == 2)? 0: MLINEEDIT_S_NOTIFY_CHANGE);
	}

	//縦横比維持

	mCheckButtonCreate(ct2, WID_CK_KEEPASPECT, 0, MLK_MAKE32_4(0,8,0,0), 0,
		MLK_TR(TRID_KEEP_ASPECT),
		APPCONF->transform.flags & CONFIG_TRANSFORM_F_KEEP_ASPECT);

	//適用

	p->wg_apply = (mWidget *)mButtonCreate(ct2, WID_BTT_APPLY, MLF_EXPAND_W, MLK_MAKE32_4(0,8,0,0), 0, MLK_TR(TRID_APPLY));
	
	p->wg_apply->fstate |= MWIDGET_STATE_ENTER_SEND;

	//-------

	//ヘルプ

	mLabelCreate(ct, MLF_EXPAND_W | MLF_EXPAND_Y | MLF_BOTTOM, MLK_MAKE32_4(0,6,0,8),
		MLABEL_S_BORDER, MLK_TR(TRID_HELP));

	//OK/キャンセル

	mButtonCreate(ct, MLK_WID_OK, MLF_EXPAND_W, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));
	
	mButtonCreate(ct, MLK_WID_CANCEL, MLF_EXPAND_W, MLK_MAKE32_4(0,3,0,0), 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	//------- 初期化
	// :表示倍率は自動で計算される。

	TransformView_init(p->view);

	_set_edit_value(p);

	CanvasSlider_setValue(p->zoombar, p->view->canv_zoom);
	
	return p;
}

/* OK 時 */

static int _set_data(TransformView *p,double *dstval,mRect *rcdst)
{
	int type = p->type;

	if(type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換

		dstval[0] = p->scalex;
		dstval[1] = p->scaley;
		dstval[2] = p->dcos;
		dstval[3] = p->dsin;
		dstval[4] = p->pt_mov.x;
		dstval[5] = p->pt_mov.y;
	}
	else
	{
		//射影変換

		memcpy(dstval, p->homog_param, sizeof(double) * 9);

		//平行移動

		dstval[9] = p->pt_mov.x;
		dstval[10] = p->pt_mov.y;

		//描画範囲

		TransformView_homog_getDstImageRect(p, rcdst);
	}

	return type;
}

/** ダイアログ実行
 *
 * dstval: 描画用パラメータ ([アフィン] 6個 [射影] 11個)
 * rcdst: 射影変換の描画先範囲
 * return: [-1] キャンセル [0] 通常 [1] 遠近 */

int TransformDlg_run(mWindow *parent,TileImage *imgcopy,double *dstval,mRect *rcdst)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent, imgcopy);
	if(!p) return -1;

	//実行

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	//ビューのサイズ記録

	APPCONF->transform.view_w = p->view->wg.w;
	APPCONF->transform.view_h = p->view->wg.h;

	//オプションフラグ

	APPCONF->transform.flags = 0;
	if(p->view->keep_aspect) APPCONF->transform.flags |= CONFIG_TRANSFORM_F_KEEP_ASPECT;

	//戻り値 + パラメータ値

	if(!ret)
		ret = -1;
	else
		ret = _set_data(p->view, dstval, rcdst);

	//

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

