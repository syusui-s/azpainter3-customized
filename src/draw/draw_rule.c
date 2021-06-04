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
 * AppDraw: 定規処理
 **********************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_rectbox.h"
#include "mlk_pixbuf.h"

#include "drawpixbuf.h"

#include "def_config.h"
#include "def_draw.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_rule.h"
#include "draw_op_def.h"
#include "draw_op_func.h"
#include "draw_op_sub.h"


//==================================
// sub
//==================================


/* ガイドを設定して、キャンバス更新 */

static void _update_set_guide(AppDraw *p)
{
	if(p->rule.func_set_guide)
		(p->rule.func_set_guide)(p);

	if(drawRule_isVisibleGuide(p))
		drawUpdate_canvas();
}

/* ボタン押し時のイメージ位置取得 (ドットの中心) */

static void _get_press_point_dot(AppDraw *p,mDoublePoint *dst)
{
	drawCalc_canvas_to_image_double(p, &dst->x, &dst->y,
		p->w.dpt_canv_press.x, p->w.dpt_canv_press.y);

	dst->x = floor(dst->x) + 0.5;
	dst->y = floor(dst->y) + 0.5;
}

/* ガイド描画用の sin,cos 取得
 *
 * return: キャンバス回転を適用したラジアン角度 */

static double _get_guide_sincos(AppDraw *p,double rd,double *dcos,double *dsin)
{
	if(p->canvas_mirror) rd = -rd;
	
	rd += p->viewparam.rd;

	*dcos = cos(rd);
	*dsin = sin(rd);

	return rd;
}

/* イメージ位置(mDoublePoint)から、ガイド表示用のキャンバス位置取得 */

static void _get_guide_point(AppDraw *p,mPoint *dst,const mDoublePoint *ptd)
{
	drawCalc_image_to_canvas_pt_double(p, dst, ptd->x, ptd->y);
}

/* キャンバス上の位置 (x,y) を中心として、角度(dcos,dsin) の線の始点と終点を取得
 *
 * len: 半径の長さ */

static void _get_guide_line_len(AppDraw *p,mRect *dst,int x,int y,double dcos,double dsin,int len)
{
	dst->x1 = x + len * dcos;
	dst->y1 = y + len * dsin;

	dst->x2 = x + len * -dcos;
	dst->y2 = y + len * -dsin;
}

/* 角度 rd の sin,cos 値を dtmp[0,1] にセット */

static void _set_tmp_sincos(AppDraw *p,double rd)
{
	p->rule.dtmp[0] = cos(rd);
	p->rule.dtmp[1] = sin(rd);
}

/* キャンバス上の位置 (x,y) を通る角度(dcos,dsin)の直線を、
 * キャンバスに描画した時の始点と終点を取得。 */

static void _get_guide_line_pos(AppDraw *p,mRect *dst,int x,int y,double dcos,double dsin)
{
	int w,h;
	double len;

	w = p->imgw * p->viewparam.scale;
	h = p->imgh * p->viewparam.scale;

	if(w < p->canvas_size.w) w = p->canvas_size.w;
	if(h < p->canvas_size.h) h = p->canvas_size.w;

	len = (w < h)? h: w;

	dst->x1 = x + len * dcos;
	dst->y1 = y + len * dsin;

	dst->x2 = x + len * -dcos;
	dst->y2 = y + len * -dsin;
}


//==================================
// ガイド描画
//==================================


/* 平行線/グリッド */

static void _draw_guide_lines(AppDraw *p,mPixbuf *pixbuf,const mBox *box)
{
	int i,num;
	mRect rcclip,*prc;

	mRectSetBox(&rcclip, box);

	num = (p->rule.type == DRAW_RULE_TYPE_LINE)? 1: 2;

	for(i = 0; i < num; i++)
	{
		prc = p->rule.rc_guide + i;

		drawpixbuf_line_clip(pixbuf, prc->x1, prc->y1, prc->x2, prc->y2,
			APPCONF->rule_guide_col, &rcclip);
	}
}

/* 集中線 */

static void _draw_guide_conc(AppDraw *p,mPixbuf *pixbuf,const mBox *box)
{
	mPoint pt;

	if(mPixbufClip_setBox(pixbuf, box))
	{
		_get_guide_point(p, &pt, &p->rule.dpt_conc);

		drawpixbuf_crossline_blend(pixbuf, pt.x, pt.y, 80, APPCONF->rule_guide_col);
	}
}

