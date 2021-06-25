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

/*************************************
 * 設定ファイルの読み書き
 *************************************/

#include <stdlib.h>
#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_window.h"
#include "mlk_panel.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_iniread.h"
#include "mlk_iniwrite.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_widget.h"
#include "def_draw.h"
#include "def_draw_toollist.h"
#include "def_tool_option.h"
#include "def_saveopt.h"
#include "def_canvaskey.h"

#include "appconfig.h"
#include "filter_save_param.h"

#include "configfile.h"

//-------------------

//パネル名
static const char *g_panel_name[] = {
	"tool", "toollist", "brushopt", "option", "layer",
	"color", "colwheel", "colpal",
	"cvctrl", "cvview", "imgview", "filterlist"
};

//-------------------



/********************************
 * 読み込み
 ********************************/


/* 読み込み前のデータ初期化 */

static void _load_init_data(AppConfig *cf,AppDraw *draw)
{
	int i;

	//起動時初期サイズ

	cf->newcanvas_init.w = cf->newcanvas_init.h = 400;
	cf->newcanvas_init.dpi = 300;
	cf->newcanvas_init.bit = 8;

	//キャンバスビュー:ボタン操作

	cf->canvview.bttcmd[CONFIG_PANELVIEW_BTT_LEFT] = 1;
	cf->canvview.bttcmd[CONFIG_PANELVIEW_BTT_LEFT_CTRL] = 2;

	//イメージビューア:ボタン操作

	cf->imgviewer.bttcmd[CONFIG_PANELVIEW_BTT_LEFT_CTRL] = 1;
	cf->imgviewer.bttcmd[CONFIG_PANELVIEW_BTT_LEFT_SHIFT] = 2;
	cf->imgviewer.bttcmd[CONFIG_PANELVIEW_BTT_RIGHT] = 2;

	//中間色:左右の色

	for(i = 0; i < DRAW_GRADBAR_NUM; i++)
		draw->col.gradcol[i][1] = 0xffffff;

	//水彩プリセット

	AppConfig_setWaterPreset_default();

	//ポインタデバイス:デフォルトのボタン操作
	// 右ボタン:スポイト
	// 中ボタン:キャンバス移動
	// ホイール上:一段階拡大
	// ホイール下:一段階縮小

	cf->pointer_btt_default[2] = CANVASKEY_OP_TOOL + TOOL_SPOIT;
	cf->pointer_btt_default[3] = CANVASKEY_OP_TOOL + TOOL_CANVAS_MOVE;
	cf->pointer_btt_default[4] = CANVASKEY_CMD_OTHER + CANVASKEY_CMD_OTHER_ZOOM_UP;
	cf->pointer_btt_default[5] = CANVASKEY_CMD_OTHER + CANVASKEY_CMD_OTHER_ZOOM_DOWN;
}

/* ConfigNewCanvas 読み込み (w,h,dpi,unit,bit) */

static void _load_newcanvas(mIniRead *ini,const char *key,ConfigNewCanvas *dst)
{
	int32_t val[5];

	if(mIniRead_getNumbers(ini, key, val, 5, 4, FALSE) < 5)
		return;

	dst->w = val[0];
	dst->h = val[1];
	dst->dpi = val[2];
	dst->unit = val[3];
	dst->bit = val[4];
}

/* 新規作成サイズ (履歴/登録) */

static void _load_newcanvas_array(mIniRead *ini,ConfigNewCanvas *buf)
{
	int32_t i;
	char m[16];

	for(i = 0; i < CONFIG_NEWCANVAS_NUM; i++)
	{
		snprintf(m, 16, "%d", i);

		_load_newcanvas(ini, m, buf + i);
	}
}

/* 定規記録データ読み込み */

static void _load_draw_rule_record(mIniRead *ini)
{
	const char *key,*param;
	char *end;
	RuleRecord *rec;
	int no,i;

	if(!mIniRead_setGroup(ini, "rule_record")) return;

	while(mIniRead_getNextItem(ini, &key, &param))
	{
		no = atoi(key);
		if(no < 0 || no >= DRAW_RULE_RECORD_NUM) continue;

		rec = APPDRAW->rule.record + no;
	
		//タイプ

		rec->type = strtoul(param, &end, 10);

		if(rec->type == 0 || !(*end)) continue;

		//値

		for(i = 0; i < 4; i++)
		{
			rec->dval[i] = strtod(end + 1, &end);
			if(!(*end)) break;
		}
	}
}

/* ウィンドウ状態読み込み */

static void _load_winstate(mIniRead *ini,const char *key,mToplevelSaveState *state)
{
	int32_t n[7];

	if(mIniRead_getNumbers(ini, key, n, 7, 4, FALSE) == 7)
	{
		state->x = n[0];
		state->y = n[1];
		state->w = n[2];
		state->h = n[3];
		state->norm_x = n[4];
		state->norm_y = n[5];
		state->flags = n[6];
	}
}

/* ウィジェット状態読み込み */

