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

/************************************
 * 自由線描画時のポイントデータ
 ************************************/

typedef struct _PointBuf PointBuf;

typedef struct
{
	double x,y,pressure;
}PointBufDat;

//手ブレ補正タイプ
enum
{
	SMOOTHING_TYPE_NONE,
	SMOOTHING_TYPE_AVG_SIMPLE,
	SMOOTHING_TYPE_AVG_LINEAR,
	SMOOTHING_TYPE_AVG_GAUSS,
	SMOOTHING_TYPE_RANGE
};


PointBuf *PointBuf_new(void);
void PointBuf_setSmoothing(PointBuf *p,int type,int val);

void PointBuf_setFirstPoint(PointBuf *p,double x,double y,double pressure);
void PointBuf_addPoint(PointBuf *p,double x,double y,double pressure);

mlkbool PointBuf_getPoint(PointBuf *p,PointBufDat *dst);
mlkbool PointBuf_getFinishPoint(PointBuf *p,PointBufDat *dst);

