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

/*****************************************
 * AppDraw: ツールリスト/ブラシ
 *****************************************/

typedef struct _BrushDrawParam BrushDrawParam;

mlkbool drawToolList_isnotBrushItem(void);
int drawToolList_getRegistItemNo(ToolListItem *item);
mlkbool drawToolList_isRegistItem_inGroup(ToolListGroupItem *group);
BrushEditData *drawToolList_getBrush(void);

mlkbool drawToolList_setSelItem(AppDraw *p,ToolListItem *item);
mlkbool drawToolList_selectMove(AppDraw *p,mlkbool next);
mlkbool drawToolList_toggleSelect_last(AppDraw *p);

mlkbool drawToolList_deleteItem(ToolListItem *item);
void drawToolList_deleteGroup(ToolListGroupItem *item);
void drawToolList_copyItem(AppDraw *p,ToolListItem *item);
void drawToolList_setRegistItem(ToolListItem *item,int no);
void drawToolList_releaseRegistAll(void);

void drawToolList_setBrushDrawParam(int regno);
ImageMaterial *drawToolList_getBrushDrawInfo(int regno,BrushDrawParam **ppdraw,BrushParam **ppparam);

/* ブラシ項目値変更 */

mlkbool drawBrushParam_save(void);
void drawBrushParam_saveSimple(void);

int drawBrushParam_toggleAutoSave(void);
mlkbool drawBrushParam_setType(int val);
void drawBrushParam_setSize(int val);
void drawBrushParam_setSizeUnit(int val);
void drawBrushParam_setSizeOpt(int min,int max);
void drawBrushParam_setOpacity(int val);
void drawBrushParam_setHoseiType(int val);
void drawBrushParam_setHoseiVal(int val);
void drawBrushParam_setPixelMode(int val);
void drawBrushParam_setInterval(int val);
void drawBrushParam_setTexturePath(const char *path);
void drawBrushParam_toggleAntialias(void);
void drawBrushParam_toggleCurve(void);
void drawBrushParam_setRandomSize(int val);
void drawBrushParam_setRandomPos(int val);
void drawBrushParam_setWaterParam(int no,int val);
void drawBrushParam_toggleWaterTpWhite(void);
void drawBrushParam_setShapePath(const char *path);
void drawBrushParam_setShapeHard(int val);
void drawBrushParam_setShapeSand(int val);
void drawBrushParam_setAngleBase(int val);
void drawBrushParam_setAngleRandom(int val);
void drawBrushParam_toggleShapeRotateDir(void);
void drawBrushParam_setPressureSize(int val);
void drawBrushParam_setPressureOpacity(int val);
void drawBrushParam_togglePressureCommon(void);
void drawBrushParam_changePressureCurve(void);

