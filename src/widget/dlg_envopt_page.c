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
 * 環境設定のページ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pager.h"
#include "mlk_groupbox.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_fontbutton.h"
#include "mlk_combobox.h"
#include "mlk_fileinput.h"
#include "mlk_event.h"
#include "mlk_str.h"

#include "def_config.h"

#include "widget_func.h"
#include "apphelp.h"

#include "trid.h"

#include "pv_envoptdlg.h"


/*
  - 「ツールバーのカスタマイズ」のダイアログを実行した場合など、
    文字列グループが変更される場合があるため、ページ作成前には常にグループがセットされている。
*/

//----------------------

mlkbool ToolBarCustomizeDlg_run(mWindow *parent,uint8_t **ppbuf,int *dst_size);

#define _MARGIN_LEFT  0x08000000

//----------------------


//**********************************
// 設定1
//**********************************

typedef struct
{
	mColorButton *colbtt[4];
	mLineEdit *edit_rulecol,
		*edit_undonum,
		*edit_undobuf,
		*edit_zoom_step,
		*edit_rotate_step;
	mCheckButton *ck_bits8;
}_pagedata_opt1;

enum
{
	WID_OPT1_HELP_UNDOBUF = 100
};


/* アンドゥバッファサイズ、単位付きのテキストセット */

static void _opt1_set_undo_bufsize(mLineEdit *le,int size)
{
	mStr str = MSTR_INIT;

	if(size < 1024)
		mStrSetInt(&str, size);
	else if(size < 1024 * 1024)
		mStrSetFormat(&str, "%.2FK", size * 100 / 1024);
	else if(size < 1024 * 1024 * 1024)
		mStrSetFormat(&str, "%.2FM", (int)((double)size / (1024 * 1024) * 100 + 0.5));
	else
		mStrSetFormat(&str, "%.2FG", (int)((double)size / (1024 * 1024 * 1024) * 100 + 0.5));

	mLineEditSetText(le, str.buf);

	mStrFree(&str);
}

/* アンドゥバッファサイズ取得 */

static int _opt1_get_bufsize(mLineEdit *edit)
{
	mStr str = MSTR_INIT;
	double d;
	char c;
	int ret;

	//double 値取得

	d = mLineEditGetDouble(edit);
	if(d < 0) d = 0;

	//末尾の単位

	mLineEditGetTextStr(edit, &str);

	c = mStrGetLastChar(&str);

	if(c == 'K' || c == 'k')
		d *= 1024;
	else if(c == 'M' || c == 'm')
		d *= 1024 * 1024;
	else if(c == 'G' || c == 'g')
		d *= 1024 * 1024 * 1024;

	//最大1G

	if(d > 1024 * 1024 * 1024)
		d = 1024 * 1024 * 1024;
	
	ret = (int)(d + 0.5);

	mStrFree(&str);

	return ret;
}

/* データ取得 */

static mlkbool _opt1_getdata(mPager *p,void *pagedat,void *dst)
{
	_pagedata_opt1 *pd = (_pagedata_opt1 *)pagedat;
	EnvOptData *dat = (EnvOptData *)dst;
	int i;

	dat->col_canvasbk = mColorButtonGetColor(pd->colbtt[0]);

	for(i = 0; i < 2; i++)
		dat->col_plaid[i] = mColorButtonGetColor(pd->colbtt[1 + i]);

	dat->col_ruleguide = mColorButtonGetColor(pd->colbtt[3])
		| ((int)(mLineEditGetNum(pd->edit_rulecol) / 100.0 * 128 + 0.5) << 24);

	dat->img_defbits = (mCheckButtonIsChecked(pd->ck_bits8))? 8: 16;

	dat->undo_maxnum = mLineEditGetNum(pd->edit_undonum);

	dat->undo_maxbufsize = _opt1_get_bufsize(pd->edit_undobuf);

	dat->canv_zoom_step = mLineEditGetNum(pd->edit_zoom_step);

	dat->canv_rotate_step = mLineEditGetNum(pd->edit_rotate_step);

	return TRUE;
}

/* イベント */

static int _opt1_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			case WID_OPT1_HELP_UNDOBUF:
				AppHelp_message(p->wg.toplevel, HELP_TRGROUP_SINGLE, HELP_TRID_ENVOPT_UNDOBUFSIZE);
				break;
		}
	}

	return 1;
}

