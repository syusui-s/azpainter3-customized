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

#ifndef MLIB_MEMAUTO_H
#define MLIB_MEMAUTO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mMemAuto
{
	uint8_t *buf;
	uintptr_t allocsize,curpos,appendsize;
}mMemAuto;


void mMemAutoInit(mMemAuto *p);
void mMemAutoFree(mMemAuto *p);
mBool mMemAutoAlloc(mMemAuto *p,uintptr_t initsize,uintptr_t appendsize);

void mMemAutoReset(mMemAuto *p);
void mMemAutoBack(mMemAuto *p,int size);
uintptr_t mMemAutoGetRemain(mMemAuto *p);
uint8_t *mMemAutoGetBottom(mMemAuto *p);

void mMemAutoCutCurrent(mMemAuto *p);

mBool mMemAutoAppend(mMemAuto *p,void *dat,uintptr_t size);
mBool mMemAutoAppendByte(mMemAuto *p,uint8_t dat);
mBool mMemAutoAppendZero(mMemAuto *p,int size);

#ifdef __cplusplus
}
#endif

#endif
