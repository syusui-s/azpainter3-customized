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

#ifndef MLIB_LIST_H
#define MLIB_LIST_H

#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

mListItem *mListAppendNew(mList *list,int size,void (*destroy)(mListItem *));
mListItem *mListInsertNew(mList *list,mListItem *pos,int size,void (*destroy)(mListItem *));

void mListAppend(mList *list,mListItem *item);
void mListInsert(mList *list,mListItem *pos,mListItem *item);
void mListRemove(mList *list,mListItem *item);

mBool mListDup(mList *dst,mList *src,int itemsize);

void mListDeleteAll(mList *list);
void mListDelete(mList *list,mListItem *item);
void mListDeleteNoDestroy(mList *list,mListItem *item);
mBool mListDeleteByIndex(mList *list,int index);
void mListDeleteTopNum(mList *list,int num);
void mListDeleteBottomNum(mList *list,int num);

mBool mListMove(mList *list,mListItem *src,mListItem *dst);
void mListMoveTop(mList *list,mListItem *item);
mBool mListMoveUpDown(mList *list,mListItem *item,mBool up);
void mListSwap(mList *list,mListItem *item1,mListItem *item2);
void mListSort(mList *list,int (*comp)(mListItem *,mListItem *,intptr_t),intptr_t param);

int mListGetDir(mList *list,mListItem *item1,mListItem *item2);
mListItem *mListGetItemByIndex(mList *list,int index);
int mListGetItemIndex(mList *list,mListItem *item);

mListItem *mListItemAlloc(int size,void (*destroy)(mListItem *));
void mListLinkAppend(mList *list,mListItem *item);
void mListLinkInsert(mList *list,mListItem *item,mListItem *pos);
void mListLinkRemove(mList *list,mListItem *item);

#ifdef __cplusplus
}
#endif

#endif
