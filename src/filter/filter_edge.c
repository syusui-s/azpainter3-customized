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
 * フィルタ処理: 輪郭
 **************************************/

#include <math.h>

#include "mlk.h"

#include "def_filterdraw.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"

#include "pv_filter_sub.h"


/** シャープ */

mlkbool FilterDraw_sharp(FilterDrawInfo *info)
{
	Filter3x3Info dat;
	int i;
	double d;

	mMemset0(&dat, sizeof(Filter3x3Info));

	d = info->val_bar[0] * -0.01;

	for(i = 0; i < 9; i++)
		dat.mul[i] = d;

	dat.mul[4] = info->val_bar[0] * 0.08 + 1;
	dat.divmul = 1;

	return FilterSub_proc_3x3(info, &dat);
}

/** 輪郭抽出 (Laplacian) */

mlkbool FilterDraw_edge_laplacian(FilterDrawInfo *info)
{
	Filter3x3Info dat;
	int i;

	for(i = 0; i < 9; i++)
		dat.mul[i] = -1;

	dat.mul[4] = 8;
	dat.divmul = 1;
	dat.add = (info->bits == 8)? 255: COLVAL_16BIT;
	dat.fgray = FALSE;

	return FilterSub_proc_3x3(info, &dat);
}


//=========================
// アンシャープマスク
//=========================


/* 8bit */

static void _unsharpmask_setpixel8(int x,int y,void *gcol,FilterDrawInfo *info)
{
	RGBA8 col,*pg;
	int i,n,amount;

	TileImage_getPixel(info->imgsrc, x, y, &col);

	if(!col.a) return;

	pg = (RGBA8 *)gcol;
	amount = info->ntmp[0];

	for(i = 0; i < 3; i++)
	{
		n = col.ar[i] + ((col.ar[i] - pg->ar[i]) * amount >> 8);

		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		col.ar[i] = n;
	}

	(g_tileimage_dinfo.func_setpixel)(info->imgdst, x, y, &col);
}

/* 16bit */

static void _unsharpmask_setpixel16(int x,int y,void *gcol,FilterDrawInfo *info)
{
	RGBA16 col,*pg;
	int i,n,amount;

	TileImage_getPixel(info->imgsrc, x, y, &col);

	if(!col.a) return;

	pg = (RGBA16 *)gcol;
	amount = info->ntmp[0];

	for(i = 0; i < 3; i++)
	{
		n = col.ar[i] + ((col.ar[i] - pg->ar[i]) * amount >> 8);

		if(n < 0) n = 0;
		else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

		col.ar[i] = n;
	}

	(g_tileimage_dinfo.func_setpixel)(info->imgdst, x, y, &col);
}

/** アンシャープマスク */

mlkbool FilterDraw_unsharpmask(FilterDrawInfo *info)
{
	info->ntmp[0] = (info->val_bar[1] << 8) / 100;

	if(info->bits == 8)
	{
		return FilterSub_proc_gaussblur8(info, info->imgsrc, info->val_bar[0],
			_unsharpmask_setpixel8, NULL);
	}
	else
	{
		return FilterSub_proc_gaussblur16(info, info->imgsrc, info->val_bar[0],
			_unsharpmask_setpixel16, NULL);
	}
}


//==========================
// 輪郭抽出 (Sobel)
//==========================


static const int8_t g_sobel_hmask[9] = {-1,-2,-1, 0,0,0, 1,2,1},
	g_sobel_vmask[9] = {-1,0,1, -2,0,2, -1,0,1};


/* 8bit */

