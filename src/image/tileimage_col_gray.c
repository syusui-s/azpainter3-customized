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
 * GRAY+A タイプ
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_simd.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "pv_tileimage.h"

#include "imagematerial.h"
#include "table_data.h"


//==========================
// 8bit
//==========================


/** タイルと px 位置から色取得 (RGBA8) */

static void _8bit_getpixel_at_tile(TileImage *p,uint8_t *tile,int x,int y,void *dst)
{
	uint8_t *pd,*ps;

	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	pd = (uint8_t *)dst;
	ps = tile + (((y << 6) + x) << 1);

	pd[0] = pd[1] = pd[2] = ps[0];
	pd[3] = ps[1];
}

/** タイルと px 位置からバッファ位置取得 */

static uint8_t *_8bit_getpixelbuf_at_tile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + (((y << 6) + x) << 1);
}

/** ピクセルバッファ位置に色をセット */

static void _8bit_setpixel(TileImage *p,uint8_t *buf,int x,int y,void *pix)
{
	uint8_t *ps = (uint8_t *)pix;

	buf[0] = (ps[0] + ps[1] + ps[2]) / 3;
	buf[1] = ps[3];
}

/** RGBA (8bit) イメージバッファからタイルイメージに変換
 *
 * ppbuf: Y 開始位置 (16byte 境界)
 * x: 64px 単位 */

static void _8bit_rgba_to_tile(uint8_t *tile,uint8_t **ppbuf,int x,int w,int h)
{
	uint8_t *pd,*ps;
	int ix,iy,xpos;

	xpos = x * 4;

	for(iy = h; iy; iy--, ppbuf++, tile += 64 * 2)
	{
		pd = tile;
		ps = *ppbuf + xpos;
		
		for(ix = w; ix; ix--, pd += 2, ps += 4)
		{
			pd[0] = ps[0];
			pd[1] = ps[3];
		}
	}
}

/** すべて透明か */

static mlkbool _8bit_is_transparent_tile(uint8_t *tile)
{
	int i;

#if MLK_ENABLE_SSE2 && _TILEIMG_SIMD_ON

	__m128i v,vand,vcmp,*ps;
	uint8_t mask[] = {0,255};

	ps = (__m128i *)tile;

	vand = _mm_set1_epi16(*((int16_t *)mask));
	vcmp = _mm_setzero_si128();

	for(i = 64 * 64 / 8; i; i--, ps++)
	{
		v = _mm_load_si128(ps);
		v = _mm_and_si128(v, vand);		//すべて A=0 なら、0
		v = _mm_cmpeq_epi32(v, vcmp);	//32bit単位で、0なら全ビットON

		//8bit x 16個の最上位ビット取得
		if(_mm_movemask_epi8(v) != 0xffff)
			return FALSE;
	}

	return TRUE;

#else

	tile += 1;

	for(i = 64 * 64; i; i--, tile += 2)
	{
		if(*tile) return FALSE;
	}

	return TRUE;

#endif
}

/** 2つの色が同じ色か */

static mlkbool _8bit_is_same_rgba(void *col1,void *col2)
{
	uint8_t *ps,*pd;

	ps = (uint8_t *)col1;
	pd = (uint8_t *)col2;

	return (
		((ps[0] + ps[1] + ps[2]) / 3 == pd[0] && ps[3] == pd[3])
		|| (ps[3] == 0 && pd[3] == 0)
	);
}

/** ImageCanvas に合成 */

