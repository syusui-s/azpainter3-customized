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

/************************************
 * 画像素材用イメージ
 ************************************/

#ifndef AZPT_IMAGEMATERIAL_H
#define AZPT_IMAGEMATERIAL_H

typedef struct _ImageMaterial ImageMaterial;

struct _ImageMaterial
{
	uint8_t *buf;
	int type,
		bits,
		width,
		height,
		pitch;
};

enum
{
	IMAGEMATERIAL_TYPE_TEXTURE,
	IMAGEMATERIAL_TYPE_BRUSH
};


ImageMaterial *ImageMaterial_new(int w,int h,int bits);
void ImageMaterial_free(ImageMaterial *p);

void ImageMaterial_clear(ImageMaterial *p);
uint8_t ImageMaterial_getPixel_forTexture(ImageMaterial *p,int x,int y);

ImageMaterial *ImageMaterial_loadTexture(mStr *strfname);
ImageMaterial *ImageMaterial_loadBrush(mStr *strfname);

void ImageMaterial_drawPreview(ImageMaterial *p,mPixbuf *pixbuf,int x,int y,int w,int h);
void ImageMaterial_drawPreview_texture(ImageMaterial *p,mPixbuf *pixbuf,int x,int y,int w,int h);
void ImageMaterial_drawPreview_brush(ImageMaterial *p,mPixbuf *pixbuf,int x,int y,int w,int h);

#endif
