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

/*********************************
 * 筆圧用スプライン曲線
 *********************************/

#include <stdlib.h>

#include "mlk.h"

#include "curve_spline.h"


/* 各パラメータ計算 (a に値をセット後) */

static void _set_param(CurveSplineParam *p,int num)
{
	double *a,*b,*c,*d;
	double tmp,w[CURVE_SPLINE_MAXNUM];
	int i,last;

	a = p->a;
	b = p->b;
	c = p->c;
	d = p->d;

	last = num - 1;

	//

	c[0] = c[last] = 0;

	for(i = 1; i < last; i++)
		c[i] = 3.0 * (a[i - 1] - 2.0 * a[i] + a[i + 1]);

	//

	w[0] = 0;

	for(i = 1; i < last; i++)
	{
		tmp = 4.0 - w[i - 1];
		c[i] = (c[i] - c[i - 1]) / tmp;
		w[i] = 1.0 / tmp;
	}

	for(i = last - 1; i > 0; i--)
		c[i] = c[i] - c[i + 1] * w[i];

	//

	b[last] = d[last] = 0;

	for(i = 0; i < last; i++)
	{
		d[i] = (c[i + 1] - c[i]) / 3.0;
		b[i] = a[i + 1] - a[i] - c[i] - d[i];
	}
}

/* スプライン補間値取得
 *
 * pos: 左側の点の位置 (0〜num - 2)
 * t: 0.0-1.0 */

static double _get_value(CurveSplineParam *p,int pos,double t)
{
	return p->a[pos] + (p->b[pos] + (p->c[pos] + p->d[pos] * t) * t) * t;
}

/* 点を打つ */

static void _draw_point(uint16_t *dst,int x,int y)
{
	if(x >= 0 && x <= CURVE_SPLINE_POS_VAL)
	{
		if(y < 0)
			y = 0;
		else if(y > CURVE_SPLINE_POS_VAL)
			y = CURVE_SPLINE_POS_VAL;

		dst[x] = y;
	}
}

/* 線を引く */

static void _draw_line(uint16_t *dst,int x1,int y1,int x2,int y2)
{
	int inc = 1,f,finc;

	if(x1 == x2 && y1 == y2)
	{
		_draw_point(dst, x1, y1);
		return;
	}

	if(abs(x2 - x1) > abs(y2 - y1))
	{
		//横長
	
		finc = ((y2 - y1) << 16) / (x2 - x1);

		if(x1 > x2) inc = -1, finc = -finc;

		for(f = y1 << 16; x1 != x2; f += finc, x1 += inc)
			_draw_point(dst, x1, (f + (1<<15)) >> 16);
	}
	else
	{
		//縦長
		
		finc = ((x2 - x1) << 16) / (y2 - y1);

		if(y1 > y2) inc = -1, finc = -finc;

		for(f = x1 << 16; y1 != y2; f += finc, y1 += inc)
			_draw_point(dst, (f + (1<<15)) >> 16, y1);
	}
}


/** 筆圧の曲線テーブルをセット
 *
 * dst: CURVE_SPLINE_POS_VAL + 1 分の配列。添字が入力。値が出力。x が最大値で終端。
 * src: 上位16bit=X, 下位16bit=Y
 * num: 点の数 (3〜)。0 以下で、データから自動取得。
 *   [1]以降が 0 で終了。なければ、最大数。 */

void CurveSpline_setCurveTable(CurveSpline *p,uint16_t *dst,const uint32_t *src,int num)
{
	double *a,t,tinc;
	int i,j,div;
	mPoint pt,ptlast;

	if(num <= 0)
	{
		for(i = 1; i < CURVE_SPLINE_MAXNUM; i++)
		{
			if(src[i] == 0) break;
		}

		num = i;
	}

	if(num < 3) return;

	//x 位置セット

	a = p->x.a;

	for(i = 0; i < num; i++)
		a[i] = src[i] >> 16;

	_set_param(&p->x, num);

	//y 位置セット

	a = p->y.a;

	for(i = 0; i < num; i++)
		a[i] = src[i] & 0xffff;

	_set_param(&p->y, num);

	//

	div = CURVE_SPLINE_POS_VAL / (num + 1);
	tinc = 1.0 / div;

	//線を引く

	ptlast.x = src[0] >> 16;
	ptlast.y = src[0] & 0xffff;

	for(i = 0; i < num - 1; i++)
	{
		for(j = div, t = tinc; j; j--, t += tinc)
		{
			pt.x = (int)(_get_value(&p->x, i, t) + 0.5);
			pt.y = (int)(_get_value(&p->y, i, t) + 0.5);

			_draw_line(dst, ptlast.x, ptlast.y, pt.x, pt.y);

			ptlast = pt;
		}
	}

	//最後の位置

	_draw_line(dst, ptlast.x, ptlast.y,
		src[num - 1] >> 16, src[num - 1] & 0xffff);
}