static void _8bit_blend_tile(TileImage *p,TileImageBlendInfo *infosrc)
{
	TileImageBlendInfo info;
	uint8_t **ppdst,*ps,*pd,r,g,b;
	int pitchs,ix,iy,dx,dy,a,c,dstx,cx,cy,thval;
	int32_t src[3],dst[3];
	int64_t fxx,fxy;

	info = *infosrc;

	ps = info.tile + (info.sy * 64 + info.sx) * 2;
	ppdst = info.dstbuf;
	dstx = info.dx * 4;
	pitchs = (64 - info.w) * 2;

	r = p->col.c8.r;
	g = p->col.c8.g;
	b = p->col.c8.b;

	//

	for(iy = info.h, dy = info.dy; iy; iy--, dy++)
	{
		pd = *ppdst + dstx;
		fxx = info.tone_fx;
		fxy = info.tone_fy;
		
		for(ix = info.w, dx = info.dx; ix; ix--, dx++, ps += 2, pd += 4,
				fxx += info.tone_fcos, fxy += info.tone_fsin)
		{
			a = ps[1];
			if(!a) continue;

			if(info.imgtex)
				a = a * ImageMaterial_getPixel_forTexture(info.imgtex, dx, dy) / 255;

			a = a * info.opacity >> 7;

			if(!a) continue;

			//トーン化

			if(!info.is_tone)
			{
				//通常 or トーンをグレイスケール表示
				
				if(info.tone_repcol == -1)
					c = ps[0];
				else
					c = info.tone_repcol;

				src[0] = src[1] = src[2] = c;
			}
			else
			{
				//トーン表示
				
				c = (info.tone_density)? 255 - info.tone_density: ps[0];

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
					//黒 = レイヤ色
					
					src[0] = r;
					src[1] = g;
					src[2] = b;
				}
			}

			//RGB 合成

			dst[0] = pd[0];
			dst[1] = pd[1];
			dst[2] = pd[2];

			if((info.func_blend)(src, dst, a) && a != 255)
			{
				//アルファ合成

				src[0] = ((src[0] - dst[0]) * a / 255) + dst[0];
				src[1] = ((src[1] - dst[1]) * a / 255) + dst[1];
				src[2] = ((src[2] - dst[2]) * a / 255) + dst[2];
			}

			//セット

			pd[0] = src[0];
			pd[1] = src[1];
			pd[2] = src[2];
		}

		ps += pitchs;
		ppdst++;

		info.tone_fx -= info.tone_fsin;
		info.tone_fy += info.tone_fcos;
	}
}

/** RGBA タイルとして取得 */

static void _8bit_gettile_rgba(TileImage *p,uint8_t *dst,const uint8_t *src)
{
	int i;

	for(i = 64 * 64; i; i--, dst += 4, src += 2)
	{
		dst[0] = dst[1] = dst[2] = src[0];
		dst[3] = src[1];
	}
}

/** RGBA タイルからセット */

static void _8bit_settile_rgba(uint8_t *dst,const uint8_t *src,mlkbool lum_to_a)
{
	int i;

	for(i = 64 * 64; i; i--, dst += 2, src += 4)
	{
		dst[0] = CONV_RGB_TO_LUM(src[0], src[1], src[2]);
		dst[1] = src[3];
	}
}

/** ファイル保存用タイルデータからタイルセット
 *
 * 各チャンネルごとに 64x64 分並んでいる */

static void _8bit_convert_from_save(uint8_t *dst,uint8_t *src)
{
	uint8_t *pd;
	int i,j;

	for(j = 0; j < 2; j++)
	{
		pd = dst + j;
	
		for(i = 64 * 64; i; i--, pd += 2)
			*pd = *(src++);
	}
}

/** ファイル保存用にタイルデータを変換 */

static void _8bit_convert_to_save(uint8_t *dst,uint8_t *src)
{
	int ch,i;
	uint8_t *ps;

	for(ch = 0; ch < 2; ch++)
	{
		ps = src + ch;

		for(i = 64 * 64; i; i--, ps += 2)
			*(dst++) = *ps;
	}
}

/** タイルを左右反転 */

static void _8bit_flip_horz(uint8_t *dst,uint8_t *src)
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

static void _8bit_flip_vert(uint8_t *dst,uint8_t *src)
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

static void _8bit_rotate_left(uint8_t *dst,uint8_t *src)
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

static void _8bit_rotate_right(uint8_t *dst,uint8_t *src)
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
// 16bit
//==========================


/** タイルと px 位置から色取得 (RGBA16) */

static void _16bit_getpixel_at_tile(TileImage *p,uint8_t *tile,int x,int y,void *dst)
{
	uint16_t *pd,*ps;

	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	pd = (uint16_t *)dst;
	ps = (uint16_t *)tile + ((y << 6) + x) * 2;

	pd[0] = pd[1] = pd[2] = ps[0];
	pd[3] = ps[1];
}

/** タイルと px 位置からバッファ位置取得 */

