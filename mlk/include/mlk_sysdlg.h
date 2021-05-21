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

#ifndef MLK_SYSDLG_H
#define MLK_SYSDLG_H

typedef struct _mFontInfo mFontInfo;

enum MMESBOX_BUTTONS
{
	MMESBOX_OK      = 1<<0,
	MMESBOX_CANCEL  = 1<<1,
	MMESBOX_YES     = 1<<2,
	MMESBOX_NO      = 1<<3,
	MMESBOX_SAVE    = 1<<4,
	MMESBOX_NOTSAVE = 1<<5,
	MMESBOX_ABORT   = 1<<6,
	MMESBOX_NOTSHOW = 1<<16,

	MMESBOX_OKCANCEL = MMESBOX_OK | MMESBOX_CANCEL,
	MMESBOX_YESNO = MMESBOX_YES | MMESBOX_NO
};

enum MSYSDLG_INPUTTEXT_FLAGS
{
	MSYSDLG_INPUTTEXT_F_NOT_EMPTY = 1<<0,
	MSYSDLG_INPUTTEXT_F_SET_DEFAULT = 1<<1
};

enum MSYSDLG_FILE_FLAGS
{
	MSYSDLG_FILE_F_MULTI_SEL = 1<<0,
	MSYSDLG_FILE_F_NO_OVERWRITE_MES = 1<<1,
	MSYSDLG_FILE_F_DEFAULT_FILENAME = 1<<2
};

enum MSYSDLF_FONT_FLAGS
{
	MSYSDLG_FONT_F_STYLE = 1<<0,
	MSYSDLG_FONT_F_SIZE = 1<<1,
	MSYSDLG_FONT_F_FILE = 1<<2,
	MSYSDLG_FONT_F_PREVIEW = 1<<3,
	MSYSDLG_FONT_F_EX = 1<<4,

	MSYSDLG_FONT_F_NORMAL = MSYSDLG_FONT_F_STYLE | MSYSDLG_FONT_F_SIZE | MSYSDLG_FONT_F_PREVIEW,
	MSYSDLG_FONT_F_ALL = 0xff
};


#ifdef __cplusplus
extern "C" {
#endif

uint32_t mMessageBox(mWindow *parent,const char *title,const char *message,uint32_t btts,uint32_t defbtt);
void mMessageBoxOK(mWindow *parent,const char *message);
void mMessageBoxErr(mWindow *parent,const char *message);
void mMessageBoxErrTr(mWindow *parent,uint16_t groupid,uint16_t strid);

void mSysDlg_about(mWindow *parent,const char *label);
void mSysDlg_about_license(mWindow *parent,const char *copying,const char *license);

mlkbool mSysDlg_inputText(mWindow *parent,
	const char *title,const char *message,uint32_t flags,mStr *strdst);
mlkbool mSysDlg_inputTextNum(mWindow *parent,
	const char *title,const char *message,uint32_t flags,int min,int max,int *dst);

mlkbool mSysDlg_selectColor(mWindow *parent,mRgbCol *col);

mlkbool mSysDlg_selectFont(mWindow *parent,uint32_t flags,mFontInfo *info);
mlkbool mSysDlg_selectFont_text(mWindow *parent,uint32_t flags,mStr *str);

mlkbool mSysDlg_openFile(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,uint32_t flags,mStr *strdst);

mlkbool mSysDlg_saveFile(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,uint32_t flags,mStr *strdst,int *pfiltertype);

mlkbool mSysDlg_selectDir(mWindow *parent,const char *initdir,uint32_t flags,mStr *strdst);

mlkbool mSysDlg_openFontFile(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,mStr *strdst,int *pindex);

#ifdef __cplusplus
}
#endif

#endif
