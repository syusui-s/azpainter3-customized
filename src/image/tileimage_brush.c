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
 * TileImage: ブラシ描画
 *****************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rand.h"

#include "colorvalue.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "pv_tileimage.h"

#include "def_brushdraw.h"
#include "table_data.h"
#include "imagematerial.h"


//-----------------

#define _BRUSHDP    (g_tileimage_dinfo.brushdp)

static void _drawbrush_point(TileImage *p,double x,double y,double radius,double opacity);

//-----------------



//==================================
// 水彩 sub
//==================================


/* ブラシ半径内の色の平均色を取得 */

static void _water_get_avg_color(TileImage *p,
	double x,double y,double radius,RGBAdouble *dst)
{
	int x1,y1,x2,y2,cnt,ix,iy,bits,fhave;
	double rd,r,g,b,a,dx,dy,dd;
	uint8_t cbuf[8];
	uint16_t *p16;

	bits = TILEIMGWORK->bits;

	p16 = (uint16_t *)cbuf;

	x1 = floor(x - radius);
	y1 = floor(y - radius);
	x2 = floor(x + radius);
	y2 = floor(y + radius);

	rd = radius * radius;

	//

	r = g = b = a = 0;
	cnt = 0;
	fhave = FALSE;

	for(iy = y1; iy <= y2; iy++)
	{
		dy = iy - y;
		dy *= dy;

		for(ix = x1; ix <= x2; ix++)
		{
			dx = ix - x;

			if(dx * dx + dy < rd)
			{
				cnt++;

				if(__TileImage_getDrawSrcColor(p, ix, iy, cbuf))
				{
					if(bits == 8)
					{
						//8bit
						
						if(cbuf[3])
						{
							dd = cbuf[3] * (1.0 / 255.0);

							r += cbuf[0] * dd;
							g += cbuf[1] * dd;
							b += cbuf[2] * dd;
							a += dd;

							fhave = TRUE;
						}
					}
					else
					{
						//16bit
						
						if(p16[3])
						{
							dd = (double)p16[3] / 0x8000;

							r += p16[0] * dd;
							g += p16[1] * dd;
							b += p16[2] * dd;
							a += dd;

							fhave = TRUE;
						}
					}
				}
			}
		}
	}

	//平均色

	if(!fhave || cnt == 0)
		dst->r = dst->g = dst->b = dst->a = 0;
	else
	{
		dd = 1.0 / a;

		dst->r = r * dd;
		dst->g = g * dd;
		dst->b = b * dd;
		dst->a = a / cnt;
	}

	//背景白と合成

	if(_BRUSHDP->flags & BRUSHDP_F_WATER_TP_WHITE)
	{
		dd = TILEIMGWORK->bits_val;

		for(ix = 0; ix < 3; ix++)
			dst->ar[ix] = (dst->ar[ix] - dd) * dst->a + dd;

		dst->a = 1;
	}
}

/* 水彩混色 */

static void _water_mix_color(RGBAdouble *dst,RGBAdouble *src,double op)
{
	double sa,da,ra;

	sa = src->a;
	da = dst->a;

	dst->a = (sa - da) * op + da;

	if(dst->a == 0)
		dst->r = dst->g = dst->b = 0;
	else
	{
		sa *= op;
		da *= 1.0 - op;
		ra = 1.0 / dst->a;

		dst->r = (dst->r * da + src->r * sa) * ra;
		dst->g = (dst->g * da + src->g * sa) * ra;
		dst->b = (dst->b * da + src->b * sa) * ra;
	}
}

/* 水彩描画色計算 */

static void _water_calc_drawcol(TileImage *p,double x,double y,double radius,
	double *dst_opacity,uint8_t *dstcol)
{
	BrushDrawParam *dp = _BRUSHDP;
	RGBAdouble col,*pcur;
	int i,n;

	pcur = &TILEIMGWORK->brush.col_water;

	//下地の色

	_water_get_avg_color(p, x, y, radius, &col);

	//下地 + 前回の色

	_water_mix_color(&col, pcur, dp->water_param[1]);

	// + 描画色

	_water_mix_color(&col, &TILEIMGWORK->brush.col_water_draw, dp->water_param[0]);

	*pcur = col;

	//結果の描画色 (dstcol)

	if(TILEIMGWORK->bits == 8)
	{
		for(i = 0; i < 3; i++)
		{
			n = round(pcur->ar[i]);

			if(n < 0) n = 0;
			else if(n > 255) n = 255;

			dstcol[i] = n;
		}
	}
	else
	{
		for(i = 0; i < 3; i++)
		{
			n = round(pcur->ar[i]);

			if(n < 0) n = 0;
			else if(n > 0x8000) n = 0x8000;

			*((uint16_t *)dstcol + i) = n;
		}
	}

	//

	if(col.a < *dst_opacity)
		*dst_opacity = col.a;
}