/* 左に余白をセット */

static void _widget_set_margin(mWidget *wg)
{
	mWidgetSetMargin_pack4(wg, _MARGIN_LEFT);
}

/** "設定1" ページ作成 */

mlkbool EnvOpt_createPage_opt1(mPager *p,mPagerInfo *info)
{
	_pagedata_opt1 *pd;
	EnvOptData *dat;
	mWidget *ct,*ct2;
	int i;

	pd = (_pagedata_opt1 *)mMalloc0(sizeof(_pagedata_opt1));
	if(!pd) return FALSE;

	info->pagedat = pd;
	info->getdata = _opt1_getdata;
	info->event = _opt1_event;

	dat = (EnvOptData *)mPagerGetDataPtr(p);

	ct = EnvOpt_createContainerView(p);

	//--------------

	//キャンバス背景色

	widget_createLabel_trid(ct, TRID_OPT1_CANVAS_BKGND_COL);

	pd->colbtt[0] = mColorButtonCreate(ct, 0, 0, _MARGIN_LEFT, MCOLORBUTTON_S_DIALOG, dat->col_canvasbk);

	//チェック柄背景色

	widget_createLabel_trid(ct, TRID_OPT1_PLAID_COL);

	ct2 = mContainerCreateHorz(ct, 5, 0, _MARGIN_LEFT);

	for(i = 0; i < 2; i++)
		pd->colbtt[1 + i] = mColorButtonCreate(ct2, 0, 0, 0, MCOLORBUTTON_S_DIALOG, dat->col_plaid[i]);

	//定規ガイドの色

	widget_createLabel_trid(ct, TRID_OPT1_RULE_GUIDE_COL);

	ct2 = mContainerCreateHorz(ct, 5, 0, _MARGIN_LEFT);

	pd->colbtt[3] = mColorButtonCreate(ct2, 0, 0, 0, MCOLORBUTTON_S_DIALOG, dat->col_ruleguide);

	widget_createLabel(ct2, MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY));

	pd->edit_rulecol = widget_createEdit_num(ct2, 5, 0, 100, 0,
		(int)((dat->col_ruleguide >> 24) / 128.0 * 100 + 0.5));

	//画像読み込み時のデフォルトビット数

	widget_createLabel_trid(ct, TRID_OPT1_IMG_DEFBITS);

	ct2 = mContainerCreateHorz(ct, 5, 0, _MARGIN_LEFT);

	pd->ck_bits8 = mCheckButtonCreate(ct2, 0, 0, 0, MCHECKBUTTON_S_RADIO, "8bit",
		(dat->img_defbits == 8));

	mCheckButtonCreate(ct2, 0, 0, 0, MCHECKBUTTON_S_RADIO, "16bit",
		(dat->img_defbits == 16));

	//アンドゥ最大回数

	widget_createLabel_trid(ct, TRID_OPT1_UNDO_MAXNUM);

	pd->edit_undonum = widget_createEdit_num(ct, 6, 2, 400, 0, dat->undo_maxnum);

	_widget_set_margin(MLK_WIDGET(pd->edit_undonum));

	//アンドゥ最大バッファ

	widget_createLabel_trid(ct, TRID_OPT1_UNDO_MAXBUFSIZE);

	ct2 = mContainerCreateHorz(ct, 5, MLF_EXPAND_W, _MARGIN_LEFT);

	pd->edit_undobuf = mLineEditCreate(ct2, 0, MLF_EXPAND_W, 0, 0);

	_opt1_set_undo_bufsize(pd->edit_undobuf, dat->undo_maxbufsize);

	widget_createHelpButton(ct2, WID_OPT1_HELP_UNDOBUF, MLF_MIDDLE, 0);

	//キャンバス表示倍率の1段階

	pd->edit_zoom_step = widget_createLabelEditNum(ct, MLK_TR(TRID_OPT1_CANVAS_ZOOM_STEP), 5, 1, 1000, dat->canv_zoom_step);

	_widget_set_margin(MLK_WIDGET(pd->edit_zoom_step));

	//キャンバス回転の1段階

	pd->edit_rotate_step = widget_createLabelEditNum(ct, MLK_TR(TRID_OPT1_CANVAS_ROTATE_STEP), 5, 1, 180, dat->canv_rotate_step);

	_widget_set_margin(MLK_WIDGET(pd->edit_rotate_step));

	return TRUE;
}


