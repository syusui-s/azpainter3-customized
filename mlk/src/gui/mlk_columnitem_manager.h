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

#ifndef MLK_COLUMNITEM_MANAGER_H
#define MLK_COLUMNITEM_MANAGER_H

#include "mlk_columnitem.h"

enum
{
	MCIMANAGER_ONCLICK_CHANGE_FOCUS = 1<<0,
	MCIMANAGER_ONCLICK_ON_FOCUS = 1<<1,
	MCIMANAGER_ONCLICK_KEY_SEL = 1<<2
};

#ifdef __cplusplus
extern "C" {
#endif

void __mColumnItemSetText(mColumnItem *pi,const char *text,mlkbool copy,mlkbool multi_column,mFont *font);

/* mCIManager */

void mCIManagerInit(mCIManager *p,mlkbool multi_sel);
void mCIManagerFree(mCIManager *p);

mColumnItem *mCIManagerInsertItem(mCIManager *p,
	mColumnItem *ins,int size,
	const char *text,int icon,uint32_t flags,intptr_t param,mFont *font,mlkbool multi_column);

void mCIManagerDeleteAllItem(mCIManager *p);
mlkbool mCIManagerDeleteItem(mCIManager *p,mColumnItem *item);
mlkbool mCIManagerDeleteItem_atIndex(mCIManager *p,int index);

void mCIManagerSelectAll(mCIManager *p);
void mCIManagerUnselectAll(mCIManager *p);

int mCIManagerGetItemIndex(mCIManager *p,mColumnItem *item);
mColumnItem *mCIManagerGetItem_atIndex(mCIManager *p,int index);
mColumnItem *mCIManagerGetItem_fromParam(mCIManager *p,intptr_t param);
mColumnItem *mCIManagerGetItem_fromText(mCIManager *p,const char *text);

mlkbool mCIManagerSetFocusItem(mCIManager *p,mColumnItem *item);
mlkbool mCIManagerSetFocusItem_atIndex(mCIManager *p,int index);
mlkbool mCIManagerSetFocus_toHomeEnd(mCIManager *p,mlkbool home);

mlkbool mCIManagerMoveItem_updown(mCIManager *p,mColumnItem *item,mlkbool down);
mlkbool mCIManagerFocusItem_updown(mCIManager *p,mlkbool down);
int mCIManagerOnClick(mCIManager *p,uint32_t key_state,mColumnItem *item);

int mCIManagerGetMaxWidth(mCIManager *p);

#ifdef __cplusplus
}
#endif

#endif
