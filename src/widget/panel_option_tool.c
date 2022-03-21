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

/************************************************
 * PanelOption
 * ツールオプション
 ************************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_pager.h"
#include "mlk_containerview.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_combobox.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_columnitem.h"
#include "mlk_pixbuf.h"
#include "mlk_sysdlg.h"
#include "mlk_list.h"
#include "mlk_str.h"

#include "def_config.h"
#include "def_draw.h"
#include "def_tool_option.h"
#include "def_pixelmode.h"

#include "valuebar.h"
#include "prev_tileimg.h"
#include "mainwindow.h"
#include "filedialog.h"
#include "widget_func.h"

#include "gradation_list.h"
#include "appresource.h"
#include "apphelp.h"

#include "draw_main.h"

#include "trid.h"
#include "trid_tooloption.h"


//-----------------------

//(塗りタイプ) ツール用、消しゴムまで
static const uint8_t g_pixmode_tool_erase[] = {
	PIXELMODE_TOOL_BLEND, PIXELMODE_TOOL_COMPARE_A,
	PIXELMODE_TOOL_OVERWRITE, PIXELMODE_TOOL_ERASE, 255
};

void GradationListEditDlg_run(mWindow *parent);
mlkbool GradationEditDlg_run(mWindow *parent,GradItem *item);

//-----------------------


/******************************
 * 共通関数
 ******************************/


/** メインコンテナ作成
 *
 * MLF_EXPAND_W、垂直コンテナ */

mWidget *PanelOption_createMainContainer(mWidget *parent,int size,mFuncWidgetHandle_event event)
{
	mWidget *p;

	p = mContainerNew(parent, size);

	p->flayout = MLF_EXPAND_W;
	p->notify_to = MWIDGET_NOTIFYTO_SELF;
	p->event = event;

	mContainerSetType_vert(MLK_CONTAINER(p), 2);
	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,0,5,0));

	return p;
}

/** 濃度のラベルとバーを作成 */

ValueBar *PanelOption_createDensityBar(mWidget *parent,int id,int val)
{
	mLabelCreate(parent, 0, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY));

	return ValueBar_new(parent, id, MLF_EXPAND_W, 0, 1, 100, val);
}

/** ラベルとバーを作成 (桁なし)
 *
 * param: label_id 正の値で現在の翻訳グループ。負の値で TRGROUP_WORD。 */

ValueBar *PanelOption_createBar(mWidget *parent,int id,int label_id,int min,int max,int val)
{
	mLabelCreate(parent, 0, 0, 0,
		(label_id < 0)? MLK_TR2(TRGROUP_WORD, -label_id): MLK_TR(label_id));

	return ValueBar_new(parent, id, MLF_EXPAND_W, 0, min, max, val);
}

/** 塗りタイプのラベルとコンボボックスを作成
 *
 * [!] 翻訳グループのカレントが変更されるので注意
 *
 * pixdat: PIXELMODE_* の配列。255 で終了。
 * sel: 初期選択値 (PIXELMODE_*)
 * is_brush: TRUE でブラシ用、FALSE でツール用 */

mComboBox *PanelOption_createPixelModeCombo(mWidget *parent,int id,const uint8_t *pixdat,int sel,mlkbool is_brush)
{
	mComboBox *cb;
	mColumnItem *pi;
	int n;

	mLabelCreate(parent, 0, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_PIXELMODE));

	//コンボボックス

	cb = mComboBoxCreate(parent, id, MLF_EXPAND_W, 0, 0);

	MLK_TRGROUP((is_brush)? TRGROUP_PIXELMODE_BRUSH: TRGROUP_PIXELMODE_TOOL);

	while(*pixdat != 255)
	{
		n = *(pixdat++);
	
		pi = mComboBoxAddItem_static(cb, MLK_TR(n), n);

		if(n == sel)
			mComboBoxSetSelItem(cb, pi);
	}

	if(!mComboBoxGetSelectItem(cb))
		mComboBoxSetSelItem_atIndex(cb, 0);

	return cb;
}

/** ラベルとコンボボックスを作成
 *
 * label_id: 正の値でカレントグループ。負の値で TRGROUP_WORD のグループ */

mComboBox *PanelOption_createComboBox(mWidget *parent,int id,int label_id)
{
	mLabelCreate(parent, 0, 0, 0,
		(label_id < 0)? MLK_TR2(TRGROUP_WORD, -label_id): MLK_TR(label_id));

	return mComboBoxCreate(parent, id, MLF_EXPAND_W, 0, 0);
}


/*****************************
 * ドットペン/消しゴム
 *****************************/
/*
 * mWidget::param1 = 1 なら消しゴム
 */


