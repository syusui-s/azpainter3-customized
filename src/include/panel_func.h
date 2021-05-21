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

/*******************************
 * 各パネルの関数
 *******************************/

typedef struct _LayerItem LayerItem;

//ツール

void PanelTool_changeTool(void);
void PanelTool_changeToolSub(void);

//ツールリスト

void PanelToolList_updateList(void);
void PanelToolList_updateBrushSize(void);
void PanelToolList_updateBrushOpacity(void);

//ブラシ設定

void PanelBrushOpt_setValue(void);
void PanelBrushOpt_updateBrushSize(void);
void PanelBrushOpt_updateBrushOpacity(void);

//オプション

void PanelOption_changeTool(void);
void PanelOption_changeStampImage(void);

//レイヤ

void PanelLayer_update_all(void);
void PanelLayer_update_curparam(void);
void PanelLayer_update_curlayer(mlkbool setparam);
void PanelLayer_update_layer(LayerItem *item);
void PanelLayer_update_layer_curparam(LayerItem *item);
void PanelLayer_update_changecurrent_visible(LayerItem *lastitem,mlkbool update_all);

//カラー

void PanelColor_changeDrawColor(void);
void PanelColor_setColor_fromWheel(uint32_t col);
void PanelColor_setColor_fromOther(uint32_t col);
void PanelColor_changeColorMask_color1(void);

//カラーホイール

void PanelColorWheel_setColor(uint32_t col);

//キャンバス操作

void PanelCanvasCtrl_setValue(void);

//キャンバスビュー

void PanelCanvasView_changeImageSize(void);
void PanelCanvasView_update(void);
void PanelCanvasView_updateBox(const mBox *boximg);


