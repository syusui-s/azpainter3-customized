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
 * MainWindow
 * メインウィンドウ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_splitter.h"
#include "mlk_menubar.h"
#include "mlk_iconbar.h"
#include "mlk_panel.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_accelerator.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_file.h"

#include "def_macro.h"
#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_mainwin.h"
#include "def_toolbar_btt.h"

#include "mainwindow.h"
#include "maincanvas.h"
#include "statusbar.h"
#include "panel.h"
#include "dialogs.h"

#include "undo.h"

#include "draw_main.h"

#include "appresource.h"
#include "configfile.h"

#include "pv_mainwin.h"

#include "trid.h"
#include "trid_mainmenu.h"

#include "def_mainmenu.h"


//---------------------

#define _APP_VERSION_TEXT  "AzPainter ver 3.0.2\n\nCopyright (c) 2013-2021 Azel"

#define _APP_LICENSE_TEXT \
"AzPainter is free software: you can redistribute it and/or modify\n" \
"it under the terms of the GNU General Public License as published by\n" \
"the Free Software Foundation, either version 3 of the License, or\n" \
"(at your option) any later version.\n\n" \
"AzPainter is distributed in the hope that it will be useful,\n" \
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
"GNU General Public License for more details.\n\n" \
"You should have received a copy of the GNU General Public License\n" \
"along with this program.  If not, see <http://www.gnu.org/licenses/>."

//---------------------

//ツールバー:各アイコンに対応するボタン
// 0x8000 = ドロップボタンあり, 0x40000 = チェック

static const uint16_t g_toolbar_btt_id[] = {
	TRMENU_FILE_NEW, TRMENU_FILE_OPEN|0x8000, TRMENU_FILE_RECENTFILE,
	TRMENU_FILE_SAVE, TRMENU_FILE_SAVE_AS|0x8000, TRMENU_FILE_SAVE_DUP|0x8000,
	TRMENU_EDIT_UNDO, TRMENU_EDIT_REDO, TRMENU_LAYER_ERASE, TRMENU_SEL_RELEASE,
	TRMENU_VIEW_PANEL_VISIBLE|0x4000, TRMENU_VIEW_CANVAS_MIRROR|0x4000,
	TRMENU_VIEW_BKGND_PLAID|0x4000, TRMENU_VIEW_GRID|0x4000,
	TRMENU_VIEW_GRID_SPLIT|0x4000, TRMENU_OPT_GRID,
	TRMENU_VIEW_PANEL_FILTER_LIST, TRMENU_VIEW_CANVAS_ZOOM
};

//---------------------

void MainCanvas_new(mWidget *parent);

mlkbool MenuKeyOptDlg_run(mWindow *parent);
mlkbool CanvasKeyDlg_run(mWindow *parent);

static int _event_handle(mWidget *wg,mEvent *ev);

//---------------------



/* mSplitter 対象取得関数*/

static int _splitter_target(mSplitter *p,mSplitterTarget *info)
{
	mWidget *wg,*canvas;

	canvas = (mWidget *)info->param;

	//前 (前にキャンバスがあればキャンバス、なければ直前のウィジェット)

	for(wg = p->wg.prev; wg && wg != canvas; wg = wg->prev);

	info->wgprev = (wg == canvas)? canvas: p->wg.prev;

	//次

	if(wg == canvas)
		wg = NULL;
	else
	{
		for(wg = p->wg.next; wg && wg != canvas; wg = wg->next);
	}

	info->wgnext = (wg == canvas)? canvas: p->wg.next;

	//

	info->prev_cur = mWidgetGetHeight_visible(info->wgprev);
	info->next_cur = mWidgetGetHeight_visible(info->wgnext);

	info->prev_min = 5;
	info->next_min = 5;

	info->prev_max = info->next_max = info->prev_cur + info->next_cur;

	return 1;
}


//=========================
// 初期化
//=========================


/* メニュー作成 */