enum
{
	WID_DOTPEN_SIZE = 100,
	WID_DOTPEN_DENSITY,
	WID_DOTPEN_PIXMODE,
	WID_DOTPEN_SHAPE,
	WID_DOTPEN_SLIM
};

const uint8_t g_pixmode_dotpen[] = {
	PIXELMODE_BLEND_PIXEL, PIXELMODE_BLEND_STROKE, PIXELMODE_COMPARE_A,
	PIXELMODE_OVERWRITE_SHAPE, PIXELMODE_OVERWRITE_SQUARE, 255
};

const uint8_t g_pixmode_dotpen_erase[] = {
	PIXELMODE_BLEND_PIXEL, PIXELMODE_BLEND_STROKE, 255
};


/* イベント */

static int _dotpen_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint32_t *pd;

		pd = (wg->param1)? &APPDRAW->tool.opt_dotpen_erase: &APPDRAW->tool.opt_dotpen;
	
		switch(ev->notify.id)
		{
			//サイズ
			case WID_DOTPEN_SIZE:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_DOTPEN_SET_SIZE(pd, ev->notify.param1);
				break;
			//濃度
			case WID_DOTPEN_DENSITY:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_DOTPEN_SET_DENSITY(pd, ev->notify.param1);
				break;
			//塗り
			case WID_DOTPEN_PIXMODE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_DOTPEN_SET_PIXMODE(pd, ev->notify.param2);
				break;
			//形状
			case WID_DOTPEN_SHAPE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_DOTPEN_SET_SHAPE(pd, ev->notify.param2);
				break;
			//細線
			case WID_DOTPEN_SLIM:
				TOOLOPT_DOTPEN_TOGGLE_SLIM(pd);
				break;
		}
	}

	return 1;
}

/* 作成 */

static mWidget *_create_dotpen(mWidget *parent)
{
	mWidget *p;
	mComboBox *cb;
	uint32_t val;

	p = PanelOption_createMainContainer(parent, 0, _dotpen_event_handle);

	p->param1 = (APPDRAW->tool.no == TOOL_DOTPEN_ERASE);

	val = (p->param1)? APPDRAW->tool.opt_dotpen_erase: APPDRAW->tool.opt_dotpen;

	//-------

	//サイズ

	PanelOption_createBar(p, WID_DOTPEN_SIZE, -TRID_WORD_SIZE, 1, 101, TOOLOPT_DOTPEN_GET_SIZE(val));

	//濃度

	PanelOption_createDensityBar(p, WID_DOTPEN_DENSITY, TOOLOPT_DOTPEN_GET_DENSITY(val));

	//塗り

	PanelOption_createPixelModeCombo(p, WID_DOTPEN_PIXMODE,
		(p->param1)? g_pixmode_dotpen_erase: g_pixmode_dotpen, TOOLOPT_DOTPEN_GET_PIXMODE(val), TRUE);

	//形状

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	cb = PanelOption_createComboBox(p, WID_DOTPEN_SHAPE, TRID_TOOLOPT_SHAPE);

	mComboBoxAddItems_sepnull(cb, MLK_TR2(TRGROUP_WORD, TRID_WORD_DOTPEN_SHAPE), 0);
	mComboBoxSetSelItem_atIndex(cb, TOOLOPT_DOTPEN_GET_SHAPE(val));

	//細線

	mCheckButtonCreate(p, WID_DOTPEN_SLIM, 0, MLK_MAKE32_4(0,2,0,0), 0,
		MLK_TR(TRID_TOOLOPT_SLIM), TOOLOPT_DOTPEN_IS_SLIM(val));

	return p;
}


/*****************************
 * 指先
 *****************************/

enum
{
	WID_FINGER_SIZE = 100,
	WID_FINGER_DENSITY,
};


/* イベント */

static int _finger_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint32_t *pd = &APPDRAW->tool.opt_finger;
	
		switch(ev->notify.id)
		{
			//サイズ
			case WID_FINGER_SIZE:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_DOTPEN_SET_SIZE(pd, ev->notify.param1);
				break;
			//強さ
			case WID_FINGER_DENSITY:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_DOTPEN_SET_DENSITY(pd, ev->notify.param1);
				break;
		}
	}

	return 1;
}

/* 作成 */

static mWidget *_create_finger(mWidget *parent)
{
	mWidget *p;
	uint32_t val;

	p = PanelOption_createMainContainer(parent, 0, _finger_event_handle);

	val = APPDRAW->tool.opt_finger;

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	//サイズ

	PanelOption_createBar(p, WID_FINGER_SIZE, -TRID_WORD_SIZE, 1, 101, TOOLOPT_DOTPEN_GET_SIZE(val));

	//強さ

	PanelOption_createBar(p, WID_FINGER_DENSITY, TRID_TOOLOPT_STRONG,
		1, 100, TOOLOPT_DOTPEN_GET_DENSITY(val));

	return p;
}


