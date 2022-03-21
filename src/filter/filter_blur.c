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
 * フィルタ処理: ぼかし
 **************************************/

#include <math.h>

#include "mlk.h"

#include "tileimage.h"

#include "def_filterdraw.h"
#include "pv_filter_sub.h"


//========================
// ぼかし
//========================


/* 8bit */

static mlkbool _proc_blur8(FilterDrawInfo *info)
{
	uint32_t *buf1,*buf2,*ps,*psY,*pd;
	int ix,iy,jx,jy,i,range,procw,xnum,ynum,pos;
	uint8_t col[4];
	double d[4],dweight;
	mRect rc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	range = info->val_bar[0];
	procw = range * 2 + 64;
	dweight = 1.0 / (range * 2 + 1);

	//バッファ

	buf1 = (uint32_t *)mMalloc(procw * procw * 4);
	buf2 = (uint32_t *)mMalloc(procw * 64 * 4);

	if(!buf1 || !buf2)
	{
		mFree(buf1);
		return FALSE;
	}

	//rc = ソースの取得範囲

	rc.y1 = info->rc.y1 - range;
	rc.y2 = rc.y1 + procw - 1;

	//64x64 単位を 1 とする

	FilterSub_prog_substep_begin_onestep(info, 50, ((info->box.w + 63) / 64) * ((info->box.h + 63) / 64));

	//---- 64x64 単位で、バッファに取得して処理

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += 64)
	{
		rc.x1 = info->rc.x1 - range;
		rc.x2 = rc.x1 + procw - 1;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += 64, rc.x1 += 64, rc.x2 += 64)
		{
			//buf1 に 64x64 処理分のソースを取得 (すべて透明なら処理なし)
		
			if(FilterSub_getPixelBuf8(info->imgsrc, &rc, (uint8_t *)buf1, info->clipping))
			{
				FilterSub_prog_substep_inc(info);
				continue;
			}

			//水平方向の処理
			// buf2 = Y[procw] x X[64]

			ps = buf1 + range;
			pd = buf2;

			for(jy = 0; jy < procw; jy++, pd++)
			{
				for(jx = 0, pos = 0; jx < 64; jx++, ps++, pos += procw)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = -range; i <= range; i++)
						FilterSub_advcol_add8(d, (uint8_t *)(ps + i));

					FilterSub_advcol_getColor8(d, dweight, (uint8_t *)(pd + pos));
				}

				ps += procw - 64;
			}

			//垂直方向の処理
			// :端は処理範囲まで

			xnum = ynum = 64;
			if(ix + 63 > info->rc.x2) xnum = info->rc.x2 + 1 - ix;
			if(iy + 63 > info->rc.y2) ynum = info->rc.y2 + 1 - iy;

			psY = buf2 + range;

			for(jx = 0; jx < xnum; jx++)
			{
				for(jy = 0, ps = psY; jy < ynum; jy++, ps++)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = -range; i <= range; i++)
						FilterSub_advcol_add8(d, (uint8_t *)(ps + i));

					FilterSub_advcol_getColor8(d, dweight, col);

					(setpix)(info->imgdst, ix + jx, iy + jy, col);
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

	return TRUE;
}

/* 16bit */

