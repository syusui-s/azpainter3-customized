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
 * ImageCanvas: 16bit
 ******************************/

#include <string.h>

#include "mlk.h"
#include "mlk_pixbuf.h"
#include "mlk_simd.h"

#include "imagecanvas.h"
#include "colorvalue.h"

#include "pv_imagecanvas.h"


//----------------

#define _PTR16_TO_PIX(ps)     mRGBtoPix_sep(ps[0] * 255 >> 15, ps[1] * 255 >> 15, ps[2] * 255 >> 15)
#define _RGB16_TO_PIX(r,g,b)  mRGBtoPix_sep((r) * 255 >> 15, (g) * 255 >> 15, (b) * 255 >> 15)

//----------------


/** すべて塗りつぶす */

void ImageCanvas_16bit_fill(ImageCanvas *p,RGBcombo *col)
{
	uint8_t **ppbuf;
	int ix,iy,w;
	uint64_t c64;

	ppbuf = p->ppbuf;

	c64 = RGB16_to_64bit_buf(&col->c16);

#if MLK_ENABLE_SSE2 && _SIMD_ON
	//16byte 単位でセット

	__m128i v128,*pd;

	v128 = _mm_set1_epi64x(c64);
	w = p->line_bytes / 16;

	for(iy = p->height; iy; iy--, ppbuf++)
	{
		pd = (__m128i *)*ppbuf;
		
		for(ix = w; ix; ix--, pd++)
			_mm_store_si128(pd, v128);
	}

#else
	//8byte 単位でセット

	uint64_t *pd;

	w = p->line_bytes / 8;

	for(iy = p->height; iy; iy--, ppbuf++)
	{
		pd = (uint64_t *)*ppbuf;
		
		for(ix = w; ix; ix--)
			*(pd++) = c64;
	}

#endif
}

/** 範囲を指定色で埋める */

void ImageCanvas_16bit_fillBox(ImageCanvas *p,const mBox *box,RGBcombo *col)
{
	uint8_t **ppbuf,*pd;
	int ix,iy,w,xtop;
	uint64_t c64;

	ppbuf = p->ppbuf + box->y;
	xtop = box->x << 3;
	w = box->w;

	c64 = RGB16_to_64bit_buf(&col->c16);

#if MLK_ENABLE_SSE2 && _SIMD_ON

	__m128i v128;
	int w1,w2,w3;

	if(w >= 8)
	{
		w1 = box->x & 1;		//16byte 境界までの幅
		w2 = (w - w1) / 2;		//16byte 転送分
		w3 = w - w1 - w2 * 2;	//残り
		
		v128 = _mm_set1_epi64x(c64);

		for(iy = box->h; iy > 0; iy--, ppbuf++)
		{
			pd = *ppbuf + xtop;

			//16byte 境界までは 8byte 単位

			if(w1)
			{
				*((uint64_t *)pd) = c64;
				pd += 8;
			}

			//16byte 転送
			
			for(ix = w2; ix; ix--, pd += 16)
				_mm_store_si128((__m128i *)pd, v128);

			//残り

			if(w3)
				*((uint64_t *)pd) = c64;
		}

		return;
	}

#endif

	//----- 通常

	for(iy = box->h; iy > 0; iy--, ppbuf++)
	{
		pd = *ppbuf + xtop;
		
		for(ix = w; ix; ix--, pd += 8)
			*((uint64_t *)pd) = c64;
	}
}

/** 範囲をチェック柄で塗りつぶす */

void ImageCanvas_16bit_fillPlaidBox(ImageCanvas *p,const mBox *box,RGBcombo *col1,RGBcombo *col2)
{
	uint64_t **ppbuf,*pd,c64[2];
	int ix,iy,xx,yy,fy;

	c64[0] = RGB16_to_64bit_buf(&col1->c16);;
	c64[1] = RGB16_to_64bit_buf(&col2->c16);;

	ppbuf = (uint64_t **)(p->ppbuf + box->y);

	//高さ 16px 分を埋める

	iy = (box->h >= 16)? 16: box->h;
	yy = box->y & 15;

	for(; iy > 0; iy--, ppbuf++)
	{
		pd = *ppbuf + box->x;
		xx = box->x & 15;
		fy = yy >> 3;

		for(ix = box->w; ix > 0; ix--)
		{
			*(pd++) = c64[fy ^ (xx >> 3)];

			xx = (xx + 1) & 15;
		}

		yy = (yy + 1) & 15;
	}

	//残りは 16px 前の行をコピー

	if(box->h > 16)
	{
		ix = box->x;
		xx = box->w << 3;

		for(iy = box->h - 16; iy > 0; iy--, ppbuf++)
			memcpy(*ppbuf + ix, ppbuf[-16] + ix, xx);
	}
}