/*****************************
 * 図形塗り/消し
 *****************************/
/*
 * mWidget::param1 : 1 なら図形消し
 */


enum
{
	WID_FILLPL_DENSITY = 100,
	WID_FILLPL_PIXMODE,
	WID_FILLPL_ANTIALIAS
};


/* イベント */

static int _fillpoly_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint16_t *pd;

		pd = (wg->param1)? &APPDRAW->tool.opt_fillpoly_erase: &APPDRAW->tool.opt_fillpoly;
	
		switch(ev->notify.id)
		{
			//濃度
			case WID_FILLPL_DENSITY:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_FILLPOLY_SET_DENSITY(pd, ev->notify.param1);
				break;
			//塗り
			case WID_FILLPL_PIXMODE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_FILLPOLY_SET_PIXMODE(pd, ev->notify.param2);
				break;
			//アンチエイリアス
			case WID_FILLPL_ANTIALIAS:
				TOOLOPT_FILLPOLY_TOGGLE_ANTIALIAS(pd);
				break;
		}
	}

	return 1;
}

/* 作成 */

static mWidget *_create_fill_polygon(mWidget *parent)
{
	mWidget *p;
	uint32_t val;
	const uint8_t pixmode[] = {
		PIXELMODE_TOOL_BLEND, PIXELMODE_TOOL_COMPARE_A,
		PIXELMODE_TOOL_OVERWRITE, 255
	};

	p = PanelOption_createMainContainer(parent, 0, _fillpoly_event_handle);

	p->param1 = (APPDRAW->tool.no == TOOL_FILL_POLYGON_ERASE);

	val = (p->param1)? APPDRAW->tool.opt_fillpoly_erase: APPDRAW->tool.opt_fillpoly;

	//-------

	//濃度

	PanelOption_createDensityBar(p, WID_FILLPL_DENSITY, TOOLOPT_FILLPOLY_GET_DENSITY(val));

	//塗り

	if(!p->param1)
	{
		PanelOption_createPixelModeCombo(p,
			WID_FILLPL_PIXMODE, pixmode, TOOLOPT_FILLPOLY_GET_PIXMODE(val), FALSE);
	}

	//アンチエイリアス

	mCheckButtonCreate(p, WID_FILLPL_ANTIALIAS, 0, MLK_MAKE32_4(0,2,0,0), 0,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_ANTIALIAS),
		TOOLOPT_FILLPOLY_IS_ANTIALIAS(val));

	return p;
}


/******************************
 * 塗りつぶし/自動選択
 ******************************/
/* mWidget::param1 = 1 で塗りつぶし、0 で自動選択 */

enum
{
	WID_FILL_TYPE = 100,
	WID_FILL_DIFF,
	WID_FILL_DENSITY,
	WID_FILL_LAYER,
	WID_FILL_PIXMODE
};


/* イベント */

static int _fill_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint32_t *pd;

		pd = (wg->param1)? &APPDRAW->tool.opt_fill: &APPDRAW->tool.opt_selfill;

		switch(ev->notify.id)
		{
			//塗りつぶす範囲
			case WID_FILL_TYPE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_FILL_SET_TYPE(pd, ev->notify.param2);
				break;
			//許容誤差
			case WID_FILL_DIFF:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_FILL_SET_DIFF(pd, ev->notify.param1);
				break;
			//濃度
			case WID_FILL_DENSITY:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_FILL_SET_DENSITY(pd, ev->notify.param1);
				break;
			//参照レイヤ
			case WID_FILL_LAYER:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_FILL_SET_LAYER(pd, ev->notify.param2);
				break;
			//塗り
			case WID_FILL_PIXMODE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_FILL_SET_PIXMODE(pd, ev->notify.param2);
				break;
		}
	}

	return 1;
}

/** "塗りつぶし" 作成 */

