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
 * フィルタ処理: 効果
 **************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rand.h"

#include "def_filterdraw.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"

#include "pv_filter_sub.h"


//=============================
// グロ―
//=============================


/* (8bit) ソース処理 */

static void _glow_setsrc8(void *buf,int num,FilterDrawInfo *info)
{
	uint8_t *pd = (uint8_t *)buf;
	int i,n,min;

	//明るさ補正

	min = info->ntmp[0];

	for(; num; num--, pd += 4)
	{
		if(pd[3])
		{
			for(i = 0; i < 3; i++)
			{
				n = pd[i];
				if(n < min) n = min;

				pd[i] = (n - min) * 255 / (255 - min);
			}
		}
	}
}

/* (16bit) ソース処理 */

static void _glow_setsrc16(void *buf,int num,FilterDrawInfo *info)
{
	uint16_t *pd = (uint16_t *)buf;
	int i,n,min;

	min = info->ntmp[0];

	for(; num; num--, pd += 4)
	{
		if(pd[3])
		{
			for(i = 0; i < 3; i++)
			{
				n = pd[i];
				if(n < min) n = min;

				pd[i] = ((n - min) << 15) / (COLVAL_16BIT - min);
			}
		}
	}
}

/* (8bit) 点のセット */

static void _glow_setpixel8(int x,int y,void *gcol,FilterDrawInfo *info)
{
	RGBA8 col,*pg;
	int i,n,a,src,dst;

	TileImage_getPixel(info->imgsrc, x, y, &col);

	if(!col.a) return;

	pg = (RGBA8 *)gcol;
	a = info->ntmp[1] * col.a >> 8;

	for(i = 0; i < 3; i++)
	{
		src = pg->ar[i] << 2;
		dst = col.ar[i] << 2;

		//スクリーン合成
		n = src + dst - (src * dst / (255 << 2));
		//アルファ合成
		n = (n - dst) * a / 255 + dst;

		col.ar[i] = (n + 2) >> 2;
	}

	(g_tileimage_dinfo.func_setpixel)(info->imgdst, x, y, &col);
}

/* (16bit) 点のセット */

static void _glow_setpixel16(int x,int y,void *gcol,FilterDrawInfo *info)
{
	RGBA16 col,*pg;
	int i,n,a,src,dst;

	TileImage_getPixel(info->imgsrc, x, y, &col);

	if(!col.a) return;

	pg = (RGBA16 *)gcol;
	a = info->ntmp[1] * col.a >> 8;

	for(i = 0; i < 3; i++)
	{
		src = pg->ar[i];
		dst = col.ar[i];

		//スクリーン合成
		n = src + dst - (src * dst >> 15);
		//アルファ合成
		n = ((n - dst) * a >> 15) + dst;

		col.ar[i] = n;
	}

	(g_tileimage_dinfo.func_setpixel)(info->imgdst, x, y, &col);
}

/** グロ― */

mlkbool FilterDraw_effect_glow(FilterDrawInfo *info)
{
	//[0]明度(1-100) [1]半径 [2]適用量(1-100)

	info->ntmp[0] = Density100_to_bitval(info->val_bar[0], info->bits);
	info->ntmp[1] = (info->val_bar[2] << 8) / 100;

	if(info->bits == 8)
	{
		info->ntmp[0] = 255 - info->ntmp[0];

		return FilterSub_proc_gaussblur8(info, info->imgsrc, info->val_bar[1],
			_glow_setpixel8, _glow_setsrc8);
	}
	else
	{
		info->ntmp[0] = COLVAL_16BIT - info->ntmp[0];

		return FilterSub_proc_gaussblur16(info, info->imgsrc, info->val_bar[1],
			_glow_setpixel16, _glow_setsrc16);
	}
}


//=============================
// RGB ずらし
//=============================
/* ntmp : [0]R-x [1]R-y [2]B-x [3]B-y */


/* 8bit */

static mlkbool _pixfunc_rgbshift8(FilterDrawInfo *info,int x,int y,RGBA8 *dst)
{
	RGBA8 col;

	if(!dst->a) return FALSE;

	//R

	FilterSub_getPixelSrc_clip(info, x + info->ntmp[0], y + info->ntmp[1], &col);
	dst->r = col.r;

	//B
	
	FilterSub_getPixelSrc_clip(info, x + info->ntmp[2], y + info->ntmp[3], &col);
	dst->b = col.b;

	return TRUE;
}

/* 16bit */

