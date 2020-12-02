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

#ifndef MLIB_FILELISTVIEW_H
#define MLIB_FILELISTVIEW_H

#include "mListView.h"
#include "mStrDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_FILELISTVIEW(p)  ((mFileListView *)(p))

typedef struct
{
	uint32_t style;
	mStr strDir,
		strFilter;
	uint8_t sort_type,sort_up;
}mFileListViewData;

typedef struct _mFileListView
{
	mWidget wg;
	mScrollViewData sv;
	mListViewData lv;
	mFileListViewData flv;
}mFileListView;

enum MFILELISTVIEW_STYLE
{
	MFILELISTVIEW_S_MULTI_SEL = 1<<0,
	MFILELISTVIEW_S_ONLY_DIR  = 1<<1,
	MFILELISTVIEW_S_SHOW_HIDDEN_FILES = 1<<2
};

enum MFILELISTVIEW_NOTIFY
{
	MFILELISTVIEW_N_SELECT_FILE,
	MFILELISTVIEW_N_DBLCLK_FILE,
	MFILELISTVIEW_N_CHANGE_DIR
};

enum MFILELISTVIEW_FILETYPE
{
	MFILELISTVIEW_FILETYPE_NONE,
	MFILELISTVIEW_FILETYPE_FILE,
	MFILELISTVIEW_FILETYPE_DIR
};


void mFileListViewDestroyHandle(mWidget *p);
int mFileListViewEventHandle(mWidget *wg,mEvent *ev);

mFileListView *mFileListViewNew(int size,mWidget *parent,uint32_t style);

void mFileListViewSetDirectory(mFileListView *p,const char *path);
void mFileListViewSetFilter(mFileListView *p,const char *filter);
void mFileListViewSetShowHiddenFiles(mFileListView *p,int type);
void mFileListViewSetSortType(mFileListView *p,int type,mBool up);

void mFileListViewUpdateList(mFileListView *p);
void mFileListViewMoveDir_parent(mFileListView *p);

int mFileListViewGetSelectFileName(mFileListView *p,mStr *str,mBool bFullPath);
int mFileListViewGetSelectMultiName(mFileListView *p,mStr *str);

#ifdef __cplusplus
}
#endif

#endif
