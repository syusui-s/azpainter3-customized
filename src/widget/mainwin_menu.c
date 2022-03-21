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
 * メインウィンドウ: メニュー関連
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_window.h"
#include "mlk_iconbar.h"
#include "mlk_menu.h"
#include "mlk_accelerator.h"
#include "mlk_str.h"
#include "mlk_iniread.h"
#include "mlk_key.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_widget.h"
#include "def_mainwin.h"

#include "panel.h"

#include "layeritem.h"

#include "draw_main.h"

#include "trid.h"
#include "trid_mainmenu.h"


//----------------------

//レイヤメニュー:フォルダ時に無効にするID
static uint16_t g_menuid_folder_disable[] = {
	TRMENU_LAYER_COPY, TRMENU_LAYER_ERASE, TRMENU_LAYER_OUTPUT, TRMENU_LAYER_OPT_TYPE,
	0
};

//----------------------

//ショートカットキーデフォルト

typedef struct
{
	uint32_t id,key;
}_sckeydat;

static const _sckeydat g_sckey_default[] = {
	{TRMENU_FILE_NEW, MLK_ACCELKEY_CTRL | 'N'},
	{TRMENU_FILE_OPEN, MLK_ACCELKEY_CTRL | 'O'},
	{TRMENU_FILE_SAVE, MLK_ACCELKEY_CTRL | 'S'},
	{TRMENU_FILE_SAVE_AS, MLK_ACCELKEY_CTRL | 'W'},
	{TRMENU_EDIT_UNDO, MLK_ACCELKEY_CTRL | 'Z'},
	{TRMENU_EDIT_REDO, MLK_ACCELKEY_CTRL | 'Y'},
	{TRMENU_EDIT_FILL, MKEY_INSERT},
	{TRMENU_EDIT_ERASE, MKEY_DELETE},
	{TRMENU_SEL_RELEASE, MLK_ACCELKEY_CTRL | 'D'},
	{TRMENU_SEL_ALL, MLK_ACCELKEY_CTRL | 'A'},
	{TRMENU_SEL_INVERSE, MLK_ACCELKEY_CTRL | 'I'},
	{TRMENU_SEL_EXPAND, MLK_ACCELKEY_CTRL | 'E'},
	{TRMENU_SEL_COPY, MLK_ACCELKEY_CTRL | 'C'},
	{TRMENU_SEL_CUT, MLK_ACCELKEY_CTRL | 'X'},
	{TRMENU_SEL_PASTE_NEWLAYER, MLK_ACCELKEY_CTRL | 'V'},
	{TRMENU_VIEW_CANVAS_MIRROR, MLK_ACCELKEY_CTRL | 'Q'},
	{TRMENU_VIEW_GRID, MLK_ACCELKEY_CTRL | 'G'},
	{0,0}
};

//----------------------


/** メニューのキーをセット (設定ファイルから読み込み) */

void MainWindow_setMenuKey(MainWindow *p)
{
	mAccelerator *accel;
	mIniRead *ini;
	const _sckeydat *dat;
	int cmdid;
	uint32_t key;
	mlkerr ret;

	accel = mToplevelGetAccelerator(MLK_TOPLEVEL(p));

	ret = mIniRead_loadFile_join(&ini, mGuiGetPath_config_text(), CONFIG_FILENAME_MENUKEY);

	if(!ini) return;

	if(ret == MLKERR_OPEN)
	{
		//ファイルが開けない・存在しない場合はデフォルト

		for(dat = g_sckey_default; dat->id; dat++)
		{
			mAcceleratorAdd(accel, dat->id, dat->key, NULL);

			mMenuSetItemShortcutKey(p->menu_main, dat->id, dat->key);
		}
	}
	else
	{
		if(mIniRead_setGroup(ini, "menukey"))
		{
			while(mIniRead_getNextItem_keyno_int32(ini, &cmdid, &key, TRUE))
			{
				mAcceleratorAdd(accel, cmdid, key, NULL);

				mMenuSetItemShortcutKey(p->menu_main, cmdid, key);
			}
		}
	}

	mIniRead_end(ini);
}


//===========================
// メニュー実行
//===========================


/** ツールバーボタンからの表示倍率メニュー */

void MainWindow_menu_canvasZoom(MainWindow *p,int id)
{
	mMenu *menu;
	mMenuItem *mi;
	int i;
	mBox box;
	char m[16];

	menu = mMenuNew();

	for(i = 25; i <= 1000; )
	{
		snprintf(m, 16, "%d%%", i);

		mMenuAppendText_copy(menu, i, m, -1);
	
		i += (i < 100)? 25: 100;
	}

	mIconBarGetItemBox(p->ib_toolbar, id, &box);

	mi = mMenuPopup(menu, MLK_WIDGET(p->ib_toolbar), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);
	
	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//セット

	if(i != -1)
	{
		drawCanvas_update(APPDRAW, i * 10, 0,
			DRAWCANVAS_UPDATE_ZOOM | DRAWCANVAS_UPDATE_RESET_SCROLL);
	}
}

