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
 * キャンバス新規作成ダイアログ
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_groupbox.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_combobox.h"
#include "mlk_lineedit.h"
#include "mlk_listview.h"
#include "mlk_tab.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_columnitem.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw_sub.h"

#include "widget_func.h"

#include "trid.h"


//------------------------

typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[3];
	mComboBox *cb_unit,
		*cb_layer;
	mCheckButton *ck_col[2];
	mColorButton *colbtt;
	mLabel *label_size;
	mListView *list;
	mWidget *btt_add,
		*btt_del;

	int tabno;
}_dialog;

enum
{
	TABNO_RECENT,
	TABNO_RECORD,
	TABNO_DEFINED
};

enum
{
	EDITNO_WIDTH,
	EDITNO_HEIGHT,
	EDITNO_DPI
};

enum
{
	WID_EDIT_WIDTH = 100,
	WID_EDIT_HEIGHT,
	WID_EDIT_DPI,
	WID_CB_UNIT,
	WID_COLBTT_BKGND,
	WID_CB_LAYER,
	WID_CK_8BIT,
	WID_CK_16BIT,
	WID_TAB,
	WID_LIST,
	WID_BTT_LISTADD,
	WID_BTT_LISTDEL,
	WID_BTT_SETINIT
};

enum
{
	TRID_TITLE,
	TRID_INIT_LAYER,
	TRID_SET_INIT,
	TRID_MES_LIMIT,

	TRID_TAB_RECENT = 100,
	TRID_TAB_RECORD,
	TRID_TAB_DEFINED
};

//単位
enum
{
	_UNIT_PX,
	_UNIT_MM,
	_UNIT_CM,
	_UNIT_INCH
};

//規定 (mm)
static const uint32_t g_defined[2][7] = {
	//A1-7
	{
		(594 << 16) | 841, (420 << 16) | 594, (297 << 16) | 420,
		(210 << 16) | 297, (148 << 16) | 210, (105 << 16) | 148,
		(74 << 16) | 105
	},
	//B1-7
	{
		(728 << 16) | 1030, (515 << 16) | 728, (364 << 16) | 515,
		(257 << 16) | 364, (182 << 16) | 257, (128 << 16) | 182,
		(91 << 16) | 128
	}
};

//------------------------



//========================
// リストにデータセット
//========================


/* 履歴/登録リストセット (param = インデックス番号) */

static void _set_list_recent_record(mListView *list,ConfigNewCanvas *buf)
{
	mStr str = MSTR_INIT;
	int i;
	const char *unit_name[] = {"mm","cm","inch"};

	for(i = 0; i < CONFIG_NEWCANVAS_NUM; i++, buf++)
	{
		if(buf->bit == 0) break;
	
		if(buf->unit == _UNIT_PX)
			//px
			mStrSetFormat(&str, "%d x %d px/%d dpi", buf->w, buf->h, buf->dpi);
		else
		{
			//mm,cm,inch
			
			mStrSetFormat(&str, "%d.%02d x %d.%02d %s/%d dpi",
				buf->w / 100, buf->w % 100,
				buf->h / 100, buf->h % 100,
				unit_name[buf->unit - 1], buf->dpi);
		}

		mStrAppendFormat(&str, "/%dbit", buf->bit);

		mListViewAddItem_text_copy_param(list, str.buf, i);
	}

	mStrFree(&str);
}

/* 規定リストセット */

static void _set_list_defined(mListView *list)
{
	int i,j;
	mStr str = MSTR_INIT;

	for(i = 0; i < 2; i++)
	{
		for(j = 0; j < 7; j++)
		{
			mStrSetFormat(&str, "%c%d : %d x %d mm",
				'A' + i, j + 1,
				g_defined[i][j] >> 16, g_defined[i][j] & 0xffff);

			mListViewAddItem_text_copy_param(list, str.buf, g_defined[i][j]);
		}
	}

	mStrFree(&str);
}

/* タブ番号からリストセット */

static void _set_list(_dialog *p,int no)
{
	mListView *list = p->list;

	mListViewDeleteAllItem(list);

	if(no == TABNO_RECENT)
		_set_list_recent_record(list, APPCONF->newcanvas_recent);
	else if(no == TABNO_RECORD)
		_set_list_recent_record(list, APPCONF->newcanvas_record);
	else
		_set_list_defined(list);
}


