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

#ifndef MLIB_IOFILEBUF_H
#define MLIB_IOFILEBUF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mIOFileBuf mIOFileBuf;

enum MIOFILEBUF_ENDIAN
{
	MIOFILEBUF_ENDIAN_SYSTEM,
	MIOFILEBUF_ENDIAN_LITTLE,
	MIOFILEBUF_ENDIAN_BIG
};

void mIOFileBuf_close(mIOFileBuf *p);

mIOFileBuf *mIOFileBuf_openRead_filename(const char *filename);
mIOFileBuf *mIOFileBuf_openRead_buf(const void *buf,uint32_t bufsize);
mIOFileBuf *mIOFileBuf_openRead_FILE(void *fp);

void mIOFileBuf_setEndian(mIOFileBuf *p,int type);

int mIOFileBuf_read(mIOFileBuf *p,void *buf,int size);
mBool mIOFileBuf_readEmpty(mIOFileBuf *p,int size);
mBool mIOFileBuf_readSize(mIOFileBuf *p,void *buf,int size);

mBool mIOFileBuf_readByte(mIOFileBuf *p,void *buf);
mBool mIOFileBuf_read16(mIOFileBuf *p,void *buf);
mBool mIOFileBuf_read32(mIOFileBuf *p,void *buf);

void mIOFileBuf_setPos(mIOFileBuf *p,uint32_t pos);
mBool mIOFileBuf_seekCur(mIOFileBuf *p,int seek);

#ifdef __cplusplus
}
#endif

#endif
