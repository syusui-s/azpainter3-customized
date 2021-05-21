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

/*****************************************
 * パネル配置設定ダイアログ
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_listview.h"
#include "mlk_checkbutton.h"
#include "mlk_arrowbutton.h"
#include "mlk_columnitem.h"
#include "mlk_event.h"
#include "mlk_str.h"

#include "def_config.h"
#include "def_widget.h"

#include "panel.h"
#include "widget_func.h"

#include "trid.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mListView *list;
	mWidget *wgbtt[4];

	int type;
	char pane_buf[PANEL_PANE_NUM + 1],	//ペインのリスト (0〜)
		panel_buf[PANEL_NUM + 3]; //パネルのリスト (文字ID)
	const char *text_canvas,
		*text_pane_name;
}_dialog;

enum
{
	WID_LIST = 100,
	WID_CK_PANEL,
	WID_CK_PANE,
	WID_BTT_UP,
	WID_BTT_DOWN,
	WID_BTT_RIGHT,
	WID_BTT_LEFT
};

enum
{
	TRID_TITLE,
	TRID_PANEL,
	TRID_PANE,
	TRID_PANE_NAME,
	TRID_CANVAS
};

//----------------------

/*
  - ペイン: param = 文字 ID
  - パネル: param = [パネル] 文字ID [ペインヘッダ] -1 * 番号(1〜)
*/


/* ペインのリストセット */

static void _setlist_pane(_dialog *p)
{
	mStr str = MSTR_INIT;
	int i,no;

	for(i = 0; i < PANEL_PANE_NUM + 1; i++)
	{
		no = p->pane_buf[i];
		
		if(no == 0)
			mListViewAddItem_text_static_param(p->list, p->text_canvas, no);
		else
		{
			mStrSetFormat(&str, p->text_pane_name, no);
		
			mListViewAddItem_text_copy_param(p->list, str.buf, no);
		}
	}

	mStrFree(&str);
}

/* ペインのヘッダアイテム追加
 *
 * param = ペイン番号(1〜) x -1 */

static void _additem_pane_header(_dialog *p,int no,mStr *str)
{
	mStrSetFormat(str, p->text_pane_name, no);
	
	mListViewAddItem(p->list, str->buf, -1,
		MCOLUMNITEM_F_COPYTEXT | MCOLUMNITEM_F_HEADER, -no);
}

/* パネルのリストセット */

static void _setlist_panel(_dialog *p)
{
	mListView *list = p->list;
	char *pc;
	int no,paneno = 2;
	mStr str = MSTR_INIT;

	_additem_pane_header(p, 1, &str);

	for(pc = p->panel_buf; *pc; pc++)
	{
		if(*pc == ':')
			_additem_pane_header(p, paneno++, &str);
		else
		{
			no = Panel_id_to_no(*pc);
			
			mListViewAddItem_text_static_param(list, MLK_TR(no), *pc);
		}
	}

	mStrFree(&str);
}

/* タイプの変更 */

static void _change_type(_dialog *p)
{
	int f;

	//リスト
	
	mListViewDeleteAllItem(p->list);

	if(p->type == 0)
		_setlist_panel(p);
	else
		_setlist_pane(p);

	//ボタン有効/無効

	f = (p->type == 0);

	mWidgetEnable(p->wgbtt[2], f);
	mWidgetEnable(p->wgbtt[3], f);
}

/* リストの状態から作業用データにセット */

static void _list_to_workbuf(_dialog *p)
{
	mColumnItem *pi;
	char *pd;
	int n;

	if(p->type == 0)
	{
		//パネル

		pd = p->panel_buf;

		for(pi = mListViewGetTopItem(p->list); pi; pi = MLK_COLUMNITEM(pi->i.next))
		{
			n = pi->param;
			
			if(n < 0)
			{
				//パネル1のヘッダはスキップ
				
				if(n != -1)
					*(pd++) = ':';
			}
			else
				*(pd++) = n;
		}

		*pd = 0;
	}
	else
	{
		//ペイン

		pd = p->pane_buf;

		for(pi = mListViewGetTopItem(p->list); pi; pi = MLK_COLUMNITEM(pi->i.next))
			*(pd++) = pi->param;
	}
}

/* アイテム移動 */

