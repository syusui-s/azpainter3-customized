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

/******************************
 * カラー関連
 ******************************/

#ifndef AZPT_COLORVALUE_H
#define AZPT_COLORVALUE_H

typedef struct _mHSVd mHSVd;


#define COLCONV_8_TO_16(c)  ((((c) << 15) + 127) / 255)
#define COLCONV_16_TO_8(c)  (((c) * 255 + 0x4000) >> 15)

typedef union _RGB8
{
	struct{ uint8_t r,g,b; };
	uint8_t ar[3];
}RGB8;

typedef union _RGBA8
{
	struct{ uint8_t r,g,b,a; };
	uint8_t ar[4];
	uint32_t v32;
}RGBA8;

typedef union _RGB16
{
	struct{ uint16_t r,g,b; };
	uint16_t ar[3];
}RGB16;

typedef union _RGBA16
{
	struct{ uint16_t r,g,b,a; };
	uint16_t ar[4];
	uint64_t v64;
}RGBA16;

typedef union _RGBAdouble
{
	struct{ double r,g,b,a; };
	double ar[4];
}RGBAdouble;

//8bit と 16bit 両方
typedef struct _RGBcombo
{
	RGB8 c8;
	RGB16 c16;
}RGBcombo;

typedef struct _RGBAcombo
{
	RGBA8 c8;
	RGBA16 c16;
}RGBAcombo;


int Color8bit_to_16bit(int n);
int Color16bit_to_8bit(int n);
int Density100_to_bitval(int n,int bits);

//カラービット数の値
void RGBcombo_to_bitcol(void *dst,const RGBcombo *src,int bits);
void bitcol_to_RGBAcombo(RGBAcombo *dst,void *src,int bits);
void bitcol_set_alpha_density100(void *dst,int val,int bits);

//32bit/64bit 値取得
uint32_t RGBcombo_to_32bit(const RGBcombo *p);
uint32_t RGBcombo_to_pixcol(const RGBcombo *p);
uint32_t RGBA8_to_32bit(const RGBA8 *p);
uint32_t RGBA8_to_pixcol(const RGBA8 *p);
uint32_t RGBA16_to_32bit(const RGBA16 *p);
uint64_t RGB16_to_64bit_buf(const RGB16 *p);

//32bit 値から変換
void RGB32bit_to_RGB8(RGB8 *dst,uint32_t col);
void RGB32bit_to_RGB16(RGB16 *dst,uint32_t col);
void RGB32bit_to_RGBcombo(RGBcombo *dst,uint32_t col);
void RGBA32bit_to_RGBA8(RGBA8 *dst,uint32_t col);

//セット・変換
void RGBcombo_set16_from8(RGBcombo *p);
void RGBAcombo_to_RGBcombo(RGBcombo *dst,const RGBAcombo *src);
void RGB8_to_RGBA8(RGBA8 *dst,const RGB8 *src,int a);
void RGB8_to_RGB16(RGB16 *dst,const RGB8 *src);
void RGB16_to_RGB8(RGB8 *dst,const RGB16 *src);
void RGB16_to_RGBA16(RGBA16 *dst,const RGB16 *src,int a);
void RGBA8_to_RGBA16(RGBA16 *dst,const RGBA8 *src);
void RGBA16_to_RGBA8(RGBA8 *dst,const RGBA16 *src);

//ほか
mlkbool RGB8_compare_32bit(const RGB8 *p,uint32_t col);
void RGBcombo_createMiddle(RGBcombo *dst,const RGBcombo *src);

//色変換
void RGB8_to_HSVd(mHSVd *dst,const RGB8 *src);
void HSVd_to_RGB8(RGB8 *dst,const mHSVd *src);

void ColorText_to_RGB8(RGB8 *dst,const char *text);

#endif
