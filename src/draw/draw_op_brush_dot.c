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
 * 操作 - ブラシ/ドットペン
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mlk_gui.h"

#include "def_config.h"
#include "def_draw.h"
#include "def_draw_toollist.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"

#include "pointbuf.h"
#include "toollist.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_toollist.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"

#include "panel_func.h"



//===========================
// ブラシ描画 [自由線]
//===========================


/* 離し */

static mlkbool _brush_free_release(AppDraw *p)
{
	PointBufDat pt,pt2,ptnext;
	int i,update,loop = 1;

	update = (p->rule.drawpoint_num >= 2)? DRAWOPSUB_UPDATE_DIRECT: DRAWOPSUB_UPDATE_TIMER;

	//残りの線を描画

	while(loop)
	{
		//終端かどうかを判断するため、次の点も取得

		loop = PointBuf_getFinishPoint(p->pointbuf, &pt);
		if(!loop) break;
		
		loop = PointBuf_getFinishPoint(p->pointbuf, &ptnext);
	
		//メイン線
		// :pt が終端なら、曲線の最後も描画
		
		TileImageDrawInfo_clearDrawRect();

		TileImage_drawBrushFree(p->w.dstimg, 0, pt.x, pt.y, pt.pressure);

		if(!loop)
			TileImage_drawBrushFree_finish(p->w.dstimg, 0);

		drawOpSub_addrect_and_update(p, update);

		//線対称

		for(i = 1; i < p->rule.drawpoint_num; i++)
		{
			pt2 = pt;
			
			(p->rule.func_get_point)(p, &pt2.x, &pt2.y, i);

			TileImageDrawInfo_clearDrawRect();

			TileImage_drawBrushFree(p->w.dstimg, i, pt2.x, pt2.y, pt2.pressure);

			if(!loop)
				TileImage_drawBrushFree_finish(p->w.dstimg, i);

			drawOpSub_addrect_and_update(p, update);
		}

		//

		pt = ptnext;
	}

	//終了

	drawOpSub_finishDraw_workrect(p);

	return TRUE;
}

/* 移動 */

static void _brush_free_motion(AppDraw *p,uint32_t state)
{
	DrawPoint dpt;
	PointBufDat pt,pt2;
	int i,update;

	//----- 位置

	drawOpSub_getDrawPoint(p, &dpt);

	//定規

	if(p->rule.func_get_point)
		(p->rule.func_get_point)(p, &dpt.x, &dpt.y, 0);

	//筆圧最大

	//if(p->w.opflags & DRAW_OPFLAGS_BRUSH_PRESSURE_MAX)
	//	dpt.pressure = 1;

	//追加

	PointBuf_addPoint(p->pointbuf, dpt.x, dpt.y, dpt.pressure);

	//---- 描画

	//線対称の場合は、それぞれを別々に更新した方が速い

	update = (p->rule.drawpoint_num >= 2)? DRAWOPSUB_UPDATE_DIRECT: DRAWOPSUB_UPDATE_TIMER;

	//pt = 手ぶれ補正後の位置

	if(PointBuf_getPoint(p->pointbuf, &pt))
	{
		TileImageDrawInfo_clearDrawRect();

		TileImage_drawBrushFree(p->w.dstimg, 0, pt.x, pt.y, pt.pressure);

		drawOpSub_addrect_and_update(p, update);

		//線対称

		for(i = 1; i < p->rule.drawpoint_num; i++)
		{
			pt2 = pt;
			
			(p->rule.func_get_point)(p, &pt2.x, &pt2.y, i);

			TileImageDrawInfo_clearDrawRect();

			TileImage_drawBrushFree(p->w.dstimg, i, pt2.x, pt2.y, pt2.pressure);

			drawOpSub_addrect_and_update(p, update);
		}
	}
}

/** 押し */