/* 同心円(正円) */

static void _draw_guide_circle(AppDraw *p,mPixbuf *pixbuf,const mBox *box)
{
	mPoint pt;

	if(mPixbufClip_setBox(pixbuf, box))
	{
		_get_guide_point(p, &pt, &p->rule.dpt_circle);

		drawpixbuf_crossline_blend(pixbuf, pt.x, pt.y, 50, APPCONF->rule_guide_col);
	}
}

/* 同心円(楕円) */

static void _draw_guide_ellipse(AppDraw *p,mPixbuf *pixbuf,const mBox *box)
{
	mPoint pt;

	if(mPixbufClip_setBox(pixbuf, box))
	{
		//十字
		
		_get_guide_point(p, &pt, &p->rule.dpt_ellipse);

		drawpixbuf_crossline_blend(pixbuf, pt.x, pt.y, 10, APPCONF->rule_guide_col);

		//楕円

		drawpixbuf_ellipse_blend(pixbuf, pt.x, pt.y,
			p->rule.dtmp_guide, p->rule.ellipse_yx, APPCONF->rule_guide_col);
	}
}

/* 線対称 */

static void _draw_guide_symm(AppDraw *p,mPixbuf *pixbuf,const mBox *box)
{
	mRect rcclip,*prc;

	mRectSetBox(&rcclip, box);

	prc = p->rule.rc_guide;

	drawpixbuf_line_clip(pixbuf, prc->x1, prc->y1, prc->x2, prc->y2,
		APPCONF->rule_guide_col, &rcclip);
}


//==================================
// ガイド情報セット
//==================================


/* 平行線 */

static void _set_guide_line(AppDraw *p)
{
	double dcos,dsin;

	_get_guide_sincos(p, p->rule.line_rd, &dcos, &dsin);

	_get_guide_line_len(p, p->rule.rc_guide,
		p->canvas_size.w / 2, p->canvas_size.h / 2,
		dcos, dsin, 200);
}

/* グリッド */

static void _set_guide_grid(AppDraw *p)
{
	double dcos,dsin;
	int cx,cy;

	cx = p->canvas_size.w / 2;
	cy = p->canvas_size.h / 2;

	//0度

	_get_guide_sincos(p, p->rule.grid_rd, &dcos, &dsin);

	_get_guide_line_len(p, p->rule.rc_guide, cx, cy, dcos, dsin, 200);

	//90度

	_get_guide_sincos(p, p->rule.grid_rd + MLK_MATH_PI / 2, &dcos, &dsin);
	
	_get_guide_line_len(p, p->rule.rc_guide + 1, cx, cy, dcos, dsin, 200);
}

/* 楕円 */

static void _set_guide_ellipse(AppDraw *p)
{
	double rd;

	rd = p->rule.ellipse_rd;

	if(p->canvas_mirror)
		rd = -rd;

	p->rule.dtmp_guide = p->viewparam.rd - rd;
}

/* 線対称 */

static void _set_guide_symm(AppDraw *p)
{
	mPoint pt;
	double dcos,dsin;

	_get_guide_point(p, &pt, &p->rule.dpt_symm);

	_get_guide_sincos(p, p->rule.symm_rd, &dcos, &dsin);

	_get_guide_line_pos(p, p->rule.rc_guide, pt.x, pt.y, dcos, dsin);
}


//==================================
// 補正位置取得
//==================================


/* 平行線/グリッド/集中線
 *
 * dtmp[0]: 指定角度の cos
 * dtmp[1]: sin */

static void _get_point_line(AppDraw *p,double *px,double *py,int no)
{
	AppDrawRule *rule = &p->rule;
	double xx,yy,vi,len,dcos,dsin;

	dcos = rule->dtmp[0];
	dsin = rule->dtmp[1];

	//描画開始位置を中心

	xx = *px - rule->dpt_press.x;
	yy = *py - rule->dpt_press.y;

	//長さ

	len = sqrt(xx * xx + yy * yy);

	//ベクトルの内積 = cos の値
	// (v1x * v2x + v1y * v2y) / (v1len * v2len)
	// v2 は長さ 1 の指定角度の線とするので、v2x = cos, v2y = sin

	if(len < MLK_MATH_DBL_EPSILON)
		vi = 0;
	else
		vi = (xx * dcos + yy * dsin) / len;

	//[グリッド]
	// 長さが一定になるまで、押し位置のまま移動しない。
	// 長さが一定まで来たら、角度を計算。

	if(rule->type == DRAW_RULE_TYPE_GRID && !rule->ntmp)
	{
		if(len >= 3)
		{
			if(vi > -0.7 && vi < 0.7)
				_set_tmp_sincos(p, rule->grid_rd + MLK_MATH_PI / 2);
			
			rule->ntmp = 1;
		}

		*px = rule->dpt_press.x;
		*py = rule->dpt_press.y;

		return;
	}

	//内積が負なら 180 度反転

	if(vi < 0)
		dcos = -dcos, dsin = -dsin;

	//

	*px = len * dcos + rule->dpt_press.x;
	*py = len * dsin + rule->dpt_press.y;
}