static mWidget *_create_fill(mWidget *parent)
{
	mWidget *p;
	mComboBox *cb;
	uint32_t val;
	mlkbool is_fill;

	is_fill = (APPDRAW->tool.no == TOOL_FILL);

	val = (is_fill)? APPDRAW->tool.opt_fill: APPDRAW->tool.opt_selfill;

	p = PanelOption_createMainContainer(parent, 0, _fill_event_handle);

	p->param1 = is_fill;

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	//---------

	//塗りつぶす範囲

	cb = PanelOption_createComboBox(p, WID_FILL_TYPE, TRID_TOOLOPT_FILL_TYPE);

	mComboBoxAddItems_sepnull(cb, MLK_TR(TRID_TOOLOPT_FILL_TYPE_LIST), 0);
	mComboBoxSetSelItem_atIndex(cb, TOOLOPT_FILL_GET_TYPE(val));

	//許容誤差

	PanelOption_createBar(p, WID_FILL_DIFF, TRID_TOOLOPT_FILL_DIFF, 0, 99, TOOLOPT_FILL_GET_DIFF(val));

	//濃度

	if(is_fill)
		PanelOption_createDensityBar(p, WID_FILL_DENSITY, TOOLOPT_FILL_GET_DENSITY(val));

	//色を参照するレイヤ

	cb = PanelOption_createComboBox(p, WID_FILL_LAYER, TRID_TOOLOPT_FILL_LAYER);

	mComboBoxAddItems_sepnull(cb, MLK_TR(TRID_TOOLOPT_FILL_LAYER_LIST), 0);
	mComboBoxSetSelItem_atIndex(cb, TOOLOPT_FILL_GET_LAYER(val));

	//塗りタイプ

	if(is_fill)
	{
		PanelOption_createPixelModeCombo(p, WID_FILL_PIXMODE,
			g_pixmode_tool_erase, TOOLOPT_FILL_GET_PIXMODE(val), FALSE);
	}

	return p;
}


/******************************
 * グラデーション
 ******************************/

/* mWidget::param1 = (mComboBox *) カスタムリスト
 *
 * データ変更時は、AppDraw::fmodify_grad_list を TRUE にする */

enum
{
	WID_GRAD_COLTYPE = 100,
	WID_GRAD_CUSTOM,
	WID_GRAD_BTT_MENU,
	WID_GRAD_DENSITY,
	WID_GRAD_PIXMODE,
	WID_GRAD_REVERSE,
	WID_GRAD_LOOP
};

static const uint16_t g_grad_menuid[] = {
	TRID_TOOLOPT_GRAD_MENU_EDIT, TRID_TOOLOPT_GRAD_MENU_NEW, MMENU_ARRAY16_SEP,
	TRID_TOOLOPT_GRAD_MENU_EDIT_LIST, MMENU_ARRAY16_SEP,
	TRID_TOOLOPT_GRAD_MENU_LOAD, TRID_TOOLOPT_GRAD_MENU_SAVE,
	MMENU_ARRAY16_END
};


/* mComboBox アイテム描画 */

static int _grad_cblist_draw(mPixbuf *pixbuf,mColumnItem *item,mColumnItemDrawInfo *info)
{
	mBox box = info->box;
	uint8_t *buf;

	//枠

	mPixbufBox(pixbuf, box.x + 3, box.y + 1, box.w - 6, box.h - 2, 0);

	//グラデーション

	buf = GRADITEM(item->param)->buf;

	GradItem_drawPixbuf(pixbuf,
		box.x + 4, box.y + 2, box.w - 8, box.h - 4,
		buf + 3, buf + 3 + buf[1] * 5, buf[0] & 2);

	return 0;
}

/* mComboBox 選択変更時 */

static void _grad_change_sel(mComboBox *cblist)
{
	mColumnItem *ci;
	GradItem *gi;

	ci = mComboBoxGetSelectItem(cblist);
	if(ci)
	{
		gi = (GradItem *)ci->param;

		//カスタムIDセット

		TOOLOPT_GRAD_SET_CUSTOM_ID(&APPDRAW->tool.opt_grad, gi->id);
	}
}

/* mComboBox にリストをセット & 選択 */

static void _grad_set_list(mComboBox *cblist)
{
	GradItem *gi;
	mColumnItem *ci;
	int selid;

	mComboBoxDeleteAllItem(cblist);
	
	selid = TOOLOPT_GRAD_GET_CUSTOM_ID(APPDRAW->tool.opt_grad);

	//

	MLK_LIST_FOR(APPDRAW->list_grad_custom, gi, GradItem)
	{
		ci = mComboBoxAddItem(cblist, NULL, 0, (intptr_t)gi);

		ci->draw = _grad_cblist_draw;

		if(gi->id == selid)
			mComboBoxSetSelItem(cblist, ci);
	}
}

/* (cmd) グラデーション編集 */

static void _grad_item_edit(mComboBox *cblist)
{
	GradItem *item;

	item = (GradItem *)mComboBoxGetItemParam(cblist, -1);;

	if(GradationEditDlg_run(cblist->wg.toplevel, item))
	{
		APPDRAW->fmodify_grad_list = TRUE;
		
		//コンボボックス更新
		mWidgetRedraw(MLK_WIDGET(cblist));
	}
}

/* (cmd) 新規作成 */

