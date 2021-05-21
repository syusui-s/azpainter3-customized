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
 * TileImage: ピクセルカラー処理関数
 *****************************************/

#include "mlk.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "pv_tileimage.h"


#define _16BIT_MAX  0x8000


//============================
// 8bit
//============================


/** 通常合成 */

static void _8bit_normal(TileImage *p,void *dst,void *src,void *param)
{
	RGBA8 *ps,*pd;
	double sa,da,na;
	int a;

	ps = (RGBA8 *)src;
	pd = (RGBA8 *)dst;

	if(ps->a == 255)
	{
		*pd = *ps;
		return;
	}

	sa = (double)ps->a / 255;
	da = (double)pd->a / 255;

	//A

	na = sa + da - sa * da;
	a = (int)(na * 255 + 0.5);

	//RGB

	if(a == 0)
		pd->v32 = 0;
	else
	{
		da = da * (1.0 - sa);
		na = 1.0 / na;

		pd->r = (uint8_t)((ps->r * sa + pd->r * da) * na + 0.5);
		pd->g = (uint8_t)((ps->g * sa + pd->g * da) * na + 0.5);
		pd->b = (uint8_t)((ps->b * sa + pd->b * da) * na + 0.5);
		pd->a = a;
	}
}

/** アルファ値を比較して大きければ上書き */

static void _8bit_compare_a(TileImage *p,void *dst,void *src,void *param)
{
	RGBA8 *ps,*pd;

	ps = (RGBA8 *)src;
	pd = (RGBA8 *)dst;

	if(ps->a > pd->a)
		//上書き
		*pd = *ps;
	else if(pd->a)
	{
		//アルファ値はそのままで色だけ変える
	
		pd->r = ps->r;
		pd->g = ps->g;
		pd->b = ps->b;
	}
}

/** 完全上書き */

static void _8bit_overwrite(TileImage *p,void *dst,void *src,void *param)
{
	*((uint32_t *)dst) = *((uint32_t *)src);
}

/** 消しゴム */

static void _8bit_erase(TileImage *p,void *dst,void *src,void *param)
{
	RGBA8 *pd = (RGBA8 *)dst;
	int a;

	a = pd->a - *((uint8_t *)src + 3);
	if(a < 0) a = 0;

	if(a == 0)
		pd->v32 = 0;
	else
		pd->a = a;
}

/** 指先 */

static void _8bit_finger(TileImage *p,void *dst,void *src,void *parambuf)
{
	TileImageFingerPixelParam *param;
	RGBA8 *buf,*ps,*pd;
	double sa,da,na,strength;
	int a,x,y;

	pd = (RGBA8 *)dst;
	ps = (RGBA8 *)src;
	param = (TileImageFingerPixelParam *)parambuf;

	//バッファ内の前回の色

	buf = (RGBA8 *)(TILEIMGWORK->finger_buf + param->bufpos * 4);

	//A

	strength = (double)ps->a / 255;
	sa = (double)buf->a / 255;
	da = (double)pd->a / 255;

	na = da + (sa - da) * strength;
	a = (int)(na * 255 + 0.5);

	//RGB

	if(a == 0)
		pd->v32 = 0;
	else
	{
		sa *= strength;
		da *= 1.0 - strength;
		na = 1.0 / na;

		pd->r = (uint8_t)((buf->r * sa + pd->r * da) * na + 0.5);
		pd->g = (uint8_t)((buf->g * sa + pd->g * da) * na + 0.5);
		pd->b = (uint8_t)((buf->b * sa + pd->b * da) * na + 0.5);
		pd->a = a;
	}

	//現在の色をバッファにセット
	// :描画位置がイメージ範囲内ならそのままセット。
	// :範囲外ならクリッピングした位置の色。

	x = param->x;
	y = param->y;

	if(x >= 0 && x < TILEIMGWORK->imgw
		&& y >= 0 && y < TILEIMGWORK->imgh)
		*buf = *pd;
	else
		TileImage_getPixel_clip(p, x, y, buf);
}

/** 覆い焼き (色は白固定) */

static void _8bit_dodge(TileImage *p,void *dst,void *src,void *param)
{
	RGBA8 *pd = (RGBA8 *)dst;
	int a,i,c;
	double d;

	a = *((uint8_t *)src + 3);

	if(a == 255)
		pd->r = pd->g = pd->b = 255;
	else
	{
		d = 255.0 / (255 - a);
	
		for(i = 0; i < 3; i++)
		{
			c = (int)(pd->ar[i] * d + 0.5);
			if(c > 255) c = 255;

			pd->ar[i] = c;
		}
	}
}

/** 焼き込み (色は白固定) */

