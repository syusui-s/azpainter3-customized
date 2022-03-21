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
 * レイヤ新規作成/設定ダイアログ
 *****************************************/

#include <string.h>
#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_groupbox.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_arrowbutton.h"
#include "mlk_combobox.h"
#include "mlk_lineedit.h"
#include "mlk_listview.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_columnitem.h"
#include "mlk_sysdlg.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw_sub.h"

#include "blendcolor.h"
#include "sel_material.h"
#include "dialogs.h"
#include "widget_func.h"
#include "appresource.h"

#include "trid.h"
#include "trid_layerdlg.h"


/* テンプレートの更新時、APPCONF->fmodify_layertempl = TRUE にする。 */

//-----------------------

typedef struct
{
	MLK_DIALOG_DEF

	LayerNewOptInfo *info;

	mLineEdit *edit_name,
		*edit_opacity,
		*edit_toneline,
		*edit_toneangle,
		*edit_tonedens;
	mComboBox *cb_type,	//NULL でレイヤ設定
		*cb_blend;
	mCheckButton *ck_tone,
		*ck_tonefix,
		*ck_tone_white;
	mColorButton *colbtt;
	mWidget *wg_linesbtt;
	SelMaterial *seltex;
}_dlg_newopt;

enum
{
	WID_NEWOPT_CB_TYPE = 100,
	WID_NEWOPT_COLBTT,
	WID_NEWOPT_CK_TONE,
	WID_NEWOPT_CK_TONEFIX,
	WID_NEWOPT_CK_TONE_WHITE,
	WID_NEWOPT_BTT_LINES,
	WID_NEWOPT_BTT_TEMPLATE
};

//線数プリセット
static const uint16_t g_lines_preset[] = {
	150,200,250,275,300,325,350,400,425,450,500,550,600,650,700,800, 0
};

static void _templatelist_edit_dialog(mWindow *parent);

//-----------------------


//========================
// sub
//========================


/* タイプによる有効/無効セット
 *
 * type: -1 でフォルダ */

static void _newopt_set_enable(_dlg_newopt *p,int type)
{
	int fimg,ftone,imgtype;

	imgtype = type & 3;
	fimg = (type != -1);
	ftone = (fimg && (imgtype == 1 || imgtype == 3));

	mWidgetEnable(MLK_WIDGET(p->cb_blend), fimg);
	mWidgetEnable(MLK_WIDGET(p->colbtt), (fimg && imgtype >= 1)); //GRAY+TONE の場合、設定可能
	mWidgetEnable(MLK_WIDGET(p->seltex), fimg);

	//トーン化

	mWidgetEnable(MLK_WIDGET(p->ck_tone), ftone);
	mWidgetEnable(MLK_WIDGET(p->edit_toneline), ftone);
	mWidgetEnable(MLK_WIDGET(p->edit_toneangle), ftone);
	mWidgetEnable(MLK_WIDGET(p->ck_tonefix), ftone);
	mWidgetEnable(MLK_WIDGET(p->edit_tonedens), ftone);
	mWidgetEnable(MLK_WIDGET(p->wg_linesbtt), ftone);
	mWidgetEnable(MLK_WIDGET(p->ck_tone_white), ftone);
}

/* 色ボタンが押された時 */

static void _newopt_color(_dlg_newopt *p)
{
	uint32_t col = mColorButtonGetColor(p->colbtt);
	
	if(LayerDialog_color(MLK_WINDOW(p), &col))
		mColorButtonSetColor(p->colbtt, col);
}

/* 線数メニュー */

