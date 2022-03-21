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
 * ディレクトリ走査
 *****************************************/

#include <string.h>

#include <sys/types.h>
#include <dirent.h>

#include "mlk.h"
#include "mlk_dir.h"

#include "mlk_str.h"
#include "mlk_file.h"
#include "mlk_filestat.h"
#include "mlk_charset.h"


//-----------------

struct _mDir
{
	DIR *dir;
	struct dirent *curent;
	mStr strPath,	//検索パス
		strCurName;	//現在のファイル名 (UTF-8)
};

//-----------------


/**@ 閉じる */

void mDirClose(mDir *p)
{
	if(p)
	{
		mStrFree(&p->strPath);
		mStrFree(&p->strCurName);
	
		closedir(p->dir);

		mFree(p);
	}
}

/**@ ディレクトリを開く
 *
 * @d:パスがリンクの場合は、リンク先が開かれる。
 * @r:NULL でエラー */

mDir *mDirOpen(const char *path)
{
	mDir *p;
	char *str;
	DIR *dir;

	//開く

	str = mUTF8toLocale(path, -1, NULL);
	if(!str) return NULL;

	dir = opendir(str);

	mFree(str);

	if(!dir) return NULL;

	//確保

	p = (mDir *)mMalloc0(sizeof(mDir));
	if(!p)
	{
		closedir(dir);
		return NULL;
	}

	p->dir = dir;

	mStrSetText(&p->strPath, path);

	return p;
}

/**@ 次のエントリを取得
 *
 * @r:FALSE で終了 */

mlkbool mDirNext(mDir *p)
{
	struct dirent *ent;

	if(p)
	{
		ent = readdir(p->dir);
		if(ent)
		{
			p->curent = ent;
		
			mStrSetText_locale(&p->strCurName, ent->d_name, -1);
			return TRUE;
		}
	}
	
	return FALSE;
}

/**@ 現在のファイル名を取得
 *
 * @r:ポインタは解放しないこと */

const char *mDirGetFilename(mDir *p)
{
	return p->strCurName.buf;
}

/**@ 現在のファイル名を mStr に取得
 *
 * @p:fullpath TRUE で、フルパス名で取得 */

void mDirGetFilename_str(mDir *p,mStr *str,mlkbool fullpath)
{
	if(!p)
		mStrEmpty(str);
	else if(fullpath)
	{
		//フルパス
		mStrCopy(str, &p->strPath);
		mStrPathJoin(str, p->strCurName.buf);
	}
	else
		mStrCopy(str, &p->strCurName);
}

/**@ ファイル情報取得 */

mlkbool mDirGetStat(mDir *p,mFileStat *dst)
{
	mStr str = MSTR_INIT;
	mlkbool ret;

	mDirGetFilename_str(p, &str, TRUE);

	ret = mGetFileStat(str.buf, dst);

	mStrFree(&str);

	return ret;
}

/**@ 現在のファイルがディレクトリか
 *
 * @d:ファイル情報を取得して判定する。 */

mlkbool mDirIsDirectory(mDir *p)
{
	if(p && p->curent)
	{
		/* DIR 構造体の値で判定すると、
		 * リンクファイルであった場合は、リンク先の種類が判別できないので、
		 * ファイル情報を取得する。*/
		
		mFileStat st;

		if(mDirGetStat(p, &st))
			return ((st.flags & MFILESTAT_F_DIRECTORY) != 0);
	}

	return FALSE;
}

/**@ 現在のファイルが特殊な名前 ("." or "..") か */

mlkbool mDirIsSpecName(mDir *p)
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

/**@ 現在のファイルが親へのリンク ("..") か */

mlkbool mDirIsSpecName_parent(mDir *p)
{
	return (p && p->strCurName.buf && strcmp(p->strCurName.buf, "..") == 0);
}

/**@ 現在のファイルが隠しファイルか
 *
 * @d:先頭が '.' なら隠しファイル。 */

mlkbool mDirIsHiddenFile(mDir *p)
{
	return (p && p->strCurName.buf && p->strCurName.buf[0] == '.');
}

/**@ 現在のファイルの拡張子を比較 (大/小文字区別しない) */

mlkbool mDirCompareExt(mDir *p,const char *ext)
{
	return (p && mStrPathCompareExtEq(&p->strCurName, ext));
}
