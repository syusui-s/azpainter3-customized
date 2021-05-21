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
 * フォント選択ダイアログ
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_sysdlg.h"
#include "mlk_event.h"
#include "mlk_font.h"
#include "mlk_fontinfo.h"
#include "mlk_fontlist.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_str.h"
#include "mlk_string.h"

#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_combobox.h"
#include "mlk_lineedit.h"
#include "mlk_listview.h"
#include "mlk_conflistview.h"
#include "mlk_columnitem.h"


/* 
 * スタイルに Medium より太いものがない場合は、"Bold" スタイルが独自で追加される。
 * この場合、スタイルなし＆ weight=bold となる。
 */

//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mFontInfo *info,
		previnfo;	//プレビュー用情報
	uint32_t flags;		//ダイアログのフラグ
	int is_ex;			//拡張設定中か

	mFont *fontprev;	//プレビュー用フォント

	mListView *lv_font,
		*lv_style;
	mLineEdit *edit_size,
		*edit_file;		//param1 に index 値
	mCheckButton *ck_italic,
		*ck_file;
	mComboBox *cb_unit;
	mConfListView *exlist;
	mWidget *wgprev,
		*wg_bttfile,
		*ct_right,	//右の項目のトップコンテナ
		*ct_normal; //右の通常項目のコンテナ
}_dialog;

//----------------------

enum
{
	WID_LV_FAMILY = 100,
	WID_LV_STYLE,
	WID_CK_ITALIC,
	WID_EDIT_SIZE,
	WID_CB_SIZEUNIT,
	WID_CK_EX,
	WID_LIST_EX,
	WID_CK_FILE,
	WID_BTT_FILE
};

//ファミリーのアイテムの param 値
#define _FAMILY_PARAM_NORMAL  0
#define _FAMILY_PARAM_DEFAULT -1

//----------------------


//*******************************
// プレビュー
//*******************************

#define _PREV_PADX  8


/* 描画ハンドラ */

static void _prev_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mFont *font;

	font = (mFont *)wg->param1;

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, 0);
	mPixbufFillBox(pixbuf, 1, 1, wg->w - 2, wg->h - 2, MGUICOL_PIX(WHITE));

	if(font && mPixbufClip_setBox_d(pixbuf, 0, 0, wg->w - _PREV_PADX, wg->h))
	{
		mFontDrawText_pixbuf(font, pixbuf,
			_PREV_PADX, (wg->h - mFontGetHeight(font)) / 2,
			MLK_TR(MLK_TRSYS_FONT_PREVIEW_TEXT), -1, 0);
	}
}


//*******************************
// ダイアログ
//*******************************


/* スタイル列挙関数 */

static int _func_fontstyle(const char *name,int weight,int slant,void *param)
{
	mListViewAddItem(MLK_LISTVIEW(param), name, 0, MCOLUMNITEM_F_COPYTEXT, (slant << 16) | weight);

	return 0;
}

/* スタイルリストセット
 *
 * init: 初期表示時、TRUE */

static void _set_stylelist(_dialog *p,mlkbool init)
{
	mListView *lv;
	mColumnItem *pi;

	lv = p->lv_style;
	if(!lv) return; //スタイル設定なし

	//クリア

	mListViewDeleteAllItem(lv);

	//現在の選択フォントのスタイルセット (default 時は除く)

	pi = mListViewGetFocusItem(p->lv_font);

	if(pi && pi->param == _FAMILY_PARAM_NORMAL)
	{
		if(mFontList_getStyles(pi->text, _func_fontstyle, lv) == 1)
		{
			//Medium より太いスタイルがない場合、Bold 追加
			
			mListViewAddItem(lv, "Bold", 0, 0, -1);
		}
	}

	//---- 初期選択

	pi = NULL;

	//初期表示時、指定があれば検索

	if(init)
	{
		if(p->info->mask & MFONTINFO_MASK_STYLE)
			pi = mListViewGetItem_fromText(lv, p->info->str_style.buf);
		else if((p->info->mask & MFONTINFO_MASK_WEIGHT) && p->info->weight == MFONTINFO_WEIGHT_BOLD)
			//bold
			pi = mListViewGetItem_fromParam(lv, -1);
	}

	//Regular 検索

	if(!pi)
		pi = mListViewGetItem_fromParam(lv, MFONTINFO_WEIGHT_REGULAR);

	//なければ先頭

	if(!pi)
		pi = mListViewGetTopItem(lv);

	//選択

	mListViewSetFocusItem_scroll(lv, pi);
}

