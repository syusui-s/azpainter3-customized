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

#ifndef MLK_POPUP_PROGRESS_H
#define MLK_POPUP_PROGRESS_H

#define MLK_POPUPPROGRESS(p)  ((mPopupProgress *)(p))
#define MLK_POPUPPROGRESS_DEF MLK_POPUP_DEF mPopupProgressData pg;

typedef struct
{
	mProgressBar *progress;
	intptr_t param;
}mPopupProgressData;

struct _mPopupProgress
{
	MLK_POPUP_DEF
	mPopupProgressData pg;
};


#ifdef __cplusplus
extern "C" {
#endif

mPopupProgress *mPopupProgressNew(int size,uint32_t progress_style);
void mPopupProgressRun(mPopupProgress *p,mWidget *parent,int x,int y,mBox *box,uint32_t popup_flags,
	int width,void (*threadfunc)(mThread *));

void mPopupProgressThreadEnd(mPopupProgress *p);
void mPopupProgressThreadSetMax(mPopupProgress *p,int max);
void mPopupProgressThreadSetPos(mPopupProgress *p,int pos);
void mPopupProgressThreadIncPos(mPopupProgress *p);
void mPopupProgressThreadAddPos(mPopupProgress *p,int add);
void mPopupProgressThreadSubStep_begin(mPopupProgress *p,int stepnum,int max);
void mPopupProgressThreadSubStep_begin_onestep(mPopupProgress *p,int stepnum,int max);
void mPopupProgressThreadSubStep_inc(mPopupProgress *p);

#ifdef __cplusplus
}
#endif

#endif
