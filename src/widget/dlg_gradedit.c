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

/***************************************
 * グラデーション編集ダイアログ
 ***************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_groupbox.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_arrowbutton.h"
#include "mlk_lineedit.h"
#include "mlk_sliderbar.h"
#include "mlk_menu.h"
#include "mlk_event.h"
#include "mlk_string.h"

#include "gradation_list.h"
#include "widget_func.h"

#include "pv_gradedit.h"

#include "trid.h"


//--------------------

typedef struct
{
	MLK_DIALOG_DEF
	
	GradItem *item;	//編集対象のアイテム

	GradEditBar *editwg[2];
	mLineEdit *edit_pos[2],
		*edit_opa;
	mSliderBar *slider_opa;
	mColorButton *colbtt;
	mCheckButton *ck_coltype[3],
		*ck_flag[2];
}_dialog;

//--------------------

enum
{
	WID_EDIT_COL_POS = 100,
	WID_EDIT_A_POS,
	WID_CK_COL_DRAW,
	WID_CK_COL_BKGND,
	WID_CK_COL_SPEC,
	WID_COLBTT,
	WID_SLIDER_A,
	WID_EDIT_A_VAL,
	WID_CK_LOOP,
	WID_CK_SINGLE_COL,

	WID_EDITWG_COL = 200,
	WID_EDITWG_A,
	WID_BTT_COL_MOVELEFT,
	WID_BTT_A_MOVELEFT,
	WID_BTT_COL_MOVERIGHT,
	WID_BTT_A_MOVERIGHT,
	WID_MENUBTT_COL,
	WID_MENUBTT_A
};

enum
{
	TRID_TITLE,
	TRID_POSITION,
	TRID_DRAWCOL,
	TRID_BKGNDCOL,
	TRID_SPECCOL,
	TRID_VALUE,
	TRID_LOOP,
	TRID_SINGLECOL,
	TRID_HELP,

	TRID_MENU_DELETE = 100,
	TRID_MENU_DIVIDE,
	TRID_MENU_MOVE_CENTER,
	TRID_MENU_ALL_EVEN,
	TRID_MENU_REVERSE
};

//メニュー
static const uint16_t g_menu_id[] = {
	TRID_MENU_DELETE, MMENU_ARRAY16_SEP,
	TRID_MENU_DIVIDE, MMENU_ARRAY16_SEP,
	TRID_MENU_MOVE_CENTER, TRID_MENU_ALL_EVEN, TRID_MENU_REVERSE,
	MMENU_ARRAY16_END
};

//--------------------



//==========================
// sub
//==========================


/* GradEditBar の更新 */

static void _update_editbar(_dialog *p,int no)
{
	mWidgetRedraw(MLK_WIDGET(p->editwg[no]));
}

/* カレントポイントの情報セット */

static void _set_curinfo(_dialog *p,int no)
{
	int n;
	uint32_t col;

	//位置

	mLineEditSetNum(p->edit_pos[no],
		(int)((double)GradEditBar_getCurrentPos(p->editwg[no]) / (1 << GRAD_DAT_POS_BIT) * 1000 + 0.5));

	//値

	if(no == 0)
	{
		//RGB
		
		n = GradEditBar_getCurrentColor(p->editwg[no], &col);

		mCheckButtonSetState(p->ck_coltype[n], TRUE);

		mColorButtonSetColor(p->colbtt, col);
		mWidgetEnable(MLK_WIDGET(p->colbtt), (n == 2));
	}
	else
	{
		//A
		
		n = GradEditBar_getCurrentOpacity(p->editwg[no]);

		mSliderBarSetPos(p->slider_opa, n);
		mLineEditSetNum(p->edit_opa, n);
	}
}

/* メニュー実行 */

static void _run_menu(_dialog *p,int no,mWidget *bttwg)
{
	GradEditBar *editwg;
	mMenu *menu,*sub;
	mMenuItem *mi;
	int i,n;
	char m[4];
	mBox box;

	editwg = p->editwg[no];

	//------- メニュー作成

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menu_id);

	//位置が端の場合、無効

	n = GradEditBar_getCurrentPos(editwg);

	if(n == 0 || n == GRAD_DAT_POS_VAL)
	{
		mMenuSetItemEnable(menu, TRID_MENU_DELETE, 0);
		mMenuSetItemEnable(menu, TRID_MENU_MOVE_CENTER, 0);
	}

	//分割数

	n = GradEditBar_getMaxSplitNum_toRight(editwg);

	if(n < 2)
		mMenuSetItemEnable(menu, TRID_MENU_DIVIDE, FALSE);
	else
	{
		sub = mMenuNew();

		for(i = 2; i <= n && i <= 10; i++)
		{
			mIntToStr(m, i);
			mMenuAppendText_copy(sub, 1000 + i, m, -1);
		}

		mMenuSetItemSubmenu(menu, TRID_MENU_DIVIDE, sub);
	}

	//--------- メニュー実行

	mWidgetGetBox_rel(bttwg, &box);

	mi = mMenuPopup(menu, bttwg, 0, 0, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_FLIP_XY, NULL);

	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(i == -1) return;

	//-------- コマンド実行

	if(i >= 1000)
		//分割
		GradEditBar_splitPoint_toRight(editwg, i - 1000);
	else
	{
		switch(i)
		{
			//カレント削除
			case TRID_MENU_DELETE:
				GradEditBar_deletePoint(editwg, -1);
				break;
			//中間位置へ
			case TRID_MENU_MOVE_CENTER:
				GradEditBar_moveCurrentPos_middle(editwg);
				break;
			//すべて等間隔に
			case TRID_MENU_ALL_EVEN:
				GradEditBar_evenAllPoints(editwg);
				break;
			//反転
			case TRID_MENU_REVERSE:
				GradEditBar_reversePoints(editwg);
				break;
		}
	}
}