mlkbool drawOp_brush_free_press(AppDraw *p)
{
	DrawPoint dpt;
	int i;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_DRAW_FREE,
		_brush_free_motion, _brush_free_release, 0);

	//描画情報

	drawOpSub_setDrawInfo(p, TOOL_TOOLLIST, 0);

	//筆圧最大フラグ

	//if(pressure_max)
	//	p->w.opflags |= DRAW_OPFLAGS_BRUSH_PRESSURE_MAX;

	//位置

	drawOpSub_getDrawPoint(p, &dpt);

	//if(pressure_max) dpt.pressure = 1;

	//PointBuf
	// :手ブレ補正のセットは、drawOpSub_setOpInfo() 時に行われる。
	// :定規ON & 線対称以外の場合、手ブレ補正はOFFとなる。

	PointBuf_setFirstPoint(p->pointbuf, dpt.x, dpt.y, dpt.pressure);

	//自由線開始

	TileImage_drawBrush_beginFree(p->w.dstimg, 0, dpt.x, dpt.y, dpt.pressure);

	for(i = 1; i < p->rule.drawpoint_num; i++)
	{
		(p->rule.func_get_point)(p, &dpt.x, &dpt.y, i);
		
		TileImage_drawBrush_beginFree(p->w.dstimg, i, dpt.x, dpt.y, dpt.pressure);
	}

	//描画開始

	drawOpSub_beginDraw(p);

	return TRUE;
}


//=============================
// ドットペン/指先 [自由線]
//=============================
/*
	pttmp[0]: 前回の位置
	pttmp[1]: 2つ前の位置
	(線対称の場合) pttmp[2-]: 前回の位置、2つ前の位置
	ntmp[0] : 細線
*/


/* 2点間の描画
 *
 * ptlast: [0]前回の位置 [1]2つ前の位置 */

static void _dotpen_draw(AppDraw *p,mPoint *pt,mPoint *ptlast)
{
	mlkbool fdraw = TRUE;

	//前回と同じ位置なら描画しない

	if(pt->x == ptlast->x && pt->y == ptlast->y)
		return;

	//(細線時) 2つ前の位置が斜めの場合は描画しない

	if(p->w.ntmp[0])
	{
		if(abs(pt->x - ptlast[1].x) == 1
			&& abs(pt->y - ptlast[1].y) == 1
			&& (pt->x == ptlast->x || pt->y == ptlast->y))
		{
			fdraw = FALSE;
		}
	}

	//描画

	if(fdraw)
	{
		TileImageDrawInfo_clearDrawRect();

		if(p->w.ntmp[0])
		{
			//細線
			TileImage_drawLineSlim(p->w.dstimg, ptlast->x, ptlast->y,
				pt->x, pt->y, &p->w.drawcol);
		}
		else
		{
			//通常
			TileImage_drawLineB(p->w.dstimg, ptlast->x, ptlast->y,
				pt->x, pt->y, &p->w.drawcol, TRUE);
		}

		//線対称の場合、別々に更新した方が速い

		drawOpSub_addrect_and_update(p,
			(p->rule.drawpoint_num == 2)? DRAWOPSUB_UPDATE_DIRECT: DRAWOPSUB_UPDATE_ADD);
	}

	//前回の位置にセット

	if(fdraw)
		ptlast[1] = ptlast[0];

	ptlast[0] = *pt;
}

/* 移動 */

static void _dotpen_free_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	int i,pos;
	double dx,dy;

	//通常

	drawOpSub_getImagePoint_dotpen(p, &pt);

	_dotpen_draw(p, &pt, p->w.pttmp);

	//定規、対称

	for(i = 1, pos = 2; i < p->rule.drawpoint_num; i++, pos += 2)
	{
		dx = pt.x + 0.5;
		dy = pt.y + 0.5;
		
		(p->rule.func_get_point)(p, &dx, &dy, i);

		pt.x = floor(dx);
		pt.y = floor(dy);

		_dotpen_draw(p, &pt, p->w.pttmp + pos);
	}
}

/* 離し */

static mlkbool _dotpen_free_release(AppDraw *p)
{
	drawOpSub_finishDraw_workrect(p);
	
	return TRUE;
}

/** 押し */

