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

#ifndef MLK_FILEINPUT_H
#define MLK_FILEINPUT_H

#define MLK_FILEINPUT(p)  ((mFileInput *)(p))
#define MLK_FILEINPUT_DEF MLK_CONTAINER_DEF mFileInputData fi;

typedef struct
{
	mLineEdit *edit;
	mButton *btt;
	mStr str_filter,
		str_initdir;
	int filter_def;
	uint32_t fstyle;
}mFileInputData;

struct _mFileInput
{
	MLK_CONTAINER_DEF
	mFileInputData fi;
};


enum MFILEINPUT_STYLE
{
	MFILEINPUT_S_DIRECTORY = 1<<0,
	MFILEINPUT_S_READ_ONLY = 1<<1
};

enum MFILEINPUT_NOTIFY
{
	MFILEINPUT_N_CHANGE
};


#ifdef __cplusplus
extern "C" {
#endif

mFileInput *mFileInputNew(mWidget *parent,int size,uint32_t fstyle);
mFileInput *mFileInputCreate_file(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,
	const char *filter,int filter_def,const char *initdir);
mFileInput *mFileInputCreate_dir(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,
	const char *path);

void mFileInputDestroy(mWidget *p);
int mFileInputHandle_event(mWidget *wg,mEvent *ev);

void mFileInputSetFilter(mFileInput *p,const char *filter,int def);
void mFileInputSetInitDir(mFileInput *p,const char *dir);
void mFileInputSetPath(mFileInput *p,const char *path);
void mFileInputGetPath(mFileInput *p,mStr *str);

#ifdef __cplusplus
}
#endif

#endif