//==========================
// イベント
//==========================


/* OK 時、グラデーションデータをセット */

static void _set_graddat(_dialog *p)
{
	int colnum,anum;
	uint8_t flags;

	colnum = GradEditBar_getPointNum(p->editwg[0]);
	anum = GradEditBar_getPointNum(p->editwg[1]);

	//フラグ

	flags = 0;
	if(mCheckButtonIsChecked(p->ck_flag[0])) flags |= GRAD_DAT_F_LOOP;
	if(mCheckButtonIsChecked(p->ck_flag[1])) flags |= GRAD_DAT_F_SINGLE_COL;

	//セット

	GradItem_setData(p->item, flags, colnum, anum,
		GradEditBar_getBuf(p->editwg[0]),
		GradEditBar_getBuf(p->editwg[1]));
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		int id,type,n;

		id = ev->notify.id;
		type = ev->notify.notify_type;
	
		switch(id)
		{
			//グラデ編集 (col)
			case WID_EDITWG_COL:
				_set_curinfo(p, 0);
				break;
			//グラデ編集 (A)
			case WID_EDITWG_A:
				_set_curinfo(p, 1);

				//データに変更があれば、色のグラデーションも再描画
				if(type == GRADEDITBAR_N_MODIFY)
					_update_editbar(p, 0);
				break;

			//mLineEdit (位置)
			case WID_EDIT_COL_POS:
			case WID_EDIT_A_POS:
				if(type == MLINEEDIT_N_CHANGE)
				{
					id -= WID_EDIT_COL_POS;

					if(GradEditBar_setCurrentPos(p->editwg[id], mLineEditGetNum(p->edit_pos[id]))
						&& id == 1)
					{
						//A の値が変化した場合、カラーを再描画
						_update_editbar(p, 0);
					}
				}
				break;
			//mCheckButton:色タイプ
			case WID_CK_COL_DRAW:
			case WID_CK_COL_BKGND:
			case WID_CK_COL_SPEC:
				n = id - WID_CK_COL_DRAW;
				
				mWidgetEnable(MLK_WIDGET(p->colbtt), (n == 2));
			
				GradEditBar_setColorType(p->editwg[0], n);
				break;
			//mColorButton:指定色
			case WID_COLBTT:
				if(type == MCOLORBUTTON_N_PRESS)
					GradEditBar_setColorRGB(p->editwg[0], ev->notify.param1);
				break;
			//mSliderBar:不透明度
			case WID_SLIDER_A:
				if(type == MSLIDERBAR_N_ACTION
					&& (ev->notify.param2 & MSLIDERBAR_ACTION_F_CHANGE_POS))
				{
					GradEditBar_setOpacity(p->editwg[1], ev->notify.param1);

					mLineEditSetNum(p->edit_opa, ev->notify.param1);

					_update_editbar(p, 0);
				}
				break;
			//mLineEdit:不透明度
			case WID_EDIT_A_VAL:
				if(type == MLINEEDIT_N_CHANGE)
				{
					n = mLineEditGetNum(p->edit_opa);

					GradEditBar_setOpacity(p->editwg[1], n);

					mSliderBarSetPos(p->slider_opa, n);

					_update_editbar(p, 0);
				}
				break;

			//(button) カレントポイント選択移動
			case WID_BTT_COL_MOVELEFT:
			case WID_BTT_A_MOVELEFT:
			case WID_BTT_COL_MOVERIGHT:
			case WID_BTT_A_MOVERIGHT:
				id -= WID_BTT_COL_MOVELEFT;
				
				GradEditBar_moveCurrentPoint(p->editwg[id & 1], (id >= 2));
				_set_curinfo(p, id & 1);
				break;
			//メニューボタン
			case WID_MENUBTT_COL:
			case WID_MENUBTT_A:
				_run_menu(p, id - WID_MENUBTT_COL, ev->notify.widget_from);
				break;

			//単色化
			case WID_CK_SINGLE_COL:
				GradEditBar_setSingleColor(p->editwg[0], -1);

				_update_editbar(p, 0);
				break;

			//OK
			case MLK_WID_OK:
				_set_graddat(p);
				mDialogEnd(MLK_DIALOG(wg), TRUE);
				break;
			//キャンセル
			case MLK_WID_CANCEL:
				mDialogEnd(MLK_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventDefault(wg, ev);
}


//==========================
// 作成
//==========================


/* 編集部分のウィジェット作成 */

static void _create_edit_widget(_dialog *p,int no)
{
	mWidget *ct;

	ct = mContainerCreateHorz(MLK_WIDGET(p), 2, MLF_EXPAND_W, 0);

	ct->margin.bottom = 10;

	p->editwg[no] = GradEditBar_new(ct, WID_EDITWG_COL + no, no, p->item->buf);

	//<-

	mArrowButtonCreate(ct, WID_BTT_COL_MOVELEFT + no, 0, MLK_MAKE32_4(8,0,0,0),
		MARROWBUTTON_S_LEFT | MARROWBUTTON_S_FONTSIZE);

	//->
	
	mArrowButtonCreate(ct, WID_BTT_COL_MOVERIGHT + no, 0, 0, MARROWBUTTON_S_RIGHT | MARROWBUTTON_S_FONTSIZE);

	//メニューボタン

	widget_createMenuButton(ct, WID_MENUBTT_COL + no, 0, 0);
}

/* ダイアログ作成 */

static _dialog *_create_dlg(mWindow *parent,GradItem *item)
{
	_dialog *p;
	mWidget *ctg,*ct;
	int i,flag;

	MLK_TRGROUP(TRGROUP_DLG_GRADATION_EDIT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog), MLK_TR(TRID_TITLE), _event_handle);
	if(!p) return NULL;

	p->item = item;

	//------- 編集部分

	_create_edit_widget(p, 0);
	_create_edit_widget(p, 1);

	GradEditBar_setAnotherModeBuf(p->editwg[0], GradEditBar_getBuf(p->editwg[1]));
	GradEditBar_setAnotherModeBuf(p->editwg[1], GradEditBar_getBuf(p->editwg[0]));

	//------- 色

	ctg = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,5,0,0), 0,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_COLOR));

	mContainerSetType_vert(MLK_CONTAINER(ctg), 6);

	//位置

	ct = mContainerCreateHorz(ctg, 7, 0, 0);

	widget_createLabel(ct, MLK_TR(TRID_POSITION));

	p->edit_pos[0] = widget_createEdit_num_change(ct, WID_EDIT_COL_POS, 7, 0, 1000, 1, 0);

	//色

	ct = mContainerCreateHorz(ctg, 4, 0, 0);

	for(i = 0; i < 3; i++)
	{
		p->ck_coltype[i] = mCheckButtonCreate(ct, WID_CK_COL_DRAW + i, MLF_MIDDLE, 0,
			MCHECKBUTTON_S_RADIO, MLK_TR(TRID_DRAWCOL + i), (i == 2));
	}

	p->colbtt = mColorButtonCreate(ct, WID_COLBTT, MLF_MIDDLE, 0, MCOLORBUTTON_S_DIALOG, 0);

	//----- 不透明度

	ctg = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0), 0,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_OPACITY));

	mContainerSetType_horz(MLK_CONTAINER(ctg), 5);

	//位置

	widget_createLabel(ctg, MLK_TR(TRID_POSITION));

	p->edit_pos[1] = widget_createEdit_num_change(ctg, WID_EDIT_A_POS, 7, 0, 1000, 1, 0);

	//値

	widget_createLabel(ctg, MLK_TR(TRID_VALUE));

	p->slider_opa = mSliderBarCreate(ctg, WID_SLIDER_A, MLF_EXPAND_W | MLF_MIDDLE, 0, 0);

	mSliderBarSetRange(p->slider_opa, 0, 1000);

	p->edit_opa = widget_createEdit_num_change(ctg, WID_EDIT_A_VAL, 7, 0, 1000, 1, 0);

	//--------

	//フラグ

	flag = item->buf[0];

	for(i = 0; i < 2; i++)
	{
		p->ck_flag[i] = mCheckButtonCreate(MLK_WIDGET(p), WID_CK_LOOP + i, 0,
			(i == 0)? MLK_MAKE32_4(0,12,0,0): MLK_MAKE32_4(0,3,0,0), 0,
			MLK_TR(TRID_LOOP + i), flag & (1 << i));
	}

	//ヘルプ

	mLabelCreate(MLK_WIDGET(p), MLF_EXPAND_W, MLK_MAKE32_4(0,12,0,0), MLABEL_S_BORDER, MLK_TR(TRID_HELP));

	//OK/キャンセル

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,12,0,0));

	//------ 初期化

	_set_curinfo(p, 0);
	_set_curinfo(p, 1);

	GradEditBar_setSingleColor(p->editwg[0], flag & GRAD_DAT_F_SINGLE_COL);

	return p;
}


//==========================
//
//==========================


/** ダイアログ実行 */

mlkbool GradationEditDlg_run(mWindow *parent,GradItem *item)
{
	_dialog *p;

	p = _create_dlg(parent, item);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

