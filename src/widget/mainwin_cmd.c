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
 * MainWindow : コマンド
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_splitter.h"
#include "mlk_panel.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_mainwin.h"
#include "def_canvaskey.h"

#include "mainwindow.h"
#include "maincanvas.h"
#include "statusbar.h"
#include "panel.h"
#include "panel_func.h"
#include "dialogs.h"
#include "popup_thread.h"

#include "layeritem.h"
#include "tileimage.h"
#include "toollist.h"
#include "undo.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_layer.h"
#include "draw_toollist.h"
#include "draw_rule.h"

#include "pv_mainwin.h"

#include "trid.h"


//----------------

mlkbool PanelLayoutDlg_run(mWindow *parent);
int EnvOptionDlg_run(mWindow *parent);

//----------------


/** キャンバスでのキー押し時
 *
 * 即座に実行するタイプのコマンドを実行 */

void MainWindow_runCanvasKeyCmd(int cmd)
{
	AppDraw *p = APPDRAW;
	int n;

	if(cmd == 0) return;

	if(cmd >= CANVASKEY_CMD_TOOL && cmd < CANVASKEY_CMD_TOOL + TOOL_NUM)
	{
		//ツール変更

		drawTool_setTool(p, cmd - CANVASKEY_CMD_TOOL);
	}
	else if(cmd >= CANVASKEY_CMD_DRAWTYPE && cmd < CANVASKEY_CMD_DRAWTYPE + TOOLSUB_DRAW_NUM)
	{
		//描画タイプ変更

		n = p->tool.no;

		if(drawTool_isType_haveDrawType(n))
			drawTool_setTool_subtype(p, cmd - CANVASKEY_CMD_DRAWTYPE);
	}
	else if(cmd >= CANVASKEY_CMD_RULE && cmd < CANVASKEY_CMD_RULE + DRAW_RULE_TYPE_NUM - 1)
	{
		//定規ON/OFF

		drawRule_setType_onoff(APPDRAW, cmd - CANVASKEY_CMD_RULE + 1);

		PanelOption_changeRuleType();
	}
	else if(cmd >= CANVASKEY_CMD_OTHER && cmd < CANVASKEY_CMD_OTHER + CANVASKEY_CMD_OTHER_NUM)
	{
		//他コマンド

		cmd -= CANVASKEY_CMD_OTHER;

		switch(cmd)
		{
			//元に戻す/やり直す
			case CANVASKEY_CMD_OTHER_UNDO:
			case CANVASKEY_CMD_OTHER_REDO:
				MainWindow_undo_redo(APPWIDGET->mainwin, (cmd == CANVASKEY_CMD_OTHER_REDO));
				break;
			//一段階拡大/縮小
			case CANVASKEY_CMD_OTHER_ZOOM_UP:
			case CANVASKEY_CMD_OTHER_ZOOM_DOWN:
				drawCanvas_zoomStep(p, (cmd == CANVASKEY_CMD_OTHER_ZOOM_UP));
				break;
			//キャンバス回転リセット
			case CANVASKEY_CMD_OTHER_ROTATE_RESET:
				drawCanvas_update(p, 0, 0, DRAWCANVAS_UPDATE_ANGLE);
				break;
			//描画色/背景色切り替え
			case CANVASKEY_CMD_OTHER_TOGGLE_DRAWCOL:
				drawColor_toggleDrawCol(p);
				PanelColor_changeDrawColor();
				break;

			//一つ上/下のレイヤを選択
			case CANVASKEY_CMD_OTHER_LAYER_SEL_UP:
			case CANVASKEY_CMD_OTHER_LAYER_SEL_DOWN:
				drawLayer_selectUpDown(p, (cmd == CANVASKEY_CMD_OTHER_LAYER_SEL_UP));
				break;
			//カレントレイヤ表示/非表示
			case CANVASKEY_CMD_OTHER_LAYER_TOGGLE_VISIBLE:
				drawLayer_toggle_visible(p, p->curlayer);
				PanelLayer_update_curlayer(FALSE);
				break;
			//一つ次/前のブラシ選択
			case CANVASKEY_CMD_OTHER_TOOLLIST_NEXT:
			case CANVASKEY_CMD_OTHER_TOOLLIST_PREV:
				if(drawToolList_selectMove(p, (cmd == CANVASKEY_CMD_OTHER_TOOLLIST_NEXT)))
					PanelToolList_updateList();
				break;
			//直前のブラシと切り替え
			case CANVASKEY_CMD_OTHER_TOOLLIST_TOGGLE_SEL:
				if(drawToolList_toggleSelect_last(p))
					PanelToolList_updateList();
				break;
		}
	}
}

