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

#ifndef MLK_IO_H
#define MLK_IO_H

typedef struct _mIO mIO;

enum MIO_ENDIAN
{
	MIO_ENDIAN_HOST = 0,
	MIO_ENDIAN_LITTLE,
	MIO_ENDIAN_BIG
};


#ifdef __cplusplus
extern "C" {
#endif

void mIO_close(mIO *p);

mIO *mIO_openReadFile(const char *filename);
mIO *mIO_openReadBuf(const void *buf,mlksize bufsize);
mIO *mIO_openReadFILEptr(void *fp);

void mIO_setEndian(mIO *p,int type);
void *mIO_getFILE(mIO *p);

mlksize mIO_getPos(mIO *p);
int mIO_seekTop(mIO *p,mlksize pos);
int mIO_seekCur(mIO *p,mlkfoff seek);
int mIO_isExistSize(mIO *p,mlksize size);

int mIO_read(mIO *p,void *buf,int32_t size);
int mIO_readOK(mIO *p,void *buf,int32_t size);
int mIO_readSkip(mIO *p,int32_t size);

int mIO_readByte(mIO *p,void *buf);
int mIO_read16(mIO *p,void *buf);
int mIO_read32(mIO *p,void *buf);
int mIO_readFormat(mIO *p,const char *format,...);

#ifdef __cplusplus
}
#endif

#endif
