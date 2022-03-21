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

#ifndef MLK_COLUMNITEM_H
#define MLK_COLUMNITEM_H

#define MLK_COLUMNITEM(p)  ((mColumnItem *)(p))

typedef struct _mColumnItemDrawInfo mColumnItemDrawInfo;
typedef int (*mFuncColumnItemDraw)(mPixbuf *pixbuf,mColumnItem *item,mColumnItemDrawInfo *info);

struct _mColumnItem
{
	mListItem i;

	char *text;
	uint8_t *text_col;
	int icon,
		text_width;
	uint32_t flags;
	intptr_t param;
	mFuncColumnItemDraw draw;
};

struct _mColumnItemDrawInfo
{
	mWidget *widget;
	mBox box;
	int column;
	uint32_t flags;
};


enum MCOLUMNITEM_FLAGS
{
	MCOLUMNITEM_F_SELECTED = 1<<0,
	MCOLUMNITEM_F_CHECKED  = 1<<1,
	MCOLUMNITEM_F_HEADER   = 1<<2,
	MCOLUMNITEM_F_COPYTEXT = 1<<3
};

enum MCOLUMNITEM_DRAW_FLAGS
{
	MCOLUMNITEM_DRAWF_DISABLED = 1<<0,
	MCOLUMNITEM_DRAWF_FOCUSED  = 1<<1,
	MCOLUMNITEM_DRAWF_ITEM_SELECTED = 1<<2,
	MCOLUMNITEM_DRAWF_ITEM_FOCUS = 1<<3
};


#ifdef __cplusplus
extern "C" {
#endif

void mColumnItemHandle_destroyItem(mList *list,mListItem *item);
int mColumnItem_getColText(mColumnItem *p,int col,char **pptop);
void mColumnItem_setColText(mColumnItem *p,int col,const char *text);

int mColumnItem_sortFunc_strcmp(mListItem *item1,mListItem *item2,void *param);

#ifdef __cplusplus
}
#endif

#endif
