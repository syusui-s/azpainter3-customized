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
 * XOR 処理
 *****************************************/

#include <stdlib.h>

#include "mlk_gui.h"
#include "mlk_rectbox.h"
#include "mlk_pixbuf.h"

#include "def_draw.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "fillpolygon.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_func.h"
#include "draw_op_sub.h"
#include "draw_rule.h"



//=============================
// sub
//=============================


/* 矩形:共通処理
 *
 * x,y を新しい位置として、左上と右下位置を、dst_rc にセットする。
 *
 * +Shift: 正方形調整
 *
 * pt_start: 押し時の位置 */

static void _common_rect_func(mRect *dst_rc,mPoint *pt_start,int x,int y,uint32_t state)
{
	int x1,y1,x2,y2,w,h,stx,sty;

	stx = pt_start->x;
	sty = pt_start->y;

	//左上、左下

	if(x < stx)
		x1 = x, x2 = stx;
	else
		x1 = stx, x2 = x;

	if(y < sty)
		y1 = y, y2 = sty;
	else
		y1 = sty, y2 = y;

	//正方形調整

	if(state & MLK_STATE_SHIFT)
	{
		w = x2 - x1;
		h = y2 - y1;

		if(w < h)
		{
			//幅を合わせる
			if(x < stx)
				x1 = x2 - h;
			else
				x2 = x1 + h;
		}
		else
		{
			//高さを合わせる
			if(y < sty)
				y1 = y2 - w;
			else
				y2 = y1 + w;
		}
	}

	//セット

	dst_rc->x1 = x1, dst_rc->y1 = y1;
	dst_rc->x2 = x2, dst_rc->y2 = y2;
}

/* 矩形の位置をセット
 * 
 * pttmp[0] : 左上
 * pttmp[1] : 右下
 * pttmp[2] : 開始点 */

static void _common_rect_point(AppDraw *p,int x,int y,uint32_t state)
{
	mRect rc;

	_common_rect_func(&rc, &p->w.pttmp[2], x, y, state);

	p->w.pttmp[0].x = rc.x1;
	p->w.pttmp[0].y = rc.y1;
	p->w.pttmp[1].x = rc.x2;
	p->w.pttmp[1].y = rc.y2;
}


//=============================
// XOR直線
//=============================
/*
	pttmp[0] : 始点
	pttmp[1] : 終点 (現在の位置)
*/


/* 移動 (連続直線/集中線/多角形 と共通) */

static void _xorline_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;

	drawOpSub_getCanvasPoint_int(p, &pt);

	if(pt.x == p->w.pttmp[1].x && pt.y == p->w.pttmp[1].y)
		return;

	//

	drawOpXor_drawline(p);

	if(state & MLK_STATE_SHIFT)
		drawCalc_fitLine45(&pt, p->w.pttmp);

	p->w.pttmp[1] = pt;

	drawOpXor_drawline(p);
}

/* 離し */

static mlkbool _xorline_release(AppDraw *p)
{
	drawOpXor_drawline(p);

	switch(p->w.optype_sub)
	{
		//直線描画
		case DRAW_OPSUB_DRAW_LINE:
			drawOpDraw_brushdot_line(p);
			break;
		//ベジェ曲線の制御点操作に移行
		case DRAW_OPSUB_TO_BEZIER:
			return drawOp_xorline_to_bezier(p);
		//定規設定
		case DRAW_OPSUB_RULE_SETTING:
			drawRule_setting_line(p);
			break;
		//グラデーション描画
		case DRAW_OPSUB_DRAW_GRADATION:
			drawOpDraw_gradation(p);
			break;
	}
	
	return TRUE;
}

/** 押し */

mlkbool drawOpXor_line_press(AppDraw *p,int sub)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_LINE,
		_xorline_motion, _xorline_release, sub);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	p->w.pttmp[1] = p->w.pttmp[0];

	drawOpXor_drawline(p);

	return TRUE;
}


//==================================
// XOR 四角形 (キャンバスに対する)
//==================================
/*
	pttmp[0] : 左上位置 (キャンバス座標)
	pttmp[1] : 右下位置
	pttmp[2] : 開始点
*/


/* 移動 */

static void _xorrect_canv_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;

	drawOpXor_drawRect_canv(p);

	drawOpSub_getCanvasPoint_int(p, &pt);

	_common_rect_point(p, pt.x, pt.y, state);

	drawOpXor_drawRect_canv(p);
}

/* 離し */

