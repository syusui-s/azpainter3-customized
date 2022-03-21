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
 * 環境設定ダイアログ
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_listview.h"
#include "mlk_containerview.h"
#include "mlk_pager.h"
#include "mlk_event.h"
#include "mlk_columnitem.h"
#include "mlk_str.h"
#include "mlk_sysdlg.h"
#include "mlk_file.h"

#include "def_config.h"
#include "def_draw.h"

#include "undo.h"
#include "draw_main.h"
#include "draw_rule.h"

#include "widget_func.h"

#include "trid.h"

#include "pv_envoptdlg.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mPager *pager;

	EnvOptData dat;
}_dialog;

#define WID_LIST  100

enum
{
	LISTNO_OPT1,
	LISTNO_FLAGS,
	LISTNO_BTT,
	LISTNO_INTERFACE,
	LISTNO_SYSTEM,

	LIST_NUM
};

//----------------------

mlkbool EnvOpt_createPage_opt1(mPager *p,mPagerInfo *info);
mlkbool EnvOpt_createPage_flags(mPager *p,mPagerInfo *info);
mlkbool EnvOpt_createPage_btt(mPager *p,mPagerInfo *info);
mlkbool EnvOpt_createPage_interface(mPager *p,mPagerInfo *info);
mlkbool EnvOpt_createPage_system(mPager *p,mPagerInfo *info);

static const mFuncPagerCreate g_func_page[] = {
	EnvOpt_createPage_opt1, EnvOpt_createPage_flags, EnvOpt_createPage_btt,
	EnvOpt_createPage_interface, EnvOpt_createPage_system
};

//----------------------


//=============================
// EnvOptData
//=============================


/* 設定データから作業用データにセット */

static void _config_to_data(EnvOptData *pd,AppConfig *cf)
{
	//設定1

	pd->col_canvasbk = cf->canvasbkcol;
	pd->col_plaid[0] = RGBcombo_to_32bit(APPDRAW->col.checkbkcol);
	pd->col_plaid[1] = RGBcombo_to_32bit(APPDRAW->col.checkbkcol + 1);
	pd->col_ruleguide = cf->rule_guide_col;
	pd->img_defbits = cf->loadimg_default_bits;
	pd->undo_maxnum = cf->undo_maxnum;
	pd->undo_maxbufsize = cf->undo_maxbufsize;
	pd->canv_zoom_step = cf->canvas_zoom_step_hi;
	pd->canv_rotate_step = cf->canvas_angle_step;

	//フラグ

	pd->foption = cf->foption;

	//操作

	memcpy(pd->btt_default, cf->pointer_btt_default, CONFIG_POINTERBTT_NUM);
	memcpy(pd->btt_pentab, cf->pointer_btt_pentab, CONFIG_POINTERBTT_NUM);

	//インターフェイス

	mStrCopy(&pd->str_font_panel, &cf->strFont_panel);

	pd->iconsize[0] = cf->iconsize_toolbar;
	pd->iconsize[1] = cf->iconsize_panel_tool;
	pd->iconsize[2] = cf->iconsize_other;

	pd->toolbar_btts = mMemdup(cf->toolbar_btts, cf->toolbar_btts_size);
	pd->toolbar_btts_size = cf->toolbar_btts_size;

	//システム

	mStrCopy(pd->str_sysdir, &cf->strTempDir);
	mStrCopy(pd->str_sysdir + 1, &cf->strUserBrushDir);
	mStrCopy(pd->str_sysdir + 2, &cf->strUserTextureDir);

	mStrCopy(&pd->str_cursor_file, &cf->strCursorFile);

	pd->cursor_hotspot[0] = cf->cursor_hotspot[0];
	pd->cursor_hotspot[1] = cf->cursor_hotspot[1];
}

/* 作業用データから設定データにセット
 *
 * return: 更新フラグ */

