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
 * テキスト描画ダイアログ
 *************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_groupbox.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_imgbutton.h"
#include "mlk_arrowbutton.h"
#include "mlk_lineedit.h"
#include "mlk_multiedit.h"
#include "mlk_combobox.h"
#include "mlk_tab.h"
#include "mlk_splitter.h"
#include "mlk_containerview.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_imagelist.h"
#include "mlk_fontlist.h"
#include "mlk_columnitem.h"
#include "mlk_sysdlg.h"
#include "mlk_menu.h"
#include "mlk_str.h"
#include "mlk_key.h"
#include "mlk_util.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"

#include "draw_main.h"
#include "apphelp.h"
#include "regfont.h"
#include "textword_list.h"

#include "widget_func.h"

#include "trid.h"


//--------------------

typedef struct _RotateWidget RotateWidget;

typedef struct
{
	MLK_DIALOG_DEF

	uint8_t fset_fontlist,	//フォント一覧のリストがセット済み
		fset_regfontlist;	//登録フォントのリストがセット済み

	mContainerView *ctview;
	mWidget *ct_top,	//上部のコンテナ
		*ct_fontsel[3];	//フォント選択の各コンテナ

	mMultiEdit *medit;
	mLineEdit *edit_fontfile,
		*edit_size,
		*edit_rsize,
		*edit_dpi,
		*edit_charsp,
		*edit_linesp,
		*edit_rotate,
		*edit_rubypos;
	mComboBox *cb_family,
		*cb_style,
		*cb_regfont,
		*cb_unit_fontsize;
	RotateWidget *rotwg;
}_dialog;

enum
{
	WID_BTT_HELP = 100,
	WID_BTT_COPY,
	WID_BTT_PASTE,
	WID_TEXTEDIT,
	WID_CK_PREVIEW,

	WID_TAB_FONTSEL,
	WID_CB_FONTLIST,
	WID_CB_STYLE,
	WID_CB_FONTREG,
	WID_BTT_FONTREG_EDIT,
	WID_BTT_FONTREG_LISTEDIT,
	WID_BTT_FONTFILE_SEL,
	
	WID_EDIT_SIZE,
	WID_EDIT_RUBYSIZE,
	WID_CB_SIZE_UNIT,
	WID_CB_RUBYSIZE_UNIT,
	WID_CK_DPI,
	WID_EDIT_DPI,
	WID_EDIT_CHARSP,
	WID_EDIT_LINESP,
	WID_CB_CHARSP_UNIT,
	WID_CB_LINESP_UNIT,
	WID_EDIT_RUBY_POS,
	WID_CB_RUBY_POS,
	WID_CK_NO_RUBY_GLYPH,
	WID_BTT_FONTSIZE_MENU,
	
	WID_CB_HINTING,
	WID_CK_AUTOHINT_DISABLE,

	WID_EDIT_ROTATE,
	WID_ROTATE_CTRL
};

enum
{
	TRID_TITLE,
	TRID_FONT,
	TRID_FONTLIST,
	TRID_REGFONT,
	TRID_SPEC_FILE,
	TRID_CHAR_SPACE,
	TRID_LINE_SPACE,
	TRID_ROTATE,
	TRID_HINTING,
	TRID_DISABLE_AUTOHINT,
	TRID_RUBY,
	TRID_RUBY_POS,
	TRID_NO_RUBY_GLYPH,

	TRID_MONO = 100,
	TRID_VERT,
	TRID_ENABLE_FORMAT,
	TRID_BOLD,
	TRID_ITALIC,
	TRID_ENABLE_BITMAP,

	TRID_MENU_EDIT_WORD = 200
};

enum
{
	_UPDATE_F_FONT = 1<<0,	//フォントを再作成
	_UPDATE_F_TIMER = 1<<1	//タイマー更新
};

//--------------------


//リスト編集ボタン
static const unsigned char g_img_listedit[]={
0xfe,0x3f,0x02,0x20,0x3a,0x20,0xba,0x2f,0x3a,0x20,0x02,0x20,0x3a,0x20,0xba,0x2f,
0x3a,0x20,0x02,0x20,0x3a,0x20,0xba,0x2f,0x3a,0x20,0x02,0x20,0xfe,0x3f };

//アイコン([16x16]x6)
static const unsigned char g_img_icon[]={
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x30,0x38,0x38,
0x06,0x00,0xf8,0x07,0x80,0x1f,0xce,0x7d,0x1c,0x38,0x38,0x38,0x06,0x00,0xf8,0x0f,
0x00,0x06,0x92,0x10,0x3c,0x3c,0x38,0x38,0x06,0x00,0x38,0x1e,0x00,0x06,0x92,0x10,
0x7c,0x3e,0x38,0x00,0x0c,0x00,0x38,0x1c,0x00,0x03,0x8e,0x10,0xfc,0x3f,0x38,0x00,
0x0c,0x00,0x38,0x1c,0x00,0x03,0x92,0x10,0xfc,0x3f,0x38,0x38,0x0c,0x00,0x38,0x0e,
0x00,0x03,0x92,0x10,0xdc,0x3b,0x38,0x38,0x0c,0x63,0xf8,0x07,0x80,0x01,0xce,0x11,
0x9c,0x39,0x38,0x38,0x18,0x63,0xf8,0x07,0x80,0x01,0x00,0x00,0x1c,0x38,0x38,0x00,
0x18,0x63,0x38,0x0e,0x80,0x01,0x12,0x31,0x1c,0x38,0x38,0x00,0x18,0x63,0x38,0x1c,
0xc0,0x00,0x1e,0x51,0x1c,0x38,0xfe,0x38,0x18,0x73,0x38,0x1c,0xc0,0x00,0x92,0x52,
0x1c,0x38,0x7c,0x38,0x30,0x73,0x38,0x1e,0xc0,0x00,0x92,0x33,0x1c,0x38,0x38,0x38,
0x30,0x6f,0xf8,0x0f,0x60,0x00,0x52,0x14,0x1c,0x38,0x10,0x00,0x30,0x6e,0xf8,0x07,
0xf8,0x01,0x52,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };

//--------------------

RotateWidget *RotateWidget_new(mWidget *parent,int id,uint32_t margin_pack4,int size);
mlkbool RotateWidget_setAngle(RotateWidget *p,int angle);

mlkbool RegFontListDlg_run(mWindow *parent,mComboBox *cblist);
mlkbool RegFontEditDlg_run(mWindow *parent,mComboBox *cblist);

void TextWordListDlg_run(mWindow *parent);

//--------------------



//========================
// sub
//========================


/* 値変更時の更新 */

static void _update(_dialog *p,uint8_t flags)
{
	//フォント作成
	
	if(flags & _UPDATE_F_FONT)
		drawText_createFont(APPDRAW);

	//プレビュー

	if(APPDRAW->text.fpreview)
	{
		if(flags & _UPDATE_F_TIMER)
			//タイマーを使う
			mWidgetTimerAdd(MLK_WIDGET(p), 0, 100, 0);
		else
			//直ちに更新
			drawText_drawPreview(APPDRAW);
	}
}


//========================
// sub - フォント選択
//========================


/* (列挙) フォント一覧 */

static int _enumfunc_family(const char *name,void *param)
{
	mComboBoxAddItem_copy((mComboBox *)param, name, 0);

	return 0;
}

/* (列挙) スタイル一覧 */

static int _enumfunc_style(const char *name,int weight,int slant,void *param)
{
	mComboBoxAddItem_copy((mComboBox *)param, name, (slant << 8) | weight);
	return 0;
}

/* フォント一覧のリストをセット & 選択 */

static void _set_fontlist(_dialog *p,mlkbool initval)
{
	mColumnItem *pi;
	mStr *pstr;

	//リストのセット
	// :最初にタブが選択された時に一度だけセット。
	
	if(!p->fset_fontlist)
	{
		mFontList_getFamilies(_enumfunc_family, p->cb_family);

		p->fset_fontlist = TRUE;
	}

	//選択

	pstr = &APPDRAW->text.dt.str_font;

	if(initval)
	{
		//先頭のフォント
		
		pi = mComboBoxGetTopItem(p->cb_family);

		if(pi)
			mStrSetText(pstr, pi->text);
		else
			mStrEmpty(pstr);
	}
	else
	{
		//最初の初期選択

		pi = NULL;

		if(mStrIsnotEmpty(pstr))
			pi = mComboBoxGetItem_fromText(p->cb_family, pstr->buf);

		if(!pi) pi = mComboBoxGetTopItem(p->cb_family);
	}

	mComboBoxSetSelItem(p->cb_family, pi);
}

/* [フォント一覧] フォントの選択が変更された時
 *
 * スタイルのリストをセット */

static void _change_fontlist_sel(_dialog *p,mlkbool initval)
{
	mComboBox *cb = p->cb_style;
	mColumnItem *pi;
	mStr *pstr;

	mComboBoxDeleteAllItem(cb);

	pi = mComboBoxGetSelectItem(p->cb_family);
	if(!pi) return;

	mFontList_getStyles(pi->text, _enumfunc_style, cb);

	//選択

	pstr = &APPDRAW->text.dt.str_style;

	if(initval)
	{
		//[regular,斜体なし]のアイテム。なければ先頭

		pi = mComboBoxGetItem_fromParam(cb, 80);

		if(!pi) pi = mComboBoxGetTopItem(cb);

		if(pi)
			mStrSetText(pstr, pi->text);
		else
			mStrEmpty(pstr);
	}
	else
	{
		//最初の初期選択

		pi = NULL;

		if(mStrIsnotEmpty(pstr))
			pi = mComboBoxGetItem_fromText(cb, pstr->buf);

		if(!pi) pi = mComboBoxGetTopItem(cb);
	}

	mComboBoxSetSelItem(cb, pi);
}

/* フォントファイルの選択 */

static void _fontfile_select(_dialog *p)
{
	int ind;

	if(mSysDlg_openFontFile(MLK_WINDOW(p), "All files\t*", 0,
		APPCONF->strFontFileDir.buf, &APPDRAW->text.dt.str_font, &ind))
	{
		//ディレクトリ記録
		
		mStrPathGetDir(&APPCONF->strFontFileDir, APPDRAW->text.dt.str_font.buf);

		//
	
		APPDRAW->text.dt.font_param = ind;

		mLineEditSetText(p->edit_fontfile, APPDRAW->text.dt.str_font.buf);

		_update(p, _UPDATE_F_FONT);
		_update(p, _UPDATE_F_FONT);
	}
}

/* [登録フォント] リストをセット & 選択 */

