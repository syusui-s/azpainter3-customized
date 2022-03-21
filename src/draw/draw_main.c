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
 * AppDraw : 初期化など
 *****************************************/

#include "mlk_gui.h"
#include "mlk_window.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_buf.h"

#include "def_macro.h"
#include "def_draw.h"
#include "def_draw_toollist.h"
#include "def_config.h"
#include "def_widget.h"
#include "def_brushdraw.h"

#include "palettelist.h"
#include "imagecanvas.h"
#include "tileimage.h"
#include "imagematerial.h"
#include "image32.h"
#include "layerlist.h"
#include "materiallist.h"
#include "gradation_list.h"
#include "toollist.h"
#include "dotshape.h"
#include "pointbuf.h"
#include "brushsize_list.h"
#include "font.h"
#include "appcursor.h"
#include "fileformat.h"

#include "panel_func.h"
#include "maincanvas.h"
#include "statusbar.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_rule.h"


//=============================
// DrawTextData
//=============================


/** DrawTextData 解放 */

void DrawTextData_free(DrawTextData *p)
{
	mStrFree(&p->str_text);
	mStrFree(&p->str_font);
	mStrFree(&p->str_style);
}

/** コピー */

void DrawTextData_copy(DrawTextData *dst,const DrawTextData *src)
{
	DrawTextData_free(dst);

	*dst = *src;

	mStrCopy_init(&dst->str_text, &src->str_text);
	mStrCopy_init(&dst->str_font, &src->str_font);
	mStrCopy_init(&dst->str_style, &src->str_style);
}


//=============================
// 初期化
//=============================


/* AppDrawToolList 解放 */

static void _free_toollist(AppDrawToolList *p)
{
	if(p)
	{
		mListDeleteAll(&p->list_group);

		BrushEditData_free(p->brush);

		ToolListItem_free(p->copyitem);

		mBufFree(&p->buf_sizelist);

		mFree(p->dp_cur);
		mFree(p->dp_reg);
	}

	mFree(p);
}

/* AppDrawToolList 確保 */

static AppDrawToolList *_alloc_toollist(void)
{
	AppDrawToolList *p;

	p = (AppDrawToolList *)mMalloc0(sizeof(AppDrawToolList));
	if(!p) return NULL;

	p->brush = (BrushEditData *)mMalloc0(sizeof(BrushEditData));

	p->dp_cur = (BrushDrawParam *)mMalloc0(sizeof(BrushDrawParam));
	p->dp_reg = (BrushDrawParam *)mMalloc0(sizeof(BrushDrawParam));

	//筆圧カーブ:共通設定

	p->pt_press_comm[1] = CURVE_SPLINE_VAL_MAXPOS;

	//筆圧カーブ:コピー

	p->pt_press_copy[1] = CURVE_SPLINE_VAL_MAXPOS;

	//リスト

	ToolList_init(&p->list_group);

	return p;
}


/** 解放 */

void AppDraw_free(void)
{
	AppDraw *p = APPDRAW;

	if(!p) return;

	TileImage_finish();

	DotShape_free();

	DrawFontFinish();

	//mList

	mListDeleteAll(&p->col.list_pal);
	mListDeleteAll(&p->list_material[0]);
	mListDeleteAll(&p->list_material[1]);
	mListDeleteAll(&p->list_grad_custom);

	//

	_free_toollist(p->tlist);

	DrawTextData_free(&p->text.dt);
	DrawTextData_free(&p->text.dt_copy);

	mStrFree(&p->strOptTexturePath);

	ImageCanvas_free(p->imgcanvas);

	ImageMaterial_free(p->imgmat_opttex);

	TileImage_free(p->sel.tileimg_copy);

	TileImage_free(p->tileimg_sel);
	TileImage_free(p->tileimg_tmp_save);
	TileImage_free(p->tileimg_tmp_brush);
	TileImage_free(p->tileimg_tmp);

	TileImage_free(p->boxsel.img);
	TileImage_free(p->stamp.img);

	LayerList_free(p->layerlist);

	mFree(p->pointbuf);

	mFree(p);
}

/** AppDraw 作成
 *
 * [!] 設定ファイルの読み込み前に行う
 * return: 0 で成功 */

int AppDraw_new(void)
{
	AppDraw *p;

	p = (AppDraw *)mMalloc0(sizeof(AppDraw));
	if(!p) return 1;

	APPDRAW = p;

	//値初期化

	p->canvas_zoom = 1000;
	p->canvas_size.w = p->canvas_size.h = 100;

	drawCalc_setCanvasViewParam(p);

	p->rule.ellipse_yx = 1;

	//初期化

	if(!TileImage_init()) return 1;

	if(DotShape_init()) return 1;

	DrawFontInit();

	PaletteList_init(&p->col.list_pal);
	MaterialList_init(p->list_material);
	GradationList_init(&p->list_grad_custom);

	//レイヤリスト

	p->layerlist = LayerList_new();

	//PointBuf

	p->pointbuf = PointBuf_new();

	//AppDrawToolList

	p->tlist = _alloc_toollist();

	return 0;
}

/** 設定ファイル (main.conf) の読み込み前に、各設定ファイルを読み込む */

