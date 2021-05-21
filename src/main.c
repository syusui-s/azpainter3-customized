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

/**********************************
 * main
 **********************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_panel.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_font.h"
#include "mlk_file.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_widget.h"
#include "def_draw_ptr.h"

#include "configfile.h"
#include "appconfig.h"
#include "appresource.h"
#include "appcursor.h"
#include "table_data.h"
#include "undo.h"
#include "regfont.h"
#include "textword_list.h"

#include "panel.h"
#include "panel_func.h"
#include "mainwindow.h"

#include "draw_main.h"

#include "trid.h"

#include "deftrans.h"


//-----------------------

#define _HELP_TEXT "[usage] exe <FILE>\n\n--help-mlk : show mlk options"

//-----------------------
/* グローバル変数定義 */

AppWidgets *g_app_widgets = NULL;
AppConfig *g_app_config = NULL;
AppDraw *g_app_draw = NULL;

//-----------------------

void MainWindow_new(ConfigFileState *state);
void MainWindow_showInit(MainWindow *p,mToplevelSaveState *st);

/* 各パネル作成 */
void PanelTool_new(mPanelState *state);
void PanelToolList_new(mPanelState *state);
void PanelBrushOpt_new(mPanelState *state);
void PanelOption_new(mPanelState *state);
void PanelLayer_new(mPanelState *state);
void PanelColor_new(mPanelState *state);
void PanelColorWheel_new(mPanelState *state);
void PanelColorPalette_new(mPanelState *state);
void PanelCanvasCtrl_new(mPanelState *state);
void PanelCanvasView_new(mPanelState *state);
void PanelImageViewer_new(mPanelState *state);
void PanelFilterList_new(mPanelState *state);

/* layer_template.c */
void LayerTemplate_loadFile(void);
void LayerTemplate_saveFile(void);

/* conv_ver2to3.c */
void ConvertConfigFile(void);

//-----------------------



//===========================
// 初期化
//===========================


/* 設定ファイルディレクトリが新規作成された時 */

static void _proc_new_confdir(void)
{
	mStr str = MSTR_INIT;
	int fnew = TRUE;

	mStrPathSetHome_join(&str, ".azpainter");

	//旧バージョンの設定ディレクトリがある場合

	if(mIsExistDir(str.buf))
	{
		MLK_TRGROUP(TRGROUP_MESSAGE);

		if(mMessageBox(NULL, MLK_TR(TRID_MESSAGE_TITLE_CONFIRM),
			MLK_TR(TRID_MESSAGE_CONVERT_VER2),
			MMESBOX_YESNO, MMESBOX_YES) == MMESBOX_YES)
		{
			ConvertConfigFile();

			fnew = FALSE;
		}
	}

	mStrFree(&str);

	//デフォルトのファイルをコピー
	// :旧バージョンのディレクトリが存在しない or 変換しない

	if(fnew)
	{
		mGuiCopyFile_dataToConfig("confdef/brushsize.dat", "brushsize.dat");
		mGuiCopyFile_dataToConfig("confdef/toollist.dat", "toollist.dat");
		mGuiCopyFile_dataToConfig("confdef/colpalette.dat", "colpalette.dat");
		mGuiCopyFile_dataToConfig("confdef/grad.dat", "grad.dat");
	}
}

/** アプリ初期化
 *
 * return: 0 で成功 */

