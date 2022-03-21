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

/**************************************
 * フィルタ処理: カラー/アルファ値
 **************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_rand.h"

#include "def_draw.h"

#include "tileimage.h"
#include "layerlist.h"
#include "layeritem.h"
#include "imagematerial.h"

#include "draw_op_sub.h"

#include "def_filterdraw.h"
#include "pv_filter_sub.h"


//========================
// 共通
//========================


/* HSV,HSL 調整時の作業用値セット */

static void _set_hsv_hsl_tmpval(FilterDrawInfo *info)
{
	info->ntmp[0] = info->val_bar[0];

	if(info->bits == 8)
	{
		info->ntmp[1] = (int)(info->val_bar[1] / 100.0 * 255 + 0.5);
		info->ntmp[2] = (int)(info->val_bar[2] / 100.0 * 255 + 0.5);
	}
	else
	{
		info->ntmp[1] = (int)(info->val_bar[1] / 100.0 * COLVAL_16BIT + 0.5);
		info->ntmp[2] = (int)(info->val_bar[2] / 100.0 * COLVAL_16BIT + 0.5);
	}
}


//========================
// カラー
//========================


/** 明度・コントラスト調整
 *
 * 明度: -100〜100
 * コントラスト: -100〜100 */

static void _colfunc_brightcont8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	uint8_t *tbl = (uint8_t *)info->ptmp[0];
	int i,val[3];

	//コントラスト

	for(i = 0; i < 3; i++)
		val[i] = tbl[col->ar[i]];

	//明度

	if(info->val_bar[0])
	{
		FilterSub_RGBtoYCrCb_8bit(val);

		val[0] += info->ntmp[0];  //Y に加算

		FilterSub_YCrCbtoRGB_8bit(val);
	}

	for(i = 0; i < 3; i++)
		col->ar[i] = val[i];
}

static void _colfunc_brightcont16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	uint16_t *tbl = (uint16_t *)info->ptmp[0];
	int i,val[3];

	//コントラスト

	for(i = 0; i < 3; i++)
		val[i] = tbl[col->ar[i]];

	//明度

	if(info->val_bar[0])
	{
		FilterSub_RGBtoYCrCb_16bit(val);

		val[0] += info->ntmp[0];  //Y に加算

		FilterSub_YCrCbtoRGB_16bit(val);
	}

	for(i = 0; i < 3; i++)
		col->ar[i] = val[i];
}

mlkbool FilterDraw_color_brightcont(FilterDrawInfo *info)
{
	uint8_t *buf,*pd8;
	uint16_t *pd16;
	int i,n,mul;

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	mul = (int)(info->val_bar[1] / 100.0 * 256 + 0.5);

	if(info->bits == 8)
	{
		info->ntmp[0] = (int)(info->val_bar[0] / 100.0 * 255 + 0.5);

		pd8 = buf;
		
		for(i = 0; i < 256; i++)
		{
			n = i + ((i - 128) * mul >> 8);

			if(n < 0) n = 0;
			else if(n > 255) n = 255;

			*(pd8++) = n;
		}
	}
	else
	{
		info->ntmp[0] = (int)(info->val_bar[0] / 100.0 * COLVAL_16BIT + 0.5);

		pd16 = (uint16_t *)buf;
		
		for(i = 0; i <= COLVAL_16BIT; i++)
		{
			n = i + ((i - 0x4000) * mul >> 8);

			if(n < 0) n = 0;
			else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

			*(pd16++) = n;
		}
	}

	FilterSub_proc_color(info, _colfunc_brightcont8, _colfunc_brightcont16);

	mFree(buf);

	return TRUE;
}

/** ガンマ補正 */

