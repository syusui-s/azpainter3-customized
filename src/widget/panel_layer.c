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
 * [panel] レイヤ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_panel.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_str.h"
#include "mlk_list.h"

#include "def_macro.h"
#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_draw_sub.h"

#include "panel.h"
#include "mainwindow.h"
#include "appresource.h"

#include "draw_layer.h"

#include "trid.h"
#include "trid_mainmenu.h"

#include "pv_panel_layer.h"


//------------------------

typedef struct
{
	MLK_CONTAINER_DEF

	PanelLayerList *list;
	mWidget *wg_param;
	mIconBar *iconbar;
}_topct;

//------------------------

//ボタンID
static const uint16_t g_iconbar_bttid[] = {
	TRMENU_LAYER_NEW, TRMENU_LAYER_COPY, TRMENU_LAYER_ERASE,
	TRMENU_LAYER_DELETE, TRMENU_LAYER_COMBINE, TRMENU_LAYER_DROP,
	TRMENU_LAYER_TB_MOVE_UP, TRMENU_LAYER_TB_MOVE_DOWN
};

//------------------------


/* テンプレートからレイヤ作成 */

static void _newlayer_template(int index)
{
	ConfigLayerTemplItem *ti;
	LayerNewOptInfo info;

	ti = (ConfigLayerTemplItem *)mListGetItemAtIndex(&APPCONF->list_layertempl, index);
	if(!ti) return;

	mMemset0(&info, sizeof(LayerNewOptInfo));

	if(ti->name_len)
		mStrSetText(&info.strname, ti->text);

	if(ti->texture_len)
		mStrSetText(&info.strtexpath, ti->text + ti->name_len + 1);

	info.type = (ti->type == 0xff)? -1: ti->type;
	info.blendmode = ti->blendmode;
	info.opacity = ti->opacity;
	info.col = ti->col;
	info.ftone = ((ti->flags & CONFIG_LAYERTEMPL_F_TONE) != 0);
	info.ftone_white = ((ti->flags & CONFIG_LAYERTEMPL_F_TONE_WHITE) != 0);
	info.tone_lines = ti->tone_lines;
	info.tone_angle = ti->tone_angle;
	info.tone_density = ti->tone_density;

	drawLayer_newLayer(APPDRAW, &info, NULL);

	mStrFree(&info.strname);
	mStrFree(&info.strtexpath);
}

/* ツールバー [新規作成]ボタンのドロップダウンメニュー */

static void _runmenu_newlayer(_topct *p)
{
	mMenu *menu,*sub;
	mMenuItem *mi;
	ConfigLayerTemplItem *ti;
	int i;
	mBox box;
	const uint16_t trid[] = {
		TRID_LAYERTYPE_TEXT_A, TRID_LAYERTYPE_TEXT_A1,
		TRID_LAYERTYPE_TONE_GRAY, TRID_LAYERTYPE_TONE_A1
	};

	//--------

	menu = mMenuNew();

	MLK_TRGROUP(TRGROUP_LAYER_TYPE);

	//フォルダ
	mMenuAppendText(menu, TRID_LAYERTYPE_FOLDER, MLK_TR(TRID_LAYERTYPE_FOLDER));
	mMenuAppendSep(menu);
	//通常レイヤ
	mMenuAppendTrText(menu, 0, LAYERTYPE_NORMAL_NUM);
	mMenuAppendSep(menu);

	//トーン/テキスト

	for(i = 0; i < 4; i++)
		mMenuAppendText(menu, trid[i], MLK_TR(trid[i]));

	//テンプレート
	
	mMenuAppendSep(menu);

	sub = mMenuNew();
	
	for(ti = (ConfigLayerTemplItem *)APPCONF->list_layertempl.top, i = 0; ti; ti = (ConfigLayerTemplItem *)ti->i.next, i++)
	{
		mMenuAppendText(sub, 1000 + i, ti->text);
	}

	mMenuAppendSubmenu(menu, -1, MLK_TR2(TRGROUP_WORD, TRID_WORD_TEMPLATE), sub, 0);

	//

	mIconBarGetItemBox(p->iconbar, TRMENU_LAYER_NEW, &box); 

	mi = mMenuPopup(menu, MLK_WIDGET(p->iconbar), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//--------

	if(i == -1) return;

	//テンプレート

	if(i >= 1000)
	{
		_newlayer_template(i - 1000);
		return;
	}

	//レイヤタイプ

	switch(i)
	{
		//フォルダ
		case TRID_LAYERTYPE_FOLDER:
			drawLayer_newLayer_direct(APPDRAW, -1);
			break;
		//トーン
		case TRID_LAYERTYPE_TONE_GRAY:
		case TRID_LAYERTYPE_TONE_A1:
			MainWindow_layer_new_tone(APPWIDGET->mainwin, (i == TRID_LAYERTYPE_TONE_A1));
			break;
		//テキスト(A)
		case TRID_LAYERTYPE_TEXT_A:
			drawLayer_newLayer_direct(APPDRAW, 0x82);
			break;
		//テキスト(A1)
		case TRID_LAYERTYPE_TEXT_A1:
			drawLayer_newLayer_direct(APPDRAW, 0x83);
			break;

		//通常レイヤ
		default:
			drawLayer_newLayer_direct(APPDRAW, i);
			break;
	}
}

/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_COMMAND)
	{
		if(ev->cmd.id == TRMENU_LAYER_NEW && ev->cmd.from == MEVENT_COMMAND_FROM_ICONBAR_DROP)
			//新規作成のドロップダウンメニュー
			_runmenu_newlayer((_topct *)wg);
		else
			//メインウィンドウへ送る
			(MLK_WIDGET(APPWIDGET->mainwin)->event)(MLK_WIDGET(APPWIDGET->mainwin), ev);
	}

	return 1;
}

