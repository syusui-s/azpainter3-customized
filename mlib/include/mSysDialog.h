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

#ifndef MLIB_SYSTEMDIALOG_H
#define MLIB_SYSTEMDIALOG_H

typedef struct _mFontInfo mFontInfo;

#define MSYSDLG_OPENFILE_F_MULTI_SEL        1
#define MSYSDLG_SAVEFILE_F_NO_OVERWRITE_MES 2

#define MSYSDLG_INPUTTEXT_F_NOEMPTY  1

#define MSYSDLG_INPUTNUM_F_DEFAULT  1


#ifdef __cplusplus
extern "C" {
#endif

mBool mSysDlgOpenFile(mWindow *owner,const char *filter,int deftype,
	const char *initdir,uint32_t flags,mStr *strdst);

mBool mSysDlgSaveFile(mWindow *owner,const char *filter,int deftype,
	const char *initdir,uint32_t flags,mStr *strdst,int *filtertype);

mBool mSysDlgSelectDir(mWindow *owner,const char *initdir,uint32_t flags,mStr *strdst);

mBool mSysDlgSelectColor(mWindow *owner,mRgbCol *pcol);

mBool mSysDlgSelectFont(mWindow *owner,mFontInfo *info,uint32_t mask);
mBool mSysDlgSelectFont_format(mWindow *owner,mStr *strformat,uint32_t mask);

void mSysDlgAbout(mWindow *owner,const char *label);
void mSysDlgAbout_license(mWindow *owner,const char *copying,const char *license);

mBool mSysDlgInputText(mWindow *owner,
	const char *title,const char *message,mStr *str,uint32_t flags);

mBool mSysDlgInputNum(mWindow *owner,
	const char *title,const char *message,int *dst,int min,int max,uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