static void _grad_item_new(mComboBox *cblist)
{
	GradItem *pi;
	mColumnItem *ci;

	pi = GradationList_newItem(&APPDRAW->list_grad_custom);
	if(!pi) return;

	if(!GradationEditDlg_run(cblist->wg.toplevel, pi))
	{
		//キャンセル時は削除

		mListDelete(&APPDRAW->list_grad_custom, MLISTITEM(pi));
	}
	else
	{
		APPDRAW->fmodify_grad_list = TRUE;

		//mComboBox 追加

		ci = mComboBoxAddItem(cblist, NULL, 0, (intptr_t)pi);
		if(!ci) return;

		ci->draw = _grad_cblist_draw;

		mComboBoxSetSelItem(cblist, ci);

		//選択 ID

		TOOLOPT_GRAD_SET_CUSTOM_ID(&APPDRAW->tool.opt_grad, pi->id);
	}
}

/* (cmd) リスト編集 */

static void _grad_listedit(mComboBox *cblist)
{
	GradationListEditDlg_run(cblist->wg.toplevel);

	//mComboBox 再セット

	_grad_set_list(cblist);
}

/* (cmd) 読み込み */

static void _grad_loadfile(mComboBox *cblist)
{
	mStr str = MSTR_INIT;
	mList *list;
	int id;

	if(mSysDlg_openFile(cblist->wg.toplevel,
		"*.apgrad\t*.apgrad\tAll files\t*", 0,
		APPCONF->strFileDlgDir.buf, 0, &str))
	{
		mStrPathGetDir(&APPCONF->strFileDlgDir, str.buf);

		list = &APPDRAW->list_grad_custom;

		//読み込み

		mListDeleteAll(list);

		GradationList_loadFile(list, str.buf);

		APPDRAW->fmodify_grad_list = TRUE;

		//先頭アイテムを選択

		if(list->top)
		{
			id = GRADITEM(list->top)->id;
			
			TOOLOPT_GRAD_SET_CUSTOM_ID(&APPDRAW->tool.opt_grad, id);
		}

		//

		_grad_set_list(cblist);

		mStrFree(&str);
	}
}

/* (cmd) 保存 */

static void _grad_savefile(mWindow *parent)
{
	mStr str = MSTR_INIT;

	if(mSysDlg_saveFile(parent, "*.apgrad\tapgrad", 0,
		APPCONF->strFileDlgDir.buf, 0, &str, NULL))
	{
		mStrPathGetDir(&APPCONF->strFileDlgDir, str.buf);

		GradationList_saveFile(&APPDRAW->list_grad_custom, str.buf);
	
		mStrFree(&str);
	}
}

/* メニュー */

static void _grad_runmenu(mWidget *bttwg,mComboBox *cblist)
{
	mMenu *menu;
	mMenuItem *mi;
	int id;
	mBox box;

	//メニュー

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_grad_menuid);

	if(!mComboBoxGetSelectItem(cblist))
		mMenuSetItemEnable(menu, TRID_TOOLOPT_GRAD_MENU_EDIT, 0);

	mWidgetGetBox_rel(bttwg, &box);

	mi = mMenuPopup(menu, bttwg, 0, 0, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//コマンド

	switch(id)
	{
		//編集
		case TRID_TOOLOPT_GRAD_MENU_EDIT:
			_grad_item_edit(cblist);
			break;
		//新規追加
		case TRID_TOOLOPT_GRAD_MENU_NEW:
			_grad_item_new(cblist);
			break;
		//リスト編集
		case TRID_TOOLOPT_GRAD_MENU_EDIT_LIST:
			_grad_listedit(cblist);
			break;
		//読み込み
		case TRID_TOOLOPT_GRAD_MENU_LOAD:
			_grad_loadfile(cblist);
			break;
		//保存
		case TRID_TOOLOPT_GRAD_MENU_SAVE:
			_grad_savefile(cblist->wg.toplevel);
			break;
	}
}

/* イベント */

static int _grad_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		uint32_t *pd = &APPDRAW->tool.opt_grad;
	
		switch(ev->notify.id)
		{
			//色タイプ
			case WID_GRAD_COLTYPE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_GRAD_SET_COLTYPE(pd, ev->notify.param2);
				break;
			//カスタムリスト
			case WID_GRAD_CUSTOM:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					_grad_change_sel((mComboBox *)wg->param1);
				break;
			//濃度
			case WID_GRAD_DENSITY:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
					TOOLOPT_GRAD_SET_DENSITY(pd, ev->notify.param1);
				break;
			//塗り
			case WID_GRAD_PIXMODE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					TOOLOPT_GRAD_SET_PIXMODE(pd, ev->notify.param2);
				break;
			//反転
			case WID_GRAD_REVERSE:
				TOOLOPT_GRAD_TOGGLE_REVERSE(pd);
				break;
			//繰り返し
			case WID_GRAD_LOOP:
				TOOLOPT_GRAD_TOGGLE_LOOP(pd);
				break;
			//カスタムメニューボタン
			case WID_GRAD_BTT_MENU:
				_grad_runmenu(ev->notify.widget_from, (mComboBox *)wg->param1);
				break;
		}
	}

	return 1;
}