/** アンドゥ/リドゥ */

void MainWindow_undo_redo(MainWindow *p,mlkbool redo)
{
	UndoUpdateInfo info;
	mlkerr ret;

	//データがあるか

	if(redo)
	{
		if(!Undo_isHaveRedo()) return;
	}
	else
	{
		if(!Undo_isHaveUndo()) return;
	}

	//実行
	// :失敗時も更新は行う。

	ret = Undo_runUndoRedo(redo, &info);

	if(ret)
		MainWindow_errmes(ret, NULL);

	//更新

	switch(info.type)
	{
		//イメージ範囲
		case UNDO_UPDATE_RECT:
			drawUpdateRect_canvas_canvasview(APPDRAW, &info.rc);
			PanelLayer_update_layer(info.layer);
			break;

		//イメージ範囲 + レイヤ一覧 (全体)
		case UNDO_UPDATE_RECT_LAYERLIST:
			drawUpdateRect_canvas_canvasview(APPDRAW, &info.rc);
			PanelLayer_update_all();
			break;

		//レイヤ一覧全体
		case UNDO_UPDATE_LAYERLIST:
			PanelLayer_update_all();
			break;

		//全体イメージ + レイヤ一覧
		case UNDO_UPDATE_FULL_LAYERLIST:
			drawUpdate_all_layer();
			break;

		//イメージビット数の変更
		case UNDO_UPDATE_IMAGEBITS:
			drawImage_update_imageBits();
			break;

		//キャンバスサイズ変更
		case UNDO_UPDATE_CANVAS_RESIZE:
			drawImage_changeImageSize(APPDRAW, info.rc.x1, info.rc.y1);
			MainWindow_updateNewCanvas(p, NULL);
			break;
	}
}


//=================================
// 選択範囲
//=================================


/** 選択範囲の拡張/縮小 */

void MainWindow_cmd_selectExpand(MainWindow *p)
{
	int n;

	//選択範囲なし

	if(!drawSel_isHave()) return;

	//

	MLK_TRGROUP(TRGROUP_DLG_SELECT_EXPAND);

	n = 1;

	if(mSysDlg_inputTextNum(MLK_WINDOW(p), MLK_TR(0), MLK_TR(1),
		MSYSDLG_INPUTTEXT_F_SET_DEFAULT,
		-500, 500, &n))
	{
		if(n != 0)
			drawSel_expand(APPDRAW, n);
	}
}

/** 選択範囲内イメージを画像に出力 */

typedef struct
{
	char *filename;
	mRect rc;
}_thdata_seloutput;

static int _thread_seloutput(mPopupProgress *prog,void *data)
{
	_thdata_seloutput *p = (_thdata_seloutput *)data;

	return TileImage_savePNG_select_rgba(
		APPDRAW->curlayer->img, APPDRAW->tileimg_sel,
		p->filename, APPDRAW->imgdpi, &p->rc, prog);
}

void MainWindow_cmd_selectOutputFile(MainWindow *p)
{
	mStr str = MSTR_INIT;
	mRect rc;
	_thdata_seloutput dat;
	int ret;

	if(!drawSel_isEnable_outputFile()) return;

	//選択範囲の正確なイメージ範囲を取得 (キャンバス範囲外は除く)
	
	TileImage_A1_getHaveRect_real(APPDRAW->tileimg_sel, &rc);

	if(!drawCalc_clipImageRect(APPDRAW, &rc))
		return;

	//ファイル名取得

	if(mSysDlg_saveFile(MLK_WINDOW(p),
		"PNG (*.png)\tpng",
		0, APPCONF->strSelectFileDir.buf, 0, &str, NULL))
	{
		//ディレクトリ記録

		mStrPathGetDir(&APPCONF->strSelectFileDir, str.buf);

		//保存

		dat.filename = str.buf;
		dat.rc = rc;

		ret = PopupThread_run(&dat, _thread_seloutput);

		if(ret) MainWindow_errmes(ret, NULL);
	}

	mStrFree(&str);
}


