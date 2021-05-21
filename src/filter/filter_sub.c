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
 * フィルタ処理: サブ関数
 **************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_rand.h"
#include "mlk_color.h"

#include "def_draw.h"

#include "def_filterdraw.h"
#include "def_tileimage.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "layerlist.h"
#include "layeritem.h"

#include "pv_filter_sub.h"



/** TileImage 点描画関数を取得 */

void FilterSub_getPixelFunc(TileImageSetPixelFunc *func)
{
	*func = g_tileimage_dinfo.func_setpixel;
}

/** TileImage 色計算関数をセット */

void FilterSub_setPixelColFunc(int coltype)
{
	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(coltype);
}

/** キャンバスイメージサイズを取得 */

void FilterSub_getImageSize(int *w,int *h)
{
	*w = APPDRAW->imgw;
	*h = APPDRAW->imgh;
}

/** DPI を取得 */

int FilterSub_getImageDPI(void)
{
	return APPDRAW->imgdpi;
}

/** チェックレイヤのリストをセットし、先頭のイメージを取得 */

TileImage *FilterSub_getCheckedLayerImage(void)
{
	LayerItem *item;

	item = LayerList_setLink_checked(APPDRAW->layerlist);

	return (item)? item->img: NULL;
}

/** タイプから、描画する色を取得 */

void FilterSub_getDrawColor_type(FilterDrawInfo *info,int type,void *dst)
{
	RGBA8 *pd8;
	RGBA16 *pd16;

	if(info->bits == 8)
	{
		pd8 = (RGBA8 *)dst;
		
		switch(type)
		{
			//描画色
			case DRAWCOL_TYPE_DRAWCOL:
				pd8->r = info->rgb_drawcol.c8.r;
				pd8->g = info->rgb_drawcol.c8.g;
				pd8->b = info->rgb_drawcol.c8.b;
				break;
			//背景色
			case DRAWCOL_TYPE_BKGNDCOL:
				pd8->r = info->rgb_bkgnd.c8.r;
				pd8->g = info->rgb_bkgnd.c8.g;
				pd8->b = info->rgb_bkgnd.c8.b;
				break;
			//黒
			case DRAWCOL_TYPE_BLACK:
				pd8->v32 = 0;
				break;
			//白
			default:
				pd8->r = pd8->g = pd8->b = 255;
				break;
		}

		pd8->a = 255;
	}
	else
	{
		pd16 = (RGBA16 *)dst;
		
		switch(type)
		{
			//描画色
			case DRAWCOL_TYPE_DRAWCOL:
				pd16->r = info->rgb_drawcol.c16.r;
				pd16->g = info->rgb_drawcol.c16.g;
				pd16->b = info->rgb_drawcol.c16.b;
				break;
			//背景色
			case DRAWCOL_TYPE_BKGNDCOL:
				pd16->r = info->rgb_bkgnd.c16.r;
				pd16->g = info->rgb_bkgnd.c16.g;
				pd16->b = info->rgb_bkgnd.c16.b;
				break;
			//黒
			case DRAWCOL_TYPE_BLACK:
				pd16->v64 = 0;
				break;
			//白
			default:
				pd16->r = pd16->g = pd16->b = COLVAL_16BIT;
				break;
		}

		pd16->a = COLVAL_16BIT;
	}
}


//========================
// 進捗
//========================


/** 最大値セット */

void FilterSub_prog_setMax(FilterDrawInfo *info,int max)
{
	mPopupProgressThreadSetMax(info->prog, max);
}

/** 位置を+1 */

void FilterSub_prog_inc(FilterDrawInfo *info)
{
	mPopupProgressThreadIncPos(info->prog);
}

/** サブステップ開始 */

void FilterSub_prog_substep_begin(FilterDrawInfo *info,int step,int max)
{
	mPopupProgressThreadSubStep_begin(info->prog, step, max);
}

/** サブステップ (1ステップのみ) 開始 */

void FilterSub_prog_substep_begin_onestep(FilterDrawInfo *info,int step,int max)
{
	mPopupProgressThreadSubStep_begin_onestep(info->prog, step, max);
}

/** サブステップ加算 */