static void _8bit_burn(TileImage *p,void *dst,void *src,void *param)
{
	RGBA8 *pd = (RGBA8 *)dst;
	int a,i,c;
	double d;

	a = *((uint8_t *)src + 3);

	if(a == 255)
		pd->r = pd->g = pd->b = 0;
	else
	{
		d = 255.0 / (255 - a);
	
		for(i = 0; i < 3; i++)
		{
			c = (int)(255 - (255 - pd->ar[i]) * d + 0.5);

			if(c < 0) c = 0;
			else if(c > 255) c = 255;

			pd->ar[i] = c;
		}
	}
}

/** 加算 */

static void _8bit_add(TileImage *p,void *dst,void *src,void *param)
{
	RGBA8 *pd,*ps;
	int a,i,c;

	pd = (RGBA8 *)dst;
	ps = (RGBA8 *)src;

	a = ps->a;

	for(i = 0; i < 3; i++)
	{
		c = pd->ar[i] + (ps->ar[i] * a / 255);

		if(c > 255) c = 255;

		pd->ar[i] = c;
	}
}

/** アルファ値を反転
 *
 * src の A が 0 以外で反転、0 なら透明に */

static void _8bit_inverse_alpha(TileImage *p,void *dst,void *src,void *parambuf)
{
	if(*((uint8_t *)src + 3))
		*((uint8_t *)dst + 3) = 255 - *((uint8_t *)dst + 3);
	else
		*((uint32_t *)dst) = 0;
}


//============================
// 16bit
//============================


/** 通常合成 */

static void _16bit_normal(TileImage *p,void *dst,void *src,void *param)
{
	RGBA16 *ps,*pd;
	double sa,da,na;
	int a;

	ps = (RGBA16 *)src;
	pd = (RGBA16 *)dst;

	if(ps->a == 0x8000)
	{
		*pd = *ps;
		return;
	}

	sa = (double)ps->a / 0x8000;
	da = (double)pd->a / 0x8000;

	//A

	na = sa + da - sa * da;
	a = (int)(na * 0x8000 + 0.5);

	//RGB

	if(a == 0)
		pd->v64 = 0;
	else
	{
		da = da * (1.0 - sa);
		na = 1.0 / na;

		pd->r = (uint16_t)((ps->r * sa + pd->r * da) * na + 0.5);
		pd->g = (uint16_t)((ps->g * sa + pd->g * da) * na + 0.5);
		pd->b = (uint16_t)((ps->b * sa + pd->b * da) * na + 0.5);
		pd->a = a;
	}
}

/** アルファ値を比較して大きければ上書き */

static void _16bit_compare_a(TileImage *p,void *dst,void *src,void *param)
{
	RGBA16 *ps,*pd;

	ps = (RGBA16 *)src;
	pd = (RGBA16 *)dst;

	if(ps->a > pd->a)
		//上書き
		*pd = *ps;
	else if(pd->a)
	{
		//アルファ値はそのままで色だけ変える
	
		pd->r = ps->r;
		pd->g = ps->g;
		pd->b = ps->b;
	}
}

/** 完全上書き */

static void _16bit_overwrite(TileImage *p,void *dst,void *src,void *param)
{
	*((uint64_t *)dst) = *((uint64_t *)src);
}

/** 消しゴム */

static void _16bit_erase(TileImage *p,void *dst,void *src,void *param)
{
	RGBA16 *pd = (RGBA16 *)dst;
	int a;

	a = pd->a - *((uint16_t *)src + 3);
	if(a < 0) a = 0;

	if(a == 0)
		pd->v64 = 0;
	else
		pd->a = a;
}

/** 指先 */

static void _16bit_finger(TileImage *p,void *dst,void *src,void *parambuf)
{
	TileImageFingerPixelParam *param;
	RGBA16 *buf,*ps,*pd;
	double sa,da,na,strength;
	int a,x,y;

	pd = (RGBA16 *)dst;
	ps = (RGBA16 *)src;
	param = (TileImageFingerPixelParam *)parambuf;

	//バッファ内の前回の色

	buf = (RGBA16 *)(TILEIMGWORK->finger_buf + param->bufpos * 8);

	//A

	strength = (double)ps->a / 0x8000;
	sa = (double)buf->a / 0x8000;
	da = (double)pd->a / 0x8000;

	na = da + (sa - da) * strength;
	a = (int)(na * 0x8000 + 0.5);

	//RGB

	if(a == 0)
		pd->v64 = 0;
	else
	{
		sa *= strength;
		da *= 1.0 - strength;
		na = 1.0 / na;

		pd->r = (int)((buf->r * sa + pd->r * da) * na + 0.5);
		pd->g = (int)((buf->g * sa + pd->g * da) * na + 0.5);
		pd->b = (int)((buf->b * sa + pd->b * da) * na + 0.5);
		pd->a = a;
	}

	//現在の色をバッファにセット
	// :描画位置がイメージ範囲内ならそのままセット。
	// :範囲外ならクリッピングした位置の色。

	x = param->x;
	y = param->y;

	if(x >= 0 && x < TILEIMGWORK->imgw
		&& y >= 0 && y < TILEIMGWORK->imgh)
		*buf = *pd;
	else
		TileImage_getPixel_clip(p, x, y, buf);
}

