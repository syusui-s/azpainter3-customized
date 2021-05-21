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

#ifndef MLK_MULTIEDIT_H
#define MLK_MULTIEDIT_H

#include "mlk_scrollview.h"

#define MLK_MULTIEDIT(p)  ((mMultiEdit *)(p))
#define MLK_MULTIEDIT_DEF MLK_SCROLLVIEW_DEF mMultiEditData me;

typedef struct
{
	mWidgetTextEdit edit;
	uint32_t fstyle;
}mMultiEditData;

typedef struct _mMultiEdit
{
	MLK_SCROLLVIEW_DEF
	mMultiEditData me;
}mMultiEdit;


enum MMULTIEDIT_STYLE
{
	MMULTIEDIT_S_READ_ONLY = 1<<0,
	MMULTIEDIT_S_NOTIFY_CHANGE = 1<<1
};

enum MMULTIEDIT_NOTIFY
{
	MMULTIEDIT_N_CHANGE,
	MMULTIEDIT_N_R_CLICK
};


#ifdef __cplusplus
extern "C" {
#endif

mMultiEdit *mMultiEditNew(mWidget *parent,int size,uint32_t fstyle);
mMultiEdit *mMultiEditCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mMultiEditDestroy(mWidget *p);
int mMultiEditHandle_event(mWidget *wg,mEvent *ev);

void mMultiEditSetText(mMultiEdit *p,const char *text);
void mMultiEditGetTextStr(mMultiEdit *p,mStr *str);
int mMultiEditGetTextLen(mMultiEdit *p);
void mMultiEditSelectAll(mMultiEdit *p);

void mMultiEditInsertText(mMultiEdit *p,const char *text);
mlkbool mMultiEditCopyText_full(mMultiEdit *p);
mlkbool mMultiEditPaste_full(mMultiEdit *p);

#ifdef __cplusplus
}
#endif

#endif
