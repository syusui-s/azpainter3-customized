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
 * レイヤ色選択ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_lineedit.h"
#include "mlk_colorprev.h"
#include "mlk_hsvpicker.h"
#include "mlk_sliderbar.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_color.h"

#include "def_draw.h"
#include "draw_main.h"

#include "widget_func.h"
#include "apphelp.h"

#include "trid.h"
#include "trid_layerdlg.h"


//------------------------

typedef struct
{
	MLK_DIALOG_DEF

	int coltype;	//現在のカラータイプ
	uint32_t curcol;

	mColorPrev *prev;
	mHSVPicker *picker;
	mLabel *label[3];
	mSliderBar *slider[3];
	mLineEdit *edit[3];
	mWidget *palette;
}_dialog;

//------------------------

#define _COLTYPE_RGB 0
#define _COLTYPE_HSV 1

#define _SETCOLORVAL_CHANGE_TYPE   0 //カラータイプの変更
#define _SETCOLORVAL_CHANGE_PICKER 1 //HSV ピッカーからの変更
#define _SETCOLORVAL_FROM_CURCOL   2 //curcol から変更

enum
{
	WID_HSV = 100,
	WID_PALETTE,
	WID_CK_RGB,
	WID_CK_HSV,
	WID_SLIDER1,
	WID_SLIDER2,
	WID_SLIDER3,
	WID_EDIT1,
	WID_EDIT2,
	WID_EDIT3,
	WID_BTT_HELP,
	WID_BTT_SETCOL,
	WID_BTT_GETCOL
};

static const char *g_colname[2][3] = {{"R","G","B"},{"H","S","V"}};

//------------------------


//************************************
// パレット
//************************************

/* 押された時、通知。
 *  param1 = 登録かどうか
 *  param2 = パレット位置 */


#define _PAL_EACHW  17
#define _PAL_EACHH  17
#define _PAL_H_NUM  (DRAW_LAYERCOLPAL_NUM / 2)
#define _PAL_V_NUM  2


/* イベント */

static int _palette_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.act == MEVENT_POINTER_ACT_PRESS)
	{
		int x,y,fregist;

		//ボタン

		if(ev->pt.btt == MLK_BTT_LEFT)
			fregist = ev->pt.state & (MLK_STATE_CTRL | MLK_STATE_SHIFT);
		else if(ev->pt.btt == MLK_BTT_RIGHT)
			fregist = TRUE;
		else
			return 1;

		//パレット位置

		x = (ev->pt.x - 1) / _PAL_EACHW;
		y = (ev->pt.y - 1) / _PAL_EACHH;

		if(x < 0) x = 0; else if(x >= _PAL_H_NUM) x = _PAL_H_NUM - 1;
		if(y < 0) y = 0; else if(y >= _PAL_V_NUM) y = _PAL_V_NUM - 1;

		//通知

		mWidgetEventAdd_notify(wg, NULL, 0, fregist, x + y * _PAL_H_NUM);
	}

	return 1;
}

/* 描画 */

static void _palette_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int ix,iy,x,y;
	uint32_t *pcol,col_white;

	pcol = APPDRAW->col.layercolpal;

	col_white = MGUICOL_PIX(WHITE);

	for(iy = 0, y = 0; iy < _PAL_V_NUM; iy++, y += _PAL_EACHH)
	{
		for(ix = 0, x = 0; ix < _PAL_H_NUM; ix++, x += _PAL_EACHW, pcol++)
		{
			mPixbufBox(pixbuf, x, y, _PAL_EACHW + 1, _PAL_EACHH + 1, col_white);
			mPixbufBox(pixbuf, x + 1, y + 1, _PAL_EACHW - 1, _PAL_EACHH - 1, 0);

			mPixbufFillBox(pixbuf,
				x + 2, y + 2, _PAL_EACHW - 3, _PAL_EACHH - 3, mRGBtoPix(*pcol));
		}
	}
}

/* パレット作成 */

static mWidget *_palette_new(mWidget *parent)
{
	mWidget *p;

	p = mWidgetNew(parent, 0);
	if(!p) return NULL;

	p->id = WID_PALETTE;
	p->hintW = _PAL_EACHW * _PAL_H_NUM + 1;
	p->hintH = _PAL_EACHH * _PAL_V_NUM + 1;
	p->fevent |= MWIDGET_EVENT_POINTER;

	p->draw = _palette_draw_handle;
	p->event = _palette_event_handle;

	return p;
}



