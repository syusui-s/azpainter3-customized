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

/***********************************************
 * キャンバスビュー/イメージビューアの
 * 操作設定ダイアログ (共通)
 ***********************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_conflistview.h"

#include "widget_func.h"

#include "trid.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mConfListView *list;
}_dialog;

//----------------------

enum
{
	TRID_TITLE = 0,
	TRID_ITEM_TOP,
	TRID_CANVASVIEW_TEXT = 100,
	TRID_IMAGEVIEWER_TEXT
};

#define _BUTTONLIST_NUM  5

//----------------------


/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,uint8_t *dstbuf,mlkbool is_imageviewer)
{
	_dialog *p;
	mConfListView *list;
	const char *seltext;
	int i;

	MLK_TRGROUP(TRGROUP_DLG_VIEWER_OPTION);

	//作成
	
	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), mDialogEventDefault_okcancel);

	if(!p) return NULL;

	//mConfListView

	seltext = MLK_TR((is_imageviewer)? TRID_IMAGEVIEWER_TEXT: TRID_CANVASVIEW_TEXT);

	p->list = list = mConfListViewNew(MLK_WIDGET(p), 0, 0);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 22, 9);

	for(i = 0; i < _BUTTONLIST_NUM; i++)
	{
		mConfListView_addItem_select(list, MLK_TR(TRID_ITEM_TOP + i),
			dstbuf[i], seltext, FALSE, i); 
	}

	mConfListView_setColumnWidth_name(list);

	//OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,12,0,0));
	
	return p;
}

/* 値の取得関数 */

static int _func_getvalue(int type,intptr_t item_param,mConfListViewValue *val,void *param)
{
	*((uint8_t *)param + item_param) = val->select;

	return 0;
}

/** 設定ダイアログ実行 */

void PanelViewer_optionDlg_run(mWindow *parent,uint8_t *dstbuf,mlkbool is_imageviewer)
{
	_dialog *p;

	p = _create_dialog(parent, dstbuf, is_imageviewer);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	if(mDialogRun(MLK_DIALOG(p), FALSE))
	{
		mConfListView_getValues(p->list, _func_getvalue, dstbuf);
	}

	mWidgetDestroy(MLK_WIDGET(p));
}

