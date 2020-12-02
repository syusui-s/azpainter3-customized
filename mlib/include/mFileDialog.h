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

#ifndef MLIB_FILEDIALOG_H
#define MLIB_FILEDIALOG_H

#include "mDialog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_FILEDIALOG(p)  ((mFileDialog *)(p))

typedef struct _mFileDialog mFileDialog;
typedef struct _mFileListView mFileListView;
typedef struct _mLineEdit mLineEdit;
typedef struct _mComboBox mComboBox;

typedef struct
{
	uint32_t style;
	int type;
	mStr *strdst;
	int *filtertypeDst;
	
	char *filter;

	void (*onSelectFile)(mFileDialog *,const char *filename);
	void (*onChangeType)(mFileDialog *,int type);
	mBool (*onOkCancel)(mFileDialog *,mBool bOK,const char *path);

	mFileListView *flist;
	mLineEdit *editdir,
		*editname;
	mComboBox *cbtype;
}mFileDialogData;

struct _mFileDialog
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
	mFileDialogData fdlg;
};


enum MFILEDIALOG_STYLE
{
	MFILEDIALOG_S_MULTI_SEL = 1<<0,
	MFILEDIALOG_S_NO_OVERWRITE_MES = 1<<1
};

enum MFILEDIALOG_TYPE
{
	MFILEDIALOG_TYPE_OPENFILE,
	MFILEDIALOG_TYPE_SAVEFILE,
	MFILEDIALOG_TYPE_DIR
};


void mFileDialogDestroyHandle(mWidget *p);
int mFileDialogEventHandle(mWidget *wg,mEvent *ev);

mFileDialog *mFileDialogNew(int size,mWindow *owner,uint32_t style,int type);

void mFileDialogInit(mFileDialog *p,const char *filter,int deftype,
	const char *initdir,mStr *strdst);

void mFileDialogShow(mFileDialog *p);

#ifdef __cplusplus
}
#endif

#endif