static void _load_widgets_state(mIniRead *ini,AppConfig *cf,ConfigFileState *dst)
{
	int i;
	int32_t val[2];
	mStr str = MSTR_INIT;
	mPanelState *pst;
	const uint16_t inith[] = { 105/*tool*/,200/*toollist*/,300/*brushopt*/,
		230/*option*/,200/*layer*/,166/*color*/,170/*wheel*/,143/*palette*/,
		63/*canvctrl*/,212/*canvview*/,190/*imgview*/,200/*filter*/ };

	mIniRead_setGroup(ini, "widgets");

	//メインウィンドウ

	_load_winstate(ini, "mainwin", &dst->mainst);

	//テキストダイアログ

	_load_winstate(ini, "textdlg", &cf->winstate_textdlg);

	cf->textdlg_toph = mIniRead_getInt(ini, "textdlg_toph", 200);

	//フィルタダイアログ

	_load_winstate(ini, "filterdlg", &cf->winstate_filterdlg);

	//ペイン幅

	for(i = 0; i < PANEL_PANE_NUM; i++)
		dst->pane_w[i] = 250;

	mIniRead_getNumbers(ini, "pane_width", dst->pane_w, PANEL_PANE_NUM, 4, FALSE);

	//パネル

	for(i = 0; i < PANEL_NUM; i++)
	{
		pst = dst->panelst + i;
		
		//ウィンドウ状態

		mStrSetFormat(&str, "%s_win", g_panel_name[i]);

		_load_winstate(ini, str.buf, &pst->winstate);

		if(!pst->winstate.w || !pst->winstate.h)
		{
			pst->winstate.w = 250;
			pst->winstate.h = 250;
		}

		//ほか値

		mStrSetFormat(&str, "%s_st", g_panel_name[i]);

		if(mIniRead_getNumbers(ini, str.buf, val, 2, 4, FALSE) == 2)
		{
			pst->height = val[0];
			pst->flags = val[1];
		}
		else
		{
			//デフォルト

			pst->height = inith[i];
			pst->flags = MPANEL_F_CREATED | MPANEL_F_VISIBLE;
		}
	}

	mStrFree(&str);
}

/* AppConfig 読み込み */

