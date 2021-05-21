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

#ifndef MLK_FONT_H
#define MLK_FONT_H

typedef void (*mFuncFontSetPixelMono)(int x,int y,void *param);
typedef void (*mFuncFontSetPixelGray)(int x,int y,int a,void *param);
typedef void (*mFuncFontSetPixelLCD)(int x,int y,int ra,int ga,int ba,void *param);
typedef void (*mFuncFontDrawUnderline)(int x,int y,int w,int h,void *param);

struct _mFontDrawInfo
{
	mFuncFontSetPixelMono setpix_mono;
	mFuncFontSetPixelGray setpix_gray;
	mFuncFontSetPixelLCD setpix_lcd;
	mFuncFontDrawUnderline draw_underline;
};

typedef struct
{
	mPixbuf *pixbuf;
	mRgbCol rgbcol;
	mPixCol pixcol;
}mFontDrawParam_pixbuf;


#ifdef __cplusplus
extern "C" {
#endif

mFontSystem *mFontSystem_init(void);
void mFontSystem_finish(mFontSystem *p);

mRgbCol mFontBlendColor_gray(mRgbCol bgcol,mRgbCol fgcol,int a);
mRgbCol mFontBlendColor_lcd(mRgbCol bgcol,mRgbCol fgcol,int ra,int ga,int ba);

int mFontGetFilename_fromFamily(char **dst,const char *family,const char *style);

/* mFont */

void mFontFree(mFont *p);

mFont *mFontCreate_file(mFontSystem *sys,const char *filename,int index);
mFont *mFontCreate(mFontSystem *sys,const mFontInfo *info);
mFont *mFontCreate_default(mFontSystem *sys);
mFont *mFontCreate_family(mFontSystem *sys,const char *family,const char *style);
mFont *mFontCreate_raw(mFontSystem *sys,const mFontInfo *info);
mFont *mFontCreate_text(mFontSystem *sys,const char *text);
mFont *mFontCreate_text_size(mFontSystem *sys,const char *text,double size);

int mFontGetListNames(mFontSystem *sys,const char *filename,
	void (*func)(int index,const char *name,void *param),void *param);

int mFontGetHeight(mFont *p);
int mFontGetLineHeight(mFont *p);
int mFontGetVertHeight(mFont *p);
int mFontGetAscender(mFont *p);
mlkbool mFontIsHaveVert(mFont *p);

void mFontSetSize(mFont *p,double size);
void mFontSetSize_pt(mFont *p,double size,int dpi);
void mFontSetSize_pixel(mFont *p,int size);
void mFontSetSize_fromText(mFont *p,const char *text);
void mFontSetInfoEx(mFont *p,const mFontInfo *info);

int mFontGetTextWidth(mFont *p,const char *text,int len);
int mFontGetTextWidth_hotkey(mFont *p,const char *text,int len);
int mFontGetTextWidth_utf32(mFont *p,const mlkuchar *text,int len);
int mFontGetCharWidth_utf32(mFont *p,mlkuchar code);

void mFontDrawText(mFont *p,int x,int y,const char *text,int len,mFontDrawInfo *info,void *param);
void mFontDrawText_hotkey(mFont *p,int x,int y,const char *text,int len,mFontDrawInfo *info,void *param);
void mFontDrawText_utf32(mFont *p,int x,int y,const mlkuchar *text,int len,mFontDrawInfo *info,void *param);

/* pixbuf */
mFontDrawInfo *mFontGetDrawInfo_pixbuf(void);
void mFontSetDrawParam_pixbuf(mFontDrawParam_pixbuf *p,mPixbuf *pixbuf,mRgbCol col);
void mFontDrawText_pixbuf(mFont *p,mPixbuf *img,int x,int y,const char *text,int len,mRgbCol col);
void mFontDrawText_pixbuf_hotkey(mFont *p,mPixbuf *img,int x,int y,const char *text,int len,mRgbCol col);
void mFontDrawText_pixbuf_utf32(mFont *p,mPixbuf *img,int x,int y,const mlkuchar *text,int len,mRgbCol col);

#ifdef __cplusplus
}
#endif

#endif
