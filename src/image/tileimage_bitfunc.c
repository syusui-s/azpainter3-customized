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

/*****************************************
 * TileImage: ビットごとの関数
 *****************************************/

#include "mlk.h"

#include "colorvalue.h"

#include "tileimage.h"
#include "pv_tileimage.h"


//==============================
// 8bit
//==============================


/** 色をコピー */

static void _8bit_copy_color(void *dst,const void *src)
{
	*((uint32_t *)dst) = *((uint32_t *)src);
}

/** RGBA 透明色をセット */

static void _8bit_setcol_rgba_transparent(void *dst)
{
	*((uint32_t *)dst) = 0;
}

/** 色が透明か */

static mlkbool _8bit_is_transparent(const void *buf)
{
	return (*((uint8_t *)buf + 3) == 0);
}

/** 重みを付けて色を加算 */

static void _8bit_add_weight_color(RGBAdouble *dst,void *srcbuf,double weight)
{
	uint8_t *ps = (uint8_t *)srcbuf;

	if(ps[3])
	{
		weight = ps[3] / 255.0 * weight;
	
		dst->r += ps[0] * weight;
		dst->g += ps[1] * weight;
		dst->b += ps[2] * weight;
		dst->a += weight;
	}
}

/** 重みを加算した色から結果の色を取得
 *
 * return: FALSE で透明 */

static mlkbool _8bit_get_weight_color(void *dst,const RGBAdouble *col)
{
	uint8_t *pd = (uint8_t *)dst;
	int i,n;
	double da;

	//A

	n = (int)(col->a * 255 + 0.5);

	if(n < 0) n = 0;
	else if(n > 255) n = 255;

	if(n == 0)
	{
		*((uint32_t *)dst) = 0;
		return FALSE;
	}

	pd[3] = n;

	//RGB

	da = 1 / col->a;

	for(i = 0; i < 3; i++)
	{
		n = (int)(col->ar[i] * da + 0.5);

		if(n < 0) n = 0;
		else if(n > 255) n = 255;

		pd[i] = n;
	}

	return TRUE;
}


//==============================
// 16bit
//==============================


/** 色をコピー */

static void _16bit_copy_color(void *dst,const void *src)
{
	*((uint64_t *)dst) = *((uint64_t *)src);
}

/** RGBA 透明色をセット */

static void _16bit_setcol_rgba_transparent(void *dst)
{
	*((uint64_t *)dst) = 0;
}

/** 色が透明か */

static mlkbool _16bit_is_transparent(const void *buf)
{
	return (*((uint16_t *)buf + 3) == 0);
}

/** 重みを付けて色を加算 */

static void _16bit_add_weight_color(RGBAdouble *dst,void *srcbuf,double weight)
{
	uint16_t *ps = (uint16_t *)srcbuf;

	if(ps[3])
	{
		weight = (double)ps[3] / 0x8000 * weight;
	
		dst->r += ps[0] * weight;
		dst->g += ps[1] * weight;
		dst->b += ps[2] * weight;
		dst->a += weight;
	}
}

/** 重みを加算した色から結果の色を取得
 *
 * return: FALSE で透明 */

static mlkbool _16bit_get_weight_color(void *dst,const RGBAdouble *col)
{
	uint16_t *pd = (uint16_t *)dst;
	int i,n;
	double da;

	//A

	n = (int)(col->a * 0x8000 + 0.5);

	if(n < 0) n = 0;
	else if(n > 0x8000) n = 0x8000;

	if(n == 0)
	{
		*((uint64_t *)dst) = 0;
		return FALSE;
	}

	pd[3] = n;

	//RGB

	da = 1 / col->a;

	for(i = 0; i < 3; i++)
	{
		n = (int)(col->ar[i] * da + 0.5);

		if(n < 0) n = 0;
		else if(n > 0x8000) n = 0x8000;

		pd[i] = n;
	}

	return TRUE;
}


//==============================
//
//==============================


/** 関数をセット */

void __TileImage_setBitFunc(void)
{
	TileImageWorkData *p = TILEIMGWORK;

	if(p->bits == 8)
	{
		p->copy_color = _8bit_copy_color;
		p->setcol_rgba_transparent = _8bit_setcol_rgba_transparent;
		p->is_transparent = _8bit_is_transparent;
		p->add_weight_color = _8bit_add_weight_color;
		p->get_weight_color = _8bit_get_weight_color;
	}
	else
	{
		p->copy_color = _16bit_copy_color;
		p->setcol_rgba_transparent = _16bit_setcol_rgba_transparent;
		p->is_transparent = _16bit_is_transparent;
		p->add_weight_color = _16bit_add_weight_color;
		p->get_weight_color = _16bit_get_weight_color;
	}
}

