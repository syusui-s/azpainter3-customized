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

/**********************************
 * AppDraw: 計算関数
 **********************************/

void drawCalc_setCanvasViewParam(AppDraw *p);
mlkbool drawCalc_isPointInCanvasArea(AppDraw *p,mPoint *pt);

/* 座標変換 */

void drawCalc_canvas_to_image_double(AppDraw *p,double *dx,double *dy,double x,double y);
void drawCalc_canvas_to_image_double_pt(AppDraw *p,mDoublePoint *dst,mPoint *src);
void drawCalc_canvas_to_image_pt(AppDraw *p,mPoint *pt,double x,double y);
void drawCalc_canvas_to_image_relative(AppDraw *p,mPoint *dst,mPoint *src);

void drawCalc_image_to_canvas_pt(AppDraw *p,mPoint *dst,int x,int y);
void drawCalc_image_to_canvas_pt_double(AppDraw *p,mPoint *dst,double x,double y);

void drawCalc_getImagePos_atCanvasCenter(AppDraw *p,double *px,double *py);
mlkbool drawCalc_image_to_canvas_box(AppDraw *p,mBox *dst,const mBox *src);
mlkbool drawCalc_canvas_to_image_box(AppDraw *p,mBox *dst,const mBox *src);

/* 範囲 */

mlkbool drawCalc_clipCanvas_toBox(AppDraw *p,mBox *dst,mRect *src);
mlkbool drawCalc_clipImageRect(AppDraw *p,mRect *rc);
mlkbool drawCalc_image_rect_to_box(AppDraw *p,mBox *dst,const mRect *rc);
mlkbool drawCalc_unionImageBox(mBox *dst,mBox *src);
void drawCalc_unionRect_relmove(mRect *dst,mRect *src,int mx,int my);

/* 描画操作 */

void drawCalc_fitLine45(mPoint *pt,mPoint *ptstart);
double drawCalc_getLineRadian_forImage(AppDraw *p);
mlkbool drawCalc_moveImage_onMotion(AppDraw *p,TileImage *img,uint32_t state,mPoint *ptret);

/* ほか */

int drawCalc_getZoom_step(AppDraw *p,mlkbool zoomup);
int drawCalc_getZoom_fitWindow(AppDraw *p);
void drawCalc_getCanvasScrollMax(AppDraw *p,mSize *size);

