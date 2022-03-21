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

#ifndef MLK_PIXBUF_H
#define MLK_PIXBUF_H

#define MPIXBUF_TPCOL  ((mPixCol)-1)

typedef void (*mFuncPixbufSetBuf)(uint8_t *buf,mPixCol col);
typedef mPixCol (*mFuncPixbufGetBuf)(uint8_t *buf);


struct _mPixbuf
{
	uint8_t *buf;
	int width,
		height,
		pixel_bytes,
		line_bytes;
};

typedef struct
{
	int sx,sy,w,h,pitch_dst,bytes;
}mPixbufClipBlt;


enum MPIXBUF_CREATE_FLAGS
{
	MPIXBUF_CREATE_HAVE_ALPHA = 1<<0
};

enum MPIXBUF_DRAWBTT_FLAGS
{
	MPIXBUF_DRAWBTT_PRESSED = 1<<0,
	MPIXBUF_DRAWBTT_FOCUSED = 1<<1,
	MPIXBUF_DRAWBTT_DISABLED = 1<<2,
	MPIXBUF_DRAWBTT_DEFAULT_BUTTON = 1<<3
};

enum MPIXBUF_DRAWCKBOX_FLAGS
{
	MPIXBUF_DRAWCKBOX_CHECKED = 1<<0,
	MPIXBUF_DRAWCKBOX_RADIO = 1<<1,
	MPIXBUF_DRAWCKBOX_DISABLE = 1<<2
};

/*---------*/