static void _create_menu(MainWindow *p)
{
	mMenuBar *bar;

	bar = mMenuBarNew(MLK_WIDGET(p), 0, MMENUBAR_S_BORDER_BOTTOM);

	mToplevelAttachMenuBar(MLK_TOPLEVEL(p), bar);

	//データからメニュー作成

	MLK_TRGROUP(TRGROUP_MAINMENU);

	mMenuBarCreateMenuTrArray16(bar, g_mainmenudat);

	p->menu_main = mMenuBarGetMenu(bar);

	//最近使ったファイル:サブメニュー

	p->menu_recentfile = mMenuNew();

	mMenuBarSetSubmenu(bar, TRMENU_FILE_RECENTFILE, p->menu_recentfile);

	MainWindow_setMenu_recentfile(p);
}

/* メイン部分作成 */

static void _create_main(MainWindow *p,ConfigFileState *state)
{
	mWidget *ct,*wg;
	int i;

	//水平コンテナ

	p->ct_main = ct = mContainerCreateHorz(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0);

	//キャンバス

	MainCanvas_new(ct);

	//ペイン

	for(i = 0; i < PANEL_PANE_NUM; i++)
	{
		//ペインウィジェット
	
		wg = mContainerCreateVert(ct, 0, MLF_FIX_W | MLF_EXPAND_H, 0);

		APPWIDGET->pane[i] = wg;

		wg->w = state->pane_w[i];
		wg->draw = mWidgetDrawHandle_bkgnd;
		wg->draw_bkgnd = mWidgetDrawBkgndHandle_fillFace;
		wg->hintRepH = 1; //パネルの推奨サイズが高さに適用されないようにする
		wg->foption |= MWIDGET_OPTION_NO_TAKE_FOCUS; //格納時はフォーカスを受け取らない

		//スプリッター

		p->splitter[i] = wg = (mWidget *)mSplitterNew(ct, 0, MSPLITTER_S_VERT);

		wg->id = 1 + i; //ペインとスプリッターの区別を付けるために
		wg->draw_bkgnd = mWidgetDrawBkgndHandle_fillFace;

		mSplitterSetFunc_getTarget(MLK_SPLITTER(wg), _splitter_target, (intptr_t)APPWIDGET->canvas);
	}
}

/** ツールバー作成
 *
 * init: FALSE でツールバー設定後の更新 */

void MainWindow_createToolBar(MainWindow *p,mlkbool init)
{
	mIconBar *ib;
	const uint8_t *buf;
	int no,id,cmdid;
	uint32_t f;

	if(!init)
	{
		ib = p->ib_toolbar;

		mIconBarDeleteAll(ib);
	}
	else
	{
		//作成
		
		p->ib_toolbar = ib = mIconBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0,
			MICONBAR_S_TOOLTIP);

		mIconBarSetPadding(ib, 2);

		mIconBarSetImageList(ib, APPRES->imglist_toolbar);

		mIconBarSetTooltipTrGroup(ib, TRGROUP_TOOLBAR_TOOLTIP);

		if(!(APPCONF->fview & CONFIG_VIEW_F_TOOLBAR))
			mWidgetShow(MLK_WIDGET(ib), 0);
	}

	//ボタン追加

	buf = APPCONF->toolbar_btts;
	if(!buf) buf = AppResource_getToolBarDefaultBtts();

	for(; *buf != 255; buf++)
	{
		no = *buf;
		
		if(no >= TOOLBAR_BTT_NUM && no < 254)
			break;
		
		if(no == 254)
		{
			id = 0xffff;
			cmdid = -1;
		}
		else
		{
			id = g_toolbar_btt_id[no];
			cmdid = id & 0x3fff;
		}

		if(id == 0xffff)
			f = MICONBAR_ITEMF_SEP;
		else if(id & 0x8000)
			f = MICONBAR_ITEMF_DROPDOWN;
		else if(id & 0x4000)
			f = MICONBAR_ITEMF_CHECKBUTTON;
		else
			f = 0;
		
		mIconBarAdd(ib, cmdid, no, no, f);
	}

	//チェック

	f = APPCONF->fview;

	mIconBarSetCheck(ib, TRMENU_VIEW_PANEL_VISIBLE, f & CONFIG_VIEW_F_PANEL);
	mIconBarSetCheck(ib, TRMENU_VIEW_BKGND_PLAID, f & CONFIG_VIEW_F_BKGND_PLAID);
	mIconBarSetCheck(ib, TRMENU_VIEW_GRID, f & CONFIG_VIEW_F_GRID);
	mIconBarSetCheck(ib, TRMENU_VIEW_GRID_SPLIT, f & CONFIG_VIEW_F_GRID_SPLIT);
}


