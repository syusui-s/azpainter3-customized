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

/**************************************
 * フィルタ処理: カラー関連関数
 **************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"

#include "def_filterdraw.h"
#include "def_tileimage.h"
#include "tileimage.h"

#include "pv_filter_sub.h"


/** 色変換テーブル作成 */

uint8_t *FilterSub_createColorTable(FilterDrawInfo *info)
{
	uint8_t *buf;

	buf = (uint8_t *)mMalloc((info->bits == 8)? 256: (0x8000 + 1) * 2);
	if(!buf) return NULL;

	info->ptmp[0] = buf;

	return buf;
}

/** FilterSub_proc_color 時の共通関数 (ptmp[0] のテーブルから変換) */

void FilterSub_func_color_table_8bit(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	uint8_t *tbl = (uint8_t *)info->ptmp[0];

	col->r = tbl[col->r];
	col->g = tbl[col->g];
	col->b = tbl[col->b];
}

void FilterSub_func_color_table_16bit(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	uint16_t *tbl = (uint16_t *)info->ptmp[0];

	col->r = tbl[col->r];
	col->g = tbl[col->g];
	col->b = tbl[col->b];
}


//=============================
// 色変換 (RGB <-> YCrCb)
//=============================

/*
  [RGB -> YCrCb]
   Y  = 0.299 * R + 0.587 * G + 0.114 * B
   Cr = 0.5 * R - 0.418688 * G - 0.081312 * B
   Cb = -0.168736 * R - 0.331264 * G + 0.5 * B

  [YCrCb -> RGB]
   R = Y + 1.402 * Cr
   G = Y - 0.344136 * Cb - 0.714136 * Cr
   B = Y + 1.772 * Cb
*/


/** RGB -> YCrCb 変換 (8bit) */

void FilterSub_RGBtoYCrCb_8bit(int *val)
{
	int c[3],i,n;

	c[0] = (val[0] * 4899 + val[1] * 9617 + val[2] * 1868) >> 14;
	c[1] = ((val[0] * 8192 - val[1] * 6860 - val[2] * 1332) >> 14) + 128;
	c[2] = ((val[0] * -2765 - val[1] * 5427 + val[2] * 8192) >> 14) + 128;

	for(i = 0; i < 3; i++)
	{
		n = c[i];
		
		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		val[i] = n;
	}
}

/** RGB -> YCrCb 変換 (16bit) */

void FilterSub_RGBtoYCrCb_16bit(int *val)
{
	int c[3],i,n;

	c[0] = (val[0] * 4899 + val[1] * 9617 + val[2] * 1868) >> 14;
	c[1] = ((val[0] * 8192 - val[1] * 6860 - val[2] * 1332) >> 14) + 0x4000;
	c[2] = ((val[0] * -2765 - val[1] * 5427 + val[2] * 8192) >> 14) + 0x4000;

	for(i = 0; i < 3; i++)
	{
		n = c[i];
		
		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		val[i] = n;
	}
}

/** YCrCb -> RGB 変換 (8bit) */

void FilterSub_YCrCbtoRGB_8bit(int *val)
{
	int c[3],i,cr,cb,n;

	cr = val[1] - 128;
	cb = val[2] - 128;

	c[0] = val[0] + (cr * 22970 >> 14);
	c[1] = val[0] - ((cb * 5638 + cr * 11700) >> 14);
	c[2] = val[0] + (cb * 29032 >> 14);

	for(i = 0; i < 3; i++)
	{
		n = c[i];
		
		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		val[i] = n;
	}
}

/** YCrCb -> RGB 変換 (16bit) */

void FilterSub_YCrCbtoRGB_16bit(int *val)
{
	int c[3],i,cr,cb,n;

	cr = val[1] - 0x4000;
	cb = val[2] - 0x4000;

	c[0] = val[0] + (cr * 22970 >> 14);
	c[1] = val[0] - ((cb * 5638 + cr * 11700) >> 14);
	c[2] = val[0] + (cb * 29032 >> 14);

	for(i = 0; i < 3; i++)
	{
		n = c[i];

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		val[i] = n;
	}
}


