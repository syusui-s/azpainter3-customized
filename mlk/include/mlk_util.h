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

#ifndef MLK_UTIL_H
#define MLK_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

void **mAllocArrayBuf(int size,int num);
void **mAllocArrayBuf_align(int size,int alignment,int num);
void mFreeArrayBuf(void **ppbuf,int num);

mlkbool mIsByteOrderLE(void);
uint32_t mRGBAtoHostOrder(uint32_t c);

int mDPMtoDPI(int dpm);
int mDPItoDPM(int dpi);

int mGetOnBitPosL(uint32_t val);
int mGetOffBitPosL(uint32_t val);
int mGetOnBitCount(uint32_t val);

mlkbool mIsChangeFlagState(int type,int current);
mlkbool mGetChangeFlagState(int type,int current,int *ret);

uint32_t mCalcStringHash(const char *str);
int mCalcMaxLog2(uint32_t n);

uint16_t mGetBufBE16(const void *buf);
uint32_t mGetBufBE32(const void *buf);
uint16_t mGetBufLE16(const void *buf);
uint32_t mGetBufLE32(const void *buf);

void mSetBufBE16(uint8_t *buf,uint16_t val);
void mSetBufBE32(uint8_t *buf,uint32_t val);
void mSetBufLE16(uint8_t *buf,uint16_t val);
void mSetBufLE32(uint8_t *buf,uint32_t val);

int mSetBuf_format(void *buf,const char *format,...);
int mGetBuf_format(const void *buf,const char *format,...);

void mCopyBuf_16bit_BEtoHOST(void *dst,const void *src,uint32_t cnt);
void mCopyBuf_32bit_BEtoHOST(void *dst,const void *src,uint32_t cnt);
void mConvBuf_16bit_BEtoHOST(void *buf,uint32_t cnt);
void mConvBuf_32bit_BEtoHOST(void *buf,uint32_t cnt);
void mReverseBit(uint8_t *buf,uint32_t bytes);
void mReverseVal_8bit(uint8_t *buf,uint32_t cnt);
void mReverseVal_16bit(void *buf,uint32_t cnt);
void mSwapByte_16bit(void *buf,uint32_t cnt);
void mSwapByte_32bit(void *buf,uint32_t cnt);

void mAddRecentVal_16bit(void *buf,int num,int addval,int endval);
void mAddRecentVal_32bit(void *buf,int num,uint32_t addval,uint32_t endval);

int mGetBase64EncodeSize(int size);
int mEncodeBase64(void *dst,int dstsize,const void *src,int size);
int mDecodeBase64(void *dst,int dstsize,const char *src,int len);

/* sys */

char *mGetProcessName(void);
mlkbool mExec(const char *cmd);
char *mGetSelfExePath(void);

#ifdef __cplusplus
}
#endif

#endif
