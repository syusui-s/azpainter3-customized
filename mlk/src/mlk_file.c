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

/*****************************************
 * mFile : ファイル関連操作
 *****************************************/

#include <string.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "mlk.h"
#include "mlk_file.h"
#include "mlk_charset.h"
#include "mlk_filestat.h"



/**@ 閉じる
 *
 * @g:mFile
 * 
 * @r:正常に閉じられた場合、MLKERR_OK。エラーの場合は MLKERR_IO。 */

mlkerr mFileClose(mFile file)
{
	if(file == MFILE_NONE)
		return MLKERR_OK;
	else
	{
		if(close(file) == 0)
			return MLKERR_OK;
		else
			return MLKERR_IO;
	}
}

/**@ 読み込み用に開く
 *
 * @p:file 開いた mFile が格納される
 * @p:filename ファイル名 (UTF-8)
 * @r:OK/OPEN (開くのに失敗)/ALLOC */

mlkerr mFileOpen_read(mFile *file,const char *filename)
{
	char *str;
	int fd;

	str = mUTF8toLocale(filename, -1, NULL);
	if(!str) return MLKERR_ALLOC;
	
	fd = open(str, O_RDONLY);

	mFree(str);

	*file = fd;

	return (fd == MFILE_NONE)? MLKERR_OPEN: MLKERR_OK;
}

/**@ 新規書き込み用に開く
 *
 * @p:perm パーミッション。負の値で 0644。 */

mlkerr mFileOpen_write(mFile *file,const char *filename,int perm)
{
	char *str;
	int fd;

	if(perm < 0) perm = 0644;

	str = mUTF8toLocale(filename, -1, NULL);
	if(!str) return MLKERR_ALLOC;

	fd = open(str, O_CREAT | O_WRONLY | O_TRUNC, perm);

	mFree(str);

	*file = fd;

	return (fd == MFILE_NONE)? MLKERR_OPEN: MLKERR_OK;
}

/**@ 作業ファイルとして開く
 *
 * @d:読み書き、パーミッション 600 で作成される。\
 * すでに存在するファイルの場合はエラー。
 *
 * @r:EXIST(ファイルが存在する)/OPEN(他エラー) */

mlkerr mFileOpen_temp(mFile *file,const char *filename)
{
	char *str;
	int fd;

	str = mUTF8toLocale(filename, -1, NULL);
	if(!str) return MLKERR_ALLOC;

	*file = fd = open(str, O_CREAT | O_RDWR | O_EXCL, 0600);

	//unlink() すると、EEXIST エラーにならない

	mFree(str);

	if(fd == -1)
	{
		if(errno == EEXIST)
			return MLKERR_EXIST;
		else
			return MLKERR_OPEN;
	}

	return MLKERR_OK;
}

/**@ ファイルサイズ取得 */

mlkfoff mFileGetSize(mFile file)
{
	struct stat st;

	if(fstat(file, &st) == 0)
		return st.st_size;
	else
		return 0;
}

/**@ 現在位置を取得
 *
 * @r:-1 でエラー */

mlkfoff mFileGetPos(mFile file)
{
	return lseek(file, 0, SEEK_CUR);
}

/**@ 先頭からの位置をセット
 *
 * @r:失敗時は MLKERR_IO */

mlkerr mFileSetPos(mFile file,mlkfoff pos)
{
	return (lseek(file, pos, SEEK_SET) == -1)? MLKERR_IO: MLKERR_OK;
}

/**@ カレント位置からシーク */

mlkerr mFileSeekCur(mFile file,mlkfoff seek)
{
	return (lseek(file, seek, SEEK_CUR) == -1)? MLKERR_IO: MLKERR_OK;
}

/**@ 終端位置からシーク */

mlkerr mFileSeekEnd(mFile file,mlkfoff seek)
{
	return (lseek(file, seek, SEEK_END) == -1)? MLKERR_IO: MLKERR_OK;
}


//=============================
// 読み込み
//=============================


/**@ 読み込み
 *
 * @r:実際に読み込んだサイズ。\
 * ※ size 以下の場合がある。\
 * ファイル終端時は 0。エラー時は -1。 */

