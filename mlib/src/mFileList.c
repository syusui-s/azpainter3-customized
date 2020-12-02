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
 * mFileList
 *****************************************/

#include <string.h>

#include "mDef.h"

#include "mFileList.h"

#include "mDirEntry.h"
#include "mPath.h"


/**
@defgroup filelist mFileList
@brief ファイルリスト

ディレクトリ内のファイルを列挙して、mList のリストに作成。

@ingroup group_etc
 
@{
@file mFileList.h
@struct mFileListItem

@def M_FILELISTITEM
(mFileListItem *) に型変換
*/


/** ファイル名比較関数 */

static int _comp_name(mListItem *pitem1,mListItem *pitem2,intptr_t param)
{
	return strcmp(M_FILELISTITEM(pitem1)->fname,
				M_FILELISTITEM(pitem2)->fname);
}

/** アイテム破棄ハンドラ */

static void _item_destroy(mListItem *p)
{
	mFree(M_FILELISTITEM(p)->fname);
}

/** ファイルリスト作成
 *
 * @param func リストに追加するかどうかを判定する関数。0 で返すと追加しない。NULL ですべて。 */

mBool mFileListGetList(mList *list,const char *path,int (*func)(const char *,mFileStat *))
{
	mDirEntry *dir;
	mFileListItem *pi;
	mFileStat st;
	const char *fname;

	mListDeleteAll(list);

	//

	dir = mDirEntryOpen(path);
	if(!dir) return FALSE;

	while(mDirEntryRead(dir))
	{
		if(mDirEntryIsSpecName(dir)) continue;

		fname = mDirEntryGetFileName(dir);

		//ステータス

		if(!mDirEntryGetStat(dir, &st)) continue;

		//追加判定

		if(func && (func)(fname, &st) == 0) continue;

		//追加

		pi = (mFileListItem *)mListAppendNew(list, sizeof(mFileListItem), _item_destroy);
		if(!pi) break;

		pi->fname = mStrdup(fname);
		pi->flags = st.flags;
	}

	mDirEntryClose(dir);

	return TRUE;
}

/** ファイル名でソート */

void mFileListSortName(mList *list)
{
	mListSort(list, _comp_name, 0);
}

/** ファイル名から検索 */

mFileListItem *mFileListFindByName(mList *list,const char *name)
{
	mFileListItem *pi;

	for(pi = M_FILELISTITEM(list->top); pi; pi = M_FILELISTITEM(pi->i.next))
	{
		if(mPathCompareEq(name, pi->fname))
			return pi;
	}

	return pi;
}

/** ファイル追加判定関数 : ディレクトリ以外を対象にする */

int mFileListFunc_notdir(const char *fname,mFileStat *st)
{
	return !(st->flags & MFILESTAT_F_DIRECTORY);
}

/** @} */
