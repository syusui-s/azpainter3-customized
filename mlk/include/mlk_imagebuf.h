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

#ifndef MLK_IMAGEBUF_H
#define MLK_IMAGEBUF_H

#define MLK_IMAGEBUF_GETBUFPT(p,x,y)  ((p)->buf + (y) * (p)->line_bytes + (x) * (p)->pixel_bytes)

typedef struct _mLoadImageOpen mLoadImageOpen;
typedef struct _mLoadImageType mLoadImageType;

struct _mImageBuf
{
	uint8_t *buf;
	int width,
		height,
		pixel_bytes,
		line_bytes;
};

struct _mImageBuf2
{
	uint8_t **ppbuf;
	int width,
		height,
		pixel_bytes,
		line_bytes;
};


#ifdef __cplusplus
extern "C" {
#endif

/* mImageBuf */

mImageBuf *mImageBuf_new(int w,int h,int bits,int line_bytes);
void mImageBuf_free(mImageBuf *p);

mImageBuf *mImageBuf_loadImage(mLoadImageOpen *open,mLoadImageType *type,int bits,int line_bytes);

void mImageBuf_blendColor(mImageBuf *p,mRgbCol col);

/* mImageBuf2 */

mImageBuf2 *mImageBuf2_new(int w,int h,int bits,int line_bytes);
mImageBuf2 *mImageBuf2_new_align(int w,int h,int bits,int line_bytes,int alignment);
void mImageBuf2_free(mImageBuf2 *p);
void mImageBuf2_freeImage(mImageBuf2 *p);

void mImageBuf2_clear0(mImageBuf2 *p);
mlkbool mImageBuf2_crop_keep(mImageBuf2 *p,int left,int top,int width,int height,int line_bytes);

#ifdef __cplusplus
}
#endif

#endif