static mlkbool _proc_blur16(FilterDrawInfo *info)
{
	uint64_t *buf1,*buf2,*ps,*psY,*pd;
	int ix,iy,jx,jy,i,range,procw,xnum,ynum,pos;
	uint16_t col[4];
	double d[4],dweight;
	mRect rc;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	range = info->val_bar[0];
	procw = range * 2 + 64;
	dweight = 1.0 / (range * 2 + 1);

	//バッファ

	buf1 = (uint64_t *)mMalloc(procw * procw * 8);
	buf2 = (uint64_t *)mMalloc(procw * 64 * 8);

	if(!buf1 || !buf2)
	{
		mFree(buf1);
		return FALSE;
	}

	//rc = ソースの取得範囲

	rc.y1 = info->rc.y1 - range;
	rc.y2 = rc.y1 + procw - 1;

	//64x64 単位を 1 とする

	FilterSub_prog_substep_begin_onestep(info, 50, ((info->box.w + 63) / 64) * ((info->box.h + 63) / 64));

	//---- 64x64 単位で、バッファに取得して処理

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += 64)
	{
		rc.x1 = info->rc.x1 - range;
		rc.x2 = rc.x1 + procw - 1;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += 64, rc.x1 += 64, rc.x2 += 64)
		{
			//buf1 に 64x64 処理分のソースを取得 (すべて透明なら処理なし)
		
			if(FilterSub_getPixelBuf16(info->imgsrc, &rc, (uint16_t *)buf1, info->clipping))
			{
				FilterSub_prog_substep_inc(info);
				continue;
			}

			//水平方向の処理
			// buf2 = Y[procw] x X[64]

			ps = buf1 + range;
			pd = buf2;

			for(jy = 0; jy < procw; jy++, pd++)
			{
				for(jx = 0, pos = 0; jx < 64; jx++, ps++, pos += procw)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = -range; i <= range; i++)
						FilterSub_advcol_add16(d, (uint16_t *)(ps + i));

					FilterSub_advcol_getColor16(d, dweight, (uint16_t *)(pd + pos));
				}

				ps += procw - 64;
			}

			//垂直方向の処理
			// :端は処理範囲まで

			xnum = ynum = 64;
			if(ix + 63 > info->rc.x2) xnum = info->rc.x2 + 1 - ix;
			if(iy + 63 > info->rc.y2) ynum = info->rc.y2 + 1 - iy;

			psY = buf2 + range;

			for(jx = 0; jx < xnum; jx++)
			{
				for(jy = 0, ps = psY; jy < ynum; jy++, ps++)
				{
					d[0] = d[1] = d[2] = d[3] = 0;

					for(i = -range; i <= range; i++)
						FilterSub_advcol_add16(d, (uint16_t *)(ps + i));

					FilterSub_advcol_getColor16(d, dweight, col);

					(setpix)(info->imgdst, ix + jx, iy + jy, col);
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

	return TRUE;
}

mlkbool FilterDraw_blur(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_blur8(info);
	else
		return _proc_blur16(info);
}


//===========================
// ガウスぼかし
//===========================


/** ガウスぼかし */

mlkbool FilterDraw_gaussblur(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return FilterSub_proc_gaussblur8(info, info->imgsrc, info->val_bar[0], NULL, NULL);
	else
		return FilterSub_proc_gaussblur16(info, info->imgsrc, info->val_bar[0], NULL, NULL);
}


//===========================
// モーションブラー
// (0.5px 間隔でガウスぼかし)
//===========================


/* ガウステーブル作成 (0.5px 間隔) */

static double *_create_motionblur_table(int radius,int range,int tblnum,double *dst_weight)
{
	double *buf,*pd,dmul,d,dpos,weight;
	int i;

	buf = pd = (double *)mMalloc(sizeof(double) * tblnum);
	if(!buf) return NULL;

	dmul = 1.0 / (2 * radius * radius);
	weight = 0;
	dpos = -range;

	for(i = 0; i < tblnum; i++, dpos += 0.5)
	{
		d = exp(-(dpos * dpos) * dmul);
		
		*(pd++) = d;
		weight += d;
	}

	*dst_weight = 1.0 / weight;

	return buf;
}

/** モーションブラー */

mlkbool FilterDraw_motionblur(FilterDrawInfo *info)
{
	int ix,iy,i,radius,range,pixnum,stx,sty,incx,incy,fx,fy;
	uint64_t col;
	double *tblbuf,dweight,d,dsin,dcos,dc[4];
	TileImageSetPixelFunc setpix;
	TileImage *imgsrc;

	radius = info->val_bar[0];
	range = radius * 3;
	pixnum = (range * 2 + 1) * 2;  //0.5px 間隔なので2倍

	FilterSub_getPixelFunc(&setpix);

	//ガウステーブル

	tblbuf = _create_motionblur_table(radius, range, pixnum, &dweight);
	if(!tblbuf) return FALSE;

	//角度

	d = -(info->val_bar[1]) / 180.0 * MLK_MATH_PI;
	dsin = sin(d);
	dcos = cos(d);

	stx = (int)(dcos * -range * (1<<16));
	sty = (int)(dsin * -range * (1<<16));
	incx = (int)(dcos * 0.5 * (1<<16));
	incy = (int)(dsin * 0.5 * (1<<16));

	//

	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			fx = stx, fy = sty;

			dc[0] = dc[1] = dc[2] = dc[3] = 0;

			if(info->bits == 8)
			{
				//8bit
				
				for(i = 0; i < pixnum; i++)
				{
					FilterSub_advcol_add_point_weight8(imgsrc,
						ix + (fx >> 16), iy + (fy >> 16), dc, tblbuf[i], info->clipping);

					fx += incx;
					fy += incy;
				}
				
				FilterSub_advcol_getColor8(dc, dweight, (uint8_t *)&col);
			}
			else
			{
				//16bit
				
				for(i = 0; i < pixnum; i++)
				{
					FilterSub_advcol_add_point_weight16(imgsrc,
						ix + (fx >> 16), iy + (fy >> 16), dc, tblbuf[i], info->clipping);

					fx += incx;
					fy += incy;
				}
				
				FilterSub_advcol_getColor16(dc, dweight, (uint16_t *)&col);
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	mFree(tblbuf);

	return TRUE;
}


//===========================
// 放射状ぼかし
//===========================


/** 放射状ぼかし */

mlkbool FilterDraw_radialblur(FilterDrawInfo *info)
{
	int ix,iy,len,i,bits;
	uint64_t col;
	double factor,rr,dx,dy,rd,dsin,dcos,xx,yy,d[4];
	TileImageSetPixelFunc setpix;
	TileImage *imgsrc;

	bits = info->bits;

	FilterSub_getPixelFunc(&setpix);

	factor = info->val_bar[0] * 0.01;
	imgsrc = info->imgsrc;

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dy = iy - info->imgy;
		
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			dx = ix - info->imgx;

			//中央位置からの距離に強さを適用して、処理長さ計算

			rr = sqrt(dx * dx + dy * dy);
			
			len = (int)(rr * factor);

			if(len < 1)
			{
				//長さ1 = ソースの点をコピー
				TileImage_getPixel(imgsrc, ix, iy, &col);
				(setpix)(info->imgdst, ix, iy, &col);
				continue;
			}

			//角度

			rd = atan2(dy, dx);
			dsin = sin(rd);
			dcos = cos(rd);

			//角度と長さの範囲から色取得

			d[0] = d[1] = d[2] = d[3] = 0;

			xx = (rr - len) * dcos + info->imgx;
			yy = (rr - len) * dsin + info->imgy;

			for(i = 0; i <= len; i++, xx += dcos, yy += dsin)
			{
				FilterSub_getLinerColor(imgsrc, xx, yy, &col, bits, info->clipping);

				if(bits == 8)
					FilterSub_advcol_add8(d, (uint8_t *)&col);
				else
					FilterSub_advcol_add16(d, (uint16_t *)&col);
			}

			//セット

			FilterSub_advcol_getColor(d, 1.0 / (len + 1), &col, bits);

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}


//===========================
// レンズぼかし
//===========================


/* テーブル作成 */

static double *_lensblur_create_table(int num,double val)
{
	double *buf;
	int i;

	buf = (double *)mMalloc(sizeof(double) * num);
	if(!buf) return NULL;

	for(i = 0; i < num; i++)
		buf[i] = pow(val, i);

	return buf;
}

/* 8bit */

static mlkbool _proc_lensblur8(FilterDrawInfo *info)
{
	int ix,iy,radius,jx,jy;
	double *tblbuf,br_base,br_res,dweight,d[4],da;
	uint8_t *shapebuf,*ps,f;
	RGBA8 col;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	radius = info->val_bar[0];
	br_base = 1.010 + info->val_bar[1] * 0.001;
	br_res = 1.0 / log(br_base);

	//テーブル

	tblbuf = _lensblur_create_table(256, br_base);
	if(!tblbuf) return FALSE;

	//円形データ (1bit)

	shapebuf = FilterSub_createShapeBuf_circle(radius, &ix);
	if(!shapebuf)
	{
		mFree(tblbuf);
		return FALSE;
	}

	dweight = 1.0 / ix;
	
	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//円形範囲の平均色

			d[0] = d[1] = d[2] = d[3] = 0;

			ps = shapebuf;
			f = 0x80;

			for(jy = -radius; jy <= radius; jy++)
			{
				for(jx = -radius; jx <= radius; jx++)
				{
					if(*ps & f)
					{
						FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &col);

						da = (double)col.a / 255;

						d[0] += tblbuf[col.r] * da;
						d[1] += tblbuf[col.g] * da;
						d[2] += tblbuf[col.b] * da;
						d[3] += da;
					}

					f >>= 1;
					if(!f) f = 0x80, ps++;
				}
			}

			//セット

			col.a = (int)(d[3] * dweight * 255 + 0.5);

			if(col.a == 0)
				col.v32 = 0;
			else
			{
				da = 1.0 / d[3];

				for(jx = 0; jx < 3; jx++)
				{
					jy = (int)(log(d[jx] * da) * br_res);

					if(jy < 0) jy = 0;
					else if(jy > 255) jy = 255;

					col.ar[jx] = jy;
				}
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	mFree(tblbuf);
	mFree(shapebuf);

	return TRUE;
}

/* 16bit */

static mlkbool _proc_lensblur16(FilterDrawInfo *info)
{
	int ix,iy,radius,jx,jy;
	double *tblbuf,br_base,br_res,dweight,d[4],da;
	uint8_t *shapebuf,*ps,f;
	RGBA16 col;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	radius = info->val_bar[0];
	br_base = 1.010 + info->val_bar[1] * 0.001;
	br_res = (1<<7) / log(br_base);

	//テーブル

	tblbuf = _lensblur_create_table(257, br_base);
	if(!tblbuf) return FALSE;

	//円形データ (1bit)

	shapebuf = FilterSub_createShapeBuf_circle(radius, &ix);
	if(!shapebuf)
	{
		mFree(tblbuf);
		return FALSE;
	}

	dweight = 1.0 / ix;
	
	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//円形範囲の平均色

			d[0] = d[1] = d[2] = d[3] = 0;

			ps = shapebuf;
			f = 0x80;

			for(jy = -radius; jy <= radius; jy++)
			{
				for(jx = -radius; jx <= radius; jx++)
				{
					if(*ps & f)
					{
						FilterSub_getPixelSrc_clip(info, ix + jx, iy + jy, &col);

						da = (double)col.a / 0x8000;

						d[0] += tblbuf[col.r >> 7] * da;
						d[1] += tblbuf[col.g >> 7] * da;
						d[2] += tblbuf[col.b >> 7] * da;
						d[3] += da;
					}

					f >>= 1;
					if(!f) f = 0x80, ps++;
				}
			}

			//セット

			col.a = (int)(d[3] * dweight * 0x8000 + 0.5);

			if(col.a == 0)
				col.v64 = 0;
			else
			{
				da = 1.0 / d[3];

				for(jx = 0; jx < 3; jx++)
				{
					jy = (int)(log(d[jx] * da) * br_res);

					if(jy < 0) jy = 0;
					else if(jy > 0x8000) jy = 0x8000;

					col.ar[jx] = jy;
				}
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	mFree(tblbuf);
	mFree(shapebuf);

	return TRUE;
}

mlkbool FilterDraw_lensblur(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_lensblur8(info);
	else
		return _proc_lensblur16(info);
}