//=============================
// 色変換 (RGB <-> HSV)
//=============================


/* RGB -> 色相 */

static int _rgb_to_hue(int r,int g,int b,int *pmin,int *pmax)
{
	int min,max,h;

	min = (r <= g)? r: g;
	if(b < min) min = b;

	max = (r <= g)? g: r;
	if(max < b) max = b;

	h = max - min;

	if(h != 0)
	{
		if(max == r)
			h = (g - b) * 60 / h;
		else if(max == g)
			h = (b - r) * 60 / h + 120;
		else
			h = (r - g) * 60 / h + 240;

		if(h < 0)
			h += 360;
		else if(h >= 360)
			h -= 360;
	}

	*pmin = min;
	*pmax = max;

	return h;
}

/** RGB -> HSV (8bit) */

void FilterSub_RGBtoHSV_8bit(int *val)
{
	int min,max,h;

	h = _rgb_to_hue(val[0], val[1], val[2], &min, &max);

	val[0] = h;
	val[1] = (max == 0)? 0: (int)((double)(max - min) / max * 255 + 0.5);
	val[2] = max;
}

/** RGB -> HSV (16bit) */

void FilterSub_RGBtoHSV_16bit(int *val)
{
	int min,max,h;

	h = _rgb_to_hue(val[0], val[1], val[2], &min, &max);

	val[0] = h;
	val[1] = (max == 0)? 0: (int)((double)(max - min) / max * 0x8000 + 0.5);
	val[2] = max;
}

/** HSV -> RGB (8bit) */

void FilterSub_HSVtoRGB_8bit(int *val)
{
	int r,g,b,t1,t2,t3,s,v,hh;
	double ds,dv,df;

	s = val[1];
	v = val[2];

	if(s == 0)
		r = g = b = v;
	else
	{
		hh = val[0] / 60;
		
		ds = (double)s / 255;
		dv = (double)v / 255;
		df = (val[0] - hh * 60) / 60.0;

		t1 = (int)(dv * (1 - ds) * 255 + 0.5);
		t2 = (int)(dv * (1 - ds * df) * 255 + 0.5);
		t3 = (int)(dv * (1 - ds * (1 - df)) * 255 + 0.5);

		switch(hh)
		{
			case 0: r = v, g = t3, b = t1; break;
			case 1: r = t2, g = v, b = t1; break;
			case 2: r = t1, g = v, b = t3; break;
			case 3: r = t1, g = t2, b = v; break;
			case 4: r = t3, g = t1, b = v; break;
			default: r = v, g = t1, b = t2;
		}
	}

	val[0] = r;
	val[1] = g;
	val[2] = b;
}

/** HSV -> RGB (16bit) */

void FilterSub_HSVtoRGB_16bit(int *val)
{
	int r,g,b,t1,t2,t3,s,v,hh;
	double ds,dv,df;

	s = val[1];
	v = val[2];

	if(s == 0)
		r = g = b = v;
	else
	{
		hh = val[0] / 60;
		
		ds = (double)s / 0x8000;
		dv = (double)v / 0x8000;
		df = (val[0] - hh * 60) / 60.0;

		t1 = (int)(dv * (1 - ds) * 0x8000 + 0.5);
		t2 = (int)(dv * (1 - ds * df) * 0x8000 + 0.5);
		t3 = (int)(dv * (1 - ds * (1 - df)) * 0x8000 + 0.5);

		switch(hh)
		{
			case 0: r = v, g = t3, b = t1; break;
			case 1: r = t2, g = v, b = t1; break;
			case 2: r = t1, g = v, b = t3; break;
			case 3: r = t1, g = t2, b = v; break;
			case 4: r = t3, g = t1, b = v; break;
			default: r = v, g = t1, b = t2;
		}
	}

	val[0] = r;
	val[1] = g;
	val[2] = b;
}


//=============================
// 色変換 (RGB <-> HSL)
//=============================


/* HSL -> RGB (各値から RGB 値いずれか取得) */

