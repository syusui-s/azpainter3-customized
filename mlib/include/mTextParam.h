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

#ifndef MLIB_TEXTPARAM_H
#define MLIB_TEXTPARAM_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mTextParam mTextParam;

void mTextParamFree(mTextParam *p);
mTextParam *mTextParamCreate(const char *text,int split,int splitparam);

mBool mTextParamGetInt(mTextParam *p,const char *key,int *dst);
mBool mTextParamGetInt_range(mTextParam *p,const char *key,int *dst,int min,int max);
mBool mTextParamGetDouble(mTextParam *p,const char *key,double *dst);
mBool mTextParamGetDoubleInt(mTextParam *p,const char *key,int *dst,int dig);
mBool mTextParamGetDoubleInt_range(mTextParam *p,const char *key,int *dst,int dig,int min,int max);
mBool mTextParamGetText_raw(mTextParam *p,const char *key,char **dst);
mBool mTextParamGetText_dup(mTextParam *p,const char *key,char **buf);
mBool mTextParamGetStr(mTextParam *p,const char *key,mStr *str);
int mTextParamFindText(mTextParam *p,const char *key,const char *words,mBool bNoCase);

#ifdef __cplusplus
}
#endif

#endif
