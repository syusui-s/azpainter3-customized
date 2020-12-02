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

#ifndef MLIB_COMBOBOX_H
#define MLIB_COMBOBOX_H

#include "mWidgetDef.h"
#include "mListViewItem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_COMBOBOX(p)  ((mComboBox *)(p))

typedef struct _mLVItemMan mLVItemMan;

typedef struct
{
	mLVItemMan *manager;
	int itemHeight;
	uint32_t style;
}mComboBoxData;

typedef struct _mComboBox
{
	mWidget wg;
	mComboBoxData cb;
}mComboBox;

enum MCOMBOBOX_NOTIFY
{
	MCOMBOBOX_N_CHANGESEL
};


void mComboBoxDestroyHandle(mWidget *p);
void mComboBoxCalcHintHandle(mWidget *p);
void mComboBoxDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mComboBoxEventHandle(mWidget *wg,mEvent *ev);

mComboBox *mComboBoxCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mComboBox *mComboBoxNew(int size,mWidget *parent,uint32_t style);

void mComboBoxSetItemHeight(mComboBox *p,int height);

mListViewItem *mComboBoxAddItem(mComboBox *p,const char *text,intptr_t param);
mListViewItem *mComboBoxAddItem_static(mComboBox *p,const char *text,intptr_t param);

void mComboBoxAddItem_ptr(mComboBox *p,mListViewItem *item);
mListViewItem *mComboBoxAddItem_draw(mComboBox *p,const char *text,intptr_t param,
	void (*draw)(mPixbuf *,mListViewItem *,mListViewItemDraw *));

void mComboBoxAddItems(mComboBox *p,const char *text,intptr_t paramtop);
void mComboBoxAddTrItems(mComboBox *p,int num,int tridtop,intptr_t paramtop);

void mComboBoxDeleteAllItem(mComboBox *p);
void mComboBoxDeleteItem(mComboBox *p,mListViewItem *item);
void mComboBoxDeleteItem_index(mComboBox *p,int index);
mListViewItem *mComboBoxDeleteItem_sel(mComboBox *p);

mListViewItem *mComboBoxGetTopItem(mComboBox *p);
mListViewItem *mComboBoxGetSelItem(mComboBox *p);
mListViewItem *mComboBoxGetItemByIndex(mComboBox *p,int index);
int mComboBoxGetItemNum(mComboBox *p);
int mComboBoxGetSelItemIndex(mComboBox *p);
int mComboBoxGetItemIndex(mComboBox *p,mListViewItem *item);

void mComboBoxGetItemText(mComboBox *p,int index,mStr *str);
intptr_t mComboBoxGetItemParam(mComboBox *p,int index);

void mComboBoxSetItemText(mComboBox *p,int index,const char *text);

void mComboBoxSetSelItem(mComboBox *p,mListViewItem *item);
void mComboBoxSetSel_index(mComboBox *p,int index);
mBool mComboBoxSetSel_findParam(mComboBox *p,intptr_t param);
void mComboBoxSetSel_findParam_notfind(mComboBox *p,intptr_t param,int notfindindex);

mListViewItem *mComboBoxFindItemParam(mComboBox *p,intptr_t param);
void mComboBoxSetWidthAuto(mComboBox *p);

#ifdef __cplusplus
}
#endif

#endif
