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
 * フィルタ処理: 漫画用、トーン
 **************************************/

#include <math.h>

#include "mlk.h"

#include "def_filterdraw.h"

#include "tileimage.h"
#include "table_data.h"

#include "perlin_noise.h"
#include "pv_filter_sub.h"


//========================
// 網トーン生成
//========================


/* 8bit */

static mlkbool _proc_amitone_create8(FilterDrawInfo *info)
{
	uint8_t *tblbuf;
	double dinc_cos,dinc_sin,dcell,dyx,dyy,dxx,dxy,dx,dy;
	int ix,iy,n,density,cx,cy,tsize,tmask;
	mlkbool fwhite;
	RGBA8 coldraw,col;
	TileImageSetPixelFunc setpix;

	dcell = FilterSub_getImageDPI() / (info->val_bar[0] * 0.1);

	tsize = (dcell < 60)? 256: 512;
	tmask = tsize - 1;

	//テーブル

	tblbuf = (uint8_t *)mMalloc(tsize * tsize);
	if(!tblbuf) return FALSE;

	ToneTable_setData(tblbuf, tsize, 8);

	//

	density = (100 - info->val_bar[1]) * 255 / 100;

	n = info->val_bar[2] * 512 / 360;

	dinc_cos = 1.0 / round(dcell / TABLEDATA_GET_COS(n));
	dinc_sin = 1.0 / round(dcell / TABLEDATA_GET_SIN(n));

	FilterSub_getPixelFunc(&setpix);

	fwhite = info->val_ckbtt[0];

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	//(0,0) の位置が基準になるように

	dyx = 0.5 + info->rc.x1 * dinc_cos - info->rc.y1 * dinc_sin;
	dyy = 0.7 + info->rc.x1 * dinc_sin + info->rc.y1 * dinc_cos;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dxx = dyx;
		dxy = dyy;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = dxx;
			dy = dxy;
			
			if(density < 128)
				dx += 0.5, dy += 0.5;

			cx = round(dx * tsize);
			cy = round(dy * tsize);

			cx &= tmask;
			cy &= tmask;

			n = tblbuf[cy * tsize + cx];

			if(density < 128)
				n = (density < 255 - n);
			else
				n = (density < n);

			if(n)
				col = coldraw;
			else
			{
				if(fwhite)
					col.v32 = 0xffffffff;
				else
					col.v32 = 0;
			}

			(setpix)(info->imgdst, ix, iy, &col);

			//

			dxx += dinc_cos;
			dxy += dinc_sin;
		}

		dyx -= dinc_sin;
		dyy += dinc_cos;

		FilterSub_prog_substep_inc(info);
	}
	
	mFree(tblbuf);

	return TRUE;
}

/* 16bit */

static mlkbool _proc_amitone_create16(FilterDrawInfo *info)
{
	uint16_t *tblbuf;
	double dinc_cos,dinc_sin,dcell,dyx,dyy,dxx,dxy,dx,dy;
	int ix,iy,n,density,cx,cy,tsize,tmask;
	mlkbool fwhite;
	RGBA16 col,coldraw;
	TileImageSetPixelFunc setpix;

	dcell = FilterSub_getImageDPI() / (info->val_bar[0] * 0.1);

	tsize = (dcell < 60)? 256: 512;
	tmask = tsize - 1;

	//テーブル

	tblbuf = (uint16_t *)mMalloc(tsize * tsize * 2);
	if(!tblbuf) return FALSE;

	ToneTable_setData((uint8_t *)tblbuf, tsize, 16);

	//

	density = ((100 - info->val_bar[1]) << 15) / 100;

	n = info->val_bar[2] * 512 / 360;

	dinc_cos = 1.0 / round(dcell / TABLEDATA_GET_COS(n));
	dinc_sin = 1.0 / round(dcell / TABLEDATA_GET_SIN(n));

	FilterSub_getPixelFunc(&setpix);

	fwhite = info->val_ckbtt[0];

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	//(0,0) の位置が基準になるように

	dyx = 0.5 + info->rc.x1 * dinc_cos - info->rc.y1 * dinc_sin;
	dyy = 0.7 + info->rc.x1 * dinc_sin + info->rc.y1 * dinc_cos;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dxx = dyx;
		dxy = dyy;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = dxx;
			dy = dxy;
			
			if(density < 0x4000)
				dx += 0.5, dy += 0.5;

			cx = round(dx * tsize);
			cy = round(dy * tsize);

			cx &= tmask;
			cy &= tmask;

			n = tblbuf[cy * tsize + cx];

			if(density < 0x4000)
				n = (density < COLVAL_16BIT - n);
			else
				n = (density < n);

			if(n)
				col = coldraw;
			else
			{
				if(fwhite)
					col.r = col.g = col.b = col.a = COLVAL_16BIT;
				else
					col.v64 = 0;
			}

			(setpix)(info->imgdst, ix, iy, &col);

			//

			dxx += dinc_cos;
			dxy += dinc_sin;
		}

		dyx -= dinc_sin;
		dyy += dinc_cos;

		FilterSub_prog_substep_inc(info);
	}
	
	mFree(tblbuf);

	return TRUE;
}