static int _data_to_config(AppConfig *cf,EnvOptData *pd)
{
	int ret = 0;

	//------- 値の変化

	//キャンバス背景色

	if(cf->canvasbkcol != pd->col_canvasbk)
		ret |= 1<<0;

	//チェック柄背景色

	if((cf->fview & CONFIG_VIEW_F_BKGND_PLAID)
		&& (pd->col_plaid[0] != RGBcombo_to_32bit(APPDRAW->col.checkbkcol)
			|| pd->col_plaid[1] != RGBcombo_to_32bit(APPDRAW->col.checkbkcol + 1)))
	{
		ret |= 1<<1;
	}

	//定規ガイドの色

	if(drawRule_isVisibleGuide(APPDRAW)
		&& pd->col_ruleguide != cf->rule_guide_col)
	{
		ret |= 1<<0;
	}

	//描画カーソル
	// :カーソルはダイアログ後に変更する。

	if(!mStrCompareEq(&pd->str_cursor_file, cf->strCursorFile.buf)
		|| pd->cursor_hotspot[0] != cf->cursor_hotspot[0]
		|| pd->cursor_hotspot[1] != cf->cursor_hotspot[1])
	{
		ret |= 1<<3;
	}

	//--------- 値のセット

	//設定1

	cf->canvasbkcol = pd->col_canvasbk;

	RGB32bit_to_RGBcombo(APPDRAW->col.checkbkcol, pd->col_plaid[0]);
	RGB32bit_to_RGBcombo(APPDRAW->col.checkbkcol + 1, pd->col_plaid[1]);

	cf->rule_guide_col = pd->col_ruleguide;
	cf->loadimg_default_bits = pd->img_defbits;
	cf->undo_maxnum = pd->undo_maxnum;
	cf->undo_maxbufsize = pd->undo_maxbufsize;
	cf->canvas_zoom_step_hi = pd->canv_zoom_step;
	cf->canvas_angle_step = pd->canv_rotate_step;
	
	//フラグ

	cf->foption = pd->foption;

	//操作

	memcpy(cf->pointer_btt_default, pd->btt_default, CONFIG_POINTERBTT_NUM);
	memcpy(cf->pointer_btt_pentab, pd->btt_pentab, CONFIG_POINTERBTT_NUM);

	//インターフェイス

	mStrCopy(&cf->strFont_panel, &pd->str_font_panel);

	cf->iconsize_toolbar = pd->iconsize[0];
	cf->iconsize_panel_tool = pd->iconsize[1];
	cf->iconsize_other = pd->iconsize[2];

	if(pd->fchange & ENVOPT_CHANGE_F_TOOLBAR_BTTS)
	{
		mFree(cf->toolbar_btts);

		cf->toolbar_btts = mMemdup(pd->toolbar_btts, pd->toolbar_btts_size);
		cf->toolbar_btts_size = pd->toolbar_btts_size;

		ret |= 1<<2;
	}

	//システム

	mStrCopy(&cf->strTempDir, pd->str_sysdir);
	mStrCopy(&cf->strUserBrushDir, pd->str_sysdir + 1);
	mStrCopy(&cf->strUserTextureDir, pd->str_sysdir + 2);

	mStrCopy(&cf->strCursorFile, &pd->str_cursor_file);

	cf->cursor_hotspot[0] = pd->cursor_hotspot[0];
	cf->cursor_hotspot[1] = pd->cursor_hotspot[1];

	//-------- 値変更によるセット

	//アンドゥ回数セット

	Undo_setMaxNum(cf->undo_maxnum);

	return ret;
}


//=============================
// sub
//=============================


/** 内容をコンテナビューで作成
 *
 * 中身のコンテナは、デフォルトで垂直コンテナ */