static void _set_regfont_list(_dialog *p,mlkbool initval)
{
	mComboBox *cb;
	mListItem *pi;
	mColumnItem *ci;
	int bfirst = FALSE;

	cb = p->cb_regfont;

	//最初にタブが選択された時のみ、リストをセット

	if(!p->fset_regfontlist)
	{
		for(pi = RegFont_getTopItem(); pi; pi = pi->next)
		{
			mComboBoxAddItem_static(cb, RegFont_getNamePtr(pi), (intptr_t)pi);
		}

		p->fset_regfontlist = TRUE;
		bfirst = TRUE;
	}

	//---- 選択

	ci = NULL;

	if(initval)
	{
		//フォントタイプの変更時
		// :最初にタブが選択されたときは先頭。それ以外は現在の選択。

		if(!bfirst)
			ci = mComboBoxGetSelectItem(cb);
	}
	else
	{
		//初期選択時 (ID から検索)

		pi = RegFont_findID(APPDRAW->text.dt.font_param);

		if(pi)
			ci = mComboBoxGetItem_fromParam(cb, (intptr_t)pi);
	}

	//デフォルトで先頭のフォント

	if(!ci)
		ci = mComboBoxGetTopItem(cb);

	mComboBoxSetSelItem(cb, ci);

	//ID 取得

	if(ci)
		APPDRAW->text.dt.font_param = RegFont_getItemID((mListItem *)ci->param);
}

/* [登録フォント] 選択変更時 */

static void _change_regfont_sel(_dialog *p)
{
	mColumnItem *pi;

	pi = mComboBoxGetSelectItem(p->cb_regfont);

	if(pi)
		APPDRAW->text.dt.font_param = RegFont_getItemID((mListItem *)pi->param);
}


//========================
// sub - ウィジェット
//========================


/* mLineEdit 数値のセット */

static void _set_lineedit_num(mLineEdit *p,int min,int max,int dig,int val)
{
	mLineEditSetNumStatus(p, min, max, dig);
	mLineEditSetNum(p, val);
	mLineEditSetSpinValue(p, (dig == 0)? 1: 10);
}

/* フォント選択の変更時 */

static void _change_fontsel(_dialog *p,mlkbool initval)
{
	int i,sel;

	sel = APPDRAW->text.dt.fontsel;

	//コンテナの表示/非表示

	for(i = 0; i < 3; i++)
		mWidgetShow(p->ct_fontsel[i], (i == sel));

	//各値

	switch(sel)
	{
		//一覧
		case DRAWTEXT_FONTSEL_LIST:
			_set_fontlist(p, initval);
			_change_fontlist_sel(p, initval);
			break;
		//登録フォント
		case DRAWTEXT_FONTSEL_REGIST:
			_set_regfont_list(p, initval);
			break;
		//フォントファイル
		case DRAWTEXT_FONTSEL_FILE:
			if(initval)
				mStrEmpty(&APPDRAW->text.dt.str_font);

			mLineEditSetText(p->edit_fontfile, APPDRAW->text.dt.str_font.buf);
			break;
	}
}

/* フォントサイズの単位変更
 *
 * initval: 値を初期値に変更 (mComboBox 選択変更時) */

static void _change_fontsize_unit(_dialog *p,mlkbool initval)
{
	int unit;

	unit = APPDRAW->text.dt.unit_fontsize;

	if(initval)
		APPDRAW->text.dt.fontsize = (unit == 0)? 90: 13;

	_set_lineedit_num(p->edit_size, 1, 10000, (unit == 0)? 1: 0, APPDRAW->text.dt.fontsize);
}

/* ルビサイズの単位変更 */

static void _change_rubysize_unit(_dialog *p,mlkbool initval)
{
	int unit,n;

	unit = APPDRAW->text.dt.unit_rubysize;

	if(initval)
	{
		if(unit == 0)
			n = 50;
		else if(unit == 1)
			n = 90;
		else
			n = 13;

		APPDRAW->text.dt.rubysize = n;
	}

	_set_lineedit_num(p->edit_rsize, 1,
		(unit == 0)? 500: 10000, (unit == 1)? 1: 0, APPDRAW->text.dt.rubysize);
}

/* 字間の単位変更 */

static void _change_charsp_unit(_dialog *p,mlkbool initval)
{
	if(initval)
		APPDRAW->text.dt.char_space = 0;

	_set_lineedit_num(p->edit_charsp, -1000, 1000, 0, APPDRAW->text.dt.char_space);
}

/* 行間の単位変更 */

static void _change_linesp_unit(_dialog *p,mlkbool initval)
{
	if(initval)
		APPDRAW->text.dt.line_space = 0;

	_set_lineedit_num(p->edit_linesp, -10000, 10000, 0, APPDRAW->text.dt.line_space);
}

/* ルビ位置の単位変更 */

static void _change_rubypos_unit(_dialog *p,mlkbool initval)
{
	if(initval)
		APPDRAW->text.dt.ruby_pos = 0;

	_set_lineedit_num(p->edit_rubypos, -10000, 10000, 0, APPDRAW->text.dt.ruby_pos);
}

/* テキスト入力:右クリックメニュー */