/* (拡張) 値を取得 */

static int _func_getconflist(int type,intptr_t no,mConfListViewValue *val,void *param)
{
	mFontInfo *p = (mFontInfo *)param;
	int sel;

	sel = val->select;
	if(sel == 0) return 0; //指定なし

	sel--;

	switch(no)
	{
		//ヒンティング
		case 0:
			p->ex.mask |= MFONTINFO_EX_MASK_HINTING;
			p->ex.hinting = sel;
			break;
		//レンダリング
		case 1:
			p->ex.mask |= MFONTINFO_EX_MASK_RENDERING;
			p->ex.rendering = sel;
			break;
		//LCD フィルタ
		case 2:
			p->ex.mask |= MFONTINFO_EX_MASK_LCD_FILTER;
			p->ex.lcd_filter = sel;
			break;
		//オートヒント
		case 3:
			p->ex.mask |= MFONTINFO_EX_MASK_AUTO_HINT;
			if(sel) p->ex.flags |= MFONTINFO_EX_FLAGS_AUTO_HINT;
			break;
		//埋め込みビットマップ
		case 4:
			p->ex.mask |= MFONTINFO_EX_MASK_EMBEDDED_BITMAP;
			if(sel) p->ex.flags |= MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP;
			break;
	}

	return 0;
}

/* mFontInfo に値を取得 */

static void _get_value(_dialog *p,mFontInfo *dst)
{
	mColumnItem *pi;
	mStr str = MSTR_INIT;
	uint32_t mask = 0;

	//クリア

	mFontInfoFree(dst);

	mMemset0(dst, sizeof(mFontInfo));

	//----- 通常

	//フォント名

	if(p->ck_file && mCheckButtonIsChecked(p->ck_file))
	{
		//---- ファイル指定
		
		mLineEditGetTextStr(p->edit_file, &str);

		if(mStrIsnotEmpty(&str))
		{
			mask |= MFONTINFO_MASK_FILE;

			mStrCopy(&dst->str_file, &str);

			dst->index = p->edit_file->wg.param1;
		}

		mStrFree(&str);
	}
	else
	{
		//---- ファミリ指定

		//ファミリ名

		pi = mListViewGetFocusItem(p->lv_font);

		if(pi && pi->param == _FAMILY_PARAM_NORMAL)
		{
			mask |= MFONTINFO_MASK_FAMILY;
			mStrSetText(&dst->str_family, pi->text);
		}

		//スタイル

		if(p->lv_style)
		{
			//スタイル
			
			pi = mListViewGetFocusItem(p->lv_style);
			if(pi)
			{
				if(pi->param == -1)
				{
					//Bold
					mask |= MFONTINFO_MASK_WEIGHT;
					dst->weight = MFONTINFO_WEIGHT_BOLD;
				}
				else
				{
					//スタイル文字列
					mask |= MFONTINFO_MASK_STYLE;
					mStrSetText(&dst->str_style, pi->text);
				}
			}

			//斜体

			if(mCheckButtonIsChecked(p->ck_italic))
			{
				mask |= MFONTINFO_MASK_SLANT;
				dst->slant = MFONTINFO_SLANT_ITALIC;
			}
		}
	}

	//サイズ

	if(p->edit_size)
	{
		mask |= MFONTINFO_MASK_SIZE;
		dst->size = mLineEditGetDouble(p->edit_size);

		//px
		if(mComboBoxGetItemParam(p->cb_unit, -1) == 1)
			dst->size = -(dst->size);
	}

	//----- 拡張

	if(p->flags & MSYSDLG_FONT_F_EX)
	{
		mask |= MFONTINFO_MASK_EX;

		mConfListView_getValues(p->exlist, _func_getconflist, dst);
	}

	//----- マスク

	dst->mask = mask;
}

