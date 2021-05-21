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

/*****************************************
 * TileImage
 * A1bit タイプ
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_simd.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "pv_tileimage.h"

#include "imagematerial.h"
#include "table_data.h"


/*
 * - 上位ビットから順。
 * - アルファ値が 0 より大きければアルファ値を ON とする。
 */


//===========================
// 8bit/16bit 共通
//===========================


/** タイルと px 位置からバッファ位置取得
 *
 * ビット位置は別途処理 */

static uint8_t *_getpixelbuf_at_tile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + ((y << 3) + (x >> 3));
}

/** すべて透明か */

static mlkbool _is_transparent_tile(uint8_t *tile)
{
	return TileImage_tile_isTransparent_forA(tile, 64 * 64 / 8);
}

/** ファイル保存用タイルデータの変換 (読み込み/保存共通) */

static void _convert_save(uint8_t *dst,uint8_t *src)
{
	memcpy(dst, src, 64 * 64 / 8);
}

/** タイルを左右反転 */

static void _flip_horz(uint8_t *dst,uint8_t *src)
{
	uint8_t *ps,fd,fs,vd,vs;
	int ix,iy,i;

	ps = src + 7;

	for(iy = 64; iy; iy--)
	{
		for(ix = 8; ix; ix--)
		{
			vd = 0;
			vs = *(ps--);
			fd = 0x80;
			fs = 1;
			
			for(i = 8; i; i--)
			{
				if(vs & fs) vd |= fd;

				fd >>= 1;
				fs <<= 1;
			}

			*(dst++) = vd;
		}

		ps += 8 * 2;
	}
}

/** タイルを上下反転 */

static void _flip_vert(uint8_t *dst,uint8_t *src)
{
	uint64_t *pd,*ps;
	int i;

	pd = (uint64_t *)dst;
	ps = (uint64_t *)src + 63;

	for(i = 64; i; i--)
		*(pd++) = *(ps--);
}

/** タイルを左に90度回転 */

static void _rotate_left(uint8_t *dst,uint8_t *src)
{
	int ix,iy,i;
	uint8_t *ps,fd,fs,vd;

	src += 7;
	fs = 1;

	for(iy = 64; iy; iy--)
	{
		ps = src;
	
		for(ix = 8; ix; ix--)
		{
			vd = 0;
			fd = 0x80;

			for(i = 8; i; i--)
			{
				if(*ps & fs) vd |= fd;
				ps += 8;
				fd >>= 1;
			}

			*(dst++) = vd;
		}

		fs <<= 1;
		if(!fs) src--, fs = 1;
	}
}

/** タイルを右に90度回転 */

static void _rotate_right(uint8_t *dst,uint8_t *src)
{
	int ix,iy,i;
	uint8_t *ps,fd,fs,vd;

	src += 63 * 8;
	fs = 0x80;

	for(iy = 64; iy; iy--)
	{
		ps = src;
	
		for(ix = 8; ix; ix--)
		{
			vd = 0;
			fd = 0x80;

			for(i = 8; i; i--)
			{
				if(*ps & fs) vd |= fd;
				ps -= 8;
				fd >>= 1;
			}

			*(dst++) = vd;
		}

		fs >>= 1;
		if(!fs) src++, fs = 0x80;
	}
}


//===========================
// 8bit
//===========================


/** タイルと px 位置から色取得 (RGBA8) */

static void _8bit_getpixel_at_tile(TileImage *p,uint8_t *tile,int x,int y,void *dst)
{
	uint8_t *pd = (uint8_t *)dst;

	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	tile += (y << 3) + (x >> 3);

	pd[0] = p->col.c8.r;
	pd[1] = p->col.c8.g;
	pd[2] = p->col.c8.b;
	pd[3] = ( *tile & (1 << (7 - (x & 7))) )? 255: 0;
}

/** ピクセルバッファ位置に色をセット */

static void _8bit_setpixel(TileImage *p,uint8_t *buf,int x,int y,void *pix)
{
	x = (x - p->offx) & 63;
	x = 1 << (7 - (x & 7));

	if(*((uint8_t *)pix + 3))
		*buf |= x;
	else
		*buf &= ~x;
}