//========================
// sub
//========================


/* 幅 or 高さの値セット */

static void _set_width_height(_dialog *p,int wno,int val,int unit)
{
	if(unit == _UNIT_PX)
		mLineEditSetInt(p->edit[wno], val);
	else
		mLineEditSetDouble(p->edit[wno], val * 0.01, 2);
}

/* ConfigNewCanvas の値をウィジェットにセット */

static void _set_from_data(_dialog *p,ConfigNewCanvas *dat)
{
	mComboBoxSetSelItem_atIndex(p->cb_unit, dat->unit);

	_set_width_height(p, EDITNO_WIDTH, dat->w, dat->unit);
	_set_width_height(p, EDITNO_HEIGHT, dat->h, dat->unit);

	mLineEditSetInt(p->edit[EDITNO_DPI], dat->dpi);

	mCheckButtonSetState(p->ck_col[(dat->bit == 16)], 1);
}

/* リストボタンの有効/無効 */

static void _enable_list_button(_dialog *p)
{
	mWidgetEnable(p->btt_add, (p->tabno == TABNO_RECORD));
	mWidgetEnable(p->btt_del, (p->tabno != TABNO_DEFINED));
}

/* 幅 or 高さの値取得 */

static int _get_width_height(_dialog *p,int wno,int unit)
{
	int n;

	if(unit == _UNIT_PX)
		n = mLineEditGetInt(p->edit[wno]);
	else
		n = mLineEditGetDouble(p->edit[wno]) * 100 + 0.5;

	return (n < 1)? 1: n;
}

/* 各ウィジェットから ConfigNewCanvas 取得 */

static void _get_data_from_widget(_dialog *p,ConfigNewCanvas *dat)
{
	int unit;

	unit = mComboBoxGetItemParam(p->cb_unit, -1);

	dat->w = _get_width_height(p, EDITNO_WIDTH, unit);
	dat->h = _get_width_height(p, EDITNO_HEIGHT, unit);
	dat->unit = unit;
	dat->dpi = mLineEditGetNum(p->edit[EDITNO_DPI]);
	dat->bit = mCheckButtonIsChecked(p->ck_col[0])? 8: 16;
}

/* ConfigNewCanvas から、幅・高さの px サイズを取得
 *
 * limit: 最大値制限を行う */

static void _get_pxsize(mSize *size,const ConfigNewCanvas *dat,mlkbool limit)
{
	double dw,dh;
	int w,h;

	w = dat->w;
	h = dat->h;

	//mm/cm/inch

	if(dat->unit != _UNIT_PX)
	{
		dw = w * 0.01 * dat->dpi;
		dh = h * 0.01 * dat->dpi;

		switch(dat->unit)
		{
			//mm
			case _UNIT_MM:
				dw /= 25.4;
				dh /= 25.4;
				break;
			//cm
			case _UNIT_CM:
				dw /= 2.54;
				dh /= 2.54;
				break;
		}

		w = (int)(dw + 0.5);
		h = (int)(dh + 0.5);

		if(w < 1) w = 1;
		if(h < 1) h = 1;
	}

	if(limit)
	{
		if(w > IMAGE_SIZE_MAX) w = IMAGE_SIZE_MAX;
		if(h > IMAGE_SIZE_MAX) h = IMAGE_SIZE_MAX;
	}

	size->w = w;
	size->h = h;
}

/* mLabel に px サイズをセット */

static void _set_label_pxsize(_dialog *p)
{
	ConfigNewCanvas dat;
	mSize size;
	char m[32];

	_get_data_from_widget(p, &dat);
	_get_pxsize(&size, &dat, FALSE);

	snprintf(m, 32, "%d x %d px", size.w, size.h);

	mLabelSetText(p->label_size, m);
}

/* ConfigNewCanvas を履歴の先頭に追加 */

