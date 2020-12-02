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
 * <Linux> mDirEntry
 *****************************************/

#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "mDef.h"
#include "mDirEntry.h"

#include "mFileStat.h"
#include "mStr.h"
#include "mUtilFile.h"
#include "mUtilCharCode.h"


//-----------------

struct _mDirEntry
{
	DIR *dir;
	struct dirent *curent;
	mStr strPath,
		strCurName;
};

//-----------------


/***********//**

@defgroup direntry mDirEntry
@brief ディレクトリ内ファイル操作

@ingroup group_system
@{

@file mDirEntry.h

****************/


/** 閉じる */

void mDirEntryClose(mDirEntry *p)
{
	if(p)
	{
		mStrFree(&p->strPath);
		mStrFree(&p->strCurName);
	
		closedir(p->dir);

		mFree(p);
	}
}

/** ディレクトリ開く
 *
 * リンクの場合は、リンク先が開かれる。 */

mDirEntry *mDirEntryOpen(const char *path)
{
	mDirEntry *p;
	char *fname;
	DIR *dir;

	//開く

	fname = mUTF8ToLocal_alloc(path, -1, NULL);
	if(!fname) return NULL;

	dir = opendir(fname);

	mFree(fname);

	if(!dir) return NULL;

	//確保

	p = (mDirEntry *)mMalloc(sizeof(mDirEntry), TRUE);
	if(!p)
	{
		closedir(dir);
		return NULL;
	}

	p->dir = dir;

	mStrSetText(&p->strPath, path);

	return p;
}

/** 次のエントリを取得 */

mBool mDirEntryRead(mDirEntry *p)
{
	struct dirent *ent;

	if(p)
	{
		ent = readdir(p->dir);
		if(ent)
		{
			p->curent = ent;
		
			mStrSetTextLocal(&p->strCurName, ent->d_name, -1);
			return TRUE;
		}
	}
	
	return FALSE;
}

/** 現在のファイル名取得 */

const char *mDirEntryGetFileName(mDirEntry *p)
{
	return p->strCurName.buf;
}

/** 現在のファイル名取得 (mStr) */

void mDirEntryGetFileName_str(mDirEntry *p,mStr *str,mBool bFullPath)
{
	if(!p)
		mStrEmpty(str);
	else if(bFullPath)
	{
		mStrCopy(str, &p->strPath);
		mStrPathAdd(str, p->strCurName.buf);
	}
	else
		mStrCopy(str, &p->strCurName);
}

/** ファイル情報取得 */

mBool mDirEntryGetStat(mDirEntry *p,mFileStat *dst)
{
	mStr str = MSTR_INIT;
	mBool ret;

	mDirEntryGetFileName_str(p, &str, TRUE);

	ret = mGetFileStat(str.buf, dst);

	mStrFree(&str);

	return ret;
}

/** 現在のファイルがディレクトリか */

mBool mDirEntryIsDirectory(mDirEntry *p)
{
	if(p && p->curent)
	{
		/* DIR 構造体の値で判定すると、
		 * リンクファイルの場合にリンク先の種類が判別できないので、stat() を使う。*/
		
		mFileStat st;

		if(mDirEntryGetStat(p, &st))
			return ((st.flags & MFILESTAT_F_DIRECTORY) != 0);
	}

	return FALSE;
}

/** 現在のファイル名が特殊な名前 ("." or "..") か */

mBool mDirEntryIsSpecName(mDirEntry *p)
{
	char *pc;

	if(p && p->strCurName.buf)
	{
		pc = p->strCurName.buf;
	
		if(*pc == '.')
			return (pc[1] == 0 || (pc[1] == '.' && pc[2] == 0));
	}

	return FALSE;
}

/** 現在のファイルが親へのリンクか */

mBool mDirEntryIsToParent(mDirEntry *p)
{
	return (p && p->strCurName.buf && strcmp(p->strCurName.buf, "..") == 0);
}

/** 現在のファイルが隠しファイルか */

mBool mDirEntryIsHiddenFile(mDirEntry *p)
{
	return (p && p->strCurName.buf && p->strCurName.buf[0] == '.');
}

/** 拡張子が指定文字列と同じか (大/小文字区別しない) */

mBool mDirEntryIsEqExt(mDirEntry *p,const char *ext)
{
	return (p && mStrPathCompareExtEq(&p->strCurName, ext));
}

/** @} */
