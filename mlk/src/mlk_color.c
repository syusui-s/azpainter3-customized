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
 * 色関連
 *****************************************/

#include <stdint.h>

#include "mlk_color.h"


/* RGB から色相計算
 *
 * r,g,b: 0.0-1.0
 * pmin : r,g,b の中の最小値
 * pmax : r,g,b の中の最大値
 * return: H (0.0-6.0 未満) */

static double _rgb_to_hue(double r,double g,double b,double *pmin,double *pmax)
{
	double min,max,h;

	min = (r <= g)? r: g;
	if(b < min) min = b;

	max = (r <= g)? g: r;
	if(max < b) max = b;

	h = max - min;

	if(h != 0)
	{
		if(max == r)
			h = (g - b) / h;
		else if(max == g)
			h = (b - r) / h + 2;
		else
			h = (r - g) / h + 4;

		if(h < 0)
			h += 6;
		else if(h >= 6.0)
			h -= 6.0;
	}

	*pmin = min;
	*pmax = max;

	return h;
}



//===============================
// HSV -> RGB
//===============================


/**@ HSV (double) -> RGB (double)
 *
 * @p:dst r,g,b 値は 0.0〜1.0
 * @p:h,s,v  h は 0.0〜6.0 (6.0=360度)。s,v は 0.0〜1.0。 */

void mHSV_to_RGBd(mRGBd *dst,double h,double s,double v)
{
	double c1,c2,c3,r,g,b,f;
	int hi;

	if(s == 0)
		r = g = b = v;
	else
	{
		hi = (int)h;
		f = h - hi;
		hi %= 6;	//0-5
	
		c1 = v * (1 - s);
		c2 = v * (1 - s * f);
		c3 = v * (1 - s * (1 - f));

		switch(hi)
		{
			case 0: r = v;  g = c3; b = c1; break;
			case 1: r = c2; g = v;  b = c1; break;
			case 2: r = c1; g = v;  b = c3; break;
			case 3: r = c1; g = c2; b = v;  break;
			case 4: r = c3; g = c1; b = v;  break;
			default: r = v;  g = c1; b = c2; break;
		}
	}

	dst->r = r;
	dst->g = g;
	dst->b = b;
}

/**@ HSV (double) -> RGB (8bit)
 *
 * @p:dst r,g,b は 0〜255 */

void mHSV_to_RGB8(mRGB8 *dst,double h,double s,double v)
{
	mRGBd d;

	mHSV_to_RGBd(&d, h, s, v);

	dst->r = (int)(d.r * 255 + 0.5);
	dst->g = (int)(d.g * 255 + 0.5);
	dst->b = (int)(d.b * 255 + 0.5);
}

/**@ HSV (double) -> RGB (8bit:pac)
 *
 * @r:RGB値を XRGB の数値にして返す */

uint32_t mHSV_to_RGB8pac(double h,double s,double v)
{
	mRGB8 d;

	mHSV_to_RGB8(&d, h, s, v);

	return (d.r << 16) | (d.g << 8) | d.b;
}

/**@ HSV (int) -> RGB (8bit:pac)
 *
 * @d:整数高速版
 * @p:h 0〜360
 * @p:s,v 0〜255 */

uint32_t mHSVi_to_RGB8pac(int h,int s,int v)
{
	int c1,c2,c3,r,g,b;
	int t;

	if(s == 0)
		r = g = b = v;
	else
	{
		t  = (h * 6) % 360;
		c1 = v * (255 - s) / 255;
		c2 = v * (255 - s * t / 360) / 255;
		c3 = v * (255 - s * (360 - t) / 360) / 255;

		switch(h / 60)
		{
			case 0: r = v;  g = c3; b = c1; break;
			case 1: r = c2; g = v;  b = c1; break;
			case 2: r = c1; g = v;  b = c3; break;
			case 3: r = c1; g = c2; b = v;  break;
			case 4: r = c3; g = c1; b = v;  break;
			default: r = v;  g = c1; b = c2; break;
		}
	}

	return (r << 16) | (g << 8) | b;
}


//===============================
// RGB -> HSV
//===============================


/**@ RGB (double) -> HSV (double)
 *
 * @p:dst h は 0.0〜6.0 未満。s,v は 0.0〜1.0。
 * @p:r,g,b 0.0〜1.0 */