static mlkbool _pixfunc_rgbshift16(FilterDrawInfo *info,int x,int y,RGBA16 *dst)
{
	RGBA16 col;

	if(!dst->a) return FALSE;

	//R

	FilterSub_getPixelSrc_clip(info, x + info->ntmp[0], y + info->ntmp[1], &col);
	dst->r = col.r;

	//B
	
	FilterSub_getPixelSrc_clip(info, x + info->ntmp[2], y + info->ntmp[3], &col);
	dst->b = col.b;

	return TRUE;
}

/** RGB ずらし */

mlkbool FilterDraw_effect_rgbshift(FilterDrawInfo *info)
{
	int i;

	//R と B のずらす方向

	for(i = 0; i < 4; i++)
		info->ntmp[i] = 0;

	i = info->val_bar[0];	//距離

	switch(info->val_combo[0])
	{
		//斜め
		case 0:
			info->ntmp[0] = info->ntmp[1] = info->ntmp[3] = i;
			info->ntmp[2] = -i;
			break;
		//横
		case 1:
			info->ntmp[0] = i;
			info->ntmp[2] = -i;
			break;
		//縦
		default:
			info->ntmp[1] = i;
			info->ntmp[3] = -i;
			break;
	}

	return FilterSub_proc_pixel(info, _pixfunc_rgbshift8, _pixfunc_rgbshift16);
}


//=============================
// 油絵風
//=============================


/* 8bit */

static void _proc_oilpaint_8bit(FilterDrawInfo *info)
{
	int ix,iy,i,j,jx,jy,vmin,min[3],max[3];
	mPoint ptst[4],pted[4];
	double d[4],dweight;
	uint8_t col[4],colmin[4];
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	mMemset0(ptst, sizeof(mPoint) * 4);
	mMemset0(pted, sizeof(mPoint) * 4);

	i = info->val_bar[0];

	ptst[0].x = ptst[0].y = ptst[1].y = ptst[3].x = -i;
	pted[1].x = pted[2].x = pted[2].y = pted[3].y = i;

	dweight = 1.0 / ((i + 1) * (i + 1));

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			vmin = 0;

			for(i = 0; i < 4; i++)
			{
				d[0] = d[1] = d[2] = d[3] = 0;

				min[0] = min[1] = min[2] = 255;
				max[0] = max[1] = max[2] = 0;

				for(jy = ptst[i].y; jy <= pted[i].y; jy++)
				{
					for(jx = ptst[i].x; jx <= pted[i].x; jx++)
					{
						FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, col);

						FilterSub_advcol_add8(d, col);

						//RGB 最小、最大値

						for(j = 0; j < 3; j++)
						{
							if(col[j] < min[j]) min[j] = col[j];
							if(col[j] > max[j]) max[j] = col[j];
						}
					}
				}

				//最小

				j = (max[0] - min[0]) + (max[1] - min[1]) + (max[2] - min[2]);

				if(i == 0 || j < vmin)
				{
					vmin = j;
					FilterSub_advcol_getColor8(d, dweight, colmin);
				}
			}

			(setpix)(info->imgdst, ix, iy, colmin);
		}

		FilterSub_prog_substep_inc(info);
	}
}

/* 16bit */

static void _proc_oilpaint_16bit(FilterDrawInfo *info)
{
	int ix,iy,i,j,jx,jy,vmin,min[3],max[3];
	mPoint ptst[4],pted[4];
	double d[4],dweight;
	uint16_t col[4],colmin[4];
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	mMemset0(ptst, sizeof(mPoint) * 4);
	mMemset0(pted, sizeof(mPoint) * 4);

	i = info->val_bar[0];

	ptst[0].x = ptst[0].y = ptst[1].y = ptst[3].x = -i;
	pted[1].x = pted[2].x = pted[2].y = pted[3].y = i;

	dweight = 1.0 / ((i + 1) * (i + 1));

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			vmin = 0;

			for(i = 0; i < 4; i++)
			{
				d[0] = d[1] = d[2] = d[3] = 0;

				min[0] = min[1] = min[2] = COLVAL_16BIT;
				max[0] = max[1] = max[2] = 0;

				for(jy = ptst[i].y; jy <= pted[i].y; jy++)
				{
					for(jx = ptst[i].x; jx <= pted[i].x; jx++)
					{
						FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, col);

						FilterSub_advcol_add16(d, col);

						//RGB 最小、最大値

						for(j = 0; j < 3; j++)
						{
							if(col[j] < min[j]) min[j] = col[j];
							if(col[j] > max[j]) max[j] = col[j];
						}
					}
				}

				//最小

				j = (max[0] - min[0]) + (max[1] - min[1]) + (max[2] - min[2]);

				if(i == 0 || j < vmin)
				{
					vmin = j;
					FilterSub_advcol_getColor16(d, dweight, colmin);
				}
			}

			(setpix)(info->imgdst, ix, iy, colmin);
		}

		FilterSub_prog_substep_inc(info);
	}
}

