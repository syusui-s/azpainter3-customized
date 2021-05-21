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

#ifndef MLK_FILELISTVIEW_H
#define MLK_FILELISTVIEW_H

#include "mlk_listview.h"

#define MLK_FILELISTVIEW(p)  ((mFileListView *)(p))
#define MLK_FILELISTVIEW_DEF MLK_LISTVIEW_DEF mFileListViewData flv;

typedef struct
{
	mStr strDir,
		strFilter;
	uint32_t fstyle;
	uint8_t sort_type,
		sort_rev;
}mFileListViewData;

struct _mFileListView
{
	MLK_LISTVIEW_DEF
	mFileListViewData flv;
};

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

enum MFILELISTVIEW_SORTTYPE
{
	MFILELISTVIEW_SORTTYPE_FILENAME,
	MFILELISTVIEW_SORTTYPE_FILESIZE,
	MFILELISTVIEW_SORTTYPE_MODIFY
};


#ifdef __cplusplus
extern "C" {
#endif

mFileListView *mFileListViewNew(mWidget *parent,int size,uint32_t fstyle);

void mFileListViewDestroy(mWidget *p);
int mFileListViewHandle_event(mWidget *wg,mEvent *ev);

void mFileListView_setDirectory(mFileListView *p,const char *path);
void mFileListView_setFilter(mFileListView *p,const char *filter);
void mFileListView_setFilter_ext(mFileListView *p,const char *ext);
void mFileListView_setShowHiddenFiles(mFileListView *p,int type);
void mFileListView_setSortType(mFileListView *p,int type,mlkbool rev);

mlkbool mFileListView_isShowHiddenFiles(mFileListView *p);

void mFileListView_updateList(mFileListView *p);
void mFileListView_changeDir_parent(mFileListView *p);

int mFileListView_getSelectFileName(mFileListView *p,mStr *str,mlkbool fullpath);
int mFileListView_getSelectMultiName(mFileListView *p,mStr *str);

#ifdef __cplusplus
}
#endif

#endif
