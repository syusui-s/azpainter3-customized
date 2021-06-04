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
 * [panel] ブラシ設定
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_panel.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_combobox.h"
#include "mlk_containerview.h"
#include "mlk_expander.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_str.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_draw_toollist.h"

#include "toollist.h"
#include "appconfig.h"

#include "draw_toollist.h"

#include "panel.h"
#include "panel_func.h"
#include "widget_func.h"
#include "valuebar.h"
#include "sel_material.h"
#include "dlg_toollist.h"

#include "trid.h"


//------------------------

typedef struct
{
	MLK_CONTAINER_DEF

	mContainerView *ctview;
	mWidget *ctview_page;

	mWidget *wg_btt_save,
		*wg_water_preset[CONFIG_WATER_PRESET_NUM],
		*wg_water_preset_menu;
	mComboBox *cb_type,
		*cb_unit_size,
		*cb_pixmode,
		*cb_hosei_type;
	mCheckButton *ck_autosave,
		*ck_antialias,
		*ck_curve,
		*ck_water_tpwhite,
		*ck_rotate_dir,
		*ck_pressure_common;
	ValueBar *bar_size,
		*bar_opacity,
		*bar_hosei,
		*bar_interval,
		*bar_rand_size,
		*bar_rand_pos,
		*bar_water[2],
		*bar_shape_hard,
		*bar_shape_sand,
		*bar_angle_base,
		*bar_angle_rand,
		*bar_pressure_size,
		*bar_pressure_opacity;
	SelMaterial *selmat_tex,
		*selmat_shape;
}_topct;

//ウィジェットID
enum
{
	WID_BTT_SAVE = 100,
	WID_CK_AUTOSAVE,

	WID_CB_TYPE,
	WID_CB_UNIT_SIZE,
	WID_BTT_SIZE_OPT,
	WID_BAR_SIZE,
	WID_BAR_OPACITY,
	WID_CB_PIXMODE,
	WID_CB_HOSEI_TYPE,
	WID_BAR_HOSEI_VAL,

	//色々
	WID_BAR_INTERVAL,
	WID_EXPAND_OTHRES,
	WID_BAR_RAND_SIZE,
	WID_BAR_RAND_POS,
	WID_SEL_TEXTURE,
	WID_CK_ANTIALIAS,
	WID_CK_CURVE,

	//水彩
	WID_EXPAND_WATER,
	WID_BAR_WATER1,
	WID_BAR_WATER2,
	WID_CK_WATER_TP_WHITE,
	WID_BTT_WATER_PRESET_MENU,

	//形状
	WID_EXPAND_SHAPE,
	WID_SEL_SHAPE,
	WID_BAR_SHAPE_HARD,
	WID_BAR_SHAPE_SAND,
	WID_BAR_ANGLE_BASE,
	WID_BAR_ANGLE_RAND,
	WID_CK_ROTATE_DIR,

	//筆圧
	WID_EXPAND_PRESSURE,
	WID_BAR_PRESSURE_SIZE,
	WID_BAR_PRESSURE_OPACITY,
	WID_CK_PRESSURE_COMMON,
	WID_BTT_PRESSURE_EDIT,

	WID_BTT_WATER_PRESET_TOP = 1000
};

//翻訳ID
enum
{
	TRID_ALWAYS_SAVE,
	TRID_TYPE_LIST,
	TRID_BRUSHSIZE,
	TRID_HOSEI,
	TRID_HOSEI_TYPE_LIST,
	TRID_INTERVAL,
	TRID_RAND_SIZE,
	TRID_RAND_POS,
	TRID_CURVE,
	TRID_WATER,
	TRID_WATER_PARAM1,
	TRID_WATER_PARAM2,
	TRID_WATER_TP_WHITE,
	TRID_PRESET,
	TRID_BRUSH_SHAPE,
	TRID_SHAPE_IMAGE,
	TRID_SHAPE_HARD,
	TRID_SHAPE_SAND,
	TRID_ANGLE_BASE,
	TRID_ANGLE_RAND,
	TRID_ROTATE_DIR,
	TRID_PRESSURE,
	TRID_PRESSURE_SIZE,
	TRID_PRESSURE_OPACITY,
	TRID_PRESSURE_EDIT,
	TRID_PRESSURE_COMMON,
	TRID_OTHRES,