/** 網トーン生成 */

mlkbool FilterDraw_comic_amitone_create(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_amitone_create8(info);
	else
		return _proc_amitone_create16(info);
}


//========================
// 網トーン化
//========================


/* 8bit */

static mlkbool _proc_to_amitone8(FilterDrawInfo *info)
{
	uint8_t *tblbuf;
	double dinc_cos,dinc_sin,dcell,dyx,dyy,dxx,dxy,dx,dy;
	int ix,iy,n,cval,cx,cy,tsize,tmask;
	mlkbool fwhite;
	RGBA8 col,coldraw;
	TileImageSetPixelFunc setpix;

	dcell = FilterSub_getImageDPI() / (info->val_bar[0] * 0.1);

	tsize = (dcell < 60)? 256: 512;
	tmask = tsize - 1;

	//テーブル

	tblbuf = (uint8_t *)mMalloc(tsize * tsize);
	if(!tblbuf) return FALSE;

	ToneTable_setData(tblbuf, tsize, 8);

	//

	n = info->val_bar[1] * 512 / 360;

	dinc_cos = 1.0 / round(dcell / TABLEDATA_GET_COS(n));
	dinc_sin = 1.0 / round(dcell / TABLEDATA_GET_SIN(n));

	FilterSub_getPixelFunc(&setpix);

	fwhite = info->val_ckbtt[0];

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	//(0,0) の位置が基準になるように

	dyx = 0.5 + info->rc.x1 * dinc_cos - info->rc.y1 * dinc_sin;
	dyy = 0.7 + info->rc.x1 * dinc_sin + info->rc.y1 * dinc_cos;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dxx = dyx;
		dxy = dyy;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++, dxx += dinc_cos, dxy += dinc_sin)
		{
			TileImage_getPixel(info->imgsrc, ix, iy, &col);

			if(!col.a) continue;
			
			dx = dxx;
			dy = dxy;

			cval = RGB_TO_LUM(col.r, col.g, col.b);

			if(cval < 128)
				dx += 0.5, dy += 0.5;

			cx = round(dx * tsize);
			cy = round(dy * tsize);

			cx &= tmask;
			cy &= tmask;

			n = tblbuf[cy * tsize + cx];

			if(cval < 128)
				n = (cval < 255 - n);
			else
				n = (cval < n);

			if(n)
				col = coldraw;
			else
			{
				if(fwhite)
					col.v32 = 0xffffffff;
				else
					col.v32 = 0;
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		dyx -= dinc_sin;
		dyy += dinc_cos;

		FilterSub_prog_substep_inc(info);
	}
	
	mFree(tblbuf);

	return TRUE;
}

/* 16bit */

