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
 * (Panel)カラーパレットの各ページ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_pager.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_sysdlg.h"
#include "mlk_list.h"
#include "mlk_str.h"

#include "def_config.h"
#include "def_draw.h"

#include "changecol.h"
#include "palettelist.h"
#include "image32.h"
#include "filedialog.h"
#include "appresource.h"
#include "apphelp.h"

#include "trid.h"
#include "trid_colorpalette.h"

#include "pv_panel_colpal.h"


//---------------

/* カラーパレット上のボタンが押された時、mPager に NOTIFY イベントが送られる。
 * 
 * [メニューボタン] id = -1, widget_from = メニューボタン
 * [移動ボタン] id = -2, param1 = 0:left, 1:right */

enum
{
	_NOTIFY_ID_MENUBTT = -1,
	_NOTIFY_ID_BTT_MOVE = -2
};

//---------------

//カラーパレットメニュー
static const uint16_t g_menudat_colpal[] = {
	TRID_MENU_COLPAL_GROUP,
	MMENU_ARRAY16_SUB_START,
		TRID_MENU_COLPAL_PALETTE | MMENU_ARRAY16_RADIO,
		TRID_MENU_COLPAL_HSL | MMENU_ARRAY16_RADIO,
		TRID_MENU_COLPAL_GRAD | MMENU_ARRAY16_RADIO,
		MMENU_ARRAY16_SEP,
		TRID_MENU_COLPAL_COMPACT,
	MMENU_ARRAY16_SUB_END,
	MMENU_ARRAY16_END
};

//パレットメニュー
static const uint16_t g_menudat_pal[] = {
	TRID_PAL_PALLIST, TRID_PAL_OPTION, MMENU_ARRAY16_SEP,
	TRID_PAL_EDIT,
	MMENU_ARRAY16_SUB_START,
		TRID_PAL_EDIT_PALETTE, TRID_PAL_EDIT_ALL_DRAWCOL,
	MMENU_ARRAY16_SUB_END,
	TRID_PAL_FILE,
	MMENU_ARRAY16_SUB_START,
		TRID_PAL_FILE_LOAD, TRID_PAL_FILE_LOAD_APPEND, TRID_PAL_FILE_LOAD_IMAGE, MMENU_ARRAY16_SEP,
		TRID_PAL_FILE_SAVE,
	MMENU_ARRAY16_SUB_END,
	MMENU_ARRAY16_SEP, TRID_PAL_HELP,
	MMENU_ARRAY16_END
};

//---------------

//各ウィジェット

mWidget *ColorPaletteView_new(mWidget *parent,int id,mListItem *item);
void ColorPaletteView_update(mWidget *wg,mListItem *item);

mWidget *HSLPalette_new(mWidget *parent);

mWidget *GradationBar_new(mWidget *parent,int id,uint32_t col_left,uint32_t col_right,int step);
void GradationBar_setStep(mWidget *wg,int step);

//ダイアログ

mlkbool PanelColorPalette_dlg_gradopt(mWindow *parent);
void PanelColorPalette_dlg_pallist(mWindow *parent);
mlkbool PanelColorPalette_dlg_palopt(mWindow *parent);

mlkbool PanelColorPalette_dlg_paledit(mWindow *parent,PaletteListItem *pi);

//---------------


//==============================
// メニュー (共通)
//==============================


/* メニュー実行
 *
 * 共通の項目は、処理される。
 *
 * addmenu: NULL 以外で、メニューを追加する関数
 * return: 追加項目の選択ID。-1 でキャンセル、-2 で処理済み */

static int _common_run_menu(mWidget *menubtt,void (*addmenu)(mMenu *))
{
	mMenu *menu;
	mMenuItem *mi;
	mBox box;
	int no;

	//----- メニュー

	MLK_TRGROUP(TRGROUP_PANEL_COLOR_PALETTE);

	menu = mMenuNew();

	//カラーパレット

	mMenuAppendTrText_array16_radio(menu, g_menudat_colpal);

	mMenuSetItemCheck(menu, TRID_MENU_COLPAL_PALETTE + APPCONF->panel.colpal_type, 1);
	mMenuSetItemCheck(menu, TRID_MENU_COLPAL_COMPACT, APPCONF->panel.colpal_flags & CONFIG_PANEL_COLPAL_F_COMPACT);

	//追加メニュー

	if(addmenu)
	{
		mMenuAppendSep(menu);

		(addmenu)(menu);
	}

	//

	mWidgetGetBox_rel(menubtt, &box);

	mi = mMenuPopup(menu, menubtt, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_RIGHT | MPOPUP_F_FLIP_XY, NULL);

	no = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(no == -1) return -1;

	//-------- 処理

	switch(no)
	{
		//タイプ切り替え
		case TRID_MENU_COLPAL_PALETTE:
		case TRID_MENU_COLPAL_HSL:
		case TRID_MENU_COLPAL_GRAD:
			PanelColorPalette_changeType(no - TRID_MENU_COLPAL_PALETTE);
			return -2;

		//コンパクト
		case TRID_MENU_COLPAL_COMPACT:
			PanelColorPalette_ToggleMode();
			return -2;
	}

	return no;
}