	TRID_WATER_PRESET_MENU_REGIST = 1000,
	TRID_WATER_PRESET_MENU_RESET
};

static void _create_widget(_topct *p,mWidget *parent);
static void _event_notify(_topct *p,mEventNotify *ev);

//------------------------


//===========================
// sub
//===========================


/* サイズのバー情報セット */

static void _set_barinfo_size(_topct *p)
{
	BrushEditData *pb = APPDRAW->tlist->brush;

	ValueBar_setStatus_dig(p->bar_size, 1, pb->v.size_min, pb->v.size_max, pb->v.size);
}

/* 塗りタイプのリストセット */

static void _set_pixelmode_list(_topct *p)
{
	BrushEditData *pb = APPDRAW->tlist->brush;
	int num;

	num = BrushEditData_getPixelModeNum(pb);

	//リスト変更

	if(num != mComboBoxGetItemNum(p->cb_pixmode))
	{
		MLK_TRGROUP(TRGROUP_PIXELMODE_BRUSH);

		mComboBoxDeleteAllItem(p->cb_pixmode);

		mComboBoxAddItems_tr(p->cb_pixmode, 0, num, 0);
	}

	//選択

	mComboBoxSetSelItem_atIndex(p->cb_pixmode, pb->v.pixelmode);
}

/* ブラシタイプによるウィジェットの有効/無効 */

static void _enable_brushtype(_topct *p,int type)
{
	mlkbool no_blur,yes_water;
	int i;

	no_blur = (type != BRUSH_TYPE_BLUR);
	yes_water = (type == BRUSH_TYPE_WATER);
	
	//水彩以外時は [水彩] を無効。
	//水彩/ぼかし時は、[塗り] を無効。

	mWidgetEnable(MLK_WIDGET(p->cb_pixmode), (no_blur && !yes_water));

	//水彩

	for(i = 0; i < 2; i++)
		mWidgetEnable(MLK_WIDGET(p->bar_water[i]), yes_water);

	for(i = 0; i < CONFIG_WATER_PRESET_NUM; i++)
		mWidgetEnable(p->wg_water_preset[i], yes_water);

	mWidgetEnable(p->wg_water_preset_menu, yes_water);

	mWidgetEnable(MLK_WIDGET(p->ck_water_tpwhite), yes_water);
}

/* 項目値セット */