static void _edit_run_menu(_dialog *p,int x,int y)
{
	mMenu *menu;
	mMenuItem *mi;
	mMenuItemInfo *minfo;
	TextWordItem *pi;
	int id;

	//----- メニュー

	menu = mMenuNew();

	//単語リスト

	for(pi = (TextWordItem *)APPCONF->list_textword.top; pi; pi = (TextWordItem *)pi->i.next)
	{
		minfo = mMenuAppendText(menu, 0, pi->txt);
		if(minfo) minfo->param1 = (intptr_t)pi;
	}

	if(APPCONF->list_textword.top)
		mMenuAppendSep(menu);
	
	mMenuAppendText(menu, TRID_MENU_EDIT_WORD, MLK_TR2(TRGROUP_DLG_TEXT, TRID_MENU_EDIT_WORD));

	//

	mi = mMenuPopup(menu, MLK_WIDGET(p->medit), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	if(!mi)
		id = -1;
	else
	{
		id = mMenuItemGetID(mi);
		pi = (TextWordItem *)mMenuItemGetInfo(mi)->param1;
	}

	mMenuDestroy(menu);

	if(id == -1) return;

	//------

	if(id == TRID_MENU_EDIT_WORD)
		//編集
		TextWordListDlg_run(MLK_WINDOW(p));
	else if(pi)
		//単語挿入
		mMultiEditInsertText(p->medit, pi->txt + pi->text_pos);
}

/* フォントサイズの履歴メニュー */

static void _runmenu_fontsize(_dialog *p,mWidget *wgbtt)
{
	mMenu *menu;
	mMenuItem *mi;
	int i,unit;
	uint16_t val;
	mBox box;
	mStr str = MSTR_INIT;

	if(APPCONF->textsize_recent[0] == 0) return;

	//----- メニュー

	menu = mMenuNew();

	for(i = 0; i < CONFIG_TEXTSIZE_RECENT_NUM; i++)
	{
		val = APPCONF->textsize_recent[i];
		if(!val) break;

		if(val & (1<<15))
			mStrSetFormat(&str, "%d px", val & ~(1<<15));
		else
			mStrSetFormat(&str, "%.1F pt", val);
	
		mMenuAppendText_copy(menu, i, str.buf, str.len);
	}

	mStrFree(&str);

	//

	mWidgetGetBox_rel(wgbtt, &box);

	mi = mMenuPopup(menu, wgbtt, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(i == -1) return;

	//値をセット

	val = APPCONF->textsize_recent[i];

	unit = (val & (1<<15))? DRAWTEXT_UNIT_FONTSIZE_PX: DRAWTEXT_UNIT_FONTSIZE_PT;
	val &= ~(1<<15);

	APPDRAW->text.dt.unit_fontsize = unit;
	APPDRAW->text.dt.fontsize = val;

	mComboBoxSetSelItem_atIndex(p->cb_unit_fontsize, unit);
	
	_set_lineedit_num(p->edit_size, 1, 10000, (unit == 0)? 1: 0, val);

	//

	drawText_changeFontSize(APPDRAW);

	_update(p, 0);
}


//========================
// イベント
//========================


/* 通知イベント */

static void _event_notify(_dialog *p,mEventNotify *ev)
{
	DrawTextData *pd = &APPDRAW->text.dt;
	int ntype;

	ntype = ev->notify_type;

	switch(ev->id)
	{
		//mMultiEdit
		case WID_TEXTEDIT:
			if(ntype == MMULTIEDIT_N_CHANGE)
			{
				//テキスト変更
				
				mMultiEditGetTextStr(p->medit, &APPDRAW->text.dt.str_text);

				_update(p, _UPDATE_F_TIMER);
			}
			else if(ntype == MMULTIEDIT_N_R_CLICK)
			{
				//右ボタン

				_edit_run_menu(p, ev->param1, ev->param2);
			}
			break;
		//プレビュー
		case WID_CK_PREVIEW:
			APPDRAW->text.fpreview ^= 1;

			drawText_drawPreview(APPDRAW);
			break;
	
		//コピー
		case WID_BTT_COPY:
			mMultiEditCopyText_full(p->medit);
			break;
		//貼り付け
		case WID_BTT_PASTE:
			if(mMultiEditPaste_full(p->medit))
			{
				mMultiEditGetTextStr(p->medit, &APPDRAW->text.dt.str_text);

				_update(p, 0);
			}
			break;
	
		//--------
		
		//フォント選択タブ
		case WID_TAB_FONTSEL:
			if(ntype == MTAB_N_CHANGE_SEL)
			{
				pd->fontsel = ev->param1;
				
				_change_fontsel(p, TRUE);
				
				mContainerViewReLayout(p->ctview);

				_update(p, _UPDATE_F_FONT);
			}
			break;
		//フォント一覧 (combo)
		case WID_CB_FONTLIST:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				mStrSetText(&pd->str_font, MLK_COLUMNITEM(ev->param1)->text);

				_change_fontlist_sel(p, TRUE);

				_update(p, _UPDATE_F_FONT);
			}
			break;
		//フォント一覧のスタイル (combo)
		case WID_CB_STYLE:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				mStrSetText(&pd->str_style, MLK_COLUMNITEM(ev->param1)->text);

				_update(p, _UPDATE_F_FONT);
			}
			break;
		//登録フォント選択 (combo)
		case WID_CB_FONTREG:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				_change_regfont_sel(p);
				
				_update(p, _UPDATE_F_FONT);
			}
			break;
		//登録フォント:フォント編集ボタン
		case WID_BTT_FONTREG_EDIT:
			if(RegFontEditDlg_run(MLK_WINDOW(p), p->cb_regfont))
			{
				_update(p, _UPDATE_F_FONT);
			}
			break;
		//登録フォント:リスト編集ボタン
		case WID_BTT_FONTREG_LISTEDIT:
			if(RegFontListDlg_run(MLK_WINDOW(p), p->cb_regfont))
			{
				//TRUE で、選択フォントが編集/削除された
				_update(p, _UPDATE_F_FONT);
			}
			break;
		//フォントファイル選択ボタン
		case WID_BTT_FONTFILE_SEL:
			_fontfile_select(p);
			break;

		//---------

		//フォントサイズ (edit)
		case WID_EDIT_SIZE:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->fontsize = mLineEditGetNum(p->edit_size);

				drawText_changeFontSize(APPDRAW);

				_update(p, 0);
			}
			break;
		//フォントサイズ単位
		case WID_CB_SIZE_UNIT:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				pd->unit_fontsize = ev->param2;

				_change_fontsize_unit(p, TRUE);

				drawText_changeFontSize(APPDRAW);

				_update(p, 0);
			}
			break;
		//フォントサイズ 履歴メニュー
		case WID_BTT_FONTSIZE_MENU:
			_runmenu_fontsize(p, ev->widget_from);
			break;
		//ルビサイズ (edit)
		case WID_EDIT_RUBYSIZE:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->rubysize = mLineEditGetNum(p->edit_rsize);
				_update(p, 0);
			}
			break;
		//ルビサイズ単位
		case WID_CB_RUBYSIZE_UNIT:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				pd->unit_rubysize = ev->param2;
				_change_rubysize_unit(p, TRUE);
				_update(p, 0);
			}
			break;
		//DPI (check)
		case WID_CK_DPI:
			pd->flags ^= DRAWTEXT_F_ENABLE_DPI;

			mWidgetEnable(MLK_WIDGET(p->edit_dpi), pd->flags & DRAWTEXT_F_ENABLE_DPI);

			drawText_changeFontSize(APPDRAW);

			_update(p, 0);
			break;
		//DPI (edit)
		case WID_EDIT_DPI:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->dpi = mLineEditGetNum(p->edit_dpi);

				drawText_changeFontSize(APPDRAW);

				_update(p, 0);
			}
			break;
		//字間 (edit)
		case WID_EDIT_CHARSP:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->char_space = mLineEditGetNum(p->edit_charsp);
				_update(p, 0);
			}
			break;
		//字間単位
		case WID_CB_CHARSP_UNIT:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				pd->unit_char_space = ev->param2;
				_change_charsp_unit(p, TRUE);
				_update(p, 0);
			}
			break;
		//行間 (edit)
		case WID_EDIT_LINESP:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->line_space = mLineEditGetNum(p->edit_linesp);
				_update(p, 0);
			}
			break;
		//行間単位
		case WID_CB_LINESP_UNIT:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				pd->unit_line_space = ev->param2;
				_change_linesp_unit(p, TRUE);
				_update(p, 0);
			}
			break;
		//ヒンティング (combo)
		case WID_CB_HINTING:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				pd->hinting = ev->param2;

				drawText_changeInfo(APPDRAW);
				
				_update(p, 0);
			}
			break;
		//オートヒント無効
		case WID_CK_AUTOHINT_DISABLE:
			pd->flags ^= DRAWTEXT_F_DISABLE_AUTOHINT;

			drawText_changeInfo(APPDRAW);
			
			_update(p, 0);
			break;
		//ルビ位置 (edit)
		case WID_EDIT_RUBY_POS:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->ruby_pos = mLineEditGetNum(p->edit_rubypos);

				_update(p, 0);
			}
			break;
		//ルビ位置単位
		case WID_CB_RUBY_POS:
			if(ntype == MCOMBOBOX_N_CHANGE_SEL)
			{
				pd->unit_ruby_pos = ev->param2;
				_change_rubypos_unit(p, TRUE);
				_update(p, 0);
			}
			break;
		//ルビ用グリフを使わない
		case WID_CK_NO_RUBY_GLYPH:
			pd->flags ^= DRAWTEXT_F_NO_RUBY_GLYPH;

			_update(p, 0);
			break;
		//回転 (edit)
		case WID_EDIT_ROTATE:
			if(ntype == MLINEEDIT_N_CHANGE)
			{
				pd->angle = mLineEditGetNum(p->edit_rotate);

				RotateWidget_setAngle(p->rotwg, pd->angle);
				
				_update(p, 0);
			}
			break;
		//回転ウィジェット
		case WID_ROTATE_CTRL:
			pd->angle = ev->param1;

			mLineEditSetNum(p->edit_rotate, pd->angle);

			_update(p, 0);
			break;

		//ヘルプ
		case WID_BTT_HELP:
			AppHelp_message(MLK_WINDOW(p), HELP_TRGROUP_SINGLE, HELP_TRID_TEXT_DIALOG);
			break;
	}
}

