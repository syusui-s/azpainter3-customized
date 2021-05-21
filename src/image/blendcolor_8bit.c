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
 * レイヤの色合成 (8bit)
 *****************************************/

#include "mlk.h"

#include "blendcolor.h"


#define _MAXVAL  255
#define _HALFVAL 128

/* dst の上に src を合成して、結果を src にセット。
 * 
 * 戻り値: TRUE でアルファ合成を行う */


/** 通常 */

static mlkbool _normal(int32_t *src,int32_t *dst,int a)
{
	return TRUE;
}

/** 乗算 */

static mlkbool _mul(int32_t *src,int32_t *dst,int a)
{
	src[0] = src[0] * dst[0] / 255;
	src[1] = src[1] * dst[1] / 255;
	src[2] = src[2] * dst[2] / 255;

	return TRUE;
}

/** 加算 */

static mlkbool _add(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src[i] + dst[i];
		if(n > _MAXVAL) n = _MAXVAL;

		src[i] = n;
	}

	return TRUE;
}

/** 減算 */

static mlkbool _sub(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = dst[i] - src[i];
		if(n < 0) n = 0;

		src[i] = n;
	}

	return TRUE;
}

/** スクリーン */

static mlkbool _screen(int32_t *src,int32_t *dst,int a)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src[i];
		d = dst[i];
		
		src[i] = s + d - (s * d / 255);
	}

	return TRUE;
}

/** オーバーレイ */

static mlkbool _overlay(int32_t *src,int32_t *dst,int a)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src[i];
		d = dst[i];
		
		if(d < _HALFVAL)
			src[i] = s * d >> 7;
		else
			src[i] = _MAXVAL - ((_MAXVAL - d) * (_MAXVAL - s) >> 7);
	}

	return TRUE;
}

/** ハードライト */

static mlkbool _hardlight(int32_t *src,int32_t *dst,int a)
{
	int i,s,d;

	for(i = 0; i < 3; i++)
	{
		s = src[i];
		d = dst[i];
		
		if(s < _HALFVAL)
			src[i] = s * d >> 7;
		else
			src[i] = _MAXVAL - ((_MAXVAL - d) * (_MAXVAL - s) >> 7);
	}

	return TRUE;
}

/** ソフトライト */

static mlkbool _softlight(int32_t *src,int32_t *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src[i];
		d = dst[i];

		n = s * d / 255;

		src[i] = n + (d * (_MAXVAL - n - (_MAXVAL - s) * (_MAXVAL - d) / 255) / 255);
	}

	return TRUE;
}

/** 覆い焼き */

static mlkbool _dodge(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		if(src[i] != _MAXVAL)
		{
			n = dst[i] * 255 / (_MAXVAL - src[i]);
			if(n > _MAXVAL) n = _MAXVAL;

			src[i] = n;
		}
	}

	return TRUE;
}

/** 焼き込み */

static mlkbool _burn(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		if(src[i])
		{
			n = _MAXVAL - (_MAXVAL - dst[i]) * 255 / src[i];
			if(n < 0) n = 0;

			src[i] = n;
		}
	}

	return TRUE;
}

/** 焼き込みリニア */

static mlkbool _linearburn(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = dst[i] + src[i] - _MAXVAL;
		if(n < 0) n = 0;
		
		src[i] = n;
	}

	return TRUE;
}

/** ビビットライト */

static mlkbool _vividlight(int32_t *src,int32_t *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src[i];
		d = dst[i];

		if(s < _HALFVAL)
		{
			n = _MAXVAL - (s << 1);

			if(d <= n || s == 0)
				n = 0;
			else
				n = (d - n) * 255 / (s << 1);
		}
		else
		{
			n = _MAXVAL * 2 - (s << 1);

			if(d >= n || n == 0)
				n = _MAXVAL;
			else
				n = d * 255 / n;
		}

		src[i] = n;
	}

	return TRUE;
}

/** リニアライト */

static mlkbool _linearlight(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = (src[i] << 1) + dst[i] - _MAXVAL;

		if(n < 0) n = 0;
		else if(n > _MAXVAL) n = _MAXVAL;
		
		src[i] = n;
	}

	return TRUE;
}

/** ピンライト */

static mlkbool _pinlight(int32_t *src,int32_t *dst,int a)
{
	int i,s,d,n;

	for(i = 0; i < 3; i++)
	{
		s = src[i];
		d = dst[i];

		if(s > _HALFVAL)
		{
			n = (s << 1) - _MAXVAL;
			if(n < d) n = d;
		}
		else
		{
			n = s << 1;
			if(n > d) n = d;
		}

		src[i] = n;
	}

	return TRUE;
}

/** 比較(暗) */

static mlkbool _darken(int32_t *src,int32_t *dst,int a)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		if(dst[i] < src[i])
			src[i] = dst[i];
	}

	return TRUE;
}

/** 比較(明) */

static mlkbool _lighten(int32_t *src,int32_t *dst,int a)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		if(dst[i] > src[i])
			src[i] = dst[i];
	}

	return TRUE;
}

/** 差の絶対値 */

static mlkbool _difference(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src[i] - dst[i];
		if(n < 0) n = -n;

		src[i] = n;
	}

	return TRUE;
}

/** 発光(加算) */

static mlkbool _luminous_add(int32_t *src,int32_t *dst,int a)
{
	int i,n;

	for(i = 0; i < 3; i++)
	{
		n = src[i] * a / 255 + dst[i];
		if(n > _MAXVAL) n = _MAXVAL;

		src[i] = n;
	}

	return FALSE;
}

/** 発光(覆い焼き) */

static mlkbool _luminous_dodge(int32_t *src,int32_t *dst,int a)
{
	int i,s,n;

	for(i = 0; i < 3; i++)
	{
		s = src[i] * a / 255;

		if(s == _MAXVAL)
			n = _MAXVAL;
		else
		{
			n = dst[i] * 255 / (_MAXVAL - s);
			if(n > _MAXVAL) n = _MAXVAL;
		}

		src[i] = n;
	}

	return FALSE;
}

/** 関数テーブルをセット */

void BlendColorFunc_setTable_8bit(BlendColorFunc *p)
{
	p[0] = _normal;
	p[1] = _mul;
	p[2] = _add;
	p[3] = _sub;
	p[4] = _screen;
	p[5] = _overlay;
	p[6] = _hardlight;
	p[7] = _softlight;
	p[8] = _dodge;
	p[9] = _burn;
	p[10] = _linearburn;
	p[11] = _vividlight;
	p[12] = _linearlight;
	p[13] = _pinlight;
	p[14] = _darken;
	p[15] = _lighten;
	p[16] = _difference;
	p[17] = _luminous_add;
	p[18] = _luminous_dodge;
}