/* 開く/保存ドロップメニューに、ディレクトリリスト追加 */

static void _add_dropmenu_dirlist(MainWindow *p,mMenu *menu,mlkbool save)
{
	mStr str = MSTR_INIT,*str_array;

	str_array = (save)? APPCONF->strRecentSaveDir: APPCONF->strRecentOpenDir;

	//編集中のファイルと同じディレクトリ

	if(!mStrIsEmpty(&p->strFilename))
	{
		mStrPathGetDir(&str, p->strFilename.buf);
	
		mMenuAppendText_copy(menu, -2, str.buf, str.len);

		mStrFree(&str);

		if(!mStrIsEmpty(str_array))
			mMenuAppendSep(menu);
	}

	//履歴

	mMenuAppendStrArray(menu, str_array, 0, CONFIG_RECENTDIR_NUM);
}

/** 開く/保存のディレクトリ履歴メニュー
 *
 * return: 選択された履歴の番号。
 *  -1 でキャンセル。-2 で編集ファイルのディレクトリ。 */

int MainWindow_menu_opensave_dir(MainWindow *p,mlkbool save)
{
	mMenu *menu;
	mMenuItem *mi;
	mBox box;
	int no;

	menu = mMenuNew();

	_add_dropmenu_dirlist(p, menu, save);

	mIconBarGetItemBox(p->ib_toolbar,
		(save)? TRMENU_FILE_SAVE_AS: TRMENU_FILE_OPEN, &box);

	mi = mMenuPopup(menu, MLK_WIDGET(p->ib_toolbar), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);
	
	no = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	return no;
}

/** ツールバーからのファイル履歴メニュー */

void MainWindow_menu_recentFile(MainWindow *p)
{
	mBox box;

	//履歴が一つもない (先頭が空)

	if(mStrIsEmpty(APPCONF->strRecentFile)) return;

	//メニュー
	// :MainWindow に COMMAND イベントを送る

	mIconBarGetItemBox(p->ib_toolbar, TRMENU_FILE_RECENTFILE, &box);

	mMenuPopup(p->menu_recentfile, MLK_WIDGET(p->ib_toolbar), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY | MPOPUP_F_MENU_SEND_COMMAND,
		MLK_WIDGET(p));
}

/** 複製保存のドロップメニュー
 *
 * return: 選択された履歴の番号。
 *  -1 でキャンセル、-2 で現在の編集ファイルのディレクトリ。 */

int MainWindow_menu_savedup(MainWindow *p)
{
	mMenu *menu,*sub;
	mMenuItem *mi;
	mBox box;
	int i,id;
	static const char *format_str[] = {
		"APD (AzPainter)", "PSD (PhotoShop)", "PNG", "JPEG",
		"BMP", "GIF", "TIFF", "WEBP", 0
	};

	MLK_TRGROUP(TRGROUP_SAVEDUP_MENU);

	//保存形式サブメニュー

	sub = mMenuNew();

	mMenuAppend(sub, 1000, MLK_TR(1), 0, MMENUITEM_F_RADIO_TYPE);

	for(i = 0; format_str[i]; i++)
		mMenuAppend(sub, 1001 + i, format_str[i], 0, MMENUITEM_F_RADIO_TYPE);

	mMenuSetItemCheck(sub, 1000 + APPCONF->savedup_type, TRUE);

	//main

	menu = mMenuNew();

	mMenuAppendSubmenu(menu, 100, MLK_TR(0), sub, 0);
	mMenuAppendSep(menu);

	_add_dropmenu_dirlist(p, menu, TRUE);
	
	//

	mIconBarGetItemBox(p->ib_toolbar, TRMENU_FILE_SAVE_DUP, &box);

	mi = mMenuPopup(menu, MLK_WIDGET(p->ib_toolbar), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);
	
	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//

	if(id >= 1000)
	{
		//保存形式変更
		APPCONF->savedup_type = id - 1000;
		return -1;
	}
	else
		//履歴ディレクトリ
		return id;
}


//===========================
// メニューセット
//===========================


/** ファイル履歴のメニューをセット */

void MainWindow_setMenu_recentfile(MainWindow *p)
{
	mMenu *menu = p->menu_recentfile;

	//削除

	mMenuDeleteAll(menu);

	//ファイル

	mMenuAppendStrArray(menu, APPCONF->strRecentFile,
		MAINWIN_CMDID_RECENTFILE, CONFIG_RECENTFILE_NUM);

	//履歴消去

	if(mStrIsnotEmpty(APPCONF->strRecentFile))
	{
		mMenuAppendSep(menu);

		mMenuAppendText(menu, TRMENU_FILE_RECENTFILE_CLEAR,
			MLK_TR2(TRGROUP_MAINMENU, TRMENU_FILE_RECENTFILE_CLEAR));
	}
}


//=============================
// ポップアップ表示時
//=============================


/* "レイヤ" */

