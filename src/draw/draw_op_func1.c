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
 *
 * 操作 - いろいろ1
 *****************************************/
/*
 * 図形塗りつぶし、塗りつぶし、グラデーション、移動ツール、スタンプ
 * キャンバス移動、キャンバス回転、上下ドラッグキャンバス倍率変更
 * スポイト、中間色作成、色置き換え
 */

#include <math.h>

#include "mlk_gui.h"
#include "mlk_rectbox.h"
#include "mlk_nanotime.h"

#include "def_draw.h"
#include "def_tool_option.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"

#include "tileimage.h"
#include "layeritem.h"
#include "layerlist.h"
#include "imagecanvas.h"

#include "fillpolygon.h"
#include "drawfill.h"
#include "undo.h"

#include "appcursor.h"
#include "maincanvas.h"
#include "panel_func.h"



//==============================
// 共通
//==============================


/** [離し] キャンバス状態変更用 */

static mlkbool _common_release_canvas(AppDraw *p)
{
	MainCanvasPage_clearTimer_updatePage();

	drawCanvas_normalQuality();
	drawUpdate_canvas();
	
	return TRUE;
}

/** [離し] グラブ解除しない */

mlkbool drawOp_common_release_none(AppDraw *p)
{
	return FALSE;
}


//=================================
// 描画
//=================================


/** 四角形塗りつぶし描画 (キャンバスに対する) */

void drawOpDraw_fillRect(AppDraw *p)
{
	if(p->canvas_angle == 0)
	{
		//キャンバス回転無しの場合

		mBox box;

		drawOpSub_getDrawRectBox_angle0(p, &box);

		drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
		drawOpSub_beginDraw_single(p);

		TileImage_drawFillBox(p->w.dstimg, box.x, box.y, box.w, box.h, &p->w.drawcol);
		
		drawOpSub_endDraw_single(p);
	}
	else
	{
		//キャンバス回転ありの場合、多角形で

		mDoublePoint pt[4];

		drawOpSub_getDrawRectPoints(p, pt);

		if((p->w.fillpolygon = FillPolygon_new()))
		{
			if(FillPolygon_addPoint4_close(p->w.fillpolygon, pt))
				drawOpDraw_fillPolygon(p);
		
			drawOpSub_freeFillPolygon(p);
		}
	}
}

/** 楕円塗りつぶし */

void drawOpDraw_fillEllipse(AppDraw *p)
{
	mDoublePoint pt,pt_r;

	//ntmp[0] : アンチエイリアス

	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	if(p->canvas_angle == 0 && !p->w.ntmp[0])
	{
		//キャンバス回転 0 で非アンチエイリアス時は、ドットで描画

		drawOpSub_getDrawEllipseParam(p, NULL, NULL, TRUE);

		TileImage_drawFillEllipse_dot(p->w.dstimg,
			p->w.pttmp[0].x, p->w.pttmp[0].y,
			p->w.pttmp[1].x, p->w.pttmp[1].y,
			&p->w.drawcol);
	}
	else
	{
		drawOpSub_getDrawEllipseParam(p, &pt, &pt_r, FALSE);

		TileImage_drawFillEllipse(p->w.dstimg,
			pt.x, pt.y, pt_r.x, pt_r.y, &p->w.drawcol,
			p->w.ntmp[0], //antialias
			&p->viewparam, p->canvas_mirror);
	}

	drawOpSub_endDraw_single(p);
}

/** 多角形塗りつぶし */

void drawOpDraw_fillPolygon(AppDraw *p)
{
	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	TileImage_drawFillPolygon(p->w.dstimg, p->w.fillpolygon,
		&p->w.drawcol, p->w.ntmp[0]);

	drawOpSub_endDraw_single(p);
}

/** グラデーション描画 */

