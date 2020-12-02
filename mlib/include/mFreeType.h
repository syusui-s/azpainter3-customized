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

#ifndef MLIB_FREETYPE_H
#define MLIB_FREETYPE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _FcPattern mFcPattern;
typedef struct _mFontInfo mFontInfo;

typedef struct _mFreeTypeInfo
{
	uint32_t flags,fLoadGlyph;
	FT_Render_Mode nRenderMode;
	int nLCDFilter;
	double dpi,size;
	FT_Matrix matrix;
}mFreeTypeInfo;

typedef struct
{
	int height,
		lineheight,
		baseline,
		underline;
}mFreeTypeMetricsInfo;


#define MFTINFO_F_SUBPIXEL_BGR 1
#define MFTINFO_F_EMBOLDEN     2
#define MFTINFO_F_MATRIX       4

enum MFT_HINTING
{
	MFT_HINTING_NONE,
	MFT_HINTING_DEFAULT,
	MFT_HINTING_MAX
};

/*------*/

void mFreeTypeGetInfoByFontConfig(mFreeTypeInfo *dst,mFcPattern *pat,mFontInfo *info);
void mFreeTypeSetInfo_hinting(mFreeTypeInfo *dst,int type);

void mFreeTypeGetMetricsInfo(FT_Library lib,FT_Face face,mFreeTypeInfo *info,
	mFreeTypeMetricsInfo *dst);

int mFreeTypeGetHeightFromGlyph(FT_Library lib,FT_Face face,
	mFreeTypeInfo *info,int ascender,uint32_t code);

FT_BitmapGlyph mFreeTypeGetBitmapGlyph(FT_Library lib,FT_Face face,mFreeTypeInfo *info,uint32_t code);

void *mFreeTypeGetGSUB(FT_Face face);

mRgbCol mFreeTypeBlendColorGray(mRgbCol bgcol,mRgbCol fgcol,int a);
mRgbCol mFreeTypeBlendColorLCD(mRgbCol bgcol,mRgbCol fgcol,int ra,int ga,int ba);

#ifdef __cplusplus
}
#endif

#endif
