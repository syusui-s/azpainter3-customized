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
 * AppDraw
 * 操作 - サブ関数
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mlk_gui.h"
#include "mlk_rectbox.h"
#include "mlk_widget.h"
#include "mlk_sysdlg.h"
#include "mlk_pixbuf.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_tool_option.h"
#include "def_pixelmode.h"
#include "def_brushdraw.h"

#include "layeritem.h"
#include "layerlist.h"
#include "gradation_list.h"
#include "toollist.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "maincanvas.h"

#include "dotshape.h"
#include "fillpolygon.h"
#include "pointbuf.h"
#include "undo.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_rule.h"
#include "draw_toollist.h"

#include "trid.h"


//---------------------------
/* 塗りタイプ */

//ブラシ用
static const uint8_t g_pixcoltype_brush[] = {
	TILEIMAGE_PIXELCOL_NORMAL, TILEIMAGE_PIXELCOL_NORMAL, TILEIMAGE_PIXELCOL_COMPARE_A,
	TILEIMAGE_PIXELCOL_OVERWRITE, TILEIMAGE_PIXELCOL_OVERWRITE,
	TILEIMAGE_PIXELCOL_DODGE, TILEIMAGE_PIXELCOL_BURN, TILEIMAGE_PIXELCOL_ADD
};

//ツール用
static const uint8_t g_pixcoltype_tool[] = {
	TILEIMAGE_PIXELCOL_NORMAL, TILEIMAGE_PIXELCOL_COMPARE_A,
	TILEIMAGE_PIXELCOL_OVERWRITE, TILEIMAGE_PIXELCOL_ERASE
};

//---------------------------



/** 操作開始時の情報セット */

void drawOpSub_setOpInfo(AppDraw *p,int optype,
	void (*motion)(AppDraw *,uint32_t),mlkbool (*release)(AppDraw *),int sub)
{
	p->w.optype = optype;
	p->w.optype_sub = sub;

	p->w.func_motion = motion;
	p->w.func_release = release;
	p->w.func_press_in_grab = NULL;
	p->w.func_action = NULL;
}

/** 現在のレイヤに描画可能かどうか
 *
 * return: CANDRAWLAYER_* */

int drawOpSub_canDrawLayer(AppDraw *p,uint32_t flags)
{
	mlkbool is_read;

	is_read = (flags & CANDRAWLAYER_F_ENABLE_READ);

	if(LAYERITEM_IS_FOLDER(p->curlayer))
	{
		//フォルダ
		return CANDRAWLAYER_FOLDER;
	}
	else if(!(flags & CANDRAWLAYER_F_ENABLE_TEXT) && LAYERITEM_IS_TEXT(p->curlayer))
	{
		//テキストレイヤ
		return CANDRAWLAYER_TEXT;
	}
	else if(!is_read && LayerItem_isLock_real(p->curlayer))
	{
		//ロック (上位フォルダがロックありの場合もON)
		return CANDRAWLAYER_LOCK;
	}
	else if(!(flags & CANDRAWLAYER_F_NO_HIDE)
		&& !is_read && !LayerItem_isVisible_real(p->curlayer))
	{
		//非表示
		return CANDRAWLAYER_HIDE;
	}
	else if(!(flags & CANDRAWLAYER_F_NO_PASTE)
		&& p->boxsel.is_paste_mode)
	{
		//貼り付けモード中
		return CANDRAWLAYER_PASTE;
	}
	else
		return CANDRAWLAYER_OK;
}

/** 現在のレイヤに描画可能かどうか (メッセージ表示付き) */

int drawOpSub_canDrawLayer_mes(AppDraw *p,uint32_t flags)
{
	int ret;

	ret = drawOpSub_canDrawLayer(p, flags);

	if(ret)
	{
		mMessageBoxOK(MLK_WINDOW(APPWIDGET->mainwin),
			MLK_TR2(TRGROUP_MESSAGE_DRAW, ret));
	}

	return ret;
}

/** カレントレイヤがフォルダか */

mlkbool drawOpSub_isCurLayer_folder(void)
{
	return LAYERITEM_IS_FOLDER(APPDRAW->curlayer);
}

/** テキストレイヤか */

mlkbool drawOpSub_isCurLayer_text(void)
{
	return LAYERITEM_IS_TEXT(APPDRAW->curlayer);
}

/** テキストツールの処理を実行するか */

mlkbool drawOpSub_isRunTextTool(void)
{
	return (LAYERITEM_IS_TEXT(APPDRAW->curlayer)
		&& (APPDRAW->tool.no == TOOL_TEXT || APPDRAW->tool.no == TOOL_TOOLLIST));
}


//========================


/** キャンバスの XOR 用に tileimg_tmp (A1) を作成 */

