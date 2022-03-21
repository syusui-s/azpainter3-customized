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

/******************************
 * ImageCanvas 内部用
 ******************************/

#define _SIMD_ON  1

#define FIXF_BIT  28
#define FIXF_VAL  ((int64_t)1 << FIXF_BIT)

typedef struct
{
	int srcw,srch,bpp,pitchd,dstw,dsth;
	uint32_t pixbkgnd;
	int64_t finc_xx,finc_xy,finc_yx,finc_yy,fx,fy;
	mFuncPixbufSetBuf setpix;
}_canvasparam;

uint8_t *__ImageCanvas_getCanvasParam(ImageCanvas *src,mPixbuf *pixbuf,CanvasDrawInfo *info,_canvasparam *param,mlkbool rotate);

//8bit

void ImageCanvas_8bit_fill(ImageCanvas *p,RGBcombo *col);
void ImageCanvas_8bit_fillBox(ImageCanvas *p,const mBox *box,RGBcombo *col);
void ImageCanvas_8bit_fillPlaidBox(ImageCanvas *p,const mBox *box,RGBcombo *col1,RGBcombo *col2);
void ImageCanvas_8bit_setAlphaMax(ImageCanvas *p);
void ImageCanvas_8bit_drawPixbuf_nearest(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_8bit_drawPixbuf_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_8bit_drawPixbuf_rotate(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_8bit_drawPixbuf_rotate_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);

//16bit

void ImageCanvas_16bit_fill(ImageCanvas *p,RGBcombo *col);
void ImageCanvas_16bit_fillBox(ImageCanvas *p,const mBox *box,RGBcombo *col);
void ImageCanvas_16bit_fillPlaidBox(ImageCanvas *p,const mBox *box,RGBcombo *col1,RGBcombo *col2);
void ImageCanvas_16bit_setAlphaMax(ImageCanvas *p);
void ImageCanvas_16bit_drawPixbuf_nearest(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_16bit_drawPixbuf_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_16bit_drawPixbuf_rotate(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_16bit_drawPixbuf_rotate_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);