static void _set_value(_topct *p)
{
	BrushEditData *pb;
	int i,fbrush;

	pb = drawToolList_getBrush();

	fbrush = (pb != NULL);

	//保存ボタン

	mWidgetEnable(p->wg_btt_save, (fbrush && !(pb->v.flags & BRUSH_FLAGS_AUTO_SAVE)));

	//常に保存

	mWidgetEnable(MLK_WIDGET(p->ck_autosave), fbrush);

	mCheckButtonSetState(p->ck_autosave, (fbrush && (pb->v.flags & BRUSH_FLAGS_AUTO_SAVE)));

	//------ ブラシ項目

	if(!pb) return;

	//タイプ

	mComboBoxSetSelItem_atIndex(p->cb_type, pb->v.brush_type);

	//サイズ

	mComboBoxSetSelItem_atIndex(p->cb_unit_size, pb->v.unit_size);

	_set_barinfo_size(p);

	//濃度

	ValueBar_setPos(p->bar_opacity, pb->v.opacity);

	//手ブレ補正

	mComboBoxSetSelItem_atIndex(p->cb_hosei_type, pb->v.hosei_type);

	ValueBar_setPos(p->bar_hosei, pb->v.hosei_val);

	//塗りタイプ

	_set_pixelmode_list(p);

	//点の間隔、サイズランダム、位置ランダム

	ValueBar_setPos(p->bar_interval, pb->v.interval);
	ValueBar_setPos(p->bar_rand_size, pb->v.rand_size);
	ValueBar_setPos(p->bar_rand_pos, pb->v.rand_pos);

	//テクスチャなど

	SelMaterial_setPath(p->selmat_tex, pb->str_texture.buf);

	mCheckButtonSetState(p->ck_antialias, pb->v.flags & BRUSH_FLAGS_ANTIALIAS);
	mCheckButtonSetState(p->ck_curve, pb->v.flags & BRUSH_FLAGS_CURVE);

	//水彩

	for(i = 0; i < 2; i++)
		ValueBar_setPos(p->bar_water[i], pb->v.water_param[i]);

	mCheckButtonSetState(p->ck_water_tpwhite, pb->v.flags & BRUSH_FLAGS_WATER_TP_WHITE);

	//形状

	SelMaterial_setPath(p->selmat_shape, pb->str_shape.buf);

	ValueBar_setPos(p->bar_shape_hard, pb->v.shape_hard);
	ValueBar_setPos(p->bar_shape_sand, pb->v.shape_sand);

	ValueBar_setPos(p->bar_angle_base, pb->v.angle_base);
	ValueBar_setPos(p->bar_angle_rand, pb->v.rand_angle);

	mCheckButtonSetState(p->ck_rotate_dir, pb->v.flags & BRUSH_FLAGS_ROTATE_DIR);

	//筆圧

	ValueBar_setPos(p->bar_pressure_size, pb->v.pressure_size);
	ValueBar_setPos(p->bar_pressure_opacity, pb->v.pressure_opacity);

	mCheckButtonSetState(p->ck_pressure_common, pb->v.flags & BRUSH_FLAGS_PRESSURE_COMMON);

	//有効/無効

	_enable_brushtype(p, pb->v.brush_type);
}


//===========================
// パネル main
//===========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify((_topct *)wg, (mEventNotify *)ev);
			break;
	}

	return 1;
}

/* mPanel 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	_topct *p;
	mWidget *ct;

	//コンテナ (トップ)

	p = (_topct *)mContainerNew(parent, sizeof(_topct));

	p->wg.event = _event_handle;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	MLK_TRGROUP(TRGROUP_PANEL_BRUSHOPT);

	//---- 上部

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, MLF_EXPAND_W, MLK_MAKE32_4(3,2,3,3));

	//保存ボタン

	p->wg_btt_save = widget_createSaveButton(ct, WID_BTT_SAVE, MLF_MIDDLE | MLF_EXPAND_X, 0);

	//常に保存

	p->ck_autosave = mCheckButtonCreate(ct, WID_CK_AUTOSAVE, MLF_MIDDLE, 0, 0, MLK_TR(TRID_ALWAYS_SAVE), 0);

	//---- 下部

	//mContainerView

	p->ctview = mContainerViewNew(MLK_WIDGET(p), 0, 0);

	p->ctview->wg.flayout = MLF_EXPAND_WH;

	//内容のコンテナ

	ct = mContainerCreateVert(MLK_WIDGET(p->ctview), 3, MLF_EXPAND_WH, 0);

	p->ctview_page = ct;

	mContainerSetPadding_pack4(MLK_CONTAINER(ct), MLK_MAKE32_4(3,2,4,4));

	mContainerViewSetPage(p->ctview, ct);

	_create_widget(p, ct);

	//-----

	_set_value(p);

	return (mWidget *)p;
}

/** パネル作成 */

void PanelBrushOpt_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_BRUSHOPT, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}


//===========================
// 外部から呼ばれる関数
//===========================


/* _topct 取得 */

static _topct *_get_topct(void)
{
	return (_topct *)Panel_getContents(PANEL_BRUSHOPT);
}

/** 項目値セット */

void PanelBrushOpt_setValue(void)
{
	_topct *p = _get_topct();

	if(p)
		_set_value(p);
}

/** ブラシサイズを更新 */

void PanelBrushOpt_updateBrushSize(void)
{
	_topct *p = _get_topct();

	if(p && drawToolList_getBrush())
	{
		_set_barinfo_size(p);
	}
}

/** ブラシ濃度を更新 */