mlkbool drawOpSub_createTmpImage_canvas(AppDraw *p)
{
	p->tileimg_tmp = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1BIT, p->canvas_size.w, p->canvas_size.h);

	return (p->tileimg_tmp != NULL);
}

/** tileimg_tmp 作成 (カレントと同じタイプ & イメージサイズ分) */

mlkbool drawOpSub_createTmpImage_curlayer_imgsize(AppDraw *p)
{
	p->tileimg_tmp = TileImage_new(p->curlayer->type, p->imgw, p->imgh);
	if(!p->tileimg_tmp) return FALSE;

	//A1/A16 の場合、カレントと同じ色に

	p->tileimg_tmp->col = p->curlayer->img->col;

	return TRUE;
}

/** tileimg_tmp 作成 (カレントと同じタイプ & 指定範囲) */

mlkbool drawOpSub_createTmpImage_curlayer_rect(AppDraw *p,const mRect *rc)
{
	p->tileimg_tmp = TileImage_newFromRect(p->curlayer->type, rc);
	if(!p->tileimg_tmp) return FALSE;

	p->tileimg_tmp->col = p->curlayer->img->col;

	return TRUE;
}

/** tileimg_tmp を削除 */

void drawOpSub_freeTmpImage(AppDraw *p)
{
	TileImage_free(p->tileimg_tmp);
	p->tileimg_tmp = NULL;
}

/** FillPolygon を解放 */

void drawOpSub_freeFillPolygon(AppDraw *p)
{
	FillPolygon_free(p->w.fillpolygon);
	p->w.fillpolygon = NULL;
}


//=============================
// イメージ
//=============================


/** (スタンプ用) 範囲決定後、各図形の範囲イメージ (A1) を作成する
 *
 * キャンバスイメージの範囲内のみとなる。
 * (キャンバス上で見えない部分は範囲選択されない)
 *
 * rcdst: 結果の範囲が入る
 * return: キャンバス範囲外や失敗時は NULL */

TileImage *drawOpSub_createSelectImage_forStamp(AppDraw *p,mRect *rcdst)
{
	TileImage *img = NULL;
	mBox box;
	mRect rc;
	uint64_t col = (uint64_t)-1;

	switch(p->w.optype)
	{
		//矩形 (イメージに対する)
		case DRAW_OPTYPE_XOR_RECT_IMAGE:
			//キャンバス範囲内にクリッピング
			
			if(!drawCalc_image_rect_to_box(p, &box, &p->w.rctmp[0]))
				return NULL;

			//作成

			img = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1BIT, box.w, box.h);
			if(!img) return NULL;

			TileImage_setOffset(img, box.x, box.y);

			//矩形内を塗りつぶし

			g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new;

			TileImage_drawFillBox(img, box.x, box.y, box.w, box.h, &col);

			mRectSetBox(rcdst, &box);
			break;

		//多角形、フリーハンド
		case DRAW_OPTYPE_XOR_POLYGON:
		case DRAW_OPTYPE_XOR_LASSO:
			//描画範囲 (mBox)
			// :キャンバス範囲内

			FillPolygon_getDrawRect(p->w.fillpolygon, &rc);
			
			if(!drawCalc_image_rect_to_box(p, &box, &rc))
				return NULL;
		
			//作成
			
			img = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1BIT, box.w, box.h);
			if(!img) return NULL;

			TileImage_setOffset(img, box.x, box.y);

			//多角形塗りつぶし

			g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new_drawrect;

			TileImageDrawInfo_clearDrawRect();
		
			TileImage_drawFillPolygon(img, p->w.fillpolygon, &col, FALSE);

			drawOpSub_freeFillPolygon(p);

			//実際に描画された範囲

			*rcdst = g_tileimage_dinfo.rcdraw;

			if(!drawCalc_clipImageRect(p, rcdst))
			{
				TileImage_free(img);
				img = NULL;
			}
			break;
	}

	return img;
}


//=============================
// 描画開始/終了
//=============================


/** 描画開始
 *
 * [!] この前に drawOpSub_setDrawInfo() を必ず実行すること。 */