mWidget *EnvOpt_createContainerView(mPager *pager)
{
	mContainerView *p;
	mWidget *ct;
	
	p = mContainerViewNew(MLK_WIDGET(pager), 0, 0); 

	p->wg.flayout = MLF_EXPAND_WH;

	//コンテナ

	ct = mContainerCreateVert(MLK_WIDGET(p), 4, MLF_EXPAND_W, 0);

	mContainerSetPadding_pack4(MLK_CONTAINER(ct), MLK_MAKE32_4(0,0,5,0));

	mContainerViewSetPage(p, ct);

	return (mWidget *)ct;
}


//=============================
// main
//=============================


/* ページ作成 */

static void _create_page(_dialog *p,int no)
{
	//[!] ダイアログなどで文字列グループが変更される場合があるため、
	//    ページ作成前は常にグループを変更する。

	MLK_TRGROUP(TRGROUP_DLG_ENVOPT);

	mPagerSetPage(p->pager, g_func_page[no]);

	if(!mWidgetResize_calchint_larger(MLK_WIDGET(p->wg.toplevel)))
		mWidgetLayout_redraw(MLK_WIDGET(p->pager));
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//リスト選択変更
			case WID_LIST:
				if(ev->notify.notify_type == MLISTVIEW_N_CHANGE_FOCUS)
				{
					_create_page(p, MLK_COLUMNITEM(ev->notify.param1)->param);
				}
				break;
			
			//OK
			case MLK_WID_OK:
				//現在の内容データを取得

				mPagerGetPageData(p->pager);

				//作業ディレクトリが存在するか

				if(!mIsExistDir(p->dat.str_sysdir[0].buf))
				{
					mMessageBoxErrTr(MLK_WINDOW(wg), TRGROUP_DLG_ENVOPT, TRID_MES_TEMPDIR);
					break;
				}
				
				mDialogEnd(MLK_DIALOG(wg), _data_to_config(APPCONF, &p->dat));
				break;
			//キャンセル
			case MLK_WID_CANCEL:
				mDialogEnd(MLK_DIALOG(wg), 0);
				break;
		}
	}

	return mDialogEventDefault(wg, ev);
}

/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	EnvOptData *p = &((_dialog *)wg)->dat;

	mStrArrayFree(p->str_sysdir, 3);

	mStrFree(&p->str_font_panel);
	mStrFree(&p->str_cursor_file);
	
	mFree(p->toolbar_btts);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent)
{
	_dialog *p;
	mWidget *ct;
	mListView *list;
	int i;

	MLK_TRGROUP(TRGROUP_DLG_ENVOPT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);

	if(!p) return NULL;

	p->wg.destroy = _destroy_handle;

	//設定をコピー

	_config_to_data(&p->dat, APPCONF);

	//----- リストと内容

	ct = mContainerCreateHorz(MLK_WIDGET(p), 12, MLF_EXPAND_WH, 0);

	//リスト

	list = mListViewCreate(ct, WID_LIST, MLF_EXPAND_H, 0,
		0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	for(i = 0; i < LIST_NUM; i++)
		mListViewAddItem_text_static_param(list, MLK_TR(TRID_LIST_OPT1 + i), i);

	mListViewSetItemHeight_min(list, mWidgetGetFontHeight(MLK_WIDGET(p)) + 2);
	mListViewSetAutoWidth(list, TRUE);
	mListViewSetFocusItem_index(list, 0);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 0, 22);

	//mPager

	p->pager = mPagerCreate(ct, MLF_EXPAND_WH, 0);

	mPagerSetDataPtr(p->pager, &p->dat);

	//----- 下ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,15,0,0));

	mLabelCreate(ct, MLF_EXPAND_X | MLF_BOTTOM, 0, 0, MLK_TR(TRID_HELP_RESTART));

	mContainerAddButtons_okcancel(ct);

	//---- 初期ページ

	mPagerSetPage(p->pager, EnvOpt_createPage_opt1);

	return p;
}

/** 環境設定ダイアログ
 *
 * return: 更新フラグ */

int EnvOptionDlg_run(mWindow *parent)
{
	_dialog *p;

	p = _create_dialog(parent);
	if(!p) return 0;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