static void _menupopup_layer(mMenu *menu)
{
	LayerItem *li = APPDRAW->curlayer;
	const uint16_t *pid;
	int fimg;

	//フォルダ時無効

	fimg = LAYERITEM_IS_IMAGE(li);

	for(pid = g_menuid_folder_disable; *pid; pid++)
		mMenuSetItemEnable(menu, *pid, fimg);

	//下レイヤに移す/結合

	mMenuSetItemEnable(menu, TRMENU_LAYER_DROP, LayerItem_isEnableUnderDrop(li));
	
	mMenuSetItemEnable(menu, TRMENU_LAYER_COMBINE, LayerItem_isEnableUnderCombine(li));

	//テキスト時無効

	mMenuSetItemEnable(menu, TRMENU_LAYER_EDIT, !LAYERITEM_IS_TEXT(li));

	//フォルダ時のみ

	mMenuSetItemEnable(menu, TRMENU_LAYER_FOLDER_CHECKED_MOVE, !fimg);

	//線の色変更

	mMenuSetItemEnable(menu, TRMENU_LAYER_OPT_COLOR, LayerItem_isNeedColor(li));

	//トーンをグレイスケール表示

	mMenuSetItemCheck(menu, TRMENU_LAYER_TONE_TO_GRAY, APPDRAW->ftonelayer_to_gray);
}

/* "選択範囲" */

static void _menupopup_select(mMenu *menu)
{
	int f;

	f = drawSel_isHave();
	
	mMenuSetItemEnable(menu, TRMENU_SEL_RELEASE, f);
	mMenuSetItemEnable(menu, TRMENU_SEL_EXPAND, f);

	mMenuSetItemEnable(menu, TRMENU_SEL_COPY, drawSel_isEnable_copy_cut(FALSE));
	mMenuSetItemEnable(menu, TRMENU_SEL_CUT, drawSel_isEnable_copy_cut(TRUE));
	mMenuSetItemEnable(menu, TRMENU_SEL_OUTPUT_FILE, drawSel_isEnable_outputFile());
}

/* "表示" */

static void _menupopup_view(mMenu *menu)
{
	int i,f;
	uint32_t fview;

	fview = APPCONF->fview;
	
	mMenuSetItemCheck(menu, TRMENU_VIEW_CANVAS_MIRROR, APPDRAW->canvas_mirror);

	mMenuSetItemCheck(menu, TRMENU_VIEW_BKGND_PLAID, fview & CONFIG_VIEW_F_BKGND_PLAID);
	mMenuSetItemCheck(menu, TRMENU_VIEW_GRID, fview & CONFIG_VIEW_F_GRID);
	mMenuSetItemCheck(menu, TRMENU_VIEW_GRID_SPLIT, fview & CONFIG_VIEW_F_GRID_SPLIT);
	mMenuSetItemCheck(menu, TRMENU_VIEW_RULE_GUIDE, fview & CONFIG_VIEW_F_RULE_GUIDE);

	mMenuSetItemCheck(menu, TRMENU_VIEW_TOOLBAR, fview & CONFIG_VIEW_F_TOOLBAR);
	mMenuSetItemCheck(menu, TRMENU_VIEW_STATUSBAR, fview & CONFIG_VIEW_F_STATUSBAR);
	mMenuSetItemCheck(menu, TRMENU_VIEW_CURSOR_POS, fview & CONFIG_VIEW_F_CURSOR_POS);
	mMenuSetItemCheck(menu, TRMENU_VIEW_LAYER_NAME, fview & CONFIG_VIEW_F_CANV_LAYER_NAME);
	mMenuSetItemCheck(menu, TRMENU_VIEW_BOXSEL_POS, fview & CONFIG_VIEW_F_BOXSEL_POS);

	//---- パネル

	f = ((fview & CONFIG_VIEW_F_PANEL) != 0);

	mMenuSetItemCheck(menu, TRMENU_VIEW_PANEL_VISIBLE, f);

	mMenuSetItemEnable(menu, TRMENU_VIEW_PANEL_ALL_WINDOW, f);
	mMenuSetItemEnable(menu, TRMENU_VIEW_PANEL_ALL_STORE, f);

	for(i = 0; i < PANEL_NUM; i++)
	{
		mMenuSetItemCheck(menu, TRMENU_VIEW_PANEL_TOOL + i, Panel_isCreated(i));

		//パネル自体が非表示の場合は無効
		mMenuSetItemEnable(menu, TRMENU_VIEW_PANEL_TOOL + i, f);
	}
}

/** メインメニューのポップアップ表示時 */

void MainWindow_on_menupopup(mMenu *menu,int id)
{
	switch(id)
	{
		//レイヤ
		case TRMENU_TOP_LAYER:
			_menupopup_layer(menu);
			break;

		//選択範囲
		case TRMENU_TOP_SELECT:
			_menupopup_select(menu);
			break;
	
		//表示
		case TRMENU_TOP_VIEW:
			_menupopup_view(menu);
			break;
	}
}