static void _set_config_recent(const ConfigNewCanvas *src)
{
	ConfigNewCanvas *buf = APPCONF->newcanvas_recent;
	int i,del;

	del = CONFIG_NEWCANVAS_NUM - 1;

	//同じ値が存在するか

	for(i = 0; i < CONFIG_NEWCANVAS_NUM; i++)
	{
		if(buf[i].bit == 0) break;
	
		if(src->w == buf[i].w
			&& src->h == buf[i].h
			&& src->unit == buf[i].unit
			&& src->dpi == buf[i].dpi
			&& src->bit == buf[i].bit)
		{
			del = i;
			break;
		}
	}

	//挿入位置を空ける

	for(i = del; i > 0; i--)
		buf[i] = buf[i - 1];

	//先頭にセット

	buf[0] = *src;
}

/* 登録リストに追加 */

static void _list_add(_dialog *p)
{
	ConfigNewCanvas *buf = APPCONF->newcanvas_record;
	int i;

	//空き位置取得

	for(i = 0; i < CONFIG_NEWCANVAS_NUM; i++)
	{
		if(buf[i].bit == 0) break;
	}

	if(i == CONFIG_NEWCANVAS_NUM) return;

	//セット

	_get_data_from_widget(p, buf + i);

	//リスト

	_set_list(p, p->tabno);
}

/* リストから削除 (履歴/登録) */

static void _list_del(_dialog *p)
{
	mColumnItem *item;
	ConfigNewCanvas *buf;
	int i,no;

	item = mListViewGetFocusItem(p->list);
	if(!item) return;

	no = item->param;

	//データ削除

	buf = (p->tabno == TABNO_RECENT)? APPCONF->newcanvas_recent: APPCONF->newcanvas_record;

	for(i = no; i < CONFIG_NEWCANVAS_NUM - 1; i++)
		buf[i] = buf[i + 1];

	buf[CONFIG_NEWCANVAS_NUM - 1].bit = 0;

	//リスト再セット (param の再セットが必要なため)

	_set_list(p, p->tabno);
}

/* 起動時のサイズとしてセット */

static void _set_initsize(_dialog *p)
{
	mSize size;

	_get_data_from_widget(p, &APPCONF->newcanvas_init);
	_get_pxsize(&size, &APPCONF->newcanvas_init, TRUE);

	APPCONF->newcanvas_init.w = size.w;
	APPCONF->newcanvas_init.h = size.h;
	APPCONF->newcanvas_init.unit = 0;
}


//========================
// イベント
//========================


/* OK 終了時 */

static void _end_ok(_dialog *p)
{
	ConfigNewCanvas dat;
	mSize size;

	_get_data_from_widget(p, &dat);
	_get_pxsize(&size, &dat, FALSE);

	//サイズ制限チェック

	if(size.w > IMAGE_SIZE_MAX || size.h > IMAGE_SIZE_MAX)
	{
		mStr str = MSTR_INIT;

		mStrSetFormat(&str, "%s\nMAX = %d px", MLK_TR2(TRGROUP_DLG_NEWCANVAS, TRID_MES_LIMIT), IMAGE_SIZE_MAX);
	
		mMessageBoxOK(MLK_WINDOW(p), str.buf);

		mStrFree(&str);

		return;
	}

	//履歴の先頭にセット

	_set_config_recent(&dat);

	mDialogEnd(MLK_DIALOG(p), TRUE);
}

/* リスト項目選択時 */