static void _load_configdata(mIniRead *ini,AppConfig *cf)
{
	int no;
	uint32_t u32;

	//----- 環境

	mIniRead_setGroup(ini, "env");

	_load_newcanvas(ini, "initimg", &cf->newcanvas_init);

	cf->loadimg_default_bits = mIniRead_getInt(ini, "loadimg_bits", 8);
	cf->canvas_zoom_step_hi = mIniRead_getInt(ini, "zoomstep_hi", 100);
	cf->canvas_angle_step = mIniRead_getInt(ini, "anglestep", 15);
	cf->tone_lines_default = mIniRead_getInt(ini, "tone_lines_default", 600);
	cf->iconsize_toolbar = mIniRead_getInt(ini, "iconsize_toolbar", 20);
	cf->iconsize_panel_tool = mIniRead_getInt(ini, "iconsize_panel_tool", 16);
	cf->iconsize_other = mIniRead_getInt(ini, "iconsize_other", 16);
	cf->canvas_scale_method = mIniRead_getInt(ini, "canvas_scale_method", 0);

	cf->undo_maxbufsize = mIniRead_getInt(ini, "undo_maxbufsize", 10 * 1024 * 1024);
	cf->undo_maxnum = mIniRead_getInt(ini, "undo_maxnum", 100);
	cf->savedup_type = mIniRead_getInt(ini, "savedup_type", 0);

	mIniRead_getNumbers(ini, "cursor_hotspot", cf->cursor_hotspot, 2, 2, FALSE);

	//hex

	cf->fview = mIniRead_getHex(ini, "fview",
		CONFIG_VIEW_F_TOOLBAR | CONFIG_VIEW_F_STATUSBAR | CONFIG_VIEW_F_PANEL
		| CONFIG_VIEW_F_RULE_GUIDE | CONFIG_VIEW_F_FILTERDLG_PREVIEW);

	cf->foption = mIniRead_getHex(ini, "foption",
		CONFIG_OPTF_MES_SAVE_OVERWRITE | CONFIG_OPTF_MES_SAVE_APD);

	cf->canvasbkcol = mIniRead_getHex(ini, "canvasbkcol", 0xc0c0c0);
	cf->rule_guide_col = mIniRead_getHex(ini, "rule_guide_col", 0x40ff0000);

	mIniRead_getNumbers(ini, "textsize_recent", cf->textsize_recent, CONFIG_TEXTSIZE_RECENT_NUM, 2, TRUE);

	//ツールバーボタン

	cf->toolbar_btts = mIniRead_getBase64(ini, "toolbar_btts", &cf->toolbar_btts_size);

	//----- パネル関連

	mIniRead_setGroup(ini, "panel");

	mIniRead_getTextBuf(ini, "pane_layout", cf->panel.pane_layout, 5, "3012");
	mIniRead_getTextBuf(ini, "panel_layout", cf->panel.panel_layout, 16, "tocs:wpbf:ivrl");

	cf->panel.color_type = mIniRead_getInt(ini, "color_type", 0);
	cf->panel.colwheel_type = mIniRead_getInt(ini, "colwheel_type", 0);
	cf->panel.colpal_type = mIniRead_getInt(ini, "colpal_type", 0);
	cf->panel.option_type = mIniRead_getInt(ini, "option_type", 0);
	cf->panel.toollist_sizelist_h = mIniRead_getInt(ini, "toollist_sizelist_h", 60);

	cf->panel.brushopt_flags = mIniRead_getHex(ini, "brushopt_flags", 0);

	mIniRead_getNumbers(ini, "water_preset", cf->panel.water_preset, CONFIG_WATER_PRESET_NUM, 4, TRUE);

	//---- 変形ダイアログ

	mIniRead_setGroup(ini, "transform");

	cf->transform.view_w = mIniRead_getInt(ini, "view_w", 500);
	cf->transform.view_h = mIniRead_getInt(ini, "view_h", 500);
	cf->transform.flags = mIniRead_getInt(ini, "flags", 0);

	//----- キャンバスビュー

	mIniRead_setGroup(ini, "canvasview");

	cf->canvview.flags = mIniRead_getInt(ini, "flags", CONFIG_CANVASVIEW_F_TOOLBAR_VISIBLE | CONFIG_CANVASVIEW_F_FIT);
	cf->canvview.zoom = mIniRead_getInt(ini, "zoom", 1000);

	mIniRead_getNumbers(ini, "btt", cf->canvview.bttcmd, 5, 1, FALSE);

	//----- イメージビューア

	mIniRead_setGroup(ini, "imgviewer");

	mIniRead_getTextStr(ini, "dir", &cf->strImageViewerDir, NULL);

	cf->imgviewer.flags = mIniRead_getInt(ini, "flags", CONFIG_IMAGEVIEWER_F_FIT);

	//ボタン

	mIniRead_getNumbers(ini, "btt", cf->imgviewer.bttcmd, 5, 1, FALSE);

	//----- グリッド

	mIniRead_setGroup(ini, "grid");

	cf->grid.gridw = mIniRead_getInt(ini, "gridw", 32);
	cf->grid.gridh = mIniRead_getInt(ini, "gridh", 32);
	cf->grid.col_grid = mIniRead_getHex(ini, "gridcol", 0x200040ff);

	cf->grid.splith = mIniRead_getInt(ini, "sph", 2);
	cf->grid.splitv = mIniRead_getInt(ini, "spv", 2);
	cf->grid.col_split = mIniRead_getHex(ini, "spcol", 0x20ff00ff);

	cf->grid.pxgrid_zoom = mIniRead_getHex(ini, "pxgrid", 0x8000 | 8);
	
	mIniRead_getNumbers(ini, "recent", cf->grid.recent, CONFIG_GRID_RECENT_NUM, 4, TRUE);

	//----- 保存設定

	mIniRead_setGroup(ini, "save");

	cf->save.png = mIniRead_getInt(ini, "png", SAVEOPT_PNG_DEFAULT);
	cf->save.jpeg = mIniRead_getInt(ini, "jpeg", SAVEOPT_JPEG_DEFAULT);
	cf->save.tiff = mIniRead_getInt(ini, "tiff", SAVEOPT_TIFF_DEFAULT);
	cf->save.webp = mIniRead_getInt(ini, "webp", SAVEOPT_WEBP_DEFAULT);
	cf->save.psd = mIniRead_getInt(ini, "psd", SAVEOPT_PSD_DEFAULT);

	//----- 操作設定

	mIniRead_setGroup(ini, "pointer");

	mIniRead_getNumbers(ini, "default", cf->pointer_btt_default, CONFIG_POINTERBTT_NUM, 1, FALSE);
	mIniRead_getNumbers(ini, "pentab", cf->pointer_btt_pentab, CONFIG_POINTERBTT_NUM, 1, FALSE);

	//----- 文字列

	mIniRead_setGroup(ini, "string");

	mIniRead_getTextStr(ini, "font_panel", &cf->strFont_panel, NULL);
	mIniRead_getTextStr(ini, "tempdir", &cf->strTempDir, NULL);
	mIniRead_getTextStr(ini, "filedlgdir", &cf->strFileDlgDir, NULL);
	mIniRead_getTextStr(ini, "layerfiledir", &cf->strLayerFileDir, NULL);
	mIniRead_getTextStr(ini, "selectfiledir", &cf->strSelectFileDir, NULL);
	mIniRead_getTextStr(ini, "fontfiledir", &cf->strFontFileDir, NULL);
	mIniRead_getTextStr(ini, "user_texdir", &cf->strUserTextureDir, NULL);
	mIniRead_getTextStr(ini, "user_brushdir", &cf->strUserBrushDir, NULL);
	mIniRead_getTextStr(ini, "cursor_file", &cf->strCursorFile, NULL);

	//------ ファイル履歴

	mIniRead_setGroup(ini, "recentfile");
	mIniRead_getTextStrArray(ini, 0, cf->strRecentFile, CONFIG_RECENTFILE_NUM);

	//------ 開いたディレクトリ履歴

	mIniRead_setGroup(ini, "recent_opendir");
	mIniRead_getTextStrArray(ini, 0, cf->strRecentOpenDir, CONFIG_RECENTDIR_NUM);

	//------ 保存ディレクトリ履歴

	mIniRead_setGroup(ini, "recent_savedir");
	mIniRead_getTextStrArray(ini, 0, cf->strRecentSaveDir, CONFIG_RECENTDIR_NUM);

	//----- 新規作成サイズ

	mIniRead_setGroup(ini, "newcanvas_recent");
	_load_newcanvas_array(ini, cf->newcanvas_recent);

	mIniRead_setGroup(ini, "newcanvas_record");
	_load_newcanvas_array(ini, cf->newcanvas_record);

	//----- キャンバスキー

	if(mIniRead_setGroup(ini, "canvaskey"))
	{
		while(mIniRead_getNextItem_keyno_int32(ini, &no, &u32, FALSE))
			cf->canvaskey[no] = u32;
	}

	//----- フィルタデータ

	FilterParam_getConfig(&cf->list_filterparam, ini);
}

