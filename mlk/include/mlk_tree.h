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

#ifndef MLK_TREE_H
#define MLK_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

/* mTree */

mTreeItem *mTreeAppendNew(mTree *p,mTreeItem *parent,int size);
mTreeItem *mTreeAppendNew_top(mTree *p,mTreeItem *parent,int size);
mTreeItem *mTreeInsertNew(mTree *p,mTreeItem *insert,int size);

void mTreeLinkBottom(mTree *p,mTreeItem *parent,mTreeItem *item);
void mTreeLinkTop(mTree *p,mTreeItem *parent,mTreeItem *item);
void mTreeLinkInsert(mTree *p,mTreeItem *ins,mTreeItem *item);
void mTreeLinkInsert_parent(mTree *p,mTreeItem *parent,mTreeItem *ins,mTreeItem *item);
void mTreeLinkRemove(mTree *p,mTreeItem *item);

void mTreeDeleteAll(mTree *p);
void mTreeDeleteItem(mTree *p,mTreeItem *item);
void mTreeDeleteChildren(mTree *p,mTreeItem *root);
void mTreeMoveItem(mTree *p,mTreeItem *src,mTreeItem *dst);
void mTreeMoveItemToTop(mTree *p,mTreeItem *src,mTreeItem *parent);
void mTreeMoveItemToBottom(mTree *p,mTreeItem *src,mTreeItem *parent);

mTreeItem *mTreeGetBottomLastItem(mTree *p);
mTreeItem *mTreeGetFirstItem(mTree *p,mTreeItem *parent);

int mTreeItemGetNum(mTree *p);
void mTreeSortChildren(mTree *p,mTreeItem *root,int (*comp)(mTreeItem *,mTreeItem *,void *),void *param);
void mTreeSortAll(mTree *p,int (*comp)(mTreeItem *,mTreeItem *,void *),void *param);

/* mTreeItem */

mTreeItem *mTreeItemNew(int size);

mTreeItem *mTreeItemGetNext(mTreeItem *p);
mTreeItem *mTreeItemGetNext_root(mTreeItem *p,mTreeItem *root);
mTreeItem *mTreeItemGetNextPass(mTreeItem *p);
mTreeItem *mTreeItemGetNextPass_root(mTreeItem *p,mTreeItem *root);

mTreeItem *mTreeItemGetPrev(mTreeItem *p);
mTreeItem *mTreeItemGetPrev_root(mTreeItem *p,mTreeItem *root);
mTreeItem *mTreeItemGetPrevPass(mTreeItem *p);

mTreeItem *mTreeItemGetBottom(mTreeItem *p);
mlkbool mTreeItemIsChild(mTreeItem *p,mTreeItem *parent);

#ifdef __cplusplus
}
#endif

#endif