//===========================
//
//===========================


/** 曲線補間用 初期化 */

void __TileImage_init_curve(void)
{
	int i;
	double t,tt,t1,t2,t3,tadd,*dst;

	dst = (double *)TILEIMGWORK->brush.curve_param;

	tt = 1.0 / 6.0;
	tadd = t = 1.0 / TILEIMAGE_BRUSH_CURVE_NUM;

	for(i = TILEIMAGE_BRUSH_CURVE_NUM; i; i--, t += tadd)
	{
		t1 = 4.0 - (t + 3.0);
		t2 = t + 2.0;
		t3 = t + 1.0;

		dst[0] = tt * t1 * t1 * t1;
		dst[1] = tt * ( 3 * t2 * t2 * t2 - 24 * t2 * t2 + 60 * t2 - 44);
		dst[2] = tt * (-3 * t3 * t3 * t3 + 12 * t3 * t3 - 12 * t3 + 4);
		dst[3] = tt * t * t * t;

		dst += 4;
	}
}


//===========================
// 描画開始
//===========================


/* 水彩開始 */

static void _begin_water(TileImage *p,double x,double y)
{
	RGBAdouble *pd;
	uint8_t *buf;

	if(_BRUSHDP->flags & BRUSHDP_F_WATER)
	{
		//描画色

		pd = &TILEIMGWORK->brush.col_water_draw;
		buf = (uint8_t *)&g_tileimage_dinfo.drawcol;

		if(TILEIMGWORK->bits == 8)
		{
			pd->r = buf[0];
			pd->g = buf[1];
			pd->b = buf[2];
		}
		else
		{
			pd->r = *((uint16_t *)buf);
			pd->g = *((uint16_t *)buf + 1);
			pd->b = *((uint16_t *)buf + 2);
		}
		
		pd->a = 1;

		//下地の色

		_water_get_avg_color(p, x, y, _BRUSHDP->radius, &TILEIMGWORK->brush.col_water);
	}
}

/** ブラシ自由線描画の開始
 *
 * no: 対象定規用、線の番号。通常は 0 */

void TileImage_drawBrush_beginFree(TileImage *p,int no,double x,double y,double pressure)
{
	TileImageBrushWorkData *pw = &TILEIMGWORK->brush;
	TileImageCurvePoint *ppt;

	pw->ptlast[no].x = x;
	pw->ptlast[no].y = y;
	pw->ptlast[no].pressure = pressure;
	pw->ptlast[no].t = 0;

	//曲線補間

	if(_BRUSHDP->flags & BRUSHDP_F_CURVE)
	{
		ppt = pw->curve_pt[no];
	
		ppt[0].x = x;
		ppt[0].y = y;
		ppt[0].pressure = pressure;
	
		ppt[1] = ppt[0];
		ppt[2] = ppt[0];
	}

	//水彩

	_begin_water(p, x, y);
}

/** 自由線以外の開始時 */

void TileImage_drawBrush_beginOther(TileImage *p,double x,double y)
{
	_begin_water(p, x, y);
}


//================================
// 線の描画
//================================


/* 曲線補間の自由線描画 */

static void _drawbrush_free_curve(TileImage *p,int no,double x,double y,double pressure)
{
	TileImageBrushWorkData *pw = &TILEIMGWORK->brush;
	TileImageCurvePoint *ppt;
	double *param,lastx,lasty,press_st,press_ed;
	double t1,t2,t3,t4,t;
	int i;

	ppt = pw->curve_pt[no];

	//最後の位置

	ppt[3].x = x;
	ppt[3].y = y;
	ppt[3].pressure = pressure;

	//前回の位置

	lastx = pw->ptlast[no].x;
	lasty = pw->ptlast[no].y;
	press_st = pw->ptlast[no].pressure;
	t = pw->ptlast[no].t;

	//--------

	param = (double *)pw->curve_param;

	for(i = TILEIMAGE_BRUSH_CURVE_NUM; i; i--, param += 4)
	{
		t1 = param[0];
		t2 = param[1];
		t3 = param[2];
		t4 = param[3];

		//

		x = ppt[0].x * t1 + ppt[1].x * t2 + ppt[2].x * t3 + ppt[3].x * t4;
		y = ppt[0].y * t1 + ppt[1].y * t2 + ppt[2].y * t3 + ppt[3].y * t4;

		press_ed = ppt[0].pressure * t1 + ppt[1].pressure * t2 + ppt[2].pressure * t3 + ppt[3].pressure * t4;

		t = TileImage_drawBrushLine(p, lastx, lasty, x, y, press_st, press_ed, t);

		//

		lastx = x;
		lasty = y;
		press_st = press_ed;
	}

	//過去の点をずらす

	for(i = 0; i < 3; i++)
		ppt[i] = ppt[i + 1];

	//

	pw->ptlast[no].x = x;
	pw->ptlast[no].y = y;
	pw->ptlast[no].pressure = press_ed;
	pw->ptlast[no].t = t;
}

