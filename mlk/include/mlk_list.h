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

#ifndef MLK_LIST_H
#define MLK_LIST_H

#define MLK_LIST_FOR(list,item,itemtype)  for(item = (itemtype *)(list).top; item; item = (itemtype *)(((mListItem *)item)->next))
#define MLK_LIST_FOR_REV(list,item,itemtype) for(item = (itemtype *)(list).bottom; item; item = (itemtype *)(((mListItem *)item)->prev))

#ifdef __cplusplus
extern "C" {
#endif

mListItem *mListItemNew(int size);

void mListInit(mList *list);
mListItem *mListAppendNew(mList *list,int size);
mListItem *mListInsertNew(mList *list,mListItem *pos,int size);

void mListAppendItem(mList *list,mListItem *item);
void mListInsertItem(mList *list,mListItem *pos,mListItem *item);
void mListRemoveItem(mList *list,mListItem *item);

mlkbool mListDup(mList *dst,mList *src,int itemsize);

void mListDeleteAll(mList *list);
void mListDelete(mList *list,mListItem *item);
void mListDelete_no_handler(mList *list,mListItem *item);
mlkbool mListDelete_index(mList *list,int index);
void mListDelete_tops(mList *list,int num);
void mListDelete_bottoms(mList *list,int num);

mlkbool mListMove(mList *list,mListItem *src,mListItem *dst);
mlkbool mListMoveToTop(mList *list,mListItem *item);
mlkbool mListMoveUpDown(mList *list,mListItem *item,mlkbool up);
void mListSwapItem(mList *list,mListItem *item1,mListItem *item2);
void mListSort(mList *list,int (*comp)(mListItem *,mListItem *,void *),void *param);

mListItem *mListGetItemAtIndex(mList *list,int index);

int mListItemGetDir(mListItem *item1,mListItem *item2);
int mListItemGetIndex(mListItem *item);

void mListLinkAppend(mList *list,mListItem *item);
void mListLinkInsert(mList *list,mListItem *item,mListItem *pos);
void mListLinkRemove(mList *list,mListItem *item);

/* mListCache */

mListCacheItem *mListCache_appendNew(mList *list,int size);
void mListCache_refItem(mList *list,mListCacheItem *item);
void mListCache_releaseItem(mList *list,mListCacheItem *item);
void mListCache_deleteUnused(mList *list,int maxnum);
void mListCache_deleteUnused_allnum(mList *list,int num);

#ifdef __cplusplus
}
#endif

#endif
