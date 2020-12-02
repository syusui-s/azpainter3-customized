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

#ifndef MLIB_INIWRITE_H
#define MLIB_INIWRITE_H

#ifdef __cplusplus
extern "C" {
#endif

FILE *mIniWriteOpenFile(const char *filename);
FILE *mIniWriteOpenFile2(const char *path,const char *filename);

void mIniWriteGroup(FILE *fp,const char *name);
void mIniWriteGroup_int(FILE *fp,int no);

void mIniWriteInt(FILE *fp,const char *key,int num);
void mIniWriteNoInt(FILE *fp,int keyno,int num);
void mIniWriteHex(FILE *fp,const char *key,uint32_t num);
void mIniWriteNoHex(FILE *fp,int keyno,uint32_t num);
void mIniWriteText(FILE *fp,const char *key,const char *text);
void mIniWriteNoText(FILE *fp,int keyno,const char *text);
void mIniWriteStr(FILE *fp,const char *key,mStr *str);
void mIniWriteNoStrs(FILE *fp,int keytop,mStr *array,int strnum);

void mIniWritePoint(FILE *fp,const char *key,mPoint *pt);
void mIniWriteSize(FILE *fp,const char *key,mSize *size);
void mIniWriteBox(FILE *fp,const char *key,mBox *box);

void mIniWriteNums(FILE *fp,const char *key,const void *buf,int numcnt,int bytes,mBool hex);
void mIniWriteBase64(FILE *fp,const char *key,const void *buf,int size);

void mIniWriteFileDialogConfig(FILE *fp,const char *key,const void *buf);

#ifdef __cplusplus
}
#endif

#endif
