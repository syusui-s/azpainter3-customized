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

#ifndef MLIB_PIXBUF_PV_H
#define MLIB_PIXBUF_PV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint8_t flags;
	int offsetX,offsetY;
	mRect clip,clipMaster;
}mPixbufPrivate;

typedef struct
{
	mPixbuf b;
	mPixbufPrivate p;
}mPixbufBase;

typedef struct
{
	int dx,dy,sx,sy,w,h;
}mPixbufBltInfo;


#define M_PIXBUFBASE(p)  ((mPixbufBase *)(p))

#define MPIXBUF_F_CLIP        1
#define MPIXBUF_F_CLIP_MASTER 2


void __mPixbufSetClipMaster(mPixbuf *p,mRect *rc);
mBool __mPixbufBltClip(mPixbufBltInfo *p,mPixbuf *dst,int dx,int dy,int sx,int sy,int w,int h);

#ifdef __cplusplus
}
#endif

#endif