void drawOpSub_beginDraw(AppDraw *p)
{
	TileImageDrawInfo *info = &g_tileimage_dinfo;

	//描画情報の初期化

	info->err = 0;

	TileImageDrawInfo_clearDrawRect();

	//描画先イメージの情報取得

	TileImage_getInfo(p->curlayer->img, &info->tileimginfo);

	//保存用イメージ作成
	// :色が変更されて描画されると、そのタイル位置の元イメージがコピーされる。
	// :同じイメージに連続して描画する場合が多いことを考え、イメージのタイル配列は使いまわす。

	p->tileimg_tmp_save = TileImage_createSame(p->tileimg_tmp_save, p->curlayer->img, -1);
	
	info->img_save = p->tileimg_tmp_save;
	
	if(!info->img_save) info->err = MLKERR_ALLOC;

	//ブラシ描画のストローク重ね塗り用の濃度イメージ

	if(!(p->w.drawinfo_flags & DRAW_DRAWINFO_F_BRUSH_STROKE))
		info->img_brush_stroke = NULL;
	else
	{
		p->tileimg_tmp_brush = TileImage_createSame(p->tileimg_tmp_brush, p->curlayer->img, TILEIMAGE_COLTYPE_ALPHA);
		
		info->img_brush_stroke = p->tileimg_tmp_brush;

		if(!info->img_brush_stroke) info->err = MLKERR_ALLOC;
	}
}

/** 描画終了
 *
 * rc: 描画されたイメージ範囲 (アンドゥ用) */

void drawOpSub_endDraw(AppDraw *p,mRect *rcdraw)
{
#if 0
	mDebug("(%d,%d)-(%d,%d)\n", rcdraw->x1, rcdraw->y1, rcdraw->x2, rcdraw->y2);
#endif

	if(g_tileimage_dinfo.err)
	{
		//エラー時はアンドゥクリア

		Undo_deleteAll();
	}
	else
	{
		//----- エラーなし
		
		//空タイルを解放

		TileImage_freeEmptyTiles_byUndo(p->curlayer->img, p->tileimg_tmp_save);

		//undo

		Undo_addTilesImage(&g_tileimage_dinfo.tileimginfo, rcdraw);
	}

	//タイル削除

	TileImage_freeAllTiles(p->tileimg_tmp_save);
	TileImage_freeAllTiles(p->tileimg_tmp_brush);
}

/** 単体描画開始 (描画中は待ちカーソル表示) */

void drawOpSub_beginDraw_single(AppDraw *p)
{
	drawCursor_wait();
	
	drawOpSub_beginDraw(p);
}

/** 単体描画終了 */

void drawOpSub_endDraw_single(AppDraw *p)
{
	drawOpSub_finishDraw_single(p);

	drawCursor_restore();
}


//=============================
// 更新/終了の共通処理
//=============================


/** TileImageDrawInfo::rcdraw の範囲を、全体の範囲 (AppDraw::w.rcdraw) に追加し、キャンバス更新
 *
 * [自由線/連続直線/集中線] */

void drawOpSub_addrect_and_update(AppDraw *p,int type)
{
	mBox box;

	if(!mRectIsEmpty(&g_tileimage_dinfo.rcdraw))
	{
		//全体の範囲追加

		mRectUnion(&p->w.rcdraw, &g_tileimage_dinfo.rcdraw);

		//更新

		if(drawCalc_image_rect_to_box(p, &box, &g_tileimage_dinfo.rcdraw))
		{
			switch(type)
			{
				//draw ハンドラで更新する範囲を追加
				case DRAWOPSUB_UPDATE_ADD:
					drawUpdateBox_canvas(p, &box);
					break;
				//キャンバスに直接描画
				// :連続直線/集中線の場合は、この後に XOR 描画をする必要があるため、
				// :即時直接キャンバスを更新する。
				case DRAWOPSUB_UPDATE_DIRECT:
					drawUpdateBox_canvas_direct(p, &box);
					break;
				//タイマー
				case DRAWOPSUB_UPDATE_TIMER:
					MainCanvasPage_setTimer_updateRect(&box);
					break;
			}

		}
	}
}

/** AppDraw::w.rcdraw の範囲で描画終了処理を行う
 *
 * 複数描画時用。イメージとキャンバスは更新済みである。 */

void drawOpSub_finishDraw_workrect(AppDraw *p)
{
	mBox box;

	if(!mRectIsEmpty(&p->w.rcdraw))
	{
		//イメージ範囲

		if(!drawCalc_image_rect_to_box(p, &box, &p->w.rcdraw))
			box.x = -1;

		//描画終了

		drawOpSub_endDraw(p, &p->w.rcdraw);

		//タイマークリア & 更新

		MainCanvasPage_clearTimer_updateRect(TRUE);

		//キャンバスビュー更新

		drawUpdate_endDraw_box(p, &box);
	}
}

/** TileImageDrawInfo::rcdraw の範囲で、描画終了処理を行う
 *
 * 単体描画時用。イメージとキャンバスの更新も行う。 */

void drawOpSub_finishDraw_single(AppDraw *p)
{
	mBox box;

	if(!mRectIsEmpty(&g_tileimage_dinfo.rcdraw))
	{
		drawOpSub_endDraw(p, &g_tileimage_dinfo.rcdraw);

		//更新

		if(drawCalc_image_rect_to_box(p, &box, &g_tileimage_dinfo.rcdraw))
		{
			drawUpdateBox_canvas(p, &box);
			drawUpdate_endDraw_box(p, &box);
		}
	}
}

