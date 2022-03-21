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
 * 塗りタイプ定義
 **********************************/

enum
{
	/* 通常ブラシ用 */

	PIXELMODE_BLEND_PIXEL = 0,	//ピクセル重ね塗り
	PIXELMODE_BLEND_STROKE,		//ストローク重ね塗り
	PIXELMODE_COMPARE_A,		//A比較上書き
	PIXELMODE_OVERWRITE_SHAPE,	//形状上書き
	PIXELMODE_OVERWRITE_SQUARE,	//矩形上書き
	PIXELMODE_DODGE,			//覆い焼き
	PIXELMODE_BURN,				//焼き込み
	PIXELMODE_ADD,				//加算

	PIXELMODE_BRUSH_NUM,

	/* 消しゴム用 */

	PIXELMODE_ERASE_PIXEL = 0,
	PIXELMODE_ERASE_STROKE,

	PIXELMODE_ERASE_NUM,

	/* ツール用 */

	PIXELMODE_TOOL_BLEND = 0,	//重ね塗り
	PIXELMODE_TOOL_COMPARE_A,	//A比較上書き
	PIXELMODE_TOOL_OVERWRITE,	//上書き
	PIXELMODE_TOOL_ERASE		//消しゴム
};

