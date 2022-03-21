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

/****************************
 * AppDraw 用定義
 ****************************/

/* 新規作成時の値 */

typedef struct _NewCanvasValue
{
	mSize size;
	int dpi,bit,layertype;
	uint32_t imgbkcol;
}NewCanvasValue;

/* 画像読み込み用 */

typedef struct _LoadImageOption
{
	uint8_t bits,		//8,16
		ignore_alpha;	//アルファチャンネル無効
}LoadImageOption;

/* レイヤ新規作成/設定時の値 */

typedef struct _LayerNewOptInfo
{
	mStr strname,
		strtexpath;	//テクスチャ
	int type,		//-1=フォルダ,下位bit=タイプ,0x80=テキスト
		blendmode,
		opacity,	//0-128
		tone_lines,	//1=0.1
		tone_angle,
		tone_density;
	uint8_t ftone,		//トーン化
		ftone_white;	//トーン:白を背景に
	uint32_t col;
}LayerNewOptInfo;