static mlkbool _xorrect_canv_release(AppDraw *p)
{
	drawOpXor_drawRect_canv(p);

	switch(p->w.optype_sub)
	{
		//四角形枠描画
		case DRAW_OPSUB_DRAW_FRAME:
			drawOpDraw_brushdot_rect(p);
			break;
		//四角形塗りつぶし
		case DRAW_OPSUB_DRAW_FILL:
			drawOpDraw_fillRect(p);
			break;
		//選択範囲セット
		case DRAW_OPSUB_SET_SELECT:
			drawOp_setSelect(p, 0);
			break;
	}
	
	return TRUE;
}

/** 押し */

mlkbool drawOpXor_rect_canv_press(AppDraw *p,int sub)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_RECT_CANV,
		_xorrect_canv_motion, _xorrect_canv_release, sub);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	p->w.pttmp[1] = p->w.pttmp[2] = p->w.pttmp[0];

	drawOpXor_drawRect_canv(p);

	return TRUE;
}


//=================================
// XOR 四角形 (イメージに対する)
//=================================
/*
	pttmp[0] : 左上位置 (イメージ座標)
	pttmp[1] : 右下位置 (イメージ座標)
	pttmp[2] : 開始点 (イメージ座標)
	pttmp[3] : 前回の位置 (イメージ座標)

	boxtmp[0] : 結果の mBox
	rctmp[0]  : 結果の mRect
	
	[!] 結果の範囲は、クリッピングは行われない。
*/


/* 移動 */

static void _xorrect_image_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	if(pt.x != p->w.pttmp[3].x || pt.y != p->w.pttmp[3].y)
	{
		drawOpXor_drawRect_image(p);

		_common_rect_point(p, pt.x, pt.y, state);

		drawOpXor_drawRect_image(p);

		p->w.pttmp[3] = pt;
	}
}

/* 離し */

static mlkbool _xorrect_image_release(AppDraw *p)
{
	drawOpXor_drawRect_image(p);

	//結果の範囲 (mBox, mRect)

	mBoxSetPoint_minmax(p->w.boxtmp, p->w.pttmp, p->w.pttmp + 1);

	p->w.rctmp[0].x1 = p->w.pttmp[0].x, p->w.rctmp[0].y1 = p->w.pttmp[0].y;
	p->w.rctmp[0].x2 = p->w.pttmp[1].x, p->w.rctmp[0].y2 = p->w.pttmp[1].y;

	//

	switch(p->w.optype_sub)
	{
		//スタンプ画像セット
		case DRAW_OPSUB_SET_STAMP:
			drawOp_setStampImage(p);
			break;
		//矩形選択範囲セット
		case DRAW_OPSUB_SET_BOXSEL:
			drawBoxSel_setRect(p, p->w.rctmp);
			break;
	}

	return TRUE;
}

/** 押し */

mlkbool drawOpXor_rect_image_press(AppDraw *p,int sub)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_RECT_IMAGE,
		_xorrect_image_motion, _xorrect_image_release, sub);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getImagePoint_int(p, p->w.pttmp);

	drawOpSub_copyTmpPoint_0toN(p, 3);

	drawOpXor_drawRect_image(p);

	return TRUE;
}


//===============================
// XOR 楕円 (キャンバスに対する)
//===============================
/*
	pttmp[0] : 中心位置 (キャンバス座標)
	pttmp[1] : 半径
	pttmp[2] : 押し時の位置
	rctmp[0] : 矩形での位置
*/


/* 移動 */

static void _xorellipse_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	int x,y,xr,yr;

	drawOpXor_drawEllipse(p);

	drawOpSub_getCanvasPoint_int(p, &pt);

	if((state & MLK_STATE_CTRL) && p->w.optype_sub != DRAW_OPSUB_RULE_SETTING)
	{
		//矩形 (円定規の場合は無効)

		_common_rect_func(p->w.rctmp, &p->w.pttmp[2], pt.x, pt.y, state);

		p->w.pttmp[1].x = (p->w.rctmp[0].x2 - p->w.rctmp[0].x1 + 1) / 2;
		p->w.pttmp[1].y = (p->w.rctmp[0].y2 - p->w.rctmp[0].y1 + 1) / 2;
		p->w.pttmp[0].x = p->w.rctmp[0].x1 + p->w.pttmp[1].x;
		p->w.pttmp[0].y = p->w.rctmp[0].y1 + p->w.pttmp[1].y;
	}
	else
	{
		//中心と半径

		x = p->w.pttmp[0].x;
		y = p->w.pttmp[0].y;

		xr = abs(pt.x - x);
		yr = abs(pt.y - y);

		if(state & MLK_STATE_SHIFT)
		{
			//正円
			
			if(xr > yr) yr = xr;
			else xr = yr;
		}

		p->w.pttmp[1].x = xr;
		p->w.pttmp[1].y = yr;

		p->w.rctmp[0].x1 = x - xr;
		p->w.rctmp[0].y1 = y - yr;
		p->w.rctmp[0].x2 = x + xr;
		p->w.rctmp[0].y2 = y + yr; 
	}

	drawOpXor_drawEllipse(p);
}