//************************************
// ダイアログ
//************************************


/* カラーの数値をセット */

static void _set_color_val(_dialog *p,int type)
{
	int i,max,c[3];
	uint32_t col;
	double d[3];
	mHSVd hsv;

	//色の数値取得

	col = p->curcol;

	if(p->coltype == _COLTYPE_RGB)
	{
		//RGB
		
		c[0] = MLK_RGB_R(col);
		c[1] = MLK_RGB_G(col);
		c[2] = MLK_RGB_B(col);
	}
	else
	{
		//HSV
		
		if(type == _SETCOLORVAL_CHANGE_PICKER)
			mHSVPickerGetHSVColor(p->picker, d);
		else
		{
			mRGB8pac_to_HSV(&hsv, col);
			d[0] = hsv.h;
			d[1] = hsv.s;
			d[2] = hsv.v;
		}

		c[0] = (int)(d[0] * 60 + 0.5);
		c[1] = (int)(d[1] * 255 + 0.5);
		c[2] = (int)(d[2] * 255 + 0.5);
	}

	//セット

	for(i = 0; i < 3; i++)
	{
		if(type == _SETCOLORVAL_CHANGE_TYPE)
		{
			//範囲

			max = (p->coltype == _COLTYPE_HSV && i == 0)? 359: 255;

			mSliderBarSetRange(p->slider[i], 0, max);
			mLineEditSetNumStatus(p->edit[i], 0, max, 0);

			//名前

			mLabelSetText(p->label[i], g_colname[p->coltype][i]);
		}

		mSliderBarSetPos(p->slider[i], c[i]);
		mLineEditSetNum(p->edit[i], c[i]);
	}

	//再レイアウト

	if(type == _SETCOLORVAL_CHANGE_TYPE)
		mWidgetReLayout(p->label[0]->wg.parent);
}

/* 数値が変更された時の色更新 */

static void _change_value(_dialog *p,int no)
{
	int i,c[3];
	double s,v;

	for(i = 0; i < 3; i++)
		c[i] = mSliderBarGetPos(p->slider[i]);

	if(p->coltype == _COLTYPE_RGB)
	{
		//RGB
		
		p->curcol = MLK_RGB(c[0], c[1], c[2]);

		mHSVPickerSetRGBColor(p->picker, p->curcol);
	}
	else
	{
		//HSV
		
		s = c[1] / 255.0;
		v = c[2] / 255.0;
	
		p->curcol = mHSV_to_RGB8pac(c[0] / 60.0, s, v);

		if(no == 0)
			mHSVPickerSetHue(p->picker, c[0]);
		else
			mHSVPickerSetSV(p->picker, s, v);
	}

	//プレビュー

	mColorPrevSetColor(p->prev, p->curcol);
}

/* 色を変更 */

static void _set_color(_dialog *p,uint32_t col)
{
	p->curcol = col;

	mColorPrevSetColor(p->prev, col);
	mHSVPickerSetRGBColor(p->picker, col);

	_set_color_val(p, _SETCOLORVAL_FROM_CURCOL);
}