//================================
// キャンバス関連
//================================


/** キャンバスサイズ変更 */

void MainWindow_cmd_resizeCanvas(MainWindow *p)
{
	mSize size;
	int sw,sh,topx,topy,n,opt,align;

	sw = APPDRAW->imgw;
	sh = APPDRAW->imgh;

	//ダイアログ

	size.w = sw;
	size.h = sh;

	if(!CanvasDialog_resize(MLK_WINDOW(p), &size, &opt))
		return;

	align = opt & 15;

	//サイズ変更なし

	if(size.w == sw && size.h == sh) return;

	//左上位置

	n = align % 3;

	if(n == 0) topx = 0;
	else if(n == 1) topx = (size.w - sw) >> 1;
	else topx = size.w - sw;

	n = align / 3;

	if(n == 0) topy = 0;
	else if(n == 1) topy = (size.h - sh) >> 1;
	else topy = size.h - sh;

	//実行

	if(!drawImage_resizeCanvas(APPDRAW, size.w, size.h, topx, topy, opt & 16))
		MainWindow_errmes(MLKERR_ALLOC, NULL);

	MainWindow_updateNewCanvas(p, NULL);
}

/** 画像を統合して拡大縮小 */

void MainWindow_cmd_scaleCanvas(MainWindow *p)
{
	CanvasScaleInfo info;

	//ダイアログ

	info.w = APPDRAW->imgw;
	info.h = APPDRAW->imgh;
	info.dpi = APPDRAW->imgdpi;

	if(!CanvasDialog_scale(MLK_WINDOW(p), &info))
		return;

	//サイズ変更なし

	if(info.w == APPDRAW->imgw && info.h == APPDRAW->imgh)
		return;

	//実行

	if(!drawImage_scaleCanvas(APPDRAW, info.w, info.h, info.dpi, info.method))
		MainWindow_errmes(MLKERR_ALLOC, NULL);
	
	MainWindow_updateNewCanvas(p, NULL);
}


//================================
// 設定関連
//================================


/** パネル配置設定 */

void MainWindow_cmd_panelLayout(MainWindow *p)
{
	if(PanelLayoutDlg_run(MLK_WINDOW(p)))
	{
		//ペインの再配置
		
		MainWindow_pane_layout(p, FALSE);

		//パネルを再配置
	
		Panels_relocate();

		//レイアウト

		mWidgetReLayout(p->ct_main);
	}
}

/** 環境設定 */

void MainWindow_cmd_envoption(MainWindow *p)
{
	int f;

	f = EnvOptionDlg_run(MLK_WINDOW(p));

	//キャンバス

	if(f & (1<<1))
	{
		//すべて変更 (チェック柄背景色変更)
		// :キャンバス背景色も変わっている場合あり

		drawUpdate_all();
	}
	else if(f & (1<<0))
	{
		//キャンバスウィジェットの更新
		// :キャンバス背景色、定規ガイドの色
		
		drawUpdate_canvas();
		PanelCanvasView_update();
	}

	//ツールバーボタン

	if(f & (1<<2))
		MainWindow_createToolBar(p, FALSE);

	//描画カーソル変更
	// :関数内でカーソルの変更と再セットが行われている。
	// :現在のカーソルとして使われている場合、削除前に取り外す必要がある。

	if(f & (1<<3))
		MainCanvasPage_changeDrawCursor();
}


//================================
// 表示関連
//================================


/* ペインレイアウト: ウィジェットを追加 */

static void _pane_add(mWidget *wg,mWidget *parent,int fshow)
{
	//パネル自体の表示/非表示時に、常に非表示のものと区別するため
	wg->param1 = fshow;

	mWidgetTreeAdd(wg, parent);
	mWidgetShow(wg, fshow);
}

/** ペインのレイアウト
 *
 * mWidget:id = ペインは 0、スプリッターは 0 以外。
 * mWidget:param1 = 0 で非表示、1 で表示。 */

