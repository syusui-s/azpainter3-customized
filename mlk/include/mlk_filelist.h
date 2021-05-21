/*$
 Copyright (C) 2013-2021 Azel.

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

#ifndef MLK_FILELIST_H
#define MLK_FILELIST_H

#include "mlk_filestat.h"

#define MLK_FILELISTITEM(p)  ((mFileListItem *)(p))

typedef int (*mFuncFileListAddItem)(const char *fname,const mFileStat *st,void *param);

typedef struct _mFileListItem
{
	mListItem i;
	mFileStat st;
	char name[1];
}mFileListItem;


#ifdef __cplusplus
extern "C" {
#endif

mlkbool mFileList_create(mList *list,const char *path,mFuncFileListAddItem func,void *param);
void mFileList_sort_name(mList *list);
mFileListItem *mFileList_find_name(mList *list,const char *name);

int mFileList_func_excludeDir(const char *fname,const mFileStat *st,void *param);

#ifdef __cplusplus
}
#endif

#endif