static void _change_list_sel(_dialog *p,mColumnItem *item)
{
	int32_t val = item->param;

	switch(p->tabno)
	{
		//履歴
		case TABNO_RECENT:
			_set_from_data(p, APPCONF->newcanvas_recent + val);
			break;
		//登録
		case TABNO_RECORD:
			_set_from_data(p, APPCONF->newcanvas_record + val);
			break;
		//規定 (mm)
		default:
			mComboBoxSetSelItem_atIndex(p->cb_unit, _UNIT_MM);

			mLineEditSetInt(p->edit[EDITNO_WIDTH], val >> 16);
			mLineEditSetInt(p->edit[EDITNO_HEIGHT], val & 0xffff);
			break;
	}

	//px

	_set_label_pxsize(p);
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;
	int ntype;

	if(ev->type == MEVENT_NOTIFY)
	{
		ntype = ev->notify.notify_type;
		
		switch(ev->notify.id)
		{
			//幅/高さ/DPI
			case WID_EDIT_WIDTH:
			case WID_EDIT_HEIGHT:
			case WID_EDIT_DPI:
				if(ntype == MLINEEDIT_N_CHANGE)
					_set_label_pxsize(p);
				break;
			//単位
			case WID_CB_UNIT:
				if(ntype == MCOMBOBOX_N_CHANGE_SEL)
					_set_label_pxsize(p);
				break;
		
			//リスト項目選択時
			case WID_LIST:
				if(ntype == MLISTVIEW_N_CHANGE_FOCUS
					|| ntype == MLISTVIEW_N_CLICK_ON_FOCUS)
					_change_list_sel(p, (mColumnItem *)ev->notify.param1);
				break;
			//タブ変更時
			case WID_TAB:
				if(ntype == MTAB_N_CHANGE_SEL)
				{
					p->tabno = ev->notify.param1;

					_enable_list_button(p);

					_set_list(p, p->tabno);
				}
				break;
			//追加
			case WID_BTT_LISTADD:
				_list_add(p);
				break;
			//削除
			case WID_BTT_LISTDEL:
				_list_del(p);
				break;
			//起動時のサイズにセット
			case WID_BTT_SETINIT:
				_set_initsize(p);
				break;
		
			//OK
			case MLK_WID_OK:
				_end_ok(p);
				break;
			//CANCEL
			case MLK_WID_CANCEL:
				mDialogEnd(MLK_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventDefault(wg, ev);
}


//========================
// 作成
//========================


/* label + mLineEdit 作成 */

static void _create_edit(_dialog *p,mWidget *parent,const char *label,int no,int id)
{
	mLineEdit *le;
	int fdpi = (no == EDITNO_DPI);

	mLabelCreate(parent, MLF_MIDDLE, 0, 0, label);

	p->edit[no] = le = mLineEditCreate(parent, id, 0, 0,
		MLINEEDIT_S_NOTIFY_CHANGE | (fdpi? MLINEEDIT_S_SPIN: 0));

	mLineEditSetWidth_textlen(le, (fdpi)? 6: 9);

	if(fdpi)
		mLineEditSetNumStatus(le, 1, 9999, 0);
}

/* ウィジェット作成 */

static void _create_widgets(_dialog *p)
{
	mWidget *cttop,*ct1,*ct2,*ct3;
	mComboBox *cb;
	mTab *tab;
	int i;

	cttop = mContainerCreateHorz(MLK_WIDGET(p), 12, MLF_EXPAND_WH, 0);
	
	//======== 左側

	ct1 = mContainerCreateVert(cttop, 8, MLF_MIDDLE, 0);

	//---- サイズ

	ct2 = (mWidget *)mGroupBoxCreate(ct1, MLF_EXPAND_W, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE));

	ct3 = mContainerCreateGrid(ct2, 3, 6, 7, 0, 0);

	//単位
	
	p->cb_unit = cb = widget_createLabelCombo(ct3, MLK_TR2(TRGROUP_WORD, TRID_WORD_UNIT), WID_CB_UNIT);

	mComboBoxAddItems_sepnull(cb, "px\0mm\0cm\0inch\0", 0);
	mComboBoxSetAutoWidth(cb);

	mWidgetNew(ct3, 0);

	//幅

	_create_edit(p, ct3, MLK_TR2(TRGROUP_WORD, TRID_WORD_WIDTH), EDITNO_WIDTH, WID_EDIT_WIDTH);

	mWidgetNew(ct3, 0);

	mWidgetSetFocus(MLK_WIDGET(p->edit[EDITNO_WIDTH]));

	//高さ

	_create_edit(p, ct3, MLK_TR2(TRGROUP_WORD, TRID_WORD_HEIGHT), EDITNO_HEIGHT, WID_EDIT_HEIGHT);

	mWidgetNew(ct3, 0);

	//解像度

	_create_edit(p, ct3, MLK_TR2(TRGROUP_WORD, TRID_WORD_RESOLUTION), EDITNO_DPI, WID_EDIT_DPI);

	mLabelCreate(ct3, MLF_MIDDLE, 0, 0, "dpi");

	//px サイズ

	p->label_size = mLabelCreate(ct2, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0),
		MLABEL_S_BORDER | MLABEL_S_COPYTEXT, NULL);

	//---- ほか

	ct2 = (mWidget *)mGroupBoxCreate(ct1, MLF_EXPAND_W, MLK_MAKE32_4(0,4,0,0), 0, NULL);

	mContainerSetType_grid(MLK_CONTAINER(ct2), 2, 6, 8);

	//カラー

	mLabelCreate(ct2, MLF_MIDDLE, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_COLOR_BITS));

	ct3 = mContainerCreateHorz(ct2, 4, 0, 0);
	
	p->ck_col[0] = mCheckButtonCreate(ct3, WID_CK_8BIT, 0, 0, MCHECKBUTTON_S_RADIO, "8bit", 1);
	p->ck_col[1] = mCheckButtonCreate(ct3, WID_CK_16BIT, 0, 0, MCHECKBUTTON_S_RADIO, "16bit", 0);

	//初期レイヤ

	p->cb_layer = cb = widget_createLabelCombo(ct2, MLK_TR(TRID_INIT_LAYER), WID_CB_LAYER);

	MLK_TRGROUP(TRGROUP_LAYER_TYPE);
	mComboBoxAddItems_tr(cb, 0, LAYERTYPE_NORMAL_NUM, 0);
	MLK_TRGROUP(TRGROUP_DLG_NEWCANVAS);
	
	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, 0);

	//背景色

	p->colbtt = widget_createLabelColorButton(ct2,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_BKGND_COLOR), WID_COLBTT_BKGND, 0xffffff);

	//======== 右側

	ct1 = mContainerCreateVert(cttop, 3, MLF_EXPAND_WH, 0);

	//タブ

	tab = mTabCreate(ct1, WID_TAB, MLF_EXPAND_W, 0, MTAB_S_TOP | MTAB_S_HAVE_SEP);

	for(i = 0; i < 3; i++)
		mTabAddItem_text_static(tab, MLK_TR(TRID_TAB_RECENT + i));

	mTabSetSel_atIndex(tab, TABNO_RECENT);

	//リスト

	p->list = mListViewCreate(ct1, WID_LIST, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->list), 17, 12);

	//ボタン

	ct2 = mContainerCreateHorz(ct1, 3, MLF_EXPAND_W, 0);

	p->btt_add = (mWidget *)mButtonCreate(ct2, WID_BTT_LISTADD, MLF_EXPAND_X | MLF_RIGHT, 0, 0,
		MLK_TR2(TRGROUP_LISTEDIT, TRID_LISTEDIT_ADD));

	p->btt_del = (mWidget *)mButtonCreate(ct2, WID_BTT_LISTDEL, 0, 0, 0,
		MLK_TR2(TRGROUP_LISTEDIT, TRID_LISTEDIT_DEL));

	//====== 下ボタン

	ct1 = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0));

	mButtonCreate(ct1, WID_BTT_SETINIT, MLF_EXPAND_X, 0, 0, MLK_TR(TRID_SET_INIT));

	mContainerAddButtons_okcancel(ct1);
}

