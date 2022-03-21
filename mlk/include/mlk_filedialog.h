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

#ifndef MLK_FILEDIALOG_H
#define MLK_FILEDIALOG_H

#define MLK_FILEDIALOG(p)  ((mFileDialog *)(p))
#define MLK_FILEDIALOG_DEF MLK_DIALOG_DEF mFileDialogData fdlg;

typedef struct
{
	uint32_t fstyle;
	int type;
	mStr *strdst;
	int *dst_filtertype;
	
	char *filter;

	void (*on_selectfile)(mFileDialog *,const char *path);
	void (*on_changetype)(mFileDialog *,int type);
	mlkbool (*on_okcancel)(mFileDialog *,mlkbool is_ok,const char *path);

	mFileListView *flist;
	mLineEdit *edit_dir,
		*edit_name;
	mComboBox *cbtype;
	mWidget *btt_moveup,
		*btt_menu;
}mFileDialogData;

struct _mFileDialog
{
	MLK_DIALOG_DEF
	mFileDialogData fdlg;
};


enum MFILEDIALOG_STYLE
{
	MFILEDIALOG_S_MULTI_SEL = 1<<0,
	MFILEDIALOG_S_NO_OVERWRITE_MES = 1<<1,
	MFILEDIALOG_S_DEFAULT_FILENAME = 1<<2
};

enum MFILEDIALOG_TYPE
{
	MFILEDIALOG_TYPE_OPENFILE,
	MFILEDIALOG_TYPE_SAVEFILE,
	MFILEDIALOG_TYPE_DIR
};


#ifdef __cplusplus
extern "C" {
#endif

mFileDialog *mFileDialogNew(mWindow *parent,int size,uint32_t fstyle,int dlgtype);

void mFileDialogDestroy(mWidget *p);
int mFileDialogHandle_event(mWidget *wg,mEvent *ev);

void mFileDialogInit(mFileDialog *p,const char *filter,int def_filter,const char *initdir,mStr *strdst);
void mFileDialogShow(mFileDialog *p);

#ifdef __cplusplus
}
#endif

#endif