/** ブラシ:自由線描画 */

void TileImage_drawBrushFree(TileImage *p,int no,double x,double y,double pressure)
{
	TileImageBrushWorkData *pw = &TILEIMGWORK->brush;

	if(_BRUSHDP->flags & BRUSHDP_F_CURVE)
	{
		//曲線

		_drawbrush_free_curve(p, no, x, y, pressure);
	}
	else
	{
		//直線

		TileImageBrushPoint *ppt;

		ppt = pw->ptlast + no;

		ppt->t = TileImage_drawBrushLine(p,
			ppt->x, ppt->y, x, y, ppt->pressure, pressure, ppt->t);

		ppt->x = x;
		ppt->y = y;
		ppt->pressure = pressure;
	}
}

/** ブラシ:自由線描画 最後の線を描画 (曲線時) */

void TileImage_drawBrushFree_finish(TileImage *p,int no)
{
	int i;
	TileImageCurvePoint pt;

	if(_BRUSHDP->flags & BRUSHDP_F_CURVE)
	{
		pt = TILEIMGWORK->brush.curve_pt[no][3];
	
		for(i = 0; i < 3; i++)
			_drawbrush_free_curve(p, no, pt.x, pt.y, pt.pressure);
	}
}


//=====================================
// 線を引く
//=====================================


/** ブラシで線を引く (線形補間)
 *
 * return: 次の t 値が返る */

double TileImage_drawBrushLine(TileImage *p,
	double x1,double y1,double x2,double y2,
	double press_st,double press_ed,double t_start)
{
	TileImageWorkData *work = TILEIMGWORK;
	BrushDrawParam *param = _BRUSHDP;
	double len,t,dx,dy,ttm,dtmp;
	double press_len,radius_len,opa_len;
	double press,press_r,press_o,radius,opacity,x,y;
	int angle,ntmp;

	//線の長さ

	dx = x2 - x1;
	dy = y2 - y1;

	len = sqrt(dx * dx + dy * dy);

	if(len == 0) return t_start;

	//間隔を、長さ 1.0 に対する値に

	ttm = param->interval / len;

	//各幅

	radius_len = 1.0 - param->min_size;
	opa_len    = 1.0 - param->min_opacity;
	press_len  = press_ed - press_st;

	//ブラシ画像回転角度

	angle = param->angle;

	if(param->flags & BRUSHDP_F_TRAVELLING_DIR)
		angle += (int)(-atan2(dy, dx) * 256 / MLK_MATH_PI);   //進行方向

	//------------------

	t = t_start / len;

	while(t < 1.0)
	{
		//--------- 筆圧 (0.0-1.0)

		//t 位置の筆圧

		press = press_len * t + press_st;

		//筆圧補正

		if(param->flags & BRUSHDP_F_PRESSURE_CURVE)
		{
			//曲線
			
			ntmp = (int)(press * (1<<12) + 0.5);
			
			press = param->press_curve_tbl[ntmp] * (1.0 / (1<<12));
		}
		else
		{
			//線形補間

			press = param->pressure_min + param->pressure_range * press;
		}

		//サイズ,濃度の筆圧

		press_r = press * radius_len + param->min_size;
		press_o = press * opa_len    + param->min_opacity;

		//--------- 半径・濃度

		//ランダムサイズ

		if(param->flags & BRUSHDP_F_RANDOM_SIZE)
		{
			press_r = (mRandSFMT_getDouble(work->rand)
				* (1.0 - param->rand_size_min) + param->rand_size_min) * press_r;
		}

		//

		radius  = press_r * param->radius;
		opacity = press_o * param->opacity;

		//---------- 位置

		x = x1 + dx * t;
		y = y1 + dy * t;

		//ランダム位置 (円範囲)

		if(param->flags & BRUSHDP_F_RANDOM_POS)
		{
			dtmp = (radius * param->rand_pos_len) * mRandSFMT_getDouble(work->rand);
			ntmp = mRandSFMT_getIntRange(work->rand, 0, 511);

			x += dtmp * TABLEDATA_GET_COS(ntmp);
			y += dtmp * TABLEDATA_GET_SIN(ntmp);
		}

		//----------- ブラシ画像回転角度

		work->brush.angle = angle;

		//ランダム角度

		ntmp = param->angle_rand;

		if(ntmp)
			work->brush.angle += mRandSFMT_getIntRange(work->rand, -ntmp, ntmp);

		//------------ 点描画

		_drawbrush_point(p, x, y, radius, opacity);

		//------------ 間隔分を加算して次の点へ

		dtmp = radius * ttm;
		if(dtmp < 0.0001) dtmp = 0.0001;

		t += dtmp;
	}

	//次回の開始 t 値 (px に変換)

	return len * (t - 1.0);
}