/* mPanel 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	_topct *p;
	mIconBar *ib;
	int i;

	//トップコンテナ

	p = (_topct *)mContainerNew(parent, sizeof(_topct));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	//上部のパラメータ部分
	// :起動中に閉じる->表示した場合のために、パラメータセット。
	// :起動時はカレントレイヤが NULL のため、その場合はセットしない。

	p->wg_param = PanelLayer_param_create(MLK_WIDGET(p));

	PanelLayer_param_setValue(p->wg_param);

	//リスト (レイヤ名用に 12px フォントセット)

	p->list = PanelLayerList_new(MLK_WIDGET(p));

	MLK_WIDGET(p->list)->font = APPWIDGET->font_panel_fix;

	//ツールバー

	p->iconbar = ib = mIconBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0, MICONBAR_S_TOOLTIP);

	mIconBarSetPadding(ib, 1);
	mIconBarSetImageList(ib, APPRES->imglist_panellayer_cmd);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_PANEL_LAYER);

	for(i = 0; i < 8; i++)
	{
		mIconBarAdd(ib, g_iconbar_bttid[i], i, TRID_LAYER_CMD_TOOLTIP_TOP + i,
			(i == 0)? MICONBAR_ITEMF_DROPDOWN: 0);
	}

	return MLK_WIDGET(p);
}

/** 初期作成 */

void PanelLayer_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_LAYER, 0, MPANEL_S_EXPAND_HEIGHT, 0,
		state, _panel_create_handle);

	mPanelCreateWidget(p);
}


//===============================
// 外部からの呼び出し
//===============================


/* _topct 取得 */

static _topct *_get_topct(void)
{
	return (_topct *)Panel_getContents(PANEL_LAYER);
}


/** すべて更新 */

void PanelLayer_update_all(void)
{
	_topct *p = _get_topct();

	if(p)
	{
		PanelLayer_param_setValue(p->wg_param);

		PanelLayerList_setScrollInfo(p->list);

		mWidgetRedraw(MLK_WIDGET(p->list));
	}
}

/** カレントレイヤの上部パラメータ更新 */

void PanelLayer_update_curparam(void)
{
	_topct *p = _get_topct();

	if(p)
		PanelLayer_param_setValue(p->wg_param);
}

/** カレントレイヤを更新
 *
 * setparam: パラメータの値もセットする */

void PanelLayer_update_curlayer(mlkbool setparam)
{
	_topct *p = _get_topct();

	if(p)
	{
		if(setparam)
			PanelLayer_param_setValue(p->wg_param);

		PanelLayerList_drawLayer(p->list, APPDRAW->curlayer);
	}
}

/** 指定レイヤのみ更新 (一覧部分のみ)
 *
 * item: NULL で何もしない */

void PanelLayer_update_layer(LayerItem *item)
{
	if(item && Panel_isVisible(PANEL_LAYER))
	{
		_topct *p = _get_topct();

		PanelLayerList_drawLayer(p->list, item);
	}
}

/** 指定レイヤのみ更新
 *
 * 指定レイヤがカレントの場合は、上部パラメータも更新 */

void PanelLayer_update_layer_curparam(LayerItem *item)
{
	if(item)
	{
		_topct *p = _get_topct();

		if(Panel_isVisible(PANEL_LAYER))
			PanelLayerList_drawLayer(p->list, item);

		//パラメータ

		if(p && item == APPDRAW->curlayer)
			PanelLayer_param_setValue(p->wg_param);
	}
}

/** レイヤ一覧上の操作以外で、任意のレイヤをカレントにした時
 *
 * カレントの更新 + スクロール。
 *
 * lastitem: 変更前のカレントレイヤ
 * update_all: フォルダの展開などにより、リスト全体を更新 */

void PanelLayer_update_changecurrent_visible(LayerItem *lastitem,mlkbool update_all)
{
	_topct *p = _get_topct();

	if(p)
	{
		PanelLayer_param_setValue(p->wg_param);

		if(update_all)
			PanelLayerList_setScrollInfo(p->list);

		PanelLayerList_changeCurrent_visible(p->list, lastitem, update_all);
	}
}

