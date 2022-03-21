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

/************************************
 * RGBA 8bit イメージ
 ************************************/

#ifndef AZPT_IMAGE32_H
#define AZPT_IMAGE32_H

typedef struct _Image32
{
	uint8_t *buf;
	int w,h,pitch;
}Image32;

/** キャンバス描画用データ */

typedef struct
{
	int scroll_x,scroll_y,	//スクロール位置
		mirror;				//左右反転
	double origin_x,origin_y,	//イメージの原点位置
		scalediv;			//1.0/scale
	uint32_t bkgndcol;
}Image32CanvasInfo;

//読み込み時フラグ

enum
{
	IMAGE32_LOAD_F_BLEND_WHITE = 1<<0,	//白の背景色と合成
	IMAGE32_LOAD_F_THUMBNAIL = 1<<1		//サムネイル画像を読み込み
};

//------------

Image32 *Image32_new(int w,int h);
void Image32_free(Image32 *p);

uint8_t *Image32_getPixelBuf(Image32 *p,int x,int y);

Image32 *Image32_loadFile(const char *filename,uint8_t flags);
void Image32_blendBkgnd(Image32 *p,uint32_t col);
void Image32_clear(Image32 *p,uint32_t col);
void Image32_putPixbuf(Image32 *p,mPixbuf *dst,int dx,int dy);
void Image32_blt(Image32 *dst,int dx,int dy,Image32 *src,int sx,int sy,int w,int h);
void Image32_drawResize(Image32 *dst,Image32 *src);

void Image32_drawCanvas_nearest(Image32 *src,mPixbuf *dst,mBox *boxdst,Image32CanvasInfo *info);
void Image32_drawCanvas_oversamp(Image32 *src,mPixbuf *dst,	mBox *boxdst,Image32CanvasInfo *info);

#endif
