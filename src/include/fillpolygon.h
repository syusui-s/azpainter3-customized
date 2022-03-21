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
 * 多角形塗りつぶし処理
 ************************************/

typedef struct _FillPolygon FillPolygon;

FillPolygon *FillPolygon_new(void);
void FillPolygon_free(FillPolygon *p);

mlkbool FillPolygon_addPoint(FillPolygon *p,double x,double y);
mlkbool FillPolygon_closePoint(FillPolygon *p);

mlkbool FillPolygon_addPoint4_close(FillPolygon *p,mDoublePoint *pt);

mlkbool FillPolygon_beginDraw(FillPolygon *p,mlkbool antialias);
void FillPolygon_getMinMaxY(FillPolygon *p,int *ymin,int *ymax);
void FillPolygon_getDrawRect(FillPolygon *p,mRect *rc);

mlkbool FillPolygon_getIntersection_noAA(FillPolygon *p,int y);
mlkbool FillPolygon_getNextLine_noAA(FillPolygon *p,int *left,int *right);

mlkbool FillPolygon_setXBuf_AA(FillPolygon *p,int y);
uint16_t *FillPolygon_getAABuf(FillPolygon *p,int *xmin,int *width);

