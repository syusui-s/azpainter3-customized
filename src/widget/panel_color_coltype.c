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
 * (Panel)カラーの各ページ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_pager.h"
#include "mlk_hsvpicker.h"
#include "mlk_sysdlg.h"
#include "mlk_event.h"
#include "mlk_str.h"
#include "mlk_color.h"

#include "def_widget.h"
#include "def_draw.h"

#include "valuebar.h"
#include "panel.h"
#include "changecol.h"
#include "draw_main.h"
#include "trid.h"


/*-----------------

 - カラーが変更された時、mPager から通知する。
   param1 = CHANGECOL
 
--------------*/


enum
{
	TRID_RGB_INPUT,
	TRID_RGB_INPUT_MES
};



//========================
// 共通
//========================


typedef struct
{
	ValueBar *bar[3];
}_pagedat_bar3;

static const char *g_label_name[] = {"R","G","B","H","S","V","L"};

enum
{
	_LABEL_NO_R,
	_LABEL_NO_G,
	_LABEL_NO_B,
	_LABEL_NO_H,
	_LABEL_NO_S,
	_LABEL_NO_V,
	_LABEL_NO_L
};


/* バーx3 を作成
 *
 * state: 3つの配列。最上位ビットONで最大値 359、OFF で255。以下ラベル番号。 */

static void _create_bar3(ValueBar **arr,mWidget *parent,const uint8_t *state)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		//ラベル

		mLabelCreate(parent, MLF_MIDDLE, 0, 0, g_label_name[state[i] & 15]);

		//バー

		arr[i] = ValueBar_new(parent, 0, MLF_EXPAND_W | MLF_MIDDLE,
			0, 0, (state[i] & 0x80)? 359: 255, 0);
	}
}

/* 色変更時、mPager から通知を送る  */

static void _send_notify(mPager *p,uint32_t col)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p),
		MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW, 0, col, 0);
}

/* バーx3 に HSV 値セット
 *
 * bar: ValueBar 配列
 * col: CHANGECOL */

static void _setbar_hsv(ValueBar **arr,uint32_t col)
{
	int i,c[3];

	ChangeColor_to_HSV_int(c, col);

	for(i = 0; i < 3; i++)
		ValueBar_setPos(arr[i], c[i]);
}



/***************************************
 * RGB
 ***************************************/


#define WID_RGB_INPUT  100


/* 数値入力 */

static void _rgb_input(mPager *p,_pagedat_bar3 *pd)
{
	mStr str = MSTR_INIT;
	int i;

	MLK_TRGROUP(TRGROUP_PANEL_COLOR);

	if(!mSysDlg_inputText(
		Panel_getDialogParent(PANEL_COLOR),
		MLK_TR(TRID_RGB_INPUT), MLK_TR(TRID_RGB_INPUT_MES),
		MSYSDLG_INPUTTEXT_F_NOT_EMPTY, &str))
		return;

	ColorText_to_RGB8(&APPDRAW->col.drawcol.c8, str.buf);

	drawColor_changeDrawColor8();

	mStrFree(&str);

	//セット

	for(i = 0; i < 3; i++)
		ValueBar_setPos(pd->bar[i], APPDRAW->col.drawcol.c8.ar[i]);

	_send_notify(p, CHANGECOL_RGB);
}

/* イベント */

static int _rgb_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_pagedat_bar3 *pd = (_pagedat_bar3 *)pagedat;
		int i;

		switch(ev->notify.id)
		{
			//数値入力
			case WID_RGB_INPUT:
				_rgb_input(p, pd);
				break;

			//ValueBar
			default:
				if(ev->notify.param2 & VALUEBAR_ACT_F_CHANGE_POS)
				{
					for(i = 0; i < 3; i++)
						APPDRAW->col.drawcol.c8.ar[i] = ValueBar_getPos(pd->bar[i]);

					drawColor_changeDrawColor8();

					_send_notify(p, CHANGECOL_RGB);
				}
				break;
		}
	}

	return 1;
}

/* データ値セット */

static mlkbool _rgb_setdata(mPager *p,void *pagedat,void *src)
{
	_pagedat_bar3 *pd = (_pagedat_bar3 *)pagedat;
	int i;

	for(i = 0; i < 3; i++)
		ValueBar_setPos(pd->bar[i], APPDRAW->col.drawcol.c8.ar[i]);

	return TRUE;
}

/** RGB ページ作成 */

