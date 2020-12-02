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

#ifndef MLIB_DIALOG_H
#define MLIB_DIALOG_H

#include "mWindowDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_DIALOG(p)  ((mDialog *)(p))

typedef struct
{
	intptr_t retval;
}mDialogData;

typedef struct _mDialog
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mDialogData dlg;
}mDialog;


int mDialogEventHandle(mWidget *wg,mEvent *ev);

mWidget *mDialogNew(int size,mWindow *owner,uint32_t style);
intptr_t mDialogRun(mDialog *p,mBool destroy);
void mDialogEnd(mDialog *p,intptr_t ret);

int mDialogEventHandle_okcancel(mWidget *wg,mEvent *ev);

#ifdef __cplusplus
}
#endif

#endif