//**********************************
// フラグ
//**********************************

#define FLAGS_CKNUM  4

typedef struct
{
	mCheckButton *ck[FLAGS_CKNUM];
}_pagedata_flags;


/* データ取得 */

static mlkbool _flags_getdata(mPager *p,void *pagedat,void *dst)
{
	_pagedata_flags *pd = (_pagedata_flags *)pagedat;
	EnvOptData *dat = (EnvOptData *)dst;
	int i;
	uint32_t flags = 0;

	for(i = 0; i < FLAGS_CKNUM; i++)
	{
		if(mCheckButtonIsChecked(pd->ck[i]))
			flags |= 1 << i;
	}

	dat->foption = flags;

	return TRUE;
}

/** "フラグ" ページ作成 */

mlkbool EnvOpt_createPage_flags(mPager *p,mPagerInfo *info)
{
	_pagedata_flags *pd;
	EnvOptData *dat;
	int i;
	uint32_t flags;

	pd = (_pagedata_flags *)mMalloc0(sizeof(_pagedata_flags));
	if(!pd) return FALSE;

	info->pagedat = pd;
	info->getdata = _flags_getdata;

	dat = (EnvOptData *)mPagerGetDataPtr(p);

	//

	flags = dat->foption;

	for(i = 0; i < FLAGS_CKNUM; i++)
	{
		pd->ck[i] = mCheckButtonCreate(MLK_WIDGET(p), 0, 0, MLK_MAKE32_4(0,0,0,4), 0,
			MLK_TR(TRID_FLAGS_TOP + i), flags & (1 << i));
	}

	return TRUE;
}


//**********************************
// インターフェイス
//**********************************

typedef struct
{
	mCheckButton *ck_fontpanel;
	mFontButton *fontbtt_panel;
	mComboBox *cb_icon[3];
}_pagedata_interface;

enum
{
	WID_INTERFACE_TOOLBAR_OPT = 100
};


/* データ取得 */

static mlkbool _interface_getdata(mPager *p,void *pagedat,void *dst)
{
	_pagedata_interface *pd = (_pagedata_interface *)pagedat;
	EnvOptData *dat = (EnvOptData *)dst;
	int i;

	//パネルフォント

	if(mCheckButtonIsChecked(pd->ck_fontpanel))
		mFontButtonGetInfo_text(pd->fontbtt_panel, &dat->str_font_panel);
	else
		mStrEmpty(&dat->str_font_panel);

	//アイコンサイズ

	for(i = 0; i < 3; i++)
		dat->iconsize[i] = mComboBoxGetItemParam(pd->cb_icon[i], -1);
	
	return TRUE;
}

/* イベント */

static int _interface_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		EnvOptData *dat = (EnvOptData *)mPagerGetDataPtr(p);

		switch(ev->notify.id)
		{
			//ツールバーのカスタマイズ
			case WID_INTERFACE_TOOLBAR_OPT:
				if(ToolBarCustomizeDlg_run(p->wg.toplevel,
					&dat->toolbar_btts, &dat->toolbar_btts_size))
				{
					dat->fchange |= ENVOPT_CHANGE_F_TOOLBAR_BTTS;
				}
				break;
		}
	}

	return 1;
}

/** "インターフェイス" ページ作成 */

