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

#ifndef MLIB_BUFOPERATE_H
#define MLIB_BUFOPERATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mBufOperate
{
	uint8_t *cur,*top;
	int32_t size;
	uint8_t endian;
}mBufOperate;

enum MBUFOPERATE_ENDIAN
{
	MBUFOPERATE_ENDIAN_SYSTEM,
	MBUFOPERATE_ENDIAN_LITTLE,
	MBUFOPERATE_ENDIAN_BIG
};


void mBufOpInit(mBufOperate *p,void *buf,int32_t size,uint8_t endian);

int32_t mBufOpGetRemain(mBufOperate *p);
mBool mBufOpIsRemain(mBufOperate *p,int32_t size);

mBool mBufOpSeek(mBufOperate *p,int32_t size);
mBool mBufOpSetPos(mBufOperate *p,int32_t pos);
int32_t mBufOpSetPosRet(mBufOperate *p,int32_t pos);

mBool mBufOpRead(mBufOperate *p,void *buf,int32_t size);
mBool mBufOpReadByte(mBufOperate *p,void *buf);
mBool mBufOpRead16(mBufOperate *p,void *buf);
mBool mBufOpRead32(mBufOperate *p,void *buf);
uint16_t mBufOpGet16(mBufOperate *p);
uint32_t mBufOpGet32(mBufOperate *p);

void mBufOpWrite(mBufOperate *p,const void *buf,int32_t size);
void mBufOpWrite16(mBufOperate *p,void *buf);
void mBufOpWrite32(mBufOperate *p,void *buf);
void mBufOpSet16(mBufOperate *p,uint16_t val);
void mBufOpSet32(mBufOperate *p,uint32_t val);

#ifdef __cplusplus
}
#endif

#endif