//=========================
// main
//=========================


/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	MainWindow *p = (MainWindow *)wg;

	mStrFree(&p->strFilename);
	
	mAcceleratorDestroy(p->top.accelerator);
}

/** メインウィンドウ作成 */

void MainWindow_new(ConfigFileState *state)
{
	MainWindow *p;

	//ウィンドウ
	
	p = (MainWindow *)mToplevelNew(NULL, sizeof(MainWindow),
			MTOPLEVEL_S_NORMAL | MTOPLEVEL_S_NO_INPUT_METHOD | MTOPLEVEL_S_ENABLE_PENTABLET);
	if(!p) return;
	
	APPWIDGET->mainwin = p;

	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->wg.foption |= MWIDGET_OPTION_NO_DRAW_BKGND;  //背景は描画しない

	//アイコン

	mToplevelSetIconPNG_file(MLK_TOPLEVEL(p), "!/img/appicon.png");

	//メニュー

	_create_menu(p);

	//ツールバー

	MainWindow_createToolBar(p, TRUE);

	//アクセラレータ

	p->top.accelerator = mAcceleratorNew();

	mAcceleratorSetDefaultWidget(p->top.accelerator, MLK_WIDGET(p));

	//ショートカットキー

	MainWindow_setMenuKey(p);

	//メイン部分

	_create_main(p, state);

	//ステータスバー

	if(APPCONF->fview & CONFIG_VIEW_F_STATUSBAR)
		StatusBar_new();
}

/** 初期表示 */

void MainWindow_showInit(MainWindow *p,mToplevelSaveState *st)
{
	//タイトル

	MainWindow_setTitle(p);

	//ステータスバー情報

	StatusBar_setImageInfo();

	//ペインレイアウト

	MainWindow_pane_layout(p, FALSE);

	//パネル自体が非表示の場合は、すべてのペインを非表示に

	if(!(APPCONF->fview & CONFIG_VIEW_F_PANEL))
		MainWindow_pane_toggle(p);

	//表示 

	if(!st->w || !st->h)
	{
		st->w = 800;
		st->h = 600;
	}

	mToplevelSetSaveState(MLK_TOPLEVEL(p), st);

	mWidgetShow(MLK_WIDGET(p), 1);
}

/** 終了 */

void MainWindow_quit(void)
{
	if(MainWindow_confirmSave(APPWIDGET->mainwin))
		mGuiQuit();
}

/** タイトルバーの文字列セット */

void MainWindow_setTitle(MainWindow *p)
{
	mStr str = MSTR_INIT;

	//文字列

	if(mStrIsEmpty(&p->strFilename))
		mStrSetText(&str, "untitled");
	else
		mStrPathGetBasename(&str, p->strFilename.buf);

	mStrAppendText(&str, " - " APPNAME);

	//

	mToplevelSetTitle(MLK_TOPLEVEL(p), str.buf);

	mStrFree(&str);
}

/** ウィンドウを指定して、エラーメッセージ表示
 *
 * detail: 詳細メッセージ (NULL でなし) */

void MainWindow_errmes_win(mWindow *parent,mlkerr err,const char *detail)
{
	int id = TRID_ERRMES_ERROR;

	switch(err)
	{
		case MLKERR_ALLOC: id = TRID_ERRMES_ALLOC; break;
		case MLKERR_MAX_SIZE: id = TRID_ERRMES_MAXSIZE; break;
		case MLKERR_UNSUPPORTED: id = TRID_ERRMES_UNSUPPORTED; break;
		case MLKERR_DAMAGED: id = TRID_ERRMES_DAMAGED; break;
	}

	//

	if(!detail)
		mMessageBoxErrTr(parent, TRGROUP_ERRMES, id);
	else
	{
		mStr str = MSTR_INIT;

		mStrSetFormat(&str, "%s\n\n%s",
			MLK_TR2(TRGROUP_ERRMES, id), detail);

		mMessageBoxErr(parent, str.buf);

		mStrFree(&str);
	}
}