/** TileImageDrawInfo::rcdraw の範囲で、更新を行う */

void drawOpSub_update_rcdraw(AppDraw *p)
{
	drawUpdateRect_canvas_canvasview(p, &g_tileimage_dinfo.rcdraw);
}


//==============================
// 描画前の情報セット
//==============================


/* ブラシの描画情報セット */

static void _setdrawinfo_brush(AppDraw *p,TileImageDrawInfo *info,int regno)
{
	BrushParam *pv;
	int coltype;

	info->texture = drawToolList_getBrushDrawInfo(regno, &info->brushdp, &pv);

	//描画色

	bitcol_set_alpha_density100(&p->w.drawcol, pv->opacity, p->imgbits);

	info->drawcol = p->w.drawcol;

	//手ブレ補正
	// :定規ONで、線対称以外の場合は OFF

	if(p->rule.type && p->rule.drawpoint_num < 2)
		PointBuf_setSmoothing(p->pointbuf, SMOOTHING_TYPE_NONE, 0);
	else
		PointBuf_setSmoothing(p->pointbuf, pv->hosei_type, pv->hosei_val);

	//タイプごと

	coltype = TILEIMAGE_PIXELCOL_NORMAL;

	switch(pv->brush_type)
	{
		//通常ブラシ
		case BRUSH_TYPE_NORMAL:
			coltype = g_pixcoltype_brush[pv->pixelmode];
			break;
		//消しゴム
		case BRUSH_TYPE_ERASE:
			coltype = TILEIMAGE_PIXELCOL_ERASE;
			break;
		//ぼかし
		case BRUSH_TYPE_BLUR:
			info->func_setpixel = TileImage_setPixel_draw_blur;
			break;
	}

	info->func_pixelcol = TileImage_global_getPixelColorFunc(coltype);

	//ストローク重ね塗り

	if((pv->brush_type == BRUSH_TYPE_NORMAL || pv->brush_type == BRUSH_TYPE_ERASE)
		&& pv->pixelmode == PIXELMODE_BLEND_STROKE)
	{
		info->func_setpixel = TileImage_setPixel_draw_brush_stroke;
		
		p->w.drawinfo_flags |= DRAW_DRAWINFO_F_BRUSH_STROKE;
	}
}

/* ドットペンの情報セット */

static void _setdrawinfo_dotpen(AppDraw *p,TileImageDrawInfo *info,uint32_t val,mlkbool erase)
{
	int pixmode;

	//サイズ & 形状

	DotShape_create(TOOLOPT_DOTPEN_GET_SHAPE(val), TOOLOPT_DOTPEN_GET_SIZE(val));

	//濃度
	
	bitcol_set_alpha_density100(&p->w.drawcol, TOOLOPT_DOTPEN_GET_DENSITY(val), p->imgbits);

	//塗り

	pixmode = TOOLOPT_DOTPEN_GET_PIXMODE(val);

	info->func_pixelcol = TileImage_global_getPixelColorFunc(
		(erase)? TILEIMAGE_PIXELCOL_ERASE: g_pixcoltype_brush[pixmode]);

	//setpixel

	if(pixmode == PIXELMODE_BLEND_STROKE)
		//ストローク重ね塗り
		info->func_setpixel = TileImage_setPixel_draw_dotpen_stroke;
	else if(pixmode == PIXELMODE_OVERWRITE_SQUARE)
		//矩形上書き
		info->func_setpixel = TileImage_setPixel_draw_dotpen_overwrite_square;
	else
		info->func_setpixel = TileImage_setPixel_draw_dotpen_direct;

	//細線

	p->w.ntmp[0] = TOOLOPT_DOTPEN_IS_SLIM(val);
}

/* 指先の情報セット */

static void _setdrawinfo_finger(AppDraw *p,TileImageDrawInfo *info)
{
	uint32_t val;

	if(p->w.is_toollist_toolopt)
		val = p->w.toollist_toolopt;
	else
		val = p->tool.opt_finger;

	//サイズ & 形状

	DotShape_create(DOTSHAPE_TYPE_CIRCLE, TOOLOPT_DOTPEN_GET_SIZE(val));

	//強さ
	
	bitcol_set_alpha_density100(&p->w.drawcol, TOOLOPT_DOTPEN_GET_DENSITY(val), p->imgbits);

	//関数

	info->func_setpixel = TileImage_setPixel_draw_finger;
	info->func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_FINGER);

	//細線
	p->w.ntmp[0] = 0;
}