static void _newopt_menu_lines(_dlg_newopt *p,mWidget *wg)
{
	mMenu *menu;
	mMenuItem *mi;
	const uint16_t *ps;
	char m[16];
	int n;
	mBox box;

	menu = mMenuNew();

	//

	for(ps = g_lines_preset; *ps; ps++)
	{
		snprintf(m, 16, "%d.%d", *ps / 10, *ps % 10);

		mMenuAppendText_copy(menu, *ps, m, -1);
	}

	mMenuAppendSep(menu);
	mMenuAppendText(menu, 0, MLK_TR2(TRGROUP_DLG_LAYER, TRID_LAYERDLG_SET_DEFAULT_LINES));

	//

	mWidgetGetBox_rel(wg, &box);

	mi = mMenuPopup(menu, wg, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	n = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//----------

	if(n == -1) return;

	if(n == 0)
	{
		//デフォルトの線数にセット
		APPCONF->tone_lines_default = mLineEditGetNum(p->edit_toneline);
	}
	else
	{
		//線数の値をセット
		mLineEditSetNum(p->edit_toneline, n);
	}
}

/* テンプレートに追加 */

static void _cmd_add_template(_dlg_newopt *p)
{
	mStr strname = MSTR_INIT,strtex = MSTR_INIT;
	ConfigLayerTemplItem *pi;

	mLineEditGetTextStr(p->edit_name, &strname);

	SelMaterial_getPath(p->seltex, &strtex);

	//リスト追加

	pi = (ConfigLayerTemplItem *)mListAppendNew(&APPCONF->list_layertempl,
		sizeof(ConfigLayerTemplItem) + strname.len + strtex.len + 1);

	if(pi)
	{
		APPCONF->fmodify_layertempl = TRUE;

		//

		if(p->cb_type)
			pi->type = mComboBoxGetItemParam(p->cb_type, -1);
		else
			pi->type = p->info->type;

		pi->blendmode = mComboBoxGetItemParam(p->cb_blend, -1);

		pi->opacity = (int)(mLineEditGetNum(p->edit_opacity) / 100.0 * 128 + 0.5);

		pi->col = mColorButtonGetColor(p->colbtt);

		pi->tone_lines = mLineEditGetNum(p->edit_toneline);

		pi->tone_angle = mLineEditGetNum(p->edit_toneangle);

		if(mCheckButtonIsChecked(p->ck_tonefix))
			pi->tone_density = mLineEditGetNum(p->edit_tonedens);

		if(mCheckButtonIsChecked(p->ck_tone))
			pi->flags |= CONFIG_LAYERTEMPL_F_TONE;
			
		if(mCheckButtonIsChecked(p->ck_tone_white))
			pi->flags |= CONFIG_LAYERTEMPL_F_TONE_WHITE;

		pi->name_len = strname.len;
		pi->texture_len = strtex.len;

		//名前
		if(strname.len)
			memcpy(pi->text, strname.buf, strname.len + 1);

		//テクスチャパス
		if(strtex.len)
			memcpy(pi->text + strname.len + 1, strtex.buf, strtex.len + 1);
	}

	mStrFree(&strname);
	mStrFree(&strtex);
}

/* テンプレートから項目値セット */

static void _newopt_set_template(_dlg_newopt *p,ConfigLayerTemplItem *pl)
{
	int type;

	mLineEditSetText(p->edit_name, pl->text);

	//タイプは、新規作成時のみ

	if(p->cb_type)
	{
		type = (pl->type == 0xff)? -1: pl->type;

		mComboBoxSetSelItem_fromParam(p->cb_type, type);

		_newopt_set_enable(p, type);
	}

	//

	mComboBoxSetSelItem_atIndex(p->cb_blend, pl->blendmode);

	mLineEditSetNum(p->edit_opacity, (int)(pl->opacity / 128.0 * 100 + 0.5));

	mColorButtonSetColor(p->colbtt, pl->col);

	SelMaterial_setPath(p->seltex, pl->text + pl->name_len + 1);

	//トーン

	mCheckButtonSetState(p->ck_tone, pl->flags & CONFIG_LAYERTEMPL_F_TONE);

	mLineEditSetNum(p->edit_toneline, pl->tone_lines);

	mLineEditSetNum(p->edit_toneangle, pl->tone_angle);

	mCheckButtonSetState(p->ck_tonefix, (pl->tone_density != 0));

	if(pl->tone_density)
		mLineEditSetNum(p->edit_tonedens, pl->tone_density);

	mCheckButtonSetState(p->ck_tone_white, pl->flags & CONFIG_LAYERTEMPL_F_TONE_WHITE);
}

/* テンプレートメニュー */

static void _newopt_menu_template(_dlg_newopt *p,mWidget *wg)
{
	mMenu *menu;
	mMenuItem *mi;
	ConfigLayerTemplItem *pl;
	int id;
	mBox box;

	MLK_TRGROUP(TRGROUP_DLG_LAYER);

	menu = mMenuNew();

	mMenuAppendText(menu, 0, MLK_TR(TRID_LAYERDLG_TEMPLATE_ADD));
	mMenuAppendText(menu, 1, MLK_TR(TRID_LAYERDLG_TEMPLATE_EDIT));

	//リスト
	
	mMenuAppendSep(menu);

	MLK_LIST_FOR(APPCONF->list_layertempl, pl, ConfigLayerTemplItem)
	{
		mMenuAppendText_param(menu, 100, pl->text, (intptr_t)pl);
	}

	//

	mWidgetGetBox_rel(wg, &box);

	mi = mMenuPopup(menu, wg, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	if(!mi)
		id = -1;
	else
	{
		id = mMenuItemGetID(mi);
		pl = (ConfigLayerTemplItem *)mMenuItemGetParam1(mi);
	}

	mMenuDestroy(menu);

	//----------

	if(id == -1) return;

	switch(id)
	{
		//テンプレートに追加
		case 0:
			_cmd_add_template(p);
			break;
		//リスト編集
		case 1:
			_templatelist_edit_dialog(MLK_WINDOW(p));
			APPCONF->fmodify_layertempl = TRUE;
			break;
		//各アイテム
		case 100:
			_newopt_set_template(p, pl);
			break;
	}
}

/* イベント */

static int _newopt_event_handle(mWidget *wg,mEvent *ev)
{
	_dlg_newopt *p = (_dlg_newopt *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//タイプ
			case WID_NEWOPT_CB_TYPE:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					_newopt_set_enable(p, ev->notify.param2);
				break;
			//色ボタン
			case WID_NEWOPT_COLBTT:
				if(ev->notify.notify_type == MCOLORBUTTON_N_PRESS)
					_newopt_color(p);
				break;
			//線数ボタン
			case WID_NEWOPT_BTT_LINES:
				_newopt_menu_lines(p, ev->notify.widget_from);
				break;
			//テンプレート
			case WID_NEWOPT_BTT_TEMPLATE:
				_newopt_menu_template(p, ev->notify.widget_from);
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* 初期化 */

static void _newopt_init(_dlg_newopt *p,LayerNewOptInfo *info,mlkbool fnew)
{
	mComboBox *cb;
	int i;

	//タイプ

	if(fnew)
	{
		MLK_TRGROUP(TRGROUP_LAYER_TYPE);

		cb = p->cb_type;
		
		for(i = 0; i < LAYERTYPE_NORMAL_NUM; i++)
			mComboBoxAddItem_static(cb, MLK_TR(i), i);

		//フォルダ
		mComboBoxAddItem_static(cb, MLK_TR(TRID_LAYERTYPE_FOLDER), -1);

		//テキスト
		mComboBoxAddItem_static(cb, MLK_TR(TRID_LAYERTYPE_TEXT_A), 0x80 + 2);
		mComboBoxAddItem_static(cb, MLK_TR(TRID_LAYERTYPE_TEXT_A1), 0x80 + 3);

		mComboBoxSetSelItem_fromParam(cb, info->type);
		mComboBoxSetAutoWidth(cb);
	}

	//合成モード

	MLK_TRGROUP(TRGROUP_BLENDMODE);

	cb = p->cb_blend;

	for(i = 0; i < BLENDMODE_NUM; i++)
		mComboBoxAddItem_static(cb, MLK_TR(i), i);

	mComboBoxSetSelItem_atIndex(cb, info->blendmode);
	mComboBoxSetAutoWidth(cb);

	//有効/無効

	_newopt_set_enable(p, info->type);
}

/* ダイアログ作成 */

static _dlg_newopt *_newopt_create(mWindow *parent,LayerNewOptInfo *info,mlkbool fnew)
{
	_dlg_newopt *p;
	mWidget *ct,*ct2,*ct3;

	p = (_dlg_newopt *)widget_createDialog(parent, sizeof(_dlg_newopt),
		MLK_TR2(TRGROUP_DLG_LAYER, (fnew)? TRID_LAYERDLG_TITLE_NEW: TRID_LAYERDLG_TITLE_OPTION),
		_newopt_event_handle);

	if(!p) return NULL;

	p->info = info;

	//----- 上 (翻訳グループは"単語")

	MLK_TRGROUP(TRGROUP_WORD);

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 7, 6, MLF_EXPAND_W, 0);

	//名前

	widget_createLabel(ct, MLK_TR(TRID_WORD_NAME));

	p->edit_name = mLineEditCreate(ct, 0, MLF_EXPAND_W, 0, 0);

	mLineEditSetText(p->edit_name, info->strname.buf);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->edit_name), 16, 0);

	//タイプ

	if(fnew)
		p->cb_type = widget_createLabelCombo(ct, MLK_TR(TRID_WORD_TYPE), WID_NEWOPT_CB_TYPE);

	//合成モード

	p->cb_blend = widget_createLabelCombo(ct, MLK_TR(TRID_WORD_BLENDMODE), 0);

	//不透明度

	widget_createLabel(ct, MLK_TR(TRID_WORD_OPACITY));

	p->edit_opacity = widget_createEdit_num(ct, 6, 0, 100, 0, (int)(info->opacity / 128.0 * 100 + 0.5));

	//色

	widget_createLabel(ct, MLK_TR(TRID_WORD_COLOR));
	
	p->colbtt = mColorButtonCreate(ct, WID_NEWOPT_COLBTT, MLF_MIDDLE, 0, 0, info->col);

	//テクスチャ

	widget_createLabel(ct, MLK_TR(TRID_WORD_TEXTURE));

	p->seltex = SelMaterial_new(ct, 0, SELMATERIAL_TYPE_TEXTURE);

	SelMaterial_setPath(p->seltex, info->strtexpath.buf);

	//---- トーン化 (翻訳グループ = DLG)

	MLK_TRGROUP(TRGROUP_DLG_LAYER);

	//トーン化

	p->ck_tone = mCheckButtonCreate(MLK_WIDGET(p), WID_NEWOPT_CK_TONE, 0, MLK_MAKE32_4(0,12,0,0),
		0, MLK_TR(TRID_LAYERDLG_TONE), info->ftone);

	//*** group (vert)

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), 0, MLK_MAKE32_4(0,6,0,0), 0, NULL);

	//*** grid

	ct2 = mContainerCreateGrid(ct, 2, 7, 6, 0, 0);

	//線数

	widget_createLabel(ct2, MLK_TR(TRID_LAYERDLG_TONE_LINES));

	ct3 = mContainerCreateHorz(ct2, 6, 0, 0);

	p->edit_toneline = widget_createEdit_num(ct3, 7, 50, 2000, 1, info->tone_lines);

	p->wg_linesbtt = (mWidget *)mArrowButtonCreate(ct3, WID_NEWOPT_BTT_LINES, 0, 0, MARROWBUTTON_S_DOWN | MARROWBUTTON_S_FONTSIZE);

	//角度

	widget_createLabel(ct2, MLK_TR2(TRGROUP_WORD, TRID_WORD_ANGLE));

	p->edit_toneangle = widget_createEdit_num(ct2, 7, -360, 360, 0, info->tone_angle);

	//濃度固定

	p->ck_tonefix = mCheckButtonCreate(ct2, WID_NEWOPT_CK_TONEFIX, 0, 0,
		0, MLK_TR(TRID_LAYERDLG_TONE_FIX), (info->tone_density != 0));

	p->edit_tonedens = widget_createEdit_num(ct2, 7, 1, 100, 0,
		(info->tone_density)? info->tone_density: 10);

	//** 背景を白にする

	p->ck_tone_white = mCheckButtonCreate(ct, WID_NEWOPT_CK_TONE_WHITE, 0, MLK_MAKE32_4(0,5,0,0),
		0, MLK_TR(TRID_LAYERDLG_TONE_BKGND_WHITE), info->ftone_white);

	//-----

	//ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0));

	mButtonCreate(ct, WID_NEWOPT_BTT_TEMPLATE, MLF_EXPAND_X, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_TEMPLATE));

	mContainerAddButtons_okcancel(ct);

	//初期化

	_newopt_init(p, info, fnew);
	
	return p;
}

