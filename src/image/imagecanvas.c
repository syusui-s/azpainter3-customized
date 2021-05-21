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

/******************************
 * キャンバス用イメージ
 ******************************/

#include <string.h>

#include "mlk.h"
#include "mlk_pixbuf.h"

#include "imagecanvas.h"
#include "colorvalue.h"
#include "canvasinfo.h"

#include "pv_imagecanvas.h"


/*
 * - 8bit で 4byte、16bit で 8byte。
 * - R-G-B-X 順に並ぶ。
 * - 各Y行のバッファは 16byte 境界。
 */


/** 解放 */

void ImageCanvas_free(ImageCanvas *p)
{
	uint8_t **ppbuf;
	int i;

	if(p)
	{
		if(p->ppbuf)
		{
			ppbuf = p->ppbuf;
			
			for(i = p->height; i; i--, ppbuf++)
				mFree(*ppbuf);
	
			mFree(p->ppbuf);
		}

		mFree(p);
	}
}

/** 作成 */

ImageCanvas *ImageCanvas_new(int width,int height,int bits)
{
	ImageCanvas *p;
	uint8_t **ppbuf;
	int i,pitch,bpp;

	p = (ImageCanvas *)mMalloc0(sizeof(ImageCanvas));
	if(!p) return NULL;

	bpp = (bits == 8)? 4: 8;
	pitch = (width * bpp + 15) & ~15; //16byte単位

	p->width = width;
	p->height = height;
	p->bits = bits;
	p->line_bytes = pitch;

	//バッファ

	p->ppbuf = ppbuf = (uint8_t **)mMalloc0(sizeof(void*) * height);
	if(!ppbuf) goto ERR;

	for(i = height; i; i--, ppbuf++)
	{
		*ppbuf = (uint8_t *)mMallocAlign(pitch, 16);
		if(!(*ppbuf)) goto ERR;
	}

	return p;

ERR:
	ImageCanvas_free(p);
	return NULL;
}

/** 指定位置のバッファを取得 (範囲チェックなし) */

uint8_t *ImageCanvas_getBufPt(ImageCanvas *p,int x,int y)
{
	return p->ppbuf[y] + ((p->bits == 8)? (x << 2): (x << 3));
}

/** 色を取得 (RGBA 現在のビット値)
 *
 * 範囲外は 0 */

void ImageCanvas_getPixel_rgba(ImageCanvas *p,int x,int y,void *dst)
{
	uint8_t *src;
	int flag;

	flag = (x < 0 || y < 0 || x >= p->width || y >= p->height);

	if(p->bits == 8)
	{
		if(flag)
			*((uint32_t *)dst) = 0;
		else
		{
			src = p->ppbuf[y] + (x << 2);

			*((uint32_t *)dst) = *((uint32_t *)src);

			*((uint8_t *)dst + 3) = 255;
		}
	}
	else
	{
		if(flag)
			*((uint64_t *)dst) = 0;
		else
		{
			src = p->ppbuf[y] + (x << 3);

			*((uint64_t *)dst) = *((uint64_t *)src);

			*((uint16_t *)dst + 3) = 0x8000;
		}
	}
}

/** 色を取得 */

void ImageCanvas_getPixel_combo(ImageCanvas *p,int x,int y,RGBcombo *dst)
{
	uint8_t *p8;
	uint16_t *p16;

	if(p->bits == 8)
	{
		p8 = p->ppbuf[y] + (x << 2);

		dst->c8.r = p8[0];
		dst->c8.g = p8[1];
		dst->c8.b = p8[2];

		RGB8_to_RGB16(&dst->c16, &dst->c8);
	}
	else
	{
		p16 = (uint16_t *)(p->ppbuf[y] + (x << 3));

		dst->c16.r = p16[0];
		dst->c16.g = p16[1];
		dst->c16.b = p16[2];

		RGB16_to_RGB8(&dst->c8, &dst->c16);
	}
}

/** 指定色で埋める */

void ImageCanvas_fill(ImageCanvas *p,RGBcombo *col)
{
	if(p->bits == 8)
		ImageCanvas_8bit_fill(p, col);
	else
		ImageCanvas_16bit_fill(p, col);
}

/** 範囲を指定色で埋める */

void ImageCanvas_fillBox(ImageCanvas *p,const mBox *box,RGBcombo *col)
{
	if(p->bits == 8)
		ImageCanvas_8bit_fillBox(p, box, col);
	else
		ImageCanvas_16bit_fillBox(p, box, col);
}