void mRGBd_to_HSV(mHSVd *dst,double r,double g,double b)
{
	double h,max,min;

	h = _rgb_to_hue(r, g, b, &min, &max);

	dst->h = h;
	dst->s = (max == 0)? 0: (max - min) / max;
	dst->v = max;
}

/**@ RGB (8bit) -> HSV (double)
 *
 * @p:r,g,b 0〜255 */

void mRGB8_to_HSV(mHSVd *dst,int r,int g,int b)
{
	double h,max,min;

	h = _rgb_to_hue(r / 255.0, g / 255.0, b / 255.0, &min, &max);

	dst->h = h;
	dst->s = (max == 0)? 0: (max - min) / max;
	dst->v = max;
}

/**@ RGB (8bit:pac) -> HSV (double) */

void mRGB8pac_to_HSV(mHSVd *dst,uint32_t rgb)
{
	mRGB8_to_HSV(dst, (rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}


//===============================
// HSL <-> RGB
//===============================


/* HSL -> RGB (各値から RGB 値いずれか取得) */

static double _hsltorgb_get_rgb(double h,double min,double max)
{
	int hi;

	if(h >= 6.0) h -= 6.0;
	else if(h < 0) h += 6.0;

	hi = (int)h;

	if(hi < 1)
		return min + (max - min) * h;
	else if(hi < 3)
		return max;
	else if(hi < 4)
		return min + (max - min) * (4 - h);
	else
		return min;
}


/**@ HSL (double) -> RGB (double)
 *
 * @p:h,s,l  h は 0.0〜6.0。s,l は 0.0〜1.0。 */

void mHSL_to_RGBd(mRGBd *dst,double h,double s,double l)
{
	double max,min;

	if(s == 0)
		dst->r = dst->g = dst->b = l;
	else
	{
		if(l < 0.5)
			max = l * (1 + s);
		else
			max = l * (1 - s) + s;

		min = 2 * l - max;

		dst->r = _hsltorgb_get_rgb(h + 2, min, max);
		dst->g = _hsltorgb_get_rgb(h, min, max);
		dst->b = _hsltorgb_get_rgb(h - 2, min, max);
	}
}

/**@ HSL (double) -> RGB (8bit) */

void mHSL_to_RGB8(mRGB8 *dst,double h,double s,double l)
{
	mRGBd d;

	mHSL_to_RGBd(&d, h, s, l);

	dst->r = (int)(d.r * 255 + 0.5);
	dst->g = (int)(d.g * 255 + 0.5);
	dst->b = (int)(d.b * 255 + 0.5);
}

/**@ HSL (double) -> RGB (8bit:pac)
 *
 * @r:RGB を XRGB の値にして返す */

uint32_t mHSL_to_RGB8pac(double h,double s,double l)
{
	mRGB8 d;

	mHSL_to_RGB8(&d, h, s, l);

	return (d.r << 16) | (d.g << 8) | d.b;
}

/**@ RGB (double) -> HSL (double)
 *
 * @p:dst h は 0.0〜6.0 未満。l,s は 0.0〜1.0。
 * @p:r,g,b 0.0〜1.0 */

void mRGBd_to_HSL(mHSLd *dst,double r,double g,double b)
{
	double min,max,l,s;

	dst->h = _rgb_to_hue(r, g, b, &min, &max);

	l = (max + min) * 0.5;
	s = max - min;

	if(s != 0)
	{
		if(l <= 0.5)
			s = s / (max + min);
		else
			s = s / (2 - max - min);
	}

	dst->s = s;
	dst->l = l;
}

/**@ RGB (8bit) -> HSL (double) */

void mRGB8_to_HSL(mHSLd *dst,int r,int g,int b)
{
	mRGBd_to_HSL(dst, r / 255.0, g / 255.0, b / 255.0);
}

/**@ RGB (8bit:pac) -> HSL (double) */

void mRGB8pac_to_HSL(mHSLd *dst,uint32_t rgb)
{
	mRGBd_to_HSL(dst,
		((rgb >> 16) & 255) / 255.0,
		((rgb >> 8) & 255) / 255.0,
		(rgb & 255) / 255.0);
}

