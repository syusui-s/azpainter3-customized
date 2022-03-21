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

/*************************************
 * フィルタ用、描画色置換のウィジェット
 *************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_checkbutton.h"
#include "mlk_colorprev.h"
#include "mlk_event.h"
#include "mlk_color.h"

#include "filterbar.h"
#include "colorvalue.h"


/*
 * 通知: type
 *  0 = バーのボタン離し時
 *  1 = エディット内容変更時
 */

//----------------

typedef struct
{
	MLK_CONTAINER_DEF

	int coltype,	//カラータイプ (0:RGB, 1:HSV, 2:HSL)
		rgb[3];		//RGB値 (8bit)

	mColorPrev *prev[2];
	mLabel *label[3];
	FilterBar *bar[3];
	mLineEdit *edit[3];
}FilterWgRepcol;

enum
{
	WID_CK_RGB = 100,
	WID_CK_HSV,
	WID_CK_HSL,
	
	WID_BAR_TOP = 110,
	WID_EDIT_TOP = 120
};

enum
{
	COLTYPE_RGB,
	COLTYPE_HSV,
	COLTYPE_HSL
};

static const char *g_coltype_name[3] = {"RGB", "HSV", "HSL"};
static const char *g_colname[3][3] = {
	{"R","G","B"}, {"H","S","V"}, {"H","S","L"}
};

//----------------


/* 通知 */

static void _notify(FilterWgRepcol *p,int notify_type)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p), MLK_WIDGET(p->wg.toplevel), 
		notify_type, 0, 0);
}

/* カラータイプ変更 */

static void _change_coltype(FilterWgRepcol *p)
{
	int i,type,max,val[3];
	mHSVd hsv;
	mHSLd hsl;

	type = p->coltype;

	//RGB から各カラーへ

	switch(type)
	{
		//RGB
		case COLTYPE_RGB:
			val[0] = p->rgb[0];
			val[1] = p->rgb[1];
			val[2] = p->rgb[2];
			break;
		//HSV
		case COLTYPE_HSV:
			mRGB8_to_HSV(&hsv, p->rgb[0], p->rgb[1], p->rgb[2]);

			val[0] = (int)(hsv.h * 60 + 0.5);
			val[1] = (int)(hsv.s * 255 + 0.5);
			val[2] = (int)(hsv.v * 255 + 0.5);
			break;
		//HSL
		case COLTYPE_HSL:
			mRGB8_to_HSL(&hsl, p->rgb[0], p->rgb[1], p->rgb[2]);

			val[0] = (int)(hsl.h * 60 + 0.5);
			val[1] = (int)(hsl.s * 255 + 0.5);
			val[2] = (int)(hsl.l * 255 + 0.5);
			break;
	}

	//値をセット

	for(i = 0; i < 3; i++)
	{
		max = (type != COLTYPE_RGB && i == 0)? 359: 255;
	
		//ラベル

		mLabelSetText(p->label[i], g_colname[type][i]);

		//バー

		FilterBar_setStatus(p->bar[i], 0, max, val[i]);

		//エディット

		mLineEditSetNumStatus(p->edit[i], 0, max, 0);
		mLineEditSetNum(p->edit[i], val[i]);
	}
}

/* 色値変更時 */