/** 範囲をチェック柄で塗りつぶす */

void ImageCanvas_fillPlaidBox(ImageCanvas *p,const mBox *box,RGBcombo *col1,RGBcombo *col2)
{
	if(p->bits == 8)
		ImageCanvas_8bit_fillPlaidBox(p, box, col1, col2);
	else
		ImageCanvas_16bit_fillPlaidBox(p, box, col1, col2);
}

/** すべてのピクセルのアルファ値を最大にする */

void ImageCanvas_setAlphaMax(ImageCanvas *p)
{
	if(p->bits == 8)
		ImageCanvas_8bit_setAlphaMax(p);
	else
		ImageCanvas_16bit_setAlphaMax(p);
}


//===========================
// サムネイル画像
//===========================


/* 縮小 (水平) */

static mlkbool _thumbnail_resize_horz(ImageCanvas *p,int dstw)
{
	uint8_t *buf,**ppbuf,*ps,*pd,*psY;
	int ix,iy,x1,x2,r,g,b,num,srcw,pitchd,fx,finc;

	pitchd = dstw * 3;

	buf = (uint8_t *)mMalloc(pitchd);
	if(!buf) return FALSE;

	srcw = p->width;
	finc = (int)((double)srcw / dstw * (1<<12) + 0.5);

	//

	ppbuf = p->ppbuf;

	for(iy = p->height; iy; iy--, ppbuf++)
	{
		//buf に縮小後の RGB セット
		
		psY = *ppbuf;
		pd = buf;
		fx = 0;

		for(ix = dstw; ix; ix--, pd += 3, fx += finc)
		{
			x1 = fx >> 12;
			x2 = (fx + finc + (1<<11)) >> 12;

			if(x2 > srcw) x2 = srcw;
			if(x1 == x2) x2++;
			
			num = x2 - x1;
			
			ps = psY + x1 * 3;
			r = g = b = 0;

			for(; x1 < x2; x1++, ps += 3)
			{
				r += ps[0];
				g += ps[1];
				b += ps[2];
			}

			pd[0] = r / num;
			pd[1] = g / num;
			pd[2] = b / num;
		}

		//コピー

		memcpy(psY, buf, pitchd);
	}

	mFree(buf);

	return TRUE;
}

/* 縮小 (垂直) */

static mlkbool _thumbnail_resize_vert(ImageCanvas *p,int dstw,int dsth)
{
	uint8_t *buf,**ppbuf,*ps,*pd;
	int ix,iy,y1,y2,r,g,b,num,srch,xpos,fy,finc;

	buf = (uint8_t *)mMalloc(dsth * 3);
	if(!buf) return FALSE;

	srch = p->height;
	finc = (int)((double)srch / dsth * (1<<12) + 0.5);

	//

	xpos = 0;

	for(ix = dstw; ix; ix--, xpos += 3)
	{
		//buf に縮小後の RGB セット

		pd = buf;
		fy = 0;

		for(iy = dsth; iy; iy--, fy += finc)
		{
			y1 = fy >> 12;
			y2 = (fy + finc + (1<<11)) >> 12;

			if(y2 > srch) y2 = srch;
			if(y1 == y2) y2++;
			
			num = y2 - y1;
			
			ppbuf = p->ppbuf + y1;
			r = g = b = 0;

			for(; y1 < y2; y1++, ppbuf++)
			{
				ps = *ppbuf + xpos;
				
				r += ps[0];
				g += ps[1];
				b += ps[2];
			}

			pd[0] = r / num;
			pd[1] = g / num;
			pd[2] = b / num;
			pd += 3;
		}

		//Y1列にセット

		ps = buf;
		ppbuf = p->ppbuf;

		for(iy = dsth; iy; iy--, ps += 3, ppbuf++)
		{
			pd = *ppbuf + xpos;

			pd[0] = ps[0];
			pd[1] = ps[1];
			pd[2] = ps[2];
		}
	}

	mFree(buf);

	return TRUE;
}

/** (8bit) RGB の状態のイメージから、RGB サムネイルイメージに変換
 *
 * 元サイズ <= 変換サイズの場合は、何もしない。 */

