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
 * <linux> ファイル関連ユーティリティ
 *****************************************/

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mDef.h"

#include "mStr.h"
#include "mFileStat.h"
#include "mUtilCharCode.h"


/**

@addtogroup util_file
@{

*/


//========================
// sub
//========================


/** ファイルから情報取得 */

static mBool _getFileStat(const char *filename,struct stat *dst)
{
	char *name;
	mBool ret;

	name = mUTF8ToLocal_alloc(filename, -1, NULL);
	if(!name) return FALSE;

	ret = (stat(name, dst) == 0);

	mFree(name);

	return ret;
}

/** struct stat から mFileStat にセット */

static void _conv_filestat(mFileStat *dst,struct stat *src)
{
	uint32_t f = 0;

	dst->perm = src->st_mode & 0777;
	dst->size = src->st_size;
	dst->timeAccess = src->st_atime;
	dst->timeModify = src->st_mtime;

	//フラグ

	if(S_ISREG(src->st_mode)) f |= MFILESTAT_F_NORMAL;
	if(S_ISDIR(src->st_mode)) f |= MFILESTAT_F_DIRECTORY;
	if(S_ISLNK(src->st_mode)) f |= MFILESTAT_F_SYMLINK;

	dst->flags = f;
}


//========================
//
//========================


/** 指定ファイルが存在するか
 *
 * @param bDirectory ディレクトリかどうかも判定 */

mBool mIsFileExist(const char *filename,mBool bDirectory)
{
	struct stat st;

	if(!_getFileStat(filename, &st)) return FALSE;

	if(bDirectory)
		return ((st.st_mode & S_IFDIR) != 0);
	else
		return TRUE;
}

/** 指定ファイルの情報取得 */

mBool mGetFileStat(const char *filename,mFileStat *dst)
{
	struct stat st;

	if(!_getFileStat(filename, &st))
		return FALSE;
	else
	{
		_conv_filestat(dst, &st);
		return TRUE;
	}
}

/** ディレクトリ作成 */

mBool mCreateDir(const char *path)
{
	char *name;
	mBool ret;

	name = mUTF8ToLocal_alloc(path, -1, NULL);
	if(!name) return FALSE;

	ret = (mkdir(name, 0755) == 0);

	mFree(name);

	return ret;
}

/** ホームディレクトリ＋指定パスのディレクトリを作成 */

mBool mCreateDirHome(const char *pathadd)
{
	mStr str = MSTR_INIT;
	mBool ret;

	mStrPathSetHomeDir(&str);
	mStrPathAdd(&str, pathadd);

	ret = mCreateDir(str.buf);

	mStrFree(&str);

	return ret;
}

/** ファイルを削除 */

mBool mDeleteFile(const char *filename)
{
	char *name;
	mBool ret;

	name = mUTF8ToLocal_alloc(filename, -1, NULL);
	if(!name) return FALSE;

	ret = (unlink(name) == 0);

	mFree(name);

	return ret;
}

/** ディレクトリを削除
 *
 * 中身が空であること。 */

mBool mDeleteDir(const char *path)
{
	char *name;
	mBool ret;

	name = mUTF8ToLocal_alloc(path, -1, NULL);
	if(!name) return FALSE;

	ret = (rmdir(name) == 0);

	mFree(name);

	return ret;
}

/* @} */