mlkbool FilterDraw_color_gamma(FilterDrawInfo *info)
{
	uint8_t *buf,*pd8;
	uint16_t *pd16;
	int i,n;
	double d;

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	d = 1.0 / (info->val_bar[0] * 0.01);

	if(info->bits == 8)
	{
		pd8 = buf;

		for(i = 0; i < 256; i++)
		{
			n = round(pow((double)i / 255, d) * 255);

			if(n < 0) n = 0;
			else if(n > 255) n = 255;

			*(pd8++) = n;
		}
	}
	else
	{
		pd16 = (uint16_t *)buf;
		
		for(i = 0; i <= COLVAL_16BIT; i++)
		{
			n = round(pow((double)i / COLVAL_16BIT, d) * COLVAL_16BIT);

			if(n < 0) n = 0;
			else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

			*(pd16++) = n;
		}
	}

	FilterSub_proc_color(info,
		FilterSub_func_color_table_8bit, FilterSub_func_color_table_16bit);

	mFree(buf);

	return TRUE;
}

/** レベル補正 */

mlkbool FilterDraw_color_level(FilterDrawInfo *info)
{
	uint8_t *buf,*pd8;
	uint16_t *pd16;
	int i,n,in_min,in_mid,in_max,out_min,out_max,out_mid;
	double d1,d2,d;

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	in_min = info->val_bar[0];
	in_mid = info->val_bar[1];
	in_max = info->val_bar[2];
	out_min = info->val_bar[3];
	out_max = info->val_bar[4];

	out_mid = (out_max - out_min) >> 1;

	d1 = (double)out_mid / (in_mid - in_min);
	d2 = (double)(out_max - out_min - out_mid) / (in_max - in_mid);

	if(info->bits == 8)
	{
		pd8 = buf;
		
		for(i = 0; i <= 255; i++)
		{
			//入力制限

			n = i;

			if(n < in_min) n = in_min;
			else if(n > in_max) n = in_max;

			//補正

			if(n < in_mid)
				d = (n - in_min) * d1;
			else
				d = (n - in_mid) * d2 + out_mid;
			
			*(pd8++) = (uint8_t)(d + out_min + 0.5);
		}
	}
	else
	{
		pd16 = (uint16_t *)buf;
		
		for(i = 0; i <= COLVAL_16BIT; i++)
		{
			//入力制限

			n = i;

			if(n < in_min) n = in_min;
			else if(n > in_max) n = in_max;

			//補正

			if(n < in_mid)
				d = (n - in_min) * d1;
			else
				d = (n - in_mid) * d2 + out_mid;
			
			*(pd16++) = (int)(d + out_min + 0.5);
		}
	}

	FilterSub_proc_color(info,
		FilterSub_func_color_table_8bit, FilterSub_func_color_table_16bit);

	mFree(buf);

	return TRUE;
}

/** RGB 補正
 *
 * R,G,B : -100〜100 */

static void _colfunc_rgb8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = col->ar[i] + info->ntmp[i];

		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		col->ar[i] = n;
	}
}

static void _colfunc_rgb16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = col->ar[i] + info->ntmp[i];

		if(n < 0) n = 0;
		else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

		col->ar[i] = n;
	}
}

mlkbool FilterDraw_color_rgb(FilterDrawInfo *info)
{
	int i;

	if(info->bits == 8)
	{
		for(i = 0; i < 3; i++)
			info->ntmp[i] = (int)(info->val_bar[i] / 100.0 * 255 + 0.5);
	}
	else
	{
		for(i = 0; i < 3; i++)
			info->ntmp[i] = (int)(info->val_bar[i] / 100.0 * COLVAL_16BIT + 0.5);
	}

	return FilterSub_proc_color(info, _colfunc_rgb8, _colfunc_rgb16);
}

/** HSV 調整 */

static void _colfunc_hsv8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int val[3],i,n;

	for(i = 0; i < 3; i++)
		val[i] = col->ar[i];

	FilterSub_RGBtoHSV_8bit(val);

	for(i = 0; i < 3; i++)
	{
		n = val[i] + info->ntmp[i];

		if(i == 0)
		{
			if(n < 0) n += 360;
			else if(n >= 360) n -= 360;
		}
		else
		{
			if(n < 0) n = 0;
			else if(n > 255) n = 255;
		}

		val[i] = n;
	}

	FilterSub_HSVtoRGB_8bit(val);

	for(i = 0; i < 3; i++)
		col->ar[i] = val[i];
}