/** 油絵風 */

mlkbool FilterDraw_effect_oilpaint(FilterDrawInfo *info)
{
	if(info->bits == 8)
		_proc_oilpaint_8bit(info);
	else
		_proc_oilpaint_16bit(info);

	return TRUE;
}


//=============================
// エンボス
//=============================


/** エンボス */

mlkbool FilterDraw_effect_emboss(FilterDrawInfo *info)
{
	Filter3x3Info dat;
	int i;

	for(i = 0; i < 9; i++)
		dat.mul[i] = 0;

	//強さ

	i = info->val_bar[0];

	if(info->val_ckbtt[0]) i = -i;	//反転

	//方向

	switch(info->val_combo[0])
	{
		case 0:
			dat.mul[0] = dat.mul[1] = -i, dat.mul[7] = dat.mul[8] = i;
			break;
		case 1:
			dat.mul[3] = -i, dat.mul[5] = i;
			break;
		default:
			dat.mul[1] = -i, dat.mul[7] = i;
			break;
	}

	//

	dat.fgray = info->val_ckbtt[1];

	if(dat.fgray)
		dat.add = (info->bits == 8)? 128: 0x4000;
	else
	{
		dat.mul[4] = 1;
		dat.add = 0;
	}

	dat.divmul = 1;

	return FilterSub_proc_3x3(info, &dat);
}


//=============================
// ノイズ
//=============================


/* 8bit */

static mlkbool _pixfunc_noise8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int i,rnd,n,val;

	if(!col->a) return FALSE;

	val = info->ntmp[1];

	for(i = 0; i < 3; i++)
	{
		if(info->ntmp[0] || i == 0)
			rnd = mRandSFMT_getIntRange(info->rand, -val, val);

		n = col->ar[i] + rnd;

		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		col->ar[i] = n;
	}

	return TRUE;
}

/* 16bit */

static mlkbool _pixfunc_noise16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	int i,rnd,n,val;

	if(!col->a) return FALSE;

	val = info->ntmp[1];

	for(i = 0; i < 3; i++)
	{
		if(info->ntmp[0] || i == 0)
			rnd = mRandSFMT_getIntRange(info->rand, -val, val);

		n = col->ar[i] + rnd;

		if(n < 0) n = 0;
		else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

		col->ar[i] = n;
	}

	return TRUE;
}

/** ノイズ */

mlkbool FilterDraw_effect_noise(FilterDrawInfo *info)
{
	info->ntmp[0] = !(info->val_ckbtt[0]);	//RGBか
	info->ntmp[1] = Density100_to_bitval(info->val_bar[0], info->bits);

	return FilterSub_proc_pixel(info, _pixfunc_noise8, _pixfunc_noise16);
}


//=============================
// 拡散
//=============================


/* 8bit/16bit 共通 */

static mlkbool _pixfunc_diffusion(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	int val = info->val_bar[0];

	x += mRandSFMT_getIntRange(info->rand, -val, val);
	y += mRandSFMT_getIntRange(info->rand, -val, val);

	FilterSub_getPixelSrc_clip(info, x, y, col);

	return TRUE;
}

/** 拡散 */

mlkbool FilterDraw_effect_diffusion(FilterDrawInfo *info)
{
	return FilterSub_proc_pixel(info, _pixfunc_diffusion, (FilterSubFunc_pixel16)_pixfunc_diffusion);
}


//=============================
// ひっかき
//=============================


/** ひっかき */

mlkbool FilterDraw_effect_scratch(FilterDrawInfo *info)
{
	mRandSFMT *rand;
	int ix,iy,yrange,xlen,ylen,x,y;
	double d;
	uint64_t col;
	mRect rc;

	rand = info->rand;
	yrange = info->val_bar[0] << 1;

	d = -info->val_bar[1] / 180.0 * MLK_MATH_PI;
	xlen = (int)(info->val_bar[0] * cos(d));
	ylen = (int)(info->val_bar[0] * sin(d));

	//

	FilterSub_copySrcImage_forPreview(info);

	rc = info->rc;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			x = mRandSFMT_getIntRange(rand, rc.x1, rc.x2);
			y = iy + mRandSFMT_getIntRange(rand, -yrange, yrange);

			FilterSub_getPixelSrc_clip(info, x, y, &col);

			x -= xlen >> 1;
			y -= ylen >> 1;

			TileImage_drawLine_forFilter(info->imgdst,
				x, y, x + xlen, y + ylen, &col, &rc);
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}