/***************************************
 * HSL パレット
 ***************************************/


/* イベント */

static int _hsl_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//メニューボタン
			case _NOTIFY_ID_MENUBTT:
				_common_run_menu(ev->notify.widget_from, NULL);
				break;
		}
	}

	return 1;
}

/** HSL ページ作成 */

mlkbool PanelColorPalette_createPage_HSL(mPager *p,mPagerInfo *info)
{
	mWidget *wg;

	info->event = _hsl_event;

	mContainerSetType_vert(MLK_CONTAINER(p), 0);

	p->ct.padding.top = 7;

	//

	wg = HSLPalette_new(MLK_WIDGET(p));

	//通知は直接送る
	wg->notify_to = MWIDGET_NOTIFYTO_WIDGET;
	wg->notify_widget = p->wg.parent;

	return TRUE;
}


/***************************************
 * 中間色
 ***************************************/


//左の色の上位8bit に段階数がセットされている。

typedef struct
{
	mWidget *bar[DRAW_GRADBAR_NUM];
}_pagedat_grad;

enum
{
	WID_GRAD_BAR_TOP = 100
};


/* メニュー追加 */

static void _grad_add_menu(mMenu *menu)
{
	mMenuAppendText(menu, TRID_MENU_GRAD_OPT, MLK_TR(TRID_MENU_GRAD_OPT));
}

/* メニュー実行 */

static void _grad_menu(mPager *p,mWidget *menubtt,_pagedat_grad *pd)
{
	int no,i;

	no = _common_run_menu(menubtt, _grad_add_menu);
	if(no < 0) return;

	//設定

	if(PanelColorPalette_dlg_gradopt(MLK_WINDOW(p->wg.toplevel)))
	{
		for(i = 0; i < DRAW_GRADBAR_NUM; i++)
			GradationBar_setStep(pd->bar[i], APPDRAW->col.gradcol[i][0] >> 24);
	}
}

/* イベント */

static int _grad_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		int id = ev->notify.id;

		if(id >= WID_GRAD_BAR_TOP && id < WID_GRAD_BAR_TOP + DRAW_GRADBAR_NUM)
		{
			//----- バー操作
			// param1=RGB色

			if(ev->notify.notify_type == 0)
			{
				//色取得 => そのまま送る

				mWidgetEventAdd_notify(MLK_WIDGET(p), MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW,
					0, ev->notify.param1, CHANGECOL_RGB);
			}
			else
			{
				//左右の色変更 (1=左, 2=右)

				int no,colno;

				no = id - WID_GRAD_BAR_TOP;
				colno = ev->notify.notify_type - 1;

				APPDRAW->col.gradcol[no][colno] &= 0xff000000;
				APPDRAW->col.gradcol[no][colno] |= ev->notify.param1;
			}
		}
		else
		{
			switch(id)
			{
				//(mPager から) メニューボタン
				case _NOTIFY_ID_MENUBTT:
					_grad_menu(p, ev->notify.widget_from, (_pagedat_grad *)pagedat);
					break;
			}
		}
	}

	return 1;
}

/** 中間色ページ作成 */

mlkbool PanelColorPalette_createPage_grad(mPager *p,mPagerInfo *info)
{
	_pagedat_grad *pd;
	int i;

	pd = (_pagedat_grad *)mMalloc0(sizeof(_pagedat_grad));

	info->pagedat = pd;
	info->event = _grad_event;

	//------------

	mContainerSetType_vert(MLK_CONTAINER(p), 8);

	p->ct.padding.top = 7;

	//バー

	for(i = 0; i < DRAW_GRADBAR_NUM; i++)
	{
		pd->bar[i] = GradationBar_new(MLK_WIDGET(p), WID_GRAD_BAR_TOP + i,
			APPDRAW->col.gradcol[i][0] & 0xffffff,
			APPDRAW->col.gradcol[i][1],
			APPDRAW->col.gradcol[i][0] >> 24);
	}

	return TRUE;
}


