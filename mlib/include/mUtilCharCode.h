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

#ifndef MLIB_UTIL_CHARCODE_H
#define MLIB_UTIL_CHARCODE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* UTF-8 */

int mUTF8CharWidth(const char *p);
int mUTF8ToUCS4Char(const char *src,int maxlen,uint32_t *dst,const char **ppnext);
int mUTF8ToUCS4(const char *src,int srclen,uint32_t *dst,int dstlen);
uint32_t *mUTF8ToUCS4_alloc(const char *src,int srclen,int *retlen);
int mUTF8ToWide(const char *src,int srclen,wchar_t *dst,int dstlen);
wchar_t *mUTF8ToWide_alloc(const char *src,int srclen,int *retlen);
int mUTF8ToLocal(const char *src,int srclen,char *dst,int dstlen);
char *mUTF8ToLocal_alloc(const char *src,int srclen,int *retlen);

/* WideChar */

int mWideToUTF8(const wchar_t *src,int srclen,char *dst,int dstlen);
char *mWideToUTF8_alloc(const wchar_t *src,int srclen,int *retlen);

/* Locale */

int mLocalToWide(const char *src,int srclen,wchar_t *dst,int dstlen);
wchar_t *mLocalToWide_alloc(const char *src,int srclen,int *retlen);
char *mLocalToUTF8_alloc(const char *src,int srclen,int *retlen);

/* UCS-4 */

int mUCS4Len(const uint32_t *p);
uint32_t *mUCS4StrDup(const uint32_t *src);
int mUCS4ToUTF8Char(uint32_t ucs,char *dst);
int mUCS4ToUTF8(const uint32_t *src,int srclen,char *dst,int dstlen);
char *mUCS4ToUTF8_alloc(const uint32_t *ucs,int srclen,int *retlen);
wchar_t *mUCS4ToWide_alloc(const uint32_t *src,int srclen,int *retlen);
int mUCS4ToFloatInt(const uint32_t *text,int dig);
int mUCS4Compare(const uint32_t *text1,const uint32_t *text2);

/* UTF-16 */

int mUTF16Len(const uint16_t *p);
int mUTF16ToUCS4Char(const uint16_t *src,uint32_t *dst,const uint16_t **ppnext);
int mUTF16ToUTF8(const uint16_t *src,int srclen,char *dst,int dstlen);
char *mUTF16ToUTF8_alloc(const uint16_t *src,int srclen,int *retlen);

#ifdef __cplusplus
}
#endif

#endif