/* プレビュー更新 (フォント再作成) */

static void _preview_update(_dialog *p)
{
	if(p->flags & MSYSDLG_FONT_F_PREVIEW)
	{
		mFontFree(p->fontprev);

		_get_value(p, &p->previnfo);

		p->fontprev = mFontCreate(mGuiGetFontSystem(), &p->previnfo);

		//

		p->wgprev->param1 = (intptr_t)p->fontprev;

		mWidgetRedraw(p->wgprev);
	}
}

/* プレビュー更新 (拡張項目) */

static void _preview_update_ex(_dialog *p)
{
	if(p->flags & MSYSDLG_FONT_F_PREVIEW)
	{
		_get_value(p, &p->previnfo);

		mFontSetInfoEx(p->fontprev, &p->previnfo);

		mWidgetRedraw(p->wgprev);
	}
}

/* フォントファイル選択 */

static void _select_file(_dialog *p)
{
	mStr str = MSTR_INIT;
	int index;

	mLineEditGetTextStr(p->edit_file, &str);

	if(mSysDlg_openFontFile(MLK_WINDOW(p), "all files\t*", 0, NULL, &str, &index))
	{
		mLineEditSetText(p->edit_file, str.buf);

		p->edit_file->wg.param1 = index;

		_preview_update(p);
	}
	
	mStrFree(&str);
}

/* ファイル指定時の有効/無効 */

static void _set_enable_file(_dialog *p,int enable)
{
	//ファイル指定
	mWidgetEnable(MLK_WIDGET(p->edit_file), enable);
	mWidgetEnable(p->wg_bttfile, enable);

	//ファミリ指定
	mWidgetEnable(MLK_WIDGET(p->lv_font), !enable);
	mWidgetEnable(MLK_WIDGET(p->lv_style), !enable);
	mWidgetEnable(MLK_WIDGET(p->ck_italic), !enable);
}


//============================
// イベント
//============================


/* 終了 */

static void _end_dialog(_dialog *p,mlkbool ok)
{
	if(ok)
		_get_value(p, p->info);

	mGuiTransRestoreGroup();

	mDialogEnd(MLK_DIALOG(p), ok);
}

/** イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;

	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//フォント変更時
			case WID_LV_FAMILY:
				if(ev->notify.notify_type == MLISTVIEW_N_CHANGE_FOCUS)
				{
					_set_stylelist(p, FALSE);
					_preview_update(p);
				}
				break;
			//スタイル
			case WID_LV_STYLE:
				if(ev->notify.notify_type == MLISTVIEW_N_CHANGE_FOCUS)
					_preview_update(p);
				break;
			//斜体
			case WID_CK_ITALIC:
				if(ev->notify.notify_type == MCHECKBUTTON_N_CHANGE)
					_preview_update(p);
				break;
			//サイズ
			case WID_EDIT_SIZE:
				if(ev->notify.notify_type == MLINEEDIT_N_CHANGE)
					_preview_update(p);
				break;
			//サイズ単位
			case WID_CB_SIZEUNIT:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
					_preview_update(p);
				break;

			//拡張リスト
			case WID_LIST_EX:
				if(ev->notify.notify_type == MCONFLISTVIEW_N_CHANGE_VAL)
					_preview_update_ex(p);
				break;
			//詳細
			case WID_CK_EX:
				if(ev->notify.notify_type == MCHECKBUTTON_N_CHANGE)
				{
					p->is_ex ^= 1;

					mWidgetShow(p->ct_normal, !p->is_ex);
					mWidgetShow(MLK_WIDGET(p->exlist), p->is_ex);
					mWidgetReLayout(p->ct_right);
				}
				break;

			//ファイル指定チェック
			case WID_CK_FILE:
				if(ev->notify.notify_type == MCHECKBUTTON_N_CHANGE)
				{
					_set_enable_file(p, ev->notify.param1);
					_preview_update(p);
				}
				break;
			//ファイルボタン
			case WID_BTT_FILE:
				_select_file(p);
				break;
		
			//OK
			case MLK_WID_OK:
				_end_dialog(p, TRUE);
				break;
			//キャンセル
			case MLK_WID_CANCEL:
				_end_dialog(p, FALSE);
				break;
		}
	
		return 1;
	}

	return mDialogEventDefault(wg, ev);
}


//============================
// 作成
//============================


/* フォントファミリー列挙関数 */

