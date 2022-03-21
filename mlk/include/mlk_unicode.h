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

#ifndef MLK_UNICODE_H
#define MLK_UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

int mUTF8GetCharBytes(const char *p);
int mUTF8GetChar(mlkuchar *dst,const char *src,int maxlen,const char **ppnext);

int mUTF8Validate(char *str,int len);
int mUTF8CopyValidate(char *dst,const char *src,int len);
int mUTF8toUTF16(const char *src,int srclen,mlkuchar16 *dst,int dstlen);
int mUTF8toUTF32(const char *src,int srclen,mlkuchar *dst,int dstlen);
mlkuchar *mUTF8toUTF32_alloc(const char *src,int srclen,int *dstlen);

int mUnicharToUTF8(mlkuchar c,char *dst,int maxlen);
int mUnicharToUTF16(mlkuchar c,mlkuchar16 *dst,int maxlen);

int mUTF32GetLen(const mlkuchar *p);
mlkuchar *mUTF32Dup(const mlkuchar *src);
int mUTF32toUTF8(const mlkuchar *src,int srclen,char *dst,int dstlen);
char *mUTF32toUTF8_alloc(const mlkuchar *src,int srclen,int *dstlen);
int mUTF32toInt_float(const mlkuchar *str,int dig);
int mUTF32Compare(const mlkuchar *str1,const uint32_t *str2);

int mUTF16GetLen(const mlkuchar16 *p);
int mUTF16GetChar(mlkuchar *dst,const mlkuchar16 *src,const mlkuchar16 **ppnext);
int mUTF16toUTF8(const mlkuchar16 *src,int srclen,char *dst,int dstlen);
char *mUTF16toUTF8_alloc(const mlkuchar16 *src,int srclen,int *dstlen);

#ifdef __cplusplus
}
#endif

#endif