void PanelBrushOpt_updateBrushOpacity(void)
{
	_topct *p = _get_topct();

	if(p && drawToolList_getBrush())
		ValueBar_setPos(p->bar_opacity, APPDRAW->tlist->brush->v.opacity);
}


//===========================
// (sub) イベント
//===========================


/* ブラシサイズ設定 */

static void _set_brushsize_opt(_topct *p)
{
	int min,max,n;

	if(drawToolList_isnotBrushItem()) return;

	min = APPDRAW->tlist->brush->v.size_min;
	max = APPDRAW->tlist->brush->v.size_max;

	if(ToolListDialog_brushSizeOpt(p->wg.toplevel, &min, &max))
	{
		if(min > max)
			n = min, min = max, max = n;

		if(min == max)
		{
			if(max == BRUSH_SIZE_MAX)
				min--;
			else
				max++;
		}

		//

		drawBrushParam_setSizeOpt(min, max);

		_set_barinfo_size(p);

		PanelToolList_updateBrushSize();
	}
}

/* 筆圧カーブ編集 */

static void _set_pressure_curve(_topct *p)
{
	uint32_t *pt;

	if(drawToolList_isnotBrushItem()) return;

	//共通 or 個別

	if(APPDRAW->tlist->brush->v.flags & BRUSH_FLAGS_PRESSURE_COMMON)
		pt = APPDRAW->tlist->pt_press_comm;
	else
		pt = APPDRAW->tlist->brush->v.pt_press_curve;

	//

	if(BrushDialog_pressure(p->wg.toplevel, pt))
	{
		drawBrushParam_changePressureCurve();
	}
}

/* 水彩プリセット呼び出し */

static void _call_water_preset(_topct *p,int no)
{
	uint32_t v = APPCONF->panel.water_preset[no];
	int i,val;

	for(i = 0; i < 2; i++)
	{
		val = (v >> (i * 10)) & 1023;

		ValueBar_setPos(p->bar_water[i], val);

		drawBrushParam_setWaterParam(i, val);
	}
}

/* 水彩プリセットメニュー実行 */

static void _run_menu_water_preset(_topct *p)
{
	mMenu *menu;
	mMenuItem *mi;
	const char *text;
	mStr str = MSTR_INIT;
	mBox box;
	int i;

	MLK_TRGROUP(TRGROUP_PANEL_BRUSHOPT);

	menu = mMenuNew();

	//%c に登録

	text = MLK_TR(TRID_WATER_PRESET_MENU_REGIST);

	for(i = 0; i < CONFIG_WATER_PRESET_NUM; i++)
	{
		mStrSetFormat(&str, text, 'A' + i);
		mMenuAppendText_copy(menu, i, str.buf, str.len);
	}

	mStrFree(&str);

	//

	mMenuAppendSep(menu);
	mMenuAppendText(menu, 100, MLK_TR(TRID_WATER_PRESET_MENU_RESET));

	//

	mWidgetGetBox_rel(p->wg_water_preset_menu, &box);

	mi = mMenuPopup(menu, MLK_WIDGET(p->wg_water_preset_menu), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);
	
	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//--------

	if(i == -1) return;

	if(i == 100)
		//初期値に戻す
		AppConfig_setWaterPreset_default();
	else
	{
		//登録

		APPCONF->panel.water_preset[i] = ValueBar_getPos(p->bar_water[0])
			| (ValueBar_getPos(p->bar_water[1]) << 10);
	}
}

/* 通知イベント */