/***************************************
 * パレット
 ***************************************/

/*
 - mPager の param1 にカラーパレットビューのポインタ。
 - パレットデータ変更時は pal_fmodify を ON にする。
*/

#define WID_PAL_LIST  100
#define CMDID_PAL_LIST_TOP 10000


/* 画像からパレット取得 */

static mlkbool _pal_cmd_load_image(mWindow *parent,PaletteListItem *pi)
{
	mStr str = MSTR_INIT;
	Image32 *img;
	mlkbool ret = FALSE;

	if(!FileDialog_openImage_confdir(parent,
		AppResource_getOpenFileFilter_normal(), 0, &str))
		return FALSE;

	//画像読み込み

	img = Image32_loadFile(str.buf, IMAGE32_LOAD_F_BLEND_WHITE);

	if(!img)
		mMessageBoxErrTr(parent, TRGROUP_MESSAGE, TRID_MESSAGE_FAILED_LOAD);
	else
	{
		PaletteListItem_setFromImage(pi, img->buf, img->w, img->h);
		ret = TRUE;
	}

	Image32_free(img);
	mStrFree(&str);

	return ret;
}

/* ファイル読み込み */

static mlkbool _pal_cmd_loadfile(mWindow *parent,PaletteListItem *pi,mlkbool append)
{
	mStr str = MSTR_INIT;
	mlkbool ret;

	if(!mSysDlg_openFile(parent,
		"Palette Files (APL/ACO)\t*.apl;*.aco\t"
		"AzPainter Palette (APL)\t*.apl\t"
		"Adobe Color File (ACO)\t*.aco\t"
		"All Files\t*",
		0, APPCONF->strFileDlgDir.buf, 0, &str))
		return FALSE;

	//ディレクトリ保存

	mStrPathGetDir(&APPCONF->strFileDlgDir, str.buf);

	//読み込み

	ret = PaletteListItem_loadFile(pi, str.buf, append);
	if(!ret)
		mMessageBoxErrTr(parent, TRGROUP_MESSAGE, TRID_MESSAGE_FAILED_LOAD);

	mStrFree(&str);

	return ret;
}

/* ファイルに保存 */

static void _pal_cmd_savefile(mWindow *parent,PaletteListItem *pi)
{
	mStr str = MSTR_INIT;
	int type;
	mlkbool ret;

	if(!mSysDlg_saveFile(parent,
		"AzPainter Palette (APL)\tapl\t"
		"Adobe Color File (ACO)\taco\t",
		0, APPCONF->strFileDlgDir.buf, 0, &str, &type))
		return;

	//ディレクトリ保存

	mStrPathGetDir(&APPCONF->strFileDlgDir, str.buf);

	//保存

	if(type == 0)
		ret = PaletteListItem_saveFile_apl(pi, str.buf);
	else
		ret = PaletteListItem_saveFile_aco(pi, str.buf);

	if(!ret)
		mMessageBoxErrTr(parent, TRGROUP_MESSAGE, TRID_MESSAGE_FAILED_SAVE);

	mStrFree(&str);
}

/* メニュー項目追加 */

static void _pal_add_menu(mMenu *menu)
{
	PaletteListItem *pi;
	int no;

	//パレットリスト

	no = 0;

	MLK_LIST_FOR(APPDRAW->col.list_pal, pi, PaletteListItem)
	{
		mMenuAppendRadio(menu, CMDID_PAL_LIST_TOP + no, pi->name);

		if(APPDRAW->col.cur_pallist == (mListItem *)pi)
			mMenuSetItemCheck(menu, CMDID_PAL_LIST_TOP + no, 1);
		
		no++;
	}

	if(no)
		mMenuAppendSep(menu);

	//項目

	mMenuAppendTrText_array16(menu, g_menudat_pal);

	if(!APPDRAW->col.cur_pallist)
	{
		mMenuSetItemEnable(menu, TRID_PAL_OPTION, 0);
		mMenuSetItemEnable(menu, TRID_PAL_EDIT, 0);
		mMenuSetItemEnable(menu, TRID_PAL_FILE, 0);
	}
}
	
/* メニューを実行して処理 */