/* 同心円(正円) */

static void _get_point_circle(AppDraw *p,double *px,double *py,int no)
{
	double rd;

	rd = atan2(*py - p->rule.dpt_circle.y, *px - p->rule.dpt_circle.x);

	*px = p->rule.dtmp[0] * cos(rd) + p->rule.dpt_circle.x;
	*py = p->rule.dtmp[0] * sin(rd) + p->rule.dpt_circle.y;
}

/* 同心円(楕円) */

static void _get_point_ellipse(AppDraw *p,double *px,double *py,int no)
{
	AppDrawRule *rule = &p->rule;
	double xx,yy,x,y,rd,dcos,dsin;

	dcos = rule->dtmp[0];
	dsin = rule->dtmp[1];

	//正円の位置から、角度取得

	xx = *px - rule->dpt_ellipse.x;
	yy = *py - rule->dpt_ellipse.y;

	x = (xx * dcos - yy * dsin) * rule->ellipse_yx;
	y = xx * dsin + yy * dcos;

	rd = atan2(y, x);

	//楕円回転

	xx = rule->dtmp[2] / rule->ellipse_yx * cos(rd);
	yy = rule->dtmp[2] * sin(rd);

	x = xx * dcos + yy * dsin;
	y = -xx * dsin + yy * dcos;

	*px = x + rule->dpt_ellipse.x;
	*py = y + rule->dpt_ellipse.y;
}

/* 線対称 */

static void _get_point_symm(AppDraw *p,double *px,double *py,int no)
{
	double x,y,xx,yy,dcos,dsin;

	//最初の点は補正なし
	if(no == 0) return;

	dcos = p->rule.dtmp[0];
	dsin = p->rule.dtmp[1];

	//逆回転し、y 反転

	xx = *px - p->rule.dpt_symm.x;
	yy = *py - p->rule.dpt_symm.y;

	x = xx * dcos + yy * dsin;
	y = -(xx * -dsin + yy * dcos);

	//再回転

	*px = x * dcos - y * dsin + p->rule.dpt_symm.x;
	*py = x * dsin + y * dcos + p->rule.dpt_symm.y;
}


//========================
// main
//========================


//ガイド描画
static void (*g_funcs_drawguide[])(AppDraw *,mPixbuf *,const mBox *) = {
	NULL, _draw_guide_lines, _draw_guide_lines, _draw_guide_conc, _draw_guide_circle,
	_draw_guide_ellipse, _draw_guide_symm
};

//ガイドセット
static void (*g_funcs_setguide[])(AppDraw *) = {
	NULL, _set_guide_line, _set_guide_grid, NULL, NULL,
	_set_guide_ellipse, _set_guide_symm
};

//位置補正
static void (*g_funcs_getpoint[])(AppDraw *p,double *,double *,int) = {
	NULL, _get_point_line, _get_point_line, _get_point_line, _get_point_circle,
	_get_point_ellipse, _get_point_symm
};


/** 定規タイプ変更 */

void drawRule_setType(AppDraw *p,int type)
{
	int f1,f2;
	
	if(p->rule.type == type) return;

	//非対応のツール時にも来るので、表示条件を確認

	f1 = drawRule_isVisibleGuide(p);

	//

	p->rule.type = type;

	p->rule.drawpoint_num = (type == DRAW_RULE_TYPE_SYMMETRY)? 2: 1;

	p->rule.func_draw_guide = g_funcs_drawguide[type];
	p->rule.func_set_guide = g_funcs_setguide[type];
	p->rule.func_get_point = g_funcs_getpoint[type];

	//更新
	// :ツール変更などによって表示が切り替わる時、
	// :または、現在表示状態で定規タイプが変わった時。

	f2 = drawRule_isVisibleGuide(p);

	if(p->rule.func_set_guide)
		(p->rule.func_set_guide)(p);

	if(f2 || f1 != f2)
		drawUpdate_canvas();
}