/** "グラデーション" 作成 */

static mWidget *_create_gradation(mWidget *parent)
{
	mWidget *p,*ct;
	mComboBox *cb;
	uint32_t val;

	p = PanelOption_createMainContainer(parent, 0, _grad_event_handle);

	val = APPDRAW->tool.opt_grad;

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	//色

	cb = mComboBoxCreate(p, WID_GRAD_COLTYPE, MLF_EXPAND_W, 0, 0);

	mComboBoxAddItems_sepnull(cb, MLK_TR(TRID_TOOLOPT_GRAD_COLTYPE_LIST), 0);
	mComboBoxSetSelItem_atIndex(cb, TOOLOPT_GRAD_GET_COLTYPE(val));

	//------ カスタムリスト

	ct = mContainerCreateHorz(p, 6, MLF_EXPAND_W, MLK_MAKE32_4(0,3,0,0));

	//リスト

	cb = mComboBoxCreate(ct, WID_GRAD_CUSTOM, MLF_EXPAND_W, 0, 0);
	cb->wg.hintRepW = 50;

	p->param1 = (intptr_t)cb;
	
	mComboBoxSetItemHeight(cb, 18);

	_grad_set_list(cb);

	//メニューボタン

	widget_createMenuButton(ct, WID_GRAD_BTT_MENU, MLF_MIDDLE, 0);

	//-------

	//濃度

	PanelOption_createDensityBar(p, WID_GRAD_DENSITY, TOOLOPT_GRAD_GET_DENSITY(val));

	//塗りタイプ

	PanelOption_createPixelModeCombo(p, WID_GRAD_PIXMODE, g_pixmode_tool_erase,
		TOOLOPT_GRAD_GET_PIXMODE(val), FALSE);

	//------ チェック

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	mCheckButtonCreate(p, WID_GRAD_REVERSE, 0, MLK_MAKE32_4(0,3,0,0), 0, MLK_TR(TRID_TOOLOPT_GRAD_REVERSE),
		TOOLOPT_GRAD_IS_REVERSE(val));

	mCheckButtonCreate(p, WID_GRAD_LOOP, 0, 0, 0, MLK_TR(TRID_TOOLOPT_GRAD_LOOP),
		TOOLOPT_GRAD_IS_LOOP(val));

	return p;
}


/******************************
 * テキスト
 ******************************/

enum
{
	WID_TEXT_HELP = 100
};


/* イベント */

static int _text_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//ヘルプ
			case WID_TEXT_HELP:
				AppHelp_message(wg->toplevel, HELP_TRGROUP_SINGLE, HELP_TRID_TOOL_TEXT);
				break;
		}
	}

	return 1;
}

/** "テキスト" 作成 */

static mWidget *_create_text(mWidget *parent)
{
	mWidget *p;

	p = PanelOption_createMainContainer(parent, 0, _text_event_handle);

	//ヘルプ

	widget_createHelpButton(p, WID_TEXT_HELP, 0, 0);

	return p;
}


/******************************
 * 選択範囲
 ******************************/


/* イベント */

static int _select_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//枠を非表示
			case 100:
				TOOLOPT_SELECT_TOGGLE_HIDE(&APPDRAW->tool.opt_select);
				break;
		}
	}

	return 1;
}

/** "選択範囲" 作成 */

static mWidget *_create_select(mWidget *parent)
{
	mWidget *p;
	uint32_t val;

	p = PanelOption_createMainContainer(parent, 0, _select_event_handle);

	val = APPDRAW->tool.opt_select;

	mCheckButtonCreate(p, 100, 0, 0, 0,
		MLK_TR2(TRGROUP_PANEL_OPTION_TOOL, TRID_TOOLOPT_SEL_HIDE),
		TOOLOPT_SELECT_IS_HIDE(val));

	return p;
}


/******************************
 * 切り貼り
 ******************************/

enum
{
	WID_CUTPASTE_OVERWRITE = 100,
	WID_CUTPASTE_ENABLE_MASK,
	WID_CUTPASTE_HELP
};


/* イベント */