/** 入り抜き指定ありの直線 */

void TileImage_drawBrushLine_headtail(TileImage *p,
	double x1,double y1,double x2,double y2,uint32_t headtail)
{
	int head,tail;
	double t,dx,dy,d,xx,yy,xx2,yy2;

	head = headtail >> 16;
	tail = headtail & 0xffff;

	if(tail > 1000 - head)
		tail = 1000 - head;

	if(head == 0 && tail == 0)
		//通常
		TileImage_drawBrushLine(p, x1, y1, x2, y2, 1, 1, 0);
	else
	{
		dx = x2 - x1;
		dy = y2 - y1;

		xx = x1, yy = y1;
		t = 0;

		//入り

		if(head)
		{
			d  = head * 0.001;
			xx = x1 + dx * d;
			yy = y1 + dy * d;

			t = TileImage_drawBrushLine(p, x1, y1, xx, yy, 0, 1, t);
		}

		//中間部分 (筆圧 = 1)

		if(tail != 1000 - head)
		{
			d   = (1000 - tail) * 0.001;
			xx2 = x1 + dx * d;
			yy2 = y1 + dy * d;

			t = TileImage_drawBrushLine(p, xx, yy, xx2, yy2, 1, 1, t);

			xx = xx2, yy = yy2;
		}

		//抜き

		if(tail)
			TileImage_drawBrushLine(p, xx, yy, x2, y2, 1, 0, t);
	}
}

/** 四角形描画 */

void TileImage_drawBrushBox(TileImage *p,mDoublePoint *pt)
{
	TileImage_drawBrushLine(p, pt[0].x, pt[0].y, pt[1].x, pt[1].y, 1, 1, 0);
	TileImage_drawBrushLine(p, pt[1].x, pt[1].y, pt[2].x, pt[2].y, 1, 1, 0);
	TileImage_drawBrushLine(p, pt[2].x, pt[2].y, pt[3].x, pt[3].y, 1, 1, 0);
	TileImage_drawBrushLine(p, pt[3].x, pt[3].y, pt[0].x, pt[0].y, 1, 1, 0);
}


//=====================================
// 点の描画
//=====================================

#define FIXF_BIT  10
#define FIXF_VAL  (1<<FIXF_BIT)

#define SUBNUM_MAX  11


//回転なし用
typedef struct
{
	int bits,
		subnum,			//サブピクセル数
		x1,y1,x2,y2,	//描画px範囲
		sand,			//砂化
		alpha_max;		//最大アルファ値
	double opacity_bit;	//現在ビット値での濃度
	mlkbool fnoaa,		//非アンチエイリアス
		fdrawtp;		//透明色も描画
	TileImageSetPixelFunc setpix;
}_drawpointinfo;

//回転あり用
typedef struct
{
	_drawpointinfo b;
	int img_size,
		img_pitch;
	double dx_left,
		dy_top,
		dinc_cos,
		dinc_sin,
		dinc_cos_sub,
		dinc_sin_sub;
}_drawpointinfo_rotate;


/* (共通) 描画情報のセット */

static void _drawpoint_set_info(_drawpointinfo *p,double x,double y,double radius,double opacity,mlkbool fimage)
{
	int n;

	p->bits = TILEIMGWORK->bits;

	//アルファ値

	n = TILEIMGWORK->bits_val;

	p->alpha_max = (int)(opacity * n + 0.5);
	p->opacity_bit = opacity * n;

	//半径の大きさによりサブピクセル数調整

	if(fimage)
	{
		if(radius < 4) n = 11;
		else if(radius < 25) n = 9;
		else n = 3;
	}
	else
	{
		if(radius < 3) n = 11;
		else if(radius < 15) n = 5;
		else n = 3;
	}

	p->subnum = n;

	//描画する範囲 (px)

	p->x1 = (int)floor(x - radius);
	p->y1 = (int)floor(y - radius);
	p->x2 = (int)floor(x + radius);
	p->y2 = (int)floor(y + radius);

	//点描画関数

	p->setpix = g_tileimage_dinfo.func_setpixel;

	//ほか

	p->sand = _BRUSHDP->shape_sand;

	p->fnoaa = ((_BRUSHDP->flags & BRUSHDP_F_NO_ANTIALIAS) != 0);
	p->fdrawtp = ((_BRUSHDP->flags & BRUSHDP_F_OVERWRITE_TP) != 0);
}

/* 形状画像回転時の情報セット */