static void _proc_sobel_8bit(FilterDrawInfo *info)
{
	TileImage *imgsrc;
	int ix,iy,ixx,iyy,n,i;
	double dx[3],dy[3],d;
	RGBA8 col,col2;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &col);

			if(!col.a) continue;

			//3x3

			dx[0] = dx[1] = dx[2] = 0;
			dy[0] = dy[1] = dy[2] = 0;

			for(iyy = -1, n = 0; iyy <= 1; iyy++)
			{
				for(ixx = -1; ixx <= 1; ixx++, n++)
				{
					TileImage_getPixel_clip(imgsrc, ix + ixx, iy + iyy, &col2);

					for(i = 0; i < 3; i++)
					{
						d = (double)col2.ar[i] / 255;
					
						dx[i] += d * g_sobel_hmask[n];
						dy[i] += d * g_sobel_vmask[n];
					}
				}
			}

			//RGB

			for(i = 0; i < 3; i++)
			{
				n = (int)(sqrt(dx[i] * dx[i] + dy[i] * dy[i]) * 255 + 0.5);

				if(n > 255) n = 255;

				col.ar[i] = n;
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}
	}
}

/* 16bit */

static void _proc_sobel_16bit(FilterDrawInfo *info)
{
	TileImage *imgsrc;
	int ix,iy,ixx,iyy,n,i;
	double dx[3],dy[3],d;
	RGBA16 col,col2;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &col);

			if(!col.a) continue;

			//3x3

			dx[0] = dx[1] = dx[2] = 0;
			dy[0] = dy[1] = dy[2] = 0;

			for(iyy = -1, n = 0; iyy <= 1; iyy++)
			{
				for(ixx = -1; ixx <= 1; ixx++, n++)
				{
					TileImage_getPixel_clip(imgsrc, ix + ixx, iy + iyy, &col2);

					for(i = 0; i < 3; i++)
					{
						d = (double)col2.ar[i] / COLVAL_16BIT;
					
						dx[i] += d * g_sobel_hmask[n];
						dy[i] += d * g_sobel_vmask[n];
					}
				}
			}

			//RGB

			for(i = 0; i < 3; i++)
			{
				n = (int)(sqrt(dx[i] * dx[i] + dy[i] * dy[i]) * COLVAL_16BIT + 0.5);

				if(n > COLVAL_16BIT) n = COLVAL_16BIT;

				col.ar[i] = n;
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}
	}
}

/** 輪郭抽出 (Sobel) */

mlkbool FilterDraw_edge_sobel(FilterDrawInfo *info)
{
	if(info->bits == 8)
		_proc_sobel_8bit(info);
	else
		_proc_sobel_16bit(info);

	return TRUE;
}


//==========================
// ハイパス
//==========================


/* 8bit */

static void _highpass_setpixel8(int x,int y,void *gcol,FilterDrawInfo *info)
{
	RGBA8 col,*pg;
	int i,n;

	TileImage_getPixel(info->imgsrc, x, y, &col);
	
	if(!col.a) return;

	pg = (RGBA8 *)gcol;

	for(i = 0; i < 3; i++)
	{
		n = col.ar[i] - pg->ar[i] + 128;

		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		col.ar[i] = n;
	}

	(g_tileimage_dinfo.func_setpixel)(info->imgdst, x, y, &col);
}

/* 16bit */

static void _highpass_setpixel16(int x,int y,void *gcol,FilterDrawInfo *info)
{
	RGBA16 col,*pg;
	int i,n;

	TileImage_getPixel(info->imgsrc, x, y, &col);
	
	if(!col.a) return;

	pg = (RGBA16 *)gcol;

	for(i = 0; i < 3; i++)
	{
		n = col.ar[i] - pg->ar[i] + 0x4000;

		if(n < 0) n = 0;
		else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

		col.ar[i] = n;
	}

	(g_tileimage_dinfo.func_setpixel)(info->imgdst, x, y, &col);
}

/** ハイパス */

mlkbool FilterDraw_highpass(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return FilterSub_proc_gaussblur8(info, info->imgsrc, info->val_bar[0], _highpass_setpixel8, NULL);
	else
		return FilterSub_proc_gaussblur16(info, info->imgsrc, info->val_bar[0], _highpass_setpixel16, NULL);
}