/* 図形塗りつぶし/図形消しゴムの情報セット */

static void _setdrawinfo_fillpoly(AppDraw *p,TileImageDrawInfo *info,uint32_t val,mlkbool erase)
{
	int n;

	//濃度

	bitcol_set_alpha_density100(&p->w.drawcol, TOOLOPT_FILLPOLY_GET_DENSITY(val), p->imgbits);

	//アンチエイリアス (ntmp[0])

	p->w.ntmp[0] = TOOLOPT_FILLPOLY_IS_ANTIALIAS(val);

	//塗り

	if(erase)
		n = TILEIMAGE_PIXELCOL_ERASE;
	else
		n = g_pixcoltype_tool[TOOLOPT_FILLPOLY_GET_PIXMODE(val)];

	info->func_pixelcol = TileImage_global_getPixelColorFunc(n);
}

/** 描画前の各情報セット
 *
 * toolno: ツール番号。負の値で、マスクなどのみセット */

void drawOpSub_setDrawInfo(AppDraw *p,int toolno,int param)
{
	TileImageDrawInfo *info = &g_tileimage_dinfo;
	uint32_t val;
	int n;

	//デフォルトで重ね塗り

	info->func_setpixel = TileImage_setPixel_draw_direct;
	info->func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_NORMAL);

	//オプションのテクスチャ

	info->texture = p->imgmat_opttex;

	//選択範囲

	info->img_sel = p->tileimg_sel;
	
	//マスクレイヤ

	if(LAYERITEM_IS_MASK_UNDER(p->curlayer))
	{
		//下レイヤでマスク
		
		info->img_mask = LayerList_getMaskUnderImage(p->layerlist, p->curlayer);
	}
	else if(p->masklayer && p->masklayer != p->curlayer)
	{
		//マスクレイヤ指定あり
		// :カレントがマスクに指定されている場合は、なし
		
		info->img_mask = p->masklayer->img;
	}
	else
		info->img_mask = NULL;

	//アルファマスク

	info->alphamask_type = p->curlayer->alphamask;

	//色マスク

	info->maskcol_type = p->col.colmask_type;
	info->maskcol_num = DRAW_COLORMASK_NUM;
	info->pmaskcol = p->col.maskcol;

	//描画先イメージ

	p->w.dstimg = p->curlayer->img;

	//描画色 (ビット数による)
	// :アルファ値は最大でセットされる。

	RGBcombo_to_bitcol(&p->w.drawcol, &p->col.drawcol, p->imgbits);

	//描画開始時に処理するもの

	p->w.drawinfo_flags = 0;

	//---------

	if(toolno < 0) return;

	switch(toolno)
	{
		//ブラシ
		case TOOL_TOOLLIST:
			_setdrawinfo_brush(p, info, p->w.brush_regno);
			break;

		//ドットペン/消しゴム
		// :param = 0 以外で、1px消しゴム
		case TOOL_DOTPEN:
		case TOOL_DOTPEN_ERASE:
			if(param)
				//1px消しゴム
				_setdrawinfo_dotpen(p, info, TOOLOPT_DOTPEN_1PX_NORMAL, TRUE);
			else
			{
				if(p->w.is_toollist_toolopt)
					val = p->w.toollist_toolopt;
				else if(toolno == TOOL_DOTPEN_ERASE)
					val = p->tool.opt_dotpen_erase;
				else
					val = p->tool.opt_dotpen;
			
				_setdrawinfo_dotpen(p, info, val, (toolno == TOOL_DOTPEN_ERASE));
			}
			break;

		//指先
		// :処理はドットペンと共通。
		case TOOL_FINGER:
			_setdrawinfo_finger(p, info);
			break;
	
		//図形塗りつぶし/消しゴム
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			if(p->w.is_toollist_toolopt)
				val = p->w.toollist_toolopt;
			else if(toolno == TOOL_FILL_POLYGON_ERASE)
				val = p->tool.opt_fillpoly_erase;
			else
				val = p->tool.opt_fillpoly;

			_setdrawinfo_fillpoly(p, info, val, (toolno == TOOL_FILL_POLYGON_ERASE));
			break;

		//塗りつぶし
		case TOOL_FILL:
			if(p->w.is_toollist_toolopt)
				val = p->w.toollist_toolopt;
			else
				val = p->tool.opt_fill;

			//濃度
			bitcol_set_alpha_density100(&p->w.drawcol, TOOLOPT_FILL_GET_DENSITY(val), p->imgbits);

			//塗りタイプ
			n = g_pixcoltype_tool[TOOLOPT_FILL_GET_PIXMODE(val)];
			info->func_pixelcol = TileImage_global_getPixelColorFunc(n);
			break;
	
		//不透明範囲消去
		// :テクスチャ無効
		case TOOL_FILL_ERASE:
			info->func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_ERASE);
			info->texture = NULL;
			break;

		//グラデーション
		case TOOL_GRADATION:
			if(p->w.is_toollist_toolopt)
				val = p->w.toollist_toolopt;
			else
				val = p->tool.opt_grad;
			
			//塗りタイプ
			n = g_pixcoltype_tool[TOOLOPT_GRAD_GET_PIXMODE(val)];
			info->func_pixelcol = TileImage_global_getPixelColorFunc(n);
			break;

		//切り貼り/スタンプ
		case TOOL_CUTPASTE:
		case TOOL_STAMP:
			info->texture = NULL;
			break;
	}
}