static int _func_fontfamily(const char *name,void *param)
{
	mListViewAddItem_text_copy(MLK_LISTVIEW(param), name);

	return 0;
}

/* フォントリストセット */

static void _set_fontlist(_dialog *p,mListView *lv)
{
	mColumnItem *focus;
	mStr *strdef;

	//デフォルトアイテム

	mListViewAddItem_text_static_param(lv, "(default)", _FAMILY_PARAM_DEFAULT);

	//リスト追加

	mFontList_getFamilies(_func_fontfamily, lv);

	//幅セット

	mListViewSetAutoWidth(lv, TRUE);

	//初期選択

	strdef = &p->info->str_family;

	if(!(p->info->mask & MFONTINFO_MASK_FAMILY) || mStrIsEmpty(strdef))
		//空または指定なしの場合、デフォルト
		focus = NULL;
	else
		//名前から検索
		focus = mListViewGetItem_fromText(lv, strdef->buf);

	if(!focus) focus = mListViewGetTopItem(lv);

	mListViewSetFocusItem_scroll(lv, focus);
}

/* 通常の右項目を作成 */

static void _create_right_normal(_dialog *p,mFontInfo *info)
{
	mWidget *ct,*parent;
	int initw,n;

	parent = p->ct_normal;
	initw = mWidgetGetFontHeight(MLK_WIDGET(p)) * ((p->flags & MSYSDLG_FONT_F_EX)? 12: 10);

	//スタイル

	if(p->flags & MSYSDLG_FONT_F_STYLE)
	{
		//スタイル
		
		mLabelCreate(parent, 0, 0, 0, MLK_TR(MLK_TRSYS_FONT_STYLE));

		p->lv_style = mListViewCreate(parent, WID_LV_STYLE, MLF_EXPAND_WH, 0,
			0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

		MLK_WIDGET(p->lv_style)->initW = initw;

		_set_stylelist(p, TRUE);

		//斜体

		p->ck_italic = mCheckButtonCreate(parent, WID_CK_ITALIC, MLF_EXPAND_X | MLF_RIGHT, 0, 0,
			MLK_TR(MLK_TRSYS_FONT_ITALIC),
			((info->mask & MFONTINFO_MASK_SLANT) && info->slant != 0));
	}

	//サイズ

	if(p->flags & MSYSDLG_FONT_F_SIZE)
	{
		mLabelCreate(parent, 0, 0, 0, MLK_TR(MLK_TRSYS_FONT_SIZE));

		//---

		ct = mContainerCreateHorz(parent, 5, MLF_EXPAND_W, 0);

		n = (info->mask & MFONTINFO_MASK_SIZE)? round(info->size * 10): 100;

		//サイズ入力

		p->edit_size = mLineEditCreate(ct, WID_EDIT_SIZE, MLF_EXPAND_W | MLF_MIDDLE, 0,
			MLINEEDIT_S_NOTIFY_CHANGE | MLINEEDIT_S_SPIN);

		p->edit_size->wg.initW = initw;

		mLineEditSetNumStatus(p->edit_size, 1, 10000, 1);
		mLineEditSetNum(p->edit_size, (n < 0)? -n: n);
		mLineEditSetSpinValue(p->edit_size, 10); //1.0単位

		//単位

		p->cb_unit = mComboBoxCreate(ct, WID_CB_SIZEUNIT, 0, 0, 0);

		mComboBoxAddItems_sepnull(p->cb_unit, "pt\0px\0", 0);
		mComboBoxSetAutoWidth(p->cb_unit);

		mComboBoxSetSelItem_atIndex(p->cb_unit, (n < 0));
	}
}

/* 拡張のリスト作成 */

static void _create_right_ex(_dialog *p,mWidget *parent,mFontInfo *info)
{
	mConfListView *clv;
	uint32_t exmask;
	int no = 0;

	exmask = info->ex.mask;

	p->exlist = clv = mConfListViewNew(parent, 0, 0);

	clv->wg.id = WID_LIST_EX;

	mWidgetShow(MLK_WIDGET(clv), 0);

	//

	mConfListView_addItem_select(clv, "hinting",
		(exmask & MFONTINFO_EX_MASK_HINTING)? info->ex.hinting + 1: 0,
		"(default)\tnone\tslight\tmedium\tfull", FALSE, no++);

	mConfListView_addItem_select(clv, "rendering",
		(exmask & MFONTINFO_EX_MASK_RENDERING)? info->ex.rendering + 1: 0,
		"(default)\tmono\tgray\tLCD-RGB\tLCD-BGR\tLCD-VRGB\tLCD-VBGR", FALSE, no++);

	mConfListView_addItem_select(clv, "LCD filter",
		(exmask & MFONTINFO_EX_MASK_LCD_FILTER)? info->ex.lcd_filter + 1: 0,
		"(default)\tnone\tdefault\tlight", FALSE, no++);

	mConfListView_addItem_select(clv, "autohint",
		(exmask & MFONTINFO_EX_MASK_AUTO_HINT)? ((info->ex.flags & MFONTINFO_EX_FLAGS_AUTO_HINT) != 0) + 1: 0,
		"(default)\tfalse\ttrue", FALSE, no++);

	mConfListView_addItem_select(clv, "embedded bitmap",
		(exmask & MFONTINFO_EX_MASK_EMBEDDED_BITMAP)? ((info->ex.flags & MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP) != 0) + 1: 0,
		"(default)\tfalse\ttrue", FALSE, no++);

	mListViewSetColumnWidth_auto(MLK_LISTVIEW(clv), 0);
}

/** メイン画面のウィジェット作成 */

static void _create_widget_main(_dialog *p)
{
	mWidget *cth,*ctv;
	mFontInfo *info;
	int no_right;

	info = p->info;

	no_right = !(p->flags & (MSYSDLG_FONT_F_STYLE | MSYSDLG_FONT_F_SIZE | MSYSDLG_FONT_F_EX));

	//水平コンテナ

	cth = mContainerCreateHorz(MLK_WIDGET(p), 10, MLF_EXPAND_WH, 0);

	//---- 左:フォントリスト

	p->lv_font = mListViewCreate(cth, WID_LV_FAMILY,
		MLF_EXPAND_H | (no_right? MLF_EXPAND_W: 0), 0,
		0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->lv_font), 0, 18);

	_set_fontlist(p, p->lv_font);

	//------ 右:項目

	if(no_right) return; //フォント名だけならなし

	p->ct_right = ctv = mContainerCreateVert(cth, 0, MLF_EXPAND_WH, 0);

	//通常項目

	p->ct_normal = mContainerCreateVert(ctv, 5, MLF_EXPAND_WH, 0);

	_create_right_normal(p, info);

	//拡張

	if(p->flags & MSYSDLG_FONT_F_EX)
		_create_right_ex(p, ctv, info);
}

