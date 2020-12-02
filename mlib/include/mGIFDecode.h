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

#ifndef MLIB_GIFDECODE_H
#define MLIB_GIFDECODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mGIFDecode mGIFDecode;
typedef void (*mGIFDecodeErrorFunc)(mGIFDecode *,const char *,void *);

enum MGIFDECODE_FORMAT
{
	MGIFDECODE_FORMAT_RAW,
	MGIFDECODE_FORMAT_RGB,
	MGIFDECODE_FORMAT_RGBA
};


mGIFDecode *mGIFDecode_create(void);
void mGIFDecode_close(mGIFDecode *p);

mBool mGIFDecode_open_filename(mGIFDecode *p,const char *filename);
mBool mGIFDecode_open_buf(mGIFDecode *p,const void *buf,uint32_t bufsize);
mBool mGIFDecode_open_stdio(mGIFDecode *p,void *fp);

void mGIFDecode_setErrorFunc(mGIFDecode *p,mGIFDecodeErrorFunc func,void *param);

mBool mGIFDecode_readHeader(mGIFDecode *p,mSize *size);
uint8_t *mGIFDecode_getPalette(mGIFDecode *p,int *pnum);
int mGIFDecode_getTransparentColor(mGIFDecode *p);

int mGIFDecode_nextImage(mGIFDecode *p);
mBool mGIFDecode_getNextLine(mGIFDecode *p,void *buf,int format,mBool trans_to_a0);

#ifdef __cplusplus
}
#endif

#endif