/* 値を取得 */

static int _newopt_getvalue(_dlg_newopt *p,LayerNewOptInfo *info)
{
	mStr str = MSTR_INIT;
	int n,ret = 0;

	//名前
	
	mLineEditGetTextStr(p->edit_name, &str);

	if(!mStrCompareEq(&info->strname, str.buf))
		ret |= LAYER_NEWOPTDLG_F_NAME;

	mStrCopy(&info->strname, &str);

	//テクスチャ

	SelMaterial_getPath(p->seltex, &str);

	if(!mStrCompareEq(&info->strtexpath, str.buf))
		ret |= LAYER_NEWOPTDLG_F_TEXPATH;

	mStrCopy(&info->strtexpath, &str);

	//タイプ (新規時のみ)

	if(p->cb_type)
		info->type = mComboBoxGetItemParam(p->cb_type, -1);

	//合成モード

	n = mComboBoxGetItemParam(p->cb_blend, -1);

	if(n != info->blendmode)
		ret |= LAYER_NEWOPTDLG_F_BLENDMODE;

	info->blendmode = n;

	//不透明度
	
	n = (int)(mLineEditGetNum(p->edit_opacity) / 100.0 * 128 + 0.5);

	if(n != info->opacity)
		ret |= LAYER_NEWOPTDLG_F_OPACITY;

	info->opacity = n;

	//色 (A/A1 のみ)

	n = mColorButtonGetColor(p->colbtt);

	if(n != info->col)
		ret |= LAYER_NEWOPTDLG_F_COLOR;

	info->col = n;

	//トーン化 (GRAY/A1 のみ)

	n = mCheckButtonIsChecked(p->ck_tone);

	if((!n) != (!info->ftone))
		ret |= LAYER_NEWOPTDLG_F_TONE;

	info->ftone = n;

	//---- トーン、パラメータ

	//線数

	n = mLineEditGetNum(p->edit_toneline);

	if(n != info->tone_lines)
		ret |= LAYER_NEWOPTDLG_F_TONE_PARAM;

	info->tone_lines = n;

	//角度

	n = mLineEditGetNum(p->edit_toneangle);

	if(n != info->tone_angle)
		ret |= LAYER_NEWOPTDLG_F_TONE_PARAM;

	info->tone_angle = n;

	//濃度固定

	if(mCheckButtonIsChecked(p->ck_tonefix))
		n = mLineEditGetNum(p->edit_tonedens);
	else
		n = 0;

	if(n != info->tone_density)
		ret |= LAYER_NEWOPTDLG_F_TONE_PARAM;

	info->tone_density = n;

	//背景を白に

	n = mCheckButtonIsChecked(p->ck_tone_white);

	if(n != info->ftone_white)
		ret |= LAYER_NEWOPTDLG_F_TONE_PARAM;

	info->ftone_white = n;

	//トーン化OFFで、ON/OFF変化なしの場合、パラメータの変化は無効

	if(!info->ftone && !(ret & LAYER_NEWOPTDLG_F_TONE))
		ret &= ~LAYER_NEWOPTDLG_F_TONE_PARAM;

	//

	mStrFree(&str);

	return ret;
}