static uint8_t *_16bit_getpixelbuf_at_tile(TileImage *p,uint8_t *tile,int x,int y)
{
	x = (x - p->offx) & 63;
	y = (y - p->offy) & 63;

	return tile + (((y << 6) + x) << 2);
}

/** ピクセルバッファ位置に色をセット */

static void _16bit_setpixel(TileImage *p,uint8_t *buf,int x,int y,void *pix)
{
	uint16_t *ps = (uint16_t *)pix;

	*((uint16_t *)buf) = (ps[0] + ps[1] + ps[2]) / 3;
	*((uint16_t *)buf + 1) = ps[3];
}

/** RGBA 16bit イメージバッファからタイルイメージに変換
 *
 * ppbuf: Y 開始位置 (16byte 境界)
 * x: 64px 単位 */

static void _16bit_rgba_to_tile(uint8_t *tile,uint8_t **ppbuf,int x,int w,int h)
{
	uint16_t *pd,*ps;
	int ix,iy,xpos;

	xpos = x * 8;

	for(iy = h; iy; iy--, ppbuf++, tile += 64 * 4)
	{
		pd = (uint16_t *)tile;
		ps = (uint16_t *)(*ppbuf + xpos);
		
		for(ix = w; ix; ix--, pd += 2, ps += 4)
		{
			pd[0] = ps[0];
			pd[1] = ps[3];
		}
	}
}

/** すべて透明か */

static mlkbool _16bit_is_transparent_tile(uint8_t *tile)
{
	uint16_t *ps;
	int i;

	ps = (uint16_t *)tile + 1;

#if MLK_ENABLE_SSE2 && _TILEIMG_SIMD_ON

	__m128i v,vcmp;

	vcmp = _mm_setzero_si128();

	for(i = 64 * 64 / 8; i; i--, ps += 2 * 8)
	{
		//16bitx8個分のアルファ値をセット
		
		v = _mm_set_epi16(ps[0], ps[2], ps[4], ps[6],
			ps[8], ps[10], ps[12], ps[14]);

		v = _mm_cmpeq_epi32(v, vcmp);	//0なら全ビットON

		//8bit x 16個の最上位ビット取得
		if(_mm_movemask_epi8(v) != 0xffff)
			return FALSE;
	}

	return TRUE;

#else

	for(i = 64 * 64; i; i--, ps += 2)
	{
		if(*ps) return FALSE;
	}

	return TRUE;

#endif
}

/** 2つの色が同じ色か (RGBA) */

static mlkbool _16bit_is_same_rgba(void *col1,void *col2)
{
	uint16_t *ps,*pd;

	ps = (uint16_t *)col1;
	pd = (uint16_t *)col2;

	return (
		((ps[0] + ps[1] + ps[2]) / 3 == pd[0] && ps[3] == pd[3])
		|| (ps[3] == 0 && pd[3] == 0)
	);
}

/** ImageCanvas に合成 */