void FilterSub_prog_substep_inc(FilterDrawInfo *info)
{
	mPopupProgressThreadSubStep_inc(info->prog);
}


//=======================
// 色々
//=======================


/** プレビュー時は、imgsrc の画像を imgdst にコピー
 *
 * - ソース画像の上に上書きするような描画を行う場合に使う。
 * - プレビューの描画範囲のイメージを、ソース画像と同じにする。
 * - imgdst は空状態である。 */

void FilterSub_copySrcImage_forPreview(FilterDrawInfo *info)
{
	if(info->in_dialog)
		TileImage_copyImage_rect(info->imgdst, info->imgsrc, &info->rc);
}

/** ソース画像から色を取得 (クリッピング設定有効) */

void FilterSub_getPixelSrc_clip(FilterDrawInfo *info,int x,int y,void *dst)
{
	if(info->clipping)
		TileImage_getPixel_clip(info->imgsrc, x, y, dst);
	else
		TileImage_getPixel(info->imgsrc, x, y, dst);
}

/** img の rc 範囲内の色をバッファに取得 (8bit)
 *
 * clipping: キャンバス範囲外はクリッピング
 * return: TRUE でバッファ内の色がすべて透明 */

mlkbool FilterSub_getPixelBuf8(TileImage *img,const mRect *rc,uint8_t *buf,mlkbool clipping)
{
	mRect rc1 = *rc;
	int ix,iy,f = 0;

	for(iy = rc1.y1; iy <= rc1.y2; iy++)
	{
		for(ix = rc1.x1; ix <= rc1.x2; ix++, buf += 4)
		{
			if(clipping)
				TileImage_getPixel_clip(img, ix, iy, buf);
			else
				TileImage_getPixel(img, ix, iy, buf);

			if(buf[3]) f = 1;
		}
	}

	return (f == 0);
}

/** img の rc 範囲内の色をバッファに取得 (16bit) */

mlkbool FilterSub_getPixelBuf16(TileImage *img,const mRect *rc,uint16_t *buf,mlkbool clipping)
{
	mRect rc1 = *rc;
	int ix,iy,f = 0;

	for(iy = rc1.y1; iy <= rc1.y2; iy++)
	{
		for(ix = rc1.x1; ix <= rc1.x2; ix++, buf += 4)
		{
			if(clipping)
				TileImage_getPixel_clip(img, ix, iy, buf);
			else
				TileImage_getPixel(img, ix, iy, buf);

			if(buf[3]) f = 1;
		}
	}

	return (f == 0);
}

/** 円形の形状バッファ (1bit) を作成
 *
 * pixnum: ONになっている点の数が入る */

uint8_t *FilterSub_createShapeBuf_circle(int radius,int *pixnum)
{
	uint8_t *buf,*pd,f;
	int ix,iy,xx,yy,rr,size,num = 0;

	size = radius * 2 + 1;

	buf = (uint8_t *)mMalloc0((size * size + 7) >> 3);
	if(!buf) return NULL;

	rr = radius * radius;
	pd = buf;
	f = 0x80;

	for(iy = 0; iy < size; iy++)
	{
		yy = iy - radius;
		yy *= yy;

		for(ix = 0, xx = -radius; ix < size; ix++, xx++)
		{
			if(xx * xx + yy <= rr)
			{
				*pd |= f;
				num++;
			}

			f >>= 1;
			if(!f) f = 0x80, pd++;
		}
	}

	*pixnum = num;

	return buf;
}


//===========================
// 点描画
// (ランダム点描画/縁取り)
//===========================

enum
{
	_DRAWPOINT_COLTYPE_DRAWCOL,
	_DRAWPOINT_COLTYPE_RAND_GRAY,
	_DRAWPOINT_COLTYPE_RAND_RGB,
	_DRAWPOINT_COLTYPE_RAND_HUE,
	_DRAWPOINT_COLTYPE_RAND_S,
	_DRAWPOINT_COLTYPE_RAND_V
};

/* 描画色取得 */

