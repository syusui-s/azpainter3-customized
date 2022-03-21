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

/************************************
 * 保存設定の値
 ************************************/

/* PNG
 * [bit0-3{4}:level (0-9)] [bit4:16bit] [bit5:alpha] */

#define SAVEOPT_PNG_DEFAULT  6

#define SAVEOPT_PNG_F_16BIT  (1<<4)
#define SAVEOPT_PNG_F_ALPHA  (1<<5)
#define SAVEOPT_PNG_GET_LEVEL(n)  ((n) & 15)

/* JPEG
 * [bit0-6{7}:quality (0-100)] [bit7-8{2}: sampling] [bit9:progressive] */

#define SAVEOPT_JPEG_DEFAULT 85

#define SAVEOPT_JPEG_BIT_SAMPLING  7

#define SAVEOPT_JPEG_F_PROGRESSIVE   (1<<9)
#define SAVEOPT_JPEG_GET_QUALITY(n)  ((n) & 127)
#define SAVEOPT_JPEG_GET_SAMPLING(n) (((n) >> 7) & 3)

/* TIFF
 * [bit0-3{4}:type] [bit4:16bit] */

#define SAVEOPT_TIFF_DEFAULT 3

#define SAVEOPT_TIFF_F_16BIT  (1<<4)
#define SAVEOPT_TIFF_GET_COMPTYPE(n)  ((n) & 15)

/* WEBP
 * [bit0:0=可逆,1=不可逆] [bit1-4{4}:level (0-9)] [bit5-11{7}:quality (0-100)]*/

#define SAVEOPT_WEBP_DEFAULT ((6<<1) | (80<<5))

#define SAVEOPT_WEBP_BIT_LEVEL   1
#define SAVEOPT_WEBP_BIT_QUALITY 5

#define SAVEOPT_WEBP_F_IRREVERSIBLE  1
#define SAVEOPT_WEBP_GET_LEVEL(n)   (((n)>>1) & 15)
#define SAVEOPT_WEBP_GET_QUALITY(n) (((n)>>5) & 127)

/* PSD
 * [bit0-1{2}:type] [bit2:16bit] [bit3:無圧縮] */

#define SAVEOPT_PSD_DEFAULT  0

#define SAVEOPT_PSD_F_16BIT      (1<<2)
#define SAVEOPT_PSD_F_UNCOMPRESS (1<<3)
#define SAVEOPT_PSD_GET_TYPE(n)  ((n) & 3)

