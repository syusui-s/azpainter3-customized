/*$
 Copyright (C) 2013-2022 Azel.

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

#ifndef MLK_UNDO_H
#define MLK_UNDO_H

typedef struct _mUndo mUndo;

typedef mlkerr (*mFuncUndoNewItem)(mUndo *p,mListItem **ppdst);
typedef mlkerr (*mFuncUndoSetReverse)(mUndo *p,mListItem *dst,mListItem *src,int type);
typedef mlkerr (*mFuncUndoRun)(mUndo *p,mListItem *item,int type);


struct _mUndo
{
	mList list;
	mListItem *current;
	int maxnum;

	mFuncUndoNewItem newitem;
	mFuncUndoSetReverse set_reverse;
	mFuncUndoRun run;
};

enum MUNDO_TYPE
{
	MUNDO_TYPE_UNDO,
	MUNDO_TYPE_REDO
};


#ifdef __cplusplus
extern "C" {
#endif

mUndo *mUndoNew(int size);
void mUndoFree(mUndo *p);

mlkbool mUndoIsHaveUndo(mUndo *p);
mlkbool mUndoIsHaveRedo(mUndo *p);

void mUndoDeleteAll(mUndo *p);
void mUndoDelete_undo(mUndo *p);
void mUndoDelete_redo(mUndo *p);

void mUndoAdd(mUndo *p,mListItem *item);

mlkerr mUndoRun_undo(mUndo *p);
mlkerr mUndoRun_redo(mUndo *p);

#ifdef __cplusplus
}
#endif

#endif