void drawOpDraw_gradation(AppDraw *p)
{
	TileImageDrawGradInfo info;
	mPoint pt[2];
	mRect rc;
	TileImageDrawGradationFunc drawfunc[] = {
		TileImage_drawGradation_line, TileImage_drawGradation_circle,
		TileImage_drawGradation_box, TileImage_drawGradation_radial
	};

	//グラデーション描画情報をセット

	drawOpSub_setDrawGradationInfo(p, &info);
	if(!info.buf) return;

	//位置

	drawCalc_canvas_to_image_pt(p, pt, p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_canvas_to_image_pt(p, pt + 1, p->w.pttmp[1].x, p->w.pttmp[1].y);

	//範囲 (選択範囲があるなら、その範囲)

	drawSel_getFullDrawRect(p, &rc);

	rc.x1 = rc.y1 = 0;
	rc.x2 = p->imgw - 1, rc.y2 = p->imgh - 1;

	//描画

	drawOpSub_setDrawInfo(p, TOOL_GRADATION, 0);
	drawOpSub_beginDraw_single(p);

	(drawfunc[p->w.optool_subno])(p->w.dstimg,
		pt[0].x, pt[0].y, pt[1].x, pt[1].y, &rc, &info);

	mFree(info.buf);

	drawOpSub_endDraw_single(p);
}


//=================================
// 塗りつぶし
//=================================


/* 離し */

static mlkbool _fill_release(AppDraw *p)
{
	DrawFill *draw;
	LayerItem *item;
	mPoint pt;
	int type,diff,density,reflayer;
	uint32_t val;

	//パラメータ値取得

	if(p->w.optoolno == TOOL_FILL)
	{
		//通常塗りつぶし

		if(p->w.is_toollist_toolopt)
			val = p->w.toollist_toolopt;
		else
			val = p->tool.opt_fill;

		type = TOOLOPT_FILL_GET_TYPE(val);
		diff = TOOLOPT_FILL_GET_DIFF(val);
		density = TOOLOPT_FILL_GET_DENSITY(val);
		reflayer = TOOLOPT_FILL_GET_LAYER(val);

		if(type == DRAWFILL_TYPE_CANVAS)
			reflayer = 1;
	}
	else
	{
		//不透明消去

		type = DRAWFILL_TYPE_OPAQUE;
		diff = 0;
		density = 100;
		reflayer = 1;
	}

	//参照レイヤのリンクをセット
	// :参照レイヤがなければ、カレントレイヤ

	item = LayerList_setLink_filltool(p->layerlist, p->curlayer, reflayer);

	//初期化

	drawOpSub_getImagePoint_int(p, &pt);

	draw = DrawFill_new(p->curlayer->img, item->img, &pt, type, diff, density);
	if(!draw) return TRUE;

	//描画

	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	DrawFill_run(draw, &p->w.drawcol);

	DrawFill_free(draw);

	drawOpSub_endDraw_single(p);

	return TRUE;
}

/** 押し */

mlkbool drawOp_fill_press(AppDraw *p)
{
	if(drawOpSub_canDrawLayer_mes(p, 0))
		//描画不可
		return FALSE;
	else
	{
		drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, _fill_release, 0);
		return TRUE;
	}
}


//=============================
// スタンプ
//=============================


/** 範囲決定後、スタンプ画像にセット */

void drawOp_setStampImage(AppDraw *p)
{
	TileImage *img;
	mRect rc;

	//選択範囲イメージ (A1) 作成

	img = drawOpSub_createSelectImage_forStamp(p, &rc);
	if(!img)
	{
		drawStamp_clearImage(p);
		return;
	}

	//範囲内のイメージをコピー

	TileImage_free(p->stamp.img);

	p->stamp.img = TileImage_createStampImage(p->curlayer->img, img, &rc);

	p->stamp.size.w = rc.x2 - rc.x1 + 1;
	p->stamp.size.h = rc.y2 - rc.y1 + 1;
	p->stamp.bits = p->imgbits;

	TileImage_free(img);

	//カーソル変更

	MainCanvasPage_setCursor(APPCURSOR_STAMP);

	//プレビュー更新

	PanelOption_changeStampImage();
}

/** スタンプイメージ貼り付け */

