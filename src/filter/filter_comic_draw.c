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

/**************************************
 * フィルタ処理: 漫画用、描画
 **************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rand.h"

#include "def_filterdraw.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "def_tileimage.h"
#include "def_brushdraw.h"
#include "canvasinfo.h"

#include "pv_filter_sub.h"


/*
  - キャンバスにイメージを挿入して、プレビューする。
  - プレビュー画像は１色でしか描画されないため、
    imgdst はアルファ値のみのタイプで作成されている。
  - 点描画関数は、プレビュー時、TileImage_setPixel_work にセットされている。
    (描画範囲が必要)
*/

//------------------

enum
{
	_DRAWTYPE_BRUSH_AA,
	_DRAWTYPE_BRUSH_NOAA,
	_DRAWTYPE_DOT_1PX
};

//------------------


//=======================
// sub
//=======================


/* 描画色取得 & プレビュー画像のレイヤ色セット */

static void _get_draw_color(FilterDrawInfo *info,void *dst)
{
	RGBA8 *pd8;
	RGBA16 *pd16;

	//プレビュー時、imgdst はアルファ値のみのため、レイヤ色をセット

	if(info->in_dialog && info->val_ckbtt[1])
	{
		//プレビュー時、赤色でプレビュー

		if(info->bits == 8)
		{
			pd8 = (RGBA8 *)dst;

			pd8->r = pd8->a = 255;
			pd8->g = pd8->b = 0;
		}
		else
		{
			pd16 = (RGBA16 *)dst;

			pd16->r = pd16->a = COLVAL_16BIT;
			pd16->g = pd16->b = 0;
		}

		if(info->in_dialog)
			RGB32bit_to_RGBcombo(&info->imgdst->col, 0xff0000);
	}
	else
	{
		//実際の描画 or 通常プレビュー時 (描画色)

		FilterSub_getDrawColor_type(info, DRAWCOL_TYPE_DRAWCOL, dst);
		
		if(info->in_dialog)
			info->imgdst->col = info->rgb_drawcol;
	}
}

/* ブラシ描画パラメータの作成と初期化 */

static BrushDrawParam *_create_brushdrawparam(int drawtype)
{
	BrushDrawParam *p;

	p = (BrushDrawParam *)mMalloc0(sizeof(BrushDrawParam));
	if(!p) return NULL;

	g_tileimage_dinfo.brushdp = p;

	p->opacity = 1;
	p->interval = 0.4;
	p->pressure_min = 0;
	p->pressure_range = 1;
	p->flags = BRUSHDP_F_SHAPE_HARD_MAX;

	if(drawtype == _DRAWTYPE_BRUSH_NOAA)
		p->flags |= BRUSHDP_F_NO_ANTIALIAS;

	return p;
}


//=======================
// 共通描画
//=======================
/*
 * <type>
 *  0: 集中線/フラッシュ (円の外周から中心へ向けて)
 *  1: ベタフラッシュ (円の外周から外へ向けて&中を塗りつぶし)
 *  2: ウニフラッシュ
 *
 * <valbar>
 *  [0] 円の半径(px)
 *  [1] 円の縦横比 (-1.000〜+1.000)
 *  [2] 線の密度最小 (360度を分割する数÷2)
 *  [3] 間隔ランダム幅 (0-99, 角度ステップに対する%)
 *  [4] 線の半径(px)
 *  [5] 線の半径ランダム (px, 0.0-100.0)
 *  [6] 線の長さ (0.1-100.0, 円の半径に対する%)
 *  [7] 線の長さランダム幅 (0.0-100.0, 半径に対する%)
 *
 * <val_ckbtt>
 *  [0] 簡易プレビュー
 *  [1] 赤色でプレビュー
 * 
 * <描画タイプ>
 *  0: ブラシ/アンチエイリアスあり
 *  1: ブラシ/アンチエイリアスなし
 *  2: 1pxドット線 */


/* 集中線/フラッシュ描画 共通 */

