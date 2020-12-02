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

#ifndef MLIB_IMAGEBUF_H
#define MLIB_IMAGEBUF_H

typedef struct _mLoadImageSource mLoadImageSource;

typedef struct _mImageBuf
{
	uint8_t *buf;
	int w,h,bpp,pitch;
}mImageBuf;

#define MIMAGEBUF_GETBUFPT(p,x,y)  ((p)->buf + (p)->pitch * (y) + (x) * (p)->bpp)


#ifdef __cplusplus
extern "C" {
#endif

void mImageBufFree(mImageBuf *p);
mImageBuf *mImageBufCreate(int w,int h,int bpp);

mImageBuf *mImageBufLoadImage(mLoadImageSource *src,mDefEmptyFunc loadfunc,int bpp,char **errmes);

#ifdef __cplusplus
}
#endif

#endif