/** すべてのピクセルのアルファ値を最大にする */

void ImageCanvas_16bit_setAlphaMax(ImageCanvas *p)
{
	uint8_t **ppbuf = p->ppbuf;
	uint16_t *pd;
	int ix,iy;

	for(iy = p->height; iy; iy--, ppbuf++)
	{
		pd = (uint16_t *)*ppbuf + 3;
		
		for(ix = p->width; ix; ix--, pd += 4)
			*pd = 0x8000;
	}
}

/** キャンバス描画 (ニアレストネイバー) */

void ImageCanvas_16bit_drawPixbuf_nearest(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	_canvasparam cp;
	int ix,iy,n;
	uint8_t *pd,*psY;
	uint16_t *ps;
	int64_t fx,fy;

	pd = __ImageCanvas_getCanvasParam(src, dst, info, &cp, FALSE);
	if(!pd) return;

	fy = cp.fy;

	for(iy = cp.dsth; iy > 0; iy--, fy += cp.finc_yy)
	{
		n = fy >> FIXF_BIT;

		//Yが範囲外

		if(fy < 0 || n >= cp.srch)
		{
			mPixbufBufLineH(dst, pd, cp.dstw, cp.pixbkgnd);
			pd += dst->line_bytes;
			continue;
		}

		//X

		psY = src->ppbuf[n];
		fx = cp.fx;

		for(ix = cp.dstw; ix > 0; ix--, fx += cp.finc_xx, pd += cp.bpp)
		{
			n = fx >> FIXF_BIT;

			if(fx < 0 || n >= cp.srcw)
				//範囲外
				(cp.setpix)(pd, cp.pixbkgnd);
			else
			{
				ps = (uint16_t *)(psY + (n << 3));
			
				(cp.setpix)(pd, _PTR16_TO_PIX(ps));
			}
		}

		pd += cp.pitchd;
	}
}

/** キャンバス描画 (回転なし/縮小) */

void ImageCanvas_16bit_drawPixbuf_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	_canvasparam cp;
	int ix,iy,jx,jy,n,r,g,b,tbl_xpos[8];
	uint8_t **ppbuf,*pd,*tbl_psY[8];
	uint16_t *ps;
	int64_t fx,fy,fincx2,fincy2,ftmp;

	pd = __ImageCanvas_getCanvasParam(src, dst, info, &cp, FALSE);
	if(!pd) return;

	fincx2 = cp.finc_xx >> 3;
	fincy2 = cp.finc_yy >> 3;

	//

	ppbuf = src->ppbuf;
	fy = cp.fy;

	for(iy = cp.dsth; iy > 0; iy--, fy += cp.finc_yy)
	{
		n = fy >> FIXF_BIT;

		//Yが範囲外

		if(n < 0 || n >= cp.srch)
		{
			mPixbufBufLineH(dst, pd, cp.dstw, cp.pixbkgnd);
			pd += dst->line_bytes;
			continue;
		}

		//Y テーブル

		for(jy = 0, ftmp = fy; jy < 8; jy++, ftmp += fincy2)
		{
			n = ftmp >> FIXF_BIT;
			if(n >= cp.srch) n = cp.srch - 1;

			tbl_psY[jy] = ppbuf[n];
		}

		//----- X

		fx = cp.fx;

		for(ix = cp.dstw; ix > 0; ix--, fx += cp.finc_xx, pd += cp.bpp)
		{
			n = fx >> FIXF_BIT;

			//範囲外

			if(n < 0 || n >= cp.srcw)
			{
				(cp.setpix)(pd, cp.pixbkgnd);
				continue;
			}

			//X テーブル

			for(jx = 0, ftmp = fx; jx < 8; jx++, ftmp += fincx2)
			{
				n = ftmp >> FIXF_BIT;
				if(n < 0) n = 0;
				else if(n >= cp.srcw) n = cp.srcw - 1;

				tbl_xpos[jx] = n << 3;
			}

			//オーバーサンプリング

			r = g = b = 0;

			for(jy = 0; jy < 8; jy++)
			{
				for(jx = 0; jx < 8; jx++)
				{
					ps = (uint16_t *)(tbl_psY[jy] + tbl_xpos[jx]);

					r += ps[0];
					g += ps[1];
					b += ps[2];
				}
			}

			r >>= 6;
			g >>= 6;
			b >>= 6;

			(cp.setpix)(pd, _RGB16_TO_PIX(r,g,b));
		}

		pd += cp.pitchd;
	}
}