mlkbool EnvOpt_createPage_interface(mPager *p,mPagerInfo *info)
{
	_pagedata_interface *pd;
	EnvOptData *dat;
	mWidget *ct,*ct2;
	mComboBox *cb;
	int i,iconsize[] = {16,20,24};

	pd = (_pagedata_interface *)mMalloc0(sizeof(_pagedata_interface));
	if(!pd) return FALSE;

	info->pagedat = pd;
	info->getdata = _interface_getdata;
	info->event = _interface_event;

	dat = (EnvOptData *)mPagerGetDataPtr(p);

	ct = EnvOpt_createContainerView(p);

	//-------

	//パネルフォント

	pd->ck_fontpanel = mCheckButtonCreate(ct, 0, 0, 0, 0,
		MLK_TR(TRID_INTERFACE_FONT_PANEL), mStrIsnotEmpty(&dat->str_font_panel));

	pd->fontbtt_panel = mFontButtonCreate(ct, 0, MLF_EXPAND_W, 0, MFONTBUTTON_S_ALL_INFO);

	mFontButtonSetInfo_text(pd->fontbtt_panel, dat->str_font_panel.buf);

	//アイコンサイズ

	ct2 = (mWidget *)mGroupBoxCreate(ct, 0, MLK_MAKE32_4(0,8,0,0), 0, MLK_TR(TRID_INTERFACE_ICONSIZE));

	mContainerSetType_grid(MLK_CONTAINER(ct2), 2, 6, 7);

	for(i = 0; i < 3; i++)
	{
		widget_createLabel_trid(ct2, TRID_INTERFACE_ICON_TOOLBAR + i);

		pd->cb_icon[i] = cb = mComboBoxCreate(ct2, 0, 0, 0, 0);

		mComboBoxAddItems_sepnull_arrayInt(cb, "16px\00020px\00024px\000", iconsize);
		mComboBoxSetAutoWidth(cb);
		mComboBoxSetSelItem_fromParam(cb, dat->iconsize[i]);
	}

	//ツールバーのカスタマイズ

	mButtonCreate(ct, WID_INTERFACE_TOOLBAR_OPT, 0, MLK_MAKE32_4(0,8,0,0), 0,
		MLK_TR(TRID_INTERFACE_TOOLBAR_OPT));

	return TRUE;
}


//**********************************
// システム
//**********************************

typedef struct
{
	mFileInput *finput[4];
	mLineEdit *edit_cpos[2];
}_pagedata_system;


/* データ取得 */

static mlkbool _system_getdata(mPager *p,void *pagedat,void *dst)
{
	_pagedata_system *pd = (_pagedata_system *)pagedat;
	EnvOptData *dat = (EnvOptData *)dst;
	int i;

	for(i = 0; i < 3; i++)
		mFileInputGetPath(pd->finput[i], dat->str_sysdir + i);

	mFileInputGetPath(pd->finput[3], &dat->str_cursor_file);

	for(i = 0; i < 2; i++)
		dat->cursor_hotspot[i] = mLineEditGetNum(pd->edit_cpos[i]);
	
	return TRUE;
}

/** "システム" ページ作成 */

mlkbool EnvOpt_createPage_system(mPager *p,mPagerInfo *info)
{
	_pagedata_system *pd;
	EnvOptData *dat;
	mWidget *ct,*ct2,*ct3;
	int i;

	pd = (_pagedata_system *)mMalloc0(sizeof(_pagedata_system));
	if(!pd) return FALSE;

	info->pagedat = pd;
	info->getdata = _system_getdata;

	dat = (EnvOptData *)mPagerGetDataPtr(p);

	ct = EnvOpt_createContainerView(p);

	//-------

	//ディレクトリ

	for(i = 0; i < 3; i++)
	{
		widget_createLabel_trid(ct, TRID_SYSTEM_TEMPDIR + i);

		pd->finput[i] = mFileInputCreate_dir(ct, 0, MLF_EXPAND_W, 0, 0, dat->str_sysdir[i].buf);
	}

	//---- 描画カーソル

	ct2 = (mWidget *)mGroupBoxCreate(ct, MLF_EXPAND_W, MLK_MAKE32_4(0,8,0,0), 0, MLK_TR(TRID_SYSTEM_DRAWCURSOR));

	mContainerSetType_vert(MLK_CONTAINER(ct2), 4);

	//画像ファイル

	widget_createLabel_trid(ct2, TRID_SYSTEM_CURSOR_FILE);

	pd->finput[3] = mFileInputCreate_file(ct2, 0, MLF_EXPAND_W, 0, 0,
		"PNG file (*.png)\t*.png\tAll files\t*", 0, APPCONF->strFileDlgDir.buf);

	mFileInputSetPath(pd->finput[3], dat->str_cursor_file.buf);

	//中心位置

	widget_createLabel_trid(ct2, TRID_SYSTEM_HOTSPOT);

	ct3 = mContainerCreateGrid(ct2, 2, 7, 6, 0, MLK_MAKE32_4(0,3,0,0));

	pd->edit_cpos[0] = widget_createLabelEditNum(ct3, "X", 5, 0, 1000, dat->cursor_hotspot[0]);
	pd->edit_cpos[1] = widget_createLabelEditNum(ct3, "Y", 5, 0, 1000, dat->cursor_hotspot[1]);

	return TRUE;
}