/* COMMAND イベント */

static void _event_command(_dialog *p,mEventCommand *ev)
{
	DrawTextData *pd = &APPDRAW->text.dt;

	switch(ev->id)
	{
		//モノクロ2値
		case TRID_MONO:
			pd->flags ^= DRAWTEXT_F_MONO;
			drawText_changeInfo(APPDRAW);
			break;
		//縦書き
		case TRID_VERT:
			pd->flags ^= DRAWTEXT_F_VERT;
			break;
		//特殊表記有効
		case TRID_ENABLE_FORMAT:
			pd->flags ^= DRAWTEXT_F_ENABLE_FORMAT;
			break;
		//太字化
		case TRID_BOLD:
			pd->flags ^= DRAWTEXT_F_BOLD;
			drawText_changeInfo(APPDRAW);
			break;
		//斜体化
		case TRID_ITALIC:
			pd->flags ^= DRAWTEXT_F_ITALIC;
			drawText_changeInfo(APPDRAW);
			break;
		//埋め込みビットマップ有効
		case TRID_ENABLE_BITMAP:
			pd->flags ^= DRAWTEXT_F_ENABLE_BITMAP;
			drawText_changeInfo(APPDRAW);
			break;
	}

	_update(p, 0);
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify((_dialog *)wg, (mEventNotify *)ev);
			break;

		//mIconBar
		case MEVENT_COMMAND:
			_event_command((_dialog *)wg, (mEventCommand *)ev);
			break;

		//タイマー (プレビュー更新)
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			drawText_drawPreview(APPDRAW);
			return 1;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//========================
// mMultiEdit
//========================


/* mMultiEdit イベントハンドラ */

static int _multiedit_event_handle(mWidget *wg,mEvent *ev)
{
	//Ctrl+上下左右で 1px 移動

	if(ev->type == MEVENT_KEYDOWN
		&& !ev->key.is_grab_pointer
		&& (ev->key.state & MLK_STATE_MASK_MODS) == MLK_STATE_CTRL)
	{
		uint8_t flag = 1;
	
		switch(ev->key.key)
		{
			case MKEY_LEFT:
			case MKEY_KP_LEFT:
				APPDRAW->w.pttmp[0].x--;
				break;
			case MKEY_RIGHT:
			case MKEY_KP_RIGHT:
				APPDRAW->w.pttmp[0].x++;
				break;
			case MKEY_UP:
			case MKEY_KP_UP:
				APPDRAW->w.pttmp[0].y--;
				break;
			case MKEY_DOWN:
			case MKEY_KP_DOWN:
				APPDRAW->w.pttmp[0].y++;
				break;
			default:
				flag = 0;
				break;
		}

		if(flag)
		{
			//プレビュー更新
			
			if(APPDRAW->text.fpreview)
				drawText_drawPreview(APPDRAW);
			
			return 1;
		}
	}

	return mMultiEditHandle_event(wg, ev);
}


//========================
// main
//========================


/* label + edit + combo 作成 */

static mLineEdit *_create_label_edit_combo(_dialog *p,mWidget *parent,const char *label,
	int edit_id,int combo_id,const char *combo_list,int combo_sel)
{
	mLineEdit *le;
	mComboBox *cb;
	mWidget *ct;

	mLabelCreate(parent, MLF_MIDDLE, 0, 0, label);

	//mLineEdit

	le = mLineEditCreate(parent, edit_id, MLF_MIDDLE, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

	mLineEditSetWidth_textlen(le, 8);

	//------

	if(edit_id == WID_EDIT_SIZE)
		ct = mContainerCreateHorz(parent, 5, 0, 0);
	else
		ct = parent;

	//mComboBox

	cb = mComboBoxCreate(ct, combo_id, MLF_MIDDLE, 0, 0);

	mComboBoxAddItems_sepnull(cb, combo_list, 0);
	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, combo_sel);

	if(edit_id == WID_EDIT_SIZE)
		p->cb_unit_fontsize = cb;

	//メニューボタン (フォントサイズ時)

	if(edit_id == WID_EDIT_SIZE)
	{
		mArrowButtonCreate_minsize(ct, WID_BTT_FONTSIZE_MENU, MLF_MIDDLE, 0,
			MARROWBUTTON_S_DOWN | MARROWBUTTON_S_FONTSIZE, 21);
	}

	return le;
}


//------------


/* フォント選択のウィジェット作成 */

static void _create_fontsel(_dialog *p,mWidget *parent)
{
	mWidget *ct;

	//一覧

	p->ct_fontsel[0] = ct = mContainerCreateVert(parent, 5, MLF_EXPAND_W, 0);

	p->cb_family = mComboBoxCreate(ct, WID_CB_FONTLIST, MLF_EXPAND_W, 0, 0);

	p->cb_style = mComboBoxCreate(ct, WID_CB_STYLE, MLF_EXPAND_W, 0, 0);

	//登録フォント

	p->ct_fontsel[1] = ct = mContainerCreateHorz(parent, 4, MLF_EXPAND_W, 0);

	p->cb_regfont = mComboBoxCreate(ct, WID_CB_FONTREG, MLF_EXPAND_W | MLF_MIDDLE, 0, 0);

	widget_createEditButton(ct, WID_BTT_FONTREG_EDIT, MLF_MIDDLE, 0);

	widget_createImgButton_1bitText(ct, WID_BTT_FONTREG_LISTEDIT, MLF_MIDDLE, 0,
		g_img_listedit, 15, 0);

	//ファイル指定

	p->ct_fontsel[2] = ct = mContainerCreateHorz(parent, 4, MLF_EXPAND_W, 0);

	p->edit_fontfile = mLineEditCreate(ct, 0, MLF_EXPAND_W | MLF_MIDDLE, 0, MLINEEDIT_S_READ_ONLY);

	mButtonCreate(ct, WID_BTT_FONTFILE_SEL, MLF_MIDDLE, 0, MBUTTON_S_REAL_W, "...");
}

/* "ルビ" グループの作成 */

static void _create_ruby_group(_dialog *p,mWidget *parent,DrawTextData *pdt)
{
	mWidget *ct,*ct2;

	ct = (mWidget *)mGroupBoxCreate(parent, 0, MLK_MAKE32_4(0,8,0,0), 0, MLK_TR(TRID_RUBY));

	//---- grid

	ct2 = mContainerCreateGrid(ct, 3, 6, 4, 0, 0);

	//ルビサイズ

	p->edit_rsize = _create_label_edit_combo(p, ct2, MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE),
		WID_EDIT_RUBYSIZE, WID_CB_RUBYSIZE_UNIT, "%\0pt\0px\0", pdt->unit_rubysize);

	_change_rubysize_unit(p, FALSE);

	//ルビ位置

	p->edit_rubypos = _create_label_edit_combo(p, ct2, MLK_TR(TRID_RUBY_POS), WID_EDIT_RUBY_POS,
		WID_CB_RUBY_POS, "%\0px\0", pdt->unit_ruby_pos);

	_change_rubypos_unit(p, FALSE);

	//----

	//ルビ用グリフを使わない

	mCheckButtonCreate(ct, WID_CK_NO_RUBY_GLYPH, 0, MLK_MAKE32_4(0,4,0,0), 0,
		MLK_TR(TRID_NO_RUBY_GLYPH), pdt->flags & DRAWTEXT_F_NO_RUBY_GLYPH);
}


