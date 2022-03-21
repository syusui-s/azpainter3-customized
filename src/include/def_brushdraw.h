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

/******************************
 * ブラシ描画用パラメータ
 ******************************/

typedef struct _ImageMaterial ImageMaterial;
typedef struct _BrushDrawParam BrushDrawParam;

struct _BrushDrawParam
{
	double shape_hard,		//円形形状の硬さパラメータ
		radius,				//半径 (px)
		opacity,			//最大濃度 [0-1]
		min_size,			//サイズ最小 [0-1]
		min_opacity,		//濃度最小 [0-1]
		interval,			//間隔
		rand_size_min,		//ランダムサイズ最小
		rand_pos_len,		//ランダム位置、距離
		water_param[2],		//水彩
		pressure_min,		//筆圧:線形補間時の出力値
		pressure_range;
	int angle,			//角度 [0-511]
		angle_rand,		//角度ランダム幅
		shape_sand,		//砂化 [0 で無効]
		blur_range;		//ぼかし半径
	uint32_t flags;

	uint16_t press_curve_tbl[(1<<12) + 1]; //筆圧カーブテーブル

	//MaterialList で管理するポインタ
	ImageMaterial *img_shape,	//NULL で通常円形
		*img_texture;
};

//フラグ
enum 
{
	BRUSHDP_F_WATER = 1<<0,				//水彩
	BRUSHDP_F_NO_ANTIALIAS = 1<<1,		//非アンチエイリアス
	BRUSHDP_F_CURVE        = 1<<2,		//曲線補間
	BRUSHDP_F_RANDOM_SIZE  = 1<<3,		//ランダムサイズあり
	BRUSHDP_F_RANDOM_POS   = 1<<4,		//ランダム位置あり
	BRUSHDP_F_TRAVELLING_DIR = 1<<5,	//進行方向
	BRUSHDP_F_OVERWRITE_TP   = 1<<6,	//ブラシ形状の透明部分も上書き
	BRUSHDP_F_SHAPE_HARD_MAX = 1<<7,	//円形形状の硬さは最大
	BRUSHDP_F_WATER_TP_WHITE = 1<<8,	//水彩、透明色は白として扱う
	BRUSHDP_F_PRESSURE_CURVE = 1<<9		//筆圧カーブあり (OFF で線形補間)
};