/** 2つの色が同じ色か (RGBA) */

static mlkbool _8bit_is_same_rgba(void *col1,void *col2)
{
	return (!*((uint8_t *)col1 + 3) == !*((uint8_t *)col2 + 3));
}

/** ImageCanvas に合成 */

static void _8bit_blend_tile(TileImage *p,TileImageBlendInfo *infosrc)
{
	TileImageBlendInfo info;
	uint8_t **ppdst,*ps,*psY,*pd,fleft,f,fval;
	int ix,iy,dx,dy,a,dstx,c,cx,cy,thval;
	int32_t src[3],dst[3],r,g,b;
	int64_t fxx,fxy;

	info = *infosrc;

	psY = info.tile + ((info.sy << 3) + (info.sx >> 3));
	ppdst = info.dstbuf;
	dstx = info.dx * 4;

	fleft = 1 << (7 - (info.sx & 7));

	r = p->col.c8.r;
	g = p->col.c8.g;
	b = p->col.c8.b;

	//

	for(iy = info.h, dy = info.dy; iy; iy--, dy++)
	{
		pd = *ppdst + dstx;
		ps = psY;
		f = fleft;
		fval = *(ps++);
		fxx = info.tone_fx;
		fxy = info.tone_fy;
	
		for(ix = info.w, dx = info.dx; ix;
			ix--, dx++, pd += 4, f >>= 1, fxx += info.tone_fcos, fxy += info.tone_fsin)
		{
			if(!f)
				f = 0x80, fval = *(ps++);

			if(!(fval & f)) continue;

			//

			if(info.imgtex)
				a = ImageMaterial_getPixel_forTexture(info.imgtex, dx, dy);
			else
				a = 255;

			a = a * info.opacity >> 7;

			if(!a) continue;

			//トーン化

			if(!info.is_tone)
			{
				if(info.tone_repcol == -1)
				{
					//通常
					src[0] = r;
					src[1] = g;
					src[2] = b;
				}
				else
				{
					//トーンのグレイスケール化
					src[0] = src[1] = src[2] = info.tone_repcol;
				}
			}
			else
			{
				//固定濃度でなければ、色は常に黒
				c = (info.tone_density)? 255 - info.tone_density: 0;

				cx = fxx >> (TILEIMG_TONE_FIX_BITS - TABLEDATA_TONE_BITS);
				cy = fxy >> (TILEIMG_TONE_FIX_BITS - TABLEDATA_TONE_BITS);

				if(c < 128)
				{
					cx += TABLEDATA_TONE_WIDTH / 2;
					cy += TABLEDATA_TONE_WIDTH / 2;
				}

				thval = TABLEDATA_TONE_GETVAL8(cx, cy);

				if(c < 128)
					thval = 255 - thval;
				
				if(c > thval)
				{
					//透明 or 白

					if(info.is_tone_bkgnd_tp) continue;

					src[0] = src[1] = src[2] = 255;
				}
				else
				{
					src[0] = r;
					src[1] = g;
					src[2] = b;
				}
			}

			//色合成

			dst[0] = pd[0];
			dst[1] = pd[1];
			dst[2] = pd[2];

			if((info.func_blend)(src, dst, a) && a != 255)
			{
				//アルファ合成

				src[0] = (src[0] - dst[0]) * a / 255 + dst[0];
				src[1] = (src[1] - dst[1]) * a / 255 + dst[1];
				src[2] = (src[2] - dst[2]) * a / 255 + dst[2];
			}

			//セット

			pd[0] = src[0];
			pd[1] = src[1];
			pd[2] = src[2];
		}

		psY += 8;
		ppdst++;

		info.tone_fx -= info.tone_fsin;
		info.tone_fy += info.tone_fcos;
	}
}

/** RGBA タイルとして取得 */

static void _8bit_gettile_rgba(TileImage *p,uint8_t *dst,const uint8_t *src)
{
	int i;
	uint8_t r,g,b,f;

	r = p->col.c8.r;
	g = p->col.c8.g;
	b = p->col.c8.b;

	f = 0x80;

	for(i = 64 * 64; i; i--, dst += 4)
	{
		dst[0] = r;
		dst[1] = g;
		dst[2] = b;
		dst[3] = (*src & f)? 255: 0;

		f >>= 1;
		if(!f) f = 0x80, src++;
	}
}