static void _drawpoint_get_color(FilterDrawPointInfo *dat,void *dst)
{
	RGBA8 *pd8;
	RGBA16 *pd16;
	mRandSFMT *rand = dat->info->rand;
	mRGBd rgbd;
	mRGB8 rgb8;
	int i;

	if(dat->info->bits == 8)
	{
		//----- 8bit
		
		pd8 = (RGBA8 *)dst;
		
		switch(dat->coltype)
		{
			//描画色
			case _DRAWPOINT_COLTYPE_DRAWCOL:
				pd8->r = dat->info->rgb_drawcol.c8.r;
				pd8->g = dat->info->rgb_drawcol.c8.g;
				pd8->b = dat->info->rgb_drawcol.c8.b;
				break;
			//ランダム (グレイスケール)
			case _DRAWPOINT_COLTYPE_RAND_GRAY:
				pd8->r = pd8->g = pd8->b = mRandSFMT_getIntRange(rand, 0, 255);
				break;
			//ランダム (RGB)
			case _DRAWPOINT_COLTYPE_RAND_RGB:
				for(i = 0; i < 3; i++)
					pd8->ar[i] = mRandSFMT_getIntRange(rand, 0, 255);
				break;
			//ランダム (色相/彩度/明度)
			case _DRAWPOINT_COLTYPE_RAND_HUE:
			case _DRAWPOINT_COLTYPE_RAND_S:
			case _DRAWPOINT_COLTYPE_RAND_V:
				if(dat->coltype == _DRAWPOINT_COLTYPE_RAND_HUE)
					//H
					mHSV_to_RGB8(&rgb8, mRandSFMT_getIntRange(rand, 0, 359) / 60.0, 1, 1);
				else if(dat->coltype == _DRAWPOINT_COLTYPE_RAND_S)
					//S
					mHSV_to_RGB8(&rgb8, dat->h, mRandSFMT_getIntRange(rand, 0, 255) / 255.0, dat->v);
				else
					//V
					mHSV_to_RGB8(&rgb8, dat->h, dat->s, mRandSFMT_getIntRange(rand, 0, 255) / 255.0);

				pd8->r = rgb8.r;
				pd8->g = rgb8.g;
				pd8->b = rgb8.b;
				break;
		}
	}
	else
	{
		//---- 16bit
		
		pd16 = (RGBA16 *)dst;
		
		switch(dat->coltype)
		{
			//描画色
			case _DRAWPOINT_COLTYPE_DRAWCOL:
				pd16->r = dat->info->rgb_drawcol.c16.r;
				pd16->g = dat->info->rgb_drawcol.c16.g;
				pd16->b = dat->info->rgb_drawcol.c16.b;
				break;
			//ランダム (グレイスケール)
			case _DRAWPOINT_COLTYPE_RAND_GRAY:
				pd16->r = pd16->g = pd16->b = mRandSFMT_getIntRange(rand, 0, COLVAL_16BIT);
				break;
			//ランダム (RGB)
			case _DRAWPOINT_COLTYPE_RAND_RGB:
				for(i = 0; i < 3; i++)
					pd16->ar[i] = mRandSFMT_getIntRange(rand, 0, COLVAL_16BIT);
				break;
			//ランダム (色相/彩度/明度)
			case _DRAWPOINT_COLTYPE_RAND_HUE:
			case _DRAWPOINT_COLTYPE_RAND_S:
			case _DRAWPOINT_COLTYPE_RAND_V:
				if(dat->coltype == _DRAWPOINT_COLTYPE_RAND_HUE)
					//H
					mHSV_to_RGBd(&rgbd, mRandSFMT_getIntRange(rand, 0, 359) / 60.0, 1, 1);
				else if(dat->coltype == _DRAWPOINT_COLTYPE_RAND_S)
					//S
					mHSV_to_RGBd(&rgbd, dat->h, mRandSFMT_getIntRange(rand, 0, 1000) / 1000.0, dat->v);
				else
					//V
					mHSV_to_RGBd(&rgbd, dat->h, dat->s, mRandSFMT_getIntRange(rand, 0, 1000) / 1000.0);

				pd16->r = (int)(rgbd.r * COLVAL_16BIT + 0.5);
				pd16->g = (int)(rgbd.g * COLVAL_16BIT + 0.5);
				pd16->b = (int)(rgbd.b * COLVAL_16BIT + 0.5);
				break;
		}
	}
}

