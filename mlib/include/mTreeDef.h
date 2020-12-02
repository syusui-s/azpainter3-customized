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

#ifndef MLIB_TREE_DEF_H
#define MLIB_TREE_DEF_H

typedef struct _mTreeItem mTreeItem;

struct _mTreeItem
{
	mTreeItem *prev,*next,*first,*last,*parent;
	void (*destroy)(mTreeItem *);
};

typedef struct _mTree
{
	mTreeItem *top,*bottom;
}mTree;


#define M_TREE(p)      ((mTree *)(p))
#define M_TREEITEM(p)  ((mTreeItem *)(p))
#define MTREE_INIT {0,0}

#endif