/** キャンバスに定規ガイドを描画するか */

mlkbool drawRule_isVisibleGuide(AppDraw *p)
{
	return (p->rule.func_draw_guide
		&& p->tool.no <= TOOL_FINGER
		&& p->tool.subno[p->tool.no] == TOOLSUB_DRAW_FREE
		&& (APPCONF->fview & CONFIG_VIEW_F_RULE_GUIDE));
}

/** 自由線描画時、ボタン押し */

void drawRule_onPress(AppDraw *p,mlkbool dotpen)
{
	mDoublePoint dpt;
	double x,y,xx,yy;

	//押し時のイメージ位置

	drawOpSub_getImagePoint_double(p, &dpt);

	if(dotpen)
	{
		dpt.x = floor(dpt.x) + 0.5;
		dpt.y = floor(dpt.y) + 0.5;
	}

	p->rule.dpt_press = dpt;

	//各タイプ

	switch(p->rule.type)
	{
		//平行線
		case DRAW_RULE_TYPE_LINE:
			_set_tmp_sincos(p, p->rule.line_rd);
			break;
		//グリッド
		// :ntmp = 90度の角度か
		case DRAW_RULE_TYPE_GRID:
			p->rule.ntmp = 0;
			_set_tmp_sincos(p, p->rule.grid_rd);
			break;
		//集中線
		case DRAW_RULE_TYPE_CONCLINE:
			_set_tmp_sincos(p,
				atan2(dpt.y - p->rule.dpt_conc.y, dpt.x - p->rule.dpt_conc.x));
			break;

		//正円
		// :dtmp[0] = 半径
		case DRAW_RULE_TYPE_CIRCLE:
			x = dpt.x - p->rule.dpt_circle.x;
			y = dpt.y - p->rule.dpt_circle.y;

			p->rule.dtmp[0] = sqrt(x * x + y * y);
			break;

		//楕円
		// :dtmp[0] = dcos
		// :dtmp[1] = dsin
		// :dtmp[2] = 半径
		case DRAW_RULE_TYPE_ELLIPSE:
			_set_tmp_sincos(p, p->rule.ellipse_rd);
		
			x = dpt.x - p->rule.dpt_ellipse.x;
			y = dpt.y - p->rule.dpt_ellipse.y;

			xx = (x * p->rule.dtmp[0] - y * p->rule.dtmp[1]) * p->rule.ellipse_yx;
			yy = x * p->rule.dtmp[1] + y * p->rule.dtmp[0];

			p->rule.dtmp[2] = sqrt(xx * xx + yy * yy);
			break;

		//線対称
		case DRAW_RULE_TYPE_SYMMETRY:
			_set_tmp_sincos(p, p->rule.symm_rd);
			break;
	}
}

/** (ボタン押し時) 定規の設定開始
 *
 * return: グラブするか */

mlkbool drawRule_onPress_setting(AppDraw *p)
{
	switch(p->rule.type)
	{
		//集中線/同心円(正円)
		case DRAW_RULE_TYPE_CONCLINE:
		case DRAW_RULE_TYPE_CIRCLE:
			return drawOpXor_rulepoint_press(p);

		//同心円(楕円)
		case DRAW_RULE_TYPE_ELLIPSE:
			return drawOpXor_ellipse_press(p, DRAW_OPSUB_RULE_SETTING);

		//平行線/グリッド/線対称
		default:
			return drawOpXor_line_press(p, DRAW_OPSUB_RULE_SETTING);
	}
}


//=========================
// パラメータ設定
//=========================


/** XOR後: 平行線/グリッド/線対称 */

void drawRule_setting_line(AppDraw *p)
{
	double rd;

	//線の角度

	rd = drawCalc_getLineRadian_forImage(p);

	switch(p->rule.type)
	{
		//平行線
		case DRAW_RULE_TYPE_LINE:
			p->rule.line_rd = rd;
			break;
		//グリッド
		case DRAW_RULE_TYPE_GRID:
			p->rule.grid_rd = rd;
			break;
		//線対称
		default:
			_get_press_point_dot(p, &p->rule.dpt_symm);

			p->rule.symm_rd = rd;
			break;
	}

	_update_set_guide(p);
}