/* 1px 点を打つ
 *
 * da: 濃度に適用する値 (0.0-1.0) */

static void _drawpoint_setpixel(int x,int y,void *col,double da,FilterDrawPointInfo *dat)
{
	mRect rc;

	//縁取り時のマスク

	if(dat->masktype)
	{
		if(dat->masktype == 1)
		{
			//外側:不透明部分には描画しない
			if(TileImage_isPixel_opaque(dat->imgsrc, x, y)) return;
		}
		else
		{
			//内側:透明部分には描画しない
			if(TileImage_isPixel_transparent(dat->imgsrc, x, y)) return;
		}
	}

	//描画範囲外には描画しない

	rc = dat->info->rc;

	if(rc.x1 <= x && x <= rc.x2 && rc.y1 <= y && y <= rc.y2)
	{
		if(dat->info->bits == 8)
			*((uint8_t *)col + 3) = (uint8_t)(255 * (dat->density * 0.01 * da) + 0.5);
		else
			*((uint16_t *)col + 3) = (int)(COLVAL_16BIT * (dat->density * 0.01 * da) + 0.5);
	
		(g_tileimage_dinfo.func_setpixel)(dat->imgdst, x, y, col);
	}
}

/* 点を打つ (ドット円) */

static void _drawpoint_setpixel_dot(int x,int y,FilterDrawPointInfo *dat)
{
	int ix,iy,yy,r,rr;
	uint64_t col;

	_drawpoint_get_color(dat, &col);

	r = dat->radius;
	rr = r * r;

	for(iy = -r; iy <= r; iy++)
	{
		yy = iy * iy;

		for(ix = -r; ix <= r; ix++)
		{
			if(ix * ix + yy <= rr)
				_drawpoint_setpixel(x + ix, y + iy, &col, 1.0, dat);
		}
	}
}

/* 点を打つ (アンチエイリアス円) */

static void _drawpoint_setpixel_aa(int x,int y,FilterDrawPointInfo *dat)
{
	int ix,iy,i,j,xtbl[4],ytbl[4],r,rr,cnt;
	uint64_t col;

	_drawpoint_get_color(dat, &col);

	r = dat->radius;
	rr = r << 2;
	rr *= rr;

	for(iy = -r; iy <= r; iy++)
	{
		//Y テーブル

		for(i = 0, j = (iy << 2) - 2; i < 4; i++, j++)
			ytbl[i] = j * j;

		//
	
		for(ix = -r; ix <= r; ix++)
		{
			//X テーブル

			for(i = 0, j = (ix << 2) - 2; i < 4; i++, j++)
				xtbl[i] = j * j;

			//4x4

			cnt = 0;

			for(i = 0; i < 4; i++)
			{
				for(j = 0; j < 4; j++)
				{
					if(xtbl[j] + ytbl[i] < rr) cnt++;
				}
			}

			//セット

			if(cnt)
				_drawpoint_setpixel(x + ix, y + iy, &col, cnt / 16.0, dat);
		}
	}
}

/* 点を打つ (柔らかい円) */

static void _drawpoint_setpixel_soft(int x,int y,FilterDrawPointInfo *dat)
{
	int ix,iy,i,j,xtbl[4],ytbl[4],r;
	double da,dr,rr;
	uint64_t col;

	_drawpoint_get_color(dat, &col);

	r = dat->radius;
	rr = 1.0 / ((r << 2) * (r << 2));

	for(iy = -r; iy <= r; iy++)
	{
		//Y テーブル

		for(i = 0, j = (iy << 2) - 2; i < 4; i++, j++)
			ytbl[i] = j * j;

		//
	
		for(ix = -r; ix <= r; ix++)
		{
			//X テーブル

			for(i = 0, j = (ix << 2) - 2; i < 4; i++, j++)
				xtbl[i] = j * j;

			//4x4

			da = 0;

			for(i = 0; i < 4; i++)
			{
				for(j = 0; j < 4; j++)
				{
					dr = (xtbl[j] + ytbl[i]) * rr;

					if(dr < 1.0) da += 1 - dr;
				}
			}

			//セット

			if(da != 0)
				_drawpoint_setpixel(x + ix, y + iy, &col, da / 16.0, dat);
		}
	}
}