static void _drawpoint_setinfo_rotate(_drawpointinfo_rotate *p,
	double x,double y,double radius,double opacity)
{
	BrushDrawParam *dp = _BRUSHDP;
	int n;
	double d,dcos,dsin,dscale;

	p->b.bits = TILEIMGWORK->bits;

	//アルファ値

	n = TILEIMGWORK->bits_val;

	p->b.alpha_max = (int)(opacity * n + 0.5);
	p->b.opacity_bit = opacity * n;

	//サブピクセル数

	if(radius < 4) n = 11;
	else if(radius < 25) n = 7;
	else n = 3;

	p->b.subnum = n;

	//点描画関数

	p->b.setpix = g_tileimage_dinfo.func_setpixel;

	//ほか

	p->b.sand = dp->shape_sand;

	p->b.fnoaa = ((dp->flags & BRUSHDP_F_NO_ANTIALIAS) != 0);
	p->b.fdrawtp = ((dp->flags & BRUSHDP_F_OVERWRITE_TP) != 0);

	p->img_size = dp->img_shape->width;
	p->img_pitch = dp->img_shape->pitch;

	//描画px範囲
	// :回転後の範囲を考慮して、範囲を広げる

	d = radius * 1.44;

	p->b.x1 = (int)floor(x - d);
	p->b.y1 = (int)floor(y - d);
	p->b.x2 = (int)floor(x + d);
	p->b.y2 = (int)floor(y + d);

	//---- 回転用パラメータ

	n = TILEIMGWORK->brush.angle;

	dscale = (double)p->img_size / (radius * 2);

	dcos = TABLEDATA_GET_COS(n);
	dsin = TABLEDATA_GET_SIN(n);

	//描画先 px 左上におけるイメージ位置

	x = p->b.x1 - x;
	y = p->b.y1 - y;
	d = p->img_size * 0.5;

	p->dx_left = (x * dcos - y * dsin) * dscale + d;
	p->dy_top  = (x * dsin + y * dcos) * dscale + d;

	//加算値

	d = 1.0 / p->b.subnum;

	p->dinc_cos = dcos * dscale;
	p->dinc_sin = dsin * dscale;
	p->dinc_cos_sub = p->dinc_cos * d;
	p->dinc_sin_sub = p->dinc_sin * d;
}

/* 点の描画 */

static void _drawpoint_setpixel(TileImage *p,int x,int y,void *drawcol,int a,_drawpointinfo *info)
{
	uint64_t col;

	if(a || info->fdrawtp)
	{
		//砂化

		if(info->sand && a)
		{
			a -= a * mRandSFMT_getIntRange(TILEIMGWORK->rand, 0, info->sand) >> 7;
			if(a < 0) a = 0;
		}

		//非アンチエイリアスの場合、濃度は最大
		
		if(info->fnoaa && a)
			a = info->alpha_max;

		//セット

		if(a == 0)
		{
			//透明上書きの場合、すべて0でセット
			
			col = 0;
			(info->setpix)(p, x, y, &col);
		}
		else
		{
			if(info->bits == 8)
				*((uint8_t *)drawcol + 3) = a;
			else
				*((uint16_t *)drawcol + 3) = a;

			(info->setpix)(p, x, y, drawcol);
		}
	}
}

/* 形状画像カラー時の色を取得
 *
 * ddiv: 総和のアルファ値を 0.0-1.0 にする値
 * return: 描画アルファ値 */

static int _drawpoint_get_image_color(uint8_t *buf,int r,int g,int b,int a,
	double ddiv,_drawpointinfo *info)
{
	double da;
	int res_a;

	da = a * ddiv;

	res_a = (int)(info->opacity_bit * da + 0.5);

	if(res_a == 0)
	{
		*((uint64_t *)buf) = 0;
	}
	else
	{
		r /= a;
		g /= a;
		b /= a;
		
		if(info->bits == 8)
		{
			buf[0] = r;
			buf[1] = g;
			buf[2] = b;
		}
		else
		{
			*((uint16_t *)buf) = (r << 15) / 255;
			*((uint16_t *)buf + 1) = (g << 15) / 255;
			*((uint16_t *)buf + 2) = (b << 15) / 255;
		}
	}

	return res_a;
}


//--------------


/* 通常円形 (硬さ最大時) */

