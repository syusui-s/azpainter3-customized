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

#ifndef MLIB_ZLIB_H
#define MLIB_ZLIB_H

#include "mFile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mZlibEncode mZlibEncode;
typedef struct _mZlibDecode mZlibDecode;

#define MZLIBDEC_WINDOWBITS_ZLIB     15
#define MZLIBDEC_WINDOWBITS_NOHEADER -15

enum
{
	MZLIBDEC_READ_OK,
	MZLIBDEC_READ_ZLIB,
	MZLIBDEC_READ_INDATA
};


void mZlibEncodeFree(mZlibEncode *p);
mZlibEncode *mZlibEncodeNew(int bufsize,int level,int windowbits,int memlevel,int strategy);
mZlibEncode *mZlibEncodeNew_simple(int bufsize,int level);
void mZlibEncodeSetIO_stdio(mZlibEncode *p,void *fp);
mBool mZlibEncodeSend(mZlibEncode *p,void *buf,int size);
mBool mZlibEncodeFlushEnd(mZlibEncode *p);
mBool mZlibEncodeReset(mZlibEncode *p);
uint32_t mZlibEncodeGetWriteSize(mZlibEncode *p);

void mZlibDecodeFree(mZlibDecode *p);
mZlibDecode *mZlibDecodeNew(int bufsize,int windowbits);
void mZlibDecodeSetIO_stdio(mZlibDecode *p,void *fp);
void mZlibDecodeSetInSize(mZlibDecode *p,uint32_t size);
void mZlibDecodeReset(mZlibDecode *p);
int mZlibDecodeReadOnce(mZlibDecode *p,void *buf,int bufsize,uint32_t insize);
int mZlibDecodeRead(mZlibDecode *p,void *buf,int size);
int mZlibDecodeReadEnd(mZlibDecode *p);

mBool mZlibDecodeFILE(mFile file,void *dstbuf,uint32_t dstsize,
	uint32_t compsize,uint32_t bufsize,int windowbits);

#ifdef __cplusplus
}
#endif

#endif
