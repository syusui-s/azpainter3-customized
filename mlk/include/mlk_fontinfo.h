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

#ifndef MLK_FONTINFO_H
#define MLK_FONTINFO_H

typedef struct
{
	uint32_t mask;
	int hinting,
		rendering,
		lcd_filter;
	uint32_t flags;
}mFontInfoEx;

struct _mFontInfo
{
	uint32_t mask;
	mStr str_family,
		str_style,
		str_file;
	double size;
	int index,
		weight,
		slant,
		dpi;

	mFontInfoEx ex;
};

enum MFONTINFO_MASK
{
	MFONTINFO_MASK_FAMILY = 1<<0,
	MFONTINFO_MASK_STYLE  = 1<<1,
	MFONTINFO_MASK_SIZE   = 1<<2,
	MFONTINFO_MASK_WEIGHT = 1<<3,
	MFONTINFO_MASK_SLANT  = 1<<4,
	MFONTINFO_MASK_DPI    = 1<<5,
	MFONTINFO_MASK_FILE   = 1<<6,
	MFONTINFO_MASK_EX     = 1<<7,

	MFONTINFO_MASK_ALL = 0xff
};

enum MFONTINFO_WEIGHT
{
	MFONTINFO_WEIGHT_THIN = 0,
	MFONTINFO_WEIGHT_EXLIGHT = 40,
	MFONTINFO_WEIGHT_LIGHT = 50,
	MFONTINFO_WEIGHT_DEMILIGHT = 55,
	MFONTINFO_WEIGHT_BOOK = 75,
	MFONTINFO_WEIGHT_REGULAR = 80,
	MFONTINFO_WEIGHT_MEDIUM = 100,
	MFONTINFO_WEIGHT_DEMIBOLD = 180,
	MFONTINFO_WEIGHT_BOLD = 200,
	MFONTINFO_WEIGHT_EXBOLD = 205,
	MFONTINFO_WEIGHT_BLACK = 210,
	MFONTINFO_WEIGHT_EXBLACK = 215
};

enum MFONTINFO_SLANT
{
	MFONTINFO_SLANT_ROMAN = 0,
	MFONTINFO_SLANT_ITALIC = 100,
	MFONTINFO_SLANT_OBLIQUE = 110
};

enum MFONTINFO_EX_MASK
{
	MFONTINFO_EX_MASK_HINTING = 1<<0,
	MFONTINFO_EX_MASK_RENDERING = 1<<1,
	MFONTINFO_EX_MASK_LCD_FILTER = 1<<2,
	MFONTINFO_EX_MASK_AUTO_HINT = 1<<3,
	MFONTINFO_EX_MASK_EMBEDDED_BITMAP = 1<<4
};

enum MFONTINFO_EX_HINTING
{
	MFONTINFO_EX_HINTING_NONE = 0,
	MFONTINFO_EX_HINTING_SLIGHT,
	MFONTINFO_EX_HINTING_MEDIUM,
	MFONTINFO_EX_HINTING_FULL
};

enum MFONTINFO_EX_RENDERING
{
	MFONTINFO_EX_RENDERING_MONO = 0,
	MFONTINFO_EX_RENDERING_GRAY,
	MFONTINFO_EX_RENDERING_LCD_RGB,
	MFONTINFO_EX_RENDERING_LCD_BGR,
	MFONTINFO_EX_RENDERING_LCD_VRGB,
	MFONTINFO_EX_RENDERING_LCD_VBGR
};

enum MFONTINFO_EX_LCDFILTER
{
	MFONTINFO_EX_LCDFILTER_NONE = 0,
	MFONTINFO_EX_LCDFILTER_DEFAULT,
	MFONTINFO_EX_LCDFILTER_LIGHT,
	MFONTINFO_EX_LCDFILTER_LEGACY
};

enum MFONTINFO_EX_FLAGS
{
	MFONTINFO_EX_FLAGS_AUTO_HINT = 1<<0,
	MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP = 1<<1
};


#ifdef __cplusplus
extern "C" {
#endif

void mFontInfoInit(mFontInfo *p);
void mFontInfoFree(mFontInfo *p);

void mFontInfoCopy(mFontInfo *dst,const mFontInfo *src);
void mFontInfoSetFromText(mFontInfo *dst,const char *format);
void mFontInfoToText(mStr *str,const mFontInfo *info);
void mFontInfoGetText_family_size(mStr *str,const mFontInfo *info);
void mFontInfoCopyName(mFontInfo *dst,const mFontInfo *src);

void mFontInfoSetDefault_embeddedBitmap(mFontInfo *p,int is_true);

#ifdef __cplusplus
}
#endif

#endif