static void _16bit_blend_tile(TileImage *p,TileImageBlendInfo *infosrc)
{
	TileImageBlendInfo info;
	uint16_t **ppdst,*ps,*pd,r,g,b;
	int pitchs,ix,iy,dx,dy,a,c,dstx,cx,cy,thval;
	int32_t src[3],dst[3];
	int64_t fxx,fxy;

	info = *infosrc;

	ps = (uint16_t *)info.tile + (info.sy * 64 + info.sx) * 2;
	ppdst = (uint16_t **)info.dstbuf;
	dstx = info.dx * 4;
	pitchs = (64 - info.w) * 2;

	r = p->col.c16.r;
	g = p->col.c16.g;
	b = p->col.c16.b;

	//

	for(iy = info.h, dy = info.dy; iy; iy--, dy++)
	{
		pd = *ppdst + dstx;
		fxx = info.tone_fx;
		fxy = info.tone_fy;
		
		for(ix = info.w, dx = info.dx; ix; ix--, dx++, ps += 2, pd += 4,
				fxx += info.tone_fcos, fxy += info.tone_fsin)
		{
			a = ps[1];
			if(!a) continue;

			if(info.imgtex)
				a = a * ImageMaterial_getPixel_forTexture(info.imgtex, dx, dy) / 255;

			a = a * info.opacity >> 7;

			if(!a) continue;

			//トーン化

			if(!info.is_tone)
			{
				if(info.tone_repcol == -1)
					c = ps[0];
				else
					c = info.tone_repcol;

				src[0] = src[1] = src[2] = c;
			}
			else
			{
				//トーン表示
				
				c = (info.tone_density)? 0x8000 - info.tone_density: ps[0];

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

			//RGB 合成

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

		info.tone_fx -= info.tone_fsin;
		info.tone_fy += info.tone_fcos;
	}
}

/** RGBA タイルとして取得 */

static void _16bit_gettile_rgba(TileImage *p,uint8_t *dst,const uint8_t *src)
{
	uint16_t *pd;
	const uint16_t *ps;
	int i;

	pd = (uint16_t *)dst;
	ps = (const uint16_t *)src;

	for(i = 64 * 64; i; i--, pd += 4, ps += 2)
	{
		pd[0] = pd[1] = pd[2] = ps[0];
		pd[3] = ps[1];
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

	for(i = 64 * 64; i; i--, pd += 2, ps += 4)
	{
		pd[0] = CONV_RGB_TO_LUM(ps[0], ps[1], ps[2]);
		pd[1] = ps[3];
	}
}

/** ファイル保存用タイルデータからタイルセット
 *
 * 各チャンネルごとに 64x64 分 BE で並んでいる */

static void _16bit_convert_from_save(uint8_t *dst,uint8_t *src)
{
	uint16_t *pd;
	int i,j;

	for(j = 0; j < 2; j++)
	{
		pd = (uint16_t *)dst + j;
	
		for(i = 64 * 64; i; i--, pd += 2, src += 2)
			*pd = (src[0] << 8) | src[1];
	}
}

/** ファイル保存用にタイルデータを変換 (BE) */

static void _16bit_convert_to_save(uint8_t *dst,uint8_t *src)
{
	int ch,i;
	uint16_t *ps;

	for(ch = 0; ch < 2; ch++)
	{
		ps = (uint16_t *)src + ch;

		for(i = 64 * 64; i; i--, ps += 2, dst += 2)
		{
			dst[0] = *ps >> 8;
			dst[1] = *ps & 255;
		}
	}
}

/** タイルを左右反転 */

static void _16bit_flip_horz(uint8_t *dst,uint8_t *src)
{
	uint32_t *pd,*ps;
	int ix,iy;

	pd = (uint32_t *)dst;
	ps = (uint32_t *)src + 63;

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

	src += 63 * (64 * 4);

	for(i = 64; i; i--)
	{
		memcpy(dst, src, 64 * 4);

		dst += 64 * 4;
		src -= 64 * 4;
	}
}

/** タイルを左に90度回転 */

static void _16bit_rotate_left(uint8_t *dst,uint8_t *src)
{
	uint32_t *pd,*ps;
	int ix,iy;

	pd = (uint32_t *)dst;
	ps = (uint32_t *)src + 63;

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
	uint32_t *pd,*ps;
	int ix,iy;

	pd = (uint32_t *)dst;
	ps = (uint32_t *)src + 63 * 64;

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

void __TileImage_setColFunc_Gray(TileImageColFuncData *p,int bits)
{
	if(bits == 8)
	{
		p->rgba_to_tile = _8bit_rgba_to_tile;
		p->is_transparent_tile = _8bit_is_transparent_tile;
		p->is_same_rgba = _8bit_is_same_rgba;
		p->blend_tile = _8bit_blend_tile;
		p->getpixel_at_tile = _8bit_getpixel_at_tile;
		p->getpixelbuf_at_tile = _8bit_getpixelbuf_at_tile;
		p->setpixel = _8bit_setpixel;
		p->convert_from_save = _8bit_convert_from_save;
		p->convert_to_save = _8bit_convert_to_save;
		p->flip_horz = _8bit_flip_horz;
		p->flip_vert = _8bit_flip_vert;
		p->rotate_left = _8bit_rotate_left;
		p->rotate_right = _8bit_rotate_right;
		p->gettile_rgba = _8bit_gettile_rgba;
		p->settile_rgba = _8bit_settile_rgba;
	}
	else
	{
		p->rgba_to_tile = _16bit_rgba_to_tile;
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