int32_t mFileRead(mFile file,void *buf,int32_t size)
{
	ssize_t ret;

	do
	{
		ret = read(file, buf, size);
	} while(ret == -1 && errno == EINTR);

	return ret;
}

/**@ 指定サイズ分読み込み
 *
 * @d:エラー時を除き、指定サイズ分まで読み込む。
 *
 * @r:MLKERR_OK で、すべて読み込めた。\
 *  エラー時は MLKERR_IO。\
 *  サイズ分を読み込めないまま終端に来た場合は、MLKERR_NEED_MORE。 */

mlkerr mFileRead_full(mFile file,void *buf,int32_t size)
{
	ssize_t ret;
	uint8_t *pd = (uint8_t *)buf;
	int32_t remain = size;

	while(remain > 0)
	{
		ret = read(file, pd, remain);

		if(ret == 0)
			//終端
			break;
		else if(ret == -1)
		{
			//エラー
			if(errno != EINTR)
				return MLKERR_IO;
		}
		else
		{
			pd += ret;
			remain -= ret;
		}
	}

	return (remain == 0)? MLKERR_OK: MLKERR_NEED_MORE;
}


//=============================
// 書き込み
//=============================


/**@ 書き込み
 *
 * @r:実際に書き込んだサイズ。\
 * ※ size 以下の場合がある。\
 * -1 でエラー。 */

int32_t mFileWrite(mFile file,const void *buf,int32_t size)
{
	ssize_t ret;

	do
	{
		ret = write(file, buf, size);
	} while(ret == -1 && errno == EINTR);

	return ret;
}

/**@ 指定サイズ分書き込み
 *
 * @d:エラー時を除き、指定サイズ分が書き込まれるまで繰り返す。
 *
 * @r:MLKERR_OK で、すべて書き込めた。\
 *  MLKERR_IO で、エラー。 */

mlkerr mFileWrite_full(mFile file,const void *buf,int32_t size)
{
	ssize_t ret;
	const uint8_t *pd = (const uint8_t *)buf;
	int32_t remain = size;

	while(remain > 0)
	{
		ret = write(file, pd, remain);

		if(ret == -1)
		{
			if(errno != EINTR)
				return MLKERR_IO;
		}
		else
		{
			pd += ret;
			remain -= ret;
		}
	}

	return MLKERR_OK;
}


//========================
// ファイル操作
//========================


/* ファイルから情報取得 */

static mlkbool _get_filestat(const char *path,struct stat *dst)
{
	char *str;
	mlkbool ret;

	str = mUTF8toLocale(path, -1, NULL);
	if(!str) return FALSE;

	ret = (stat(str, dst) == 0);

	mFree(str);

	return ret;
}

/* struct stat -> mFileStat に変換 */

static void _conv_filestat(mFileStat *dst,struct stat *src)
{
	uint32_t f = 0;

	dst->perm = src->st_mode & 0777;
	dst->size = src->st_size;
	dst->time_access = src->st_atime;
	dst->time_modify = src->st_mtime;

	//フラグ

	if(S_ISREG(src->st_mode)) f |= MFILESTAT_F_NORMAL;
	if(S_ISDIR(src->st_mode)) f |= MFILESTAT_F_DIRECTORY;
	if(S_ISLNK(src->st_mode)) f |= MFILESTAT_F_SYMLINK;

	dst->flags = f;
}


/**@ 指定パスが存在するか
 *
 * @g:ファイル操作 */

mlkbool mIsExistPath(const char *path)
{
	struct stat st;

	return _get_filestat(path, &st);
}

/**@ 指定パスが存在し、通常ファイルかどうか */

mlkbool mIsExistFile(const char *path)
{
	struct stat st;

	if(!_get_filestat(path, &st))
		return FALSE;
	else
		return ((st.st_mode & S_IFREG) != 0);
}

/**@ 指定パスが存在し、ディレクトリかどうか */

mlkbool mIsExistDir(const char *path)
{
	struct stat st;

	if(!_get_filestat(path, &st))
		return FALSE;
	else
		return ((st.st_mode & S_IFDIR) != 0);
}

/**@ 指定パスの情報取得 */