/* 色マスク読み込み */

static void _load_colmask(mIniRead *ini,AppDraw *p)
{
	int i,n;
	uint32_t col[DRAW_COLORMASK_NUM];

	n = mIniRead_getNumbers(ini, "colmask", col, DRAW_COLORMASK_NUM, 4, TRUE);

	for(i = 0; i < n; i++)
		RGBA32bit_to_RGBA8(p->col.maskcol + i, col[i]);
}

/* AppDraw 読み込み */

static void _load_drawdata(mIniRead *ini,AppDraw *p)
{
	int32_t n;

	//------- main

	mIniRead_setGroup(ini, "draw_main");

	mIniRead_getTextStr(ini, "opt_texture", &p->strOptTexturePath, NULL);

	//------- カラー

	mIniRead_setGroup(ini, "draw_color");

	RGB32bit_to_RGBcombo(&p->col.drawcol, mIniRead_getHex(ini, "drawcol", 0));
	RGB32bit_to_RGBcombo(&p->col.bkgndcol, mIniRead_getHex(ini, "bkgndcol", 0xffffff));
	RGB32bit_to_RGBcombo(&p->col.checkbkcol[0], mIniRead_getHex(ini, "checkbkcol1", 0xd0d0d0));
	RGB32bit_to_RGBcombo(&p->col.checkbkcol[1], mIniRead_getHex(ini, "checkbkcol2", 0xf0f0f0));

	_load_colmask(ini, p);

	mIniRead_getNumbers(ini, "gradbarcol", p->col.gradcol, DRAW_GRADBAR_NUM * 2, 4, TRUE);
	mIniRead_getNumbers(ini, "layercolpal", p->col.layercolpal, DRAW_LAYERCOLPAL_NUM, 4, TRUE);

	//パレット選択
	
	n = mIniRead_getInt(ini, "pal_sel", 0);
	p->col.cur_pallist = mListGetItemAtIndex(&p->col.list_pal, n);

	//------- ツール

	mIniRead_setGroup(ini, "draw_tool");

	p->tool.no = mIniRead_getInt(ini, "tool", 0);
	
	mIniRead_getNumbers(ini, "toolsub", p->tool.subno, TOOL_NUM, 1, FALSE);

	p->tool.opt_dotpen = mIniRead_getHex(ini, "opt_dotpen", TOOLOPT_DOTPEN_DEFAULT);
	p->tool.opt_dotpen_erase = mIniRead_getHex(ini, "opt_dotpen_erase", TOOLOPT_DOTPEN_DEFAULT);
	p->tool.opt_finger = mIniRead_getHex(ini, "opt_finger", TOOLOPT_FINGER_DEFAULT);
	p->tool.opt_fillpoly = mIniRead_getHex(ini, "opt_fillpoly", TOOLOPT_FILLPOLY_DEFAULT);
	p->tool.opt_fillpoly_erase = mIniRead_getHex(ini, "opt_fillpoly_erase", TOOLOPT_FILLPOLY_DEFAULT);
	p->tool.opt_fill = mIniRead_getHex(ini, "opt_fill", TOOLOPT_FILL_DEFAULT);
	p->tool.opt_grad = mIniRead_getHex(ini, "opt_grad", TOOLOPT_GRAD_DEFAULT);
	p->tool.opt_select = mIniRead_getHex(ini, "opt_select", TOOLOPT_SELECT_DEFAULT);
	p->tool.opt_selfill = mIniRead_getInt(ini, "opt_selfill", TOOLOPT_SELFILL_DEFAULT);
	p->tool.opt_stamp = mIniRead_getInt(ini, "opt_stamp", TOOLOPT_STAMP_DEFAULT);
	p->tool.opt_cutpaste = mIniRead_getInt(ini, "opt_cutpaste", TOOLOPT_CUTPASTE_DEFAULT);
	p->tool.opt_boxedit = mIniRead_getInt(ini, "opt_boxedit", 0);

	//------ ブラシ

	mIniRead_setGroup(ini, "draw_brush");

	mIniRead_getNumbers(ini, "press_comm", p->tlist->pt_press_comm, CURVE_SPLINE_MAXNUM, 4, TRUE);

	//-------- 入り抜き

	mIniRead_setGroup(ini, "draw_headtail");

	p->headtail.selno = mIniRead_getInt(ini, "sel", 0);
	p->headtail.curval[0] = mIniRead_getHex(ini, "line", 0);
	p->headtail.curval[1] = mIniRead_getHex(ini, "bezier", 0);

	mIniRead_getNumbers(ini, "regist", p->headtail.regist, DRAW_HEADTAIL_REGIST_NUM, 4, TRUE);

	//------- テキスト描画

	mIniRead_setGroup(ini, "draw_text");

	p->text.fpreview = mIniRead_getInt(ini, "preview", 1);

	mIniRead_getTextStr(ini, "font", &p->text.dt.str_font, NULL);
	mIniRead_getTextStr(ini, "style", &p->text.dt.str_style, NULL);

	p->text.dt.font_param = mIniRead_getInt(ini, "font_param", 0);
	p->text.dt.fontsize = mIniRead_getInt(ini, "fontsize", 90);
	p->text.dt.rubysize = mIniRead_getInt(ini, "rubysize", 50);
	p->text.dt.dpi = mIniRead_getInt(ini, "dpi", 96);
	p->text.dt.char_space = mIniRead_getInt(ini, "char_space", 0);
	p->text.dt.line_space = mIniRead_getInt(ini, "line_space", 0);
	p->text.dt.ruby_pos = mIniRead_getInt(ini, "ruby_pos", 0);
	p->text.dt.angle = mIniRead_getInt(ini, "angle", 0);

	p->text.dt.fontsel = mIniRead_getInt(ini, "fontsel", 0);
	p->text.dt.unit_fontsize = mIniRead_getInt(ini, "unit_fontsize", 0);
	p->text.dt.unit_rubysize = mIniRead_getInt(ini, "unit_rubysize", 0);
	p->text.dt.unit_char_space = mIniRead_getInt(ini, "unit_charsp", 0);
	p->text.dt.unit_line_space = mIniRead_getInt(ini, "unit_linesp", 0);
	p->text.dt.unit_ruby_pos = mIniRead_getInt(ini, "unit_ruby_pos", 0);
	p->text.dt.hinting = mIniRead_getInt(ini, "hinting", 0);
	
	p->text.dt.flags = mIniRead_getHex(ini, "flags", 0);

	//---- 定規

	_load_draw_rule_record(ini);
}

