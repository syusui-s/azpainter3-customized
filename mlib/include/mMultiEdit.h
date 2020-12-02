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

#ifndef MLIB_MULTIEDIT_H
#define MLIB_MULTIEDIT_H

#include "mScrollView.h"

#define M_MULTIEDIT(p)  ((mMultiEdit *)(p))

typedef struct _mEditTextBuf mEditTextBuf;
typedef struct _mStr mStr;


typedef struct
{
	mEditTextBuf *buf;
	uint32_t style;
}mMultiEditData;

typedef struct _mMultiEdit
{
	mWidget wg;
	mScrollViewData sv;
	mMultiEditData me;
}mMultiEdit;


enum MMULTIEDIT_STYLE
{
	MMULTIEDIT_S_READONLY = 1<<0,
	MMULTIEDIT_S_NOTIFY_CHANGE = 1<<1
};

enum MMULTIEDIT_NOTIFY
{
	MMULTIEDIT_N_CHANGE
};


#ifdef __cplusplus
extern "C" {
#endif

void mMultiEditDestroyHandle(mWidget *p);
int mMultiEditEventHandle(mWidget *wg,mEvent *ev);

mMultiEdit *mMultiEditNew(int size,mWidget *parent,uint32_t style);
mMultiEdit *mMultiEditCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

void mMultiEditSetText(mMultiEdit *p,const char *text);
void mMultiEditGetTextStr(mMultiEdit *p,mStr *str);
int mMultiEditGetTextLen(mMultiEdit *p);
void mMultiEditSelectAll(mMultiEdit *p);

#ifdef __cplusplus
}
#endif

#endif