void _event_notify(_topct *p,mEventNotify *ev)
{
	int id,type,n;
	intptr_t param1,param2;

	id = ev->id;
	type = ev->notify_type;
	param1 = ev->param1;
	param2 = ev->param2;

	//水彩プリセット呼び出し

	if(id >= WID_BTT_WATER_PRESET_TOP && id < WID_BTT_WATER_PRESET_TOP + CONFIG_WATER_PRESET_NUM)
	{
		_call_water_preset(p, id - WID_BTT_WATER_PRESET_TOP);
		return;
	}

	//

	switch(id)
	{
		//保存ボタン
		case WID_BTT_SAVE:
			//TRUE でタイプ変更によるアイコンの変更
			
			if(drawBrushParam_save())
				PanelToolList_updateList();
			break;
		//常に保存
		case WID_CK_AUTOSAVE:
			n = drawBrushParam_toggleAutoSave();

			mWidgetEnable(p->wg_btt_save, n & 1);

			if(n & 2)
				PanelToolList_updateList();
			break;

		//タイプ
		// :塗りタイプも変更される
		case WID_CB_TYPE:
			if(type == MCOMBOBOX_N_CHANGE_SEL)
			{
				//TRUE でアイコン変更
				
				if(drawBrushParam_setType(param2))
					PanelToolList_updateList();

				_set_pixelmode_list(p);
				_enable_brushtype(p, param2);
			}
			break;
		//サイズ
		case WID_BAR_SIZE:
			if(param2 & VALUEBAR_ACT_F_ONCE) //値の確定時
			{
				drawBrushParam_setSize(param1);

				PanelToolList_updateBrushSize();
			}
			break;
		//サイズ単位
		case WID_CB_UNIT_SIZE:
			if(type == MCOMBOBOX_N_CHANGE_SEL)
				drawBrushParam_setSizeUnit(param2);
			break;
		//サイズ設定
		case WID_BTT_SIZE_OPT:
			_set_brushsize_opt(p);
			break;
		//濃度
		case WID_BAR_OPACITY:
			if(param2 & VALUEBAR_ACT_F_ONCE)
			{
				drawBrushParam_setOpacity(param1);

				PanelToolList_updateBrushOpacity();
			}
			break;

		//補正タイプ
		case WID_CB_HOSEI_TYPE:
			if(type == MCOMBOBOX_N_CHANGE_SEL)
				drawBrushParam_setHoseiType(param2);
			break;
		//補正強さ
		case WID_BAR_HOSEI_VAL:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setHoseiVal(param1);
			break;

		//塗りタイプ
		case WID_CB_PIXMODE:
			if(type == MCOMBOBOX_N_CHANGE_SEL)
				drawBrushParam_setPixelMode(param2);
			break;

		//間隔
		case WID_BAR_INTERVAL:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setInterval(param1);
			break;
		//ランダムサイズ
		case WID_BAR_RAND_SIZE:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setRandomSize(param1);
			break;
		//ランダム位置
		case WID_BAR_RAND_POS:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setRandomPos(param1);
			break;

		//テクスチャ画像
		case WID_SEL_TEXTURE:
			drawBrushParam_setTexturePath((const char *)param1);
			break;
		//アンチエイリアス
		case WID_CK_ANTIALIAS:
			drawBrushParam_toggleAntialias();
			break;
		//曲線補間
		case WID_CK_CURVE:
			drawBrushParam_toggleCurve();
			break;

		//水彩パラメータ
		case WID_BAR_WATER1:
		case WID_BAR_WATER2:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setWaterParam(id - WID_BAR_WATER1, param1);
			break;
		//水彩:透明色は白
		case WID_CK_WATER_TP_WHITE:
			drawBrushParam_toggleWaterTpWhite();
			break;
		//水彩プリセットメニュー
		case WID_BTT_WATER_PRESET_MENU:
			_run_menu_water_preset(p);
			break;

		//形状画像
		case WID_SEL_SHAPE:
			drawBrushParam_setShapePath((const char *)param1);
			break;
		//硬さ
		case WID_BAR_SHAPE_HARD:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setShapeHard(param1);
			break;
		//砂化
		case WID_BAR_SHAPE_SAND:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setShapeSand(param1);
			break;
		//角度ベース
		case WID_BAR_ANGLE_BASE:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setAngleBase(param1);
			break;
		//角度ランダム
		case WID_BAR_ANGLE_RAND:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setAngleRandom(param1);
			break;
		//進行方向
		case WID_CK_ROTATE_DIR:
			drawBrushParam_toggleShapeRotateDir();
			break;

		//筆圧サイズ
		case WID_BAR_PRESSURE_SIZE:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setPressureSize(param1);
			break;
		//筆圧濃度
		case WID_BAR_PRESSURE_OPACITY:
			if(param2 & VALUEBAR_ACT_F_ONCE)
				drawBrushParam_setPressureOpacity(param1);
			break;
		//共通の筆圧カーブ
		case WID_CK_PRESSURE_COMMON:
			drawBrushParam_togglePressureCommon();
			break;
		//筆圧カーブ編集
		case WID_BTT_PRESSURE_EDIT:
			_set_pressure_curve(p);
			break;

		//mExpander 展開状態変化
		case WID_EXPAND_OTHRES:
		case WID_EXPAND_WATER:
		case WID_EXPAND_SHAPE:
		case WID_EXPAND_PRESSURE:
			APPCONF->panel.brushopt_flags ^= ev->widget_from->param1;
		
			mContainerViewReLayout(p->ctview);
			break;
	}
}