static mlkbool _stamp_press_paste(AppDraw *p)
{
	mPoint pt;
	int fcur,trans;

	if(drawOpSub_canDrawLayer_mes(p, 0)) return FALSE;

	trans = TOOLOPT_STAMP_GET_TRANS(p->tool.opt_stamp);

	fcur = (trans == 5);

	//貼り付け位置

	drawOpSub_getImagePoint_int(p, &pt);

	//描画

	if(fcur) drawCursor_wait();

	drawOpSub_setDrawInfo(p, TOOL_STAMP, 0);
	drawOpSub_beginDraw(p);

	TileImage_pasteStampImage(p->w.dstimg, pt.x, pt.y, trans,
		p->stamp.img, p->stamp.size.w, p->stamp.size.h);

	drawOpSub_finishDraw_single(p);

	if(fcur) drawCursor_restore();

	//離されるまでグラブ

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}

/** 押し */

mlkbool drawOp_stamp_press(AppDraw *p,int subno)
{
	//+Ctrl : イメージをクリアして範囲選択へ

	if(p->w.press_state & MLK_STATE_CTRL)
		drawStamp_clearImage(p);

	//イメージがあれば貼り付け、なければ範囲選択
	
	if(p->stamp.img)
		return _stamp_press_paste(p);
	else
	{
		//----- 範囲選択

		//読み込みが可能な状態なら有効

		if(drawOpSub_canDrawLayer_mes(p, CANDRAWLAYER_F_ENABLE_READ))
			return FALSE;

		//各図形

		switch(subno)
		{
			//四角形
			case 0:
				return drawOpXor_rect_image_press(p, DRAW_OPSUB_SET_STAMP);
			//多角形
			case 1:
				return drawOpXor_polygon_press(p, DRAW_OPSUB_SET_STAMP);
			//フリーハンド
			default:
				return drawOpXor_lasso_press(p, DRAW_OPSUB_SET_STAMP);
		}
	}
}


//==================================
// 移動ツール
//==================================
/*
	pttmp[0]  : 移動開始時の先頭レイヤのオフセット位置
	pttmp[1]  : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動数 (キャンバス座標値, px (double))
	rctmp[0]  : 現在の、対象レイヤすべての表示範囲 (px)
	rcdraw    : 更新範囲 (タイマーにより更新されるとクリア)
	layer     : リンクの先頭

	- フォルダレイヤ選択時は、フォルダ下すべて移動。
	- 移動対象のレイヤは AppDraw::w.layer を先頭に LayerItem::link でリンクされている。
	  (フォルダ/ロックレイヤは含まれない)
*/


/* 移動 */

static void _movetool_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	mRect rc;
	LayerItem *pi;

	//+Shift/+Ctrl でX/Y移動

	if(drawCalc_moveImage_onMotion(p, p->w.layer->img, state, &pt))
	{
		//各レイヤのオフセット位置移動

		for(pi = p->w.layer; pi; pi = pi->link)
			TileImage_moveOffset_rel(pi->img, pt.x, pt.y);

		//更新範囲
		// :rctmp[0] と、rctmp[0] を pt 分相対移動した範囲を合わせたもの

		drawCalc_unionRect_relmove(&rc, p->w.rctmp, pt.x, pt.y);

		//全体の範囲を移動

		mRectMove(p->w.rctmp, pt.x, pt.y);

		//更新
		// :タイマーで更新が行われると、rcdraw が空になる。

		mRectUnion(&p->w.rcdraw, &rc);

		MainCanvasPage_setTimer_updateMove();
	}
}

/* 離し */

static mlkbool _movetool_release(AppDraw *p)
{
	LayerItem *pi;
	mRect rc;
	mPoint ptmov;

	//タイマークリア

	MainCanvasPage_clearTimer_updateMove();

	//残っている範囲を更新

	drawUpdateRect_canvas(p, &p->w.rcdraw);

	//最終的な相対移動数が 0 の場合は、処理なし

	ptmov = p->w.pttmp[1];

	if(ptmov.x || ptmov.y)
	{
		//各レイヤのテキストの位置を移動

		for(pi = p->w.layer; pi; pi = pi->link)
			LayerItem_moveTextPos_all(pi, ptmov.x, ptmov.y);
	
		//undo

		Undo_addLayerMoveOffset(ptmov.x, ptmov.y, p->w.layer);

        //キャンバスビュー更新
        // :移動前の範囲 (p->w.rctmp[0] を総移動数分戻す)
        // : + 移動後の範囲 (p->w.rctmp[0])

        drawCalc_unionRect_relmove(&rc, p->w.rctmp, -(ptmov.x), -(ptmov.y));

        drawUpdateRect_canvasview(p, &rc);
	}

	return TRUE;
}