static mlkbool _proc_draw_flash(FilterDrawInfo *info,int flash_type)
{
	int drawtype,angle,angle_step,angle_rnd,n;
	uint32_t headtail;
	uint64_t coldraw;
	mlkbool fprev_simple;
	double r,brush_size,brush_sizernd,line_len,line_lenrnd,xscale,yscale;
	double dtmp,dcos,dsin,xx,yy,x1,y1,x2,y2;
	mDoublePoint dpt_ct;
	BrushDrawParam *brushdp = NULL;
	TileImage *imgdst = info->imgdst;
	mRandSFMT *rand = info->rand;

	drawtype = info->val_combo[0];

	//ブラシ描画パラメータ

	if(drawtype != _DRAWTYPE_DOT_1PX)
	{
		brushdp = _create_brushdrawparam(drawtype);
		if(!brushdp) return FALSE;
	}

	//簡易プレビュー

	fprev_simple = (info->in_dialog && info->val_ckbtt[0]);

	//描画色

	_get_draw_color(info, &coldraw);

	g_tileimage_dinfo.drawcol = coldraw;

	//------ 準備

	dpt_ct.x = info->imgx;
	dpt_ct.y = info->imgy;
	r = info->val_bar[0];
	brush_size = info->val_bar[4] * 0.1;
	brush_sizernd = info->val_bar[5] * 0.1;
	line_len = r * (info->val_bar[6] * 0.001);
	line_lenrnd = r * (info->val_bar[7] * 0.001);
	angle_step = (360 << 12) / (info->val_bar[2] * 2);
	angle_rnd = (int)(angle_step * (info->val_bar[3] * 0.01) + 0.5);

	//円のX,Y倍率

	if(info->val_bar[1] < 0)
	{
		xscale = 1;
		yscale = 1.0 + info->val_bar[1] * 0.001;
	}
	else
	{
		xscale = 1.0 - info->val_bar[1] * 0.001;
		yscale = 1;
	}

	//入り抜き (ウニフラッシュの場合 50:50、他は 0:100)

	headtail = (flash_type == 2)? (500 << 16) | 500: 1000;

	//2値の場合、濃度は最大値で固定

	if(brushdp)
		brushdp->min_opacity = (drawtype != _DRAWTYPE_BRUSH_AA);

	//アンチエイリアスで通常描画の場合のみ色合成

	FilterSub_setPixelColFunc((drawtype == _DRAWTYPE_BRUSH_AA && !fprev_simple)?
		TILEIMAGE_PIXELCOL_NORMAL: TILEIMAGE_PIXELCOL_OVERWRITE);

	//-------- 描画

	FilterSub_prog_substep_begin_onestep(info, 50, info->val_bar[2] * 2);

	for(angle = 0; angle < (360 << 12); angle += angle_step)
	{
		//角度

		n = angle;
		
		if(info->val_bar[3])
			n += mRandSFMT_getIntRange(rand, 0, angle_rnd);

		dtmp = (double)n / (180 << 12) * MLK_MATH_PI;

		//楕円時は、正円における角度の線との交点位置となるように

		if(n % (90<<12))
		{
			dtmp = atan(tan(dtmp) * xscale / yscale);

			if(n > (90<<12) && n < (180<<12))
				dtmp += MLK_MATH_PI;
			else if(n > (180<<12) && n < (270<<12))
				dtmp -= MLK_MATH_PI;
		}

		dcos = cos(dtmp);
		dsin = sin(dtmp);

		//円の縁の位置

		xx = dpt_ct.x + r * xscale * dcos;
		yy = dpt_ct.y + r * yscale * dsin;

		//始点と終点

		switch(flash_type)
		{
			//集中線/フラッシュ
			// :縁から内側へ
			case 0:
				x1 = xx, y1 = yy;

				dtmp = r - line_len;
				if(info->val_bar[7])
					dtmp -= mRandSFMT_getDouble(rand) * line_lenrnd;

				x2 = dpt_ct.x + dtmp * xscale * dcos;
				y2 = dpt_ct.y + dtmp * yscale * dsin;
				break;
			//ベタフラッシュ
			// :縁から外側へ
			case 1:
				x1 = xx, y1 = yy;

				dtmp = r + line_len;
				if(info->val_bar[7])
					dtmp += mRandSFMT_getDouble(rand) * line_lenrnd;

				x2 = dpt_ct.x + dtmp * xscale * dcos;
				y2 = dpt_ct.y + dtmp * yscale * dsin;
				break;
			//ウニフラッシュ
			// :縁から両側へ (正円の状態で)
			default:
				dtmp = line_len;
				if(info->val_bar[7])
					dtmp += mRandSFMT_getDouble(rand) * line_lenrnd;

				dtmp *= 0.5;

				x1 = xx + -dtmp * dcos;
				y1 = yy + -dtmp * dsin;
				x2 = xx + dtmp * dcos;
				y2 = yy + dtmp * dsin;
				break;
		}

		//ブラシ半径

		if(brushdp)
		{
			dtmp = brush_size;

			if(info->val_bar[5])
				dtmp += mRandSFMT_getDouble(rand) * brush_sizernd;

			brushdp->radius = dtmp;
		}

		//描画

		if(drawtype == _DRAWTYPE_DOT_1PX)
		{
			//1pxドット線

			TileImage_drawLineB(imgdst, floor(x1), floor(y1), floor(x2), floor(y2), &coldraw, FALSE);
		}
		else if(!fprev_simple)
		{
			//ブラシ:通常描画

			TileImage_drawBrushLine_headtail(imgdst, x1, y1, x2, y2, headtail);
		}
		else
		{
			//ブラシ:簡易プレビュー

			if(dtmp <= 0.5)
			{
				TileImage_drawLineB(imgdst, floor(x1), floor(y1), floor(x2), floor(y2), &coldraw, FALSE);
			}
			else if(flash_type == 2)
			{
				//ウニフラッシュ

				xx = x1 + (x2 - x1) * 0.5;
				yy = y1 + (y2 - y1) * 0.5;

				dsin = dtmp * -dsin * xscale;
				dcos = dtmp * dcos * yscale;

				TileImage_drawLineB(imgdst, x1, y1, xx + dsin, yy + dcos, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, xx + dsin, yy + dcos, x2, y2, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, x2, y2, xx - dsin, yy - dcos, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, xx - dsin, yy - dcos, x1, y1, &coldraw, FALSE);
			}
			else
			{
				//集中線、フラッシュ

				dsin = dtmp * -dsin * xscale;
				dcos = dtmp * dcos * yscale;

				TileImage_drawLineB(imgdst, x2, y2, x1 + dsin, y1 + dcos, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, x1 + dsin, y1 + dcos, x1 - dsin, y1 - dcos, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, x1 - dsin, y1 - dcos, x2, y2, &coldraw, FALSE);
			}
		}

		FilterSub_prog_substep_inc(info);
	}

	//ベタフラッシュ、内側塗りつぶし

	if(flash_type == 1)
	{
		CanvasViewParam param;

		param.cos = param.cosrev = 1;
		param.sin = param.sinrev = 0;

		if(fprev_simple)
		{
			//dot
			TileImage_drawEllipse(imgdst, dpt_ct.x, dpt_ct.y, r * xscale, r * yscale,
				&coldraw, TRUE, &param, FALSE);
		}
		else
		{
			//AA
			TileImage_drawFillEllipse(imgdst, dpt_ct.x, dpt_ct.y, r * xscale, r * yscale,
				&coldraw, FALSE, &param, FALSE);
		}
	}

	mFree(brushdp);

	return TRUE;
}