static void _change_val(FilterWgRepcol *p)
{
	int i,val[3];
	mRGB8 rgb;

	//値取得

	for(i = 0; i < 3; i++)
		val[i] = FilterBar_getPos(p->bar[i]);

	//RGB 取得

	switch(p->coltype)
	{
		//RGB
		case COLTYPE_RGB:
			rgb.r = val[0];
			rgb.g = val[1];
			rgb.b = val[2];
			break;
		//HSV
		case COLTYPE_HSV:
			mHSV_to_RGB8(&rgb, val[0] / 60.0, val[1] / 255.0, val[2] / 255.0);
			break;
		//HSL
		case COLTYPE_HSL:
			mHSL_to_RGB8(&rgb, val[0] / 60.0, val[1] / 255.0, val[2] / 255.0);
			break;
	}

	p->rgb[0] = rgb.r;
	p->rgb[1] = rgb.g;
	p->rgb[2] = rgb.b;

	//色プレビュー

	mColorPrevSetColor(p->prev[1], MLK_RGB(rgb.r, rgb.g, rgb.b));
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		FilterWgRepcol *p = (FilterWgRepcol *)wg;
		int id = ev->notify.id;

		if(id >= WID_CK_RGB && id <= WID_CK_HSL)
		{
			//カラータイプ変更

			p->coltype = id - WID_CK_RGB;

			_change_coltype(p);

			mWidgetReLayout(wg);
		}
		else if(id >= WID_BAR_TOP && id < WID_BAR_TOP + 3)
		{
			//FilterBar
			// :ボタン離し時は、通知を送ってプレビューさせる

			mLineEditSetNum(p->edit[id - WID_BAR_TOP], ev->notify.param1);

			_change_val(p);

			if(ev->notify.param2)
				_notify(p, 0);
		}
		else if(id >= WID_EDIT_TOP && id < WID_EDIT_TOP + 3)
		{
			//エディット

			id -= WID_EDIT_TOP;

			FilterBar_setPos(p->bar[id], mLineEditGetNum(p->edit[id]));

			_change_val(p);
			_notify(p, 1);
		}
	}

	return 1;
}

/** 作成 */

mWidget *FilterWgRepcol_new(mWidget *parent,int id,RGB8 *drawcol)
{
	FilterWgRepcol *p;
	mWidget *ct,*ct2;
	int i;
	uint32_t col;

	//作成

	p = (FilterWgRepcol *)mContainerNew(parent, sizeof(FilterWgRepcol));
	if(!p) return NULL;

	mContainerSetType_horz(MLK_CONTAINER(p), 12);

	p->wg.id = id;
	p->wg.flayout = MLF_EXPAND_W;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;
	p->wg.event = _event_handle;

	p->coltype = COLTYPE_HSL;

	p->rgb[0] = drawcol->r;
	p->rgb[1] = drawcol->g;
	p->rgb[2] = drawcol->b;

	//色プレビュー x 2

	ct = mContainerCreateVert(MLK_WIDGET(p), 10, 0, 0);

	col = MLK_RGB(drawcol->r, drawcol->g, drawcol->b);

	for(i = 0; i < 2; i++)
		p->prev[i] = mColorPrevCreate(ct, 50, 40, 0, MCOLORPREV_S_FRAME, col);

	//------- 色項目

	ct = mContainerCreateVert(MLK_WIDGET(p), 15, MLF_EXPAND_W, 0);

	//タイプ

	ct2 = mContainerCreateHorz(ct, 4, 0, 0);

	for(i = 0; i < 3; i++)
	{
		mCheckButtonCreate(ct2, WID_CK_RGB + i, 0, 0, MCHECKBUTTON_S_RADIO,
			g_coltype_name[i], (i == p->coltype));
	}

	//name + bar + edit

	ct2 = mContainerCreateGrid(ct, 3, 5, 7, MLF_EXPAND_W, 0);

	for(i = 0; i < 3; i++)
	{
		//label
		
		p->label[i] = mLabelCreate(ct2, MLF_MIDDLE, 0, 0, NULL);

		//bar

		p->bar[i] = FilterBar_new(ct2, WID_BAR_TOP + i, MLF_MIDDLE | MLF_EXPAND_W, 240,
			0, 255, 0, 0);

		//edit

		p->edit[i] = mLineEditCreate(ct2, WID_EDIT_TOP + i,
			MLF_MIDDLE, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

		mLineEditSetWidth_textlen(p->edit[i], 5);
	}

	_change_coltype(p);

	return (mWidget *)p;
}

/** 置換色 RGB 取得 (8bit) */

void FilterWgRepcol_getColor(mWidget *wg,int *dst)
{
	FilterWgRepcol *p = (FilterWgRepcol *)wg;
	int i;

	for(i = 0; i < 3; i++)
		dst[i] = p->rgb[i];
}