mlkbool drawOp_dotpen_free_press(AppDraw *p)
{
	mPoint pt;
	int i,pos;
	double dx,dy;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_DRAW_FREE,
		_dotpen_free_motion, _dotpen_free_release, 0);

	//描画情報
	// :+Shift: 1px消しゴム (ドットペン/消しゴム時)

	drawOpSub_setDrawInfo(p, p->w.optoolno, (p->w.press_state & MLK_STATE_SHIFT));

	//描画位置

	drawOpSub_getImagePoint_dotpen(p, &pt);

	p->w.pttmp[0] = pt;

	//(指先) 始点の色をセット

	if(p->w.optoolno == TOOL_FINGER)
		TileImage_setFingerBuf(p->w.dstimg, pt.x, pt.y);

	//描画開始

	drawOpSub_beginDraw(p);

	//最初の点を打つ

	(g_tileimage_dinfo.func_setpixel)(p->w.dstimg, pt.x, pt.y, &p->w.drawcol);

	//定規、対称の点

	for(i = 1, pos = 2; i < p->rule.drawpoint_num; i++, pos += 2)
	{
		dx = pt.x + 0.5;
		dy = pt.y + 0.5;
		
		(p->rule.func_get_point)(p, &dx, &dy, i);

		pt.x = floor(dx);
		pt.y = floor(dy);

		(g_tileimage_dinfo.func_setpixel)(p->w.dstimg, pt.x, pt.y, &p->w.drawcol);

		p->w.pttmp[pos] = p->w.pttmp[pos + 1] = pt;
	}

	//更新

	drawOpSub_addrect_and_update(p,
		(p->rule.drawpoint_num == 2)? DRAWOPSUB_UPDATE_DIRECT: DRAWOPSUB_UPDATE_ADD);

	return TRUE;
}


//=============================
// 自由線以外の描画共通処理
//=============================


/** 自由線以外の描画開始時、開始点の状態をセット */

static void _set_startpoint(AppDraw *p,mDoublePoint *pt)
{
	switch(p->w.optoolno)
	{
		//ブラシ
		case TOOL_TOOLLIST:
			TileImage_drawBrush_beginOther(p->w.dstimg, pt->x, pt->y);
			break;
		//指先
		case TOOL_FINGER:
			TileImage_setFingerBuf(p->w.dstimg, pt->x, pt->y);
			break;
	}
}

/** 自由線以外の描画開始
 *
 * pt: 開始点 */

static void _begin_draw_nofree(AppDraw *p,mDoublePoint *pt)
{
	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw_single(p);

	_set_startpoint(p, pt);
}


//=============================
// 描画処理
//=============================


/** 直線描画 */

void drawOpDraw_brushdot_line(AppDraw *p)
{
	mDoublePoint pt1,pt2;

	drawOpSub_getDrawLinePoints(p, &pt1, &pt2);

	_begin_draw_nofree(p, &pt1);

	if(p->w.optoolno != TOOL_TOOLLIST)
		TileImage_drawLineB(p->w.dstimg, pt1.x, pt1.y, pt2.x, pt2.y, &p->w.drawcol, FALSE);
	else
	{
		TileImage_drawBrushLine_headtail(p->w.dstimg,
			pt1.x, pt1.y, pt2.x, pt2.y, p->headtail.curval[0]);
	}

	drawOpSub_endDraw_single(p);
}

/** 四角形枠描画 */

void drawOpDraw_brushdot_rect(AppDraw *p)
{
	mDoublePoint pt[4];
	int i,n;

	drawOpSub_getDrawRectPoints(p, pt);

	_begin_draw_nofree(p, pt);

	if(p->w.optoolno == TOOL_TOOLLIST)
		TileImage_drawBrushBox(p->w.dstimg, pt);
	else
	{
		//ドットペン/指先
		
		for(i = 0; i < 4; i++)
		{
			n = (i + 1) & 3;
		
			TileImage_drawLineB(p->w.dstimg,
				pt[i].x, pt[i].y, pt[n].x, pt[n].y, &p->w.drawcol, TRUE);
		}
	}

	drawOpSub_endDraw_single(p);
}