static void _colfunc_hsv16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int val[3],i,n;

	for(i = 0; i < 3; i++)
		val[i] = col->ar[i];

	FilterSub_RGBtoHSV_16bit(val);

	for(i = 0; i < 3; i++)
	{
		n = val[i] + info->ntmp[i];

		if(i == 0)
		{
			if(n < 0) n += 360;
			else if(n >= 360) n -= 360;
		}
		else
		{
			if(n < 0) n = 0;
			else if(n > COLVAL_16BIT) n = COLVAL_16BIT;
		}

		val[i] = n;
	}

	FilterSub_HSVtoRGB_16bit(val);

	for(i = 0; i < 3; i++)
		col->ar[i] = val[i];
}

mlkbool FilterDraw_color_hsv(FilterDrawInfo *info)
{
	_set_hsv_hsl_tmpval(info);

	return FilterSub_proc_color(info, _colfunc_hsv8, _colfunc_hsv16);
}

/** HSL 調整 */

static void _colfunc_hsl8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int val[3],i,n;

	for(i = 0; i < 3; i++)
		val[i] = col->ar[i];

	FilterSub_RGBtoHSL_8bit(val);

	for(i = 0; i < 3; i++)
	{
		n = val[i] + info->ntmp[i];

		if(i == 0)
		{
			if(n < 0) n += 360;
			else if(n >= 360) n -= 360;
		}
		else
		{
			if(n < 0) n = 0;
			else if(n > 255) n = 255;
		}

		val[i] = n;
	}

	FilterSub_HSLtoRGB_8bit(val);

	for(i = 0; i < 3; i++)
		col->ar[i] = val[i];
}

static void _colfunc_hsl16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int val[3],i,n;

	for(i = 0; i < 3; i++)
		val[i] = col->ar[i];

	FilterSub_RGBtoHSL_16bit(val);

	for(i = 0; i < 3; i++)
	{
		n = val[i] + info->ntmp[i];

		if(i == 0)
		{
			if(n < 0) n += 360;
			else if(n >= 360) n -= 360;
		}
		else
		{
			if(n < 0) n = 0;
			else if(n > COLVAL_16BIT) n = COLVAL_16BIT;
		}

		val[i] = n;
	}

	FilterSub_HSLtoRGB_16bit(val);

	for(i = 0; i < 3; i++)
		col->ar[i] = val[i];
}

mlkbool FilterDraw_color_hsl(FilterDrawInfo *info)
{
	_set_hsv_hsl_tmpval(info);

	return FilterSub_proc_color(info, _colfunc_hsl8, _colfunc_hsl16);
}

/** ネガポジ反転 */

static void _colfunc_nega8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	col->r = 255 - col->r;
	col->g = 255 - col->g;
	col->b = 255 - col->b;
}

static void _colfunc_nega16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	col->r = COLVAL_16BIT - col->r;
	col->g = COLVAL_16BIT - col->g;
	col->b = COLVAL_16BIT - col->b;
}

mlkbool FilterDraw_color_nega(FilterDrawInfo *info)
{
	return FilterSub_proc_color(info, _colfunc_nega8, _colfunc_nega16);
}

/** グレイスケール */

static void _colfunc_grayscale8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	col->r = col->g = col->b = RGB_TO_LUM(col->r, col->g, col->b);
}

static void _colfunc_grayscale16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	col->r = col->g = col->b = RGB_TO_LUM(col->r, col->g, col->b);
}

mlkbool FilterDraw_color_grayscale(FilterDrawInfo *info)
{
	return FilterSub_proc_color(info, _colfunc_grayscale8, _colfunc_grayscale16);
}

/** セピアカラー */

static void _colfunc_sepia8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int v;

	v = RGB_TO_LUM(col->r, col->g, col->b);

	col->r = (v + 14 > 255)? 255: v + 14;
	col->g = v;
	col->b = (v - 8 < 0)? 0: v - 8;
}

static void _colfunc_sepia16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int v;

	v = RGB_TO_LUM(col->r, col->g, col->b);

	col->r = (v + 1928 > COLVAL_16BIT)? COLVAL_16BIT: v + 1928;
	col->g = v;
	col->b = (v - 964 < 0)? 0: v - 964;
}

mlkbool FilterDraw_color_sepia(FilterDrawInfo *info)
{
	return FilterSub_proc_color(info, _colfunc_sepia8, _colfunc_sepia16);
}

