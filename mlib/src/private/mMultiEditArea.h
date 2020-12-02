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

#ifndef MLIB_MULTIEDITAREA_H
#define MLIB_MULTIEDITAREA_H

#include "mScrollViewArea.h"

#define M_MULTIEDITAREA(p)  ((mMultiEditArea *)(p))

typedef struct _mEditTextBuf mEditTextBuf;

typedef struct
{
	mEditTextBuf *buf;
	int drag_pos;
	mBool fpress;
}mMultiEditAreaData;

typedef struct _mMultiEditArea
{
	mWidget wg;
	mScrollViewAreaData sva;
	mMultiEditAreaData mea;
}mMultiEditArea;


#ifdef __cplusplus
extern "C" {
#endif

mMultiEditArea *mMultiEditAreaNew(int size,mWidget *parent,mEditTextBuf *textbuf);

void mMultiEditArea_setScrollInfo(mMultiEditArea *p);
void mMultiEditArea_onChangeCursorPos(mMultiEditArea *p);

#ifdef __cplusplus
}
#endif

#endif