static mlkbool _proc_to_amitone16(FilterDrawInfo *info)
{
	uint16_t *tblbuf;
	double dinc_cos,dinc_sin,dcell,dyx,dyy,dxx,dxy,dx,dy;
	int ix,iy,n,cval,cx,cy,tsize,tmask;
	mlkbool fwhite;
	RGBA16 col,coldraw;
	TileImageSetPixelFunc setpix;

	dcell = FilterSub_getImageDPI() / (info->val_bar[0] * 0.1);

	tsize = (dcell < 60)? 256: 512;
	tmask = tsize - 1;

	//テーブル

	tblbuf = (uint16_t *)mMalloc(tsize * tsize * 2);
	if(!tblbuf) return FALSE;

	ToneTable_setData((uint8_t *)tblbuf, tsize, 16);

	//

	n = info->val_bar[1] * 512 / 360;

	dinc_cos = 1.0 / round(dcell / TABLEDATA_GET_COS(n));
	dinc_sin = 1.0 / round(dcell / TABLEDATA_GET_SIN(n));

	FilterSub_getPixelFunc(&setpix);

	fwhite = info->val_ckbtt[0];

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	//(0,0) の位置が基準になるように

	dyx = 0.5 + info->rc.x1 * dinc_cos - info->rc.y1 * dinc_sin;
	dyy = 0.7 + info->rc.x1 * dinc_sin + info->rc.y1 * dinc_cos;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dxx = dyx;
		dxy = dyy;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++, dxx += dinc_cos, dxy += dinc_sin)
		{
			TileImage_getPixel(info->imgsrc, ix, iy, &col);

			if(!col.a) continue;
			
			dx = dxx;
			dy = dxy;

			cval = RGB_TO_LUM(col.r, col.g, col.b);

			if(cval < 0x4000)
				dx += 0.5, dy += 0.5;

			cx = round(dx * tsize);
			cy = round(dy * tsize);

			cx &= tmask;
			cy &= tmask;

			n = tblbuf[cy * tsize + cx];

			if(cval < 0x4000)
				n = (cval < COLVAL_16BIT - n);
			else
				n = (cval < n);

			if(n)
				col = coldraw;
			else
			{
				if(fwhite)
					col.r = col.g = col.b = col.a = COLVAL_16BIT;
				else
					col.v64 = 0;
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		dyx -= dinc_sin;
		dyy += dinc_cos;

		FilterSub_prog_substep_inc(info);
	}
	
	mFree(tblbuf);

	return TRUE;
}

/** 網トーン化 */

mlkbool FilterDraw_comic_to_amitone(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_to_amitone8(info);
	else
		return _proc_to_amitone16(info);
}


//========================
// 砂目トーン化
//========================


/* 8bit */

static mlkbool _proc_sandtone8(FilterDrawInfo *info)
{
	PerlinNoise *noise;
	int ix,iy,n,density;
	mlkbool fwhite,ffix;
	RGBA8 col,coldraw;
	TileImageSetPixelFunc setpix;

	//PerlinNoise 初期化

	noise = PerlinNoise_new(info->val_bar[0] / 400.0, 0.15, info->rand);
	if(!noise) return FALSE;

	//

	density = (100 - info->val_bar[1]) * 255 / 100;
	ffix = info->val_ckbtt[0];
	fwhite = info->val_ckbtt[1];

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	FilterSub_getPixelFunc(&setpix);

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			if(!ffix)
			{
				TileImage_getPixel(info->imgsrc, ix, iy, &col);

				if(!col.a) continue;

				density = RGB_TO_LUM(col.r, col.g, col.b);
			}

			if(density == 0 || density == 255)
				n = (density != 0);
			else
			{
				n = (int)(PerlinNoise_getNoise(noise, ix, iy) * 1500 + 128);
				n = (n < density);
			}

			//

			if(!n)
				col = coldraw;
			else
			{
				if(fwhite)
					col.v32 = 0xffffffff;
				else
					col.v32 = 0;
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	PerlinNoise_free(noise);

	return TRUE;
}

/* 16bit */

static mlkbool _proc_sandtone16(FilterDrawInfo *info)
{
	PerlinNoise *noise;
	int ix,iy,n,density;
	mlkbool fwhite,ffix;
	RGBA16 col,coldraw;
	TileImageSetPixelFunc setpix;

	//PerlinNoise 初期化

	noise = PerlinNoise_new(info->val_bar[0] / 400.0, 0.15, info->rand);
	if(!noise) return FALSE;

	//

	density = ((100 - info->val_bar[1]) << 15) / 100;
	ffix = info->val_ckbtt[0];
	fwhite = info->val_ckbtt[1];

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	FilterSub_getPixelFunc(&setpix);

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			if(!ffix)
			{
				TileImage_getPixel(info->imgsrc, ix, iy, &col);

				if(!col.a) continue;

				density = RGB_TO_LUM(col.r, col.g, col.b);
			}

			if(density == 0 || density == COLVAL_16BIT)
				n = (density != 0);
			else
			{
				n = (int)(PerlinNoise_getNoise(noise, ix, iy) * 192752 + 0x4000);
				n = (n < density);
			}

			//

			if(!n)
				col = coldraw;
			else
			{
				if(fwhite)
					col.r = col.g = col.b = col.a = COLVAL_16BIT;
				else
					col.v64 = 0;
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	PerlinNoise_free(noise);

	return TRUE;
}

/** 砂目トーン化 */

mlkbool FilterDraw_comic_sand_tone(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_sandtone8(info);
	else
		return _proc_sandtone16(info);
}