//=============================
// メディアン
//=============================


/* 8bit */

static mlkbool _proc_median8(FilterDrawInfo *info)
{
	int ix,iy,jx,jy,no,radius,num,pos;
	RGBA8 col;
	uint32_t *buf;
	uint8_t col_v[7*7];
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	radius = info->val_bar[0];
	num = radius * 2 + 1;
	num *= num;

	//バッファ

	buf = (uint32_t *)mMalloc(num * 4);
	if(!buf) return FALSE;

	//取得する値の位置

	ix = info->val_combo[0];

	if(ix == 0)
		pos = 0;
	else if(ix == 1)
		pos = num >> 1;
	else
		pos = num - 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//周囲の色と輝度を取得

			for(jy = -radius, no = 0; jy <= radius; jy++)
			{
				for(jx = -radius; jx <= radius; jx++, no++)
				{
					FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &col);

					buf[no] = col.v32;
					col_v[no] = RGB_TO_LUM(col.r, col.g, col.b);
				}
			}

			//輝度の小さい順に並び替え

			for(jx = 0; jx < num - 1; jx++)
			{
				no = jx;

				for(jy = jx + 1; jy < num; jy++)
				{
					if(col_v[jy] < col_v[no]) no = jy;
				}

				if(no != jx)
				{
					jy = col_v[jx], col_v[jx] = col_v[no], col_v[no] = jy;

					col.v32 = buf[jx], buf[jx] = buf[no], buf[no] = col.v32;
				}
			}

			//セット

			(setpix)(info->imgdst, ix, iy, buf + pos);
		}

		FilterSub_prog_substep_inc(info);
	}

	mFree(buf);

	return TRUE;
}

/* 16bit */

static mlkbool _proc_median16(FilterDrawInfo *info)
{
	int ix,iy,jx,jy,no,radius,num,pos;
	RGBA16 col;
	uint64_t *buf;
	uint16_t col_v[7*7];
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	radius = info->val_bar[0];
	num = radius * 2 + 1;
	num *= num;

	//バッファ

	buf = (uint64_t *)mMalloc(num * 8);
	if(!buf) return FALSE;

	//取得する値の位置

	ix = info->val_combo[0];

	if(ix == 0)
		pos = 0;
	else if(ix == 1)
		pos = num >> 1;
	else
		pos = num - 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//周囲の色と輝度を取得

			for(jy = -radius, no = 0; jy <= radius; jy++)
			{
				for(jx = -radius; jx <= radius; jx++, no++)
				{
					FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &col);

					buf[no] = col.v64;
					col_v[no] = RGB_TO_LUM(col.r, col.g, col.b);
				}
			}

			//輝度の小さい順に並び替え

			for(jx = 0; jx < num - 1; jx++)
			{
				no = jx;

				for(jy = jx + 1; jy < num; jy++)
				{
					if(col_v[jy] < col_v[no]) no = jy;
				}

				if(no != jx)
				{
					jy = col_v[jx], col_v[jx] = col_v[no], col_v[no] = jy;

					col.v64 = buf[jx], buf[jx] = buf[no], buf[no] = col.v64;
				}
			}

			//セット

			(setpix)(info->imgdst, ix, iy, buf + pos);
		}

		FilterSub_prog_substep_inc(info);
	}

	mFree(buf);

	return TRUE;
}

/** メディアン */

mlkbool FilterDraw_effect_median(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_median8(info);
	else
		return _proc_median16(info);
}


//=============================
// ぶれ
//=============================


/** ぶれ */

mlkbool FilterDraw_effect_blurring(FilterDrawInfo *info)
{
	int ix,iy,i,bits,fclip;
	double d[4];
	mPoint pt[4];
	uint64_t col;
	TileImage *imgsrc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	bits = info->bits;
	fclip = info->clipping;
	
	i = info->val_bar[0];

	pt[0].x = pt[0].y = pt[1].x = pt[2].y = -i;
	pt[1].y = pt[2].x = pt[3].x = pt[3].y = i;

	//

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			d[0] = d[1] = d[2] = d[3] = 0;

			for(i = 0; i < 4; i++)
				FilterSub_advcol_add_point(imgsrc, ix + pt[i].x, iy + pt[i].y, d, bits, fclip);

			FilterSub_advcol_getColor(d, 1.0 / 4, &col, bits);

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}

