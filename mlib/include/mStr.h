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

#ifndef MLIB_STR_H
#define MLIB_STR_H

#include "mStrDef.h"

#ifdef __cplusplus
extern "C" {
#endif

void mStrFree(mStr *str);
mBool mStrAlloc(mStr *str,int size);
void mStrInit(mStr *str);

mBool mStrResize(mStr *str,int len);
void mStrCalcLen(mStr *str);

mBool mStrIsEmpty(mStr *str);
void mStrEmpty(mStr *str);
void mStrSetLen(mStr *str,int len);
void mStrCopy(mStr *dst,mStr *src);
void mStrCopy_init(mStr *dst,mStr *src);
void mStrCopy_alloc(mStr *dst,mStr *src);

int mStrCharLenToByte(mStr *str,int len);
mBool mStrLimitBytes(mStr *str,int bytes);

char mStrGetLastChar(mStr *str);

void mStrSetChar(mStr *str,char c);
void mStrSetText(mStr *str,const char *text);
void mStrSetTextLen(mStr *str,const char *text,int len);
void mStrSetInt(mStr *str,int val);
void mStrSetDouble(mStr *str,double d,int dig);

void mStrSetTextWide(mStr *str,const void *text,int len);
void mStrSetTextLocal(mStr *str,const char *text,int len);
void mStrSetTextUCS4(mStr *str,const uint32_t *text,int len);
void mStrSetFormat(mStr *str,const char *format,...);
void mStrSetPercentEncoding(mStr *str,const char *text);
void mStrDecodePercentEncoding(mStr *str,const char *text);
void mStrDecodePercentEncoding_plus(mStr *str,const char *text);

int mStrSetURIList(mStr *str,const char *uri,mBool localfile);
void mStrSetURLEncode(mStr *str,const char *text);

void mStrAppendChar(mStr *str,char c);
void mStrAppendCharUCS4(mStr *str,uint32_t ucs);
void mStrAppendText(mStr *str,const char *text);
void mStrAppendTextLen(mStr *str,const char *text,int len);
void mStrAppendInt(mStr *str,int val);
void mStrAppendDouble(mStr *str,double d,int dig);
void mStrAppendStr(mStr *dst,mStr *src);
void mStrAppendTextLocal(mStr *str,const char *text,int len);
void mStrAppendFormat(mStr *str,const char *format,...);
void mStrPrependText(mStr *str,const char *text);

void mStrMid(mStr *dst,mStr *src,int pos,int len);
void mStrGetSplitText(mStr *dst,const char *text,char split,int index);

void mStrLower(mStr *str);
void mStrUpper(mStr *str);

int mStrToInt(mStr *str);
double mStrToDouble(mStr *str);
int mStrToIntArray(mStr *str,int *dst,int maxnum,char ch);
int mStrToIntArray_range(mStr *str,int *dst,int maxnum,char ch,int min,int max);

void mStrReplaceChar(mStr *str,char ch,char chnew);
void mStrReplaceSplitText(mStr *str,char split,int index,const char *text);
void mStrReplaceParams(mStr *str,char paramch,mStr *reparray,int arraynum);

int mStrFindChar(mStr *str,char ch);
int mStrFindCharRev(mStr *str,char ch);
void mStrFindCharToEnd(mStr *str,char ch);

mBool mStrCompareEq(mStr *str,const char *text);
mBool mStrCompareCaseEq(mStr *str,const char *text);

void mStrEscapeCmdline(mStr *str,const char *text);

mBool mStrPathIsTop(mStr *str);
void mStrPathSetHomeDir(mStr *str);
void mStrPathSetHomeDir_add(mStr *str,const char *path);
void mStrPathRemoveFileName(mStr *str);
void mStrPathRemoveBottomPathSplit(mStr *str);
void mStrPathReplaceDisableChar(mStr *str,char rep);
void mStrPathGetDir(mStr *dst,const char *path);
void mStrPathGetFileName(mStr *dst,const char *path);
void mStrPathGetFileNameNoExt(mStr *dst,const char *path);
void mStrPathGetExt(mStr *dst,const char *path);
void mStrPathSplitByDir(mStr *dst1,mStr *dst2,const char *path);
void mStrPathSplitByExt(mStr *dst1,mStr *dst2,const char *path);
void mStrPathAdd(mStr *dst,const char *path);
void mStrPathSetExt(mStr *path,const char *ext);
void mStrPathCombine(mStr *dst,const char *dir,const char *fname,const char *ext);
void mStrPathGetOutputFile(mStr *dst,const char *infile,const char *outdir,const char *outext);
mBool mStrPathCompareEq(mStr *str,const char *path);
mBool mStrPathCompareExtEq(mStr *path,const char *ext);
mBool mStrPathCompareExts(mStr *path,const char *exts);
mBool mStrPathCompareDir(mStr *path,const char *dir);
mBool mStrPathExtractMultiFiles(mStr *dst,const char *text,int *param);

/*----*/

void mStrArrayFree(mStr *array,int num);
void mStrArrayInit(mStr *array,int num);
void mStrArrayCopy(mStr *dst,mStr *src,int num);
void mStrArrayShiftUp(mStr *array,int start,int end);
void mStrArraySetRecent(mStr *array,int arraynum,int index,const char *text);
void mStrArrayAddRecent(mStr *array,int arraynum,const char *text,mBool bPath);

#ifdef __cplusplus
}
#endif

#endif