/* 離し */

static mlkbool _xorellipse_release(AppDraw *p)
{
	drawOpXor_drawEllipse(p);

	switch(p->w.optype_sub)
	{
		//円枠描画
		case DRAW_OPSUB_DRAW_FRAME:
			drawOpDraw_brushdot_ellipse(p);
			break;
		//円塗りつぶし描画
		case DRAW_OPSUB_DRAW_FILL:
			drawOpDraw_fillEllipse(p);
			break;
		//定規設定
		case DRAW_OPSUB_RULE_SETTING:
			drawRule_setting_ellipse(p);
			break;
	}
	
	return TRUE;
}

/** 押し */

mlkbool drawOpXor_ellipse_press(AppDraw *p,int sub)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_ELLIPSE,
		_xorellipse_motion, _xorellipse_release, sub);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	p->w.pttmp[1].x = p->w.pttmp[1].y = 0;
	p->w.pttmp[2] = p->w.pttmp[0];

	mRectSetPoint(p->w.rctmp, p->w.pttmp);

	drawOpXor_drawEllipse(p);

	return TRUE;
}


//========================================
// XOR 複数直線 (連続直線/集中線)
//========================================
/*
	操作中、直接描画していく。

	pttmp[0] : 始点
	pttmp[1] : 終点 (現在位置)
	pttmp[2] : 一番最初の点 (保存用)
	rcdraw : 全体の描画範囲
*/


/* 2回目以降の押し時 */

static void _xorsumline_press_in_grab(AppDraw *p,uint32_t state)
{
	drawOpXor_drawline(p);

	drawOpDraw_brushdot_lineSuccConc(p);

	//連続直線の場合、始点変更

	if(p->w.optype_sub == DRAW_OPSUB_DRAW_SUCCLINE)
		p->w.pttmp[0] = p->w.pttmp[1];

	drawOpXor_drawline(p);
}

/* 各アクション */

static mlkbool _xorsumline_action(AppDraw *p,int action)
{
	//BackSpace は連続直線のみ

	if(action == DRAW_FUNC_ACTION_KEY_BACKSPACE
		&& p->w.optype_sub != DRAW_OPSUB_DRAW_SUCCLINE)
		return FALSE;

	//----- 終了

	drawOpXor_drawline(p);

	//BackSpace (連続直線) => 始点と結ぶ

	if(action == DRAW_FUNC_ACTION_KEY_BACKSPACE)
	{
		p->w.pttmp[1] = p->w.pttmp[2];

		drawOpDraw_brushdot_lineSuccConc(p);
	}

	drawOpSub_finishDraw_workrect(p);

	return TRUE;
}

/* 複数直線の初期化 (多角形と共通) */

static void _xorsumline_init(AppDraw *p,int optype,int sub)
{
	drawOpSub_setOpInfo(p, optype,
		_xorline_motion, drawOp_common_release_none, sub);

	p->w.opflags |= DRAW_OPFLAGS_MOTION_POS_INT;

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	drawOpSub_copyTmpPoint_0toN(p, 2);

	drawOpXor_drawline(p);
}

/** 押し */

mlkbool drawOpXor_sumline_press(AppDraw *p,int sub)
{
	_xorsumline_init(p, DRAW_OPTYPE_XOR_SUMLINE, sub);

	p->w.func_press_in_grab = _xorsumline_press_in_grab;
	p->w.func_action = _xorsumline_action;

	//描画開始
	
	drawOpSub_setDrawInfo(p, p->w.optoolno, 0);
	drawOpSub_beginDraw(p);

	return TRUE;
}


//=================================
// 多角形
//=================================
/*
	確定した線は XOR 用イメージ (tileimg_tmp) に描画。
	現在の直線は、キャンバスに直接 XOR 描画。
	作業値は連続直線/集中線と同じ。

	boxtmp[0] : XOR 描画用範囲
*/


