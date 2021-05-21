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
 * 色選択ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_color.h"

#include "mlk_label.h"
#include "mlk_colorprev.h"
#include "mlk_hsvpicker.h"
#include "mlk_sliderbar.h"
#include "mlk_lineedit.h"


//-------------------------

#define _DIALOG(p)  ((_dialog *)(p))

typedef struct
{
	MLK_DIALOG_DEF

	mHSVPicker *hsv;
	mColorPrev *prev;
	mSliderBar *slider[6];
	mLineEdit *edit[6];

	mRgbCol curcol;
}_dialog;

//-------------------------

enum
{
	WID_HSV    = 100,
	WID_SLIDER = 200,
	WID_EDIT   = 300
};

#define _SETVAL_HSV  1
#define _SETVAL_RGB  2
#define _SETVAL_BOTH 3

//色名 (静的文字列としてセットする)
static const char *g_col_label[6] = {
	"H","S","V","R","G","B"
};

//-------------------------


//========================
// sub
//========================


/** スライダーとエディットの数値セット */

static void _set_val_widget(_dialog *p,int no,int val)
{
	mSliderBarSetPos(p->slider[no], val);
	mLineEditSetNum(p->edit[no], val);
}

/** HSV/RGB 値から数値セット */

static void _set_color_val(_dialog *p,double *hsv,mRgbCol rgb,uint8_t flags)
{
	int i,n;

	//HSV

	if(flags & _SETVAL_HSV)
	{
		for(i = 0; i < 3; i++)
		{
			if(i == 0)
				//[!] H の値は 0.0〜6.0
				n = (int)(hsv[0] * 60 + 0.5);
			else
				n = (int)(hsv[i] * 100 + 0.5);

			_set_val_widget(p, i, n);
		}
	}

	//RGB

	if(flags & _SETVAL_RGB)
	{
		for(i = 0; i < 3; i++)
			_set_val_widget(p, i + 3, (rgb >> ((2 - i) << 3)) & 255);
	}
}

/** 色をセット */

static void _set_color(_dialog *p,mRgbCol col)
{
	mHSVd hsv;

	p->curcol = col;

	//プレビュー
	
	mColorPrevSetColor(p->prev, col);

	//HSV ピッカー

	mHSVPickerSetRGBColor(p->hsv, col);

	//数値

	mRGB8pac_to_HSV(&hsv, col);

	_set_color_val(p, (double *)&hsv, col, _SETVAL_BOTH);
}


//====================
// イベント
//====================


/** RGB/HSV の各色が変更された時 (スライダー/エディット) */

static void _change_val(_dialog *p,int no)
{
	int i,n[3];
	mHSVd hsv;
	uint32_t col;

	//もう一方のタイプの値をセット

	if(no < 3)
	{
		//------ HSV 変更

		//HSV 値

		for(i = 0; i < 3; i++)
			n[i] = mSliderBarGetPos(p->slider[i]);

		//HSV -> RGB

		col = mHSV_to_RGB8pac(n[0] / 60.0, n[1] / 100.0, n[2] / 100.0);

		//RGB 数値

		_set_color_val(p, NULL, col, _SETVAL_RGB);
	}
	else
	{
		//------ RGB 変更

		//RGB 値

		for(i = 0; i < 3; i++)
			n[i] = mSliderBarGetPos(p->slider[i + 3]);

		//色

		col = MLK_RGB(n[0], n[1], n[2]);

		//HSV 数値

		mRGB8pac_to_HSV(&hsv, col);

		_set_color_val(p, (double *)&hsv, 0, _SETVAL_HSV);
	}

	//プレビュー

	mColorPrevSetColor(p->prev, col);

	//現在色セット

	p->curcol = col;

	//HSV ピッカー連動

	if(no == 0)
	{
		//H

		mHSVPickerSetHue(p->hsv, mSliderBarGetPos(p->slider[0]));
	}
	else if(no < 3)
	{
		//SV

		mHSVPickerSetSV(p->hsv,
			mSliderBarGetPos(p->slider[1]) / 100.0,
			mSliderBarGetPos(p->slider[2]) / 100.0);
	}
	else
		//RGB
		mHSVPickerSetRGBColor(p->hsv, p->curcol);
}

/** 通知イベント */

