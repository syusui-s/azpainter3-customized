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

#ifndef MLK_COLOR_H
#define MLK_COLOR_H

typedef struct _mRGB8
{
	uint8_t r,g,b;
}mRGB8;

typedef struct _mRGBd
{
	double r,g,b;
}mRGBd;

typedef struct _mHSVd
{
	double h,s,v;
}mHSVd;

typedef struct _mHSLd
{
	double h,s,l;
}mHSLd;


#ifdef __cplusplus
extern "C" {
#endif

void mHSV_to_RGBd(mRGBd *dst,double h,double s,double v);
void mHSV_to_RGB8(mRGB8 *dst,double h,double s,double v);
uint32_t mHSV_to_RGB8pac(double h,double s,double v);
uint32_t mHSVi_to_RGB8pac(int h,int s,int v);

void mRGBd_to_HSV(mHSVd *dst,double r,double g,double b);
void mRGB8_to_HSV(mHSVd *dst,int r,int g,int b);
void mRGB8pac_to_HSV(mHSVd *dst,uint32_t rgb);

void mHSL_to_RGBd(mRGBd *dst,double h,double s,double l);
void mHSL_to_RGB8(mRGB8 *dst,double h,double s,double l);
uint32_t mHSL_to_RGB8pac(double h,double s,double l);

void mRGBd_to_HSL(mHSLd *dst,double r,double g,double b);
void mRGB8_to_HSL(mHSLd *dst,int r,int g,int b);
void mRGB8pac_to_HSL(mHSLd *dst,uint32_t rgb);

#ifdef __cplusplus
}
#endif

#endif
