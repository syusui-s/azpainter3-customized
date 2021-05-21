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

/*********************************************
 * [panel] レイヤで使うウィジェット
 *********************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_combobox.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_str.h"

#include "def_draw.h"

#include "layeritem.h"
#include "valuebar.h"
#include "sel_material.h"
#include "appresource.h"
#include "blendcolor.h"
#include "panel_func.h"

#include "draw_main.h"
#include "draw_layer.h"

#include "trid.h"

#include "pv_panel_layer.h"



//**************************************
// パラメータ部分
//**************************************


typedef struct
{
	MLK_CONTAINER_DEF

	mComboBox *cb_blend;
	ValueBar *bar_opacity;
	mWidget *wg_tex;
	mIconBar *iconbar;
}_paramct;

enum
{
	WID_CB_BLENDMODE = 100,
	WID_BAR_OPACITY,
	WID_TEXTURE
};

enum
{
	CMDID_CHECK_FILLREF,
	CMDID_CHECK_LOCK,
	CMDID_CHECK_CHECKED,
	CMDID_CHECK_TONE_GRAY,

	CMDID_ALPHAMASK_OFF = 100,
	CMDID_ALPHAMASK_ALPHA,
	CMDID_ALPHAMASK_TP,
	CMDID_ALPHAMASK_NOTTP
};


/* テクスチャ画像選択 */

static void _select_texture(_paramct *p)
{
	mStr str = MSTR_INIT;

	mStrSetText(&str, APPDRAW->curlayer->texture_path);

	if(SelMaterialDlg_run(MLK_WINDOW(p->wg.toplevel), &str, TRUE))
	{
		drawLayer_setTexture(APPDRAW, APPDRAW->curlayer, str.buf);

		SelMaterialTex_setFlag(p->wg_tex, 1);
	}

	mStrFree(&str);
}

/* コマンド */

static void _param_event_command(mEventCommand *ev)
{
	LayerItem *pi = APPDRAW->curlayer;

	switch(ev->id)
	{
		//アルファマスク
		case CMDID_ALPHAMASK_OFF:
		case CMDID_ALPHAMASK_ALPHA:
		case CMDID_ALPHAMASK_TP:
		case CMDID_ALPHAMASK_NOTTP:
			pi->alphamask = ev->id - CMDID_ALPHAMASK_OFF;
			PanelLayer_update_layer(pi);
			break;
	
		//塗りつぶし参照元
		case CMDID_CHECK_FILLREF:
			pi->flags ^= LAYERITEM_F_FILLREF;
			PanelLayer_update_layer(pi);
			break;
		//ロック
		case CMDID_CHECK_LOCK:
			drawLayer_toggle_lock(APPDRAW, pi);
			PanelLayer_update_layer(pi);
			break;
		//チェック
		case CMDID_CHECK_CHECKED:
			pi->flags ^= LAYERITEM_F_CHECKED;
			PanelLayer_update_layer(pi);
			break;
		//トーンレイヤをグレイスケール表示
		case CMDID_CHECK_TONE_GRAY:
			drawLayer_toggle_tone_to_gray(APPDRAW);
			break;
	}
}

/* 通知イベント */

static void _param_event_notify(_paramct *p,mEventNotify *ev)
{
	switch(ev->id)
	{
		//不透明度
		case WID_BAR_OPACITY:
			if(ev->param2 & VALUEBAR_ACT_F_ONCE)
			{
				APPDRAW->curlayer->opacity = (int)(ev->param1 * 128.0 / 100.0 + 0.5);
			
				drawUpdateRect_canvas_canvasview_inLayerHave(APPDRAW, NULL);
				PanelLayer_update_curlayer(FALSE);
			}
			break;
		//合成モード
		case WID_CB_BLENDMODE:
			if(ev->notify_type == MCOMBOBOX_N_CHANGE_SEL)
			{
				APPDRAW->curlayer->blendmode = ev->param2;

				drawUpdateRect_canvas_canvasview_inLayerHave(APPDRAW, NULL);
				PanelLayer_update_curlayer(FALSE);
			}
			break;
		//テクスチャ
		case WID_TEXTURE:
			if(ev->notify_type == 0)
			{
				//なし
				drawLayer_setTexture(APPDRAW, APPDRAW->curlayer, NULL);
				SelMaterialTex_setFlag(p->wg_tex, 0);
			}
			else
				//選択
				_select_texture(p);
			break;
	}
}

/* イベントハンドラ */

static int _param_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_param_event_notify((_paramct *)wg, (mEventNotify *)ev);
			break;
		case MEVENT_COMMAND:
			_param_event_command((mEventCommand *)ev);
			break;
	}

	return 1;
}

