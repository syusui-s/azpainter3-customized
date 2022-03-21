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
 * キャンバス用イメージ
 ******************************/

typedef struct _ImageCanvas ImageCanvas;
typedef struct _RGBcombo RGBcombo;
typedef struct _CanvasDrawInfo CanvasDrawInfo;
typedef struct _mPopupProgress mPopupProgress;

struct _ImageCanvas
{
	uint8_t **ppbuf;
	int width,
		height,
		bits, //8 or 16
		line_bytes;
};


ImageCanvas *ImageCanvas_new(int width,int height,int bits);
void ImageCanvas_free(ImageCanvas *p);

uint8_t *ImageCanvas_getBufPt(ImageCanvas *p,int x,int y);
void ImageCanvas_getPixel_rgba(ImageCanvas *p,int x,int y,void *dst);
void ImageCanvas_getPixel_combo(ImageCanvas *p,int x,int y,RGBcombo *dst);

void ImageCanvas_fill(ImageCanvas *p,RGBcombo *col);
void ImageCanvas_fillBox(ImageCanvas *p,const mBox *box,RGBcombo *col);
void ImageCanvas_fillPlaidBox(ImageCanvas *p,const mBox *box,RGBcombo *col1,RGBcombo *col2);
void ImageCanvas_setAlphaMax(ImageCanvas *p);
mlkbool ImageCanvas_setThumbnailImage_8bit(ImageCanvas *p,int width,int height);

void ImageCanvas_drawPixbuf_nearest(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_drawPixbuf_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_drawPixbuf_rotate(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);
void ImageCanvas_drawPixbuf_rotate_oversamp(ImageCanvas *src,mPixbuf *dst,CanvasDrawInfo *info);

ImageCanvas *ImageCanvas_resize(ImageCanvas *src,int neww,int newh,int method,mPopupProgress *prog,int stepnum);