/** 覆い焼き */

static void _16bit_dodge(TileImage *p,void *dst,void *src,void *param)
{
	RGBA16 *pd = (RGBA16 *)dst;
	int a,i,c;
	double d;

	a = *((uint16_t *)src + 3);

	if(a == _16BIT_MAX)
		pd->r = pd->g = pd->b = _16BIT_MAX;
	else
	{
		d = (double)_16BIT_MAX / (_16BIT_MAX - a);
	
		for(i = 0; i < 3; i++)
		{
			c = (int)(pd->ar[i] * d + 0.5);
			if(c > _16BIT_MAX) c = _16BIT_MAX;

			pd->ar[i] = c;
		}
	}
}

/** 焼き込み */

static void _16bit_burn(TileImage *p,void *dst,void *src,void *param)
{
	RGBA16 *pd = (RGBA16 *)dst;
	int a,i,c;
	double d;

	a = *((uint16_t *)src + 3);

	if(a == _16BIT_MAX)
		pd->r = pd->g = pd->b = 0;
	else
	{
		d = (double)_16BIT_MAX / (_16BIT_MAX - a);
	
		for(i = 0; i < 3; i++)
		{
			c = (int)(_16BIT_MAX - (_16BIT_MAX - pd->ar[i]) * d + 0.5);

			if(c < 0) c = 0;
			else if(c > _16BIT_MAX) c = _16BIT_MAX;

			pd->ar[i] = c;
		}
	}
}

/** 加算 */

static void _16bit_add(TileImage *p,void *dst,void *src,void *param)
{
	RGBA16 *pd,*ps;
	int a,i,c;

	pd = (RGBA16 *)dst;
	ps = (RGBA16 *)src;

	a = ps->a;

	for(i = 0; i < 3; i++)
	{
		c = pd->ar[i] + (ps->ar[i] * a >> 15);

		if(c > _16BIT_MAX) c = _16BIT_MAX;

		pd->ar[i] = c;
	}
}

/** アルファ値を反転
 *
 * src の A が 0 以外で反転、0 なら透明に */

static void _16bit_inverse_alpha(TileImage *p,void *dst,void *src,void *parambuf)
{
	if(*((uint16_t *)src + 3))
		*((uint16_t *)dst + 3) = 0x8000 - *((uint16_t *)dst + 3);
	else
		*((uint64_t *)dst) = 0;
}


//============================
//
//============================


/** 関数をセット */

void __TileImage_setPixelColorFunc(TileImagePixelColorFunc *p,int bits)
{
	if(bits == 8)
	{
		p[TILEIMAGE_PIXELCOL_NORMAL] = _8bit_normal;
		p[TILEIMAGE_PIXELCOL_COMPARE_A] = _8bit_compare_a;
		p[TILEIMAGE_PIXELCOL_OVERWRITE] = _8bit_overwrite;
		p[TILEIMAGE_PIXELCOL_ERASE] = _8bit_erase;
		p[TILEIMAGE_PIXELCOL_FINGER] = _8bit_finger;
		p[TILEIMAGE_PIXELCOL_DODGE] = _8bit_dodge;
		p[TILEIMAGE_PIXELCOL_BURN] = _8bit_burn;
		p[TILEIMAGE_PIXELCOL_ADD] = _8bit_add;
		p[TILEIMAGE_PIXELCOL_INVERSE_ALPHA] = _8bit_inverse_alpha;
	}
	else
	{
		p[TILEIMAGE_PIXELCOL_NORMAL] = _16bit_normal;
		p[TILEIMAGE_PIXELCOL_COMPARE_A] = _16bit_compare_a;
		p[TILEIMAGE_PIXELCOL_OVERWRITE] = _16bit_overwrite;
		p[TILEIMAGE_PIXELCOL_ERASE] = _16bit_erase;
		p[TILEIMAGE_PIXELCOL_FINGER] = _16bit_finger;
		p[TILEIMAGE_PIXELCOL_DODGE] = _16bit_dodge;
		p[TILEIMAGE_PIXELCOL_BURN] = _16bit_burn;
		p[TILEIMAGE_PIXELCOL_ADD] = _16bit_add;
		p[TILEIMAGE_PIXELCOL_INVERSE_ALPHA] = _16bit_inverse_alpha;
	}
}