/** グラデーションマップ */

static void _colfunc_gradmap8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int v;

	v = RGB_TO_LUM(col->r, col->g, col->b);

	TileImage_getGradationColor(col, (double)v / 255, (TileImageDrawGradInfo *)info->ptmp[0]);
}

static void _colfunc_gradmap16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int v;

	v = RGB_TO_LUM(col->r, col->g, col->b);

	TileImage_getGradationColor(col, (double)v / COLVAL_16BIT, (TileImageDrawGradInfo *)info->ptmp[0]);
}

mlkbool FilterDraw_color_gradmap(FilterDrawInfo *info)
{
	TileImageDrawGradInfo ginfo;

	drawOpSub_setDrawGradationInfo(APPDRAW, &ginfo);

	if(ginfo.buf)
	{
		info->ptmp[0] = &ginfo;

		FilterSub_proc_color(info, _colfunc_gradmap8, _colfunc_gradmap16);

		mFree(ginfo.buf);
	}

	return TRUE;
}

//=======================

/** 2値化 */

static void _colfunc_threshold8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int v;

	v = RGB_TO_LUM(col->r, col->g, col->b);

	v = (v >= info->val_bar[0])? 255: 0;

	col->r = col->g = col->b = v;
}

static void _colfunc_threshold16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int v;

	v = RGB_TO_LUM(col->r, col->g, col->b);

	v = (v >= info->val_bar[0])? COLVAL_16BIT: 0;

	col->r = col->g = col->b = v;
}

mlkbool FilterDraw_color_threshold(FilterDrawInfo *info)
{
	return FilterSub_proc_color(info, _colfunc_threshold8, _colfunc_threshold16);
}

/** 2値化 (ディザ)
 *
 * 0:bayer2x2, 1:bayer4x4, 2:渦巻き4x4, 3:網点4x4, 4:ランダム */

static const uint8_t g_dither_bayer2x2[2][2] = {{0,2},{3,1}},
	g_dither_4x4[3][4][4] = {
	{{0 ,8,2,10}, {12,4,14, 6}, {3,11, 1,9}, {15, 7,13, 5}},
	{{13,7,6,12}, { 8,1, 0, 5}, {9, 2, 3,4}, {14,10,11,15}},
	{{10,4,6, 8}, {12,0, 2,14}, {7, 9,11,5}, { 3,15,13, 1}}
};

static void _colfunc_threshold_dither8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int v,cmp;

	//比較値

	switch(info->val_combo[0])
	{
		case 0:
			cmp = g_dither_bayer2x2[y & 1][x & 1] * 255 >> 2;
			break;
		case 1:
		case 2:
		case 3:
			cmp = g_dither_4x4[info->val_combo[0] - 1][y & 3][x & 3] * 255 >> 4;
			break;
		case 4:
			cmp = mRandSFMT_getIntRange(info->rand, 0, 254);
			break;
	}

	//

	v = RGB_TO_LUM(col->r, col->g, col->b);
	v = (v <= cmp)? 0: 255;

	col->r = col->g = col->b = v;
}

static void _colfunc_threshold_dither16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int v,cmp;

	//比較値

	switch(info->val_combo[0])
	{
		case 0:
			cmp = g_dither_bayer2x2[y & 1][x & 1] << (15 - 2);
			break;
		case 1:
		case 2:
		case 3:
			cmp = g_dither_4x4[info->val_combo[0] - 1][y & 3][x & 3] << (15 - 4);
			break;
		case 4:
			cmp = mRandSFMT_getIntRange(info->rand, 0, COLVAL_16BIT - 1);
			break;
	}

	//

	v = RGB_TO_LUM(col->r, col->g, col->b);
	v = (v <= cmp)? 0: COLVAL_16BIT;

	col->r = col->g = col->b = v;
}

mlkbool FilterDraw_color_threshold_dither(FilterDrawInfo *info)
{
	return FilterSub_proc_color(info, _colfunc_threshold_dither8, _colfunc_threshold_dither16);
}

/** ポスタリゼーション */