/* 押し */

mlkbool drawOp_movetool_press(AppDraw *p,int subno)
{
	LayerItem *pi;
	mPoint pt;

	//対象レイヤのリンクをセット

	switch(subno)
	{
		//カレントレイヤ
		case 0:
			pi = LayerList_setLink_movetool_single(p->layerlist, p->curlayer);
			break;
		//つかんだレイヤ
		case 1:
			drawOpSub_getImagePoint_int(p, &pt);

			pi = LayerList_getItem_topPixelLayer(p->layerlist, pt.x, pt.y);
			if(!pi) return FALSE;
			
			pi = LayerList_setLink_movetool_single(p->layerlist, pi);
			break;
		//チェックレイヤ
		case 2:
			pi = LayerList_setLink_checked_nolock(p->layerlist);
			break;
		//すべてのレイヤ
		default:
			pi = LayerList_setLink_movetool_all(p->layerlist);
			break;
	}

	if(!pi) return FALSE;

	//リンクの先頭

	p->w.layer = pi;

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_movetool_motion, _movetool_release, 0);

	//対象レイヤ全体の範囲

	LayerItem_getVisibleImageRect_link(pi, &p->w.rctmp[0]);

	//作業用値、初期化

	drawOpSub_startMoveImage(p, p->w.layer->img);

	return TRUE;
}


//==============================
// キャンバス位置移動
//==============================
/*
	pttmp[0] : 開始時のスクロール位置
*/


/* 移動 */

static void _canvasmove_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;

	pt.x = p->w.pttmp[0].x + (int)(p->w.dpt_canv_press.x - p->w.dpt_canv_cur.x);
	pt.y = p->w.pttmp[0].y + (int)(p->w.dpt_canv_press.y - p->w.dpt_canv_cur.y);

	if(pt.x != p->w.pttmp[0].x || pt.y != p->w.pttmp[0].y)
	{
		p->canvas_scroll = pt;

		MainCanvas_setScrollPos();

		drawCanvas_update_scrollpos(p, FALSE);

		MainCanvasPage_setTimer_updatePage(5);
	}
}

/** 押し */

mlkbool drawOp_canvasMove_press(AppDraw *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_canvasmove_motion, _common_release_canvas, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;
	p->w.pttmp[0] = p->canvas_scroll;
	p->w.drag_cursor_type = APPCURSOR_HAND_GRAB;

	drawCanvas_lowQuality();

	return TRUE;
}


//==============================
// キャンバス回転
//==============================
/*
	ntmp[0] : 現在の回転角度
	ntmp[1] : 前回の角度
*/


/* カーソル位置とキャンバス中央から、角度を取得 */

static int _canvasrotate_get_angle(AppDraw *p)
{
	double x,y;

	x = p->w.dpt_canv_cur.x - p->canvas_size.w * 0.5;
	y = p->w.dpt_canv_cur.y - p->canvas_size.h * 0.5;

	return (int)(atan2(y, x) * 18000 / MLK_MATH_PI);
}

/* 移動 */

static void _canvasrotate_motion(AppDraw *p,uint32_t state)
{
	int angle,n;

	angle = _canvasrotate_get_angle(p);

	//前回の角度からの移動分を加算

	n = p->w.ntmp[0] + angle - p->w.ntmp[1];

	if(n < -18000) n += 36000;
	else if(n > 18000) n -= 36000;

	p->w.ntmp[0] = n;
	p->w.ntmp[1] = angle;

	//+Shift: 45度単位

	if(state & MLK_STATE_SHIFT)
		n = n / 4500 * 4500;

	//更新

	if(n != p->canvas_angle)
	{
		drawCanvas_update(p, 0, n, DRAWCANVAS_UPDATE_ANGLE | DRAWCANVAS_UPDATE_NO_CANVAS_UPDATE);

		MainCanvasPage_setTimer_updatePage(5);
	}
}

