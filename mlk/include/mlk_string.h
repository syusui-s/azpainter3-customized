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

#ifndef MLK_STRING_H
#define MLK_STRING_H

#ifdef __cplusplus
extern "C" {
#endif

int mIntToStr(char *buf,int32_t n);
int mIntToStr_dig(char *buf,int32_t n,int dig);
int mIntToStr_float(char *buf,int32_t n,int dig);
int mStrToInt_range(const char *str,int min,int max);

void mStringFreeArray_tonull(char **buf);
void mStringReplace(char *str,char from,char to);
void mStringReverse(char *str,int len);
char *mStringFindChar(const char *str,char ch);
int mStringCompare_null_len(const char *str1,const char *str2,int len2);
int mStringCompare_len(const char *str1,const char *str2,int len1,int len2);
int mStringCompare_tochar(const char *str1,const char *str2,char ch);
int mStringCompare_number(const char *str,const char *str2);
char *mStringDup_replace0(const char *str,char ch);

mlkbool mStringGetNextSplit(const char **pptop,const char **ppend,char ch);
int mStringGetSplitText_atIndex(const char *str,char ch,int index,char **pptop);
char *mStringGetNullSplitText(const char *str,int index);
int mStringGetLineCnt(const char *str);
char *mStringGetNextLine(char *str,mlkbool replace0);
int mStringGetNullSplitNum(const char *str);
int mStringGetSplitInt(const char *str,char ch,int *dst,int maxnum);
int mStringGetSplitTextIndex(const char *target,int len,const char *str,char ch,mlkbool iscase);

mlkbool mStringIsASCII(const char *str);
mlkbool mStringIsNum(const char *str);
mlkbool mStringIsValidFilename(const char *str);
void mStringUpper(char *str);
void mStringLower(char *str);
int mStringGetVariableSize(const char *str);

mlkbool mStringMatch(const char *str,const char *pattern,mlkbool iscase);
int mStringMatch_sum(const char *str,const char *patterns,mlkbool iscase);

#ifdef __cplusplus
}
#endif

#endif