/** 楕円枠描画 */

void drawOpDraw_brushdot_ellipse(AppDraw *p)
{
	mDoublePoint pt,pt_r,ptst;
	double x;
	int fdot;

	//ドットペンでキャンバス回転0の場合は、ドット単位で描画 (指先は除く)

	fdot = ((p->w.optoolno == TOOL_DOTPEN || p->w.optoolno == TOOL_DOTPEN_ERASE)
		&& p->canvas_angle == 0);

	drawOpSub_getDrawEllipseParam(p, &pt, &pt_r, fdot);

	//

	if(fdot)
	{
		//ドット単位
		
		_begin_draw_nofree(p, NULL);
		
		TileImage_drawEllipse_dot(p->w.dstimg,
			p->w.pttmp[0].x, p->w.pttmp[0].y,
			p->w.pttmp[1].x, p->w.pttmp[1].y, &p->w.drawcol);
	}
	else
	{
		//描画開始点は角度 0 の位置

		x = pt_r.x * p->viewparam.cosrev;
		if(p->canvas_mirror) x = -x;

		ptst.x = pt.x + x;
		ptst.y = pt.y + pt_r.x * p->viewparam.sinrev;

		_begin_draw_nofree(p, &ptst);

		//

		TileImage_drawEllipse(p->w.dstimg,
			pt.x, pt.y, pt_r.x, pt_r.y, &p->w.drawcol,
			(p->w.optoolno != TOOL_TOOLLIST), &p->viewparam, p->canvas_mirror);
	}

	drawOpSub_endDraw_single(p);
}

/** 連続直線/集中線 描画 */

void drawOpDraw_brushdot_lineSuccConc(AppDraw *p)
{
	mDoublePoint pt1,pt2;

	drawOpSub_getDrawLinePoints(p, &pt1, &pt2);

	TileImageDrawInfo_clearDrawRect();

	_set_startpoint(p, &pt1);

	if(p->w.optoolno != TOOL_TOOLLIST)
		TileImage_drawLineB(p->w.dstimg, pt1.x, pt1.y, pt2.x, pt2.y, &p->w.drawcol, FALSE);
	else
	{
		TileImage_drawBrushLine_headtail(p->w.dstimg,
			pt1.x, pt1.y, pt2.x, pt2.y, p->headtail.curval[0]);
	}

	//この後に XOR を描画するので、キャンバスは直接描画

	drawOpSub_addrect_and_update(p, DRAWOPSUB_UPDATE_DIRECT);
}


//===========================
// ベジェ曲線
//===========================
/*
	pttmp[0-3] : 始点、制御点1、制御点2、終点
	boxtmp[0]  : XOR で描画された範囲
	optype_sub : [0] 1つ目の制御点 [1] 2つ目の制御点
*/


/* 移動 */

static void _bezier_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;

	drawOpXor_drawBezier(p, TRUE);

	drawOpSub_getCanvasPoint_int(p, &pt);

	if(state & MLK_STATE_SHIFT)
		drawCalc_fitLine45(&pt, p->w.pttmp + (p->w.optype_sub? 3: 0));

	p->w.pttmp[1 + p->w.optype_sub] = pt;

	if(p->w.optype_sub == 0)
		p->w.pttmp[2] = pt;

	drawOpXor_drawBezier(p, FALSE);
}

/* 離し */

static mlkbool _bezier_release(AppDraw *p)
{
	drawOpXor_drawBezier(p, TRUE);

	if(p->w.optype_sub == 0)
	{
		//----- 2つ目の制御点へ

		p->w.pttmp[2] = p->w.pttmp[1];
		p->w.optype_sub = 1;

		drawOpXor_drawBezier(p, FALSE);

		return FALSE;
	}
	else
	{
		//----- 終了

		mDoublePoint pt[4];
		int i;

		//各点のイメージ位置

		for(i = 0; i < 4; i++)
			drawCalc_canvas_to_image_double_pt(p, pt + i, p->w.pttmp + i);

		//描画

		_begin_draw_nofree(p, pt);

		TileImage_drawBezier(p->w.dstimg, pt, &p->w.drawcol,
			(p->w.optoolno != TOOL_TOOLLIST), p->headtail.curval[1]);

		drawOpSub_endDraw_single(p);

		//

		drawOpSub_freeTmpImage(p);

		return TRUE;
	}
}