void drawInit_loadConfig_before(void)
{
	AppDraw *p = APPDRAW;

	//カラーパレット (選択リストは main.conf から読み込む)

	PaletteList_loadConfig(&p->col.list_pal);

	//グラデーション

	GradationList_loadConfig(&p->list_grad_custom);

	//

	BrushSizeList_loadConfigFile(&p->tlist->buf_sizelist);

	ToolList_loadConfigFile();
}

/** 設定ファイル読み込み後、ウィジェット作成前の初期化 */

void drawInit_createWidget_before(void)
{
	//オプションテクスチャ画像読み込み

	drawTexture_loadOptionTextureImage(APPDRAW);

	//トーンをグレイスケール表示

	TileImage_global_setToneToGray(APPDRAW->ftonelayer_to_gray);
}

/** ウィンドウ表示前の初期化 (ウィンドウ作成後) */

void drawInit_beforeShow(void)
{
	AppDraw *p = APPDRAW;

	//新規イメージ

	drawImage_newCanvas(p, NULL);

	//合成

	drawUpdate_blendImage_full(p, NULL);

	//キャンバス状態

	drawCanvas_scroll_default(p);
	drawCanvas_update(p, 0, 0, DRAWCANVAS_UPDATE_DEFAULT);
}

/** 終了時、設定ファイル保存 */

void drawEnd_saveConfig(void)
{
	AppDraw *p = APPDRAW;

	//カラーパレット (変更時のみ)

	if(p->col.pal_fmodify)
		PaletteList_saveConfig(&p->col.list_pal);

	//グラデーション

	if(p->fmodify_grad_list)
		GradationList_saveConfig(&p->list_grad_custom);

	//ブラシサイズリスト

	if(p->fmodify_brushsize)
		BrushSizeList_saveConfigFile(&p->tlist->buf_sizelist);

	//ツールリスト

	ToolList_saveConfigFile();
}


//=============================
// カラー関連
//=============================


/** 描画色を RGB 32bit で取得 */

uint32_t drawColor_getDrawColor(void)
{
	return RGBcombo_to_32bit(&APPDRAW->col.drawcol);
}

/** 描画色セット
 *
 * return: 現在の色と異なる色がセットされたら TRUE */

mlkbool drawColor_setDrawColor(uint32_t col)
{
	mlkbool ret = !RGB8_compare_32bit(&APPDRAW->col.drawcol.c8, col);
	
	RGB32bit_to_RGBcombo(&APPDRAW->col.drawcol, col);

	return ret;
}

/** 描画色をセットして、パネルを更新 */

void drawColor_setDrawColor_update(uint32_t col)
{
	RGB32bit_to_RGBcombo(&APPDRAW->col.drawcol, col);

	//カラーホイールも同時更新
	PanelColor_changeDrawColor();
}

/** 背景色セット */

void drawColor_setBkgndColor(uint32_t col)
{
	RGB32bit_to_RGBcombo(&APPDRAW->col.bkgndcol, col);
}

/** 描画色/背景色を入れ替え*/

void drawColor_toggleDrawCol(AppDraw *p)
{
	RGBcombo tmp;

	tmp = p->col.drawcol;
	p->col.drawcol = p->col.bkgndcol;
	p->col.bkgndcol = tmp;
}

/** 描画色 8bit のRGB値が変わった時 */

void drawColor_changeDrawColor8(void)
{
	RGBcombo_set16_from8(&APPDRAW->col.drawcol);
}

/** 色マスクのタイプを変更
 *
 * type: 0-2 = off/mask/rev, -1 = マスク反転, -2 = 逆マスク反転 */

void drawColorMask_setType(AppDraw *p,int type)
{
	if(type >= 0)
		//指定タイプ
		p->col.colmask_type = type;
	else
	{
		//反転
		
		type = -type;
	
		if(p->col.colmask_type == type)
			p->col.colmask_type = 0;
		else
			p->col.colmask_type = type;
	}
}


//=============================
// ツール
//=============================


/** ツール変更 */

void drawTool_setTool(AppDraw *p,int no)
{
	int frule;

	if(p->tool.no == no) return;

	//矩形選択から変更する時

	if(p->tool.no == TOOL_CUTPASTE || p->tool.no == TOOL_BOXEDIT)
		drawBoxSel_onChangeState(p, 2);

	frule = drawRule_isVisibleGuide(p);

	//

	p->tool.no = no;

	//各ウィジェット

	PanelTool_changeTool();
	PanelOption_changeTool();

	StatusBar_setHelp_tool();

	//カーソル

	MainCanvasPage_setCursor_forTool();

	//定規ガイドの表示状態が変わる時

	if(frule != drawRule_isVisibleGuide(p))
		drawUpdate_canvas();
}

/** ツールサブタイプ変更 */

void drawTool_setTool_subtype(AppDraw *p,int subno)
{
	int frule;

	if(subno == p->tool.subno[p->tool.no])
		return;

	frule = drawRule_isVisibleGuide(p);

	//
	
	p->tool.subno[p->tool.no] = subno;

	PanelTool_changeToolSub();

	StatusBar_setHelp_tool();

	//定規ガイドの表示状態が変わる時

	if(frule != drawRule_isVisibleGuide(p))
		drawUpdate_canvas();
}

