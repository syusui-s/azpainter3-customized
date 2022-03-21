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
 * グリッド設定ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_groupbox.h"
#include "mlk_arrowbutton.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_combobox.h"
#include "mlk_lineedit.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_str.h"
#include "mlk_util.h"

#include "def_config.h"

#include "trid.h"


//------------------------

typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[2][3];
	mColorButton *colbtt[2];
	mCheckButton *ck_pxgrid;
	mComboBox *cb_pxgrid;
	mWidget *menubtt;
}_dialog;

enum
{
	TRID_TITLE,
	TRID_GRID,
	TRID_SPLIT,
	TRID_SPLIT_H,
	TRID_SPLIT_V,
	TRID_PXGRID,
	TRID_PXGRID_TEXT
};

enum
{
	WID_BTT_RECENT = 100
};

#define _GRID_WIDTH_MIN  2
#define _GRID_WIDTH_MAX  10000
#define _GRID_SPLIT_MAX  10

//------------------------


/* 履歴メニュー */

static void _run_menu(_dialog *p)
{
	mMenu *menu;
	mMenuItem *mi;
	mStr str = MSTR_INIT;
	mBox box;
	int i;
	uint32_t val;

	if(APPCONF->grid.recent[0] == 0) return;

	//---- メニュー

	menu = mMenuNew();

	for(i = 0; i < CONFIG_GRID_RECENT_NUM; i++)
	{
		val = APPCONF->grid.recent[i];
		if(!val) break;
		
		mStrSetFormat(&str, "%d x %d", val >> 16, val & 0xffff);

		mMenuAppendText_copy(menu, i, str.buf, str.len);
	}

	mStrFree(&str);

	mWidgetGetBox_rel(p->menubtt, &box);

	mi = mMenuPopup(menu, p->menubtt, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);
	
	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(i == -1) return;

	//----- セット

	val = APPCONF->grid.recent[i];

	mLineEditSetNum(p->edit[0][0], val >> 16);
	mLineEditSetNum(p->edit[0][1], val & 0xffff);
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == WID_BTT_RECENT)
	{
		_run_menu((_dialog *)wg);
		return 1;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//=====================


/* label + mLineEdit + (spacer) 作成 */

static mLineEdit *_create_edit(_dialog *p,mGroupBox *gb,const char *label,
	int min,int max,int val,mlkbool spacer)
{
	mLineEdit *le;

	mLabelCreate(MLK_WIDGET(gb), MLF_MIDDLE, 0, 0, label);

	le = mLineEditCreate(MLK_WIDGET(gb), 0, MLF_MIDDLE, 0, MLINEEDIT_S_SPIN);

	mLineEditSetWidth_textlen(le, 6);
	mLineEditSetNumStatus(le, min, max, 0);
	mLineEditSetNum(le, val);

	if(spacer) mWidgetNew(MLK_WIDGET(gb), 0);

	return le;
}

/* label + mColorButton 作成 */

static mColorButton *_create_colbtt(_dialog *p,mGroupBox *gb,const char *label,uint32_t col)
{
	mLabelCreate(MLK_WIDGET(gb), MLF_MIDDLE, 0, 0, label);

	return mColorButtonCreate(MLK_WIDGET(gb), 0, 0, 0, MCOLORBUTTON_S_DIALOG, col);
}

/* 1段目 (グリッド/分割線) 作成 */

static void _create_gridsplit(_dialog *p)
{
	mWidget *ct;
	mGroupBox *gb;
	const char *txt_color,*txt_density;

	txt_color = MLK_TR2(TRGROUP_WORD, TRID_WORD_COLOR);
	txt_density = MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY);

	ct = mContainerCreateHorz(MLK_WIDGET(p), 8, 0, 0);

	//---- グリッド

	gb = mGroupBoxCreate(ct, 0, 0, 0, MLK_TR(TRID_GRID));

	mContainerSetType_grid(MLK_CONTAINER(gb), 3, 6, 7);

	//幅

	p->edit[0][0] = _create_edit(p, gb, MLK_TR2(TRGROUP_WORD, TRID_WORD_WIDTH),
		_GRID_WIDTH_MIN, _GRID_WIDTH_MAX, APPCONF->grid.gridw, TRUE);

	//高さ
	
	p->edit[0][1] = _create_edit(p, gb, MLK_TR2(TRGROUP_WORD, TRID_WORD_HEIGHT),
		_GRID_WIDTH_MIN, _GRID_WIDTH_MAX, APPCONF->grid.gridh, FALSE);

	p->menubtt = (mWidget *)mArrowButtonCreate(MLK_WIDGET(gb),
		WID_BTT_RECENT, MLF_MIDDLE, 0, MARROWBUTTON_S_DOWN | MARROWBUTTON_S_FONTSIZE);

	//色

	p->colbtt[0] = _create_colbtt(p, gb, txt_color, APPCONF->grid.col_grid);

	mWidgetNew(MLK_WIDGET(gb), 0);

	//濃度

	p->edit[0][2] = _create_edit(p, gb, txt_density, 1, 100,
		(int)((APPCONF->grid.col_grid >> 24) / 128.0 * 100 + 0.5), TRUE);

	//---- 分割線

	gb = mGroupBoxCreate(ct, 0, 0, 0, MLK_TR(TRID_SPLIT));

	mContainerSetType_grid(MLK_CONTAINER(gb), 2, 6, 7);

	//横分割数

	p->edit[1][0] = _create_edit(p, gb, MLK_TR(TRID_SPLIT_H), 2, _GRID_SPLIT_MAX, APPCONF->grid.splith, FALSE);

	//縦分割数
	
	p->edit[1][1] = _create_edit(p, gb, MLK_TR(TRID_SPLIT_V), 2, _GRID_SPLIT_MAX, APPCONF->grid.splitv, FALSE);

	//色

	p->colbtt[1] = _create_colbtt(p, gb, txt_color, APPCONF->grid.col_split);

	//濃度

	p->edit[1][2] = _create_edit(p, gb, txt_density, 1, 100,
		(int)((APPCONF->grid.col_split >> 24) / 128.0 * 100 + 0.5), FALSE);
}

