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

#ifndef MLK_IMAGECONV_H
#define MLK_IMAGECONV_H

typedef struct _mImageConv mImageConv;

typedef void (*mFuncImageConv)(mImageConv *);

struct _mImageConv
{
	void *dstbuf;
	const void *srcbuf;
	const uint8_t *palbuf;
	
	int srcbits,
		dstbits,
		convtype,
		chno;
	uint32_t width,
		flags;
};


enum MIMAGECONV_CONVTYPE
{
	MIMAGECONV_CONVTYPE_NONE = 0,
	MIMAGECONV_CONVTYPE_RGB,
	MIMAGECONV_CONVTYPE_RGBA
};

enum MIMAGECONV_FLAGS
{
	MIMAGECONV_FLAGS_REVERSE = 1<<0,
	MIMAGECONV_FLAGS_SRC_BGRA = 1<<1,
	MIMAGECONV_FLAGS_SRC_ALPHA = 1<<2,
	MIMAGECONV_FLAGS_INVALID_ALPHA = 1<<3
};


#ifdef __cplusplus
extern "C" {
#endif

void mImageConv_swap_rb_8(uint8_t *buf,uint32_t width,int bytes);

void mImageConv_rgb8_to_rgba8_extend(uint8_t *buf,uint32_t width);

void mImageConv_rgbx8_to_rgb8(uint8_t *dst,const uint8_t *src,uint32_t width);
void mImageConv_rgbx8_to_gray8(uint8_t *dst,const uint8_t *src,uint32_t width);
void mImageConv_rgbx8_to_gray1(uint8_t *dst,const uint8_t *src,uint32_t width);

void mImageConv_gray_1_2_4_8(mImageConv *p);
void mImageConv_gray16(mImageConv *p);
void mImageConv_palette_1_2_4_8(mImageConv *p);
void mImageConv_rgb555(mImageConv *p);
void mImageConv_rgb8(mImageConv *p);
void mImageConv_rgba8(mImageConv *p);
void mImageConv_rgb_rgba_16(mImageConv *p);
void mImageConv_cmyk16(mImageConv *p);

void mImageConv_sepch_gray_a_8(mImageConv *p);
void mImageConv_sepch_gray_a_16(mImageConv *p);
void mImageConv_sepch_rgb_rgba_8(mImageConv *p);
void mImageConv_sepch_rgb_rgba_16(mImageConv *p);
void mImageConv_sepch_cmyk8(mImageConv *p);
void mImageConv_sepch_cmyk16(mImageConv *p);

#ifdef __cplusplus
}
#endif

#endif