//=======================
// フィルタ描画
//=======================


/** 集中線/フラッシュ */

mlkbool FilterDraw_comic_concline_flash(FilterDrawInfo *info)
{
	return _proc_draw_flash(info, 0);
}

/** ベタフラッシュ */

mlkbool FilterDraw_comic_popupflash(FilterDrawInfo *info)
{
	return _proc_draw_flash(info, 1);
}

/** ウニフラッシュ */

mlkbool FilterDraw_comic_uniflash(FilterDrawInfo *info)
{
	return _proc_draw_flash(info, 2);
}

/** ウニフラッシュ(波) */
/*
 * <valbar>
 * [0] 半径(px)
 * [1] 縦横比(-1.000〜+1.000)
 * [2] 波の密度 (360度の分割数)
 * [3] 波の長さ (半径に対する%)
 * [4] 線の太さ(px)
 * [5] 線の長さ(0.1-100.0, 半径に対する%)
 * [6] 抜きの太さ(%) */

mlkbool FilterDraw_comic_uniflash_wave(FilterDrawInfo *info)
{
	int drawtype,i;
	uint64_t coldraw;
	mlkbool fprev_simple;
	double r,line_len,angle_step,angle_step2,xscale,yscale;
	double angle,dtmp,dcos,dsin,tbl_radius[6],x1,y1,x2,y2;
	mDoublePoint dpt_ct;
	BrushDrawParam *brushdp = NULL;
	TileImage *imgdst = info->imgdst;

	drawtype = info->val_combo[0];

	//ブラシ描画パラメータ

	if(drawtype != _DRAWTYPE_DOT_1PX)
	{
		brushdp = _create_brushdrawparam(drawtype);
		if(!brushdp) return FALSE;
	}

	//簡易プレビュー

	fprev_simple = (info->in_dialog && info->val_ckbtt[0]);

	//描画色

	_get_draw_color(info, &coldraw);

	g_tileimage_dinfo.drawcol = coldraw;

	//------ 準備

	dpt_ct.x = info->imgx;
	dpt_ct.y = info->imgy;
	r = info->val_bar[0];
	line_len = r * (info->val_bar[5] * 0.001);
	angle_step = MLK_MATH_PI * 2 / info->val_bar[2];
	angle_step2 = angle_step / 6;

	//円のX,Y倍率

	if(info->val_bar[1] < 0)
	{
		xscale = 1;
		yscale = 1.0 + info->val_bar[1] * 0.001;
	}
	else
	{
		xscale = 1.0 - info->val_bar[1] * 0.001;
		yscale = 1;
	}

	//半径のテーブル

	dtmp = r * (info->val_bar[3] * 0.001);

	tbl_radius[0] = 0;
	tbl_radius[1] = dtmp / 3;
	tbl_radius[2] = dtmp * 2 / 3;
	tbl_radius[3] = dtmp;
	tbl_radius[4] = tbl_radius[2];
	tbl_radius[5] = tbl_radius[1];

	//描画情報

	if(brushdp)
	{
		brushdp->radius = info->val_bar[4] * 0.1;
		brushdp->min_size = info->val_bar[6] * 0.01;
		brushdp->min_opacity = 1;
	}

	FilterSub_setPixelColFunc((drawtype == _DRAWTYPE_BRUSH_AA && !fprev_simple)?
		TILEIMAGE_PIXELCOL_NORMAL: TILEIMAGE_PIXELCOL_OVERWRITE);

	//-------- 描画

	FilterSub_prog_substep_begin_onestep(info, 50, info->val_bar[2]);

	for(angle = 0; angle < MLK_MATH_PI * 2; angle += angle_step)
	{
		//1つの波に対して6個描画

		for(i = 0; i < 6; i++)
		{
			dtmp = angle + i * angle_step2;

			dcos = cos(dtmp) * xscale;
			dsin = sin(dtmp) * yscale;

			//線の位置

			dtmp = r + tbl_radius[i];

			x1 = dtmp * dcos + dpt_ct.x;
			y1 = dtmp * dsin + dpt_ct.y;

			dtmp -= line_len;

			x2 = dtmp * dcos + dpt_ct.x;
			y2 = dtmp * dsin + dpt_ct.y;

			//描画

			if(drawtype == _DRAWTYPE_DOT_1PX)
			{
				//1pxドット線
				TileImage_drawLineB(imgdst, floor(x1), floor(y1), floor(x2), floor(y2), &coldraw, FALSE);
			}
			else if(!fprev_simple)
			{
				//ブラシ:通常描画
				TileImage_drawBrushLine_headtail(imgdst, x1, y1, x2, y2, 1000);
			}
			else
			{
				//ブラシ:簡易プレビュー

				dsin = brushdp->radius * -dsin * xscale;
				dcos = brushdp->radius * dcos * yscale;

				TileImage_drawLineB(imgdst, x2, y2, x1 + dsin, y1 + dcos, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, x1 + dsin, y1 + dcos, x1 - dsin, y1 - dcos, &coldraw, FALSE);
				TileImage_drawLineB(imgdst, x1 - dsin, y1 - dcos, x2, y2, &coldraw, FALSE);
			}
		}

		FilterSub_prog_substep_inc(info);
	}

	mFree(brushdp);

	return TRUE;
}

