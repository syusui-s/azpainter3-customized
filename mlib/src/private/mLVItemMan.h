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

#ifndef MLIB_LISTVIEWITEMMANAGER_H
#define MLIB_LISTVIEWITEMMANAGER_H

#include "mListViewItem.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mLVItemMan
{
	mList list;
	mListViewItem *itemFocus;
	mBool bMultiSel;
}mLVItemMan;


void mLVItemMan_free(mLVItemMan *p);
mLVItemMan *mLVItemMan_new(void);

mListViewItem *mLVItemMan_addItem(mLVItemMan *p,int size,
	const char *text,int icon,uint32_t flags,intptr_t param);
mListViewItem *mLVItemMan_insertItem(mLVItemMan *p,mListViewItem *ins,
	const char *text,int icon,uint32_t flags,intptr_t param);

void mLVItemMan_deleteAllItem(mLVItemMan *p);
mBool mLVItemMan_deleteItem(mLVItemMan *p,mListViewItem *item);
mBool mLVItemMan_deleteItemByIndex(mLVItemMan *p,int index);
void mLVItemMan_selectAll(mLVItemMan *p);
void mLVItemMan_unselectAll(mLVItemMan *p);

mListViewItem *mLVItemMan_getItemByIndex(mLVItemMan *p,int index);
int mLVItemMan_getItemIndex(mLVItemMan *p,mListViewItem *item);

mListViewItem *mLVItemMan_findItemParam(mLVItemMan *p,intptr_t param);
mListViewItem *mLVItemMan_findItemText(mLVItemMan *p,const char *text);
mBool mLVItemMan_moveItem_updown(mLVItemMan *p,mListViewItem *item,mBool down);

void mLVItemMan_setText(mListViewItem *pi,const char *text);

mBool mLVItemMan_setFocusItem(mLVItemMan *p,mListViewItem *item);
mBool mLVItemMan_setFocusItemByIndex(mLVItemMan *p,int index);
mBool mLVItemMan_setFocusHomeEnd(mLVItemMan *p,mBool home);

mBool mLVItemMan_updownFocus(mLVItemMan *p,mBool down);
mBool mLVItemMan_select(mLVItemMan *p,uint32_t state,mListViewItem *item);

#ifdef __cplusplus
}
#endif

#endif