mlkbool PanelColor_createPage_RGB(mPager *p,mPagerInfo *info)
{
	_pagedat_bar3 *pd;
	mWidget *ct;
	const uint8_t state[] = {_LABEL_NO_R, _LABEL_NO_G, _LABEL_NO_B};

	pd = (_pagedat_bar3 *)mMalloc0(sizeof(_pagedat_bar3));

	info->pagedat = pd;
	info->setdata = _rgb_setdata;
	info->event = _rgb_event;

	//------------

	mContainerSetType_vert(MLK_CONTAINER(p), 0);

	//バー

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 5, 5, MLF_EXPAND_W, 0);

	_create_bar3(pd->bar, ct, state);

	//ボタン

	mButtonCreate(MLK_WIDGET(p), WID_RGB_INPUT, MLF_RIGHT, MLK_MAKE32_4(0,5,0,0),
		0, MLK_TR2(TRGROUP_PANEL_COLOR, TRID_RGB_INPUT));

	return TRUE;
}


/***************************************
 * HSV
 ***************************************/


/* イベント */

static int _hsv_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY
		&& (ev->notify.param2 & VALUEBAR_ACT_F_CHANGE_POS))
	{
		_pagedat_bar3 *pd = (_pagedat_bar3 *)pagedat;
		mHSVd hsv;
		int i,c[3];

		for(i = 0; i < 3; i++)
			c[i] = ValueBar_getPos(pd->bar[i]);
	
		hsv.h = c[0] / 60.0;
		hsv.s = c[1] / 255.0;
		hsv.v = c[2] / 255.0;

		HSVd_to_RGB8(&APPDRAW->col.drawcol.c8, &hsv);

		drawColor_changeDrawColor8();

		_send_notify(p, CHANGECOL_MAKE(CHANGECOL_TYPE_HSV, c[0], c[1], c[2]));
	}

	return 1;
}

/* データ値セット */

static mlkbool _hsv_setdata(mPager *p,void *pagedat,void *src)
{
	_setbar_hsv(((_pagedat_bar3 *)pagedat)->bar, *((uint32_t *)src));

	return TRUE;
}

/** HSV ページ作成 */

mlkbool PanelColor_createPage_HSV(mPager *p,mPagerInfo *info)
{
	_pagedat_bar3 *pd;
	const uint8_t state[] = {_LABEL_NO_H|0x80, _LABEL_NO_S, _LABEL_NO_V};

	pd = (_pagedat_bar3 *)mMalloc0(sizeof(_pagedat_bar3));

	info->pagedat = pd;
	info->setdata = _hsv_setdata;
	info->event = _hsv_event;

	//

	mContainerSetType_grid(MLK_CONTAINER(p), 2, 5, 5);

	_create_bar3(pd->bar, MLK_WIDGET(p), state);

	return TRUE;
}


/***************************************
 * HSL
 ***************************************/


/* イベント */

static int _hsl_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY
		&& (ev->notify.param2 & VALUEBAR_ACT_F_CHANGE_POS))
	{
		_pagedat_bar3 *pd = (_pagedat_bar3 *)pagedat;
		mHSLd hls;
		int i,c[3];

		for(i = 0; i < 3; i++)
			c[i] = ValueBar_getPos(pd->bar[i]);
	
		hls.h = c[0] / 60.0;
		hls.s = c[1] / 255.0;
		hls.l = c[2] / 255.0;

		drawColor_setDrawColor(mHSL_to_RGB8pac(hls.h, hls.s, hls.l));

		_send_notify(p, CHANGECOL_MAKE(CHANGECOL_TYPE_HSL, c[0], c[1], c[2]));
	}

	return 1;
}

/* データ値セット */

static mlkbool _hsl_setdata(mPager *p,void *pagedat,void *src)
{
	_pagedat_bar3 *pd = (_pagedat_bar3 *)pagedat;
	int i,c[3];

	ChangeColor_to_HSL_int(c, *((uint32_t *)src));

	for(i = 0; i < 3; i++)
		ValueBar_setPos(pd->bar[i], c[i]);

	return TRUE;
}

/** HSL ページ作成 */

