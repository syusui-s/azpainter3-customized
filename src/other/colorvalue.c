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
 * カラー関連
 *****************************************/

#include <stdlib.h>

#include "mlk_gui.h"
#include "mlk_pixbuf.h"
#include "mlk_color.h"

#include "colorvalue.h"


/** 8bit -> 16bit カラー変換 */

int Color8bit_to_16bit(int n)
{
	return (int)(n / 255.0 * 0x8000 + 0.5);
}

/** 16bit -> 8bit カラー変更 */

int Color16bit_to_8bit(int n)
{
	return (int)((double)n / 0x8000 * 255 + 0.5);
}

/** 0-100 の値を、カラービットの数値に変換 */

int Density100_to_bitval(int n,int bits)
{
	if(bits == 8)
		return (int)(n / 100.0 * 255 + 0.5);
	else
		return (int)(n / 100.0 * 0x8000 + 0.5);
}


//------- カラービット数の値

/** RGBcombo から指定ビット数のカラー値取得
 *
 * 先頭から順に R,G,B,A と並ぶ。アルファ値は最大。 */

void RGBcombo_to_bitcol(void *dst,const RGBcombo *src,int bits)
{
	if(bits == 8)
	{
		uint8_t *p8 = (uint8_t *)dst;

		p8[0] = src->c8.r;
		p8[1] = src->c8.g;
		p8[2] = src->c8.b;
		p8[3] = 255;
	}
	else
	{
		uint16_t *p16 = (uint16_t *)dst;

		p16[0] = src->c16.r;
		p16[1] = src->c16.g;
		p16[2] = src->c16.b;
		p16[3] = 0x8000;
	}
}

/** カラービット値を RGBAcombo にセット (いずれかに) */

void bitcol_to_RGBAcombo(RGBAcombo *dst,void *src,int bits)
{
	if(bits == 8)
		dst->c8.v32 = *((uint32_t *)src);
	else
		dst->c16.v64 = *((uint64_t *)src);
}

/** 0-100 の濃度をアルファ値としてセット */

void bitcol_set_alpha_density100(void *dst,int val,int bits)
{
	if(bits == 8)
	{
		*((uint8_t *)dst + 3) = (int)(val / 100.0 * 255 + 0.5);
	}
	else
	{
		*((uint16_t *)dst + 3) = (int)(val / 100.0 * 0x8000 + 0.5);
	}
}

//---------- 32bit/64bit 色値取得


/** RGBcombo から 32bit RGB 値を取得 */

uint32_t RGBcombo_to_32bit(const RGBcombo *p)
{
	return MLK_RGB(p->c8.r, p->c8.g, p->c8.b);
}

/** RGBcombo から mPixCol 取得 */

uint32_t RGBcombo_to_pixcol(const RGBcombo *p)
{
	return mRGBtoPix_sep(p->c8.r, p->c8.g, p->c8.b);
}

/** RGBA8 から RGBA 32bit 値取得 */

uint32_t RGBA8_to_32bit(const RGBA8 *p)
{
	return MLK_ARGB(p->r, p->g, p->b, p->a);
}

/** RGBA8 から mPixCol 取得 */

uint32_t RGBA8_to_pixcol(const RGBA8 *p)
{
	return mRGBtoPix_sep(p->r, p->g, p->b);
}

/** RGBA16 から 32bit RGBA 値を取得 */

uint32_t RGBA16_to_32bit(const RGBA16 *p)
{
	int32_t r,g,b;

	r = Color16bit_to_8bit(p->r);
	g = Color16bit_to_8bit(p->g);
	b = Color16bit_to_8bit(p->b);

	return (r << 16) | (g << 8) | b;
}

/** RGB16 から 64bit RGB 値を取得 (バッファに置くための値) */

uint64_t RGB16_to_64bit_buf(const RGB16 *p)
{
	uint16_t buf[4] = {p->r, p->g, p->b, 0};

	return *((uint64_t *)buf);
}

//-------- 32bit RGB から変換


/** 32bit RGB 値から RGB8 へセット */

void RGB32bit_to_RGB8(RGB8 *dst,uint32_t col)
{
	dst->r = MLK_RGB_R(col);
	dst->g = MLK_RGB_G(col);
	dst->b = MLK_RGB_B(col);
}

/** 32bit RGB 値から RGB16 へセット */

void RGB32bit_to_RGB16(RGB16 *dst,uint32_t col)
{
	dst->r = Color8bit_to_16bit(MLK_RGB_R(col));
	dst->g = Color8bit_to_16bit(MLK_RGB_G(col));
	dst->b = Color8bit_to_16bit(MLK_RGB_B(col));
}

/** 32bit RGB 値から RGBcombo へセット (両方の値) */

void RGB32bit_to_RGBcombo(RGBcombo *dst,uint32_t col)
{
	dst->c8.r = MLK_RGB_R(col);
	dst->c8.g = MLK_RGB_G(col);
	dst->c8.b = MLK_RGB_B(col);

	RGBcombo_set16_from8(dst);
}