static int _init_app(void)
{
	ConfigFileState st;

	//------ 初期化

	//設定ファイル用ディレクトリ作成

	if(mGuiCreateConfigDir(NULL) == MLKERR_OK)
	{
		//サブディレクトリ作成
		
		mGuiCreateConfigDir(APP_DIRNAME_BRUSH);
		mGuiCreateConfigDir(APP_DIRNAME_TEXTURE);

		_proc_new_confdir();
	}

	//データ初期化

	g_app_widgets = (AppWidgets *)mMalloc0(sizeof(AppWidgets));

	TableData_init();
	RegFont_init();

	//データ確保

	if(AppConfig_new()
		|| AppDraw_new()
		|| Undo_new())
		return 1;

	//------- 設定読み込み

	drawInit_loadConfig_before();

	//設定ファイル読み込み

	mMemset0(&st, sizeof(ConfigFileState));

	app_load_config(&st);

	//他データファイル読み込み

	LayerTemplate_loadFile();

	TextWordList_loadFile(&APPCONF->list_textword);

	RegFont_loadConfigFile();

	//------ 設定読み込み後

	//AppConfig 設定

	AppConfig_set_afterConfig();

	//リソース

	AppResource_init();

	//設定

	Undo_setMaxNum(APPCONF->undo_maxnum);

	//カーソル

	AppCursor_init();

	//描画カーソル

	if(mStrIsnotEmpty(&APPCONF->strCursorFile))
	{
		AppCursor_setDrawCursor(APPCONF->strCursorFile.buf,
			APPCONF->cursor_hotspot[0], APPCONF->cursor_hotspot[1]);
	}

	//フォント作成

	if(mStrIsnotEmpty(&APPCONF->strFont_panel))
		APPWIDGET->font_panel = mFontCreate_text(mGuiGetFontSystem(), APPCONF->strFont_panel.buf);
	
	APPWIDGET->font_panel_fix = mFontCreate_text_size(mGuiGetFontSystem(), APPCONF->strFont_panel.buf, -12);

	//AppDraw

	drawInit_createWidget_before();

	//------ ウィジェット

	//メインウィンドウ作成

	MainWindow_new(&st);

	//パネル作成

	PanelTool_new(st.panelst + PANEL_TOOL);
	PanelToolList_new(st.panelst + PANEL_TOOLLIST);
	PanelBrushOpt_new(st.panelst + PANEL_BRUSHOPT);
	PanelOption_new(st.panelst + PANEL_OPTION);
	PanelLayer_new(st.panelst + PANEL_LAYER);
	PanelColor_new(st.panelst + PANEL_COLOR);
	PanelColorWheel_new(st.panelst + PANEL_COLOR_WHEEL);
	PanelColorPalette_new(st.panelst + PANEL_COLOR_PALETTE);
	PanelCanvasCtrl_new(st.panelst + PANEL_CANVAS_CTRL);
	PanelCanvasView_new(st.panelst + PANEL_CANVAS_VIEW);
	PanelImageViewer_new(st.panelst + PANEL_IMAGE_VIEWER);
	PanelFilterList_new(st.panelst + PANEL_FILTER_LIST);

	//ウィンドウ表示前の初期化

	drawInit_beforeShow();

	//ウィンドウ表示

	MainWindow_showInit(APPWIDGET->mainwin, &st.mainst);

	Panels_showInit();

	//表示後更新

	PanelCanvasView_changeImageSize();
	PanelLayer_update_all();

	//--------------

	//作業用ディレクトリの作成に失敗時

	if(mStrIsEmpty(&APPCONF->strTempDirProc))
	{
		mMessageBoxErrTr(MLK_WINDOW(APPWIDGET->mainwin),
			TRGROUP_MESSAGE, TRID_MESSAGE_FAILED_TEMPDIR);
	}
 
	return 0;
}

/** 引数のファイルを開く */

static void _open_arg_file(const char *fname)
{
	mStr str = MSTR_INIT;

	mStrSetText_locale(&str, fname, -1);
	
	MainWindow_loadImage(APPWIDGET->mainwin, str.buf, NULL);

	mStrFree(&str);
}


//===========================
// メイン
//===========================


/* AppWidgets・ウィジェット解放
 *
 * [!] mGuiEnd() によるウィジェット破棄前に解放されるため、
 *     destroy ハンドラ内で APPWIDGET を参照しないこと。 */

static void _free_widgets(AppWidgets *p)
{
	Panels_destroy();

	mFontFree(p->font_panel);
	mFontFree(p->font_panel_fix);

	mFree(p);

	APPWIDGET = NULL;
}

/** 終了処理 */

static void _finish(void)
{
	//----- 設定ファイル保存

	app_save_config();

	drawEnd_saveConfig();

	RegFont_saveConfigFile();

	if(APPCONF->fmodify_layertempl)
		LayerTemplate_saveFile();

	if(APPCONF->fmodify_textword)
		TextWordList_saveFile(&APPCONF->list_textword);

	//----- 解放

	AppResource_free();
	AppCursor_free();

	TableData_free();
	RegFont_free();

	Undo_free();

	AppDraw_free();
	
	//作業用ディレクトリ削除

	AppConfig_deleteTempDir();

	AppConfig_free();

	_free_widgets(APPWIDGET);
}

/** 初期化メイン */

static int _init_main(int argc,char **argv)
{
	int top,i;

	if(mGuiInit(argc, argv, &top)) return 1;

	//"--help"

	for(i = top; i < argc; i++)
	{
		if(strcmp(argv[i], "--help") == 0)
		{
			puts(_HELP_TEXT);
			mGuiEnd();
			return 1;
		}
	}

	//

	mGuiSetWMClass("azpainter", "AzPainter");

	//パスセット

	mGuiSetPath_data_exe("../share/azpainter3");
	mGuiSetPath_config_home(".config/azpainter");

	//翻訳データ

	mGuiLoadTranslation(g_deftransdat, NULL, "tr");

	//バックエンド初期化

	mGuiSetEnablePenTablet();

	if(mGuiInitBackend()) return 1;

	//アプリ初期化

	if(_init_app())
	{
		mError("failed initialize\n");
		return 1;
	}

	//ファイル開く

	if(top < argc)
		_open_arg_file(argv[top]);

	return 0;
}

/** メイン */

int main(int argc,char **argv)
{
	//初期化

	if(_init_main(argc, argv))
		return 1;

	//実行

	mGuiRun();

	//終了

	_finish();

	mGuiEnd();

	return 0;
}