static void _pal_proc_menu(mWidget *menubtt,mWidget *palview)
{
	int id;
	mWindow *parent;
	PaletteListItem *pi;
	mlkbool update = FALSE;

	id = _common_run_menu(menubtt, _pal_add_menu);
	if(id < 0) return;

	if(id >= CMDID_PAL_LIST_TOP)
	{
		//リスト選択
		
		APPDRAW->col.cur_pallist = mListGetItemAtIndex(&APPDRAW->col.list_pal, id - CMDID_PAL_LIST_TOP);
		update = TRUE;
	}
	else
	{
		//---- 各コマンド
		
		pi = (PaletteListItem *)APPDRAW->col.cur_pallist;
		parent = MLK_WINDOW(menubtt->toplevel);
		
		switch(id)
		{
			//パレットリスト (直接編集)
			case TRID_PAL_PALLIST:
				PanelColorPalette_dlg_pallist(parent);

				PaletteList_setModify(); //更新
				update = TRUE;
				break;
			//設定
			case TRID_PAL_OPTION:
				update = PanelColorPalette_dlg_palopt(parent);
				break;
			//パレット編集
			case TRID_PAL_EDIT_PALETTE:
				update = PanelColorPalette_dlg_paledit(parent, pi);
				break;
			//すべて描画色に
			case TRID_PAL_EDIT_ALL_DRAWCOL:
				if(mMessageBox(parent,
					MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_TITLE_CONFIRM),
					MLK_TR2(TRGROUP_PANEL_COLOR_PALETTE, TRID_PAL_MESSAGE_ALL_DRAWCOL),
					MMESBOX_YES | MMESBOX_CANCEL, MMESBOX_YES) == MMESBOX_YES)
				{
					PaletteListItem_setAllColor(pi,
						APPDRAW->col.drawcol.c8.r, APPDRAW->col.drawcol.c8.g, APPDRAW->col.drawcol.c8.b);

					update = TRUE;
				}
				break;
			//ファイル読み込み
			case TRID_PAL_FILE_LOAD:
				update = _pal_cmd_loadfile(parent, pi, FALSE);
				break;
			//ファイル追加読み込み
			case TRID_PAL_FILE_LOAD_APPEND:
				update = _pal_cmd_loadfile(parent, pi, TRUE);
				break;
			//画像から取得
			case TRID_PAL_FILE_LOAD_IMAGE:
				update = _pal_cmd_load_image(parent, pi);
				break;
			//ファイルに保存
			case TRID_PAL_FILE_SAVE:
				_pal_cmd_savefile(parent, pi);
				break;
			//ヘルプ
			case TRID_PAL_HELP:
				AppHelp_message(parent, HELP_TRGROUP_SINGLE, HELP_TRID_COLORPALETTEVIEW); 
				break;
		}
	}

	//パレットビュー更新

	if(update)
		ColorPaletteView_update(palview, APPDRAW->col.cur_pallist);
}

/* パレットのリストを前後に移動 */

static void _pal_move_list(mWidget *palview,int right)
{
	mListItem *pi;

	pi = APPDRAW->col.cur_pallist;
	if(!pi) return;

	pi = (right)? pi->next: pi->prev;
	if(!pi) return;

	APPDRAW->col.cur_pallist = pi;

	ColorPaletteView_update(palview, pi);
}

/* イベント */

static int _pal_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//メニューボタン
			case _NOTIFY_ID_MENUBTT:
				_pal_proc_menu(ev->notify.widget_from, (mWidget *)p->wg.param1);
				break;
			//移動ボタン
			case _NOTIFY_ID_BTT_MOVE:
				_pal_move_list((mWidget *)p->wg.param1, ev->notify.param1);
				break;
		}
	}

	return 1;
}

/** パレットページ作成 */

mlkbool PanelColorPalette_createPage_palette(mPager *p,mPagerInfo *info)
{
	mWidget *wg;

	info->event = _pal_event;

	mContainerSetType_vert(MLK_CONTAINER(p), 0);

	p->ct.padding.top = 3;

	//パレットビュー: 通知は直接送る

	wg = ColorPaletteView_new(MLK_WIDGET(p), WID_PAL_LIST, APPDRAW->col.cur_pallist);

	wg->notify_to = MWIDGET_NOTIFYTO_WIDGET;
	wg->notify_widget = p->wg.parent;

	p->wg.param1 = (intptr_t)wg;

	return TRUE;
}