/** RGBA タイルからセット */

static void _8bit_settile_rgba(uint8_t *dst,const uint8_t *src,mlkbool lum_to_a)
{
	int i,j;
	uint8_t f,val,a;

	for(i = 64 * 8; i; i--)
	{
		f = 0x80;
		val = 0;
		
		for(j = 8; j; j--, f >>= 1, src += 4)
		{
			a = src[3];

			if(lum_to_a && a)
				a = 255 - CONV_RGB_TO_LUM(src[0], src[1], src[2]);

			if(a) val |= f;
		}

		*(dst++) = val;
	}
}


//===========================
// 16bit
//===========================


/** タイルと px 位置から色取得 (RGBA16) */

static void _16bit_getpixel_at_tile(TileImage *p,uint8_t *tile,int x,int y,void *dst)
{
	uint16_t *pd = (uint16_t *)dst;

	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	tile += (y << 3) + (x >> 3);

	pd[0] = p->col.c16.r;
	pd[1] = p->col.c16.g;
	pd[2] = p->col.c16.b;
	pd[3] = ( *tile & (1 << (7 - (x & 7))) )? 0x8000: 0;
}

/** ピクセルバッファ位置に色をセット */

static void _16bit_setpixel(TileImage *p,uint8_t *buf,int x,int y,void *pix)
{
	x = (x - p->offx) & 63;
	x = 1 << (7 - (x & 7));

	if(*((uint16_t *)pix + 3))
		*buf |= x;
	else
		*buf &= ~x;
}

/** 2つの色が同じ色か (RGBA) */

static mlkbool _16bit_is_same_rgba(void *col1,void *col2)
{
	return (!*((uint16_t *)col1 + 3) == !*((uint16_t *)col2 + 3));
}

/** ImageCanvas に合成 */

static void _16bit_blend_tile(TileImage *p,TileImageBlendInfo *infosrc)
{
	TileImageBlendInfo info;
	uint8_t *ps,*psY,fleft,f,fval;
	uint16_t **ppdst,*pd;
	int ix,iy,dx,dy,a,dstx,c,cx,cy,thval;
	int32_t src[3],dst[3],r,g,b;
	int64_t fxx,fxy;

	info = *infosrc;

	psY = info.tile + ((info.sy << 3) + (info.sx >> 3));
	ppdst = (uint16_t **)info.dstbuf;
	dstx = info.dx * 4;

	fleft = 1 << (7 - (info.sx & 7));

	r = p->col.c16.r;
	g = p->col.c16.g;
	b = p->col.c16.b;

	//

	for(iy = info.h, dy = info.dy; iy; iy--, dy++)
	{
		pd = *ppdst + dstx;
		ps = psY;
		f = fleft;
		fval = *(ps++);
		fxx = info.tone_fx;
		fxy = info.tone_fy;
		
		for(ix = info.w, dx = info.dx; ix;
			ix--, dx++, pd += 4, f >>= 1, fxx += info.tone_fcos, fxy += info.tone_fsin)
		{
			if(!f)
				f = 0x80, fval = *(ps++);

			if(!(fval & f)) continue;

			//

			if(info.imgtex)
				a = (ImageMaterial_getPixel_forTexture(info.imgtex, dx, dy) << 15) / 255;
			else
				a = 0x8000;

			a = a * info.opacity >> 7;

			if(!a) continue;

			//トーン化

			if(!info.is_tone)
			{
				if(info.tone_repcol == -1)
				{
					//通常
					src[0] = r;
					src[1] = g;
					src[2] = b;
				}
				else
				{
					//トーンのグレイスケール化
					src[0] = src[1] = src[2] = info.tone_repcol;
				}
			}
			else
			{
				//固定濃度でなければ、色は常に黒
				c = (info.tone_density)? 0x8000 - info.tone_density: 0;

				cx = fxx >> (TILEIMG_TONE_FIX_BITS - TABLEDATA_TONE_BITS);
				cy = fxy >> (TILEIMG_TONE_FIX_BITS - TABLEDATA_TONE_BITS);

				if(c < 0x4000)
				{
					cx += TABLEDATA_TONE_WIDTH / 2;
					cy += TABLEDATA_TONE_WIDTH / 2;
				}

				thval = TABLEDATA_TONE_GETVAL16(cx, cy);

				if(c < 0x4000)
					thval = 0x8000 - thval;
				
				if(c > thval)
				{
					//透明 or 白

					if(info.is_tone_bkgnd_tp) continue;

					src[0] = src[1] = src[2] = 0x8000;
				}
				else
				{
					src[0] = r;
					src[1] = g;
					src[2] = b;
				}
			}

			//色合成

			dst[0] = pd[0];
			dst[1] = pd[1];
			dst[2] = pd[2];

			if((info.func_blend)(src, dst, a) && a != 0x8000)
			{
				//アルファ合成

				src[0] = ((src[0] - dst[0]) * a >> 15) + dst[0];
				src[1] = ((src[1] - dst[1]) * a >> 15) + dst[1];
				src[2] = ((src[2] - dst[2]) * a >> 15) + dst[2];
			}

			//セット

			pd[0] = src[0];
			pd[1] = src[1];
			pd[2] = src[2];
		}

		psY += 8;
		ppdst++;

		info.tone_fx -= info.tone_fsin;
		info.tone_fy += info.tone_fcos;
	}
}

