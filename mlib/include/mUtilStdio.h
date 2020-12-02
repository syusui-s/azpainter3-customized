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

#ifndef MLIB_UTIL_STDIO_H
#define MLIB_UTIL_STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

FILE *mFILEopenUTF8(const char *filename,const char *mode);

mBool mFILEreadByte(FILE *fp,void *buf);
mBool mFILEread16LE(FILE *fp,void *buf);
mBool mFILEread32LE(FILE *fp,void *buf);
mBool mFILEread16BE(FILE *fp,void *buf);
mBool mFILEread32BE(FILE *fp,void *buf);
uint16_t mFILEget16LE(FILE *fp);
uint32_t mFILEget32LE(FILE *fp);
mBool mFILEreadCompareStr(FILE *fp,const char *text);
int mFILEreadStr_variableLen(FILE *fp,char **ppbuf);
int mFILEreadStr_len16BE(FILE *fp,char **ppbuf);
int mFILEreadArray16BE(FILE *fp,void *buf,int num);
int mFILEreadArray32BE(FILE *fp,void *buf,int num);
mBool mFILEreadArgsLE(FILE *fp,const char *format,...);
mBool mFILEreadArgsBE(FILE *fp,const char *format,...);

void mFILEwriteByte(FILE *fp,uint8_t val);
void mFILEwrite16LE(FILE *fp,uint16_t val);
void mFILEwrite32LE(FILE *fp,uint32_t val);
void mFILEwrite16BE(FILE *fp,uint16_t val);
void mFILEwrite32BE(FILE *fp,uint32_t val);
void mFILEwriteZero(FILE *fp,int size);
int mFILEwriteStr_variableLen(FILE *fp,const char *text,int len);
int mFILEwriteStr_len16BE(FILE *fp,const char *text,int len);
void mFILEwriteArray16BE(FILE *fp,void *buf,int num);
void mFILEwriteArray32BE(FILE *fp,void *buf,int num);

#ifdef __cplusplus
}
#endif

#endif