static void _drawbrush_point_circle_max(TileImage *p,
	double x,double y,double radius,double opacity,void *drawcol)
{
	_drawpointinfo info;
	int ix,iy,px,py,n;
	int xtbl[11],ytbl[11],subpos[11],fx,fy,fx_left,rr;
	int64_t fpos;
	double ddiv;

	_drawpoint_set_info(&info, x, y, radius, opacity, FALSE);

	fx_left = floor((info.x1 - x) * FIXF_VAL);
	fy = floor((info.y1 - y) * FIXF_VAL);
	rr = floor(radius * radius * FIXF_VAL);
	ddiv = 1.0 / (info.subnum * info.subnum);

	for(ix = 0; ix < info.subnum; ix++)
		subpos[ix] = (ix << FIXF_BIT) / info.subnum;

	//各 px

	for(py = info.y1; py <= info.y2; py++, fy += FIXF_VAL)
	{
		//Y テーブル

		for(iy = 0; iy < info.subnum; iy++)
		{
			fpos = fy + subpos[iy];
			ytbl[iy] = fpos * fpos >> FIXF_BIT;
		}

		//

		for(px = info.x1, fx = fx_left; px <= info.x2; px++, fx += FIXF_VAL)
		{
			//X テーブル

			for(ix = 0; ix < info.subnum; ix++)
			{
				fpos = fx + subpos[ix];
				xtbl[ix] = fpos * fpos >> FIXF_BIT;
			}

			//円の内側ならカウント加算

			n = 0;

			for(iy = 0; iy < info.subnum; iy++)
			{
				for(ix = 0; ix < info.subnum; ix++)
				{
					if(xtbl[ix] + ytbl[iy] < rr) n++;
				}
			}

			//点をセット

			_drawpoint_setpixel(p, px, py, drawcol, (int)(info.opacity_bit * (n * ddiv) + 0.5), &info);
		}
	}
}

/* 通常円形 (硬さのパラメータ値から) */

static void _drawbrush_point_circle(TileImage *p,
	double x,double y,double radius,double opacity,void *drawcol)
{
	_drawpointinfo info;
	int ix,iy,px,py;
	int xtbl[11],ytbl[11],subpos[11],rr,fx,fy,fx_left;
	int64_t fpos;
	double hard_param,dd,dsum,ddiv;

	_drawpoint_set_info(&info, x, y, radius, opacity, FALSE);

	hard_param = _BRUSHDP->shape_hard;

	//

	fx_left = floor((info.x1 - x) * FIXF_VAL);
	fy = floor((info.y1 - y) * FIXF_VAL);
	rr = floor(radius * radius * FIXF_VAL);
	ddiv = 1.0 / (info.subnum * info.subnum);

	for(ix = 0; ix < info.subnum; ix++)
		subpos[ix] = (ix << FIXF_BIT) / info.subnum;

	//--------------

	for(py = info.y1; py <= info.y2; py++, fy += FIXF_VAL)
	{
		//Y テーブル

		for(iy = 0; iy < info.subnum; iy++)
		{
			fpos = fy + subpos[iy];
			ytbl[iy] = fpos * fpos >> FIXF_BIT;
		}

		//

		for(px = info.x1, fx = fx_left; px <= info.x2; px++, fx += FIXF_VAL)
		{
			//X テーブル

			for(ix = 0; ix < info.subnum; ix++)
			{
				fpos = fx + subpos[ix];
				xtbl[ix] = fpos * fpos >> FIXF_BIT;
			}

			//オーバーサンプリング

			dsum = 0;

			for(iy = 0; iy < info.subnum; iy++)
			{
				for(ix = 0; ix < info.subnum; ix++)
				{
					if(xtbl[ix] + ytbl[iy] < rr)
					{
						dd = (double)(xtbl[ix] + ytbl[iy]) / rr;

						dd = dd + hard_param - dd * hard_param;

						if(dd < 0) dd = 0;
						else if(dd > 1) dd = 1;

						dsum += 1.0 - dd;
					}
				}
			}

			//点をセット

			_drawpoint_setpixel(p, px, py, drawcol, (int)(info.opacity_bit * (dsum * ddiv) + 0.5), &info);
		}
	}
}

/* 形状画像/回転なし (8bit:濃度のみ) */

static void _drawbrush_point_image_8bit(TileImage *p,
	double x,double y,double radius,double opacity,void *drawcol)
{
	_drawpointinfo info;
	int img_size,img_pitch,px,py,ix,iy,xtbl[11],n;
	double dsubadd,dadd,dx,dy,dx_left,dsumdiv,dtmp;
	uint8_t *img_buf,*buf,*ytbl[11];

	_drawpoint_set_info(&info, x, y, radius, opacity, TRUE);

	img_buf = _BRUSHDP->img_shape->buf;
	img_size = _BRUSHDP->img_shape->width;
	img_pitch = _BRUSHDP->img_shape->pitch;

	dadd = (double)img_size / (radius * 2); //1px進むごとに加算する画像の位置
	dsubadd = dadd / info.subnum;
	dsumdiv = 1.0 / ((info.subnum * info.subnum) * 255.0);

	//左上pxにおける画像の位置

	dtmp = img_size * 0.5;
	dx_left = (info.x1 - x) * dadd + dtmp;
	dy = (info.y1 - y) * dadd + dtmp;

	//-----------

	for(py = info.y1; py <= info.y2; py++, dy += dadd)
	{
		//Y テーブル (ポインタ位置)

		for(iy = 0, dtmp = dy; iy < info.subnum; iy++, dtmp += dsubadd)
		{
			n = (int)dtmp;
			ytbl[iy] = (n >= 0 && n < img_size)? img_buf + n * img_pitch: NULL;
		}

		//

		for(px = info.x1, dx = dx_left; px <= info.x2; px++, dx += dadd)
		{
			//X テーブル (x 位置)

			for(ix = 0, dtmp = dx; ix < info.subnum; ix++, dtmp += dsubadd)
			{
				n = (int)dtmp;
				xtbl[ix] = (n >= 0 && n < img_size)? n: -1;
			}

			//オーバーサンプリング

			n = 0;

			for(iy = 0; iy < info.subnum; iy++)
			{
				buf = ytbl[iy];
				if(!buf) continue;

				for(ix = 0; ix < info.subnum; ix++)
				{
					if(xtbl[ix] != -1)
						n += *(buf + xtbl[ix]);
				}
			}

			//1px 点を打つ

			_drawpoint_setpixel(p, px, py, drawcol, (int)(info.opacity_bit * (n * dsumdiv) + 0.5), &info);
		}
	}
}