mlkbool FilterDraw_color_posterize(FilterDrawInfo *info)
{
	uint8_t *buf,*pd8;
	uint16_t *pd16;
	int i,level,n;

	buf = FilterSub_createColorTable(info);
	if(!buf) return FALSE;

	level = info->val_bar[0];

	if(info->bits == 8)
	{
		pd8 = buf;
		
		for(i = 0; i < 255; i++)
		{
			n = i * level / 255;

			*(pd8++) = (n == level - 1)? 255: n * 255 / (level - 1);
		}

		*pd8 = 255;
	}
	else
	{
		pd16 = (uint16_t *)buf;
		
		for(i = 0; i < COLVAL_16BIT; i++)
		{
			n = i * level >> 15;

			*(pd16++) = (n == level - 1)? COLVAL_16BIT: (n << 15) / (level - 1);
		}

		*pd16 = COLVAL_16BIT;
	}
	
	FilterSub_proc_color(info,
		FilterSub_func_color_table_8bit, FilterSub_func_color_table_16bit);

	mFree(buf);

	return TRUE;
}


//=============================
// 色置換
//=============================


/** 描画色置換 */

static void _colfunc_repdrawcol8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	if(col->r == info->rgb_drawcol.c8.r
		&& col->g == info->rgb_drawcol.c8.g
		&& col->b == info->rgb_drawcol.c8.b)
	{
		col->r = info->val_bar[0];
		col->g = info->val_bar[1];
		col->b = info->val_bar[2];
	}
}

static void _colfunc_repdrawcol16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	if(col->r == info->rgb_drawcol.c16.r
		&& col->g == info->rgb_drawcol.c16.g
		&& col->b == info->rgb_drawcol.c16.b)
	{
		col->r = info->ntmp[0];
		col->g = info->ntmp[1];
		col->b = info->ntmp[2];
	}
}

mlkbool FilterDraw_color_replace_drawcol(FilterDrawInfo *info)
{
	int i;

	//val_bar の値は 8bit RGB

	if(info->bits == 16)
	{
		for(i = 0; i < 3; i++)
			info->ntmp[i] = Color8bit_to_16bit(info->val_bar[i]);
	}

	return FilterSub_proc_color(info, _colfunc_repdrawcol8, _colfunc_repdrawcol16);
}

/** 色置換 (共通)
 *
 * return: TRUE で色を変更 */

static mlkbool _pixfunc_colreplace8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	mlkbool fdrawcol = FALSE;

	//描画色かどうか

	if(col->a && info->ntmp[0] < 3)
	{
		fdrawcol = (col->r == info->rgb_drawcol.c8.r
			&& col->g == info->rgb_drawcol.c8.g
			&& col->b == info->rgb_drawcol.c8.b);
	}

	//

	switch(info->ntmp[0])
	{
		//描画色を透明に
		case 0:
			if(fdrawcol)
			{
				col->v32 = 0;
				return TRUE;
			}
			break;
		//描画色以外を透明に
		case 1:
			if(col->a && !fdrawcol)
			{
				col->v32 = 0;
				return TRUE;
			}
			break;
		//描画色を背景色に
		case 2:
			if(fdrawcol)
			{
				col->r = info->rgb_bkgnd.c8.r;
				col->g = info->rgb_bkgnd.c8.g;
				col->b = info->rgb_bkgnd.c8.b;
				return TRUE;
			}
			break;
		//透明色を描画色に
		case 3:
			if(!col->a)
			{
				col->r = info->rgb_drawcol.c8.r;
				col->g = info->rgb_drawcol.c8.g;
				col->b = info->rgb_drawcol.c8.b;
				col->a = 255;
				return TRUE;
			}
			break;
	}

	return FALSE;
}

