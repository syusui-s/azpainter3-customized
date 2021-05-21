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

/**********************************
 * テキストの単語リストダイアログ
 **********************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_listview.h"
#include "mlk_listheader.h"
#include "mlk_event.h"
#include "mlk_columnitem.h"
#include "mlk_str.h"
#include "mlk_list.h"

#include "def_config.h"

#include "textword_list.h"
#include "appresource.h"

#include "widget_func.h"

#include "trid.h"


//----------------

enum
{
	TRID_TITLE_LIST,
	TRID_TITLE_WORD,
	TRID_NAME,
	TRID_TEXT
};

//----------------


//**************************************
// 単語編集ダイアログ
//**************************************

typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit_name,
		*edit_text;
}_dlg_word;


/* イベント */

static int _word_event_handle(mWidget *wg,mEvent *ev)
{
	//OK 時、テキストが空なら終了しない

	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == MLK_WID_OK)
	{
		_dlg_word *p = (_dlg_word *)wg;
	
		if(mLineEditIsEmpty(p->edit_name)
			|| mLineEditIsEmpty(p->edit_text))
			return 1;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dlg_word *_create_dlg_word(mWindow *parent,mStr *strname,mStr *strtxt)
{
	_dlg_word *p;

	p = (_dlg_word *)widget_createDialog(parent, sizeof(_dlg_word),
		MLK_TR(TRID_TITLE_WORD), _word_event_handle);

	if(!p) return NULL;

	mContainerSetType_vert(MLK_CONTAINER(p), 6);

	//表示名

	mLabelCreate(MLK_WIDGET(p), 0, 0, 0, MLK_TR(TRID_NAME));

	p->edit_name = mLineEditCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0, 0);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->edit_name), 20, 0);

	mLineEditSetText(p->edit_name, strname->buf);

	//テキスト

	mLabelCreate(MLK_WIDGET(p), 0, 0, 0, MLK_TR(TRID_TEXT));

	p->edit_text = mLineEditCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0, 0);

	mLineEditSetText(p->edit_text, strtxt->buf);

	//OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,10,0,0));

	return p;
}

/* 単語編集ダイアログ */

