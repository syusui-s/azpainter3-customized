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

#ifndef MLIB_TREE_H
#define MLIB_TREE_H

#include "mTreeDef.h"

#ifdef __cplusplus
extern "C" {
#endif

mTreeItem *mTreeAppendNew(mTree *tree,mTreeItem *parent,int size,void (*destroy)(mTreeItem *));
mTreeItem *mTreeInsertNew(mTree *tree,mTreeItem *insert,int size,void (*destroy)(mTreeItem *));

void mTreeLinkBottom(mTree *tree,mTreeItem *parent,mTreeItem *item);
void mTreeLinkInsert(mTree *tree,mTreeItem *ins,mTreeItem *item);
void mTreeLinkInsert_parent(mTree *tree,mTreeItem *parent,mTreeItem *ins,mTreeItem *item);
void mTreeLinkRemove(mTree *tree,mTreeItem *item);

void mTreeDeleteAll(mTree *p);
void mTreeDeleteItem(mTree *p,mTreeItem *item);
void mTreeMoveItem(mTree *p,mTreeItem *src,mTreeItem *dst);
void mTreeMoveItem_top(mTree *p,mTreeItem *src,mTreeItem *parent);
void mTreeMoveItem_bottom(mTree *p,mTreeItem *src,mTreeItem *parent);

mTreeItem *mTreeGetLastItem(mTree *p);
mTreeItem *mTreeGetFirstItem(mTree *p,mTreeItem *parent);

int mTreeItemGetNum(mTree *p);
void mTreeSortChild(mTree *p,mTreeItem *root,int (*comp)(mTreeItem *,mTreeItem *,intptr_t),intptr_t param);
void mTreeSortAll(mTree *p,int (*comp)(mTreeItem *,mTreeItem *,intptr_t),intptr_t param);

/*--------*/

mTreeItem *mTreeItemGetNext(mTreeItem *p);
mTreeItem *mTreeItemGetNext_root(mTreeItem *p,mTreeItem *root);
mTreeItem *mTreeItemGetNextPass(mTreeItem *p);
mTreeItem *mTreeItemGetNextPass_root(mTreeItem *p,mTreeItem *root);

mTreeItem *mTreeItemGetPrev(mTreeItem *p);
mTreeItem *mTreeItemGetPrev_root(mTreeItem *p,mTreeItem *root);
mTreeItem *mTreeItemGetPrevPass(mTreeItem *p);

mTreeItem *mTreeItemGetBottom(mTreeItem *p);

mBool mTreeItemIsChild(mTreeItem *p,mTreeItem *parent);

#ifdef __cplusplus
}
#endif

#endif