//============================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;
	int id,ntype;

	if(ev->type == MEVENT_NOTIFY)
	{
		id = ev->notify.id;
		ntype = ev->notify.notify_type;
	
		switch(id)
		{
			//RGB/HSV 切り替え
			case WID_CK_RGB:
			case WID_CK_HSV:
				if(ntype == MCHECKBUTTON_N_CHANGE)
				{
					p->coltype = id - WID_CK_RGB;
					_set_color_val(p, _SETCOLORVAL_CHANGE_TYPE);
				}
				break;
			//HSV picker
			case WID_HSV:
				if(ntype == MHSVPICKER_N_CHANGE_HUE
					|| ntype == MHSVPICKER_N_CHANGE_SV)
				{
					p->curcol = mHSVPickerGetRGBColor(p->picker);

					mColorPrevSetColor(p->prev, p->curcol);
					_set_color_val(p, _SETCOLORVAL_CHANGE_PICKER);
				}
				break;
			//バー
			case WID_SLIDER1:
			case WID_SLIDER2:
			case WID_SLIDER3:
				if(ntype == MSLIDERBAR_N_ACTION
					&& (ev->notify.param2 & MSLIDERBAR_ACTION_F_CHANGE_POS))
				{
					id -= WID_SLIDER1;
				
					mLineEditSetNum(p->edit[id], ev->notify.param1);
					_change_value(p, id);
				}
				break;
			//エディット
			case WID_EDIT1:
			case WID_EDIT2:
			case WID_EDIT3:
				if(ntype == MLINEEDIT_N_CHANGE)
				{
					id -= WID_EDIT1;

					mSliderBarSetPos(p->slider[id], mLineEditGetNum(p->edit[id]));
					_change_value(p, id);
				}
				break;
			//パレット
			case WID_PALETTE:
				id = ev->notify.param2;
			
				if(ev->notify.param1)
				{
					//登録

					APPDRAW->col.layercolpal[id] = p->curcol;

					mWidgetRedraw(p->palette);
				}
				else
				{
					//色取得

					_set_color(p, APPDRAW->col.layercolpal[id]);
				}
				break;

			//描画色からセット
			case WID_BTT_SETCOL:
				_set_color(p, RGBcombo_to_32bit(&APPDRAW->col.drawcol));
				break;
			//描画色にセット
			case WID_BTT_GETCOL:
				drawColor_setDrawColor_update(p->curcol);
				break;
			//ヘルプ
			case WID_BTT_HELP:
				AppHelp_message(MLK_WINDOW(p), HELP_TRGROUP_SINGLE, HELP_TRID_LAYER_COLOR);
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ウィジェット作成 */

static void _create_widget(_dialog *p)
{
	mWidget *ct,*cth,*ctv;
	int i;

	cth = mContainerCreateHorz(MLK_WIDGET(p), 18, MLF_EXPAND_W, 0);

	//-------- 左側 (色)

	ct = mContainerCreateVert(cth, 10, 0, 0);

	//色プレビュー

	p->prev = mColorPrevCreate(ct, 60, 40, 0, MCOLORPREV_S_FRAME, p->curcol);

	//HSV

	p->picker = mHSVPickerCreate(ct, WID_HSV, 0, 0, 110);

	mHSVPickerSetRGBColor(p->picker, p->curcol);

	//--------- 右側

	ctv = mContainerCreateVert(cth, 0, MLF_EXPAND_W, 0);

	//RGB/HSV

	ct = mContainerCreateHorz(ctv, 5, 0, 0);

	mCheckButtonCreate(ct, WID_CK_RGB, 0, 0, MCHECKBUTTON_S_RADIO, "RGB", FALSE);
	mCheckButtonCreate(ct, WID_CK_HSV, 0, 0, MCHECKBUTTON_S_RADIO, "HSV", TRUE);

	//バー/値

	ct = mContainerCreateGrid(ctv, 3, 5, 4, MLF_EXPAND_W, MLK_MAKE32_4(0,8,0,12));

	for(i = 0; i < 3; i++)
	{
		p->label[i] = mLabelCreate(ct, MLF_MIDDLE, 0, 0, NULL);

		//mSliderBar

		p->slider[i] = mSliderBarCreate(ct, WID_SLIDER1 + i, MLF_EXPAND_W | MLF_MIDDLE, 0, 0);
		p->slider[i]->wg.initW = 180;

		//mLineEdit

		p->edit[i] = mLineEditCreate(ct, WID_EDIT1 + i, MLF_MIDDLE, 0,
			MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

		mLineEditSetWidth_textlen(p->edit[i], 4);
	}

	//パレット

	ct = mContainerCreateHorz(ctv, 5, 0, 0);

	p->palette = _palette_new(ct);

	widget_createHelpButton(ct, WID_BTT_HELP, 0, 0);

	//----- ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, 0, MLK_MAKE32_4(0,10,0,0));

	mButtonCreate(ct, WID_BTT_SETCOL, 0, 0, 0, MLK_TR(TRID_LAYERDLG_FROM_DRAWCOL));

	mButtonCreate(ct, WID_BTT_GETCOL, 0, 0, 0, MLK_TR(TRID_LAYERDLG_TO_DRAWCOL));
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,uint32_t defcol)
{
	_dialog *p;

	MLK_TRGROUP(TRGROUP_DLG_LAYER);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_LAYERDLG_TITLE_COLOR), _event_handle);

	if(!p) return NULL;

	p->curcol = defcol;
	p->coltype = _COLTYPE_HSV;

	//ウィジェット

	_create_widget(p);

	_set_color_val(p, _SETCOLORVAL_CHANGE_TYPE);

	//OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** ダイアログ実行
 *
 * pcol: 色をセットしておく。OK 時、色が入る。 */

mlkbool LayerDialog_color(mWindow *parent,uint32_t *pcol)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent, *pcol);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		*pcol = p->curcol;

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