static mlkbool _pixfunc_colreplace16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	mlkbool fdrawcol = FALSE;

	//描画色かどうか

	if(col->a && info->ntmp[0] < 3)
	{
		fdrawcol = (col->r == info->rgb_drawcol.c16.r
			&& col->g == info->rgb_drawcol.c16.g
			&& col->b == info->rgb_drawcol.c16.b);
	}

	//

	switch(info->ntmp[0])
	{
		//描画色を透明に
		case 0:
			if(fdrawcol)
			{
				col->v64 = 0;
				return TRUE;
			}
			break;
		//描画色以外を透明に
		case 1:
			if(col->a && !fdrawcol)
			{
				col->v64 = 0;
				return TRUE;
			}
			break;
		//描画色を背景色に
		case 2:
			if(fdrawcol)
			{
				col->r = info->rgb_bkgnd.c16.r;
				col->g = info->rgb_bkgnd.c16.g;
				col->b = info->rgb_bkgnd.c16.b;
				return TRUE;
			}
			break;
		//透明色を描画色に
		case 3:
			if(!col->a)
			{
				col->r = info->rgb_drawcol.c16.r;
				col->g = info->rgb_drawcol.c16.g;
				col->b = info->rgb_drawcol.c16.b;
				col->a = COLVAL_16BIT;
				return TRUE;
			}
			break;
	}

	return FALSE;
}

mlkbool FilterDraw_color_replace(FilterDrawInfo *info)
{
	return FilterSub_proc_pixel(info, _pixfunc_colreplace8, _pixfunc_colreplace16);
}


//=============================
// アルファ操作
//=============================


/** アルファ操作 (チェックレイヤ) */

static mlkbool _pixfunc_alpha_checked8(FilterDrawInfo *info,int x,int y,RGBA8 *dst)
{
	LayerItem *pi = (LayerItem *)info->ptmp[0];
	RGBA8 c;
	int type,a;

	type = info->ntmp[0];
	a = dst->a;

	switch(type)
	{
		//透明な部分を透明に
		//不透明な部分を透明に
		case 0:
		case 1:
			//すべてのレイヤが透明なら pi == NULL
			
			for(; pi; pi = pi->link)
			{
				TileImage_getPixel(pi->img, x, y, &c);
				if(c.a) break;
			}

			if((type == 0 && !pi) || (type == 1 && pi))
				a = 0;
			break;
		//値を合成してコピー (描画先で透明な部分は除く)
		case 2:
			if(a)
			{
				//各レイヤのアルファ値を合成
				
				for(a = 0; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a = (((a + c.a) << 4) - (a * c.a << 4) / 255 + 8) >> 4;
				}
			}
			break;
		//値を足す
		case 3:
			if(a != 255)
			{
				for(; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a += c.a;

					if(a >= 255)
					{
						a = 255;
						break;
					}
				}
			}
			break;
		//値を引く
		case 4:
			if(a)
			{
				for(; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a -= c.a;

					if(a <= 0)
					{
						a = 0;
						break;
					}
				}
			}
			break;
		//値を乗算
		case 5:
			if(a)
			{
				for(; pi && a; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a = (a * c.a + 127) / 255;
				}
			}
			break;
		//(単体) 明るい色ほど透明に
		//(単体) 暗い色ほど透明に
		case 6:
		case 7:
			if(a)
			{
				TileImage_getPixel(pi->img, x, y, &c);

				if(c.a)
				{
					a = RGB_TO_LUM(c.r, c.g, c.b);
					if(type == 6) a = 255 - a;
				}
			}
			break;
	}

	//アルファ値セット

	if(a == dst->a)
		return FALSE;
	else
	{
		if(a == 0)
			dst->v32 = 0;
		else
			dst->a = a;

		return TRUE;
	}
}

