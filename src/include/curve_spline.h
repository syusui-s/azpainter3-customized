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

#ifndef AZPT_CURVE_SPLINE_H
#define AZPT_CURVE_SPLINE_H

#define CURVE_SPLINE_MAXNUM  8
#define CURVE_SPLINE_POS_BIT 12
#define CURVE_SPLINE_POS_VAL (1<<12)
#define CURVE_SPLINE_VAL_MAXPOS  ((1<<(12+16)) | (1<<12))

typedef struct _CurveSpline CurveSpline;

typedef struct
{
	double a[CURVE_SPLINE_MAXNUM],
		b[CURVE_SPLINE_MAXNUM],
		c[CURVE_SPLINE_MAXNUM],
		d[CURVE_SPLINE_MAXNUM];
}CurveSplineParam;

struct _CurveSpline
{
	CurveSplineParam x,y;
};

void CurveSpline_setCurveTable(CurveSpline *p,uint16_t *dst,const uint32_t *src,int num);

#endif