//===========================
// (sub) ウィジェット作成
//===========================


/* mExpander 作成 */

static mWidget *_create_expander(mWidget *parent,int wid,int trid,uint32_t expand_flag)
{
	mExpander *p;

	p = mExpanderCreate(parent, wid, MLF_EXPAND_W, MLK_MAKE32_4(0,8,0,0),
		MEXPANDER_S_SEP_TOP, MLK_TR(trid));

	p->wg.param1 = expand_flag;

	MLK_CONTAINER(p)->ct.sep = 3;
	
	mExpanderSetPadding_pack4(p, MLK_MAKE32_4(0,8,0,3));

	mExpanderToggle(p, !(APPCONF->panel.brushopt_flags & expand_flag));

	return (mWidget *)p;
}

/* label + ValueBar 作成 (縦) */

static ValueBar *_create_label_bar_vert(mWidget *parent,const char *label,uint32_t margin,int wid,int dig,int min,int max)
{
	mLabelCreate(parent, 0, margin, 0, label);
	
	return ValueBar_new(parent, wid, MLF_EXPAND_W, dig, min, max, min);
}

/* label + combo 作成 (横) */

static mComboBox *_create_label_combo_horz(mWidget *parent,const char *label,int wid,const char *liststr)
{
	mWidget *ct;
	mComboBox *cb;

	ct = mContainerCreateHorz(parent, 4, 0, 0);

	mLabelCreate(ct, MLF_MIDDLE, 0, 0, label);

	//

	cb = mComboBoxCreate(ct, wid, 0, 0, 0);

	mComboBoxAddItems_sepnull(cb, liststr, 0);
	mComboBoxSetAutoWidth(cb);

	return cb;
}

/* combobox 作成 (ヌル区切り) */

static mComboBox *_create_combo_null(mWidget *parent,int wid,uint32_t flayout,const char *liststr)
{
	mComboBox *cb;

	cb = mComboBoxCreate(parent, wid, flayout, 0, 0);

	mComboBoxAddItems_sepnull(cb, liststr, 0);
	mComboBoxSetAutoWidth(cb);

	return cb;
}


//-----------------


/* メイン項目 */