/** RGBA タイルとして取得 */

static void _16bit_gettile_rgba(TileImage *p,uint8_t *dst,const uint8_t *src)
{
	int i;
	uint16_t r,g,b;
	uint8_t f;
	uint16_t *pd;

	pd = (uint16_t *)dst;

	r = p->col.c16.r;
	g = p->col.c16.g;
	b = p->col.c16.b;

	f = 0x80;

	for(i = 64 * 64; i; i--, pd += 4)
	{
		pd[0] = r;
		pd[1] = g;
		pd[2] = b;
		pd[3] = (*src & f)? 0x8000: 0;

		f >>= 1;
		if(!f) f = 0x80, src++;
	}
}

/** RGBA タイルからセット */

static void _16bit_settile_rgba(uint8_t *dst,const uint8_t *src,mlkbool lum_to_a)
{
	int i,j,a;
	uint8_t f,val;
	const uint16_t *ps;

	ps = (const uint16_t *)src;

	for(i = 64 * 8; i; i--)
	{
		f = 0x80;
		val = 0;
		
		for(j = 8; j; j--, f >>= 1, ps += 4)
		{
			a = ps[3];

			if(lum_to_a && a)
				a = 0x8000 - CONV_RGB_TO_LUM(ps[0], ps[1], ps[2]);

			if(a) val |= f;
		}

		*(dst++) = val;
	}
}


//==========================
//
//==========================


/** 関数をセット */

void __TileImage_setColFunc_alpha1bit(TileImageColFuncData *p,int bits)
{
	p->is_transparent_tile = _is_transparent_tile;
	p->convert_from_save = _convert_save;
	p->convert_to_save = _convert_save;
	p->flip_horz = _flip_horz;
	p->flip_vert = _flip_vert;
	p->rotate_left = _rotate_left;
	p->rotate_right = _rotate_right;
	p->getpixelbuf_at_tile = _getpixelbuf_at_tile;

	if(bits == 8)
	{
		p->getpixel_at_tile = _8bit_getpixel_at_tile;
		p->setpixel = _8bit_setpixel;
		p->is_same_rgba = _8bit_is_same_rgba;
		p->blend_tile = _8bit_blend_tile;
		p->gettile_rgba = _8bit_gettile_rgba;
		p->settile_rgba = _8bit_settile_rgba;
	}
	else
	{
		p->getpixel_at_tile = _16bit_getpixel_at_tile;
		p->setpixel = _16bit_setpixel;
		p->is_same_rgba = _16bit_is_same_rgba;
		p->blend_tile = _16bit_blend_tile;
		p->gettile_rgba = _16bit_gettile_rgba;
		p->settile_rgba = _16bit_settile_rgba;
	}
}