static double _hsltorgb_get_rgb(int h,double min,double max)
{
	if(h >= 360) h -= 360;
	else if(h < 0) h += 360;

	if(h < 60)
		return min + (max - min) * (h / 60.0);
	else if(h < 180)
		return max;
	else if(h < 240)
		return min + (max - min) * ((240 - h) / 60.0);
	else
		return min;
}

/** RGB -> HSL (8bit) */

void FilterSub_RGBtoHSL_8bit(int *val)
{
	int min,max,l,s;

	val[0] = _rgb_to_hue(val[0], val[1], val[2], &min, &max);

	l = (max + min) / 2;
	s = max - min;

	if(s)
	{
		if(l < 128)
			s = (int)((double)s / (max + min) * 255 + 0.5);
		else
			s = (int)((double)s / (255 * 2 - max - min) * 255 + 0.5);
	}

	val[1] = s;
	val[2] = l;
}

/** RGB -> HSL (16bit) */

void FilterSub_RGBtoHSL_16bit(int *val)
{
	int min,max,l,s;

	val[0] = _rgb_to_hue(val[0], val[1], val[2], &min, &max);

	l = (max + min) / 2;
	s = max - min;

	if(s)
	{
		if(l <= 0x4000)
			s = (int)((double)s / (max + min) * 0x8000 + 0.5);
		else
			s = (int)((double)s / (0x8000 * 2 - max - min) * 0x8000 + 0.5);
	}

	val[1] = s;
	val[2] = l;
}

/** HSL -> RGB (8bit) */

void FilterSub_HSLtoRGB_8bit(int *val)
{
	int h,s,l;
	double max,min,ds,dl;

	h = val[0];
	s = val[1];
	l = val[2];

	if(s == 0)
		val[0] = val[1] = val[2] = l;
	else
	{
		ds = s / 255.0;
		dl = l / 255.0;

		if(l < 128)
			max = dl * (1 + ds);
		else
			max = dl * (1 - ds) + ds;

		min = 2 * dl - max;
	
		val[0] = (int)(_hsltorgb_get_rgb(h + 120, min, max) * 255 + 0.5);
		val[1] = (int)(_hsltorgb_get_rgb(h, min, max) * 255 + 0.5);
		val[2] = (int)(_hsltorgb_get_rgb(h - 120, min, max) * 255 + 0.5);
	}
}

/** HSL -> RGB (16bit) */

void FilterSub_HSLtoRGB_16bit(int *val)
{
	int h,s,l;
	double max,min,ds,dl;

	h = val[0];
	s = val[1];
	l = val[2];

	if(s == 0)
		val[0] = val[1] = val[2] = l;
	else
	{
		ds = (double)s / 0x8000;
		dl = (double)l / 0x8000;

		if(l < 0x4000)
			max = dl * (1 + ds);
		else
			max = dl * (1 - ds) + ds;

		min = 2 * dl - max;
	
		val[0] = (int)(_hsltorgb_get_rgb(h + 120, min, max) * 0x8000 + 0.5);
		val[1] = (int)(_hsltorgb_get_rgb(h, min, max) * 0x8000 + 0.5);
		val[2] = (int)(_hsltorgb_get_rgb(h - 120, min, max) * 0x8000 + 0.5);
	}
}


//=============================
// フィルタ処理
//=============================

/*
  - レイヤのカラータイプがアルファ値のみの場合、
    処理しても効果はないので、対象外。
    そのため、イメージは RGBA or GRAY のみ。
  
  - キャンバスプレビュー時、imgdst は imgsrc から複製されているため、
    タイル配列は同じ。
 
  - 透明な点は処理されないため、空タイルはスキップされる。
*/


/* キャンバスプレビュー時のフィルタ処理 (RGBA)
 *
 * [!] アルファ値は変化しない。 */