/* 2回目以降の左ボタン押し時 */

static void _xorpolygon_press_in_grab(AppDraw *p,uint32_t state)
{
	mDoublePoint pt;

	//ポイント追加

	drawCalc_canvas_to_image_double(p, &pt.x, &pt.y, p->w.pttmp[1].x, p->w.pttmp[1].y);

	FillPolygon_addPoint(p->w.fillpolygon, pt.x, pt.y);

	//XOR

	drawOpXor_drawline(p);
	drawOpXor_drawPolygon(p);

	TileImage_drawLineB(p->tileimg_tmp,
		p->w.pttmp[0].x, p->w.pttmp[0].y,
		p->w.pttmp[1].x, p->w.pttmp[1].y, &p->w.drawcol, FALSE);

	drawOpXor_drawPolygon(p);

	p->w.pttmp[0] = p->w.pttmp[1];
	
	drawOpXor_drawline(p);
}

/* 各アクション (操作終了) */

static mlkbool _xorpolygon_action(AppDraw *p,int action)
{
	//BackSpace 無効

	if(action == DRAW_FUNC_ACTION_KEY_BACKSPACE) return FALSE;

	//XOR 消去

	drawOpXor_drawline(p);
	drawOpXor_drawPolygon(p);

	drawOpSub_freeTmpImage(p);

	//描画 (ESC はキャンセル)

	if(action != DRAW_FUNC_ACTION_KEY_ESC
		&& action != DRAW_FUNC_ACTION_UNGRAB)
	{
		if(FillPolygon_closePoint(p->w.fillpolygon))
		{
			switch(p->w.optype_sub)
			{
				//多角形塗りつぶし
				case DRAW_OPSUB_DRAW_FILL:
					drawOpDraw_fillPolygon(p);
					break;
				//選択範囲セット
				case DRAW_OPSUB_SET_SELECT:
					drawOp_setSelect(p, 1);
					break;
				//スタンプイメージセット
				case DRAW_OPSUB_SET_STAMP:
					drawOp_setStampImage(p);
					break;
			}
		}
	}

	drawOpSub_freeFillPolygon(p);

	return TRUE;
}

/* 多角形初期化 (投げ縄と共通) */

static mlkbool _fillpolygon_init(AppDraw *p)
{
	mDoublePoint pt;

	if(!drawOpSub_createTmpImage_canvas(p))
		return FALSE;

	//FillPolygon 作成

	if(!(p->w.fillpolygon = FillPolygon_new()))
	{
		drawOpSub_freeTmpImage(p);
		return FALSE;
	}

	//現在位置追加

	drawOpSub_getImagePoint_double(p, &pt);

	FillPolygon_addPoint(p->w.fillpolygon, pt.x, pt.y);

	//

	g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new;

	p->w.drawcol = (uint64_t)-1;

	return TRUE;
}

/** 最初の押し時 */

mlkbool drawOpXor_polygon_press(AppDraw *p,int sub)
{
	if(!_fillpolygon_init(p)) return FALSE;

	//XOR 更新範囲

	p->w.boxtmp[0].x = p->w.boxtmp[0].y = 0;
	p->w.boxtmp[0].w = p->canvas_size.w;
	p->w.boxtmp[0].h = p->canvas_size.h;

	//

	_xorsumline_init(p, DRAW_OPTYPE_XOR_POLYGON, sub);

	p->w.func_press_in_grab = _xorpolygon_press_in_grab;
	p->w.func_action = _xorpolygon_action;

	return TRUE;
}


//=============================
// XOR 投げ縄
//=============================
/*
	pttmp[0] : 線描画用、現在の位置
	pttmp[1] : 前回の位置
*/


/* 移動 */

static void _xorlasso_motion(AppDraw *p,uint32_t state)
{
	mDoublePoint pt;

	p->w.pttmp[1] = p->w.pttmp[0];

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	drawOpXor_drawLasso(p, FALSE);

	//ポイント追加

	drawOpSub_getImagePoint_double(p, &pt);
	
	FillPolygon_addPoint(p->w.fillpolygon, pt.x, pt.y);
}

/* 離し */

