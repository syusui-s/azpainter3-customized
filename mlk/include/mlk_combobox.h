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

#ifndef MLK_COMBOBOX_H
#define MLK_COMBOBOX_H

#define MLK_COMBOBOX(p)  ((mComboBox *)(p))
#define MLK_COMBOBOX_DEF mWidget wg; mComboBoxData cb;

typedef struct
{
	mCIManager manager;
	int item_height;
	uint32_t fstyle;
}mComboBoxData;

struct _mComboBox
{
	mWidget wg;
	mComboBoxData cb;
};

enum MCOMBOBOX_NOTIFY
{
	MCOMBOBOX_N_CHANGE_SEL
};


#ifdef __cplusplus
extern "C" {
#endif

mComboBox *mComboBoxNew(mWidget *parent,int size,uint32_t fstyle);
mComboBox *mComboBoxCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mComboBoxDestroy(mWidget *p);
void mComboBoxHandle_calcHint(mWidget *p);
void mComboBoxHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mComboBoxHandle_event(mWidget *wg,mEvent *ev);

int mComboBoxGetItemNum(mComboBox *p);
void mComboBoxSetItemHeight(mComboBox *p,int height);
void mComboBoxSetItemHeight_min(mComboBox *p,int height);
void mComboBoxSetAutoWidth(mComboBox *p);

mColumnItem *mComboBoxAddItem(mComboBox *p,const char *text,uint32_t flags,intptr_t param);
mColumnItem *mComboBoxAddItem_static(mComboBox *p,const char *text,intptr_t param);
mColumnItem *mComboBoxAddItem_copy(mComboBox *p,const char *text,intptr_t param);
void mComboBoxAddItem_ptr(mComboBox *p,mColumnItem *item);

void mComboBoxAddItems_sepnull(mComboBox *p,const char *text,intptr_t param);
void mComboBoxAddItems_sepnull_arrayInt(mComboBox *p,const char *text,const int *array);
void mComboBoxAddItems_sepchar(mComboBox *p,const char *text,char split,intptr_t param);
void mComboBoxAddItems_tr(mComboBox *p,int idtop,int num,intptr_t param);

void mComboBoxDeleteAllItem(mComboBox *p);
void mComboBoxDeleteItem(mComboBox *p,mColumnItem *item);
void mComboBoxDeleteItem_index(mComboBox *p,int index);
mColumnItem *mComboBoxDeleteItem_select(mComboBox *p);

mColumnItem *mComboBoxGetTopItem(mComboBox *p);
mColumnItem *mComboBoxGetSelectItem(mComboBox *p);
mColumnItem *mComboBoxGetItem_atIndex(mComboBox *p,int index);
mColumnItem *mComboBoxGetItem_fromParam(mComboBox *p,intptr_t param);
mColumnItem *mComboBoxGetItem_fromText(mComboBox *p,const char *text);

int mComboBoxGetItemIndex(mComboBox *p,mColumnItem *item);
void mComboBoxGetItemText(mComboBox *p,int index,mStr *str);
intptr_t mComboBoxGetItemParam(mComboBox *p,int index);
void mComboBoxSetItemText_static(mComboBox *p,mColumnItem *item,const char *text);
void mComboBoxSetItemText_copy(mComboBox *p,int index,const char *text);

int mComboBoxGetSelIndex(mComboBox *p);
void mComboBoxSetSelItem(mComboBox *p,mColumnItem *item);
void mComboBoxSetSelItem_atIndex(mComboBox *p,int index);
mlkbool mComboBoxSetSelItem_fromParam(mComboBox *p,intptr_t param);
void mComboBoxSetSelItem_fromParam_notfound(mComboBox *p,intptr_t param,int notfound_index);
mlkbool mComboBoxSetSelItem_fromText(mComboBox *p,const char *text);

#ifdef __cplusplus
}
#endif

#endif