//--------------


/* 上部のウィジェット作成 */

static void _create_top_widget(_dialog *p)
{
	mWidget *ct,*ct2;

	ct = mContainerCreateVert(MLK_WIDGET(p), 0, MLF_EXPAND_W | MLF_FIX_H, 0);

	p->ct_top = ct;

	//---- ボタン

	ct2 = mContainerCreateHorz(ct, 3, MLF_EXPAND_W, 0);

	//?

	widget_createHelpButton(ct2, WID_BTT_HELP, MLF_MIDDLE, 0);

	//コピー

	widget_createCopyButton(ct2, WID_BTT_COPY, MLF_MIDDLE, MLK_MAKE32_4(4,0,0,0), 0);

	//貼り付け

	widget_createPasteButton(ct2, WID_BTT_PASTE, MLF_MIDDLE | MLF_EXPAND_X, 0, 0);

	//OK/Cancel

	mButtonCreate(ct2, MLK_WID_OK, MLF_MIDDLE, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));

	mButtonCreate(ct2, MLK_WID_CANCEL, MLF_MIDDLE, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	//---- mMultiEdit

	p->medit = mMultiEditCreate(ct, WID_TEXTEDIT, MLF_EXPAND_WH, MLK_MAKE32_4(0,6,0,0),
		MMULTIEDIT_S_NOTIFY_CHANGE);

	p->medit->wg.event = _multiedit_event_handle;

	mWidgetSetFocus(MLK_WIDGET(p->medit));

	//---- プレビュー

	mCheckButtonCreate(ct, WID_CK_PREVIEW, MLF_RIGHT, MLK_MAKE32_4(0,3,0,0), 0,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_PREVIEW), APPDRAW->text.fpreview);
}