static mlkbool _wordeditdlg_run(mWindow *parent,mStr *strname,mStr *strtxt)
{
	_dlg_word *p;
	int ret;

	p = _create_dlg_word(parent, strname, strtxt);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		mLineEditGetTextStr(p->edit_name, strname);
		mLineEditGetTextStr(p->edit_text, strtxt);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//**************************************
// リスト編集ダイアログ
//**************************************

/* [!] データの変更時は、AppConfig::fmodify_textword = TRUE にする。 */

typedef struct
{
	MLK_DIALOG_DEF

	mListView *list;
}_dlg_list;

enum
{
	WID_LIST_LIST = 100
};


/* 追加 */

static void _list_cmd_add(_dlg_list *p)
{
	mStr strname = MSTR_INIT,strtxt = MSTR_INIT;
	mColumnItem *ci;

	if(!_wordeditdlg_run(MLK_WINDOW(p), &strname, &strtxt))
		return;

	//mListView 追加

	mStrAppendChar(&strname, '\t');
	mStrAppendStr(&strname, &strtxt);

	ci = mListViewAddItem_text_static(p->list, strname.buf);

	mListViewSetFocusItem(p->list, ci);

	//

	mStrFree(&strname);
	mStrFree(&strtxt);
}

/* 編集 */

static void _list_cmd_edit(_dlg_list *p,mColumnItem *item)
{
	mStr strname = MSTR_INIT,strtxt = MSTR_INIT;

	mListViewGetItemColumnText(p->list, item, 0, &strname);
	mListViewGetItemColumnText(p->list, item, 1, &strtxt);

	if(_wordeditdlg_run(MLK_WINDOW(p), &strname, &strtxt))
	{
		mStrAppendChar(&strname, '\t');
		mStrAppendStr(&strname, &strtxt);

		mListViewSetItemText_static(p->list, item, strname.buf);
	}

	mStrFree(&strname);
	mStrFree(&strtxt);
}

/* COMMAND イベント */

static void _list_event_command(_dlg_list *p,int id)
{
	mColumnItem *item;

	item = mListViewGetFocusItem(p->list);

	switch(id)
	{
		//追加
		case APPRES_LISTEDIT_ADD:
			_list_cmd_add(p);
			break;
		//編集
		case APPRES_LISTEDIT_EDIT:
			if(item)
				_list_cmd_edit(p, item);
			break;
		//削除
		case APPRES_LISTEDIT_DEL:
			mListViewDeleteItem_focus(p->list);
			break;
		//上へ/下へ
		case APPRES_LISTEDIT_UP:
		case APPRES_LISTEDIT_DOWN:
			if(item)
				mListViewMoveItem_updown(p->list, item, (id == APPRES_LISTEDIT_DOWN));
			break;
	}
}

/* イベント */

static int _list_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_COMMAND:
			_list_event_command((_dlg_list *)wg, ev->cmd.id);
			break;

		case MEVENT_NOTIFY:
			//リストをダブルクリックで編集
			if(ev->notify.id == WID_LIST_LIST
				&& ev->notify.notify_type == MLISTVIEW_N_ITEM_L_DBLCLK)
			{
				_list_cmd_edit((_dlg_list *)wg, (mColumnItem *)ev->notify.param1);
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dlg_list *_create_dlg_list(mWindow *parent)
{
	_dlg_list *p;
	mListView *list;
	mListHeader *lh;
	TextWordItem *pi;
	mStr str = MSTR_INIT;
	const uint8_t bttno[] = {
		APPRES_LISTEDIT_ADD, APPRES_LISTEDIT_EDIT, APPRES_LISTEDIT_DEL,
		APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, 255
	};

	MLK_TRGROUP(TRGROUP_DLG_TEXTWORDLIST);

	p = (_dlg_list *)widget_createDialog(parent, sizeof(_dlg_list),
		MLK_TR(TRID_TITLE_LIST), _list_event_handle);

	if(!p) return NULL;

	//---- リスト

	p->list = list = mListViewCreate(MLK_WIDGET(p),
		WID_LIST_LIST, MLF_EXPAND_WH, MLK_MAKE32_4(0,0,0,3),
		MLISTVIEW_S_MULTI_COLUMN | MLISTVIEW_S_HAVE_HEADER | MLISTVIEW_S_GRID_COL,
		MSCROLLVIEW_S_HORZVERT_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 25, 20);

	//mListHeader

	lh = mListViewGetHeaderWidget(list);

	mListHeaderAddItem(lh, MLK_TR(TRID_NAME), mWidgetGetFontHeight(MLK_WIDGET(p)) * 10, 0, 0);
	mListHeaderAddItem(lh, MLK_TR(TRID_TEXT), 100, MLISTHEADER_ITEM_F_EXPAND, 0);

	//

	MLK_LIST_FOR(APPCONF->list_textword, pi, TextWordItem)
	{
		mStrSetFormat(&str, "%s\t%s",
			pi->txt, pi->txt + pi->text_pos);
	
		mListViewAddItem_text_static(list, str.buf);
	}

	mStrFree(&str);

	//--- ボタン

	widget_createListEdit_iconbar_btt(MLK_WIDGET(p), bttno, TRUE);

	return p;
}

/* OK 時、セット */

static void _set_ok(_dlg_list *p)
{
	mListView *list = p->list;
	mColumnItem *pi;
	mStr strname = MSTR_INIT,strtxt = MSTR_INIT;

	mListDeleteAll(&APPCONF->list_textword);

	//

	pi = mListViewGetTopItem(list);

	for(; pi; pi = (mColumnItem *)pi->i.next)
	{
		mListViewGetItemColumnText(list, pi, 0, &strname);
		mListViewGetItemColumnText(list, pi, 1, &strtxt);

		TextWordList_add(&APPCONF->list_textword, &strname, &strtxt);
	}

	mStrFree(&strname);
	mStrFree(&strtxt);

	//

	APPCONF->fmodify_textword = TRUE;
}

/** 単語リスト編集ダイアログ */

void TextWordListDlg_run(mWindow *parent)
{
	_dlg_list *p;

	p = _create_dlg_list(parent);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	if(mDialogRun(MLK_DIALOG(p), FALSE))
		_set_ok(p);

	mWidgetDestroy(MLK_WIDGET(p));
}