/* 形状画像/回転なし (32bit:RGBA) */

static void _drawbrush_point_image_32bit(TileImage *p,
	double x,double y,double radius,double opacity)
{
	_drawpointinfo info;
	int img_size,img_pitch,px,py,ix,iy,xtbl[11],n,r,g,b,a;
	double dsubadd,dadd,dx,dy,dx_left,dsumdiv,dtmp;
	uint8_t *img_buf,*bufY,*buf,*ytbl[11];
	uint64_t col;

	_drawpoint_set_info(&info, x, y, radius, opacity, TRUE);

	img_buf = _BRUSHDP->img_shape->buf;
	img_size = _BRUSHDP->img_shape->width;
	img_pitch = _BRUSHDP->img_shape->pitch;

	dadd = (double)img_size / (radius * 2);
	dsubadd = dadd / info.subnum;
	dsumdiv = 1.0 / (info.subnum * info.subnum * 255.0);

	//左上pxにおける画像の位置

	dtmp = img_size * 0.5;
	dx_left = (info.x1 - x) * dadd + dtmp;
	dy = (info.y1 - y) * dadd + dtmp;

	//-----------

	for(py = info.y1; py <= info.y2; py++, dy += dadd)
	{
		//Y テーブル (ポインタ位置)

		for(iy = 0, dtmp = dy; iy < info.subnum; iy++, dtmp += dsubadd)
		{
			n = (int)dtmp;
			ytbl[iy] = (n >= 0 && n < img_size)? img_buf + n * img_pitch: NULL;
		}

		//

		for(px = info.x1, dx = dx_left; px <= info.x2; px++, dx += dadd)
		{
			//X テーブル (x 位置)

			for(ix = 0, dtmp = dx; ix < info.subnum; ix++, dtmp += dsubadd)
			{
				n = (int)dtmp;
				xtbl[ix] = (n >= 0 && n < img_size)? (n << 2): -1;
			}

			//オーバーサンプリング

			r = g = b = a = 0;

			for(iy = 0; iy < info.subnum; iy++)
			{
				bufY = ytbl[iy];
				if(!bufY) continue;

				for(ix = 0; ix < info.subnum; ix++)
				{
					if(xtbl[ix] != -1)
					{
						buf = bufY + xtbl[ix];
						n = buf[3];

						r += buf[0] * n;
						g += buf[1] * n;
						b += buf[2] * n;
						a += n;
					}
				}
			}

			//点をセット

			a = _drawpoint_get_image_color((uint8_t *)&col, r, g, b, a, dsumdiv, &info);

			_drawpoint_setpixel(p, px, py, &col, a, &info);
		}
	}
}

/* 形状画像/回転あり (8bit) */

static void _drawbrush_point_image_rotate_8bit(TileImage *p,
	double x,double y,double radius,double opacity,void *drawcol)
{
	_drawpointinfo_rotate info;
	int px,py,c,ix,iy,nx,ny;
	double dsumdiv,dx,dy,dx2,dy2,dx3,dy3;
	uint8_t *img_buf;
	mlkbool fhave;

	_drawpoint_setinfo_rotate(&info, x, y, radius, opacity);

	img_buf = _BRUSHDP->img_shape->buf;

	dsumdiv = 1.0 / ((info.b.subnum * info.b.subnum) * 255.0);

	//----------------
	// xx/yy:cos, xy:sin, yx:-sin

	for(py = info.b.y1; py <= info.b.y2; py++)
	{
		dx = info.dx_left;
		dy = info.dy_top;

		for(px = info.b.x1; px <= info.b.x2; px++)
		{
			dx2 = dx;
			dy2 = dy;

			//オーバーサンプリング

			c = 0;
			fhave = FALSE;

			for(iy = info.b.subnum; iy; iy--)
			{
				dx3 = dx2;
				dy3 = dy2;

				for(ix = info.b.subnum; ix; ix--)
				{
					nx = (int)dx3;
					ny = (int)dy3;

					if(nx >= 0 && nx < info.img_size
						&& ny >= 0 && ny < info.img_size)
					{
						c += *(img_buf + ny * info.img_pitch + nx);
						
						fhave = TRUE;
					}

					dx3 += info.dinc_cos_sub;
					dy3 += info.dinc_sin_sub;
				}

				dx2 -= info.dinc_sin_sub;
				dy2 += info.dinc_cos_sub;
			}

			//点をセット
			// :形状の矩形範囲外は除く

			if(fhave)
			{
				c = (int)(info.b.opacity_bit * (c * dsumdiv) + 0.5);

				_drawpoint_setpixel(p, px, py, drawcol, c, &info.b);
			}

			//

			dx += info.dinc_cos;
			dy += info.dinc_sin;
		}

		info.dx_left -= info.dinc_sin;
		info.dy_top  += info.dinc_cos;
	}
}

