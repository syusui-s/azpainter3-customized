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

#ifndef MLK_TEXTPARAM_H
#define MLK_TEXTPARAM_H

typedef struct _mTextParam mTextParam;

#ifdef __cplusplus
extern "C" {
#endif

mTextParam *mTextParam_new(const char *text,int splitpair,int splitparam);
void mTextParam_free(mTextParam *p);

mlkbool mTextParam_getInt(mTextParam *p,const char *key,int *dst);
mlkbool mTextParam_getIntRange(mTextParam *p,const char *key,int *dst,int min,int max);
mlkbool mTextParam_getDouble(mTextParam *p,const char *key,double *dst);
mlkbool mTextParam_getDoubleInt(mTextParam *p,const char *key,int *dst,int dig);
mlkbool mTextParam_getDoubleIntRange(mTextParam *p,const char *key,int *dst,int dig,int min,int max);
mlkbool mTextParam_getTextRaw(mTextParam *p,const char *key,char **dst);
mlkbool mTextParam_getTextDup(mTextParam *p,const char *key,char **buf);
mlkbool mTextParam_getTextStr(mTextParam *p,const char *key,mStr *str);
int mTextParam_findWord(mTextParam *p,const char *key,const char *wordlist,mlkbool iscase);

#ifdef __cplusplus
}
#endif

#endif
