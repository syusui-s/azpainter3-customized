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

/*****************************************
 * グラデーションリスト編集ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_listview.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_columnitem.h"
#include "mlk_pixbuf.h"
#include "mlk_list.h"

#include "def_draw.h"
#include "def_tool_option.h"

#include "appresource.h"
#include "gradation_list.h"
#include "widget_func.h"

#include "trid.h"


//------------------

typedef struct
{
	MLK_DIALOG_DEF

	mListView *list;
}_dialog;


#define _SET_MODIFY  APPDRAW->fmodify_grad_list = TRUE

//------------------


/* アイテム描画 */

static int _item_draw_handle(mPixbuf *pixbuf,mColumnItem *item,mColumnItemDrawInfo *info)
{
	mBox box = info->box;
	uint8_t *buf;

	//枠

	mPixbufBox(pixbuf, box.x + 3, box.y + 2, box.w - 6, box.h - 4, 0);

	//グラデーション

	buf = GRADITEM(item->param)->buf;

	GradItem_drawPixbuf(pixbuf,
		box.x + 4, box.y + 3, box.w - 8, box.h - 6,
		buf + 3, buf + 3 + buf[1] * 5, buf[0] & 2);

	return 0;
}

/* mListView にアイテム追加 & 選択 */

static void _add_listitem_sel(_dialog *p,GradItem *gi)
{
	mColumnItem *ci;

	ci = mListViewAddItem(p->list, NULL, 0, 0, (intptr_t)gi);

	if(ci)
	{
		ci->draw = _item_draw_handle;

		mListViewSetFocusItem(p->list, ci);
	}
}

/* 追加 */

static void _cmd_add(_dialog *p)
{
	GradItem *gi;

	gi = GradationList_newItem(&APPDRAW->list_grad_custom);
	if(!gi) return;

	_SET_MODIFY;

	_add_listitem_sel(p, gi);
}

/* 複製 */

static void _cmd_copy(_dialog *p,mColumnItem *item)
{
	GradItem *gi;

	if(!item) return;

	gi = GradationList_copyItem(&APPDRAW->list_grad_custom, GRADITEM(item->param));
	if(!gi) return;
	
	_SET_MODIFY;

	_add_listitem_sel(p, gi);
}

/* 削除 */

static void _cmd_del(_dialog *p,mColumnItem *item)
{
	if(!item) return;
	
	_SET_MODIFY;

	mListDelete(&APPDRAW->list_grad_custom, (mListItem *)item->param);

	mListViewDeleteItem_focus(p->list);
}

/* 上/下へ */

static void _cmd_updown(_dialog *p,mColumnItem *item,mlkbool down)
{
	if(!item) return;

	if(mListViewMoveItem_updown(p->list, item, down))
	{
		_SET_MODIFY;

		mListMoveUpDown(&APPDRAW->list_grad_custom, (mListItem *)item->param, !down);
	}
}

/* COMMAND イベント */

static void _event_command(_dialog *p,int id)
{
	mColumnItem *item;

	item = mListViewGetFocusItem(p->list);

	switch(id)
	{
		//追加
		case APPRES_LISTEDIT_ADD:
			_cmd_add(p);
			break;
		//複製
		case APPRES_LISTEDIT_COPY:
			_cmd_copy(p, item);
			break;
		//削除
		case APPRES_LISTEDIT_DEL:
			_cmd_del(p, item);
			break;
		//上へ/下へ
		case APPRES_LISTEDIT_UP:
		case APPRES_LISTEDIT_DOWN:
			_cmd_updown(p, item, (id == APPRES_LISTEDIT_DOWN));
			break;
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_COMMAND:
			_event_command((_dialog *)wg, ev->cmd.id);
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dialog *_dialog_create(mWindow *parent)
{
	_dialog *p;
	mListView *list;
	GradItem *pi;
	mColumnItem *ci;
	const uint8_t bttno[] = {
		APPRES_LISTEDIT_ADD, APPRES_LISTEDIT_COPY, APPRES_LISTEDIT_DEL,
		APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, 255
	};

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR2(TRGROUP_DLG_TITLES, TRID_DLG_TITLE_GRADLIST_EDIT),
		_event_handle);

	if(!p) return NULL;

	//リスト

	p->list = list = mListViewCreate(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0,
		0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	mListViewSetItemHeight_min(list, 20);

	list->wg.hintW = 50;
	list->wg.initW = 300;
	list->wg.initH = 300;

	MLK_LIST_FOR(APPDRAW->list_grad_custom, pi, GradItem)
	{
		ci = mListViewAddItem(list, NULL, 0, 0, (intptr_t)pi);

		ci->draw = _item_draw_handle;
	}

	//ボタン

	widget_createListEdit_iconbar_btt(MLK_WIDGET(p), bttno, FALSE);

	return p;
}

/* 実行後の調整 */

static void _after_run(AppDraw *p)
{
	int id;

	//現在の選択IDが存在しない時、先頭アイテムのIDを選択

	id = TOOLOPT_GRAD_GET_CUSTOM_ID(p->tool.opt_grad);

	if(!GradationList_getItem_id(&p->list_grad_custom, id)
		&& p->list_grad_custom.top)
	{
		id = GRADITEM(p->list_grad_custom.top)->id;

		TOOLOPT_GRAD_SET_CUSTOM_ID(&p->tool.opt_grad, id);
	}
}

/** ダイアログ実行 (データは直接変更する) */

void GradationListEditDlg_run(mWindow *parent)
{
	_dialog *p;

	p = _dialog_create(parent);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	mDialogRun(MLK_DIALOG(p), TRUE);

	_after_run(APPDRAW);
}