static int _cutpaste_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//上書き貼り付け
			case WID_CUTPASTE_OVERWRITE:
				TOOLOPT_CUTPASTE_TOGGLE_OVERWRITE(&APPDRAW->tool.opt_cutpaste);
				break;
			//マスク類適用
			case WID_CUTPASTE_ENABLE_MASK:
				TOOLOPT_CUTPASTE_TOGGLE_ENABLE_MASK(&APPDRAW->tool.opt_cutpaste);
				break;
			//ヘルプ
			case WID_CUTPASTE_HELP:
				AppHelp_message(wg->toplevel, HELP_TRGROUP_SINGLE, HELP_TRID_TOOL_CUTPASTE);
				break;
		}
	}

	return 1;
}

/** "切り貼り" 作成 */

static mWidget *_create_cutpaste(mWidget *parent)
{
	mWidget *p;
	uint32_t val;

	p = PanelOption_createMainContainer(parent, 0, _cutpaste_event_handle);

	val = APPDRAW->tool.opt_cutpaste;

	//上書き貼り付け

	mCheckButtonCreate(p, WID_CUTPASTE_OVERWRITE, 0, 0, 0,
		MLK_TR2(TRGROUP_PANEL_OPTION_TOOL, TRID_TOOLOPT_OVERWRITE_PASTE),
		TOOLOPT_CUTPASTE_IS_OVERWRITE(val));

	//貼り付け時、マスク類を適用

	mCheckButtonCreate(p, WID_CUTPASTE_ENABLE_MASK, 0, MLK_MAKE32_4(0,3,0,0), 0,
		MLK_TR2(TRGROUP_PANEL_OPTION_TOOL, TRID_TOOLOPT_PASTE_ENABLE_MASK),
		TOOLOPT_CUTPASTE_IS_ENABLE_MASK(val));

	//ヘルプ

	widget_createHelpButton(p, WID_CUTPASTE_HELP, MLF_RIGHT, 0);

	return p;
}


/******************************
 * 矩形編集
 ******************************/

enum
{
	WID_BOXEDIT_LIST = 100,
	WID_BOXEDIT_RUN
};


/* イベント */

static int _boxedit_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//リスト
			case WID_BOXEDIT_LIST:
				APPDRAW->tool.opt_boxedit = ev->notify.param2;
				break;
			//実行
			case WID_BOXEDIT_RUN:
				drawBoxSel_run_edit(APPDRAW, 100 + APPDRAW->tool.opt_boxedit);
				break;
		}
	}

	return 1;
}

/** "矩形編集" 作成 */

static mWidget *_create_boxedit(mWidget *parent)
{
	mWidget *p;
	mComboBox *cb;

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	p = PanelOption_createMainContainer(parent, 0, _boxedit_event_handle);

	//mComboBox

	cb = mComboBoxCreate(p, WID_BOXEDIT_LIST, MLF_EXPAND_W, 0, 0);

	mComboBoxAddItems_sepnull(cb, MLK_TR(TRID_TOOLOPT_BOXEDIT_LIST), 0);
	mComboBoxSetSelItem_atIndex(cb, APPDRAW->tool.opt_boxedit);

	//実行

	mButtonCreate(p, WID_BOXEDIT_RUN, 0, MLK_MAKE32_4(0,3,0,0), 0, MLK_TR(TRID_TOOLOPT_RUN));

	return p;
}


/******************************
 * スタンプ
 ******************************/
/* mWidget::param1 = (PrevTileImage *) */


enum
{
	WID_STAMP_LOAD = 100,
	WID_STAMP_CLEAR,
	WID_STAMP_CB_TRANS
};


/* イメージ変更時
 *
 * 外部からのイメージ変更時用 */

void PanelOption_toolStamp_changeImage(mWidget *wg)
{
	PrevTileImage_setImage((PrevTileImage *)wg->param1,
		APPDRAW->stamp.img, APPDRAW->stamp.size.w, APPDRAW->stamp.size.h);
}

/* 画像読み込み */

static void _stamp_loadimage(mWidget *wg)
{
	mStr str = MSTR_INIT;
	mlkerr ret;

	if(FileDialog_openImage_confdir(wg->toplevel,
		AppResource_getOpenFileFilter_normal(), 0, &str))
	{
		ret = drawStamp_loadImage(APPDRAW, str.buf);

		if(ret)
			MainWindow_errmes_win(wg->toplevel, ret, NULL);
		else
			PanelOption_toolStamp_changeImage(wg);

		mStrFree(&str);
	}
}

/* イベント */

