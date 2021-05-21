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

#ifndef MLK_ZLIB_H
#define MLK_ZLIB_H

typedef struct _mZlib mZlib;

#define MZLIB_WINDOWBITS_ZLIB  15
#define MZLIB_WINDOWBITS_ZLIB_NO_HEADER -15


#ifdef __cplusplus
extern "C" {
#endif

void mZlibFree(mZlib *p);
void mZlibSetIO_stdio(mZlib *p,void *fp);

mZlib *mZlibEncNew(int bufsize,int level,int windowbits,int memlevel,int strategy);
mZlib *mZlibEncNew_default(int bufsize,int level);
uint32_t mZlibEncGetSize(mZlib *p);
mlkbool mZlibEncReset(mZlib *p);
mlkerr mZlibEncSend(mZlib *p,void *buf,uint32_t size);
mlkerr mZlibEncFinish(mZlib *p);

mZlib *mZlibDecNew(int bufsize,int windowbits);
void mZlibDecSetSize(mZlib *p,uint32_t size);
void mZlibDecReset(mZlib *p);
mlkerr mZlibDecReadOnce(mZlib *p,void *buf,int bufsize,uint32_t insize);
mlkerr mZlibDecRead(mZlib *p,void *buf,int size);
mlkerr mZlibDecFinish(mZlib *p);

#ifdef __cplusplus
}
#endif

#endif