/** 押し */

mlkbool drawOp_canvasRotate_press(AppDraw *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_canvasrotate_motion, _common_release_canvas, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;
	p->w.ntmp[0] = p->canvas_angle;
	p->w.ntmp[1] = _canvasrotate_get_angle(p);
	p->w.drag_cursor_type = APPCURSOR_ROTATE;

	drawCanvas_lowQuality();
	drawCanvas_update(p, 0, 0, DRAWCANVAS_UPDATE_RESET_SCROLL | DRAWCANVAS_UPDATE_NO_CANVAS_UPDATE);

	return TRUE;
}


//===============================
// 上下ドラッグで表示倍率変更
//===============================


/* 移動 */

static void _canvaszoom_motion(AppDraw *p,uint32_t state)
{
	int n,x,cur;

	if(p->w.dpt_canv_cur.y == p->w.dpt_canv_last.y)
		return;

	cur = p->canvas_zoom;

	if(p->w.dpt_canv_cur.y < p->w.dpt_canv_last.y)
	{
		n = cur + (p->w.dpt_canv_last.y - p->w.dpt_canv_cur.y) * 10;
		if(n == cur) n = cur + 1;
	}
	else
	{
		n = cur + (p->w.dpt_canv_last.y - p->w.dpt_canv_cur.y) * 10;
		if(n == cur) n = cur - 1;
	}

	drawCanvas_update(p, n, 0, DRAWCANVAS_UPDATE_ZOOM | DRAWCANVAS_UPDATE_NO_CANVAS_UPDATE);

	MainCanvasPage_setTimer_updatePage(5);
}

/** 押し */

mlkbool drawOp_canvasZoom_press(AppDraw *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_canvaszoom_motion, _common_release_canvas, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;
	p->w.drag_cursor_type = APPCURSOR_ZOOM_DRAG;

	drawCanvas_lowQuality();
	drawCanvas_update(p, 0, 0, DRAWCANVAS_UPDATE_RESET_SCROLL | DRAWCANVAS_UPDATE_NO_CANVAS_UPDATE);

	return TRUE;
}


//===============================
// スポイト
//===============================


/* カレントレイヤ上で、クリックした色を描画色 or 透明に置き換える */

static mlkbool _press_spoit_replace(AppDraw *p,int x,int y,mlkbool rep_tp)
{
	RGBA8 src8,dst8;
	RGBA16 src16,dst16;
	void *psrc,*pdst;
	int is_src_tp;

	if(drawOpSub_canDrawLayer_mes(p, 0)) return FALSE;

	if(p->imgbits == 8)
	{
		//---- 8bit
		
		TileImage_getPixel(p->curlayer->img, x, y, &src8);

		if(rep_tp)
			dst8.v32 = 0;
		else
			RGB8_to_RGBA8(&dst8, &p->col.drawcol.c8, 255);

		//同じ色か

		if((src8.a == 0 && dst8.a == 0)
			|| (src8.a && dst8.a && src8.r == dst8.r && src8.g == dst8.g && src8.b == dst8.b))
			return FALSE;

		psrc = &src8;
		pdst = &dst8;
		is_src_tp = (src8.a == 0);
	}
	else
	{
		//---- 16bit

		TileImage_getPixel(p->curlayer->img, x, y, &src16);

		if(rep_tp)
			dst16.v64 = 0;
		else
			RGB16_to_RGBA16(&dst16, &p->col.drawcol.c16, 0x8000);

		//同じ色か

		if((src16.a == 0 && dst16.a == 0)
			|| (src16.a && dst16.a && src16.r == dst16.r && src16.g == dst16.g && src16.b == dst16.b))
			return FALSE;

		psrc = &src16;
		pdst = &dst16;
		is_src_tp = (src16.a == 0);
	}

	//処理

	drawOpSub_setDrawInfo_overwrite();
	drawOpSub_beginDraw_single(p);

	if(is_src_tp)
		//透明 -> 指定色
		TileImage_replaceColor_tp_to_col(p->w.dstimg, pdst);
	else
		//不透明 -> 指定色 or 透明
		TileImage_replaceColor_col_to_col(p->w.dstimg, psrc, pdst);

	drawOpSub_endDraw_single(p);
	
	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}

