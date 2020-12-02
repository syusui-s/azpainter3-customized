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

#ifndef MLIB_UNDO_H
#define MLIB_UNDO_H

#include "mListDef.h"

typedef struct _mUndo mUndo;

struct _mUndo
{
	mList list;
	mListItem *current;
	int maxnum;

	mListItem *(*create)(mUndo *);
	mBool (*setreverse)(mUndo *,mListItem *,mListItem *,int);
	mBool (*run)(mUndo *,mListItem *,int);
};

enum MUNDO_RUNERR
{
	MUNDO_RUNERR_OK,
	MUNDO_RUNERR_NO_DATA,
	MUNDO_RUNERR_CREATE,
	MUNDO_RUNERR_RUN
};

enum MUNDO_TYPE
{
	MUNDO_TYPE_UNDO,
	MUNDO_TYPE_REDO
};


#ifdef __cplusplus
extern "C" {
#endif

void mUndoFree(mUndo *p);
mUndo *mUndoNew(int size);

mBool mUndoIsHave(mUndo *p,mBool redo);

void mUndoDeleteAll(mUndo *p);
void mUndoDelete_onlyUndo(mUndo *p);
void mUndoDelete_onlyRedo(mUndo *p);

void mUndoAdd(mUndo *p,mListItem *item);

int mUndoRunUndo(mUndo *p);
int mUndoRunRedo(mUndo *p);

#ifdef __cplusplus
}
#endif

#endif