#ifdef __cplusplus
extern "C" {
#endif

mRgbCol mPixtoRGB(mPixCol c);
mPixCol mRGBtoPix(mRgbCol c);
mPixCol mRGBtoPix_sep(uint8_t r,uint8_t g,uint8_t b);
mPixCol mRGBtoPix_blend256(mRgbCol src,mRgbCol dst,int a);

void mPixbufSetBufFunc_blend128(uint8_t *buf,mPixCol col);

/* mPixbuf */

mPixbuf *mPixbufCreate(int w,int h,uint8_t flags);
void mPixbufFree(mPixbuf *p);

mlkbool mPixbufResizeStep(mPixbuf *p,int w,int h,int stepw,int steph);

void mPixbufSetOffset(mPixbuf *p,int x,int y,mPoint *pt);
void mPixbufSetPixelModeXor(mPixbuf *p);
void mPixbufSetPixelModeCol(mPixbuf *p);

void mPixbufSetFunc_setbuf(mPixbuf *p,mFuncPixbufSetBuf func);
void mPixbufGetFunc_setbuf(mPixbuf *p,mFuncPixbufSetBuf *dst);
void mPixbufGetFunc_getbuf(mPixbuf *p,mFuncPixbufGetBuf *dst);

//clip

void mPixbufClip_clear(mPixbuf *p);
mlkbool mPixbufClip_setBox_d(mPixbuf *p,int x,int y,int w,int h);
mlkbool mPixbufClip_setBox(mPixbuf *p,const mBox *box);
mlkbool mPixbufClip_isPointIn_abs(mPixbuf *p,int x,int y);
mlkbool mPixbufClip_getRect_d(mPixbuf *p,mRect *rc,int x,int y,int w,int h);
mlkbool mPixbufClip_getBox_d(mPixbuf *p,mBox *box,int x,int y,int w,int h);
mlkbool mPixbufClip_getBox(mPixbuf *p,mBox *boxdst,const mBox *boxsrc);
uint8_t *mPixbufClip_getBltInfo(mPixbuf *p,mPixbufClipBlt *dst,int x,int y,int w,int h);
uint8_t *mPixbufClip_getBltInfo_srcpos(mPixbuf *p,mPixbufClipBlt *dst,int x,int y,int w,int h,int sx,int sy);

//get

uint8_t *mPixbufGetBufPt(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPtFast(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPt_abs(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPtFast_abs(mPixbuf *p,int x,int y);
uint8_t *mPixbufGetBufPt_clip(mPixbuf *p,int x,int y);

mRgbCol mPixbufGetPixelRGB(mPixbuf *p,int x,int y);
void mPixbufGetPixelBufRGB(mPixbuf *p,uint8_t *buf,int *pr,int *pg,int *pb);

//setpixel

void mPixbufSetPixel(mPixbuf *p,int x,int y,mPixCol pix);
void mPixbufSetPixelRGB(mPixbuf *p,int x,int y,mRgbCol rgb);
void mPixbufSetPixelRGB_sep(mPixbuf *p,int x,int y,uint8_t r,uint8_t g,uint8_t b);
void mPixbufSetPixels(mPixbuf *p,mPoint *pt,int num,mPixCol pix);

void mPixbufBlendPixel(mPixbuf *p,int x,int y,mRgbCol rgb,int a);
void mPixbufBlendPixel_lcd(mPixbuf *p,int x,int y,mRgbCol rgb,uint8_t ra,uint8_t ga,uint8_t ba);
void mPixbufBlendPixel_a128(mPixbuf *p,int x,int y,uint32_t col);
void mPixbufBlendPixel_a128_buf(mPixbuf *p,uint8_t *buf,uint32_t col);

//draw

void mPixbufBufLineH(mPixbuf *p,uint8_t *buf,int w,mPixCol pix);
void mPixbufBufLineV(mPixbuf *p,uint8_t *buf,int h,mPixCol pix);
void mPixbufLineH(mPixbuf *p,int x,int y,int w,mPixCol pix);
void mPixbufLineV(mPixbuf *p,int x,int y,int h,mPixCol pix);
void mPixbufLineH_blend256(mPixbuf *p,int x,int y,int w,mRgbCol rgb,int a);
void mPixbufLineV_blend256(mPixbuf *p,int x,int y,int h,mRgbCol rgb,int a);

void mPixbufLine(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix);
void mPixbufLine_excludeEnd(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix);
void mPixbufLines_close(mPixbuf *p,mPoint *pt,int num,mPixCol pix);
void mPixbufBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufBox_pixel(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufEllipse(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix);
void mPixbufFill(mPixbuf *p,mPixCol pix);
void mPixbufFillBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);
void mPixbufFillBox_pat2x2(mPixbuf *p,int x,int y,int w,int h,mPixCol pix);

void mPixbufDraw1bitPattern(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,mPixCol coloff,mPixCol colon);
void mPixbufDraw1bitPattern_2col_mask(mPixbuf *p,int x,int y,
	const uint8_t *bufcol,const uint8_t *bufmask,int patw,int path,mPixCol coloff,mPixCol colon);
void mPixbufDraw1bitPattern_list(mPixbuf *p,int x,int y,
	const uint8_t *img,int imgw,int imgh,int eachw,const uint8_t *index,mPixCol col);
void mPixbufDraw2bitPattern(mPixbuf *p,int x,int y,const uint8_t *pat,int patw,int path,mPixCol *pcol);
void mPixbufDraw2bitPattern_list(mPixbuf *p,int x,int y,
	const uint8_t *img,int imgw,int imgh,int eachw,const uint8_t *index,mPixCol *pcol);

void mPixbufDrawNumberPattern_5x7(mPixbuf *p,int x,int y,const char *text,mPixCol col);

//blt

void mPixbufBlt(mPixbuf *dst,int dx,int dy,mPixbuf *src,int sx,int sy,int w,int h);
void mPixbufBlt_imagebuf(mPixbuf *p,int dx,int dy,mImageBuf *src,int sx,int sy,int w,int h);
void mPixbufBlt_1bit(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw,mPixCol coloff,mPixCol colon);
void mPixbufBlt_2bit(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw,mPixCol *pcol);
void mPixbufBlt_gray8(mPixbuf *p,int dx,int dy,const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw);
void mPixbufBltFill_imagebuf(mPixbuf *p,int x,int y,int w,int h,mImageBuf *src);

void mPixbufBltScale_oversamp_imagebuf(mPixbuf *p,int x,int y,int w,int h,mImageBuf *src,int overnum);

//ui

void mPixbufDrawBkgndShadow(mPixbuf *p,int x,int y,int w,int h,int colno);
void mPixbufDrawBkgndShadowRev(mPixbuf *p,int x,int y,int w,int h,int colno);
void mPixbufDrawButton(mPixbuf *p,int x,int y,int w,int h,uint8_t flags);
void mPixbufDrawCheckBox(mPixbuf *p,int x,int y,uint32_t flags);
void mPixbufDraw3DFrame(mPixbuf *p,int x,int y,int w,int h,mPixCol col_lt,mPixCol col_rb);
void mPixbufDrawChecked(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawMenuChecked(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawMenuRadio(mPixbuf *p,int x,int y,mPixCol col);
void mPixbufDrawArrowDown(mPixbuf *p,int x,int y,int size,mPixCol col);
void mPixbufDrawArrowUp(mPixbuf *p,int x,int y,int size,mPixCol col);
void mPixbufDrawArrowLeft(mPixbuf *p,int x,int y,int size,mPixCol col);
void mPixbufDrawArrowRight(mPixbuf *p,int x,int y,int size,mPixCol col);

#ifdef __cplusplus
}
#endif

#endif