/* 各アクション */

static mlkbool _bezier_action(AppDraw *p,int action)
{
	switch(action)
	{
		//右ボタン/ESC/グラブ解放 : キャンセル
		case DRAW_FUNC_ACTION_RBTT_UP:
		case DRAW_FUNC_ACTION_KEY_ESC:
		case DRAW_FUNC_ACTION_UNGRAB:
			drawOpXor_drawBezier(p, TRUE);
			drawOpSub_freeTmpImage(p);
			return TRUE;

		//BackSpace : 一つ目の制御点に戻る
		case DRAW_FUNC_ACTION_KEY_BACKSPACE:
			if(p->w.optype_sub == 1)
			{
				p->w.optype_sub = 0;
				_bezier_motion(p, 0);
			}
			return FALSE;
	}

	return FALSE;
}

/** xorline から制御点指定へ移行
 *
 * return: グラブ解除するか */

mlkbool drawOp_xorline_to_bezier(AppDraw *p)
{
	if(!drawOpSub_createTmpImage_canvas(p))
		return TRUE;
		
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_BEZIER,
		_bezier_motion, _bezier_release, 0);

	p->w.func_action = _bezier_action;

	p->w.pttmp[3] = p->w.pttmp[1];
	p->w.pttmp[1] = p->w.pttmp[2] = p->w.pttmp[3];

	//作業用イメージへの描画用
	// :TileImageDrawInfo::rcdraw に描画した範囲を追加
	
	g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new_drawrect;

	drawOpXor_drawBezier(p, FALSE);

	return FALSE;
}


//============================================
// 選択アイテムのブラシサイズ変更
//============================================
/*
	pttmp[0]  : 押し時の位置
	ntmp[0]   : 直径サイズ(キャンバス上でのpx)
	boxtmp[0] : XOR 描画範囲
*/


/* 移動 */

static void _brushsize_motion(AppDraw *p,uint32_t state)
{
	BrushEditData *pb;
	int size,n;

	pb = p->tlist->brush;

	//サイズ

	size = pb->v.size;

	if(p->w.dpt_canv_cur.x > p->w.dpt_canv_last.x)
	{
		if(size < 100)
			n = size + 10;
		else
			n = (int)(size * 1.03);
	}
	else
	{
		if(size < 100)
			n = size - 10;
		else
			n = (int)(size * 0.97);
	}

	size = BrushEditData_adjustSize(pb, n);
	
	//変更

	if(size != pb->v.size)
	{
		drawBrushParam_setSize(size);

		//円描画
		
		drawOpXor_drawBrushSizeCircle(p, TRUE);

		p->w.ntmp[0] = (int)(size * 0.1 * p->viewparam.scale + 0.5);

		drawOpXor_drawBrushSizeCircle(p, FALSE);

		//更新

		PanelToolList_updateBrushSize();
		PanelBrushOpt_updateBrushSize();
	}
}

/* 離し */

static mlkbool _brushsize_release(AppDraw *p)
{
	drawOpXor_drawBrushSizeCircle(p, TRUE);
	
	return TRUE;
}

/** 押し */

mlkbool drawOp_dragBrushSize_press(AppDraw *p)
{
	int size;

	if(!p->tlist->selitem
		|| (p->tlist->selitem)->type != TOOLLIST_ITEMTYPE_BRUSH)
		return FALSE;

	//

	size = p->tlist->brush->v.size;

	drawOpSub_getCanvasPoint_int(p, &p->w.pttmp[0]);

	p->w.ntmp[0] = (int)(size * 0.1 * p->viewparam.scale + 0.5);

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_brushsize_motion, _brushsize_release, 0);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	//円描画

	drawOpXor_drawBrushSizeCircle(p, FALSE);

	return TRUE;
}