/** 色セット関数をセット
 *
 * type: TILEIMAGE_PIXELCOL_* */

void drawOpSub_setDrawInfo_pixelcol(int type)
{
	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(type);
}

/** マスク類 (選択範囲/レイヤマスク/テクスチャ/色マスク/Aマスク) をクリア */

void drawOpSub_setDrawInfo_clearMasks(void)
{
	TileImageDrawInfo *p = &g_tileimage_dinfo;

	p->img_sel = NULL;
	p->img_mask = NULL;
	p->texture = NULL;
	p->maskcol_type = 0;
	p->alphamask_type = 0;
}

/** 選択範囲の図形描画/自動選択用にセット */

void drawOpSub_setDrawInfo_select(void)
{
	TileImageDrawInfo *p = &g_tileimage_dinfo;

	drawOpSub_setDrawInfo_clearMasks();

	p->func_setpixel = TileImage_setPixel_work;
	p->func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_OVERWRITE);

	APPDRAW->w.drawcol = (uint64_t)-1;
}

/** 範囲イメージの移動/コピー時の貼り付け用 (マスク類無効) */

void drawOpSub_setDrawInfo_select_paste(void)
{
	drawOpSub_setDrawInfo(APPDRAW, -1, 0);
	drawOpSub_setDrawInfo_clearMasks();
}

/** 選択範囲: 切り取り用 */

void drawOpSub_setDrawInfo_select_cut(void)
{
	drawOpSub_setDrawInfo(APPDRAW, -1, 0);
	drawOpSub_setDrawInfo_clearMasks();

	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_OVERWRITE);
}

/** 塗りつぶし/消去コマンド用 */

void drawOpSub_setDrawInfo_fillerase(mlkbool erase)
{
	drawOpSub_setDrawInfo(APPDRAW, -1, 0);

	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(
		(erase)? TILEIMAGE_PIXELCOL_ERASE: TILEIMAGE_PIXELCOL_OVERWRITE);
}

/** 上書き描画用 */

void drawOpSub_setDrawInfo_overwrite(void)
{
	drawOpSub_setDrawInfo(APPDRAW, -1, 0);
	drawOpSub_setDrawInfo_clearMasks();

	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_OVERWRITE);
}

/** テキストレイヤの描画用 */

void drawOpSub_setDrawInfo_textlayer(AppDraw *p,mlkbool erase)
{
	//アンドゥ用に、描画前の情報を取得しておく

	TileImage_getInfo(p->curlayer->img, &g_tileimage_dinfo.tileimginfo);

	//

	TileImageDrawInfo_clearDrawRect();

	g_tileimage_dinfo.func_setpixel = TileImage_setPixel_nolimit;

	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(
		(erase)? TILEIMAGE_PIXELCOL_ERASE: TILEIMAGE_PIXELCOL_NORMAL);

	p->w.dstimg = p->curlayer->img;
	p->w.drawcol = (uint64_t)-1;
}

/** フィルタ用に情報セット */

void drawOpSub_setDrawInfo_filter(void)
{
	drawOpSub_setDrawInfo(APPDRAW, -1, 0);
	drawOpSub_setDrawInfo_clearMasks();

	g_tileimage_dinfo.img_sel = APPDRAW->tileimg_sel;

	drawOpSub_setDrawInfo_pixelcol(TILEIMAGE_PIXELCOL_OVERWRITE);
}

/** グラデーション描画時の情報をセット
 *
 * info->buf == NULL で描画しない。
 * 終了時、info->buf を解放すること。 */

