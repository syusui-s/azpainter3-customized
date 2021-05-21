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
 * アルファ値のみタイプ
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_simd.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "pv_tileimage.h"

#include "imagematerial.h"



//==========================
// 8bit
//==========================


/** タイルと px 位置から色取得 (RGBA8) */

static void _8bit_getpixel_at_tile(TileImage *p,uint8_t *tile,int x,int y,void *dst)
{
	uint8_t *pd = (uint8_t *)dst;

	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	pd[0] = p->col.c8.r;
	pd[1] = p->col.c8.g;
	pd[2] = p->col.c8.b;
	pd[3] = *(tile + (y << 6) + x);
}

/** タイルと px 位置からバッファ位置取得 */

static uint8_t *_8bit_getpixelbuf_at_tile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + (y << 6) + x;
}

/** ピクセルバッファ位置に色をセット */

static void _8bit_setpixel(TileImage *p,uint8_t *buf,int x,int y,void *pix)
{
	*buf = *((uint8_t *)pix + 3);
}

/** すべて透明か */

static mlkbool _8bit_is_transparent_tile(uint8_t *tile)
{
	return TileImage_tile_isTransparent_forA(tile, 64 * 64);
}

/** 2つの色が同じ色か (RGBA) */

static mlkbool _8bit_is_same_rgba(void *col1,void *col2)
{
	return (*((uint8_t *)col1 + 3) == *((uint8_t *)col2 + 3));
}

/** ImageCanvas に合成 */

static void _8bit_blend_tile(TileImage *p,TileImageBlendInfo *infosrc)
{
	TileImageBlendInfo info;
	uint8_t **ppdst,*ps,*pd;
	int pitchs,ix,iy,dx,dy,a,dstx;
	int32_t src[3],dst[3],r,g,b;

	info = *infosrc;

	ps = info.tile + info.sy * 64 + info.sx;
	ppdst = info.dstbuf;
	dstx = info.dx * 4;
	pitchs = 64 - info.w;

	r = p->col.c8.r;
	g = p->col.c8.g;
	b = p->col.c8.b;

	//

	for(iy = info.h, dy = info.dy; iy; iy--, dy++)
	{
		pd = *ppdst + dstx;
		
		for(ix = info.w, dx = info.dx; ix; ix--, dx++, ps++, pd += 4)
		{
			a = *ps;
			if(!a) continue;

			if(info.imgtex)
				a = a * ImageMaterial_getPixel_forTexture(info.imgtex, dx, dy) / 255;

			a = a * info.opacity >> 7;
			if(!a) continue;

			//色合成

			src[0] = r;
			src[1] = g;
			src[2] = b;

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

		ps += pitchs;
		ppdst++;
	}
}

/** RGBA タイルとして取得 */

static void _8bit_gettile_rgba(TileImage *p,uint8_t *dst,const uint8_t *src)
{
	int i;
	uint8_t r,g,b;

	r = p->col.c8.r;
	g = p->col.c8.g;
	b = p->col.c8.b;

	for(i = 64 * 64; i; i--, dst += 4, src++)
	{
		dst[0] = r;
		dst[1] = g;
		dst[2] = b;
		dst[3] = *src;
	}
}

/** RGBA タイルからセット */

static void _8bit_settile_rgba(uint8_t *dst,const uint8_t *src,mlkbool lum_to_a)
{
	int i;

	if(lum_to_a)
	{
		//RGB -> アルファ値
		
		for(i = 64 * 64; i; i--, src += 4)
		{
			if(src[3])
				*(dst++) = 255 - CONV_RGB_TO_LUM(src[0], src[1], src[2]);
			else
				*(dst++) = 0;
		}
	}
	else
	{
		for(i = 64 * 64; i; i--, src += 4)
			*(dst++) = src[3];
	}
}

/** ファイル保存用タイルデータの変換 (読み込み/保存共通) */

static void _8bit_convert_save(uint8_t *dst,uint8_t *src)
{
	memcpy(dst, src, 64 * 64);
}

/** タイルを左右反転 */

static void _8bit_flip_horz(uint8_t *dst,uint8_t *src)
{
	uint8_t *pd,*ps;
	int ix,iy;

	pd = (uint8_t *)dst;
	ps = (uint8_t *)src + 63;

	for(iy = 64; iy; iy--)
	{
		for(ix = 64; ix; ix--, pd++, ps--)
			*pd = *ps;

		ps += 64 * 2;
	}
}

/** タイルを上下反転 */

static void _8bit_flip_vert(uint8_t *dst,uint8_t *src)
{
	int i;

	src += 63 * 64;

	for(i = 64; i; i--)
	{
		memcpy(dst, src, 64);

		dst += 64;
		src -= 64;
	}
}

/** タイルを左に90度回転 */

static void _8bit_rotate_left(uint8_t *dst,uint8_t *src)
{
	uint8_t *pd,*ps;
	int ix,iy;

	pd = (uint8_t *)dst;
	ps = (uint8_t *)src + 63;

	for(iy = 64; iy; iy--)
	{
		for(ix = 64; ix; ix--)
		{
			*(pd++) = *ps;
			ps += 64;
		}

		ps -= 64 * 64 + 1;
	}
}

/** タイルを右に90度回転 */

static void _8bit_rotate_right(uint8_t *dst,uint8_t *src)
{
	uint8_t *pd,*ps;
	int ix,iy;

	pd = (uint8_t *)dst;
	ps = (uint8_t *)src + 63 * 64;

	for(iy = 64; iy; iy--)
	{
		for(ix = 64; ix; ix--)
		{
			*(pd++) = *ps;
			ps -= 64;
		}

		ps += 64 * 64 + 1;
	}
}


//==========================
// 16bit
//==========================


/** タイルと px 位置から色取得 (RGBA16) */

static void _16bit_getpixel_at_tile(TileImage *p,uint8_t *tile,int x,int y,void *dst)
{
	uint16_t *pd = (uint16_t *)dst;

	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	pd[0] = p->col.c16.r;
	pd[1] = p->col.c16.g;
	pd[2] = p->col.c16.b;
	pd[3] = *((uint16_t *)tile + (y << 6) + x);
}

/** タイルと px 位置からバッファ位置取得 */

static uint8_t *_16bit_getpixelbuf_at_tile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + (((y << 6) + x) << 1);
}