/* 形状画像/回転あり (32bit) */

static void _drawbrush_point_image_rotate_32bit(TileImage *p,
	double x,double y,double radius,double opacity)
{
	_drawpointinfo_rotate info;
	int px,py,ix,iy,nx,ny,r,g,b,a;
	double dsumdiv,dx,dy,dx2,dy2,dx3,dy3;
	uint8_t *img_buf,*buf;
	uint64_t col;
	mlkbool fhave;

	_drawpoint_setinfo_rotate(&info, x, y, radius, opacity);

	img_buf = _BRUSHDP->img_shape->buf;

	dsumdiv = 1.0 / ((info.b.subnum * info.b.subnum) * 255.0);

	//----------------
	// xx/yy:cos, xy:sin, yx:-sin

	for(py = info.b.y1; py <= info.b.y2; py++)
	{
		dx = info.dx_left;
		dy = info.dy_top;

		for(px = info.b.x1; px <= info.b.x2; px++)
		{
			dx2 = dx;
			dy2 = dy;

			//オーバーサンプリング

			r = g = b = a = 0;
			fhave = FALSE;

			for(iy = info.b.subnum; iy; iy--)
			{
				dx3 = dx2;
				dy3 = dy2;

				for(ix = info.b.subnum; ix; ix--)
				{
					nx = (int)dx3;
					ny = (int)dy3;

					if(nx >= 0 && nx < info.img_size
						&& ny >= 0 && ny < info.img_size)
					{
						fhave = TRUE;

						buf = img_buf + ny * info.img_pitch + (nx << 2);
						nx = buf[3];

						r += buf[0] * nx;
						g += buf[1] * nx;
						b += buf[2] * nx;
						a += nx;
					}

					dx3 += info.dinc_cos_sub;
					dy3 += info.dinc_sin_sub;
				}

				dx2 -= info.dinc_sin_sub;
				dy2 += info.dinc_cos_sub;
			}

			//点をセット
			// :形状の矩形範囲外は除く

			if(fhave)
			{
				nx = _drawpoint_get_image_color((uint8_t *)&col, r, g, b, a, dsumdiv, &info.b);

				_drawpoint_setpixel(p, px, py, &col, nx, &info.b);
			}

			//

			dx += info.dinc_cos;
			dy += info.dinc_sin;
		}

		info.dx_left -= info.dinc_sin;
		info.dy_top  += info.dinc_cos;
	}
}

/* ブラシ点の描画 (メイン)
 *
 * radius: 半径(px)
 * opacity: 0.0-1.0 */

void _drawbrush_point(TileImage *p,double x,double y,double radius,double opacity)
{
	uint64_t col;

	if(opacity == 0 || radius < 0.05) return;

	col = g_tileimage_dinfo.drawcol;

	//水彩:描画色の計算

	if(_BRUSHDP->flags & BRUSHDP_F_WATER)
	{
		_water_calc_drawcol(p, x, y, radius, &opacity, (uint8_t *)&col);
	}

	//点描画

	if(!_BRUSHDP->img_shape)
	{
		//通常円形

		if(_BRUSHDP->flags & BRUSHDP_F_SHAPE_HARD_MAX)
			_drawbrush_point_circle_max(p, x, y, radius, opacity, &col);
		else
			_drawbrush_point_circle(p, x, y, radius, opacity, &col);
	}
	else
	{
		//形状画像

		if(TILEIMGWORK->brush.angle)
		{
			if(_BRUSHDP->img_shape->bits == 8)
				_drawbrush_point_image_rotate_8bit(p, x, y, radius, opacity, &col);
			else
				_drawbrush_point_image_rotate_32bit(p, x, y, radius, opacity);
		}
		else
		{
			if(_BRUSHDP->img_shape->bits == 8)
				_drawbrush_point_image_8bit(p, x, y, radius, opacity, &col);
			else
				_drawbrush_point_image_32bit(p, x, y, radius, opacity);
		}
	}
}
