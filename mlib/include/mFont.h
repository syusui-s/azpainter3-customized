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

#ifndef MLIB_FONT_H
#define MLIB_FONT_H

#include "mStrDef.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _mFont
{
	int height,lineheight;
};

typedef struct _mFontInfo
{
	uint32_t mask;
	mStr strFamily,strStyle;
	double size;
	int weight,slant,render;
}mFontInfo;


enum MFONTINFO_MASK
{
	MFONTINFO_MASK_FAMILY = 1<<0,
	MFONTINFO_MASK_STYLE  = 1<<1,
	MFONTINFO_MASK_SIZE   = 1<<2,
	MFONTINFO_MASK_WEIGHT = 1<<3,
	MFONTINFO_MASK_SLANT  = 1<<4,
	MFONTINFO_MASK_RENDER = 1<<5,

	MFONTINFO_MASK_ALL = 0xff
};

enum MFONTINFO_WEIGHT
{
	MFONTINFO_WEIGHT_NORMAL = 100,
	MFONTINFO_WEIGHT_BOLD   = 200
};

enum MFONTINFO_SLANT
{
	MFONTINFO_SLANT_ROMAN,
	MFONTINFO_SLANT_ITALIC,
	MFONTINFO_SLANT_OBLIQUE
};

enum MFONTINFO_RENDER
{
	MFONTINFO_RENDER_DEFAULT,
	MFONTINFO_RENDER_MONO,
	MFONTINFO_RENDER_GRAY,
	MFONTINFO_RENDER_LCD_RGB,
	MFONTINFO_RENDER_LCD_BGR,
	MFONTINFO_RENDER_LCD_VRGB,
	MFONTINFO_RENDER_LCD_VBGR
};

/*---------*/

void mFontFree(mFont *font);
mFont *mFontCreate(mFontInfo *info);

void *mFontGetHandle(mFont *font);

void mFontDrawText(mFont *font,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col);
void mFontDrawTextHotkey(mFont *font,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col);
void mFontDrawTextUCS4(mFont *font,mPixbuf *img,int x,int y,
	const uint32_t *text,int len,mRgbCol col);

int mFontGetTextWidth(mFont *font,const char *text,int len);
int mFontGetTextWidthHotkey(mFont *font,const char *text,int len);
int mFontGetTextWidthUCS4(mFont *font,const uint32_t *text,int len);
int mFontGetCharWidthUCS4(mFont *font,uint32_t ucs);

/* common */

void mFontInfoFree(mFontInfo *p);
mFont *mFontCreateFromFormat(const char *format);
mFont *mFontCreateFromFormat_size(const char *format,double size);

void mFontFormatToInfo(mFontInfo *p,const char *format);
void mFontInfoToFormat(mStr *str,mFontInfo *info);

#ifdef __cplusplus
}
#endif

#endif
