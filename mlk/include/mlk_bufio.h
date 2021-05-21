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

#ifndef MLK_BUFIO_H
#define MLK_BUFIO_H

struct _mBufIO
{
	uint8_t *cur,*top;
	mlksize bufsize;
	int endian;
};

enum MBUFIO_ENDIAN
{
	MBUFIO_ENDIAN_HOST = 0,
	MBUFIO_ENDIAN_LITTLE,
	MBUFIO_ENDIAN_BIG
};


#ifdef __cplusplus
extern "C" {
#endif

void mBufIO_init(mBufIO *p,void *buf,mlksize bufsize,int endian);

mlksize mBufIO_getPos(mBufIO *p);
mlksize mBufIO_getRemain(mBufIO *p);
int mBufIO_isRemain(mBufIO *p,mlksize size);

int mBufIO_seek(mBufIO *p,int32_t size);
int mBufIO_setPos(mBufIO *p,mlksize pos);
int mBufIO_setPos_get(mBufIO *p,mlksize pos,mlksize *plast);

int32_t mBufIO_read(mBufIO *p,void *buf,int32_t size);
int mBufIO_readOK(mBufIO *p,void *buf,int32_t size);
int mBufIO_readByte(mBufIO *p,void *buf);
int mBufIO_read16(mBufIO *p,void *buf);
int mBufIO_read32(mBufIO *p,void *buf);
int mBufIO_read16_array(mBufIO *p,void *buf,int cnt);
int mBufIO_read32_array(mBufIO *p,void *buf,int cnt);
uint16_t mBufIO_get16(mBufIO *p);
uint32_t mBufIO_get32(mBufIO *p);

int32_t mBufIO_write(mBufIO *p,const void *buf,int32_t size);
int mBufIO_write16(mBufIO *p,void *buf);
int mBufIO_write32(mBufIO *p,void *buf);
int mBufIO_set16(mBufIO *p,uint16_t val);
int mBufIO_set32(mBufIO *p,uint32_t val);

#ifdef __cplusplus
}
#endif

#endif