static const FilterSubFunc_drawpoint_setpix g_drawpoint_funcs[3] = {
	_drawpoint_setpixel_dot, _drawpoint_setpixel_aa, _drawpoint_setpixel_soft
};

/** 点描画の初期化
 *
 * pixcoltype: TILEIMAGE_PIXELCOL_* */

void FilterSub_drawpoint_init(FilterDrawPointInfo *dat,int pixcoltype)
{
	mHSVd hsv;

	//プレビュー時は色計算を有効に

	if(dat->info->in_dialog)
		g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new_colfunc;

	//色計算

	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(pixcoltype);

	//描画色の HSV

	if(dat->coltype >= _DRAWPOINT_COLTYPE_RAND_S)
	{
		mRGB8_to_HSV(&hsv, dat->info->rgb_drawcol.c8.r,
			dat->info->rgb_drawcol.c8.g, dat->info->rgb_drawcol.c8.b);

		dat->h = hsv.h;
		dat->s = hsv.s;
		dat->v = hsv.v;
	}
}

/** 点描画の関数を取得 */

void FilterSub_drawpoint_getPixelFunc(int type,FilterSubFunc_drawpoint_setpix *dst)
{
	*dst = g_drawpoint_funcs[type];
}


//========================
// 平均色
//========================


/** 平均色用に色を追加 (8bit) */

void FilterSub_advcol_add8(double *d,uint8_t *buf)
{
	int a = buf[3];

	if(a == 255)
	{
		d[0] += buf[0];
		d[1] += buf[1];
		d[2] += buf[2];
		d[3] += 1.0;
	}
	else if(a)
	{
		double da = (double)a / 255;

		d[0] += buf[0] * da;
		d[1] += buf[1] * da;
		d[2] += buf[2] * da;
		d[3] += da;
	}
}

/** 平均色用に色を追加 (16bit) */

void FilterSub_advcol_add16(double *d,uint16_t *buf)
{
	int a = buf[3];

	if(a == COLVAL_16BIT)
	{
		d[0] += buf[0];
		d[1] += buf[1];
		d[2] += buf[2];
		d[3] += 1.0;
	}
	else if(a)
	{
		double da = (double)a / COLVAL_16BIT;

		d[0] += buf[0] * da;
		d[1] += buf[1] * da;
		d[2] += buf[2] * da;
		d[3] += da;
	}
}

/** 平均色用に色を追加 (8bit, 重みあり) */

void FilterSub_advcol_add_weight8(double *d,double weight,uint8_t *buf)
{
	int a = buf[3];

	if(a == 255)
	{
		d[0] += buf[0] * weight;
		d[1] += buf[1] * weight;
		d[2] += buf[2] * weight;
		d[3] += weight;
	}
	else if(a)
	{
		double da = (double)a / 255 * weight;

		d[0] += buf[0] * da;
		d[1] += buf[1] * da;
		d[2] += buf[2] * da;
		d[3] += da;
	}
}

/** 平均色用に色を追加 (16bit, 重みあり) */

void FilterSub_advcol_add_weight16(double *d,double weight,uint16_t *buf)
{
	int a = buf[3];

	if(a == COLVAL_16BIT)
	{
		d[0] += buf[0] * weight;
		d[1] += buf[1] * weight;
		d[2] += buf[2] * weight;
		d[3] += weight;
	}
	else if(a)
	{
		double da = (double)a / COLVAL_16BIT * weight;

		d[0] += buf[0] * da;
		d[1] += buf[1] * da;
		d[2] += buf[2] * da;
		d[3] += da;
	}
}

/** 平均色用に色を追加 (ビット指定, 位置指定) */

void FilterSub_advcol_add_point(TileImage *img,int x,int y,double *d,int bits,mlkbool clipping)
{
	uint64_t col;

	if(clipping)
		TileImage_getPixel_clip(img, x, y, &col);
	else
		TileImage_getPixel(img, x, y, &col);

	if(bits == 8)
		FilterSub_advcol_add8(d, (uint8_t *)&col);
	else
		FilterSub_advcol_add16(d, (uint16_t *)&col);
}