void MainWindow_pane_layout(MainWindow *p,mlkbool relayout)
{
	mWidget *wg,*parent;
	char *pc;
	int i,no,spno;
	uint8_t fhave[PANEL_PANE_NUM];

	parent = p->ct_main;

	//各ペインにパネルが一つでもあるか

	mMemset0(fhave, PANEL_PANE_NUM);

	for(i = 0; i < PANEL_NUM; i++)
	{
		if(mPanelIsStoreVisible(APPWIDGET->panel[i]))
		{
			no = Panel_getPaneNo(i);
			if(no != -1)
				fhave[no] = 1;
		}
	}

	//すべてツリーから取り外す

	mWidgetTreeRemove_child(parent);

	//ペインとキャンバスを配置順にツリーに追加
	// :スプリッターは配置順に使用していく。

	spno = 0;

	for(pc = APPCONF->panel.pane_layout; *pc; pc++)
	{
		no = *pc - '0'; //0=キャンバス, 1-3=ペイン番号

		wg = (no == 0)? (mWidget *)APPWIDGET->canvas: APPWIDGET->pane[no - 1];

		//追加

		if(no == 0 || fhave[no - 1])
		{
			_pane_add(wg, parent, 1);

			//終端の場合はスプリッターは追加しない
			if(pc[1])
			{
				_pane_add(p->splitter[spno], parent, 1);
				spno++;
			}
		}
	}

	//終端にスプリッターがある場合は、取り外す
	// :ペインの id は 0

	if(parent->last->id)
		mWidgetTreeRemove(parent->last);

	//取り外されているウィジェットは、非表示にして終端へ追加
	// :ペインを取り外したままにすると、パネルの状態が切り替わった時に、
	// :ウィジェットツリーが繋がらない場合があり、正しく表示できない。

	for(i = 0; i < PANEL_PANE_NUM; i++)
	{
		//ペイン
		
		wg = APPWIDGET->pane[i];
		if(!(wg->parent))
			_pane_add(wg, parent, 0);

		//スプリッター

		wg = p->splitter[i];
		if(!(wg->parent))
			_pane_add(wg, parent, 0);
	}

	//再レイアウト

	if(relayout)
		mWidgetReLayout(parent);
}

/** すべてのペインとスプリッターの表示/非表示を反転 */

void MainWindow_pane_toggle(MainWindow *p)
{
	int i;

	//param1 が 0 の場合は、非表示で固定

	for(i = 0; i < PANEL_PANE_NUM; i++)
	{
		if(APPWIDGET->pane[i]->param1)
			mWidgetShow(APPWIDGET->pane[i], -1);

		if(p->splitter[i]->param1)
			mWidgetShow(p->splitter[i], -1);
	}
}

/** 各パネルの表示/非表示を切替 */

void MainWindow_panel_toggle(MainWindow *p,int no)
{
	if(APPCONF->fview & CONFIG_VIEW_F_PANEL)
	{
		mPanelSetCreate(APPWIDGET->panel[no], -1);

		MainWindow_pane_layout(p, TRUE);
	}
}

/** パネル自体の表示/非表示を切り替え */

void MainWindow_panel_all_toggle(MainWindow *p)
{
	int i;

	APPCONF->fview ^= CONFIG_VIEW_F_PANEL;

	//各パネルの表示状態を反転

	for(i = 0; i < PANEL_NUM; i++)
		mPanelSetVisible(APPWIDGET->panel[i], -1);

	//すべてのペインとスプリッターの表示を反転

	MainWindow_pane_toggle(p);

	//非表示->表示の時はペインの再セット

	if(APPCONF->fview & CONFIG_VIEW_F_PANEL)
		MainWindow_pane_layout(p, TRUE);
	else
		mWidgetReLayout(p->ct_main);
}

/** すべてのパネルのウィンドウ/格納状態を変更 */

void MainWindow_panel_all_toggle_winmode(MainWindow *p,int type)
{
	if(APPCONF->fview & CONFIG_VIEW_F_PANEL)
	{
		Panels_toggle_winmode(type);

		MainWindow_pane_layout(p, TRUE);
	}
}

