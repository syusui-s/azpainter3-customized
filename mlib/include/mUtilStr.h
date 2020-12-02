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

#ifndef MLIB_UTIL_STR_H
#define MLIB_UTIL_STR_H

#ifdef __cplusplus
extern "C" {
#endif

void mFreeStrsBuf(char **buf);

char *mStrchr_end(const char *text,char ch);
int mStrcmp_endchar(const char *text1,const char *text2,char ch);
int mStrcmp_number(const char *text1,const char *text2);

mBool mGetStrNextSplit(const char **pptop,const char **ppend,char ch);
int mGetStrLines(const char *text);
char *mGetStrNextLine(char *buf,mBool repnull);

char *mGetSplitCharReplaceStr(const char *text,char split);

void mReverseBuf(char *buf,int len);
void mReplaceStrChar(char *text,char find,char rep);

mBool mIsASCIIString(const char *text);
mBool mIsNumString(const char *text);

int mGetStrVariableLenBytes(const char *text);

void mToLower(char *text);

int mIntToStr(char *buf,int num);
int mFloatIntToStr(char *buf,int num,int dig);
int mIntToStrDig(char *buf,int num,int dig);
int mUCS4StrToFloatInt(const uint32_t *text,int dig);

mBool mIsMatchString(const char *text,const char *pattern,mBool bNoCase);
mBool mIsMatchStringSum(const char *text,const char *pattern,char split,mBool bNoCase);
int mGetEqStringIndex(const char *text,const char *enumtext,char split,mBool bNoCase);

char *mGetFormatStrParam(const char *text,const char *key,int split,int paramsplit,mBool bNoCase);

#ifdef __cplusplus
}
#endif

#endif
