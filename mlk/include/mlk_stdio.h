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

#ifndef MLK_STDIO_H
#define MLK_STDIO_H

#ifdef __cplusplus
extern "C" {
#endif

FILE *mFILEopen(const char *filename,const char *mode);
void mFILEclose(FILE *fp);
void mFILEclose_null(FILE **fp);

mlkfoff mFILEgetSize(FILE *fp);

int mFILEreadOK(FILE *fp,void *buf,int32_t size);
int mFILEreadByte(FILE *fp,void *buf);
int mFILEreadLE16(FILE *fp,void *buf);
int mFILEreadLE32(FILE *fp,void *buf);
int mFILEreadBE16(FILE *fp,void *buf);
int mFILEreadBE32(FILE *fp,void *buf);
uint16_t mFILEgetLE16(FILE *fp);
uint32_t mFILEgetLE32(FILE *fp);

int mFILEreadStr_compare(FILE *fp,const char *cmp);
int mFILEreadStr_variable(FILE *fp,char **ppdst);
int mFILEreadStr_lenBE16(FILE *fp,char **ppdst);

int mFILEreadArrayLE16(FILE *fp,void *buf,int num);
int mFILEreadArrayBE16(FILE *fp,void *buf,int num);
int mFILEreadArrayBE32(FILE *fp,void *buf,int num);

int mFILEreadFormatLE(FILE *fp,const char *format,...);
int mFILEreadFormatBE(FILE *fp,const char *format,...);

int mFILEwriteOK(FILE *fp,const void *buf,int32_t size);
int mFILEwriteByte(FILE *fp,uint8_t val);
int mFILEwriteLE16(FILE *fp,uint16_t val);
int mFILEwriteLE32(FILE *fp,uint32_t val);
int mFILEwriteBE16(FILE *fp,uint16_t val);
int mFILEwriteBE32(FILE *fp,uint32_t val);
int mFILEwrite0(FILE *fp,int size);

int mFILEwriteStr_variable(FILE *fp,const char *text,int len);
int mFILEwriteStr_lenBE16(FILE *fp,const char *text,int len);

void mFILEwriteArrayBE16(FILE *fp,void *buf,int num);
void mFILEwriteArrayBE32(FILE *fp,void *buf,int num);

#ifdef __cplusplus
}
#endif

#endif