/** メインウィンドウでエラーメッセージ表示 */

void MainWindow_errmes(mlkerr err,const char *detail)
{
	MainWindow_errmes_win(MLK_WINDOW(APPWIDGET->mainwin), err, detail);
}

/** 現在のイメージの保存確認
 *
 * return: FALSE でキャンセル */

mlkbool MainWindow_confirmSave(MainWindow *p)
{
	uint32_t ret;

	//変更なし
	
	if(!Undo_isModify()) return TRUE;

	//確認

	MLK_TRGROUP(TRGROUP_MESSAGE);

	ret = mMessageBox(MLK_WINDOW(p),
		MLK_TR(TRID_MESSAGE_TITLE_CONFIRM),
		MLK_TR(TRID_MESSAGE_SAVE_CLOSE_CONFIRM),
		MMESBOX_SAVE | MMESBOX_NOTSAVE | MMESBOX_CANCEL,
		MMESBOX_CANCEL);

	if(ret == MMESBOX_CANCEL)
		return FALSE;
	else if(ret == MMESBOX_SAVE)
	{
		if(!MainWindow_saveFile(p, SAVEFILE_TYPE_OVERWRITE, 0))
			return FALSE;
	}

	return TRUE;
}

/** キャンバスが新しくなった時の更新
 *
 * filename: 読み込んだファイル名。NULL でサイズのみ変更。空文字列で新規作成。 */

void MainWindow_updateNewCanvas(MainWindow *p,const char *filename)
{
	//ファイル変更時

	if(filename)
	{
		mStrSetText(&p->strFilename, filename);

		p->fsaved = FALSE;
	}

	//タイトル

	MainWindow_setTitle(p);

	//各変更

	drawImage_afterNewCanvas(APPDRAW, (filename != NULL));

	StatusBar_setImageInfo();
}

/** プログレスバーを表示する親ウィジェットと、範囲を取得 */

mWidget *MainWindow_getProgressBarPos(mBox *box)
{
	mWidget *wg;

	wg = StatusBar_getProgressBarPos(box);
	if(!wg)
	{
		//ステータスバーが非表示なら、メインウィンドウ

		wg = MLK_WIDGET(APPWIDGET->mainwin);

		mWidgetGetBox_rel(wg, box);
	}

	return wg;
}


//============================
// コマンド
//============================


/* 履歴からファイルを開く */

static void _open_recentfile(MainWindow *p,int no)
{
	if(!mIsExistFile(APPCONF->strRecentFile[no].buf))
	{
		//ファイルが存在しない場合、メッセージ後に履歴を削除

		mMessageBoxOK(MLK_WINDOW(p),
			MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_NOT_EXIST_RECENT_FILE));

		mStrArrayShiftUp(APPCONF->strRecentFile, no, CONFIG_RECENTFILE_NUM - 1);

		MainWindow_setMenu_recentfile(p);
	}
	else
	{
		//開く
		
		if(MainWindow_confirmSave(p))
			MainWindow_loadImage(p, APPCONF->strRecentFile[no].buf, NULL);
	}
}

/* イメージの設定 */

static void _cmd_image_option(MainWindow *p)
{
	int ret;

	ret = ImageOptionDlg_run(MLK_WINDOW(p));

	if(ret)
	{
		if(ret & IMAGEOPTDLG_F_BITS)
			drawImage_changeImageBits(APPDRAW);
		else
			drawUpdate_all();
	}
}


//============================
// イベント
//============================


/* ツールバーのチェックボタンを反転 */

static void _toolbar_check_toggle(MainWindow *p,int id,mEventCommand *ev)
{
	if(ev->from != MEVENT_COMMAND_FROM_ICONBAR_BUTTON)
		mIconBarSetCheck(p->ib_toolbar, id, -1);
}

/* コマンド */