/** レイヤ新規作成/設定ダイアログ
 *
 * fnew: TRUE で新規作成。FALSE で設定。
 * 
 * 新規時、strname/type に値を入れておく。
 * 設定時、type にレイヤタイプを入れておく。
 *
 * type は 0x80 が ON でテキスト。
 *
 * return: -1 でキャンセル。設定時は、更新された値のフラグ。 */

int LayerDialog_newopt(mWindow *parent,LayerNewOptInfo *info,mlkbool fnew)
{
	_dlg_newopt *p;
	int ret;

	p = _newopt_create(parent, info, fnew);
	if(!p) return -1;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(!ret)
		ret = -1;
	else
		ret = _newopt_getvalue(p, info);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//***********************************
// テンプレートリスト編集
//***********************************


typedef struct
{
	MLK_DIALOG_DEF

	mListView *list;
}_dlg_templist;


/* 名前変更 */

static void _templist_cmd_rename(_dlg_templist *p,mColumnItem *item)
{
	mStr str = MSTR_INIT;
	const char *text;
	mList *list;
	ConfigLayerTemplItem *pi,*pinew;

	pi = (ConfigLayerTemplItem *)item->param;

	//名前取得

	mStrSetText(&str, pi->text);

	text = MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME);

	if(!mSysDlg_inputText(MLK_WINDOW(p), text, text,
		MSYSDLG_INPUTTEXT_F_NOT_EMPTY | MSYSDLG_INPUTTEXT_F_SET_DEFAULT,
		&str))
	{
		mStrFree(&str);
		return;
	}

	//-----

	list = &APPCONF->list_layertempl;

	//新規確保,コピー
	// :まるごとコピーするため、後でリストに追加

	pinew = (ConfigLayerTemplItem *)mMalloc0(
		sizeof(ConfigLayerTemplItem) + str.len + pi->texture_len + 1);

	*pinew = *pi;

	//名前セット

	pinew->name_len = str.len;

	memcpy(pinew->text, str.buf, str.len);
	memcpy(pinew->text + str.len + 1, pi->text + pi->name_len + 1, pi->texture_len);

	//リストに挿入,元アイテム削除

	mListInsertItem(list, MLISTITEM(pi), MLISTITEM(pinew));

	mListDelete(list, MLISTITEM(pi));
	
	//listview アイテム変更

	mListViewSetItemText_static(p->list, item, pinew->text);

	item->param = (intptr_t)pinew;

	//

	mStrFree(&str);
}