void drawOpSub_setDrawGradationInfo(AppDraw *p,TileImageDrawGradInfo *info)
{
	const uint8_t *rawbuf;
	uint32_t val;
	int coltype;
	uint8_t flags = 0;

	if(p->w.is_toollist_toolopt)
		val = p->w.toollist_toolopt;
	else
		val = p->tool.opt_grad;

	coltype = TOOLOPT_GRAD_GET_COLTYPE(val);

	//フラグ (ツール設定から)

	if(TOOLOPT_GRAD_IS_REVERSE(val)) flags |= TILEIMAGE_DRAWGRAD_F_REVERSE;
	if(TOOLOPT_GRAD_IS_LOOP(val)) flags |= TILEIMAGE_DRAWGRAD_F_LOOP;

	//グラデーションデータ

	switch(coltype)
	{
		//黒->白
		case 1:
			rawbuf = Gradation_getData_black_to_white();
			break;
		//白->黒
		case 2:
			rawbuf = Gradation_getData_white_to_black();
			break;
		//カスタム
		case 3:
			rawbuf = GradationList_getBuf_fromID(&p->list_grad_custom, TOOLOPT_GRAD_GET_CUSTOM_ID(val));
			if(rawbuf)
			{
				if(*rawbuf & GRAD_DAT_F_LOOP) flags |= TILEIMAGE_DRAWGRAD_F_LOOP;
				if(*rawbuf & GRAD_DAT_F_SINGLE_COL) flags |= TILEIMAGE_DRAWGRAD_F_SINGLE_COL;

				rawbuf++;
			}
			break;
		//描画色->背景色
		default:
			rawbuf = Gradation_getData_draw_to_bkgnd();
			break;
	}

	//info にセット

	if(!rawbuf)
		info->buf = NULL;
	else if(p->imgbits == 8)
		info->buf = Gradation_convertData_draw_8bit(rawbuf, &p->col.drawcol, &p->col.bkgndcol);
	else
		info->buf = Gradation_convertData_draw_16bit(rawbuf, &p->col.drawcol, &p->col.bkgndcol);

	info->density = (int)(TOOLOPT_GRAD_GET_DENSITY(val) / 100.0 * 256.0 + 0.5);
	info->flags = flags;
}


//=================================
// キャンバス領域への直接描画
//=================================


/** キャンバス領域への直接描画 (XOR) 開始 */

mPixbuf *drawOpSub_beginCanvasDraw(void)
{
	mPixbuf *pixbuf;

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(APPWIDGET->canvaspage));

	if(pixbuf)
		mPixbufSetPixelModeXor(pixbuf);

	return pixbuf;
}

/** キャンバス領域への直接描画終了
 *
 * box: 更新範囲 (キャンバス座標) */

void drawOpSub_endCanvasDraw(mPixbuf *pixbuf,mBox *box)
{
	//[debug]
	//mPixbufBox(pixbuf, box->x, box->y, box->w, box->h, mRGBtoPix(0xff0000));

	mPixbufSetPixelModeCol(pixbuf);

	mWidgetDirectDraw_end(MLK_WIDGET(APPWIDGET->canvaspage), pixbuf);

	mWidgetUpdateBox(MLK_WIDGET(APPWIDGET->canvaspage), box);
}


//============================
// 位置取得
//============================


/** 現在のキャンバス位置を int で取得 */

void drawOpSub_getCanvasPoint_int(AppDraw *p,mPoint *pt)
{	
	pt->x = floor(p->w.dpt_canv_cur.x);
	pt->y = floor(p->w.dpt_canv_cur.y);
}

/** 現在のイメージ位置を int で取得 */

void drawOpSub_getImagePoint_int(AppDraw *p,mPoint *pt)
{
	double x,y;

	drawCalc_canvas_to_image_double(p, &x, &y, p->w.dpt_canv_cur.x, p->w.dpt_canv_cur.y);

	pt->x = floor(x);
	pt->y = floor(y);
}

/** 現在のイメージ位置を取得 (mDoublePoint) */

void drawOpSub_getImagePoint_double(AppDraw *p,mDoublePoint *pt)
{
	drawCalc_canvas_to_image_double(p, &pt->x, &pt->y, p->w.dpt_canv_cur.x, p->w.dpt_canv_cur.y);
}

/** 描画位置取得 (ドットペン自由線時) */

void drawOpSub_getImagePoint_dotpen(AppDraw *p,mPoint *pt)
{
	mDoublePoint dpt;

	drawCalc_canvas_to_image_double(p, &dpt.x, &dpt.y,
		p->w.dpt_canv_cur.x, p->w.dpt_canv_cur.y);

	//定規

	if(p->rule.func_get_point)
	{
		dpt.x = floor(dpt.x) + 0.5;
		dpt.y = floor(dpt.y) + 0.5;

		(p->rule.func_get_point)(p, &dpt.x, &dpt.y, 0);
	}
	
	pt->x = floor(dpt.x);
	pt->y = floor(dpt.y);
}

/** 現在の描画位置取得 (DrawPoint) */

void drawOpSub_getDrawPoint(AppDraw *p,DrawPoint *dst)
{
	drawCalc_canvas_to_image_double(p, &dst->x, &dst->y, p->w.dpt_canv_cur.x, p->w.dpt_canv_cur.y);

	dst->pressure = p->w.dpt_canv_cur.pressure;
}


