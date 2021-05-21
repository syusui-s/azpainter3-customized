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
 * AppDraw: 操作のメイン関数
 **********************************/

mlkbool drawOp_onPress(AppDraw *p,mEvent *ev);
mlkbool drawOp_onRelease(AppDraw *p,mEvent *ev);
void drawOp_onMotion(AppDraw *p,mEvent *ev);
mlkbool drawOp_onLBttDblClk(AppDraw *p,mEvent *ev);
mlkbool drawOp_onKeyDown(AppDraw *p,uint32_t key);
void drawOp_onUngrab(AppDraw *p);