/** 32bit RGBA 値から RGBA8 へセット */

void RGBA32bit_to_RGBA8(RGBA8 *dst,uint32_t col)
{
	dst->r = MLK_RGB_R(col);
	dst->g = MLK_RGB_G(col);
	dst->b = MLK_RGB_B(col);
	dst->a = MLK_ARGB_A(col);
}


//----------- セット・変換


/** RGBcombo の RGB8 から RGB16 値をセット */

void RGBcombo_set16_from8(RGBcombo *p)
{
	RGB8_to_RGB16(&p->c16, &p->c8);
}

/** RGBAcombo -> RGBcombo */

void RGBAcombo_to_RGBcombo(RGBcombo *dst,const RGBAcombo *src)
{
	dst->c8.r = src->c8.r;
	dst->c8.g = src->c8.g;
	dst->c8.b = src->c8.b;

	dst->c16.r = src->c16.r;
	dst->c16.g = src->c16.g;
	dst->c16.b = src->c16.b;
}

/** RGB8 -> RGBA8 */

void RGB8_to_RGBA8(RGBA8 *dst,const RGB8 *src,int a)
{
	dst->r = src->r;
	dst->g = src->g;
	dst->b = src->b;
	dst->a = a;
}

/** RGB8 -> RGB16 */

void RGB8_to_RGB16(RGB16 *dst,const RGB8 *src)
{
	dst->r = Color8bit_to_16bit(src->r);
	dst->g = Color8bit_to_16bit(src->g);
	dst->b = Color8bit_to_16bit(src->b);
}

/** RGB16 -> RGB8 */

void RGB16_to_RGB8(RGB8 *dst,const RGB16 *src)
{
	dst->r = Color16bit_to_8bit(src->r);
	dst->g = Color16bit_to_8bit(src->g);
	dst->b = Color16bit_to_8bit(src->b);
}

/** RGB16 -> RGBA16 */

void RGB16_to_RGBA16(RGBA16 *dst,const RGB16 *src,int a)
{
	dst->r = src->r;
	dst->g = src->g;
	dst->b = src->b;
	dst->a = a;
}

/** RGBA8 -> RGBA16 */

void RGBA8_to_RGBA16(RGBA16 *dst,const RGBA8 *src)
{
	dst->r = Color8bit_to_16bit(src->r);
	dst->g = Color8bit_to_16bit(src->g);
	dst->b = Color8bit_to_16bit(src->b);
	dst->a = Color8bit_to_16bit(src->a);
}

/** RGBA16 -> RGBA8 */

void RGBA16_to_RGBA8(RGBA8 *dst,const RGBA16 *src)
{
	dst->r = Color16bit_to_8bit(src->r);
	dst->g = Color16bit_to_8bit(src->g);
	dst->b = Color16bit_to_8bit(src->b);
	dst->a = Color16bit_to_8bit(src->a);
}


//------------ ほか


/** RGB8 を 32bit RGB と比較 */

mlkbool RGB8_compare_32bit(const RGB8 *p,uint32_t col)
{
	return (MLK_RGB(p->r, p->g, p->b) == (col & 0xffffff));
}

/** 中間色を作成 */

void RGBcombo_createMiddle(RGBcombo *dst,const RGBcombo *src)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		dst->c8.ar[i] = (src->c8.ar[i] - dst->c8.ar[i]) / 2 + dst->c8.ar[i];
		dst->c16.ar[i] = (src->c16.ar[i] - dst->c16.ar[i]) / 2 + dst->c16.ar[i];
	}
}


//------------ 色変換


/** RGB8 -> HSV */

void RGB8_to_HSVd(mHSVd *dst,const RGB8 *src)
{
	mRGB8_to_HSV(dst, src->r, src->g, src->b);
}

/** HSV -> RGB8 */

void HSVd_to_RGB8(RGB8 *dst,const mHSVd *src)
{
	mRGB8 rgb;

	mHSV_to_RGB8(&rgb, src->h, src->s, src->v);

	dst->r = rgb.r;
	dst->g = rgb.g;
	dst->b = rgb.b;
}

//------------

/** RGB 文字列から RGB8 にセット
 *
 * 先頭が # なら "#RRGGBB (6桁のみ)"、それ以外で "R,G,B" */

void ColorText_to_RGB8(RGB8 *dst,const char *text)
{
	int i,n;

	dst->r = dst->g = dst->b = 0;

	if(!text) return;

	if(text[0] == '#')
	{
		//16進数
		
		RGB32bit_to_RGB8(dst, strtol(text + 1, NULL, 16));
	}
	else
	{
		//R,G,B

		for(i = 0; i < 3; i++)
		{
			n = atoi(text);
			if(n > 255) n = 255;
			
			dst->ar[i] = n;

			//数字以外が来るまで
			for(; *text && *text >= '0' && *text <= '9'; text++);

			//数字が来るまで
			for(; *text && !(*text >= '0' && *text <= '9'); text++);

			if(!(*text)) break;
		}
	}
}