//============================
// 操作関連
//============================


/** pttmp[0] を 1 .. max までにコピー */

void drawOpSub_copyTmpPoint_0toN(AppDraw *p,int max)
{
	int i;

	for(i = 1; i <= max; i++)
		p->w.pttmp[i] = p->w.pttmp[0];
}

/** (イメージのドラッグ移動) 開始時の初期化
 *
 * img: 移動対象のイメージ  */

void drawOpSub_startMoveImage(AppDraw *p,TileImage *img)
{
	//pttmp[0] : 開始時のオフセット位置

	TileImage_getOffset(img, p->w.pttmp);

	//pttmp[1] : オフセット位置の総相対移動数

	p->w.pttmp[1].x = p->w.pttmp[1].y = 0;

	//ptd_tmp[0] : 総相対移動数 (キャンバス座標値)

	p->w.ptd_tmp[0].x = p->w.ptd_tmp[0].y = 0;

	//rcdraw : 更新範囲

	mRectEmpty(&p->w.rcdraw);
}

/** 押し位置が選択範囲の中かどうか */

mlkbool drawOpSub_isPress_inSelect(AppDraw *p)
{
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	return TileImage_isPixel_opaque(p->tileimg_sel, pt.x, pt.y);
}

/** 直線描画時の位置を取得 */

void drawOpSub_getDrawLinePoints(AppDraw *p,mDoublePoint *pt1,mDoublePoint *pt2)
{
	drawCalc_canvas_to_image_double(p, &pt1->x, &pt1->y, p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_canvas_to_image_double(p, &pt2->x, &pt2->y, p->w.pttmp[1].x, p->w.pttmp[1].y);
}

/** キャンバス回転が 0 の時の四角形描画のイメージ範囲取得
 *
 * box: イメージ座標 */

void drawOpSub_getDrawRectBox_angle0(AppDraw *p,mBox *box)
{
	mPoint pt[4];

	drawCalc_canvas_to_image_pt(p, pt    , p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_canvas_to_image_pt(p, pt + 1, p->w.pttmp[1].x, p->w.pttmp[0].y);
	drawCalc_canvas_to_image_pt(p, pt + 2, p->w.pttmp[1].x, p->w.pttmp[1].y);
	drawCalc_canvas_to_image_pt(p, pt + 3, p->w.pttmp[0].x, p->w.pttmp[1].y);

	//4点を含む範囲
	mBoxSetPoints(box, pt, 4);
}

/** 四角形描画時の頂点４つを取得 */

void drawOpSub_getDrawRectPoints(AppDraw *p,mDoublePoint *pt)
{
	mDoublePoint pt1,pt2;

	pt1.x = p->w.pttmp[0].x, pt1.y = p->w.pttmp[0].y;
	pt2.x = p->w.pttmp[1].x, pt2.y = p->w.pttmp[1].y;

	drawCalc_canvas_to_image_double(p, &pt[0].x, &pt[0].y, pt1.x, pt1.y);
	drawCalc_canvas_to_image_double(p, &pt[1].x, &pt[1].y, pt2.x, pt1.y);
	drawCalc_canvas_to_image_double(p, &pt[2].x, &pt[2].y, pt2.x, pt2.y);
	drawCalc_canvas_to_image_double(p, &pt[3].x, &pt[3].y, pt1.x, pt2.y);
}

/** 楕円描画時の位置と半径を取得 */

void drawOpSub_getDrawEllipseParam(AppDraw *p,mDoublePoint *pt_ct,mDoublePoint *pt_radius,mlkbool fdot)
{
	if(fdot)
	{
		//ドットで描画
		// :キャンバス左右反転の場合は入れ替え

		drawCalc_canvas_to_image_pt(p, &p->w.pttmp[0], p->w.rctmp[0].x1, p->w.rctmp[0].y1);
		drawCalc_canvas_to_image_pt(p, &p->w.pttmp[1], p->w.rctmp[0].x2, p->w.rctmp[0].y2);

		if(p->canvas_mirror)
		{
			p->w.pttmp[2].x = p->w.pttmp[0].x;
			p->w.pttmp[0].x = p->w.pttmp[1].x;
			p->w.pttmp[1].x = p->w.pttmp[2].x;
		}
	}
	else
	{
		//キャンバスに対する
		
		drawCalc_canvas_to_image_double_pt(p, pt_ct, &p->w.pttmp[0]);

		pt_radius->x = p->w.pttmp[1].x * p->viewparam.scalediv;
		pt_radius->y = p->w.pttmp[1].y * p->viewparam.scalediv;
	}
}