/* COMMAND イベント */

static void _templist_event_command(_dlg_templist *p,int id)
{
	mColumnItem *item;
	int fdown;

	item = mListViewGetFocusItem(p->list);
	if(!item) return;

	switch(id)
	{
		//名前変更
		case APPRES_LISTEDIT_RENAME:
			_templist_cmd_rename(p, item);
			break;
		//削除
		case APPRES_LISTEDIT_DEL:
			mListDelete(&APPCONF->list_layertempl, (mListItem *)item->param);

			mListViewDeleteItem_focus(p->list);
			break;
		//上へ/下へ
		case APPRES_LISTEDIT_UP:
		case APPRES_LISTEDIT_DOWN:
			fdown = (id == APPRES_LISTEDIT_DOWN);
			
			if(mListViewMoveItem_updown(p->list, item, fdown))
				mListMoveUpDown(&APPCONF->list_layertempl, (mListItem *)item->param, !fdown);
			break;
	}
}

/* イベント */

static int _templist_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_COMMAND:
			_templist_event_command((_dlg_templist *)wg, ev->cmd.id);
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* 作成 */

static _dlg_templist *_templistdlg_create(mWindow *parent)
{
	_dlg_templist *p;
	mListView *list;
	ConfigLayerTemplItem *pi;
	const uint8_t bttno[] = {
		APPRES_LISTEDIT_RENAME, APPRES_LISTEDIT_DEL,
		APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, 255
	};

	p = (_dlg_templist *)widget_createDialog(parent, sizeof(_dlg_templist),
		MLK_TR2(TRGROUP_DLG_LAYER, TRID_LAYERDLG_TITLE_TEMPLATE_EDIT),
		_templist_event_handle);

	if(!p) return NULL;

	//リスト

	p->list = list = mListViewCreate(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH,	MSCROLLVIEW_S_HORZVERT_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 16, 15);

	MLK_LIST_FOR(APPCONF->list_layertempl, pi, ConfigLayerTemplItem)
	{
		mListViewAddItem_text_static_param(list, pi->text, (intptr_t)pi);
	}

	mListViewSetFocusItem_index(list, 0);

	//ボタン

	widget_createListEdit_iconbar_btt(MLK_WIDGET(p), bttno, FALSE);

	return p;
}

/** テンプレートリスト編集ダイアログ */

void _templatelist_edit_dialog(mWindow *parent)
{
	_dlg_templist *p;

	p = _templistdlg_create(parent);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	mDialogRun(MLK_DIALOG(p), TRUE);
}