static mlkbool _pixfunc_alpha_checked16(FilterDrawInfo *info,int x,int y,RGBA16 *dst)
{
	LayerItem *pi = (LayerItem *)info->ptmp[0];
	RGBA16 c;
	int type,a;

	type = info->ntmp[0];
	a = dst->a;

	switch(type)
	{
		//透明な部分を透明に
		//不透明な部分を透明に
		case 0:
		case 1:
			//すべてのレイヤが透明なら pi == NULL
			
			for(; pi; pi = pi->link)
			{
				TileImage_getPixel(pi->img, x, y, &c);
				if(c.a) break;
			}

			if((type == 0 && !pi) || (type == 1 && pi))
				a = 0;
			break;
		//値を合成してコピー (描画先で透明な部分は除く)
		case 2:
			if(a)
			{
				for(a = 0; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a = (((a + c.a) << 4) - (a * c.a >> (15 - 4)) + 8) >> 4;
				}
			}
			break;
		//値を足す
		case 3:
			if(a != COLVAL_16BIT)
			{
				for(; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a += c.a;

					if(a >= COLVAL_16BIT)
					{
						a = COLVAL_16BIT;
						break;
					}
				}
			}
			break;
		//値を引く
		case 4:
			if(a)
			{
				for(; pi; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a -= c.a;

					if(a <= 0)
					{
						a = 0;
						break;
					}
				}
			}
			break;
		//値を乗算
		case 5:
			if(a)
			{
				for(; pi && a; pi = pi->link)
				{
					TileImage_getPixel(pi->img, x, y, &c);

					a = (a * c.a + 0x4000) >> 15;
				}
			}
			break;
		//(単体) 明るい色ほど透明に
		//(単体) 暗い色ほど透明に
		case 6:
		case 7:
			if(a)
			{
				TileImage_getPixel(pi->img, x, y, &c);

				if(c.a)
				{
					a = RGB_TO_LUM(c.r, c.g, c.b);
					if(type == 6) a = COLVAL_16BIT - a;
				}
			}
			break;
	}

	//アルファ値セット

	if(a == dst->a)
		return FALSE;
	else
	{
		if(a == 0)
			dst->v64 = 0;
		else
			dst->a = a;

		return TRUE;
	}
}

mlkbool FilterDraw_alpha_checked(FilterDrawInfo *info)
{
	//チェックレイヤのリンクをセット

	info->ptmp[0] = LayerList_setLink_checked(APPDRAW->layerlist);
	
	return FilterSub_proc_pixel(info, _pixfunc_alpha_checked8, _pixfunc_alpha_checked16);
}

/** アルファ操作 (カレント) */

static mlkbool _pixfunc_alpha_current8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int a;

	a = col->a;

	switch(info->ntmp[0])
	{
		//明るい色ほど透明に
		case 0:
			if(a)
				a = 255 - RGB_TO_LUM(col->r, col->g, col->b);
			break;
		//暗い色ほど透明に
		case 1:
			if(a)
				a = RGB_TO_LUM(col->r, col->g, col->b);
			break;
		//透明以外を最大値に
		case 2:
			if(a) a = 255;
			break;
		//テクスチャ適用
		case 3:
			if(a)
				a = (a * ImageMaterial_getPixel_forTexture(APPDRAW->imgmat_opttex, x, y) + 127) / 255;
			break;
		//アルファ値からグレイスケール作成 ([!] RGB も変更)
		case 4:
			col->r = col->g = col->b = a;
			col->a = 255;
			return TRUE;
	}

	//アルファ値セット

	if(a == col->a)
		return FALSE;
	else
	{
		if(a == 0)
			col->v32 = 0;
		else
			col->a = a;

		return TRUE;
	}
}

static mlkbool _pixfunc_alpha_current16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int a;

	a = col->a;

	switch(info->ntmp[0])
	{
		//明るい色ほど透明に
		case 0:
			if(a)
				a = COLVAL_16BIT - RGB_TO_LUM(col->r, col->g, col->b);
			break;
		//暗い色ほど透明に
		case 1:
			if(a)
				a = RGB_TO_LUM(col->r, col->g, col->b);
			break;
		//透明以外を最大値に
		case 2:
			if(a) a = COLVAL_16BIT;
			break;
		//テクスチャ適用
		case 3:
			if(a)
				a = (a * ImageMaterial_getPixel_forTexture(APPDRAW->imgmat_opttex, x, y) + 127) / 255;
			break;
		//アルファ値からグレイスケール作成 ([!] RGB も変更)
		case 4:
			col->r = col->g = col->b = a;
			col->a = COLVAL_16BIT;
			return TRUE;
	}

	//アルファ値セット

	if(a == col->a)
		return FALSE;
	else
	{
		if(a == 0)
			col->v64 = 0;
		else
			col->a = a;

		return TRUE;
	}
}

mlkbool FilterDraw_alpha_current(FilterDrawInfo *info)
{
	return FilterSub_proc_pixel(info, _pixfunc_alpha_current8, _pixfunc_alpha_current16);
}