/** ピクセルバッファ位置に色をセット */

static void _16bit_setpixel(TileImage *p,uint8_t *buf,int x,int y,void *pix)
{
	*((uint16_t *)buf) = *((uint16_t *)pix + 3);
}

/** すべて透明か */

static mlkbool _16bit_is_transparent_tile(uint8_t *tile)
{
	return TileImage_tile_isTransparent_forA(tile, 64 * 64 * 2);
}

/** 2つの色が同じ色か (RGBA) */

static mlkbool _16bit_is_same_rgba(void *col1,void *col2)
{
	return (*((uint16_t *)col1 + 3) == *((uint16_t *)col2 + 3));
}

/** ImageCanvas に合成 */

static void _16bit_blend_tile(TileImage *p,TileImageBlendInfo *infosrc)
{
	TileImageBlendInfo info;
	uint16_t **ppdst,*ps,*pd;
	int pitchs,ix,iy,dx,dy,a,dstx;
	int32_t src[3],dst[3],r,g,b;

	info = *infosrc;

	ps = (uint16_t *)info.tile + info.sy * 64 + info.sx;
	ppdst = (uint16_t **)info.dstbuf;
	dstx = info.dx * 4;
	pitchs = 64 - info.w;

	r = p->col.c16.r;
	g = p->col.c16.g;
	b = p->col.c16.b;
	
	//

	for(iy = info.h, dy = info.dy; iy; iy--, dy++)
	{
		pd = *ppdst + dstx;
		
		for(ix = info.w, dx = info.dx; ix; ix--, dx++, ps++, pd += 4)
		{
			a = *ps;
			if(!a) continue;

			if(info.imgtex)
				a = a * ImageMaterial_getPixel_forTexture(info.imgtex, dx, dy) / 255;

			a = a * info.opacity >> 7;

			if(!a) continue;

			//RGB 合成

			src[0] = r;
			src[1] = g;
			src[2] = b;

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

		ps += pitchs;
		ppdst++;
	}
}

/** RGBA タイルとして取得 */

static void _16bit_gettile_rgba(TileImage *p,uint8_t *dst,const uint8_t *src)
{
	int i;
	uint16_t r,g,b,*pd;
	const uint16_t *ps;

	pd = (uint16_t *)dst;
	ps = (const uint16_t *)src;

	r = p->col.c16.r;
	g = p->col.c16.g;
	b = p->col.c16.b;

	for(i = 64 * 64; i; i--, pd += 4, ps++)
	{
		pd[0] = r;
		pd[1] = g;
		pd[2] = b;
		pd[3] = *ps;
	}
}

/** RGBA タイルからセット */

static void _16bit_settile_rgba(uint8_t *dst,const uint8_t *src,mlkbool lum_to_a)
{
	uint16_t *pd;
	const uint16_t *ps;
	int i;

	pd = (uint16_t *)dst;
	ps = (const uint16_t *)src;

	if(lum_to_a)
	{
		//RGB -> アルファ値
		
		for(i = 64 * 64; i; i--, ps += 4)
		{
			if(ps[3])
				*(pd++) = 0x8000 - CONV_RGB_TO_LUM(ps[0], ps[1], ps[2]);
			else
				*(pd++) = 0;
		}
	}
	else
	{
		for(i = 64 * 64; i; i--, ps += 4)
			*(pd++) = ps[3];
	}
}

/** ファイル保存用タイルデータからタイルセット
 *
 * 64x64 分 BE で並んでいる */

static void _16bit_convert_from_save(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd = (uint16_t *)dst;
	int i;

	for(i = 64 * 64; i; i--, pd++, src += 2)
		*pd = (src[0] << 8) | src[1];
}

/** ファイル保存用にタイルデータを変換 (BE) */

static void _16bit_convert_to_save(uint8_t *dst,uint8_t *src)
{
	int i;
	uint16_t *ps;

	ps = (uint16_t *)src;

	for(i = 64 * 64; i; i--, ps++, dst += 2)
	{
		dst[0] = *ps >> 8;
		dst[1] = *ps & 255;
	}
}

/** タイルを左右反転 */

static void _16bit_flip_horz(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd,*ps;
	int ix,iy;

	pd = (uint16_t *)dst;
	ps = (uint16_t *)src + 63;

	for(iy = 64; iy; iy--)
	{
		for(ix = 64; ix; ix--, pd++, ps--)
			*pd = *ps;

		ps += 64 * 2;
	}
}

/** タイルを上下反転 */

static void _16bit_flip_vert(uint8_t *dst,uint8_t *src)
{
	int i;

	src += 63 * (64 * 2);

	for(i = 64; i; i--)
	{
		memcpy(dst, src, 64 * 2);

		dst += 64 * 2;
		src -= 64 * 2;
	}
}

/** タイルを左に90度回転 */

static void _16bit_rotate_left(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd,*ps;
	int ix,iy;

	pd = (uint16_t *)dst;
	ps = (uint16_t *)src + 63;

	for(iy = 64; iy; iy--)
	{
		for(ix = 64; ix; ix--)
		{
			*(pd++) = *ps;
			ps += 64;
		}

		ps -= 64 * 64 + 1;
	}
}

/** タイルを右に90度回転 */

static void _16bit_rotate_right(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd,*ps;
	int ix,iy;

	pd = (uint16_t *)dst;
	ps = (uint16_t *)src + 63 * 64;

	for(iy = 64; iy; iy--)
	{
		for(ix = 64; ix; ix--)
		{
			*(pd++) = *ps;
			ps -= 64;
		}

		ps += 64 * 64 + 1;
	}
}


//==========================
// 関数をセット
//==========================


/** 関数をセット */

void __TileImage_setColFunc_alpha(TileImageColFuncData *p,int bits)
{
	if(bits == 8)
	{
		p->is_transparent_tile = _8bit_is_transparent_tile;
		p->is_same_rgba = _8bit_is_same_rgba;
		p->blend_tile = _8bit_blend_tile;
		p->getpixel_at_tile = _8bit_getpixel_at_tile;
		p->getpixelbuf_at_tile = _8bit_getpixelbuf_at_tile;
		p->setpixel = _8bit_setpixel;
		p->convert_from_save = _8bit_convert_save;
		p->convert_to_save = _8bit_convert_save;
		p->flip_horz = _8bit_flip_horz;
		p->flip_vert = _8bit_flip_vert;
		p->rotate_left = _8bit_rotate_left;
		p->rotate_right = _8bit_rotate_right;
		p->gettile_rgba = _8bit_gettile_rgba;
		p->settile_rgba = _8bit_settile_rgba;
	}
	else
	{
		p->is_transparent_tile = _16bit_is_transparent_tile;
		p->is_same_rgba = _16bit_is_same_rgba;
		p->blend_tile = _16bit_blend_tile;
		p->getpixel_at_tile = _16bit_getpixel_at_tile;
		p->getpixelbuf_at_tile = _16bit_getpixelbuf_at_tile;
		p->setpixel = _16bit_setpixel;
		p->convert_from_save = _16bit_convert_from_save;
		p->convert_to_save = _16bit_convert_to_save;
		p->flip_horz = _16bit_flip_horz;
		p->flip_vert = _16bit_flip_vert;
		p->rotate_left = _16bit_rotate_left;
		p->rotate_right = _16bit_rotate_right;
		p->gettile_rgba = _16bit_gettile_rgba;
		p->settile_rgba = _16bit_settile_rgba;
	}
}


