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

#ifndef MLIB_COLORCONV_H
#define MLIB_COLORCONV_H

#ifdef __cplusplus
extern "C" {
#endif

void mHSVtoRGB(double h,double s,double v,int *dst);
mRgbCol mHSVtoRGB_pac(double h,double s,double v);
mRgbCol mHSVtoRGB_fast(int h,int s,int v);

void mRGBtoHSV(int r,int g,int b,double *dst);
void mRGBtoHSV_pac(mRgbCol rgb,double *dst);

void mHLStoRGB(int h,double l,double s,int *dst);
mRgbCol mHLStoRGB_pac(int h,double l,double s);

void mRGBtoHLS(int r,int g,int b,double *dst);
void mRGBtoHLS_pac(mRgbCol rgb,double *dst);

#ifdef __cplusplus
}
#endif

#endif
