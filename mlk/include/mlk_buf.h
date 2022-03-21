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

#ifndef MLK_BUF_H
#define MLK_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

void mBufInit(mBuf *p);
void mBufFree(mBuf *p);
mlkbool mBufAlloc(mBuf *p,mlksize allocsize,mlksize expand_size);

void mBufReset(mBuf *p);
void mBufSetCurrent(mBuf *p,mlksize pos);
void mBufBack(mBuf *p,int size);
mlksize mBufGetRemain(mBuf *p);
uint8_t *mBufGetCurrent(mBuf *p);

void mBufCutCurrent(mBuf *p);

mlkbool mBufAppend(mBuf *p,const void *buf,int size);
void *mBufAppend_getptr(mBuf *p,const void *buf,int size);
mlkbool mBufAppendByte(mBuf *p,uint8_t dat);
mlkbool mBufAppend0(mBuf *p,int size);
int32_t mBufAppendUTF8(mBuf *p,const char *buf,int len);

void *mBufCopyToBuf(mBuf *p);

#ifdef __cplusplus
}
#endif

#endif
