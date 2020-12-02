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

#ifndef MLIB_POPUPPROGRESS_H
#define MLIB_POPUPPROGRESS_H

#include "mWindowDef.h"
#include "mProgressBar.h"
#include "mThread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_POPUPPROGRESS(p)  ((mPopupProgress *)(p))

typedef struct
{
	mProgressBar *progress;
	intptr_t param;
}mPopupProgressData;

typedef struct _mPopupProgress
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mPopupProgressData pg;
}mPopupProgress;


mPopupProgress *mPopupProgressNew(int size,int w,uint32_t progress_style);
void mPopupProgressRun(mPopupProgress *p,int x,int y,void (*threadfunc)(mThread *));

void mPopupProgressThreadEnd(void);
void mPopupProgressThreadSetMax(mPopupProgress *p,int max);
void mPopupProgressThreadSetPos(mPopupProgress *p,int pos);
void mPopupProgressThreadIncPos(mPopupProgress *p);
void mPopupProgressThreadAddPos(mPopupProgress *p,int add);
void mPopupProgressThreadBeginSubStep(mPopupProgress *p,int stepnum,int max);
void mPopupProgressThreadBeginSubStep_onestep(mPopupProgress *p,int stepnum,int max);
void mPopupProgressThreadIncSubStep(mPopupProgress *p);

#ifdef __cplusplus
}
#endif

#endif