/** 設定ファイル読み込み
 *
 * dst: あらかじめゼロクリアされている */

void app_load_config(ConfigFileState *dst)
{
	mIniRead *ini;

	mIniRead_loadFile_join(&ini, mGuiGetPath_config_text(), CONFIG_FILENAME_MAIN);
	if(!ini) return;

	//バージョン

	mIniRead_setGroup(ini, "azpainter");

	if(mIniRead_getInt(ini, "ver", 0) != 3)
		mIniRead_setEmpty(ini);

	//読み込み前の初期化

	_load_init_data(APPCONF, APPDRAW);

	//ウィジェット状態

	_load_widgets_state(ini, APPCONF, dst);

	//AppConfig

	_load_configdata(ini, APPCONF);

	//AppDraw

	_load_drawdata(ini, APPDRAW);

	//mlk

	mGuiReadIni_system(ini);

	mIniRead_end(ini);
}


/********************************
 * 書き込み
 ********************************/


/* ConfigNewCanvas 書き込み */

static void _save_newcanvas(FILE *fp,const char *key,ConfigNewCanvas *src)
{
	int32_t val[5];

	val[0] = src->w;
	val[1] = src->h;
	val[2] = src->dpi;
	val[3] = src->unit;
	val[4] = src->bit;

	mIniWrite_putNumbers(fp, key, val, 5, 4, FALSE);
}

/* ConfigNewCanvas 配列書き込み */

static void _save_newcanvas_array(FILE *fp,ConfigNewCanvas *buf)
{
	int32_t i;
	char m[16];

	for(i = 0; i < CONFIG_NEWCANVAS_NUM; i++, buf++)
	{
		if(buf->bit == 0) break;
	
		snprintf(m, 16, "%d", i);
		_save_newcanvas(fp, m, buf);
	}
}

/* 定規記録データ 書き込み */

static void _save_draw_rule_record(FILE *fp)
{
	RuleRecord *rec = APPDRAW->rule.record;
	int i;

	mIniWrite_putGroup(fp, "rule_record");

	for(i = 0; i < DRAW_RULE_RECORD_NUM; i++, rec++)
	{
		if(rec->type)
		{
			fprintf(fp, "%d=%d,%e,%e,%e,%e\n",
				i, rec->type, rec->dval[0], rec->dval[1], rec->dval[2], rec->dval[3]);
		}
	}
}

/* ウィンドウ状態書き込み */

static void _save_winstate(FILE *fp,const char *key,mToplevel *win,mToplevelSaveState *state)
{
	mToplevelSaveState st;
	int32_t n[7];

	if(state)
		st = *state;
	else
		mToplevelGetSaveState(win, &st);

	n[0] = st.x;
	n[1] = st.y;
	n[2] = st.w;
	n[3] = st.h;
	n[4] = st.norm_x;
	n[5] = st.norm_y;
	n[6] = st.flags;

	mIniWrite_putNumbers(fp, key, n, 7, 4, FALSE);
}

/* ウィジェット状態書き込み */

