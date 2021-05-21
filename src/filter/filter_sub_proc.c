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
 * フィルタ処理: 共通描画処理関数
 **************************************/

#include <math.h>

#include "mlk.h"

#include "tileimage.h"

#include "def_filterdraw.h"
#include "pv_filter_sub.h"



/** 点処理関数を指定して、描画
 *
 * 関数で FALSE が返った場合、点を描画しない。 */

mlkbool FilterSub_proc_pixel(FilterDrawInfo *info,
	FilterSubFunc_pixel8 func8,FilterSubFunc_pixel16 func16)
{
	TileImage *imgsrc,*imgdst;
	TileImageSetPixelFunc setpix; 
	int ix,iy,bits;
	uint64_t col;
	mRect rc;

	imgsrc = info->imgsrc;
	imgdst = info->imgdst;
	rc = info->rc;
	bits = info->bits;

	FilterSub_getPixelFunc(&setpix);

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &col);

			if(bits == 8)
			{
				if((func8)(info, ix, iy, (RGBA8 *)&col))
					(setpix)(imgdst, ix, iy, &col);
			}
			else
			{
				if((func16)(info, ix, iy, (RGBA16 *)&col))
					(setpix)(imgdst, ix, iy, &col);
			}
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}


//============================
// 3x3 フィルタ
//============================
/*
  - 透明色は処理しない。RGB のみ処理。
  - クリッピング有効。
*/


/* 8bit */

static void _proc_3x3_8bit(FilterDrawInfo *info,Filter3x3Info *dat)
{
	TileImage *imgsrc,*imgdst;
	int ix,iy,ixx,iyy,n;
	double d[3],divmul,add,dd;
	RGBA8 col,col2;
	mRect rc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix); 

	imgsrc = info->imgsrc;
	imgdst = info->imgdst;
	rc = info->rc;

	divmul = dat->divmul;
	add = dat->add;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &col);

			if(!col.a) continue;

			//3x3

			d[0] = d[1] = d[2] = 0;

			for(iyy = -1, n = 0; iyy <= 1; iyy++)
			{
				for(ixx = -1; ixx <= 1; ixx++, n++)
				{
					if(info->clipping)
						TileImage_getPixel_clip(imgsrc, ix + ixx, iy + iyy, &col2);
					else
						TileImage_getPixel(imgsrc, ix + ixx, iy + iyy, &col2);

					dd = dat->mul[n];

					d[0] += col2.r * dd;
					d[1] += col2.g * dd;
					d[2] += col2.b * dd;
				}
			}

			//RGB

			for(ixx = 0; ixx < 3; ixx++)
			{
				n = (int)(d[ixx] * divmul + add);

				if(n < 0) n = 0;
				else if(n > 255) n = 255;

				col.ar[ixx] = n;
			}

			//グレイスケール化

			if(dat->fgray)
				col.r = col.g = col.b = RGB_TO_LUM(col.r, col.g, col.b);

			//セット

			(setpix)(imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}
}

/* 16bit */

static void _proc_3x3_16bit(FilterDrawInfo *info,Filter3x3Info *dat)
{
	TileImage *imgsrc,*imgdst;
	int ix,iy,ixx,iyy,n;
	double d[3],divmul,add,dd;
	RGBA16 col,col2;
	mRect rc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix); 

	imgsrc = info->imgsrc;
	imgdst = info->imgdst;
	rc = info->rc;

	divmul = dat->divmul;
	add = dat->add;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(imgsrc, ix, iy, &col);

			if(!col.a) continue;

			//3x3

			d[0] = d[1] = d[2] = 0;

			for(iyy = -1, n = 0; iyy <= 1; iyy++)
			{
				for(ixx = -1; ixx <= 1; ixx++, n++)
				{
					if(info->clipping)
						TileImage_getPixel_clip(imgsrc, ix + ixx, iy + iyy, &col2);
					else
						TileImage_getPixel(imgsrc, ix + ixx, iy + iyy, &col2);

					dd = dat->mul[n];

					d[0] += col2.r * dd;
					d[1] += col2.g * dd;
					d[2] += col2.b * dd;
				}
			}

			//RGB

			for(ixx = 0; ixx < 3; ixx++)
			{
				n = (int)(d[ixx] * divmul + add);

				if(n < 0) n = 0;
				else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

				col.ar[ixx] = n;
			}

			//グレイスケール化

			if(dat->fgray)
				col.r = col.g = col.b = RGB_TO_LUM(col.r, col.g, col.b);

			//セット

			(setpix)(imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}
}