/* 作成 */

static _dialog *_create_dlg(mWindow *parent)
{
	_dialog *p;
	ConfigNewCanvas dat;

	MLK_TRGROUP(TRGROUP_DLG_NEWCANVAS);

	//作成
	
	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);

	if(!p) return NULL;
	
	//ウィジェット

	_create_widgets(p);

	//--------- 初期化

	//最新の履歴から値セット

	if(APPCONF->newcanvas_recent[0].bit != 0)
		dat = APPCONF->newcanvas_recent[0];
	else
	{
		dat.w = dat.h = 500;
		dat.dpi = 300;
		dat.unit = 0;
		dat.bit = 8;
	}
	
	_set_from_data(p, &dat);

	//label

	_set_label_pxsize(p);

	//リスト

	_set_list(p, TABNO_RECENT);

	//ボタン無効

	_enable_list_button(p);

	return p;
}


//=====================


/** キャンバス新規作成ダイアログ実行 */

mlkbool NewCanvasDialog_run(mWindow *parent,NewCanvasValue *dst)
{
	_dialog *p;
	mlkbool ret;

	p = _create_dlg(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		_get_pxsize(&dst->size, APPCONF->newcanvas_recent, TRUE);

		dst->dpi = APPCONF->newcanvas_recent[0].dpi;
		dst->bit = APPCONF->newcanvas_recent[0].bit;

		dst->imgbkcol = mColorButtonGetColor(p->colbtt);

		dst->layertype = mComboBoxGetItemParam(p->cb_layer, -1);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