static void _save_widgets_state(FILE *fp,AppConfig *cf)
{
	AppWidgets *wg = APPWIDGET;
	mPanelState st;
	int i;
	int32_t val[PANEL_PANE_NUM];
	mStr str = MSTR_INIT;

	mIniWrite_putGroup(fp, "widgets");

	//メインウィンドウ

	_save_winstate(fp, "mainwin", MLK_TOPLEVEL(wg->mainwin), NULL);

	//テキストダイアログ

	_save_winstate(fp, "textdlg", NULL, &cf->winstate_textdlg);

	mIniWrite_putInt(fp, "textdlg_toph", cf->textdlg_toph);

	//フィルタダイアログ

	_save_winstate(fp, "filterdlg", NULL, &cf->winstate_filterdlg);

	//ペイン幅

	for(i = 0; i < PANEL_PANE_NUM; i++)
		val[i] = wg->pane[i]->w;

	mIniWrite_putNumbers(fp, "pane_width", val, PANEL_PANE_NUM, 4, FALSE);

	//パネル

	for(i = 0; i < PANEL_NUM; i++)
	{
		if(!wg->panel[i]) continue;

		mPanelGetState(wg->panel[i], &st);

		//ウィンドウ状態

		mStrSetFormat(&str, "%s_win", g_panel_name[i]);

		_save_winstate(fp, str.buf, NULL, &st.winstate);

		//ほか値

		mStrSetFormat(&str, "%s_st", g_panel_name[i]);

		val[0] = st.height;
		val[1] = st.flags;

		mIniWrite_putNumbers(fp, str.buf, val, 2, 4, FALSE);
	}

	mStrFree(&str);
}

/* AppConfig */