static void _event_notify(_dialog *p,mEvent *ev)
{
	int id,type,n;
	double d[3];

	id = ev->notify.id;
	type = ev->notify.notify_type;

	if(id == WID_HSV)
	{
		//------ HSV ピッカー

		p->curcol = ev->notify.param1;

		//プレビュー

		mColorPrevSetColor(p->prev, p->curcol);
		
		//数値

		mHSVPickerGetHSVColor(p->hsv, d);

		_set_color_val(p, d, p->curcol, _SETVAL_BOTH);
	}
	else if(id >= WID_SLIDER && id < WID_SLIDER + 6)
	{
		//------ スライダー

		if(type == MSLIDERBAR_N_ACTION
			&& (ev->notify.param2 & MSLIDERBAR_ACTION_F_CHANGE_POS))
		{
			n = id - WID_SLIDER;

			mLineEditSetNum(p->edit[n], ev->notify.param1);

			_change_val(p, n);
		}
	}
	else if(id >= WID_EDIT && id < WID_EDIT + 6)
	{
		//------ エディット数値

		if(type == MLINEEDIT_N_CHANGE)
		{
			n = id - WID_EDIT;

			mSliderBarSetPos(p->slider[n], mLineEditGetNum(p->edit[n]));

			_change_val(p, n);
		}
	}
}

/** イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
		_event_notify(_DIALOG(wg), ev);

	return mDialogEventDefault_okcancel(wg, ev);
}


//====================
// 作成
//====================


/** ウィジェット作成 */

static void _create_widget(_dialog *p)
{
	mWidget *cth,*ct,*wg;
	int i,max;
	uint32_t margin;

	//トップレイアウト

	mContainerSetPadding_same(MLK_CONTAINER(p), 10);

	//horz

	cth = mContainerCreateHorz(MLK_WIDGET(p), 20, MLF_EXPAND_WH, 0);

	//------ 色

	ct = mContainerCreateVert(cth, 20, 0, 0);

	//色プレビュー

	p->prev = mColorPrevCreate(ct, 60, 50, 0, MCOLORPREV_S_FRAME, 0);

	//HSV

	p->hsv = mHSVPickerCreate(ct, WID_HSV, 0, 0, 120);

	//------ 色値

	ct = mContainerCreateGrid(cth, 3, 5, 5, MLF_EXPAND_W, 0);

	for(i = 0; i < 6; i++)
	{
		if(i < 3)
			max = (i == 0)? 359: 100;
		else
			max = 255;

		//RGB と HSV の間の bottom
		margin = (i == 2)? MLK_MAKE32_4(0,0,0,15): 0;

		//ラベル

		mLabelCreate(ct, MLF_MIDDLE, margin, 0, g_col_label[i]);

		//スライダー

		p->slider[i] = mSliderBarCreate(ct, WID_SLIDER + i,
			MLF_EXPAND_W | MLF_MIDDLE, margin | 0x03000400, 0);

		wg = (mWidget *)p->slider[i];

		wg->initW = 150;

		mSliderBarSetStatus(MLK_SLIDERBAR(wg), 0, max, 0);

		//エディット

		p->edit[i] = mLineEditCreate(ct, WID_EDIT + i,
			MLF_MIDDLE, margin,
			MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

		wg = (mWidget *)p->edit[i];

		mLineEditSetWidth_textlen(MLK_LINEEDIT(wg), 5);
		mLineEditSetNumStatus(MLK_LINEEDIT(wg), 0, max, 0);
	}

	//------ OK/CANCEL

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));
}

/** ダイアログ作成 */

static _dialog *_dialog_new(mWindow *parent)
{
	_dialog *p;
	
	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;

	//タイトル

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR_SYS(MLK_TRSYS_SELECT_COLOR));

	//ウィジェット

	_create_widget(p);
		
	return p;
}


//*******************************
// 関数
//*******************************


/**@ 色選択ダイアログ
 *
 * @p:col 初期色を入れておく。また、結果の色が入る。*/

mlkbool mSysDlg_selectColor(mWindow *parent,mRgbCol *col)
{
	_dialog *p;
	int ret;

	p = _dialog_new(parent);
	if(!p) return FALSE;

	_set_color(p, *col);

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		*col = p->curcol;

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