static void _move_item(_dialog *p,int dir)
{
	mColumnItem *item,*pi;

	item = mListViewGetFocusItem(p->list);
	if(!item) return;

	//パネルのヘッダ

	if(item->param < 0) return;

	//

	if(dir < 2)
	{
		//----- 上下

		//パネルの上移動時、ペイン1ヘッダの下まで

		if(p->type == 0
			&& dir == 0 && item->i.prev
			&& MLK_COLUMNITEM(item->i.prev)->param == -1)
			return;

		mListViewMoveItem_updown(p->list, item, (dir == 1));
	}
	else if(dir == 2)
	{
		//右: 次のペインの1番下へ移動

		for(pi = MLK_COLUMNITEM(item->i.next); pi; pi = MLK_COLUMNITEM(pi->i.next))
		{
			if(pi->param < -1) break;
		}

		if(pi)
		{
			for(pi = MLK_COLUMNITEM(pi->i.next); pi; pi = MLK_COLUMNITEM(pi->i.next))
			{
				if(pi->param < -1) break;
			}

			mListViewMoveItem(p->list, item, pi);
		}
	}
	else
	{
		//左: 前のペインの一番下へ移動

		for(pi = MLK_COLUMNITEM(item->i.prev); pi; pi = MLK_COLUMNITEM(pi->i.prev))
		{
			if(pi->param < -1) break;
		}

		if(pi)
			mListViewMoveItem(p->list, item, pi);
	}
}

/* 設定データから作業用データにセット */

static void _set_workbuf(_dialog *p)
{
	char *pc,*dst;

	//ペイン

	dst = p->pane_buf;

	for(pc = APPCONF->panel.pane_layout; *pc; pc++)
		*(dst++) = *pc - '0';

	//パネル

	strcpy(p->panel_buf, APPCONF->panel.panel_layout);
}

/* OK 時 */

static void _set_config(_dialog *p)
{
	int i;
	char *pc;

	_list_to_workbuf(p);

	//ペイン

	pc = APPCONF->panel.pane_layout;

	for(i = 0; i < PANEL_PANE_NUM + 1; i++)
		*(pc++) = p->pane_buf[i] + '0';

	*pc = 0;

	//パネル

	strcpy(APPCONF->panel.panel_layout, p->panel_buf);
}


//==========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//タイプ
			case WID_CK_PANEL:
			case WID_CK_PANE:
				_list_to_workbuf(p);
				
				p->type = ev->notify.id - WID_CK_PANEL;

				_change_type(p);
				break;
			//移動ボタン
			case WID_BTT_UP:
			case WID_BTT_DOWN:
			case WID_BTT_LEFT:
			case WID_BTT_RIGHT:
				_move_item(p, ev->notify.id - WID_BTT_UP);
				break;
		
			//OK
			case MLK_WID_OK:
				_set_config(p);
				mDialogEnd(MLK_DIALOG(wg), TRUE);
				break;
			//キャンセル
			case MLK_WID_CANCEL:
				mDialogEnd(MLK_DIALOG(wg), FALSE);
				break;
		}
	}

	return mDialogEventDefault(wg, ev);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent)
{
	_dialog *p;
	mWidget *ct,*ct2,*wg;
	mListView *list;
	int i;
	int bttstyle[4] = { MARROWBUTTON_S_UP, MARROWBUTTON_S_DOWN, MARROWBUTTON_S_RIGHT, MARROWBUTTON_S_LEFT };

	MLK_TRGROUP(TRGROUP_DLG_PANEL_LAYOUT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);

	if(!p) return NULL;

	//データ

	p->text_canvas = MLK_TR(TRID_CANVAS);
	p->text_pane_name = MLK_TR(TRID_PANE_NAME);

	_set_workbuf(p);

	//------ タイプ

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, 0, 0);

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_CK_PANEL + i, 0, 0, MCHECKBUTTON_S_RADIO,
			MLK_TR(TRID_PANEL + i), (i == 0));
	}
	
	//----- リスト

	//リスト

	ct = mContainerCreateHorz(MLK_WIDGET(p), 8, MLF_EXPAND_WH, MLK_MAKE32_4(0,8,0,0));

	list = p->list = mListViewCreate(ct, WID_LIST, MLF_EXPAND_WH, 0,
		0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 20, 20);

	//ボタン

	ct2 = mContainerCreateVert(ct, 5, 0, 0);

	for(i = 0; i < 4; i++)
	{
		p->wgbtt[i] = wg = (mWidget *)mArrowButtonCreate_minsize(ct2, WID_BTT_UP + i, 0,
			(i == 2)? MLK_MAKE32_4(0,6,0,0): 0,
			bttstyle[i] | MARROWBUTTON_S_FONTSIZE, 26);
	}

	//---- OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	//パネル名のグループにしておく

	MLK_TRGROUP(TRGROUP_PANEL_NAME);

	//

	_change_type(p);

	return p;
}

/** ダイアログ実行 */

mlkbool PanelLayoutDlg_run(mWindow *parent)
{
	_dialog *p;

	p = _create_dialog(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