/* ファイル指定ウィジェット作成 */

static void _create_filewg(_dialog *p)
{
	mWidget *ct;
	mFontInfo *info = p->info;
	int enable;

	enable = (info->mask & MFONTINFO_MASK_FILE);

	ct = mContainerCreateHorz(MLK_WIDGET(p), 5, MLF_EXPAND_W, 0);

	//チェック

	p->ck_file = mCheckButtonCreate(ct, WID_CK_FILE, MLF_MIDDLE, 0, 0, MLK_TR(MLK_TRSYS_FONT_FILE), enable);

	//エディット

	p->edit_file = mLineEditCreate(ct, 0, MLF_EXPAND_W | MLF_MIDDLE, 0, MLINEEDIT_S_READ_ONLY);

	if(enable)
	{
		mLineEditSetText(p->edit_file, info->str_file.buf);

		p->edit_file->wg.param1 = info->index;
	}

	//ボタン

	p->wg_bttfile = (mWidget *)mButtonCreate(ct, WID_BTT_FILE, 0, 0, MBUTTON_S_REAL_W, "...");
}

/** ウィジェット作成 */

static void _create_widget(_dialog *p)
{
	mWidget *wg,*cth;

	//ファイル指定

	if(p->flags & MSYSDLG_FONT_F_FILE)
		_create_filewg(p);

	//メイン部分

	_create_widget_main(p);

	//有効/無効

	_set_enable_file(p, ((p->info->mask & MFONTINFO_MASK_FILE) != 0));

	//プレビュー

	if(p->flags & MSYSDLG_FONT_F_PREVIEW)
	{
		p->wgprev = wg = mWidgetNew(MLK_WIDGET(p), 0);

		wg->draw = _prev_draw_handle;
		wg->param1 = (intptr_t)p->fontprev;
		wg->flayout = MLF_EXPAND_W;
		wg->hintH = 55;
	}

	//------- ボタン

	cth = mContainerCreateHorz(MLK_WIDGET(p), 6, MLF_EXPAND_W, MLK_MAKE32_4(0,4,0,0));

	//詳細

	if(p->flags & MSYSDLG_FONT_F_EX)
	{
		wg = (mWidget *)mCheckButtonCreate(cth, WID_CK_EX, 0, 0, MCHECKBUTTON_S_BUTTON,
			MLK_TR(MLK_TRSYS_FONT_DETAIL), 0);

		wg->hintMinW = 64;
	}

	//OK

	wg = (mWidget *)mButtonCreate(cth, MLK_WID_OK, MLF_EXPAND_X | MLF_RIGHT, 0, 0, MLK_TR(MLK_TRSYS_OK));
	wg->fstate |= MWIDGET_STATE_ENTER_SEND;

	//キャンセル
	
	mButtonCreate(cth, MLK_WID_CANCEL, 0, 0, 0, MLK_TR(MLK_TRSYS_CANCEL));
}