/* 下部のウィジェット作成 */

static void _create_bottom_widget(_dialog *p)
{
	mWidget *cttop,*ct;
	mTab *tab;
	mIconBar *ib;
	mComboBox *cb;
	mLineEdit *le;
	DrawTextData *pdt = &APPDRAW->text.dt;
	int i;

	//mContainerView

	p->ctview = mContainerViewNew(MLK_WIDGET(p), 0, 0);

	p->ctview->wg.flayout = MLF_EXPAND_WH;

	//中身コンテナ

	cttop = mContainerCreateVert(MLK_WIDGET(p->ctview), 0, MLF_EXPAND_W, 0);

	mContainerSetPadding_pack4(MLK_CONTAINER(cttop), MLK_MAKE32_4(0,0,6,0));

	mContainerViewSetPage(p->ctview, cttop);

	//---- mIconBar

	ib = mIconBarCreate(cttop, 0, 0, 0,
		MICONBAR_S_TOOLTIP | MICONBAR_S_BUTTON_FRAME | MICONBAR_S_DESTROY_IMAGELIST);

	mIconBarSetImageList(ib, mImageListCreate_1bit_textcol(g_img_icon, 16 * 6, 16, 16));
	mIconBarSetTooltipTrGroup(ib, TRGROUP_DLG_TEXT);

	for(i = 0; i < 6; i++)
	{
		mIconBarAdd(ib, TRID_MONO + i, i, TRID_MONO + i,
			MICONBAR_ITEMF_CHECKBUTTON | ((pdt->flags & (1<<(i+2)))? MICONBAR_ITEMF_CHECKED: 0));
	}

	//---- フォント (groupbox)

	ct = (mWidget *)mGroupBoxCreate(cttop, MLF_EXPAND_W, MLK_MAKE32_4(0,5,0,0), 0,
		MLK_TR(TRID_FONT));

	mContainerSetType_vert(MLK_CONTAINER(ct), 6);

	//タブ

	tab = mTabCreate(ct, WID_TAB_FONTSEL, MLF_EXPAND_W, 0, MTAB_S_TOP | MTAB_S_HAVE_SEP | MTAB_S_SHORTEN);

	for(i = 0; i < 3; i++)
		mTabAddItem(tab, MLK_TR(TRID_FONTLIST + i), -1, 0, i);

	mTabSetSel_atIndex(tab, pdt->fontsel);

	//フォント選択ウィジェット

	_create_fontsel(p, ct);

	_change_fontsel(p, FALSE);

	//----- サイズ/字間/行間/DPI

	ct = mContainerCreateGrid(cttop, 3, 6, 4, 0, MLK_MAKE32_4(0,8,0,0));

	//サイズ

	p->edit_size = _create_label_edit_combo(p, ct, MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE),
		WID_EDIT_SIZE, WID_CB_SIZE_UNIT, "pt\0px\0", pdt->unit_fontsize);

	_change_fontsize_unit(p, FALSE);

	//字間

	p->edit_charsp = _create_label_edit_combo(p, ct, MLK_TR(TRID_CHAR_SPACE), WID_EDIT_CHARSP,
		WID_CB_CHARSP_UNIT, "%\0px\0", pdt->unit_char_space);

	_change_charsp_unit(p, FALSE);

	//行間

	p->edit_linesp = _create_label_edit_combo(p, ct, MLK_TR(TRID_LINE_SPACE), WID_EDIT_LINESP,
		WID_CB_LINESP_UNIT, "%\0px\0", pdt->unit_line_space);

	_change_linesp_unit(p, FALSE);

	//DPI

	mCheckButtonCreate(ct, WID_CK_DPI, MLF_MIDDLE, 0, 0, "DPI",
		pdt->flags & DRAWTEXT_F_ENABLE_DPI);

	p->edit_dpi = mLineEditCreate(ct, WID_EDIT_DPI, MLF_MIDDLE, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

	mLineEditSetWidth_textlen(p->edit_dpi, 8);
	mWidgetEnable(MLK_WIDGET(p->edit_dpi), pdt->flags & DRAWTEXT_F_ENABLE_DPI);
	
	_set_lineedit_num(p->edit_dpi, 1, 10000, 0, pdt->dpi);

	//----- ヒンティング

	//ヒンティング

	ct = mContainerCreateHorz(cttop, 6, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0));

	mLabelCreate(ct, MLF_MIDDLE, 0, 0, MLK_TR(TRID_HINTING));

	cb = mComboBoxCreate(ct, WID_CB_HINTING, 0, 0, 0);

	mComboBoxAddItems_sepnull(cb, "none\0slight\0middle\0", 0);
	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, pdt->hinting);

	//オートヒント無効

	mCheckButtonCreate(cttop, WID_CK_AUTOHINT_DISABLE, 0, MLK_MAKE32_4(0,6,0,0), 0,
		MLK_TR(TRID_DISABLE_AUTOHINT), pdt->flags & DRAWTEXT_F_DISABLE_AUTOHINT);

	//---- ルビ

	_create_ruby_group(p, cttop, pdt);

	//---- 回転

	//edit

	ct = mContainerCreateHorz(cttop, 6, 0, MLK_MAKE32_4(0,10,0,0));

	mLabelCreate(ct, MLF_MIDDLE, 0, 0, MLK_TR(TRID_ROTATE));

	p->edit_rotate = le = mLineEditCreate(ct, WID_EDIT_ROTATE, 0, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

	mLineEditSetWidth_textlen(le, 7);
	mLineEditSetNumStatus(le, -360, 360, 0);
	mLineEditSetNum(le, pdt->angle);

	//回転ウィジェット

	p->rotwg = RotateWidget_new(cttop, WID_ROTATE_CTRL, MLK_MAKE32_4(0,10,0,0), 100);

	RotateWidget_setAngle(p->rotwg, pdt->angle);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent)
{
	_dialog *p;

	MLK_TRGROUP(TRGROUP_DLG_TEXT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);
	
	if(!p) return NULL;

	//パネルフォントを使う

	p->wg.font = APPWIDGET->font_panel;

	//上部

	_create_top_widget(p);

	//mSplitter

	mSplitterNew(MLK_WIDGET(p), 0, MSPLITTER_S_HORZ);

	//下部 (mContainerView)

	_create_bottom_widget(p);

	//デフォルトテキスト

	if(mStrIsnotEmpty(&APPDRAW->text.dt.str_text))
	{
		mMultiEditSetText(p->medit, APPDRAW->text.dt.str_text.buf);
		mMultiEditSelectAll(p->medit);

		_update(p, 0);
	}

	return p;
}