static void _create_widget_main(_topct *p,mWidget *ct)
{
	mWidget *ct2;

	//---- タイプ

	p->cb_type = _create_label_combo_horz(ct, MLK_TR2(TRGROUP_WORD, TRID_WORD_TYPE),
		WID_CB_TYPE, MLK_TR(TRID_TYPE_LIST));

	//---- サイズ

	ct2 = mContainerCreateHorz(ct, 4, MLF_EXPAND_W, 0);

	//label

	mLabelCreate(ct2, MLF_BOTTOM | MLF_EXPAND_X, 0, 0, MLK_TR(TRID_BRUSHSIZE));

	//単位

	p->cb_unit_size = _create_combo_null(ct2, WID_CB_UNIT_SIZE, MLF_MIDDLE, "px\0mm\0");

	//設定ボタン

	widget_createEditButton(ct2, WID_BTT_SIZE_OPT, MLF_MIDDLE, 0);

	//バー

	p->bar_size = ValueBar_new(ct, WID_BAR_SIZE, MLF_EXPAND_W,
		1, BRUSH_SIZE_MIN, BRUSH_SIZE_MAX, 1);

	//---- 濃度

	p->bar_opacity = _create_label_bar_vert(ct, MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY), 0,
		WID_BAR_OPACITY, 0, 1, 100);

	//---- 手ブレ補正

	mLabelCreate(ct, 0, 0, 0, MLK_TR(TRID_HOSEI));

	p->cb_hosei_type = _create_combo_null(ct, WID_CB_HOSEI_TYPE, 0, MLK_TR(TRID_HOSEI_TYPE_LIST));

	p->bar_hosei = ValueBar_new(ct, WID_BAR_HOSEI_VAL, MLF_EXPAND_W,
		0, 0, BRUSH_HOSEI_VAL_MAX, 0);

	//---- 塗りタイプ

	mLabelCreate(ct, 0, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_PIXELMODE));

	p->cb_pixmode = mComboBoxCreate(ct, WID_CB_PIXMODE, MLF_EXPAND_W, 0, 0);
}

/* 色々 */

static void _create_widget_othres(_topct *p,mWidget *cttop)
{
	mWidget *ct;

	//mExpander

	ct = _create_expander(cttop, WID_EXPAND_OTHRES, TRID_OTHRES, CONFIG_PANEL_BRUSHOPT_F_HIDE_OTHRES);

	//---- 点の間隔

	p->bar_interval = _create_label_bar_vert(ct, MLK_TR(TRID_INTERVAL), 0,
		WID_BAR_INTERVAL, 2, BRUSH_INTERVAL_MIN, BRUSH_INTERVAL_MAX);

	//---- ブラシサイズランダム

	p->bar_rand_size = _create_label_bar_vert(ct, MLK_TR(TRID_RAND_SIZE), 0,
		WID_BAR_RAND_SIZE, 1, 0, 1000);

	//---- 位置ランダム

	p->bar_rand_pos = _create_label_bar_vert(ct, MLK_TR(TRID_RAND_POS), 0,
		WID_BAR_RAND_POS, 2, 0, 5000);

	//---- テクスチャ

	mLabelCreate(ct, 0, MLK_MAKE32_4(0,3,0,0), 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_TEXTURE));

	p->selmat_tex = SelMaterial_new(ct, WID_SEL_TEXTURE, SELMATERIAL_TYPE_BRUSH_TEXTURE);

	//---- アンチエイリアス

	p->ck_antialias = mCheckButtonCreate(ct, WID_CK_ANTIALIAS, 0, MLK_MAKE32_4(0,2,0,0), 0,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_ANTIALIAS), 0);

	//---- 曲線補間

	p->ck_curve = mCheckButtonCreate(ct, WID_CK_CURVE, 0, 0, 0, MLK_TR(TRID_CURVE), 0);
}

/* 水彩項目 */

static void _create_widget_water(_topct *p,mWidget *cttop)
{
	mWidget *ct,*ct2,*wg;
	int i;
	char m[2] = {0,0};

	//mExpander

	ct = _create_expander(cttop, WID_EXPAND_WATER, TRID_WATER, CONFIG_PANEL_BRUSHOPT_F_HIDE_WATER);

	//プリセット

	ct2 = mContainerCreateHorz(ct, 0, 0, 0);

	mLabelCreate(ct2, MLF_MIDDLE, MLK_MAKE32_4(0,0,5,0), 0, MLK_TR(TRID_PRESET));

	for(i = 0; i < CONFIG_WATER_PRESET_NUM; i++)
	{
		m[0] = 'A' + i;
	
		p->wg_water_preset[i] = wg = (mWidget *)mButtonCreate(ct2,
			WID_BTT_WATER_PRESET_TOP + i, MLF_MIDDLE, MLK_MAKE32_4(0,0,1,0),
			MBUTTON_S_COPYTEXT | MBUTTON_S_REAL_WH, m);

		wg->hintMinW = wg->hintMinH = 21;
	}

	p->wg_water_preset_menu = (mWidget *)widget_createMenuButton(ct2,
		WID_BTT_WATER_PRESET_MENU, MLF_MIDDLE, MLK_MAKE32_4(8,0,0,0));

	//各値

	for(i = 0; i < 2; i++)
	{
		p->bar_water[i] = _create_label_bar_vert(ct, MLK_TR(TRID_WATER_PARAM1 + i), 0,
			WID_BAR_WATER1 + i, 1, 0, 1000);
	}

	//透明色は白

	p->ck_water_tpwhite = mCheckButtonCreate(ct, WID_CK_WATER_TP_WHITE, 0, MLK_MAKE32_4(0,2,0,0),
		0, MLK_TR(TRID_WATER_TP_WHITE), 0);
}