mlkbool ImageCanvas_setThumbnailImage_8bit(ImageCanvas *p,int width,int height)
{
	if(p->width > width || p->height > height)
	{
		if(!_thumbnail_resize_horz(p, width))
			return FALSE;

		if(!_thumbnail_resize_vert(p, width, height))
			return FALSE;
	}

	return TRUE;
}


//===========================
// キャンバス描画
//===========================


/** キャンバス描画 (ニアレストネイバー) */

void ImageCanvas_drawPixbuf_nearest(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	if(src->bits == 8)
		ImageCanvas_8bit_drawPixbuf_nearest(src, dst, info);
	else
		ImageCanvas_16bit_drawPixbuf_nearest(src, dst, info);
}

/** キャンバス描画 (回転なし/縮小) */

void ImageCanvas_drawPixbuf_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	if(src->bits == 8)
		ImageCanvas_8bit_drawPixbuf_oversamp(src, dst, info);
	else
		ImageCanvas_16bit_drawPixbuf_oversamp(src, dst, info);
}

/** キャンバス描画 (回転あり/補間なし) */

void ImageCanvas_drawPixbuf_rotate(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	if(src->bits == 8)
		ImageCanvas_8bit_drawPixbuf_rotate(src, dst, info);
	else
		ImageCanvas_16bit_drawPixbuf_rotate(src, dst, info);
}

/** キャンバス描画 (回転あり/補間あり) */

void ImageCanvas_drawPixbuf_rotate_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	if(src->bits == 8)
		ImageCanvas_8bit_drawPixbuf_rotate_oversamp(src, dst, info);
	else
		ImageCanvas_16bit_drawPixbuf_rotate_oversamp(src, dst, info);
}

/* キャンバス描画用、パラメータ取得
 *
 * return: mPixbuf の先頭バッファ位置 (NULL で範囲外) */

uint8_t *__ImageCanvas_getCanvasParam(ImageCanvas *src,mPixbuf *pixbuf,
	CanvasDrawInfo *info,_canvasparam *param,mlkbool rotate)
{
	mBox box;
	double scalex,dx,dy,dcos,dsin;
	int64_t finc_xx,finc_xy,finc_yx,finc_yy;

	if(!mPixbufClip_getBox(pixbuf, &box, &info->boxdst)) return NULL;

	param->dstw = box.w;
	param->dsth = box.h;
	param->srcw = src->width;
	param->srch = src->height;
	param->bpp = pixbuf->pixel_bytes;
	param->pitchd = pixbuf->line_bytes - box.w * param->bpp;
	param->pixbkgnd = mRGBtoPix(info->bkgndcol);

	mPixbufGetFunc_setbuf(pixbuf, &param->setpix);

	//加算数

	scalex = info->param->scalediv;
	dcos = info->param->cosrev;
	dsin = info->param->sinrev;

	if(rotate)
	{
		finc_xx = (int64_t)(dcos * scalex * FIXF_VAL + 0.5);
		finc_xy = (int64_t)(dsin * scalex * FIXF_VAL + 0.5);
		finc_yx = -finc_xy;
		finc_yy = finc_xx;
	}
	else
	{
		finc_xx = finc_yy = (int64_t)(scalex * FIXF_VAL + 0.5);
		finc_xy = finc_yx = 0;
	}

	if(info->mirror)
	{
		scalex = -scalex;
		finc_xx = -finc_xx;
		finc_yx = -finc_yx;
	}

	param->finc_xx = finc_xx;
	param->finc_xy = finc_xy;
	param->finc_yx = finc_yx;
	param->finc_yy = finc_yy;

	//初期位置

	if(rotate)
	{
		dx = info->scrollx;
		dy = info->scrolly;
	
		param->fx = (int64_t)(((dx * dcos - dy * dsin) * scalex + info->originx) * FIXF_VAL)
			+ box.x * finc_xx + box.y * finc_yx;

		param->fy = (int64_t)(((dx * dsin + dy * dcos) * info->param->scalediv + info->originy) * FIXF_VAL)
			+ box.x * finc_xy + box.y * finc_yy;
	}
	else
	{
		param->fx = (int64_t)((info->scrollx * scalex + info->originx) * FIXF_VAL) + box.x * finc_xx;
		param->fy = (int64_t)((info->scrolly * info->param->scalediv + info->originy) * FIXF_VAL) + box.y * finc_yy;
	}

	return mPixbufGetBufPtFast(pixbuf, box.x, box.y);
}

