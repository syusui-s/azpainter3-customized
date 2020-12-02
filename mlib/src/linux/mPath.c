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

/*****************************************
 * <Linux> mPath [パス関連]
 *****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mDef.h"


/*****************//**

@defgroup path mPath
@brief ファイルパス関連

@ingroup group_system
@{

*********************/


/** ホームディレクトリパスを取得
 *
 * @return メモリ確保された文字列 */

char *mGetHomePath(void)
{
	char *pc = getenv("HOME");

	if(pc)
		return mStrdup(pc);
	else
		return mStrdup("/");
}

/** デフォルトの作業用ディレクトリパスを取得
 *
 * @return メモリ確保された文字列 */

char *mGetTempPath(void)
{
	return mStrdup("/tmp");
}

/** 各プロセスで異なる作業用ファイル名を取得 */

char *mGetProcessTempName(void)
{
	char m[32];

	snprintf(m, 32, "%d", (int)getpid());

	return mStrdup(m);
}

/** パス区切り文字を取得 */

char mPathGetSplitChar(void)
{
	return '/';
}

/** パスがトップのディレクトリか */

mBool mPathIsTop(const char *path)
{
	return (!path || (path[0] == '/' && path[1] == 0));
}

/** 指定文字がファイル名に使えない文字か */

mBool mPathIsDisableFilenameChar(char c)
{
	return (c == '/');
}

/** ファイル名として正しいか
 *
 * @return ファイル名に使えない文字が含まれていれば FALSE が返る */

mBool mPathIsEnableFilename(const char *filename)
{
	return (filename && !strchr(filename, '/'));
}

/** パス区切り文字の位置を取得
 *
 * @param last 最後に現れた文字位置を取得
 * @return -1 で見つからなかった */

int mPathGetSplitCharPos(const char *path,mBool last)
{
	char *pc;

	if(!path) return -1;

	if(last)
		pc = strrchr(path, '/');
	else
		pc = strchr(path, '/');

	return (pc)? pc - path: -1;
}

/** path の終端にパス区切り文字があればその位置を返す
 *
 * (Linux 時) "/" のみの場合は -1 が返る。
 *
 * @param len 文字長さ (必須)
 * @return -1 でなし */

int mPathGetBottomSplitCharPos(const char *path,int len)
{
	if(len <= 1)
		return -1;
	else
		return (path[len - 1] == '/')? len - 1: -1;
}

/** ２つのパスが同じか比較 */

mBool mPathCompareEq(const char *path1,const char *path2)
{
	return (path1 && path2 && strcmp(path1, path2) == 0);
}

/** @} */