static int _stamp_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//画像読み込み
				case WID_STAMP_LOAD:
					_stamp_loadimage(wg);
					break;
				//クリア
				case WID_STAMP_CLEAR:
					drawStamp_clearImage(APPDRAW);
					break;
				//変形
				case WID_STAMP_CB_TRANS:
					if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
						TOOLOPT_STAMP_SET_TRANS(&APPDRAW->tool.opt_stamp, ev->notify.param2);
					break;
			}
			break;
	}

	return 1;
}

/* (プレビュー) イベント */

static int _stamp_prev_event_handle(mWidget *wg,mEvent *ev)
{
	//ファイルドロップ

	if(ev->type == MEVENT_DROP_FILES)
	{
		if(drawStamp_loadImage(APPDRAW, *(ev->dropfiles.files)) == MLKERR_OK)
		{
			PrevTileImage_setImage((PrevTileImage *)wg,
				APPDRAW->stamp.img, APPDRAW->stamp.size.w, APPDRAW->stamp.size.h);
		}
	}

	return 1;
}

/** "スタンプ" 作成 */

static mWidget *_create_stamp(mWidget *parent)
{
	mWidget *p,*wg,*cth,*ctv;
	mComboBox *cb;
	int val,i;

	val = APPDRAW->tool.opt_stamp;

	p = PanelOption_createMainContainer(parent, 0, _stamp_event_handle);

	MLK_TRGROUP(TRGROUP_PANEL_OPTION_TOOL);

	//---- プレビュー + ボタン

	cth = mContainerCreateHorz(MLK_WIDGET(p), 10, 0, MLK_MAKE32_4(0,0,0,5));

	//プレビュー

	wg = (mWidget *)PrevTileImage_new(cth, 70, 70);

	p->param1 = (intptr_t)wg;

	wg->fstate |= MWIDGET_STATE_ENABLE_DROP;
	wg->event = _stamp_prev_event_handle;

	PrevTileImage_setImage((PrevTileImage *)wg,
		APPDRAW->stamp.img, APPDRAW->stamp.size.w, APPDRAW->stamp.size.h);

	//ボタン

	ctv = mContainerCreateVert(cth, 3, 0, 0);

	for(i = 0; i < 2; i++)
		mButtonCreate(ctv, WID_STAMP_LOAD + i, 0, 0, 0, MLK_TR(TRID_TOOLOPT_LOAD + i));

	//------ 変形

	cb = PanelOption_createComboBox(MLK_WIDGET(p), WID_STAMP_CB_TRANS, TRID_TOOLOPT_TRANSFORM);

	mComboBoxAddItems_sepnull(cb, MLK_TR(TRID_TOOLOPT_STAMP_TRANS_LIST), 0);
	mComboBoxSetSelItem_atIndex(cb, TOOLOPT_STAMP_GET_TRANS(val));

	return p;
}


//==============================
// main
//==============================

/* mPager の mWidget::param1 = 中身のコンテナ
 * (外部からの情報更新用) */


/** "ツール" のページ作成 */

mlkbool PanelOption_createPage_tool(mPager *p,mPagerInfo *info)
{
	mWidget *ctv,*page;

	mContainerSetType_vert(MLK_CONTAINER(p), 0);
	mContainerSetPadding_same(MLK_CONTAINER(p), 0);

	//mContainerView

	ctv = (mWidget *)mContainerViewNew(MLK_WIDGET(p), 0, 0);

	ctv->flayout = MLF_EXPAND_WH;
	ctv->hintRepW = 1;

	//ツールごとの内容

	page = NULL;

	switch(APPDRAW->tool.no)
	{
		//ドットペン/消しゴム
		case TOOL_DOTPEN:
		case TOOL_DOTPEN_ERASE:
			page = _create_dotpen(ctv);
			break;
		//指先
		case TOOL_FINGER:
			page = _create_finger(ctv);
			break;
		//図形塗り/消し
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			page = _create_fill_polygon(ctv);
			break;
		//塗りつぶし/自動選択
		case TOOL_FILL:
		case TOOL_SELECT_FILL:
			page = _create_fill(ctv);
			break;
		//グラデーション
		case TOOL_GRADATION:
			page = _create_gradation(ctv);
			break;
		//テキスト
		case TOOL_TEXT:
			page = _create_text(ctv);
			break;
		//選択範囲
		case TOOL_SELECT:
			page = _create_select(ctv);
			break;
		//切り貼り
		case TOOL_CUTPASTE:
			page = _create_cutpaste(ctv);
			break;
		//矩形編集
		case TOOL_BOXEDIT:
			page = _create_boxedit(ctv);
			break;
		//スタンプ
		case TOOL_STAMP:
			page = _create_stamp(ctv);
			break;
	}

	mContainerViewSetPage(MLK_CONTAINERVIEW(ctv), page);

	p->wg.param1 = (intptr_t)page;

	return TRUE;
}