/* ダイアログ作成 */

static _dialog *_create_dlg(mWindow *parent)
{
	_dialog *p;
	mWidget *ct;
	mComboBox *cb;
	int i;
	mStr str = MSTR_INIT;
	const char *txtformat;

	//作成
	
	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = _event_handle;

	//

	MLK_TRGROUP(TRGROUP_DLG_GRIDOPT);

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR(TRID_TITLE));

	//---- 1段目 (mGroupBox x 2)

	_create_gridsplit(p);

	//---- 2段目 (1pxグリッド)

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, 0, MLK_MAKE32_4(0,10,0,0));

	p->ck_pxgrid = mCheckButtonCreate(ct, 0, MLF_MIDDLE, 0, 0, MLK_TR(TRID_PXGRID),
		APPCONF->grid.pxgrid_zoom & 0x8000);

	//mComboBox

	p->cb_pxgrid = cb = mComboBoxCreate(ct, 0, 0, 0, 0);

	txtformat = MLK_TR(TRID_PXGRID_TEXT);

	for(i = 4; i <= 16; i++)
	{
		mStrSetFormat(&str, txtformat, i * 100);
		mComboBoxAddItem_copy(cb, str.buf, i);
	}

	mStrFree(&str);

	i = (APPCONF->grid.pxgrid_zoom & 0xff) - 4;
	if(i < 0) i = 0;

	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, i);

	//---- 3段目 (ボタン)

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** グリッド設定ダイアログ実行 */

mlkbool GridOptDialog_run(mWindow *parent)
{
	_dialog *p;
	mlkbool ret;

	p = _create_dlg(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		//グリッド
		
		APPCONF->grid.gridw = mLineEditGetNum(p->edit[0][0]);
		APPCONF->grid.gridh = mLineEditGetNum(p->edit[0][1]);
		APPCONF->grid.col_grid = mColorButtonGetColor(p->colbtt[0]);
		APPCONF->grid.col_grid |= (int)(mLineEditGetNum(p->edit[0][2]) / 100.0 * 128.0 + 0.5) << 24;

		//分割線

		APPCONF->grid.splith = mLineEditGetNum(p->edit[1][0]);
		APPCONF->grid.splitv = mLineEditGetNum(p->edit[1][1]);
		APPCONF->grid.col_split = mColorButtonGetColor(p->colbtt[1]);
		APPCONF->grid.col_split |= (int)(mLineEditGetNum(p->edit[1][2]) / 100.0 * 128.0 + 0.5) << 24;

		//1pxグリッド

		APPCONF->grid.pxgrid_zoom = (mCheckButtonIsChecked(p->ck_pxgrid))? 0x8000: 0;
		APPCONF->grid.pxgrid_zoom |= mComboBoxGetItemParam(p->cb_pxgrid, -1);

		//履歴に追加

		mAddRecentVal_32bit(APPCONF->grid.recent, CONFIG_GRID_RECENT_NUM,
			((uint32_t)APPCONF->grid.gridw << 16) | APPCONF->grid.gridh, 0);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}
