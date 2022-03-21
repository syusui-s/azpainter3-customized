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
 * AppDraw 各操作の関数
 **********************************/

/* brush/dot */

mlkbool drawOp_brush_free_press(AppDraw *p);
mlkbool drawOp_dotpen_free_press(AppDraw *p);

void drawOpDraw_brushdot_line(AppDraw *p);
void drawOpDraw_brushdot_rect(AppDraw *p);
void drawOpDraw_brushdot_ellipse(AppDraw *p);
void drawOpDraw_brushdot_lineSuccConc(AppDraw *p);

mlkbool drawOp_xorline_to_bezier(AppDraw *p);
mlkbool drawOp_dragBrushSize_press(AppDraw *p);

/* func1 */

mlkbool drawOp_common_release_none(AppDraw *p);

void drawOpDraw_fillRect(AppDraw *p);
void drawOpDraw_fillEllipse(AppDraw *p);
void drawOpDraw_fillPolygon(AppDraw *p);
void drawOpDraw_gradation(AppDraw *p);

mlkbool drawOp_fill_press(AppDraw *p);
mlkbool drawOp_movetool_press(AppDraw *p,int subno);

mlkbool drawOp_stamp_press(AppDraw *p,int subno);
void drawOp_setStampImage(AppDraw *p);

mlkbool drawOp_canvasMove_press(AppDraw *p);
mlkbool drawOp_canvasRotate_press(AppDraw *p);
mlkbool drawOp_canvasZoom_press(AppDraw *p);

mlkbool drawOp_spoit_press(AppDraw *p,int subno,mlkbool enable_state);

/* func2 */

void drawOp_setSelect(AppDraw *p,int type);

mlkbool drawOp_selectFill_press(AppDraw *p);
mlkbool drawOp_selectMove_press(AppDraw *p);
mlkbool drawOp_selimgmove_press(AppDraw *p,mlkbool cut);

mlkbool drawOp_cutpaste_press(AppDraw *p);

void drawOp_boxsel_nograb_motion(AppDraw *p);
mlkbool drawOp_boxsel_press(AppDraw *p);

/* text */

mlkbool drawOp_drawtext_press(AppDraw *p);
void drawOp_drawtext_press_rbtt(AppDraw *p);
void drawOp_drawtext_dblclk(AppDraw *p);

/* xor */

mlkbool drawOpXor_line_press(AppDraw *p,int sub);
mlkbool drawOpXor_rect_canv_press(AppDraw *p,int sub);
mlkbool drawOpXor_rect_image_press(AppDraw *p,int sub);
mlkbool drawOpXor_ellipse_press(AppDraw *p,int sub);
mlkbool drawOpXor_sumline_press(AppDraw *p,int sub);
mlkbool drawOpXor_polygon_press(AppDraw *p,int sub);
mlkbool drawOpXor_lasso_press(AppDraw *p,int sub);
mlkbool drawOpXor_rulepoint_press(AppDraw *p);

void drawOpXor_drawline(AppDraw *p);
void drawOpXor_drawRect_canv(AppDraw *p);
void drawOpXor_drawRect_image(AppDraw *p);
void drawOpXor_drawEllipse(AppDraw *p);
void drawOpXor_drawBezier(AppDraw *p,mlkbool erase);
void drawOpXor_drawPolygon(AppDraw *p);
void drawOpXor_drawLasso(AppDraw *p,mlkbool erase);
void drawOpXor_drawCrossPoint(AppDraw *p);
void drawOpXor_drawBrushSizeCircle(AppDraw *p,mlkbool erase);