/** 平均色用に色を追加 (8bit, 位置指定, 重みあり) */

void FilterSub_advcol_add_point_weight8(TileImage *img,
	int x,int y,double *d,double weight,mlkbool clipping)
{
	uint8_t col[4];

	if(clipping)
		TileImage_getPixel_clip(img, x, y, col);
	else
		TileImage_getPixel(img, x, y, col);

	FilterSub_advcol_add_weight8(d, weight, col);
}

/** 平均色用に色を追加 (16bit, 位置指定, 重みあり) */

void FilterSub_advcol_add_point_weight16(TileImage *img,
	int x,int y,double *d,double weight,mlkbool clipping)
{
	uint16_t col[4];

	if(clipping)
		TileImage_getPixel_clip(img, x, y, col);
	else
		TileImage_getPixel(img, x, y, col);

	FilterSub_advcol_add_weight16(d, weight, col);
}


//-----------

/** 結果の平均色を取得 (ビット指定) */

void FilterSub_advcol_getColor(double *d,double weight_mul,void *buf,int bits)
{
	if(bits == 8)
		FilterSub_advcol_getColor8(d, weight_mul, (uint8_t *)buf);
	else
		FilterSub_advcol_getColor16(d, weight_mul, (uint16_t *)buf);
}

/** 結果の平均色を取得 (8bit) */

void FilterSub_advcol_getColor8(double *d,double weight_mul,uint8_t *buf)
{
	int a;
	double da;

	a = (int)(d[3] * weight_mul * 255 + 0.5);

	if(a == 0)
		*((uint32_t *)buf) = 0;
	else
	{
		da = 1.0 / d[3];

		buf[0] = (int)(d[0] * da + 0.5);
		buf[1] = (int)(d[1] * da + 0.5);
		buf[2] = (int)(d[2] * da + 0.5);
		buf[3] = a;
	}
}

/** 結果の平均色を取得 (16bit) */

void FilterSub_advcol_getColor16(double *d,double weight_mul,uint16_t *buf)
{
	int a;
	double da;

	a = (int)(d[3] * weight_mul * COLVAL_16BIT + 0.5);

	if(a == 0)
		*((uint64_t *)buf) = 0;
	else
	{
		da = 1.0 / d[3];

		buf[0] = (int)(d[0] * da + 0.5);
		buf[1] = (int)(d[1] * da + 0.5);
		buf[2] = (int)(d[2] * da + 0.5);
		buf[3] = a;
	}
}


//========================
// 線形補間で色取得
//========================


/* 指定位置の色取得 */

static void _linercolor_getpixel(TileImage *img,int x,int y,void *dst,uint8_t flags)
{
	int max;

	//位置をループ

	if(flags & LINERCOL_F_LOOP)
	{
		max = APPDRAW->imgw;

		if(x < 0) x = max + x % max;
		if(x >= max) x %= max;

		max = APPDRAW->imgh;

		if(y < 0) y = max + y % max;
		if(y >= max) y %= max;
	}

	//色取得

	if(flags & LINERCOL_F_CLIPPING)
		TileImage_getPixel_clip(img, x, y, dst);
	else
		TileImage_getPixel(img, x, y, dst);
}

/** 線形補間で指定位置の色取得
 *
 * flags: bit0=クリッピング, bit1=ループ */

