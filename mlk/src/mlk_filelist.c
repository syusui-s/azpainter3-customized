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
 * mFileList
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_filelist.h"
#include "mlk_dir.h"
#include "mlk_list.h"
#include "mlk_string.h"



/* ファイル名比較関数 */

static int _compfunc_name(mListItem *item1,mListItem *item2,void *param)
{
	return mStringCompare_number(
		MLK_FILELISTITEM(item1)->name,
		MLK_FILELISTITEM(item2)->name);
}

/**@ ファイルリストを作成
 *
 * @d:リストのアイテムをすべて削除して、指定ディレクトリのファイルをリストに追加する。
 *
 * @p:list アイテムが追加されるリスト
 * @p:path ファイルを検索するディレクトリ
 * @p:func リストに追加するかどうかを判定する関数。\
 *  NULL を指定すると、すべて追加する。\
 *  関数の戻り値で正の値を返すと、アイテムを追加する。\
 *  0 で追加しない。負の値で、中止。
 * @p:param 関数に渡すパラメータ値
 *
 * @r:ディレクトリが開けなかった場合、FALSE。 */

mlkbool mFileList_create(mList *list,const char *path,mFuncFileListAddItem func,void *param)
{
	mDir *dir;
	mFileListItem *pi;
	const char *fname;
	mFileStat st;
	int len,ret;

	mListDeleteAll(list);

	//ディレクトリ検索

	dir = mDirOpen(path);
	if(!dir) return FALSE;

	while(mDirNext(dir))
	{
		if(mDirIsSpecName(dir)) continue;

		fname = mDirGetFilename(dir);

		//ステータス

		if(!mDirGetStat(dir, &st)) continue;

		//追加判定

		if(func)
		{
			ret = (func)(fname, &st, param);

			if(ret == 0)
				continue;
			else if(ret < 0)
				break;
		}

		//追加

		len = strlen(fname);

		pi = (mFileListItem *)mListAppendNew(list, sizeof(mFileListItem) + len);
		if(!pi) break;

		pi->st = st;

		memcpy(pi->name, fname, len);
	}

	mDirClose(dir);

	return TRUE;
}

/**@ ファイル名でソート
 *
 * @d:自然な名前順。 */

void mFileList_sort_name(mList *list)
{
	mListSort(list, _compfunc_name, 0);
}

/**@ リスト内をファイル名で検索 */

mFileListItem *mFileList_find_name(mList *list,const char *name)
{
	mFileListItem *pi;

	for(pi = MLK_FILELISTITEM(list->top); pi; pi = MLK_FILELISTITEM(pi->i.next))
	{
		if(strcmp(name, pi->name) == 0)
			return pi;
	}

	return NULL;
}

/**@ 作成時の関数:ディレクトリを除外 */

int mFileList_func_excludeDir(const char *fname,const mFileStat *st,void *param)
{
	return ((st->flags & MFILESTAT_F_DIRECTORY) == 0);
}

