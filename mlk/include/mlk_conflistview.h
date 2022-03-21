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

#ifndef MLK_CONFLISTVIEW_H
#define MLK_CONFLISTVIEW_H

#include "mlk_listview.h"

#define MLK_CONFLISTVIEW(p)  ((mConfListView *)(p))
#define MLK_CONFLISTVIEW_DEF MLK_LISTVIEW_DEF mConfListViewData clv;

typedef union
{
	char *text;
	int num;
	int checked;
	int select;
	uint32_t color;
	uint32_t key;
}mConfListViewValue;

typedef int (*mFuncConfListView_getValue)(int type,intptr_t item_param,mConfListViewValue *val,void *param);

typedef struct
{
	mWidget *editwg;
	mColumnItem *item;
}mConfListViewData;

struct _mConfListView
{
	MLK_LISTVIEW_DEF
	mConfListViewData clv;
};

enum MCONFLISTVIEW_NOTIFY
{
	MCONFLISTVIEW_N_CHANGE_VAL
};

enum MCONFLISTVIEW_VALTYPE
{
	MCONFLISTVIEW_VALTYPE_TEXT,
	MCONFLISTVIEW_VALTYPE_NUM,
	MCONFLISTVIEW_VALTYPE_CHECK,
	MCONFLISTVIEW_VALTYPE_SELECT,
	MCONFLISTVIEW_VALTYPE_COLOR,
	MCONFLISTVIEW_VALTYPE_ACCEL,
	MCONFLISTVIEW_VALTYPE_FONT,
	MCONFLISTVIEW_VALTYPE_FILE
};


#ifdef __cplusplus
extern "C" {
#endif

mConfListView *mConfListViewNew(mWidget *parent,int size,uint32_t fstyle);

void mConfListViewDestroy(mWidget *p);
int mConfListViewHandle_event(mWidget *wg,mEvent *ev);

void mConfListView_setColumnWidth_name(mConfListView *p);

void mConfListView_addItem_header(mConfListView *p,const char *text);
void mConfListView_addItem_text(mConfListView *p,const char *name,const char *val,intptr_t param);
void mConfListView_addItem_num(mConfListView *p,const char *name,int val,int min,int max,int dig,intptr_t param);
void mConfListView_addItem_check(mConfListView *p,const char *name,int checked,intptr_t param);
void mConfListView_addItem_select(mConfListView *p,const char *name,int def,const char *list,mlkbool copy,intptr_t param);
void mConfListView_addItem_color(mConfListView *p,const char *name,mRgbCol col,intptr_t param);
void mConfListView_addItem_accel(mConfListView *p,const char *name,uint32_t key,intptr_t param);
void mConfListView_addItem_font(mConfListView *p,const char *name,const char *infotext,uint32_t flags,intptr_t param);
void mConfListView_addItem_file(mConfListView *p,const char *name,const char *path,const char *filter,intptr_t param);

void mConfListView_setItemValue(mConfListView *p,mColumnItem *item,mConfListViewValue *val);
void mConfListView_setItemValue_atIndex(mConfListView *p,int index,mConfListViewValue *val);

mlkbool mConfListView_getValues(mConfListView *p,mFuncConfListView_getValue func,void *param);

void mConfListView_setFocusValue_zero(mConfListView *p);
void mConfListView_clearAll_accel(mConfListView *p);

intptr_t mConfListViewItem_getParam(mColumnItem *pi);
uint32_t mConfListViewItem_getAccelKey(mColumnItem *pi);

#ifdef __cplusplus
}
#endif

#endif
