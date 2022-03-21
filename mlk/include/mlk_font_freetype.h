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

#ifndef MLK_FONT_FREETYPE_H
#define MLK_FONT_FREETYPE_H

typedef struct _FcPattern * mFcPattern;
typedef struct _mFTMetrics mFTMetrics;
typedef struct _mFTGlyphDraw mFTGlyphDraw;
typedef struct _mFTSubData mFTSubData;


struct _mFTMetrics
{
	int height,
		line_height,
		pixel_per_em,
		ascender,
		underline_pos,
		underline_thickness;
};

typedef struct _mFTPos
{
	int x,y,advance;
}mFTPos;


#ifdef MLK_FONT_FREETYPE_DEFINE

#define MFT_DOUBLE_TO_F16(d)       ((d) * (1<<16) + 0.5)
#define MFT_FUNIT_TO_PX_FLOOR(v,s) (FT_MulFix(v,s) >> 6)
#define MFT_FUNIT_TO_PX_CEIL(v,s)  ((FT_MulFix(v,s) + 32) >> 6)
#define MFT_F6_TO_INT_FLOOR(n)     ((n) >> 6)
#define MFT_F6_TO_INT_CEIL(n)      (((n) + 32) >> 6)

/*----------------*/

struct _mFTGlyphDraw
{
	uint32_t flags,
		fload_glyph;
	FT_Render_Mode render_mode;
	int lcd_filter;
	FT_Matrix slant_matrix;

	uint8_t def_hinting,
		def_lcdfilter,
		def_rendering,
		def_flags;
};

struct _mFontSystem
{
	FT_Library lib;
	int lcd_filter;
};

struct _mFont
{
	FT_Face face;
	mFontSystem *sys;

	mFTSubData *sub;
	void (*free_font)(mFont *);

	mFTMetrics mt;
	mFTGlyphDraw gdraw;

	int dpi;
};

#endif /* MLK_FONT_FREETYPE_DEFINE */

/*----------------*/

enum MFTGLYPHDRAW_FLAGS
{
	MFTGLYPHDRAW_F_SUBPIXEL_BGR = 1<<0,
	MFTGLYPHDRAW_F_EMBOLDEN = 1<<1,
	MFTGLYPHDRAW_F_SLANT_MATRIX = 1<<2
};

enum MFTGLYPHDRAW_HINTING
{
	MFTGLYPHDRAW_HINTING_NONE,
	MFTGLYPHDRAW_HINTING_SLIGHT,
	MFTGLYPHDRAW_HINTING_MEDIUM,
	MFTGLYPHDRAW_HINTING_FULL
};

enum MFTGLYPHDRAW_RENDER
{
	MFTGLYPHDRAW_RENDER_MONO,
	MFTGLYPHDRAW_RENDER_GRAY,
	MFTGLYPHDRAW_RENDER_LCD_RGB,
	MFTGLYPHDRAW_RENDER_LCD_BGR,
	MFTGLYPHDRAW_RENDER_LCD_VRGB,
	MFTGLYPHDRAW_RENDER_LCD_VBGR,

	MFTGLYPHDRAW_RENDER_NUM
};

enum MFTGLYPHDRAW_SWITCH
{
	MFTGLYPHDRAW_SWITCH_AUTOHINT = 1<<0,
	MFTGLYPHDRAW_SWITCH_EMBED_BITMAP = 1<<1,
	MFTGLYPHDRAW_SWITCH_EMBOLDEN = 1<<2,
	MFTGLYPHDRAW_SWITCH_SLANT_MATRIX = 1<<3
};

/*----------------*/


#ifdef __cplusplus
extern "C" {
#endif

int mFontFT_round_fix6(int32_t n);

void *mFontFT_getFace(mFont *p);
void mFontFT_getMetrics(mFont *p,mFTMetrics *dst);

void mFontFT_setLCDFilter(mFont *p);
void mFontFT_setMetrics(mFont *p);
void mFontFT_setFontSize(mFont *p,const mFontInfo *info,mFcPattern pat);

uint32_t mFontFT_codeToGID(mFont *p,uint32_t code);
uint32_t mFontFT_GID_to_CID(mFont *p,uint32_t gid);
int mFontFT_getGlyphHeight(mFont *p,uint32_t code);
int mFontFT_getGlyphWidth_code(mFont *p,uint32_t code);

mlkbool mFontFT_loadGlyphH_gid(mFont *p,uint32_t gid,mFTPos *pos);
mlkbool mFontFT_loadGlyphH_code(mFont *p,uint32_t code,mFTPos *pos);
int mFontFT_drawGlyphH_gid(mFont *p,int x,int y,uint32_t gid,mFTPos *pos,mFontDrawInfo *info,void *param);
int mFontFT_drawGlyphH_code(mFont *p,int x,int y,uint32_t code,mFontDrawInfo *info,void *param);

void mFontFT_drawGlyph(mFont *p,int x,int y,mFontDrawInfo *drawinfo,void *param);

void mFontFT_setGlyphDraw_default(mFont *p);
void mFontFT_setGlyphDraw_fontconfig(mFont *p,mFcPattern pat,const mFontInfo *info);
void mFontFT_setGlyphDraw_infoEx(mFont *p,const mFontInfo *info);
void mFontFT_setGlyphDraw_hinting(mFont *p,int type);
void mFontFT_setGlyphDraw_rendermode(mFont *p,int type);
void mFontFT_setGlyphDraw_switch(mFont *p,uint32_t mask,uint32_t flags);

mlkerr mFontFT_loadTable(mFont *p,uint32_t tag,void **ppbuf,uint32_t *psize);
mlkbool mFontFT_getNameTbl_id(void *face,int id,mStr *str);
mlkbool mFontFT_getFontFullName(void *face,mStr *str);
void mFontFT_enumVariableStyle(void *ftlib,void *face,void (*func)(int index,const char *name,void *param),void *param);

#ifdef __cplusplus
}
#endif

#endif