void FilterSub_getLinerColor(TileImage *img,double x,double y,void *dst,int bits,uint8_t flags)
{
	double d1,d2,fx1,fx2,fy1,fy2;
	int ix,iy,i,n;
	RGBA8 c8[4],*pd8;
	RGBA16 c16[4],*pd16;

	d2 = modf(x, &d1); if(d2 < 0) d2 += 1;
	ix = floor(d1);
	fx1 = d2;
	fx2 = 1 - d2;

	d2 = modf(y, &d1); if(d2 < 0) d2 += 1;
	iy = floor(d1);
	fy1 = d2;
	fy2 = 1 - d2;

	//

	if(bits == 8)
	{
		//---- 8bit

		pd8 = (RGBA8 *)dst;
		
		//周囲4点の色取得

		_linercolor_getpixel(img, ix, iy, c8, flags);
		_linercolor_getpixel(img, ix + 1, iy, c8 + 1, flags);
		_linercolor_getpixel(img, ix, iy + 1, c8 + 2, flags);
		_linercolor_getpixel(img, ix + 1, iy + 1, c8 + 3, flags);

		//全て透明なら透明に

		if(c8[0].a + c8[1].a + c8[2].a + c8[3].a == 0)
		{
			pd8->v32 = 0;
			return;
		}

		//アルファ値

		d1 = fy2 * (fx2 * c8[0].a + fx1 * c8[1].a) +
			 fy1 * (fx2 * c8[2].a + fx1 * c8[3].a);

		pd8->a = (int)(d1 + 0.5);

		//RGB

		if(pd8->a == 0)
			pd8->v32 = 0;
		else
		{
			d1 = 1.0 / d1;
			
			for(i = 0; i < 3; i++)
			{
				d2 = fy2 * (fx2 * c8[0].ar[i] * c8[0].a + fx1 * c8[1].ar[i] * c8[1].a) +
					 fy1 * (fx2 * c8[2].ar[i] * c8[2].a + fx1 * c8[3].ar[i] * c8[3].a);

				n = (int)(d2 * d1 + 0.5);
				if(n > 255) n = 255;

				pd8->ar[i] = n;
			}
		}
	}
	else
	{
		//---- 16bit

		pd16 = (RGBA16 *)dst;
		
		//周囲4点の色取得

		_linercolor_getpixel(img, ix, iy, c16, flags);
		_linercolor_getpixel(img, ix + 1, iy, c16 + 1, flags);
		_linercolor_getpixel(img, ix, iy + 1, c16 + 2, flags);
		_linercolor_getpixel(img, ix + 1, iy + 1, c16 + 3, flags);

		//全て透明なら透明に

		if(c16[0].a + c16[1].a + c16[2].a + c16[3].a == 0)
		{
			pd16->v64 = 0;
			return;
		}

		//アルファ値

		d1 = fy2 * (fx2 * c16[0].a + fx1 * c16[1].a) +
			 fy1 * (fx2 * c16[2].a + fx1 * c16[3].a);

		pd16->a = (int)(d1 + 0.5);

		//RGB

		if(pd16->a == 0)
			pd16->v64 = 0;
		else
		{
			d1 = 1.0 / d1;
			
			for(i = 0; i < 3; i++)
			{
				d2 = fy2 * (fx2 * c16[0].ar[i] * c16[0].a + fx1 * c16[1].ar[i] * c16[1].a) +
					 fy1 * (fx2 * c16[2].ar[i] * c16[2].a + fx1 * c16[3].ar[i] * c16[3].a);

				n = (int)(d2 * d1 + 0.5);
				if(n > COLVAL_16BIT) n = COLVAL_16BIT;

				pd16->ar[i] = n;
			}
		}
	}
}

/** 線形補間色取得 (イメージ範囲外時の背景指定あり)
 *
 * x,y: 背景そのままの場合に色を取得する位置
 * type: 背景指定 (0:透明, 1:端の色, 2:元のまま) */

void FilterSub_getLinerColor_bkgnd(TileImage *img,double dx,double dy,
	int x,int y,void *dst,int bits,int type)
{
	int nx,ny;

	nx = floor(dx);
	ny = floor(dy);

	if(type == 1
		|| (nx >= 0 && nx < APPDRAW->imgw && ny >= 0 && ny < APPDRAW->imgh))
	{
		//背景は端の色、または位置が範囲内の場合、クリッピングして取得

		FilterSub_getLinerColor(img, dx, dy, dst, bits, LINERCOL_F_CLIPPING);
	}
	else if(type == 0)
	{
		//範囲外:透明

		if(bits == 8)
			*((uint32_t *)dst) = 0;
		else
			*((uint64_t *)dst) = 0;
	}
	else if(type == 2)
	{
		//範囲外:元のまま
		TileImage_getPixel(img, x, y, dst);
	}
}