static void _event_command(MainWindow *p,mEventCommand *ev)
{
	int id = ev->id,n;

	//レイヤ関連

	if(id >= TRMENU_LAYER_ID_TOP && id <= TRMENU_LAYER_ID_BOTTOM)
	{
		MainWindow_layercmd(p, id);
		return;
	}

	//フィルタ

	if(id >= TRMENU_FILTER_ID_TOP && id <= TRMENU_FILTER_ID_BOTTOM)
	{
		MainWindow_runFilterCommand(id);
		return;
	}

	//ファイル履歴

	if(id >= MAINWIN_CMDID_RECENTFILE && id < MAINWIN_CMDID_RECENTFILE + CONFIG_RECENTFILE_NUM)
	{
		_open_recentfile(p, id - MAINWIN_CMDID_RECENTFILE);
		return;
	}

	//各パネル表示

	if(id >= TRMENU_VIEW_PANEL_TOOL && id < TRMENU_VIEW_PANEL_TOOL + PANEL_NUM)
	{
		MainWindow_panel_toggle(p, id - TRMENU_VIEW_PANEL_TOOL);
		return;
	}

	//

	switch(id)
	{
		//ツールバー:倍率指定
		case TRMENU_VIEW_CANVAS_ZOOM:
			MainWindow_menu_canvasZoom(p, id);
			break;
		//ツールバー:ファイル履歴
		case TRMENU_FILE_RECENTFILE:
			MainWindow_menu_recentFile(p);
			break;
	
		//---- ファイル

		//新規作成
		case TRMENU_FILE_NEW:
			MainWindow_newCanvas(p);
			break;
		//開く
		case TRMENU_FILE_OPEN:
			if(ev->from == MEVENT_COMMAND_FROM_ICONBAR_DROP)
			{
				n = MainWindow_menu_opensave_dir(p, FALSE);
				if(n != -1)
					MainWindow_openFileDialog(p, n);
			}
			else
				MainWindow_openFileDialog(p, 0);
			break;
		//上書き保存
		case TRMENU_FILE_SAVE:
			MainWindow_saveFile(p, SAVEFILE_TYPE_OVERWRITE, 0);
			break;
		//別名で保存
		case TRMENU_FILE_SAVE_AS:
			if(ev->from == MEVENT_COMMAND_FROM_ICONBAR_DROP)
			{
				n = MainWindow_menu_opensave_dir(p, TRUE);
				if(n != -1)
					MainWindow_saveFile(p, SAVEFILE_TYPE_RENAME, n);
			}
			else
				MainWindow_saveFile(p, SAVEFILE_TYPE_RENAME, 0);
			break;
		//複製を保存
		case TRMENU_FILE_SAVE_DUP:
			if(ev->from == MEVENT_COMMAND_FROM_ICONBAR_DROP)
			{
				n = MainWindow_menu_savedup(p);
				if(n != -1)
					MainWindow_saveFile(p, SAVEFILE_TYPE_DUP, n);
			}
			else
				MainWindow_saveFile(p, SAVEFILE_TYPE_DUP, 0);
			break;
		//終了
		case TRMENU_FILE_EXIT:
			MainWindow_quit();
			break;
		//履歴消去
		case TRMENU_FILE_RECENTFILE_CLEAR:
			mStrArrayFree(APPCONF->strRecentFile, CONFIG_RECENTFILE_NUM);
			MainWindow_setMenu_recentfile(p);
			break;

		//---- 編集

		//元に戻す/やり直す
		case TRMENU_EDIT_UNDO:
		case TRMENU_EDIT_REDO:
			MainWindow_undo_redo(p, (id == TRMENU_EDIT_REDO));
			break;
		//塗りつぶし/消去
		case TRMENU_EDIT_FILL:
		case TRMENU_EDIT_ERASE:
			drawSel_fill_erase(APPDRAW, (id == TRMENU_EDIT_ERASE));
			break;
		//キャンバスサイズ変更
		case TRMENU_EDIT_RESIZE_CANVAS:
			MainWindow_cmd_resizeCanvas(p);
			break;
		//画像を統合して拡大縮小
		case TRMENU_EDIT_SCALE_CANVAS:
			MainWindow_cmd_scaleCanvas(p);
			break;
		//イメージの設定
		case TRMENU_EDIT_IMAGE_OPTION:
			_cmd_image_option(p);
			break;
		//描画色をイメージ背景色に
		case TRMENU_EDIT_DRAWCOL_TO_BKGNDCOL:
			APPDRAW->imgbkcol = APPDRAW->col.drawcol;
			drawUpdate_all();
			break;

		//---- 選択範囲

		//解除
		case TRMENU_SEL_RELEASE:
			drawSel_release(APPDRAW, TRUE);
			break;
		//反転
		case TRMENU_SEL_INVERSE:
			drawSel_inverse(APPDRAW);
			break;
		//すべて選択
		case TRMENU_SEL_ALL:
			drawSel_all(APPDRAW);
			break;
		//範囲の拡張/縮小
		case TRMENU_SEL_EXPAND:
			MainWindow_cmd_selectExpand(p);
			break;
		//コピー
		case TRMENU_SEL_COPY:
			drawSel_copy_cut(APPDRAW, FALSE);
			break;
		//切り取り
		case TRMENU_SEL_CUT:
			drawSel_copy_cut(APPDRAW, TRUE);
			break;
		//貼り付け
		case TRMENU_SEL_PASTE_NEWLAYER:
			drawSel_paste_newlayer(APPDRAW);
			break;
		//レイヤの不透明部分を選択
		case TRMENU_SEL_LAYER_OPACITY:
			drawSel_fromLayer(APPDRAW, 0);
			break;
		//レイヤの描画色部分を選択
		case TRMENU_SEL_LAYER_DRAWCOL:
			drawSel_fromLayer(APPDRAW, 1);
			break;
		//範囲内イメージを出力
		case TRMENU_SEL_OUTPUT_FILE:
			MainWindow_cmd_selectOutputFile(p);
			break;
	
		//---- 表示

		//最小化
		case TRMENU_VIEW_MINIMIZE:
			mToplevelMinimize(MLK_TOPLEVEL(p));
			break;
		//パネル:表示
		case TRMENU_VIEW_PANEL_VISIBLE:
			MainWindow_panel_all_toggle(p);

			_toolbar_check_toggle(p, id, ev);
			break;
		//パネル:すべてウィンドウモードに
		case TRMENU_VIEW_PANEL_ALL_WINDOW:
			MainWindow_panel_all_toggle_winmode(p, 1);
			break;
		//パネル:すべて格納
		case TRMENU_VIEW_PANEL_ALL_STORE:
			MainWindow_panel_all_toggle_winmode(p, 0);
			break;
		//キャンバス左右反転表示
		case TRMENU_VIEW_CANVAS_MIRROR:
			drawCanvas_mirror(APPDRAW);

			_toolbar_check_toggle(p, id, ev);
			break;
		//背景をチェック柄で表示
		case TRMENU_VIEW_BKGND_PLAID:
			APPCONF->fview ^= CONFIG_VIEW_F_BKGND_PLAID;

			_toolbar_check_toggle(p, id, ev);
			drawUpdate_all();
			break;
		//グリッド
		case TRMENU_VIEW_GRID:
			APPCONF->fview ^= CONFIG_VIEW_F_GRID;

			_toolbar_check_toggle(p, id, ev);
			drawUpdate_canvas();
			break;
		//分割線
		case TRMENU_VIEW_GRID_SPLIT:
			APPCONF->fview ^= CONFIG_VIEW_F_GRID_SPLIT;

			_toolbar_check_toggle(p, id, ev);
			drawUpdate_canvas();
			break;
		//定規のガイド
		case TRMENU_VIEW_RULE_GUIDE:
			APPCONF->fview ^= CONFIG_VIEW_F_RULE_GUIDE;
			drawUpdate_canvas();
			break;
		//ツールバー
		case TRMENU_VIEW_TOOLBAR:
			APPCONF->fview ^= CONFIG_VIEW_F_TOOLBAR;

			mWidgetShow(MLK_WIDGET(p->ib_toolbar), -1);
			mWidgetReLayout(MLK_WIDGET(p));
			break;
		//ステータスバー
		case TRMENU_VIEW_STATUSBAR:
			StatusBar_toggleVisible();
			break;
		//カーソル位置
		case TRMENU_VIEW_CURSOR_POS:
			APPCONF->fview ^= CONFIG_VIEW_F_CURSOR_POS;
			StatusBar_layout();
			break;
		//キャンバス操作時、レイヤ名表示
		case TRMENU_VIEW_LAYER_NAME:
			APPCONF->fview ^= CONFIG_VIEW_F_CANV_LAYER_NAME;
			APPDRAW->ttip_layer = NULL;
			break;
		//矩形選択時、座標を表示
		case TRMENU_VIEW_BOXSEL_POS:
			APPCONF->fview ^= CONFIG_VIEW_F_BOXSEL_POS;
			break;

		//表示倍率拡大
		case TRMENU_VIEW_CANVASZOOM_UP:
			drawCanvas_zoomStep(APPDRAW, TRUE);
			break;
		//表示倍率縮小
		case TRMENU_VIEW_CANVASZOOM_DOWN:
			drawCanvas_zoomStep(APPDRAW, FALSE);
			break;
		//表示倍率100%
		case TRMENU_VIEW_CANVASZOOM_ORIGINAL:
			drawCanvas_update(APPDRAW, 1000, 0,
				DRAWCANVAS_UPDATE_ZOOM | DRAWCANVAS_UPDATE_RESET_SCROLL);
			break;
		//ウィンドウに合わせる
		case TRMENU_VIEW_CANVASZOOM_FIT:
			drawCanvas_fitWindow(APPDRAW);
			break;

		//左回転
		case TRMENU_VIEW_CANVASROTATE_LEFT:
			drawCanvas_rotateStep(APPDRAW, TRUE);
			break;
		//右回転
		case TRMENU_VIEW_CANVASROTATE_RIGHT:
			drawCanvas_rotateStep(APPDRAW, FALSE);
			break;
		//角度指定
		case TRMENU_VIEW_CANVASROTATE_0:
		case TRMENU_VIEW_CANVASROTATE_90:
		case TRMENU_VIEW_CANVASROTATE_180:
		case TRMENU_VIEW_CANVASROTATE_270:
			drawCanvas_update(APPDRAW, 0, (id - TRMENU_VIEW_CANVASROTATE_0) * 9000,
				DRAWCANVAS_UPDATE_ANGLE | DRAWCANVAS_UPDATE_RESET_SCROLL);
			break;
	
		//---- 設定

		//環境設定
		case TRMENU_OPT_ENV:
			MainWindow_cmd_envoption(p);
			break;
		//グリッド設定
		case TRMENU_OPT_GRID:
			if(GridOptDialog_run(MLK_WINDOW(p)))
				drawUpdate_canvas();
			break;
		//メニューのキー設定
		case TRMENU_OPT_SHORTCUTKEY:
			MenuKeyOptDlg_run(MLK_WINDOW(p));
			break;
		//キャンバスキー設定
		case TRMENU_OPT_CANVASKEY:
			CanvasKeyDlg_run(MLK_WINDOW(p));
			break;
		//パネル配置設定
		case TRMENU_OPT_PANEL:
			MainWindow_cmd_panelLayout(p);
			break;
		//バージョン情報
		case TRMENU_OPT_VERSION:
			mSysDlg_about_license(MLK_WINDOW(p), _APP_VERSION_TEXT, _APP_LICENSE_TEXT);
			break;
	}
}

/** イベントハンドラ */

int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		//コマンド
		case MEVENT_COMMAND:
			_event_command(MAINWINDOW(wg), (mEventCommand *)ev);
			break;

		//メニュー:ポップアップ表示時
		case MEVENT_MENU_POPUP:
			if(ev->popup.is_menubar)
				MainWindow_on_menupopup(ev->popup.menu, ev->popup.submenu_id);
			break;

		//パネル
		case MEVENT_PANEL:
			if(ev->panel.act == MPANEL_ACT_CLOSE
				|| ev->panel.act == MPANEL_ACT_TOGGLE_STORE)
			{
				//閉じるかモードが変わった時、ペインの表示/非表示を再セット
				MainWindow_pane_layout(MAINWINDOW(wg), TRUE);
			}
			else if(ev->panel.act == MPANEL_ACT_TOGGLE_SHUT
				|| ev->panel.act == MPANEL_ACT_TOGGLE_RESIZE)
			{
				//状態変更時、再レイアウト
				mPanelStoreReLayout(ev->panel.panel);
			}
			break;
	
		//閉じるボタン
		case MEVENT_CLOSE:
			MainWindow_quit();
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}