/* ブラシ形状項目 */

static void _create_widget_shape(_topct *p,mWidget *cttop)
{
	mWidget *ct;

	//mExpander

	ct = _create_expander(cttop, WID_EXPAND_SHAPE, TRID_BRUSH_SHAPE, CONFIG_PANEL_BRUSHOPT_F_HIDE_SHAPE);

	//形状画像

	widget_createLabel(ct, MLK_TR(TRID_SHAPE_IMAGE));

	p->selmat_shape = SelMaterial_new(ct, WID_SEL_SHAPE, SELMATERIAL_TYPE_BRUSH_SHAPE);

	//硬さ

	p->bar_shape_hard = _create_label_bar_vert(ct, MLK_TR(TRID_SHAPE_HARD), 0,
		WID_BAR_SHAPE_HARD, 0, 0, 100);

	//砂化

	p->bar_shape_sand = _create_label_bar_vert(ct, MLK_TR(TRID_SHAPE_SAND), 0,
		WID_BAR_SHAPE_SAND, 0, 0, 100);

	//角度のベース

	p->bar_angle_base = _create_label_bar_vert(ct, MLK_TR(TRID_ANGLE_BASE), 0,
		WID_BAR_ANGLE_BASE, 0, 0, 359);

	//ランダム回転

	p->bar_angle_rand = _create_label_bar_vert(ct, MLK_TR(TRID_ANGLE_RAND), 0,
		WID_BAR_ANGLE_RAND, 0, 0, 180);

	//進行方向に回転

	p->ck_rotate_dir = mCheckButtonCreate(ct, WID_CK_ROTATE_DIR, 0, MLK_MAKE32_4(0,2,0,0), 0,
		MLK_TR(TRID_ROTATE_DIR), 0);
}

/* 筆圧項目 */

static void _create_widget_pressure(_topct *p,mWidget *cttop)
{
	mWidget *ct;

	//mExpander

	ct = _create_expander(cttop, WID_EXPAND_PRESSURE, TRID_PRESSURE, CONFIG_PANEL_BRUSHOPT_F_HIDE_PRESSURE);

	//サイズ

	p->bar_pressure_size = _create_label_bar_vert(ct, MLK_TR(TRID_PRESSURE_SIZE), 0,
		WID_BAR_PRESSURE_SIZE, 1, 0, 1000);

	//濃度

	p->bar_pressure_opacity = _create_label_bar_vert(ct, MLK_TR(TRID_PRESSURE_OPACITY), 0,
		WID_BAR_PRESSURE_OPACITY, 1, 0, 1000);

	//共通の筆圧

	p->ck_pressure_common = mCheckButtonCreate(ct, WID_CK_PRESSURE_COMMON, 0, MLK_MAKE32_4(0,2,0,0),
		0, MLK_TR(TRID_PRESSURE_COMMON), 0);

	//編集

	mButtonCreate(ct, WID_BTT_PRESSURE_EDIT, 0, MLK_MAKE32_4(0,2,0,0), 0, MLK_TR(TRID_PRESSURE_EDIT));
}

/* 項目のウィジェット作成 */

void _create_widget(_topct *p,mWidget *parent)
{
	_create_widget_main(p, parent);
	_create_widget_othres(p, parent);
	_create_widget_pressure(p, parent);
	_create_widget_water(p, parent);
	_create_widget_shape(p, parent);
}