mlkbool PanelColor_createPage_HSL(mPager *p,mPagerInfo *info)
{
	_pagedat_bar3 *pd;
	const uint8_t state[] = {_LABEL_NO_H|0x80, _LABEL_NO_S, _LABEL_NO_L};

	pd = (_pagedat_bar3 *)mMalloc0(sizeof(_pagedat_bar3));

	info->pagedat = pd;
	info->setdata = _hsl_setdata;
	info->event = _hsl_event;

	//

	mContainerSetType_grid(MLK_CONTAINER(p), 2, 5, 5);

	_create_bar3(pd->bar, MLK_WIDGET(p), state);

	return TRUE;
}


/***************************************
 * HSV カラーマップ
 ***************************************/


typedef struct
{
	mHSVPicker *pic;
	ValueBar *bar[3];
}_pagedat_hsvmap;

#define WID_HSVMAP_PICKER  100


/* HSV ピッカー色変更時 */

static void _hsvmap_change_picker(mPager *p,_pagedat_hsvmap *pd)
{
	double d[3];
	int i,c[3];

	mHSVPickerGetHSVColor(pd->pic, d);

	c[0] = (int)(d[0] * 60 + 0.5);
	c[1] = (int)(d[1] * 255 + 0.5);
	c[2] = (int)(d[2] * 255 + 0.5);

	for(i = 0; i < 3; i++)
		ValueBar_setPos(pd->bar[i], c[i]);

	//

	drawColor_setDrawColor(mHSVPickerGetRGBColor(pd->pic));

	_send_notify(p, CHANGECOL_MAKE(CHANGECOL_TYPE_HSV, c[0], c[1], c[2]));
}

/* バーの値変更時 */

static void _hsvmap_change_bar(mPager *p,_pagedat_hsvmap *pd)
{
	int i,c[3];
	mHSVd hsv;

	for(i = 0; i < 3; i++)
		c[i] = ValueBar_getPos(pd->bar[i]);

	hsv.h = c[0] / 60.0;
	hsv.s = c[1] / 255.0;
	hsv.v = c[2] / 255.0;

	mHSVPickerSetHSVColor(pd->pic, hsv.h, hsv.s, hsv.v);

	//

	drawColor_setDrawColor(mHSVPickerGetRGBColor(pd->pic));

	_send_notify(p, CHANGECOL_MAKE(CHANGECOL_TYPE_HSV, c[0], c[1], c[2]));
}

/* イベント */

static int _hsvmap_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_pagedat_hsvmap *pd = (_pagedat_hsvmap *)pagedat;

		switch(ev->notify.id)
		{
			//HSV ピッカー
			case WID_HSVMAP_PICKER:
				if(ev->notify.notify_type == MHSVPICKER_N_CHANGE_HUE
					|| ev->notify.notify_type == MHSVPICKER_N_CHANGE_SV)
				{
					_hsvmap_change_picker(p, pd);
				}
				break;
			//ValueBar
			default:
				if(ev->notify.param2 & VALUEBAR_ACT_F_CHANGE_POS)
					_hsvmap_change_bar(p, pd);
				break;
		}
	}

	return 1;
}

/* データ値セット */

static mlkbool _hsvmap_setdata(mPager *p,void *pagedat,void *src)
{
	_pagedat_hsvmap *pd = (_pagedat_hsvmap *)pagedat;
	uint32_t col;
	mHSVd hsv;

	col = *((uint32_t *)src);

	//HSV バー

	_setbar_hsv(pd->bar, col);

	//ピッカー

	ChangeColor_to_HSV(&hsv, col);

	mHSVPickerSetHSVColor(pd->pic, hsv.h, hsv.s, hsv.v);

	return TRUE;
}

/** HSV マップページ作成 */

mlkbool PanelColor_createPage_HSVMap(mPager *p,mPagerInfo *info)
{
	_pagedat_hsvmap *pd;
	mWidget *ct;
	const uint8_t state[] = {_LABEL_NO_H|0x80, _LABEL_NO_S, _LABEL_NO_V};

	pd = (_pagedat_hsvmap *)mMalloc0(sizeof(_pagedat_hsvmap));

	info->pagedat = pd;
	info->setdata = _hsvmap_setdata;
	info->event = _hsvmap_event;

	//--------

	mContainerSetType_horz(MLK_CONTAINER(p), 10);

	//バー

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 3, 7, MLF_EXPAND_W, 0);

	_create_bar3(pd->bar, ct, state);

	//HSV ピッカー

	pd->pic = mHSVPickerCreate(MLK_WIDGET(p), WID_HSVMAP_PICKER, 0, 0, 90);

	return TRUE;
}

