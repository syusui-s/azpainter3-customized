/*$
 Copyright (C) 2013-2020 Azel.

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
 * レイヤの色合成
 *****************************************/

#include "mDef.h"

#include "blendcol.h"
#include "ColorValue.h"


//-----------------

BlendColorFunc g_blendcolfuncs[19];

#define _MAXVAL  0x8000

//-----------------


/** 関数テーブルをセット */

void BlendColor_setFuncTable()
{
	BlendColorFunc *p = g_blendcolfuncs;

	p[0] = BlendColor_normal;
	p[1] = BlendColor_mul;
	p[2] = BlendColor_add;
	p[3] = BlendColor_sub;
	p[4] = BlendColor_screen;
	p[5] = BlendColor_overlay;
	p[6] = BlendColor_hardlight;
	p[7] = BlendColor_softlight;
	p[8] = BlendColor_dodge;
	p[9] = BlendColor_burn;
	p[10] = BlendColor_linearburn;
	p[11] = BlendColor_vividlight;
	p[12] = BlendColor_linearlight;
	p[13] = BlendColor_pinlight;
	p[14] = BlendColor_darken;
	p[15] = BlendColor_lighten;
	p[16] = BlendColor_difference;
	p[17] = BlendColor_luminous_add;
	p[18] = BlendColor_luminous_dodge;
}

/* a: ソースのアルファ値
 * 戻り値: FALSE で、この後アルファ合成を行う。
 *         TRUE で、ソースのアルファ値を使ったのでアルファ合成は行わない。 */

/** 通常 */

mBool BlendColor_normal(RGBFix15 *src,RGBFix15 *dst,int a)
{
	return FALSE;
}

/** 乗算 */

mBool BlendColor_mul(RGBFix15 *src,RGBFix15 *dst,int a)
{
	src->r = src->r * dst->r >> 15;
	src->g = src->g * dst->g >> 15;
	src->b = src->b * dst->b >> 15;

	return FALSE;
}

/** 加算 */

mBool BlendColor_add(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src->c[i] + dst->c[i];
		if(n > _MAXVAL) n = _MAXVAL;

		src->c[i] = n;
	}

	return FALSE;
}

/** 減算 */

mBool BlendColor_sub(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = dst->c[i] - src->c[i];
		if(n < 0) n = 0;

		src->c[i] = n;
	}

	return FALSE;
}

/** スクリーン */

mBool BlendColor_screen(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];
		
		src->c[i] = s + d - (s * d >> 15);
	}

	return FALSE;
}

/** オーバーレイ */

mBool BlendColor_overlay(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];
		
		if(d < 0x4000)
			src->c[i] = s * d >> 14;
		else
			src->c[i] = _MAXVAL - ((_MAXVAL - d) * (_MAXVAL - s) >> 14);
	}

	return FALSE;
}

/** ハードライト */

mBool BlendColor_hardlight(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];
		
		if(s < 0x4000)
			src->c[i] = s * d >> 14;
		else
			src->c[i] = _MAXVAL - ((_MAXVAL - d) * (_MAXVAL - s) >> 14);
	}

	return FALSE;
}

/** ソフトライト */

mBool BlendColor_softlight(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		n = s * d >> 15;

		src->c[i] = n + (d * (_MAXVAL - n - ((_MAXVAL - s) * (_MAXVAL - d) >> 15)) >> 15);
	}

	return FALSE;
}

/** 覆い焼き */

mBool BlendColor_dodge(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s == _MAXVAL)
			n = _MAXVAL;
		else
		{
			n = (d << 15) / (_MAXVAL - s);
			if(n > _MAXVAL) n = _MAXVAL;
		}

		src->c[i] = n;
	}

	return FALSE;
}

/** 焼き込み */

mBool BlendColor_burn(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s == 0)
			n = 0;
		else
		{
			n = _MAXVAL - ((_MAXVAL - d) << 15) / s;
			if(n < 0) n = 0;
		}

		src->c[i] = n;
	}

	return FALSE;
}

/** 焼き込みリニア */

mBool BlendColor_linearburn(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = dst->c[i] + src->c[i] - _MAXVAL;
		if(n < 0) n = 0;
		
		src->c[i] = n;
	}

	return FALSE;
}

/** ビビットライト */

mBool BlendColor_vividlight(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s < 0x4000)
		{
			n = _MAXVAL - (s << 1);

			if(d <= n || s == 0)
				n = 0;
			else
				n = ((d - n) << 15) / (s << 1);
		}
		else
		{
			n = 0x10000 - (s << 1);

			if(d >= n || n == 0)
				n = _MAXVAL;
			else
				n = (d << 15) / n;
		}

		src->c[i] = n;
	}

	return FALSE;
}

/** リニアライト */

mBool BlendColor_linearlight(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = (src->c[i] << 1) + dst->c[i] - _MAXVAL;

		if(n < 0) n = 0;
		else if(n > _MAXVAL) n = _MAXVAL;
		
		src->c[i] = n;
	}

	return FALSE;
}

/** ピンライト */

mBool BlendColor_pinlight(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i];
		d = dst->c[i];

		if(s > 0x4000)
		{
			n = (s << 1) - _MAXVAL;
			if(n < d) n = d;
		}
		else
		{
			n = s << 1;
			if(n > d) n = d;
		}

		src->c[i] = n;
	}

	return FALSE;
}

/** 比較(暗) */

mBool BlendColor_darken(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		if(dst->c[i] < src->c[i])
			src->c[i] = dst->c[i];
	}

	return FALSE;
}

/** 比較(明) */

mBool BlendColor_lighten(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		if(dst->c[i] > src->c[i])
			src->c[i] = dst->c[i];
	}

	return FALSE;
}

/** 差の絶対値 */

mBool BlendColor_difference(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src->c[i] - dst->c[i];
		if(n < 0) n = -n;

		src->c[i] = n;
	}

	return FALSE;
}

/** 発光(加算) */

mBool BlendColor_luminous_add(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = (src->c[i] * a >> 15) + dst->c[i];
		if(n > _MAXVAL) n = _MAXVAL;

		src->c[i] = n;
	}

	return TRUE;
}

/** 発光(覆い焼き) */

mBool BlendColor_luminous_dodge(RGBFix15 *src,RGBFix15 *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src->c[i] * a >> 15;
		d = dst->c[i];

		if(s == _MAXVAL)
			n = _MAXVAL;
		else
		{
			n = (d << 15) / (_MAXVAL - s);
			if(n > _MAXVAL) n = _MAXVAL;
		}

		src->c[i] = n;
	}

	return TRUE;
}
