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

#ifndef MLIB_INIREAD_H
#define MLIB_INIREAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mIniRead mIniRead;

void mIniReadEnd(mIniRead *p);
mIniRead *mIniReadLoadFile(const char *filename);
mIniRead *mIniReadLoadFile2(const char *path,const char *filename);

void mIniReadEmpty(mIniRead *p);

mBool mIniReadSetGroup(mIniRead *p,const char *name);
mBool mIniReadSetGroup_int(mIniRead *p,int no);
const char *mIniReadSetFirstGroup(mIniRead *p);
const char *mIniReadSetNextGroup(mIniRead *p);
int mIniReadGetGroupItemNum(mIniRead *p);
mBool mIniReadIsHaveKey(mIniRead *p,const char *key);

mBool mIniReadGetNextItem(mIniRead *p,const char **key,const char **param);
mBool mIniReadGetNextItem_nonum32(mIniRead *p,int *keyno,void *buf,mBool hex);

int mIniReadInt(mIniRead *p,const char *key,int def);
uint32_t mIniReadHex(mIniRead *p,const char *key,uint32_t def);
const char *mIniReadText(mIniRead *p,const char *key,const char *def);
int mIniReadTextBuf(mIniRead *p,const char *key,char *buf,int size,const char *def);
mBool mIniReadStr(mIniRead *p,const char *key,mStr *str,const char *def);
mBool mIniReadNoStr(mIniRead *p,int keyno,mStr *str,const char *def);
void mIniReadNoStrs(mIniRead *p,int keytop,mStr *array,int maxnum);
int mIniReadNums(mIniRead *p,const char *key,void *buf,int bufnum,int bytes,mBool hex);
void *mIniReadNums_alloc(mIniRead *p,const char *key,int bytes,mBool hex,int append_bytes,int *psize);
mBool mIniReadPoint(mIniRead *p,const char *key,mPoint *pt,int defx,int defy);
mBool mIniReadSize(mIniRead *p,const char *key,mSize *size,int defw,int defh);
mBool mIniReadBox(mIniRead *p,const char *key,mBox *box,int defx,int defy,int defw,int defh);
void *mIniReadBase64(mIniRead *p,const char *key,int *psize);
mBool mIniReadFileDialogConfig(mIniRead *p,const char *key,void *val);

#ifdef __cplusplus
}
#endif

#endif
