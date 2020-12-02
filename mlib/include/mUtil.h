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

#ifndef MLIB_UTIL_H
#define MLIB_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

int mDPMtoDPI(int dpm);
int mDPItoDPM(int dpi);

int mGetBitOnPos(uint32_t val);
int mGetBitOffPos(uint32_t val);

mBool mIsChangeState(int type,int current_on);
mBool mGetChangeState(int type,int current_on,int *ret);

uint16_t mGetBuf16BE(const void *buf);
uint32_t mGetBuf32BE(const void *buf);
uint16_t mGetBuf16LE(const void *buf);
uint32_t mGetBuf32LE(const void *buf);
void mSetBuf16BE(uint8_t *buf,uint16_t val);
void mSetBuf32BE(uint8_t *buf,uint32_t val);
void mConvertEndianBuf(void *buf,int endian,const char *pattern);
void mSetBufLE_args(void *buf,const char *format,...);
void mSetBufBE_args(void *buf,const char *format,...);

int mBase64GetEncodeSize(int size);
int mBase64Encode(void *dst,int bufsize,const void *src,int size);
int mBase64Decode(void *dst,int bufsize,const char *src,int len);

#ifdef __cplusplus
}
#endif

#endif