/** 作成 */

mWidget *PanelLayer_param_create(mWidget *parent)
{
	_paramct *p;
	mWidget *ct;
	mIconBar *ib;
	int i;

	//作成

	p = (_paramct *)mContainerNew(parent, sizeof(_paramct));
	if(!p) return NULL;

	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,5,3,3));

	p->wg.flayout = MLF_EXPAND_W;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;
	p->wg.event = _param_event_handle;
	p->ct.sep = 5;

	//---- 1段目

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, MLF_EXPAND_W, 0);

	//合成モード
	
	p->cb_blend = mComboBoxCreate(ct, WID_CB_BLENDMODE, MLF_EXPAND_W, 0, 0);

	MLK_TRGROUP(TRGROUP_BLENDMODE);
	mComboBoxAddItems_tr(p->cb_blend, 0, BLENDMODE_NUM, 0);

	//テクスチャ

	p->wg_tex = SelMaterialTex_new(ct, WID_TEXTURE);

	//---- 2段目 (不透明度)

	p->bar_opacity = ValueBar_new(MLK_WIDGET(p), WID_BAR_OPACITY, MLF_EXPAND_W,
		0, 0, 100, 100);

	//---- 3段目 (mIconBar)

	p->iconbar = ib = mIconBarCreate(MLK_WIDGET(p), 0, 0, 0, MICONBAR_S_TOOLTIP);

	mIconBarSetImageList(ib, APPRES->imglist_panellayer_check);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_PANEL_LAYER);

	//アルファマスク

	for(i = 0; i < 4; i++)
		mIconBarAdd(ib, CMDID_ALPHAMASK_OFF + i, i, TRID_LAYER_ALPHAMASK_OFF + i, MICONBAR_ITEMF_CHECKGROUP);

	//フラグ

	mIconBarAddSep(ib);

	mIconBarAdd(ib, CMDID_CHECK_FILLREF, 4, TRID_LAYER_FILLREF, MICONBAR_ITEMF_CHECKBUTTON);
	mIconBarAdd(ib, CMDID_CHECK_LOCK, 5, TRID_LAYER_LOCK, MICONBAR_ITEMF_CHECKBUTTON);
	mIconBarAdd(ib, CMDID_CHECK_CHECKED, 6, TRID_LAYER_CHECK, MICONBAR_ITEMF_CHECKBUTTON);

	mIconBarAddSep(ib);
	mIconBarAdd(ib, CMDID_CHECK_TONE_GRAY, 7, TRID_LAYER_TONE_GRAY, MICONBAR_ITEMF_CHECKBUTTON);

	return MLK_WIDGET(p);
}

/** カレントレイヤの値をセット */

void PanelLayer_param_setValue(mWidget *wg)
{
	_paramct *p = (_paramct *)wg;
	LayerItem *pi = APPDRAW->curlayer;
	mIconBar *ib = p->iconbar;
	int fimg,i;

	//起動時の初期作成時は除く
	if(!pi) return;

	//合成モード

	mComboBoxSetSelItem_atIndex(p->cb_blend, pi->blendmode);

	//テクスチャ

	SelMaterialTex_setFlag(p->wg_tex, (pi->texture_path && *(pi->texture_path)) );

	//不透明度

	ValueBar_setPos(p->bar_opacity, (int)(pi->opacity * 100 / 128.0 + 0.5));

	//mIconBar

	mIconBarSetCheck(ib, CMDID_ALPHAMASK_OFF + pi->alphamask, 1);

	mIconBarSetCheck(ib, CMDID_CHECK_FILLREF, LAYERITEM_IS_FILLREF(pi));
	mIconBarSetCheck(ib, CMDID_CHECK_LOCK, LAYERITEM_IS_LOCK(pi));
	mIconBarSetCheck(ib, CMDID_CHECK_CHECKED, LAYERITEM_IS_CHECKED(pi));

	mIconBarSetCheck(ib, CMDID_CHECK_TONE_GRAY, APPDRAW->ftonelayer_to_gray);

	//有効/無効

	fimg = LAYERITEM_IS_IMAGE(pi);

	mWidgetEnable(MLK_WIDGET(p->cb_blend), fimg);
	mWidgetEnable(MLK_WIDGET(p->wg_tex), fimg);

	for(i = 0; i < 4; i++)
		mIconBarSetEnable(ib, CMDID_ALPHAMASK_OFF + i, fimg);

	mIconBarSetEnable(ib, CMDID_CHECK_FILLREF, fimg);
	mIconBarSetEnable(ib, CMDID_CHECK_CHECKED, fimg);
}