static mlkbool _xorlasso_release(AppDraw *p)
{
	drawOpXor_drawLasso(p, TRUE);
	drawOpSub_freeTmpImage(p);

	if(FillPolygon_closePoint(p->w.fillpolygon))
	{
		switch(p->w.optype_sub)
		{
			//塗りつぶし
			case DRAW_OPSUB_DRAW_FILL:
				drawOpDraw_fillPolygon(p);
				break;
			//選択範囲セット
			case DRAW_OPSUB_SET_SELECT:
				drawOp_setSelect(p, 1);
				break;
			//スタンプイメージセット
			case DRAW_OPSUB_SET_STAMP:
				drawOp_setStampImage(p);
				break;
		}
	}

	drawOpSub_freeFillPolygon(p);

	return TRUE;
}

/* 各アクション (キーなどの操作は無効) */

static mlkbool _xorlasso_action(AppDraw *p,int action)
{
	if(action == DRAW_FUNC_ACTION_UNGRAB)
		return _xorlasso_release(p);
	else
		return FALSE;
}

/** 押し時 */

mlkbool drawOpXor_lasso_press(AppDraw *p,int sub)
{
	if(!_fillpolygon_init(p)) return FALSE;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_XOR_LASSO,
		_xorlasso_motion, _xorlasso_release, sub);

	p->w.func_action = _xorlasso_action;

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	drawOpSub_copyTmpPoint_0toN(p, 2);

	drawOpXor_drawLasso(p, FALSE);

	return TRUE;
}


//=========================================
// 定規の位置設定 (集中線/同心円[正円])
//=========================================
/*
	pttmp[0] : 押し位置 (キャンバス座標)
*/


/* 離し */

static mlkbool _rulepoint_release(AppDraw *p)
{
	drawOpXor_drawCrossPoint(p);

	drawRule_setting_point(p);

	return TRUE;
}

/** 押し */

mlkbool drawOpXor_rulepoint_press(AppDraw *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		NULL, _rulepoint_release, 0);

	drawOpSub_getCanvasPoint_int(p, p->w.pttmp);

	drawOpXor_drawCrossPoint(p);

	return TRUE;
}


/********************************
 * XOR 描画
 ********************************/


/** XOR 直線描画
 *
 * pttmp[0]: 始点
 * pttmp[1]: 終点 */

