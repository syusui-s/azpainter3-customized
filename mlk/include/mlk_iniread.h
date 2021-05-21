/*$
 Copyright (C) 2013-2021 Azel.

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

#ifndef MLK_INIREAD_H
#define MLK_INIREAD_H

#ifdef __cplusplus
extern "C" {
#endif

mlkerr mIniRead_loadFile(mIniRead **dst,const char *filename);
mlkerr mIniRead_loadFile_join(mIniRead **dst,const char *path,const char *filename);
void mIniRead_end(mIniRead *p);

mlkbool mIniRead_isEmpty(mIniRead *p);
void mIniRead_setEmpty(mIniRead *p);

mlkbool mIniRead_setGroup(mIniRead *p,const char *name);
mlkbool mIniRead_setGroupNo(mIniRead *p,int no);
const char *mIniRead_setGroupTop(mIniRead *p);
const char *mIniRead_setGroupNext(mIniRead *p);

int mIniRead_getGroupItemNum(mIniRead *p);
mlkbool mIniRead_groupIsHaveKey(mIniRead *p,const char *key);

mlkbool mIniRead_getNextItem(mIniRead *p,const char **key,const char **param);
mlkbool mIniRead_getNextItem_keyno_int32(mIniRead *p,int *keyno,void *buf,mlkbool hex);

int mIniRead_getInt(mIniRead *p,const char *key,int def);
uint32_t mIniRead_getHex(mIniRead *p,const char *key,uint32_t def);
mlkbool mIniRead_compareText(mIniRead *p,const char *key,const char *comptxt,mlkbool iscase);

const char *mIniRead_getText(mIniRead *p,const char *key,const char *def);
char *mIniRead_getText_dup(mIniRead *p,const char *key,const char *def);
int mIniRead_getTextBuf(mIniRead *p,const char *key,char *buf,uint32_t size,const char *def);
mlkbool mIniRead_getTextStr(mIniRead *p,const char *key,mStr *str,const char *def);
mlkbool mIniRead_getTextStr_keyno(mIniRead *p,int keyno,mStr *str,const char *def);

void mIniRead_getTextStrArray(mIniRead *p,int keytop,mStr *array,int arraynum);
int mIniRead_getNumbers(mIniRead *p,const char *key,void *buf,int maxnum,int bytes,mlkbool hex);
void *mIniRead_getNumbers_alloc(mIniRead *p,const char *key,int bytes,mlkbool hex,int append_bytes,uint32_t *psize);
mlkbool mIniRead_getPoint(mIniRead *p,const char *key,mPoint *pt,int defx,int defy);
mlkbool mIniRead_getSize(mIniRead *p,const char *key,mSize *size,int defw,int defh);
mlkbool mIniRead_getBox(mIniRead *p,const char *key,mBox *box,int defx,int defy,int defw,int defh);
void *mIniRead_getBase64(mIniRead *p,const char *key,uint32_t *psize);

#ifdef __cplusplus
}
#endif

#endif