static void _proc_preview_rgba(FilterDrawInfo *info,
	FilterSubFunc_color8 func8,FilterSubFunc_color16 func16)
{
	TileImageTileRectInfo tinfo;
	TileImage *imgsel;
	uint8_t **ppsrc,**ppdst,*ps8,*pd8;
	uint16_t *ps16,*pd16;
	int px,py,tx,ty,ix,iy,xx,yy,bits;

	bits = info->bits;
	imgsel = info->imgsel;

	//処理範囲px内のタイル範囲

	ppsrc = TileImage_getTileRectInfo(info->imgsrc, &tinfo, &info->box);
	if(!ppsrc) return;

	ppdst = TILEIMAGE_GETTILE_BUFPT(info->imgdst, tinfo.rctile.x1, tinfo.rctile.y1);

	//タイルごとに、各ピクセルを処理
	// :処理範囲内か判定 & 選択範囲があれば、範囲内か判定

	py = tinfo.pxtop.y;

	for(ty = tinfo.rctile.y1; ty <= tinfo.rctile.y2; ty++, py += 64)
	{
		px = tinfo.pxtop.x;

		for(tx = tinfo.rctile.x1; tx <= tinfo.rctile.x2; tx++, px += 64, ppsrc++, ppdst++)
		{
			if(!(*ppsrc)) continue;

			//タイル単位で処理

			if(bits == 8)
			{
				//8bit
				
				ps8 = *ppsrc;
				pd8 = *ppdst;

				for(iy = 0, yy = py; iy < 64; iy++, yy++)
				{
					for(ix = 0, xx = px; ix < 64; ix++, xx++, ps8 += 4, pd8 += 4)
					{
						if(ps8[3]
							&& tinfo.rcclip.x1 <= xx && xx < tinfo.rcclip.x2
							&& tinfo.rcclip.y1 <= yy && yy < tinfo.rcclip.y2
							&& (!imgsel || TileImage_isPixel_opaque(imgsel, xx, yy)))
						{
							*((uint32_t *)pd8) = *((uint32_t *)ps8);
							
							(func8)(info, xx, yy, (RGBA8 *)pd8);
						}
					}
				}
			}
			else
			{
				//16bit
				
				ps16 = (uint16_t *)*ppsrc;
				pd16 = (uint16_t *)*ppdst;

				for(iy = 0, yy = py; iy < 64; iy++, yy++)
				{
					for(ix = 0, xx = px; ix < 64; ix++, xx++, ps16 += 4, pd16 += 4)
					{
						if(ps16[3]
							&& tinfo.rcclip.x1 <= xx && xx < tinfo.rcclip.x2
							&& tinfo.rcclip.y1 <= yy && yy < tinfo.rcclip.y2
							&& (!imgsel || TileImage_isPixel_opaque(imgsel, xx, yy)))
						{
							*((uint64_t *)pd16) = *((uint64_t *)ps16);
							
							(func16)(info, xx, yy, (RGBA16 *)pd16);
						}
					}
				}
			}

		}

		ppsrc += tinfo.pitch_tile;
		ppdst += tinfo.pitch_tile;
	}
}

/* キャンバスプレビュー時のフィルタ処理 (GRAY) */