static void _save_configdata(FILE *fp,AppConfig *cf)
{
	int i;

	//----- env

	mIniWrite_putGroup(fp, "env");

	_save_newcanvas(fp, "initimg", &cf->newcanvas_init);

	mIniWrite_putInt(fp, "loadimg_bits", cf->loadimg_default_bits);
	mIniWrite_putInt(fp, "zoomstep_hi", cf->canvas_zoom_step_hi);
	mIniWrite_putInt(fp, "anglestep", cf->canvas_angle_step);
	mIniWrite_putInt(fp, "tone_lines_default", cf->tone_lines_default);
	mIniWrite_putInt(fp, "iconsize_toolbar", cf->iconsize_toolbar);
	mIniWrite_putInt(fp, "iconsize_panel_tool", cf->iconsize_panel_tool);
	mIniWrite_putInt(fp, "iconsize_other", cf->iconsize_other);
	mIniWrite_putInt(fp, "canvas_scale_method", cf->canvas_scale_method);

	mIniWrite_putInt(fp, "undo_maxbufsize", cf->undo_maxbufsize);
	mIniWrite_putInt(fp, "undo_maxnum", cf->undo_maxnum);
	mIniWrite_putInt(fp, "savedup_type", cf->savedup_type);

	mIniWrite_putNumbers(fp, "cursor_hotspot", cf->cursor_hotspot, 2, 2, FALSE);

	//hex

	mIniWrite_putHex(fp, "fview", cf->fview);
	mIniWrite_putHex(fp, "foption", cf->foption);
	mIniWrite_putHex(fp, "canvasbkcol", cf->canvasbkcol);
	mIniWrite_putHex(fp, "rule_guide_col", cf->rule_guide_col);

	mIniWrite_putNumbers(fp, "textsize_recent", cf->textsize_recent, CONFIG_TEXTSIZE_RECENT_NUM, 2, TRUE);

	//

	if(cf->toolbar_btts)
		mIniWrite_putBase64(fp, "toolbar_btts", cf->toolbar_btts, cf->toolbar_btts_size);

	//----- パネル関連

	mIniWrite_putGroup(fp, "panel");

	mIniWrite_putText(fp, "pane_layout", cf->panel.pane_layout);
	mIniWrite_putText(fp, "panel_layout", cf->panel.panel_layout);

	mIniWrite_putInt(fp, "color_type", cf->panel.color_type);
	mIniWrite_putInt(fp, "colwheel_type", cf->panel.colwheel_type);
	mIniWrite_putInt(fp, "colpal_type", cf->panel.colpal_type);
	mIniWrite_putInt(fp, "option_type", cf->panel.option_type);
	mIniWrite_putInt(fp, "toollist_sizelist_h", cf->panel.toollist_sizelist_h);

	mIniWrite_putHex(fp, "brushopt_flags", cf->panel.brushopt_flags);

	mIniWrite_putNumbers(fp, "water_preset", cf->panel.water_preset, CONFIG_WATER_PRESET_NUM, 4, TRUE);

	//---- 変形ダイアログ

	mIniWrite_putGroup(fp, "transform");

	mIniWrite_putInt(fp, "view_w", cf->transform.view_w);
	mIniWrite_putInt(fp, "view_h", cf->transform.view_h);
	mIniWrite_putInt(fp, "flags", cf->transform.flags);

	//----- キャンバスビュー

	mIniWrite_putGroup(fp, "canvasview");

	mIniWrite_putInt(fp, "flags", cf->canvview.flags);
	mIniWrite_putInt(fp, "zoom", cf->canvview.zoom);
	mIniWrite_putNumbers(fp, "btt", cf->canvview.bttcmd, 5, 1, FALSE);

	//----- イメージビューア

	mIniWrite_putGroup(fp, "imgviewer");

	mIniWrite_putStr(fp, "dir", &cf->strImageViewerDir);
	mIniWrite_putInt(fp, "flags", cf->imgviewer.flags);
	mIniWrite_putNumbers(fp, "btt", cf->imgviewer.bttcmd, 5, 1, FALSE);

	//----- グリッド

	mIniWrite_putGroup(fp, "grid");

	mIniWrite_putInt(fp, "gridw", cf->grid.gridw);
	mIniWrite_putInt(fp, "gridh", cf->grid.gridh);
	mIniWrite_putHex(fp, "gridcol", cf->grid.col_grid);
	
	mIniWrite_putInt(fp, "sph", cf->grid.splith);
	mIniWrite_putInt(fp, "spv", cf->grid.splitv);
	mIniWrite_putHex(fp, "spcol", cf->grid.col_split);

	mIniWrite_putHex(fp, "pxgrid", cf->grid.pxgrid_zoom);

	mIniWrite_putNumbers(fp, "recent", cf->grid.recent, CONFIG_GRID_RECENT_NUM, 4, TRUE);

	//----- 保存設定

	mIniWrite_putGroup(fp, "save");

	mIniWrite_putInt(fp, "png", cf->save.png);
	mIniWrite_putInt(fp, "jpeg", cf->save.jpeg);
	mIniWrite_putInt(fp, "tiff", cf->save.tiff);
	mIniWrite_putInt(fp, "webp", cf->save.webp);
	mIniWrite_putInt(fp, "psd", cf->save.psd);

	//----- ポインタデバイス

	mIniWrite_putGroup(fp, "pointer");

	mIniWrite_putNumbers(fp, "default", cf->pointer_btt_default, CONFIG_POINTERBTT_NUM, 1, FALSE);
	mIniWrite_putNumbers(fp, "pentab", cf->pointer_btt_pentab, CONFIG_POINTERBTT_NUM, 1, FALSE);

	//----- 文字列

	mIniWrite_putGroup(fp, "string");

	mIniWrite_putStr(fp, "font_panel", &cf->strFont_panel);
	mIniWrite_putStr(fp, "tempdir", &cf->strTempDir);
	mIniWrite_putStr(fp, "filedlgdir", &cf->strFileDlgDir);
	mIniWrite_putStr(fp, "layerfiledir", &cf->strLayerFileDir);
	mIniWrite_putStr(fp, "selectfiledir", &cf->strSelectFileDir);
	mIniWrite_putStr(fp, "fontfiledir", &cf->strFontFileDir);
	mIniWrite_putStr(fp, "user_texdir", &cf->strUserTextureDir);
	mIniWrite_putStr(fp, "user_brushdir", &cf->strUserBrushDir);
	mIniWrite_putStr(fp, "cursor_file", &cf->strCursorFile);

	//----- ファイル履歴

	mIniWrite_putGroup(fp, "recentfile");
	mIniWrite_putStrArray(fp, 0, cf->strRecentFile, CONFIG_RECENTFILE_NUM);

	//----- 開いたディレクトリ履歴

	mIniWrite_putGroup(fp, "recent_opendir");
	mIniWrite_putStrArray(fp, 0, cf->strRecentOpenDir, CONFIG_RECENTDIR_NUM);

	//----- 保存ディレクトリ履歴

	mIniWrite_putGroup(fp, "recent_savedir");
	mIniWrite_putStrArray(fp, 0, cf->strRecentSaveDir, CONFIG_RECENTDIR_NUM);

	//----- 新規作成サイズ

	mIniWrite_putGroup(fp, "newcanvas_recent");
	_save_newcanvas_array(fp, cf->newcanvas_recent);

	mIniWrite_putGroup(fp, "newcanvas_record");
	_save_newcanvas_array(fp, cf->newcanvas_record);

	//----- キャンバスキー

	mIniWrite_putGroup(fp, "canvaskey");

	for(i = 0; i < 256; i++)
	{
		if(cf->canvaskey[i])
			mIniWrite_putInt_keyno(fp, i, cf->canvaskey[i]);
	}

	//----- フィルタデータ

	FilterParam_setConfig(&cf->list_filterparam, fp);
}

/* 色マスク書き込み */

static void _save_colmask(FILE *fp,AppDraw *p)
{
	uint32_t i,col[DRAW_COLORMASK_NUM];

	for(i = 0; i < DRAW_COLORMASK_NUM; i++)
		col[i] = RGBA8_to_32bit(p->col.maskcol + i);

	mIniWrite_putNumbers(fp, "colmask", col, DRAW_COLORMASK_NUM, 4, TRUE);
}

/* AppDraw 書き込み */