/** 3x3 フィルタ */

mlkbool FilterSub_proc_3x3(FilterDrawInfo *info,Filter3x3Info *dat)
{
	if(info->bits == 8)
		_proc_3x3_8bit(info, dat);
	else
		_proc_3x3_16bit(info, dat);

	return TRUE;
}


//=========================
// ガウスぼかし
//=========================


/** ガウスぼかしの重みテーブル作成 */

double *FilterSub_createGaussTable(int radius,int range,double *dst_weight)
{
	double *buf,*pd,dmul,d,weight;
	int i;

	buf = pd = (double *)mMalloc(sizeof(double) * (range * 2 + 1));
	if(!buf) return NULL;

	dmul = 1.0 / (2 * radius * radius);
	weight = 0;

	for(i = -range; i <= range; i++)
	{
		d = exp(-(i * i) * dmul);
		
		*(pd++) = d;
		weight += d;
	}

	*dst_weight = 1.0 / weight;

	return buf;
}

/** ガウスぼかし処理 (8bit)
 *
 * setpix: 結果の点を描画する関数。NULL で imgdst に通常描画。
 * setsrc: ぼかし処理の前にソースの色を処理する関数。NULL でなし。 */

mlkbool FilterSub_proc_gaussblur8(FilterDrawInfo *info,
	TileImage *imgsrc,int radius,
	FilterSubFunc_gaussblur_setpix setpix,FilterSubFunc_gaussblur_setsrc setsrc)
{
	uint32_t *buf1,*buf2,*ps,*psY,*pd;
	int ix,iy,jx,jy,i,range,procw,xnum,ynum,pos,blurlen,srcpixnum;
	uint8_t col[4];
	double d[4],*tblbuf,dweight;
	mRect rc;
	TileImageSetPixelFunc setpix_def;

	range = radius * 3;
	procw = range * 2 + 64;
	blurlen = range * 2 + 1;
	srcpixnum = procw * procw;

	FilterSub_getPixelFunc(&setpix_def);

	//バッファ

	buf1 = (uint32_t *)mMalloc(procw * procw * 4);
	buf2 = (uint32_t *)mMalloc(procw * 64 * 4);

	if(!buf1 || !buf2)
	{
		mFree(buf1);
		return FALSE;
	}

	//ガウステーブル

	tblbuf = FilterSub_createGaussTable(radius, range, &dweight);
	if(!tblbuf)
	{
		mFree(buf1);
		mFree(buf2);
		return FALSE;
	}

	//--------------

	FilterSub_prog_substep_begin_onestep(info, 50, ((info->box.w + 63) / 64) * ((info->box.h + 63) / 64));

	//rc = ソースの取得範囲

	rc.y1 = info->rc.y1 - range;
	rc.y2 = rc.y1 + procw - 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += 64)
	{
		rc.x1 = info->rc.x1 - range;
		rc.x2 = rc.x1 + procw - 1;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += 64, rc.x1 += 64, rc.x2 += 64)
		{
			//buf1 に 64x64 処理分のソースセット (すべて透明なら処理なし)
		
			if(FilterSub_getPixelBuf8(imgsrc, &rc, (uint8_t *)buf1, info->clipping))
			{
				FilterSub_prog_substep_inc(info);
				continue;
			}

			//ソースの色を処理

			if(setsrc)
				(setsrc)(buf1, srcpixnum, info);

			//水平方向
			// buf2: Y[procw] x X[64]

			ps = buf1 + range;
			pd = buf2;

			for(jy = 0; jy < procw; jy++, pd++)
			{
				for(jx = 0, pos = 0; jx < 64; jx++, ps++, pos += procw)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = 0; i < blurlen; i++)
						FilterSub_advcol_add_weight8(d, tblbuf[i], (uint8_t *)(ps + i - range));

					FilterSub_advcol_getColor8(d, dweight, (uint8_t *)(pd + pos));
				}

				ps += procw - 64;
			}

			//垂直方向

			xnum = ynum = 64;
			if(ix + 63 > info->rc.x2) xnum = info->rc.x2 + 1 - ix;
			if(iy + 63 > info->rc.y2) ynum = info->rc.y2 + 1 - iy;

			psY = buf2 + range;

			for(jx = 0; jx < xnum; jx++)
			{
				for(jy = 0, ps = psY; jy < ynum; jy++, ps++)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = 0; i < blurlen; i++)
						FilterSub_advcol_add_weight8(d, tblbuf[i], (uint8_t *)(ps + i - range));

					FilterSub_advcol_getColor8(d, dweight, col);

					if(setpix)
						(setpix)(ix + jx, iy + jy, col, info);
					else
						(setpix_def)(info->imgdst, ix + jx, iy + jy, col);
				}

				psY += procw;
			}

			FilterSub_prog_substep_inc(info);
		}

		rc.y1 += 64;
		rc.y2 += 64;
	}

	mFree(buf1);
	mFree(buf2);
	mFree(tblbuf);

	return TRUE;
}