/** 指定ツールが、描画などを行わないタイプか */

mlkbool drawTool_isType_notDraw(int no)
{
	return (no == TOOL_CANVAS_MOVE || no == TOOL_CANVAS_ROTATE);
}

/** 指定ツールが、描画タイプ(フリーハンドなど)を持つタイプか */

mlkbool drawTool_isType_haveDrawType(int no)
{
	return (no == TOOL_TOOLLIST || no == TOOL_DOTPEN
		|| no == TOOL_DOTPEN_ERASE || no == TOOL_FINGER);
}


//==========================
// カーソル
//==========================


/** カーソルを砂時計にセット */

void drawCursor_wait(void)
{
	mWindowSetCursor(MLK_WINDOW(APPWIDGET->mainwin), AppCursor_getWaitCursor());
}

/** カーソルを(砂時計から)元に戻す */

void drawCursor_restore(void)
{
	mWindowResetCursor(MLK_WINDOW(APPWIDGET->mainwin));
}

/** ツール番号から、カーソル番号を取得 */

int drawCursor_getToolCursor(int toolno)
{
	int no;

	switch(toolno)
	{
		case TOOL_MOVE:
			no = APPCURSOR_MOVE;
			break;
		case TOOL_SELECT:
		case TOOL_CUTPASTE:
		case TOOL_BOXEDIT:
			no = APPCURSOR_SELECT;
			break;
		case TOOL_STAMP:
			if(APPDRAW->stamp.img)
				no = APPCURSOR_STAMP;
			else
				no = APPCURSOR_SELECT;
			break;
		case TOOL_CANVAS_MOVE:
			no = APPCURSOR_HAND;
			break;
		case TOOL_CANVAS_ROTATE:
			no = APPCURSOR_ROTATE;
			break;
		case TOOL_SPOIT:
			no = APPCURSOR_SPOIT;
			break;
		
		default:
			no = APPCURSOR_DRAW;
			break;
	}

	return no;
}


//=============================
// テクスチャなど
//=============================


/** オプションのテクスチャイメージの読み込み
 *
 * strOptTexturePath に画像パスをセットしておく。
 *
 * return: イメージが読み込まれたか */

mlkbool drawTexture_loadOptionTextureImage(AppDraw *p)
{
	mStr str = MSTR_INIT,*strpath;

	//イメージ解放
	
	ImageMaterial_free(p->imgmat_opttex);
	p->imgmat_opttex = NULL;

	//パスが空なら、イメージ無し

	strpath = &p->strOptTexturePath;

	if(mStrIsEmpty(strpath))
		return FALSE;

	//絶対パス取得
	// :先頭が '/' でシステムディレクトリ。
	// :でなければ、ユーザーディレクトリ。

	if(strpath->buf[0] == '/')
	{
		mGuiGetPath_data(&str, APP_DIRNAME_TEXTURE);
		mStrPathJoin(&str, strpath->buf + 1);
	}
	else
	{
		mStrCopy(&str, &APPCONF->strUserTextureDir);
		mStrPathJoin(&str, strpath->buf);
	}

	//読み込み

	p->imgmat_opttex = ImageMaterial_loadTexture(&str);

	mStrFree(&str);

	return (p->imgmat_opttex != NULL);
}

/** 画像ファイルから TileImage を読み込み
 * (切り貼り/スタンプツール時)
 *
 * ppdst: 読み込み前に解放される
 * psize: 画像のサイズが入る */

mlkerr drawSub_loadTileImage(TileImage **ppdst,const char *filename,mSize *psize)
{
	uint32_t format;
	mSize size;
	mlkerr ret;

	//ファイルフォーマット

	format = FileFormat_getFromFile(filename);

	if(!(format & FILEFORMAT_NORMAL_IMAGE))
		return MLKERR_UNSUPPORTED;

	//読み込み

	TileImage_free(*ppdst);

	ret = TileImage_loadFile(ppdst, filename, format, &size, NULL);

	if(!ret)
	{
		psize->w = size.w;
		psize->h = size.h;
	}

	return ret;
}


//=============================
// スタンプ
//=============================


/** スタンプイメージをクリア */

void drawStamp_clearImage(AppDraw *p)
{
	if(p->stamp.img)
	{
		TileImage_free(p->stamp.img);
		p->stamp.img = NULL;

		//オプションのプレビュー更新

		PanelOption_changeStampImage();

		//カーソル変更

		MainCanvasPage_setCursor_forTool();
	}
}

/** 画像ファイルからスタンプイメージ読み込み */

mlkerr drawStamp_loadImage(AppDraw *p,const char *filename)
{
	mSize size;
	mlkerr ret;

	ret = drawSub_loadTileImage(&p->stamp.img, filename, &size);
	if(ret) return ret;

	p->stamp.size.w = size.w;
	p->stamp.size.h = size.h;
	p->stamp.bits = p->imgbits;

	//カーソル変更
	
	MainCanvasPage_setCursor_forTool();

	return MLKERR_OK;
}