/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	_dialog *p = (_dialog *)wg;

	mFontFree(p->fontprev);
	mFontInfoFree(&p->previnfo);
}

/** ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,uint32_t flags,mFontInfo *info)
{
	_dialog *p;
	
	p = (_dialog *)mDialogNew(parent, sizeof(_dialog), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;

	p->info = info;
	p->flags = flags;

	//プレビュー用

	if(flags & MSYSDLG_FONT_F_PREVIEW)
	{
		mFontInfoCopy(&p->previnfo, info);

		p->fontprev = mFontCreate(mGuiGetFontSystem(), &p->previnfo);
	}

	//

	mGuiTransSaveGroup();

	MLK_TRGROUP(MLK_TRGROUP_ID_SYSTEM);

	//

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR(MLK_TRSYS_SELECT_FONT));

	mContainerSetType_vert(MLK_CONTAINER(p), 8);
	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	//ウィジェット

	_create_widget(p);

	return p;
}


//*******************************
// 関数
//*******************************


/**@ フォント選択
 *
 * @p:info 初期値を入れておく。結果の値が入る。 */

mlkbool mSysDlg_selectFont(mWindow *parent,uint32_t flags,mFontInfo *info)
{
	_dialog *p;

	p = _create_dialog(parent, flags, info);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

/**@ フォント選択 (文字列フォーマット)
 *
 * @p:str 初期値を入れておく。結果の値が入る。 */

mlkbool mSysDlg_selectFont_text(mWindow *parent,uint32_t flags,mStr *str)
{
	_dialog *p;
	mFontInfo info;
	mlkbool ret = FALSE;

	//mFontInfo

	mMemset0(&info, sizeof(mFontInfo));

	mFontInfoSetFromText(&info, str->buf);

	//ダイアログ

	p = _create_dialog(parent, flags, &info);

	if(p)
	{
		mWindowResizeShow_initSize(MLK_WINDOW(p));

		ret = mDialogRun(MLK_DIALOG(p), TRUE);

		if(ret)
			mFontInfoToText(str, &info);
	}

	mFontInfoFree(&info);

	return ret;
}