mlkbool mGetFileStat(const char *path,mFileStat *dst)
{
	struct stat st;

	if(!_get_filestat(path, &st))
		return FALSE;
	else
	{
		_conv_filestat(dst, &st);
		return TRUE;
	}
}

/**@ ファイルから更新日時を取得
 *
 * @p:dst ファイルが存在しない場合は、0 が入る
 * @r:取得できたか */

mlkbool mGetFileModifyTime(const char *path,int64_t *dst)
{
	struct stat st;

	if(!_get_filestat(path, &st))
	{
		*dst = 0;
		return FALSE;
	}
	else
	{
		*dst = st.st_mtime;
		return TRUE;
	}
}

/**@ ファイルサイズ取得 */

mlkbool mGetFileSize(const char *path,mlkfoff *dst)
{
	struct stat st;

	if(!_get_filestat(path, &st))
		return FALSE;
	else
	{
		*dst = st.st_size;
		return TRUE;
	}
}

/**@ ディレクトリ作成
 *
 * @p:perm パーミッション (負の値でデフォルト = 0755)
 * @r:MLKERR_OK で成功、MLKERR_EXIST ですでに存在している。それ以外でエラー。 */

mlkerr mCreateDir(const char *path,int perm)
{
	char *str;
	int ret;

	if(perm < 0) perm = 0755;

	str = mUTF8toLocale(path, -1, NULL);
	if(!str) return MLKERR_ALLOC;

	ret = mkdir(str, perm);

	mFree(str);

	//結果

	if(ret == 0)
		return MLKERR_OK;
	else
		return (errno == EEXIST)? MLKERR_EXIST: MLKERR_UNKNOWN;
}

/**@ ディレクトリ作成 (上位の親も)
 *
 * @d:親のディレクトリが作成されていない場合、すべて作成する。
 * @r:MLKERR_OK で、一つでも新たに作成した。\
 *  MLKERR_EXIST で、既にすべて存在している。それ以外はエラー。 */

mlkerr mCreateDir_parents(const char *path,int perm)
{
	char *copy,*pc,*pc2,*pcend;
	mlkerr ret;

	//すでに存在しているか

	if(mIsExistPath(path)) return MLKERR_EXIST;

	//作業用にパスをコピー

	copy = mStrdup(path);
	if(!copy) return MLKERR_ALLOC;

	//先頭から順に

	pc = copy;
	pcend = copy + strlen(copy);

	while(*pc)
	{
		//ディレクトリ名取得

		pc2 = strchr(pc + (*pc == '/'), '/');
		if(pc2)
			*pc2 = 0;
		else
			pc2 = pcend;

		//作成

		ret = mCreateDir(copy, perm);

		if(ret != MLKERR_OK && ret != MLKERR_EXIST)
		{
			mFree(copy);
			return ret;
		}

		//

		if(pc2 == pcend) break;

		*pc2 = '/';
		pc = pc2 + 1;
	}

	mFree(copy);

	return MLKERR_OK;
}

/**@ ファイルを削除
 *
 * @r:FALSE で失敗 */

mlkbool mDeleteFile(const char *path)
{
	char *str;
	mlkbool ret;

	str = mUTF8toLocale(path, -1, NULL);
	if(!str) return FALSE;

	ret = (unlink(str) == 0);

	mFree(str);

	return ret;
}

/**@ ディレクトリを削除
 *
 * @d:{em:中身が空であること。:em} */

mlkbool mDeleteDir(const char *path)
{
	char *str;
	mlkbool ret;

	str = mUTF8toLocale(path, -1, NULL);
	if(!str) return FALSE;

	ret = (rmdir(str) == 0);

	mFree(str);

	return ret;
}

/**@ 2つのファイルの更新日時を比較
 *
 * @d:ファイルが存在しない場合は、日時を 0 とする。
 *
 * @r:-1 で、path2 の方が新しい。0 で等しい。1 で path1 の方が新しい。 */

int mCompareFileModify(const char *path1,const char *path2)
{
	int64_t t1,t2;

	mGetFileModifyTime(path1, &t1);
	mGetFileModifyTime(path2, &t2);

	if(t1 < t2)
		return -1;
	else if(t1 > t2)
		return 1;

	return 0;
}
