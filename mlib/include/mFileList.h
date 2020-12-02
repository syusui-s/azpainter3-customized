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

#ifndef MLIB_FILELIST_H
#define MLIB_FILELIST_H

#include "mFileStat.h"
#include "mList.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_FILELISTITEM(p)  ((mFileListItem *)(p))

typedef struct _mFileListItem
{
	mListItem i;
	char *fname;
	uint32_t flags;
}mFileListItem;


mBool mFileListGetList(mList *list,const char *path,int (*func)(const char *,mFileStat *));

void mFileListSortName(mList *list);

mFileListItem *mFileListFindByName(mList *list,const char *name);

int mFileListFunc_notdir(const char *fname,mFileStat *st);

#ifdef __cplusplus
}
#endif

#endif