static void _save_drawdata(FILE *fp,AppDraw *p)
{
	//------ main

	mIniWrite_putGroup(fp, "draw_main");

	mIniWrite_putStr(fp, "opt_texture", &p->strOptTexturePath);

	//------ カラー

	mIniWrite_putGroup(fp, "draw_color");

	mIniWrite_putHex(fp, "drawcol", RGBcombo_to_32bit(&p->col.drawcol));
	mIniWrite_putHex(fp, "bkgndcol", RGBcombo_to_32bit(&p->col.bkgndcol));
	mIniWrite_putHex(fp, "checkbkcol1", RGBcombo_to_32bit(&p->col.checkbkcol[0]));
	mIniWrite_putHex(fp, "checkbkcol2", RGBcombo_to_32bit(&p->col.checkbkcol[1]));

	_save_colmask(fp, p);

	mIniWrite_putNumbers(fp, "gradbarcol", p->col.gradcol, DRAW_GRADBAR_NUM * 2, 4, TRUE);
	mIniWrite_putNumbers(fp, "layercolpal", p->col.layercolpal, DRAW_LAYERCOLPAL_NUM, 4, TRUE);

	mIniWrite_putInt(fp, "pal_sel", mListItemGetIndex(p->col.cur_pallist));
	
	//------- ツール

	mIniWrite_putGroup(fp, "draw_tool");

	mIniWrite_putInt(fp, "tool", p->tool.no);
	mIniWrite_putNumbers(fp, "toolsub", p->tool.subno, TOOL_NUM, 1, FALSE);

	mIniWrite_putHex(fp, "opt_dotpen", p->tool.opt_dotpen);
	mIniWrite_putHex(fp, "opt_dotpen_erase", p->tool.opt_dotpen_erase);
	mIniWrite_putHex(fp, "opt_finger", p->tool.opt_finger);
	mIniWrite_putHex(fp, "opt_fillpoly", p->tool.opt_fillpoly);
	mIniWrite_putHex(fp, "opt_fillpoly_erase", p->tool.opt_fillpoly_erase);
	mIniWrite_putHex(fp, "opt_fill", p->tool.opt_fill);
	mIniWrite_putHex(fp, "opt_grad", p->tool.opt_grad);
	mIniWrite_putInt(fp, "opt_select", p->tool.opt_select);
	mIniWrite_putInt(fp, "opt_selfill", p->tool.opt_selfill);
	mIniWrite_putInt(fp, "opt_stamp", p->tool.opt_stamp);
	mIniWrite_putInt(fp, "opt_cutpaste", p->tool.opt_cutpaste);
	mIniWrite_putInt(fp, "opt_boxedit", p->tool.opt_boxedit);

	//------ ブラシ

	mIniWrite_putGroup(fp, "draw_brush");

	mIniWrite_putNumbers(fp, "press_comm", p->tlist->pt_press_comm, CURVE_SPLINE_MAXNUM, 4, TRUE);

	//-------- 入り抜き

	mIniWrite_putGroup(fp, "draw_headtail");

	mIniWrite_putInt(fp, "sel", p->headtail.selno);
	mIniWrite_putHex(fp, "line", p->headtail.curval[0]);
	mIniWrite_putHex(fp, "bezier", p->headtail.curval[1]);

	mIniWrite_putNumbers(fp, "regist", p->headtail.regist, DRAW_HEADTAIL_REGIST_NUM, 4, TRUE);

	//------- テキスト描画

	mIniWrite_putGroup(fp, "draw_text");

	mIniWrite_putInt(fp, "preview", p->text.fpreview);

	mIniWrite_putStr(fp, "font", &p->text.dt.str_font);
	mIniWrite_putStr(fp, "style", &p->text.dt.str_style);
	
	mIniWrite_putInt(fp, "font_param", p->text.dt.font_param);
	mIniWrite_putInt(fp, "fontsize", p->text.dt.fontsize);
	mIniWrite_putInt(fp, "rubysize", p->text.dt.rubysize);
	mIniWrite_putInt(fp, "dpi", p->text.dt.dpi);
	mIniWrite_putInt(fp, "char_space", p->text.dt.char_space);
	mIniWrite_putInt(fp, "line_space", p->text.dt.line_space);
	mIniWrite_putInt(fp, "ruby_pos", p->text.dt.ruby_pos);
	mIniWrite_putInt(fp, "angle", p->text.dt.angle);
	
	mIniWrite_putInt(fp, "fontsel", p->text.dt.fontsel);
	mIniWrite_putInt(fp, "unit_fontsize", p->text.dt.unit_fontsize);
	mIniWrite_putInt(fp, "unit_rubysize", p->text.dt.unit_rubysize);
	mIniWrite_putInt(fp, "unit_charsp", p->text.dt.unit_char_space);
	mIniWrite_putInt(fp, "unit_linesp", p->text.dt.unit_line_space);
	mIniWrite_putInt(fp, "unit_rubypos", p->text.dt.unit_ruby_pos);
	mIniWrite_putInt(fp, "hinting", p->text.dt.hinting);

	mIniWrite_putHex(fp, "flags", p->text.dt.flags);

	//----- 定規

	_save_draw_rule_record(fp);
}

/** 設定ファイル書き込み */

void app_save_config(void)
{
	FILE *fp;

	fp = mIniWrite_openFile_join(mGuiGetPath_config_text(), CONFIG_FILENAME_MAIN);
	if(!fp) return;

	//バージョン

	mIniWrite_putGroup(fp, "azpainter");
	mIniWrite_putInt(fp, "ver", 3);

	//ウィジェット状態

	_save_widgets_state(fp, APPCONF);

	//AppConfig

	_save_configdata(fp, APPCONF);

	//AppDraw

	_save_drawdata(fp, APPDRAW);

	//mlk

	mGuiWriteIni_system(fp);

	fclose(fp);
}