/* OK 時 */

static void _end_ok(DrawTextData *dt)
{
	int val;

	//テキストサイズの履歴を追加

	val = dt->fontsize;

	if(dt->unit_fontsize == DRAWTEXT_UNIT_FONTSIZE_PX)
		val |= 1<<15;

	mAddRecentVal_16bit(APPCONF->textsize_recent, CONFIG_TEXTSIZE_RECENT_NUM,
		val, 0);
}

/** テキストダイアログ */

mlkbool TextDialog_run(void)
{
	_dialog *p;
	mToplevelSaveState *pst;
	int ret;

	p = _create_dialog(MLK_WINDOW(APPWIDGET->mainwin));
	if(!p) return FALSE;

	//ウィンドウ状態セット

	p->ct_top->h = APPCONF->textdlg_toph;

	pst = &APPCONF->winstate_textdlg;

	if(pst->w == 0 || pst->h == 0)
		//初期状態
		mWidgetResize(MLK_WIDGET(p), 400, 500);
	else
		mToplevelSetSaveState(MLK_TOPLEVEL(p), pst);

	mWidgetShow(MLK_WIDGET(p), 1);

	//実行

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		_end_ok(&APPDRAW->text.dt);

	//ウィンドウ状態保存

	mToplevelGetSaveState(MLK_TOPLEVEL(p), &APPCONF->winstate_textdlg);

	APPCONF->textdlg_toph = p->ct_top->h;

	//

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