/* 中間色作成
 *
 * return: TRUE で色をセット */

static mlkbool _press_spoit_middle(AppDraw *p,int x,int y,RGBcombo *dst)
{
	RGBcombo col;
	mNanoTime nt;

	//キャンバス色

	ImageCanvas_getPixel_combo(p->imgcanvas, x, y, &col);

	//

	mNanoTimeGet(&nt);

	if(p->w.midcol_sec == 0 || nt.sec > p->w.midcol_sec + 5)
	{
		//最初の色 (前回押し時から5秒以上経った場合は初期化)

		p->w.midcol_col = col;
		p->w.midcol_sec = nt.sec;

		return FALSE;
	}
	else
	{
		//中間色作成

		*dst = p->w.midcol_col;

		RGBcombo_createMiddle(dst, &col);
		
		p->w.midcol_sec = 0;

		return TRUE;
	}
}

/* 色のスポイト */

static mlkbool _press_spoit_color(AppDraw *p,int subno,int x,int y,uint32_t state)
{
	RGBAcombo rgba;
	RGBcombo rgb;

	if(subno == TOOLSUB_SPOIT_MIDDLE)
	{
		//--- 中間色

		if(!_press_spoit_middle(p, x, y, &rgb))
			return FALSE;
	}
	else
	{
		//---- 通常スポイト

		//+Ctrl : カレントレイヤ

		if(state & MLK_STATE_CTRL) subno = 1;
		
		//色取得

		if(subno == 1)
		{
			//カレントレイヤ (フォルダは除く)
			
			if(drawOpSub_canDrawLayer_mes(p, CANDRAWLAYER_F_ENABLE_READ | CANDRAWLAYER_F_ENABLE_TEXT))
				return FALSE;

			TileImage_getPixel_combo(p->curlayer->img, x, y, &rgba);

			RGBAcombo_to_RGBcombo(&rgb, &rgba);
		}
		else
		{
			//キャンバス色
			// :背景がチェック柄の場合は、その色となる。
			// :ピクセル単位で合成する場合、トーン化レイヤが問題になるので、そのまま取得する。

			ImageCanvas_getPixel_combo(p->imgcanvas, x, y, &rgb);
		}
	}

	//----- セット

	if(state & MLK_STATE_SHIFT)
	{
		//+Shift : 色マスクの1番目にセット

		p->col.maskcol[0].r = rgb.c8.r;
		p->col.maskcol[0].g = rgb.c8.g;
		p->col.maskcol[0].b = rgb.c8.b;

		PanelColor_changeColorMask_color1();
	}
	else
	{
		//描画色にセット

		p->col.drawcol = rgb;
		
		PanelColor_changeDrawColor();
	}

	//グラブは行う

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}

/** 押し */

mlkbool drawOp_spoit_press(AppDraw *p,int subno,mlkbool enable_state)
{
	mPoint pt;
	uint32_t state;

	state = (enable_state)? p->w.press_state: 0;

	drawOpSub_getImagePoint_int(p, &pt);

	//イメージ範囲外

	if(pt.x < 0 || pt.y < 0 || pt.x >= p->imgw || pt.y >= p->imgh)
		return FALSE;

	//

	if(subno == TOOLSUB_SPOIT_REPLACE_DRAWCOL
		|| subno == TOOLSUB_SPOIT_REPLACE_TP)
	{
		//色置き換え

		return _press_spoit_replace(p, pt.x, pt.y, (subno == TOOLSUB_SPOIT_REPLACE_TP));
	}
	else
	{
		//色のスポイト

		return _press_spoit_color(p, subno, pt.x, pt.y, state);
	}
}