static void _proc_preview_gray(FilterDrawInfo *info,
	FilterSubFunc_color8 func8,FilterSubFunc_color16 func16)
{
	TileImageTileRectInfo tinfo;
	TileImage *imgsel;
	uint8_t **ppsrc,**ppdst,*ps8,*pd8;
	uint16_t *ps16,*pd16;
	int px,py,tx,ty,ix,iy,xx,yy,bits;
	RGBA8 col8;
	RGBA16 col16;

	bits = info->bits;
	imgsel = info->imgsel;

	//処理範囲px内のタイル範囲

	ppsrc = TileImage_getTileRectInfo(info->imgsrc, &tinfo, &info->box);
	if(!ppsrc) return;

	ppdst = TILEIMAGE_GETTILE_BUFPT(info->imgdst, tinfo.rctile.x1, tinfo.rctile.y1);

	//タイルごとに、各ピクセルを処理

	py = tinfo.pxtop.y;

	for(ty = tinfo.rctile.y1; ty <= tinfo.rctile.y2; ty++, py += 64)
	{
		px = tinfo.pxtop.x;

		for(tx = tinfo.rctile.x1; tx <= tinfo.rctile.x2; tx++, px += 64, ppsrc++, ppdst++)
		{
			if(!(*ppsrc)) continue;

			//タイル単位で処理

			if(bits == 8)
			{
				//8bit
				
				ps8 = *ppsrc;
				pd8 = *ppdst;

				for(iy = 0, yy = py; iy < 64; iy++, yy++)
				{
					for(ix = 0, xx = px; ix < 64; ix++, xx++, ps8 += 2, pd8 += 2)
					{
						if(ps8[1]
							&& tinfo.rcclip.x1 <= xx && xx < tinfo.rcclip.x2
							&& tinfo.rcclip.y1 <= yy && yy < tinfo.rcclip.y2
							&& (!imgsel || TileImage_isPixel_opaque(imgsel, xx, yy)))
						{
							col8.r = col8.g = col8.b = ps8[0];
							col8.a = ps8[1];
							
							(func8)(info, xx, yy, &col8);

							pd8[0] = (col8.r + col8.g + col8.b) / 3;
						}
					}
				}
			}
			else
			{
				//16bit
				
				ps16 = (uint16_t *)*ppsrc;
				pd16 = (uint16_t *)*ppdst;

				for(iy = 0, yy = py; iy < 64; iy++, yy++)
				{
					for(ix = 0, xx = px; ix < 64; ix++, xx++, ps16 += 2, pd16 += 2)
					{
						if(ps16[1]
							&& tinfo.rcclip.x1 <= xx && xx < tinfo.rcclip.x2
							&& tinfo.rcclip.y1 <= yy && yy < tinfo.rcclip.y2
							&& (!imgsel || TileImage_isPixel_opaque(imgsel, xx, yy)))
						{
							col16.r = col16.g = col16.b = ps16[0];
							col16.a = ps16[1];
							
							(func16)(info, xx, yy, &col16);

							pd16[0] = (col16.r + col16.g + col16.b) / 3;
						}
					}
				}
			}

		}

		ppsrc += tinfo.pitch_tile;
		ppdst += tinfo.pitch_tile;
	}
}

/* 実際のフィルタ処理 (8bit)
 *
 * [!] グラデーションマップの場合はアルファ値が変化する場合あり。 */

static mlkbool _proc_real_8bit(FilterDrawInfo *info,FilterSubFunc_color8 func)
{
	TileImage *img;
	int ix,iy;
	mRect rc;
	RGBA8 col;

	img = info->imgdst;
	rc = info->rc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, &col);

			if(col.a)
			{
				(func)(info, ix, iy, &col);

				TileImage_setPixel_draw_direct(img, ix, iy, &col);
			}
		}

		mPopupProgressThreadSubStep_inc(info->prog);
	}

	return TRUE;
}

/* 実際のフィルタ処理 (16bit) */

static mlkbool _proc_real_16bit(FilterDrawInfo *info,FilterSubFunc_color16 func)
{
	TileImage *img;
	int ix,iy;
	mRect rc;
	RGBA16 col;

	img = info->imgdst;
	rc = info->rc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(img, ix, iy, &col);

			if(col.a)
			{
				(func)(info, ix, iy, &col);

				TileImage_setPixel_draw_direct(img, ix, iy, &col);
			}
		}

		mPopupProgressThreadSubStep_inc(info->prog);
	}

	return TRUE;
}

/** 色処理の共通フィルタ処理
 *
 * func8: 8bit 用色処理関数
 * func16: 16bit 用色処理関数 */

mlkbool FilterSub_proc_color(FilterDrawInfo *info,
	FilterSubFunc_color8 func8,FilterSubFunc_color16 func16)
{
	if(!info->in_dialog)
	{
		if(info->bits == 8)
			return _proc_real_8bit(info, func8);
		else
			return _proc_real_16bit(info, func16);
	}
	else
	{
		//プレビュー

		if(info->imgsrc->type == TILEIMAGE_COLTYPE_RGBA)
			_proc_preview_rgba(info, func8, func16);
		else
			_proc_preview_gray(info, func8, func16);

		return TRUE;
	}
}