/** ガウスぼかし処理 (16bit) */

mlkbool FilterSub_proc_gaussblur16(FilterDrawInfo *info,
	TileImage *imgsrc,int radius,
	FilterSubFunc_gaussblur_setpix setpix,FilterSubFunc_gaussblur_setsrc setsrc)
{
	uint64_t *buf1,*buf2,*ps,*psY,*pd;
	int ix,iy,jx,jy,i,range,procw,xnum,ynum,pos,blurlen,srcpixnum;
	uint16_t col[4];
	double d[4],*tblbuf,dweight;
	mRect rc;
	TileImageSetPixelFunc setpix_def;

	range = radius * 3;
	procw = range * 2 + 64;
	blurlen = range * 2 + 1;
	srcpixnum = procw * procw;

	FilterSub_getPixelFunc(&setpix_def);

	//バッファ

	buf1 = (uint64_t *)mMalloc(procw * procw * 8);
	buf2 = (uint64_t *)mMalloc(procw * 64 * 8);

	if(!buf1 || !buf2)
	{
		mFree(buf1);
		return FALSE;
	}

	//ガウステーブル

	tblbuf = FilterSub_createGaussTable(radius, range, &dweight);
	if(!tblbuf)
	{
		mFree(buf1);
		mFree(buf2);
		return FALSE;
	}

	//--------------

	FilterSub_prog_substep_begin_onestep(info, 50, ((info->box.w + 63) / 64) * ((info->box.h + 63) / 64));

	//rc = ソースの取得範囲

	rc.y1 = info->rc.y1 - range;
	rc.y2 = rc.y1 + procw - 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += 64)
	{
		rc.x1 = info->rc.x1 - range;
		rc.x2 = rc.x1 + procw - 1;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += 64, rc.x1 += 64, rc.x2 += 64)
		{
			//buf1 に 64x64 処理分のソースセット (すべて透明なら処理なし)
		
			if(FilterSub_getPixelBuf16(imgsrc, &rc, (uint16_t *)buf1, info->clipping))
			{
				FilterSub_prog_substep_inc(info);
				continue;
			}

			//ソースの色を処理

			if(setsrc)
				(setsrc)(buf1, srcpixnum, info);

			//水平方向
			// buf2: Y[procw] x X[64]

			ps = buf1 + range;
			pd = buf2;

			for(jy = 0; jy < procw; jy++, pd++)
			{
				for(jx = 0, pos = 0; jx < 64; jx++, ps++, pos += procw)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = 0; i < blurlen; i++)
						FilterSub_advcol_add_weight16(d, tblbuf[i], (uint16_t *)(ps + i - range));

					FilterSub_advcol_getColor16(d, dweight, (uint16_t *)(pd + pos));
				}

				ps += procw - 64;
			}

			//垂直方向

			xnum = ynum = 64;
			if(ix + 63 > info->rc.x2) xnum = info->rc.x2 + 1 - ix;
			if(iy + 63 > info->rc.y2) ynum = info->rc.y2 + 1 - iy;

			psY = buf2 + range;

			for(jx = 0; jx < xnum; jx++)
			{
				for(jy = 0, ps = psY; jy < ynum; jy++, ps++)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = 0; i < blurlen; i++)
						FilterSub_advcol_add_weight16(d, tblbuf[i], (uint16_t *)(ps + i - range));

					FilterSub_advcol_getColor16(d, dweight, col);

					if(setpix)
						(setpix)(ix + jx, iy + jy, col, info);
					else
						(setpix_def)(info->imgdst, ix + jx, iy + jy, col);
				}

				psY += procw;
			}

			FilterSub_prog_substep_inc(info);
		}

		rc.y1 += 64;
		rc.y2 += 64;
	}

	mFree(buf1);
	mFree(buf2);
	mFree(tblbuf);

	return TRUE;
}

