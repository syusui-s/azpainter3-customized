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

/*********************************************
 * CanvasSlider
 *********************************************/

typedef struct _CanvasSlider CanvasSlider;

//タイプ
enum
{
	CANVASSLIDER_TYPE_ZOOM,	//表示倍率
	CANVASSLIDER_TYPE_ANGLE	//回転角度
};

//通知タイプ
enum
{
	CANVASSLIDER_N_BAR_PRESS,		//バー上でドラッグが開始された (param1=現在値)
	CANVASSLIDER_N_BAR_MOTION,		//(ドラッグ中) 値が変更した時のみ
	CANVASSLIDER_N_BAR_RELEASE,
	CANVASSLIDER_N_PRESSED_BUTTON	//ボタンが押された
};

//ボタン番号 (param1)
enum
{
	CANVASSLIDER_BTT_SUB,
	CANVASSLIDER_BTT_ADD,
	CANVASSLIDER_BTT_RESET
};

CanvasSlider *CanvasSlider_new(mWidget *parent,int type,int zoom_max);
void CanvasSlider_setValue(CanvasSlider *p,int val);
int CanvasSlider_getMax(CanvasSlider *p);