/** XOR後: 集中線/同心円(正円) */

void drawRule_setting_point(AppDraw *p)
{
	switch(p->rule.type)
	{
		//集中線
		case DRAW_RULE_TYPE_CONCLINE:
			_get_press_point_dot(p, &p->rule.dpt_conc);
			break;
		//同心円(正円)
		case DRAW_RULE_TYPE_CIRCLE:
			_get_press_point_dot(p, &p->rule.dpt_circle);
			break;
	}
	
	_update_set_guide(p);
}

/** XOR後: 同心円(楕円) */

void drawRule_setting_ellipse(AppDraw *p)
{
	AppDrawRule *rule = &p->rule;
	double rx,ry;

	//中心位置

	_get_press_point_dot(p, &rule->dpt_ellipse);

	//半径

	rx = p->w.pttmp[1].x * p->viewparam.scalediv;
	ry = p->w.pttmp[1].y * p->viewparam.scalediv;

	if(rx < 0.01) rx = 0.01;
	if(ry < 0.01) ry = 0.01;

	//パラメータ

	rule->ellipse_yx = ry / rx;
	rule->ellipse_rd = p->viewparam.rd;

	if(p->canvas_mirror)
		rule->ellipse_rd = -rule->ellipse_rd;

	//

	_update_set_guide(p);
}


//==========================
// 登録
//==========================


/** 現在のデータを記録する */

void drawRule_setRecord(AppDraw *p,int no)
{
	RuleRecord *pd;
	AppDrawRule *rule = &p->rule;

	pd = p->rule.record + no;

	pd->type = rule->type;

	switch(rule->type)
	{
		case DRAW_RULE_TYPE_OFF:
			break;
		case DRAW_RULE_TYPE_LINE:
			pd->dval[0] = rule->line_rd;
			break;
		case DRAW_RULE_TYPE_GRID:
			pd->dval[0] = rule->grid_rd;
			break;
		case DRAW_RULE_TYPE_CONCLINE:
			pd->dval[0] = rule->dpt_conc.x;
			pd->dval[1] = rule->dpt_conc.y;
			break;
		case DRAW_RULE_TYPE_CIRCLE:
			pd->dval[0] = rule->dpt_circle.x;
			pd->dval[1] = rule->dpt_circle.y;
			break;
		case DRAW_RULE_TYPE_ELLIPSE:
			pd->dval[0] = rule->dpt_ellipse.x;
			pd->dval[1] = rule->dpt_ellipse.y;
			pd->dval[2] = rule->ellipse_rd;
			pd->dval[3] = rule->ellipse_yx;
			break;
		case DRAW_RULE_TYPE_SYMMETRY:
			pd->dval[0] = rule->dpt_symm.x;
			pd->dval[1] = rule->dpt_symm.y;
			pd->dval[2] = rule->symm_rd;
			break;
	}
}

/** 記録データを読込 */

void drawRule_readRecord(AppDraw *p,int no)
{
	RuleRecord *ps;
	AppDrawRule *rule = &p->rule;

	ps = p->rule.record + no;

	switch(ps->type)
	{
		case DRAW_RULE_TYPE_OFF:
			break;
		case DRAW_RULE_TYPE_LINE:
			rule->line_rd = ps->dval[0];
			break;
		case DRAW_RULE_TYPE_GRID:
			rule->grid_rd = ps->dval[0];
			break;
		case DRAW_RULE_TYPE_CONCLINE:
			rule->dpt_conc.x = ps->dval[0];
			rule->dpt_conc.y = ps->dval[1];
			break;
		case DRAW_RULE_TYPE_CIRCLE:
			rule->dpt_circle.x = ps->dval[0];
			rule->dpt_circle.y = ps->dval[1];
			break;
		case DRAW_RULE_TYPE_ELLIPSE:
			rule->dpt_ellipse.x = ps->dval[0];
			rule->dpt_ellipse.y = ps->dval[1];
			rule->ellipse_rd = ps->dval[2];
			rule->ellipse_yx = ps->dval[3];
			break;
		case DRAW_RULE_TYPE_SYMMETRY:
			rule->dpt_symm.x = ps->dval[0];
			rule->dpt_symm.y = ps->dval[1];
			rule->symm_rd = ps->dval[2];
			break;
	}

	drawRule_setType(p, ps->type);
}