/** キャンバス描画 (回転あり/補間なし) */

void ImageCanvas_16bit_drawPixbuf_rotate(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	_canvasparam cp;
	int ix,iy,sx,sy;
	uint8_t **ppbuf,*pd;
	uint16_t *ps;
	int64_t fx,fy,fxY,fyY;

	pd = __ImageCanvas_getCanvasParam(src, dst, info, &cp, TRUE);
	if(!pd) return;

	ppbuf = src->ppbuf;

	fxY = cp.fx;
	fyY = cp.fy;

	for(iy = cp.dsth; iy > 0; iy--)
	{
		fx = fxY;
		fy = fyY;
	
		//X

		for(ix = cp.dstw; ix > 0; ix--, pd += cp.bpp)
		{
			sx = fx >> FIXF_BIT;
			sy = fy >> FIXF_BIT;

			if(sx < 0 || sy < 0 || sx >= cp.srcw || sy >= cp.srch)
				//範囲外
				(cp.setpix)(pd, cp.pixbkgnd);
			else
			{
				ps = (uint16_t *)(ppbuf[sy] + (sx << 3));
			
				(cp.setpix)(pd, _PTR16_TO_PIX(ps));
			}

			fx += cp.finc_xx;
			fy += cp.finc_xy;
		}

		fxY += cp.finc_yx;
		fyY += cp.finc_yy;
		pd += cp.pitchd;
	}
}

/** キャンバス描画 (回転あり/補間あり) */

void ImageCanvas_16bit_drawPixbuf_rotate_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info)
{
	_canvasparam cp;
	int ix,iy,jx,jy,sx,sy,r,g,b;
	uint8_t **ppbuf,*pd;
	uint16_t *ps;
	int64_t fx,fy,fxY,fyY,finc_xx2,finc_xy2,finc_yx2,finc_yy2,
		fjxx,fjxy,fjyx,fjyy;

	pd = __ImageCanvas_getCanvasParam(src, dst, info, &cp, TRUE);
	if(!pd) return;

	finc_xx2 = cp.finc_xx / 5;
	finc_xy2 = cp.finc_xy / 5;
	finc_yx2 = cp.finc_yx / 5;
	finc_yy2 = cp.finc_yy / 5;

	//

	ppbuf = src->ppbuf;

	fxY = cp.fx;
	fyY = cp.fy;

	for(iy = cp.dsth; iy > 0; iy--)
	{
		fx = fxY;
		fy = fyY;
	
		//X

		for(ix = cp.dstw; ix > 0; ix--, pd += cp.bpp)
		{
			sx = fx >> FIXF_BIT;
			sy = fy >> FIXF_BIT;

			if(sx < 0 || sy < 0 || sx >= cp.srcw || sy >= cp.srch)
				//範囲外
				(cp.setpix)(pd, cp.pixbkgnd);
			else
			{
				r = g = b = 0;
				fjyx = fx;
				fjyy = fy;

				for(jy = 5; jy; jy--, fjyx += finc_yx2, fjyy += finc_yy2)
				{
					fjxx = fjyx, fjxy = fjyy;

					for(jx = 5; jx; jx--, fjxx += finc_xx2, fjxy += finc_xy2)
					{
						sx = fjxx >> FIXF_BIT;
						sy = fjxy >> FIXF_BIT;

						if(sx < 0) sx = 0; else if(sx >= cp.srcw) sx = cp.srcw - 1;
						if(sy < 0) sy = 0; else if(sy >= cp.srch) sy = cp.srch - 1;

						ps = (uint16_t *)(ppbuf[sy] + (sx << 3));

						r += ps[0];
						g += ps[1];
						b += ps[2];
					}
				}

				r /= 25;
				g /= 25;
				b /= 25;

				(cp.setpix)(pd, _RGB16_TO_PIX(r,g,b));
			}

			fx += cp.finc_xx;
			fy += cp.finc_xy;
		}

		fxY += cp.finc_yx;
		fyY += cp.finc_yy;
		pd += cp.pitchd;
	}
}
