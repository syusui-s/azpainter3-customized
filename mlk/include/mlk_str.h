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

#ifndef MLK_STR_H
#define MLK_STR_H

#ifdef __cplusplus
extern "C" {
#endif

void mStrFree(mStr *str);
void mStrInit(mStr *str);
mlkbool mStrAlloc(mStr *str,int size);
mlkbool mStrResize(mStr *str,int len);

mlkbool mStrIsEmpty(const mStr *str);
mlkbool mStrIsnotEmpty(const mStr *str);
char mStrGetLastChar(const mStr *str);
int mStrGetBytes_tolen(const mStr *str,int len);

void mStrEmpty(mStr *str);
void mStrSetLen(mStr *str,int len);
void mStrSetLen_null(mStr *str);
mlkbool mStrSetLen_bytes(mStr *str,int size);
mlkbool mStrLimitText(mStr *str,int len);
void mStrValidate(mStr *str);

void mStrCopy(mStr *dst,const mStr *src);
void mStrCopy_init(mStr *dst,const mStr *src);
void mStrCopy_alloc(mStr *dst,const mStr *src);

void mStrSetChar(mStr *str,char c);
void mStrSetText(mStr *str,const char *text);
void mStrSetText_len(mStr *str,const char *text,int len);
void mStrSetText_locale(mStr *str,const char *text,int len);
void mStrSetText_utf32(mStr *str,const mlkuchar *text,int len);
void mStrSetText_utf16be(mStr *str,const void *text,int len);
void mStrSetInt(mStr *str,int val);
void mStrSetIntDig(mStr *str,int val,int dig);
void mStrSetDouble(mStr *str,double d,int dig);
void mStrSetFormat(mStr *str,const char *format,...);

void mStrAppendChar(mStr *str,char c);
void mStrAppendUnichar(mStr *str,mlkuchar c);
void mStrAppendText(mStr *str,const char *text);
void mStrAppendText_len(mStr *str,const char *text,int len);
void mStrAppendText_locale(mStr *str,const char *text,int len);
void mStrAppendText_escapeChar(mStr *str,const char *text,const char *lists);
void mStrAppendText_escapeForCmdline(mStr *str,const char *text);
void mStrAppendInt(mStr *str,int val);
void mStrAppendDouble(mStr *str,double d,int dig);
void mStrAppendStr(mStr *dst,mStr *src);
void mStrAppendFormat(mStr *str,const char *format,...);

void mStrPrependText(mStr *str,const char *text);

void mStrAppendEncode_percentEncoding(mStr *str,const char *text,mlkbool space_to_plus);
void mStrSetDecode_percentEncoding(mStr *str,const char *text,mlkbool plus_to_space);
int mStrSetDecode_urilist(mStr *str,const char *text,mlkbool localfile);

void mStrGetMid(mStr *dst,mStr *src,int pos,int len);
void mStrGetSplitText(mStr *dst,const char *text,char split,int index);

void mStrLower(mStr *str);
void mStrUpper(mStr *str);

int mStrToInt(mStr *str);
double mStrToDouble(mStr *str);
int mStrToIntArray(mStr *str,int *dst,int maxnum,char split);
int mStrToIntArray_range(mStr *str,int *dst,int maxnum,char split,int min,int max);

void mStrReplaceChar(mStr *str,char from,char to);
void mStrReplaceChar_null(mStr *str,char ch);
void mStrReplaceSplitText(mStr *str,char split,int index,const char *replace);
void mStrReplaceParams(mStr *str,char paramch,mStr *reparray,int arraynum);
void mStrDecodeEscape(mStr *str);

int mStrFindChar(mStr *str,char ch);
int mStrFindChar_rev(mStr *str,char ch);
void mStrFindChar_toend(mStr *str,char ch);

mlkbool mStrCompareEq(mStr *str,const char *text);
mlkbool mStrCompareEq_case(mStr *str,const char *text);
mlkbool mStrCompareEq_len(mStr *str,const char *text,int len);

/* path */

mlkbool mStrPathIsTop(mStr *str);
void mStrPathAppendDirSep(mStr *str);
void mStrPathJoin(mStr *str,const char *path);
void mStrPathNormalize(mStr *str);

void mStrPathSetHome(mStr *str);
void mStrPathSetHome_join(mStr *str,const char *path);
void mStrPathReplaceHome(mStr *str);
void mStrPathSetTempDir(mStr *str);

void mStrPathRemoveBottomDirSep(mStr *str);
void mStrPathRemoveBasename(mStr *str);
void mStrPathRemoveExt(mStr *str);
void mStrPathReplaceDisableChar(mStr *str,char rep);
void mStrPathGetDir(mStr *dst,const char *path);
void mStrPathGetBasename(mStr *dst,const char *path);
void mStrPathGetBasename_noext(mStr *dst,const char *path);
void mStrPathGetExt(mStr *dst,const char *path);
void mStrPathSplit(mStr *dstdir,mStr *dstname,const char *path);
void mStrPathSplitExt(mStr *dstpath,mStr *dstext,const char *path);
void mStrPathAppendExt(mStr *str,const char *ext);
mlkbool mStrPathSetExt(mStr *str,const char *ext);
void mStrPathCombine(mStr *dst,const char *dir,const char *fname,const char *ext);
void mStrPathCombine_prefix(mStr *dst,const char *dir,const char *prefix,const char *fname,const char *ext);
void mStrPathGetOutputFile(mStr *dst,const char *infile,const char *outdir,const char *outext);
void mStrPathGetOutputFile_suffix(mStr *dst,const char *infile,const char *outdir,const char *outext,const char *suffix);

mlkbool mStrPathCompareEq(mStr *str,const char *path);
mlkbool mStrPathCompareExtEq(mStr *str,const char *ext);
mlkbool mStrPathCompareExts(mStr *str,const char *exts);
mlkbool mStrPathCompareDir(mStr *str,const char *dir);

mlkbool mStrPathExtractMultiFiles(mStr *dst,const char *text,const char **tmp1,const char **tmp2);

/* array */

void mStrArrayFree(mStr *p,int num);
void mStrArrayInit(mStr *p,int num);
void mStrArrayCopy(mStr *dst,mStr *src,int num);
void mStrArrayShiftUp(mStr *p,int start,int end);
void mStrArraySetRecent(mStr *p,int arraynum,int index,const char *text);
void mStrArrayAddRecent(mStr *p,int arraynum,const char *text);

#ifdef __cplusplus
}
#endif

#endif