void drawOpXor_drawline(AppDraw *p)
{
	mPixbuf *pixbuf;
	mBox box;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		mPixbufLine(pixbuf,
			p->w.pttmp[0].x, p->w.pttmp[0].y,
			p->w.pttmp[1].x, p->w.pttmp[1].y, 0);

		mBoxSetPoint_minmax(&box, p->w.pttmp, p->w.pttmp + 1);

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

/** XOR 四角形描画 (キャンバス) */

void drawOpXor_drawRect_canv(AppDraw *p)
{
	mPixbuf *pixbuf;
	mBox box;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		box.x = p->w.pttmp[0].x;
		box.y = p->w.pttmp[0].y;
		box.w = p->w.pttmp[1].x - p->w.pttmp[0].x + 1;
		box.h = p->w.pttmp[1].y - p->w.pttmp[0].y + 1;

		mPixbufBox_pixel(pixbuf, box.x, box.y, box.w, box.h, 0);

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

/** XOR 四角形描画 (イメージに対する) */

void drawOpXor_drawRect_image(AppDraw *p)
{
	mPixbuf *pixbuf;
	int x1,y1,x2,y2,i;
	mPoint pt[5];
	mBox box;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		x1 = p->w.pttmp[0].x, y1 = p->w.pttmp[0].y;
		x2 = p->w.pttmp[1].x, y2 = p->w.pttmp[1].y;

		if(p->canvas_zoom != 1000)
			x2++, y2++;

		//イメージ -> 領域座標

		drawCalc_image_to_canvas_pt(p, pt, x1, y1);
		drawCalc_image_to_canvas_pt(p, pt + 1, x2, y1);
		drawCalc_image_to_canvas_pt(p, pt + 2, x2, y2);
		drawCalc_image_to_canvas_pt(p, pt + 3, x1, y2);
		pt[4] = pt[0];

		//更新範囲

		mBoxSetPoints(&box, pt, 4);

		//描画

		if(x1 == x2 && y1 == y2)
			//1点
			mPixbufSetPixel(pixbuf, pt[0].x, pt[0].y, 0);
		else if(x1 == x2 || y1 == y2)
			//水平/垂直線
			mPixbufLine(pixbuf, pt[0].x, pt[0].y, pt[2].x, pt[2].y, 0);
		else
		{
			//四角形
			for(i = 0; i < 4; i++)
				mPixbufLine_excludeEnd(pixbuf, pt[i].x, pt[i].y, pt[i + 1].x, pt[i + 1].y, 0);
		}

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

/** XOR 円描画
 *
 * rctmp[0]: 矩形での位置 */

void drawOpXor_drawEllipse(AppDraw *p)
{
	mPixbuf *pixbuf;
	mBox box;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		mPixbufEllipse(pixbuf,
			p->w.rctmp[0].x1, p->w.rctmp[0].y1,
			p->w.rctmp[0].x2, p->w.rctmp[0].y2, 0);

		mBoxSetRect(&box, p->w.rctmp);

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

/** XOR ベジェ曲線描画 */

void drawOpXor_drawBezier(AppDraw *p,mlkbool erase)
{
	mPixbuf *pixbuf;

	//描画

	if(!erase)
	{
		TileImage_freeAllTiles(p->tileimg_tmp);

		TileImageDrawInfo_clearDrawRect();
		TileImage_drawBezier_forXor(p->tileimg_tmp, p->w.pttmp, p->w.optype_sub);

		drawCalc_clipCanvas_toBox(p, p->w.boxtmp, &g_tileimage_dinfo.rcdraw);
	}

	//XOR

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		TileImage_blendXor_pixbuf(p->tileimg_tmp, pixbuf, p->w.boxtmp);

		drawOpSub_endCanvasDraw(pixbuf, p->w.boxtmp);
	}
}

/** XOR 多角形描画 */

void drawOpXor_drawPolygon(AppDraw *p)
{
	mPixbuf *pixbuf;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		TileImage_blendXor_pixbuf(p->tileimg_tmp, pixbuf, p->w.boxtmp);

		drawOpSub_endCanvasDraw(pixbuf, p->w.boxtmp);
	}
}

/** XOR 投げ縄描画 */

void drawOpXor_drawLasso(AppDraw *p,mlkbool erase)
{
	mPixbuf *pixbuf;
	mBox box;
	mRect rc;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		if(erase)
		{
			//消去

			box.x = box.y = 0;
			box.w = p->canvas_size.w;
			box.h = p->canvas_size.h;

			TileImage_blendXor_pixbuf(p->tileimg_tmp, pixbuf, &box);
		}
		else
		{
			//新しい線描画

			mRectSetPoint_minmax(&rc, p->w.pttmp, p->w.pttmp + 1);

			if(drawCalc_clipCanvas_toBox(p, &box, &rc))
			{
				TileImage_blendXor_pixbuf(p->tileimg_tmp, pixbuf, &box);

				TileImage_drawLineB(p->tileimg_tmp,
					p->w.pttmp[1].x, p->w.pttmp[1].y,
					p->w.pttmp[0].x, p->w.pttmp[0].y, &p->w.drawcol, FALSE);

				TileImage_blendXor_pixbuf(p->tileimg_tmp, pixbuf, &box);
			}
		}

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

/** XOR 十字線描画 */

void drawOpXor_drawCrossPoint(AppDraw *p)
{
	mPixbuf *pixbuf;
	int x,y;
	mBox box;

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		x = p->w.pttmp[0].x;
		y = p->w.pttmp[0].y;
	
		mPixbufLineH(pixbuf, x - 9, y, 19, 0);
		mPixbufLineV(pixbuf, x, y - 9, 19, 0);

		box.x = x - 9, box.y = y - 9;
		box.w = box.h = 19;

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

/** XOR ブラシサイズ円描画
 *
 * pttmp[0] : 中心位置
 * ntmp[0]  : 直径サイズ */

void drawOpXor_drawBrushSizeCircle(AppDraw *p,mlkbool erase)
{
	mPixbuf *pixbuf;
	mBox box;
	int r,size;

	size = p->w.ntmp[0];
	if(size == 0) size = 1;

	if(erase)
		box = p->w.boxtmp[0];
	else
	{
		r = size / 2;
		
		box.x = p->w.pttmp[0].x - r;
		box.y = p->w.pttmp[0].y - r;
		
		box.w = box.h = size;

		p->w.boxtmp[0] = box;
	}

	//描画

	if((pixbuf = drawOpSub_beginCanvasDraw()))
	{
		mPixbufEllipse(pixbuf,
			box.x, box.y, box.x + box.w - 1, box.y + box.h - 1, 0);

		drawOpSub_endCanvasDraw(pixbuf, &box);
	}
}

