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

#ifndef MLK_INIWRITE_H
#define MLK_INIWRITE_H

#ifdef __cplusplus
extern "C" {
#endif

FILE *mIniWrite_openFile(const char *filename);
FILE *mIniWrite_openFile_join(const char *path,const char *filename);

void mIniWrite_putGroup(FILE *fp,const char *name);
void mIniWrite_putGroupInt(FILE *fp,int no);

void mIniWrite_putInt(FILE *fp,const char *key,int n);
void mIniWrite_putInt_keyno(FILE *fp,int keyno,int n);
void mIniWrite_putHex(FILE *fp,const char *key,uint32_t n);
void mIniWrite_putHex_keyno(FILE *fp,int keyno,uint32_t n);
void mIniWrite_putText(FILE *fp,const char *key,const char *text);
void mIniWrite_putText_keyno(FILE *fp,int keyno,const char *text);
void mIniWrite_putStr(FILE *fp,const char *key,mStr *str);
void mIniWrite_putStrArray(FILE *fp,int keytop,mStr *array,int num);

void mIniWrite_putPoint(FILE *fp,const char *key,mPoint *pt);
void mIniWrite_putSize(FILE *fp,const char *key,mSize *size);
void mIniWrite_putBox(FILE *fp,const char *key,mBox *box);

void mIniWrite_putNumbers(FILE *fp,const char *key,const void *buf,int num,int bytes,mlkbool hex);
void mIniWrite_putBase64(FILE *fp,const char *key,const void *buf,uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
