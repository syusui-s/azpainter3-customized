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
 * キャンバスキー設定
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_button.h"
#include "mlk_listview.h"
#include "mlk_listheader.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"
#include "mlk_columnitem.h"
#include "mlk_str.h"
#include "mlk_key.h"

#include "def_config.h"
#include "def_draw_toollist.h"
#include "def_tool.h"
#include "def_canvaskey.h"

#include "def_draw_ptr.h"
#include "draw_rule.h"

#include "apphelp.h"
#include "widget_func.h"

#include "trid.h"
#include "trid_dialog.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mListView *list;
}_dialog;

enum
{
	WID_BTT_CLEAR_ALL = 100,
	WID_BTT_HELP
};

#define _KEY_NONE  256

//----------------------


//=======================
// リスト作成
//=======================


/* グループ項目追加 */

static void _add_group(_dialog *p,int trid)
{
	mListViewAddItem(p->list,
		MLK_TR2(TRGROUP_DLG_KEYOPT, trid), -1, MCOLUMNITEM_F_HEADER, -1);
}

/* 項目追加
 *
 * trid: -1 で str の文字列を使う */

static void _add_item(_dialog *p,int trid,int cmdid,mStr *str)
{
	uint8_t *pkey;
	int i,key;

	if(trid >= 0)
		mStrSetText(str, MLK_TR(trid));

	mStrAppendChar(str, '\t');

	//キーを検索

	pkey = APPCONF->canvaskey;
	key = _KEY_NONE;

	for(i = 0; i < 256; i++, pkey++)
	{
		if(*pkey == cmdid)
		{
			key = i;
			
			if(!mKeycodeGetName(str + 1, mKeycodeToKey(key)))
				mStrSetText(str + 1, "<no name>");

			mStrAppendStr(str, str + 1);
			break;
		}
	}

	//追加

	mListViewAddItem_text_copy_param(p->list, str->buf, (key << 8) | cmdid);
}

/* グループ + 項目を追加
 *
 * trid_group: グループ名の文字列ID
 * trgroup_item: 項目名の文字列グループID
 * trid_item_top: 項目名の文字列ID先頭
 * keycmd_top: キーコマンド値の先頭
 * num: 項目数
 * str: 作業用 */

static void _add_group_item(_dialog *p,int trid_group,
	int trgroup_item,int trid_item_top,int keycmd_top,int num,mStr *str)
{
	int i;
	
	_add_group(p, trid_group);

	MLK_TRGROUP(trgroup_item);

	for(i = 0; i < num; i++)
		_add_item(p, trid_item_top + i, keycmd_top + i, str);
}

/* 初期リストセット */

static void _set_list(_dialog *p)
{
	mStr str[2];
	int i;

	mStrArrayInit(str, 2);

	//ツール変更

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_CMD_TOOL, TRGROUP_TOOL,
		0, CANVASKEY_CMD_TOOL, TOOL_NUM, str);

	//描画タイプ変更

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_CMD_DRAWTYPE, TRGROUP_TOOL_SUB,
		0, CANVASKEY_CMD_DRAWTYPE, TOOLSUB_DRAW_NUM, str);

	//定規

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_CMD_RULE, TRGROUP_RULE,
		1, CANVASKEY_CMD_RULE, DRAW_RULE_TYPE_NUM - 1, str);

	//他コマンド

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_CMD_OTHER, TRGROUP_DLG_KEYOPT,
		TRID_DLG_KEYOPT_CMD_OTHER_TOP, CANVASKEY_CMD_OTHER, CANVASKEY_CMD_OTHER_NUM, str);

	//+キーでツール動作 (一部ツールは除く)

	_add_group(p, TRID_DLG_KEYOPT_GROUP_OP_TOOL);

	MLK_TRGROUP(TRGROUP_TOOL);

	for(i = 0; i < TOOL_NUM; i++)
	{
		if(IS_TOOL_OPCMD_EXCLUDE(i)) continue;
	
		_add_item(p, i, CANVASKEY_OP_TOOL + i, str);
	}

	//+キーで描画タイプ動作

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_OP_DRAWTYPE, TRGROUP_TOOL_SUB,
		0, CANVASKEY_OP_DRAWTYPE, TOOLSUB_DRAW_NUM, str);

	//+キーで選択範囲ツール動作

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_OP_SELECT, TRGROUP_DLG_KEYOPT,
		TRID_DLG_KEYOPT_OP_SELECT_TOP, CANVASKEY_OP_SELECT, TOOLSUB_SEL_NUM, str);

	//+キーで登録ツール動作

	_add_group(p, TRID_DLG_KEYOPT_GROUP_OP_REGIST);

	for(i = 0; i < DRAW_TOOLLIST_REG_NUM; i++)
	{
		mStrSetFormat(str, " tool%d", i + 1);

		_add_item(p, -1, CANVASKEY_OP_REGIST + i, str);
	}

	//+キーで他動作

	_add_group_item(p, TRID_DLG_KEYOPT_GROUP_OP_OTHER, TRGROUP_DLG_KEYOPT,
		TRID_DLG_KEYOPT_OP_OTHER_TOP, CANVASKEY_OP_OTHER, CANVASKEY_OP_OTHER_NUM, str);

	mStrArrayFree(str, 2);
}


//=======================
//
//=======================


/* キー名をセット */

static void _set_item_keyname(_dialog *p,mColumnItem *pi,int key)
{
	mStr str = MSTR_INIT;

	if(key == _KEY_NONE)
		mListViewSetItemColumnText(p->list, pi, 1, NULL);
	else
	{
		key = mKeycodeToKey(key);
		
		if(!mKeycodeGetName(&str, key))
			mStrSetText(&str, "<no name>");

		mListViewSetItemColumnText(p->list, pi, 1, str.buf);

		mStrFree(&str);
	}
}

/* 指定キーが使われているか */

static mlkbool _check_key(_dialog *p,int key)
{
	mColumnItem *pi;

	pi = mListViewGetTopItem(p->list);

	for(; pi; pi = (mColumnItem *)pi->i.next)
	{
		if(pi->param != -1 && (pi->param >> 8) == key)
			return TRUE;
	}

	return FALSE;
}

/* リスト上でのキー押し時 */

static void _list_keydown(_dialog *p,int key,int rawcode)
{
	mColumnItem *pi;

	//選択アイテム

	pi = mListViewGetFocusItem(p->list);

	if(!pi || pi->param == -1)
		return;

	//設定できないキーの場合、"なし"とする

	if(key == MKEY_SPACE || key == MKEY_KP_SPACE
		|| key == MKEY_ESCAPE
		|| key == MKEY_MENU
		|| key == MKEY_SCROLL_LOCK
		|| key == MKEY_NUM_LOCK
		|| (key >= MKEY_SHIFT_L && key <= MKEY_HYPER_R))
	{
		rawcode = _KEY_NONE;
	}

	//キーが変更された

	if((pi->param >> 8) != rawcode)
	{
		//すでに使われているキーか

		if(rawcode != _KEY_NONE && _check_key(p, rawcode))
		{
			mMessageBoxOK(MLK_WINDOW(p),
				MLK_TR2(TRGROUP_DLG_KEYOPT, TRID_DLG_KEYOPT_MES_EXIST_KEY));
			return;
		}
	
		//
		
		pi->param = (rawcode << 8) | (pi->param & 255);

		//リストテキスト

		_set_item_keyname(p, pi, rawcode);
	}
}

/* すべてクリア */

static void _cmd_clear_all(_dialog *p)
{
	mColumnItem *pi;

	pi = mListViewGetTopItem(p->list);

	for(; pi; pi = (mColumnItem *)pi->i.next)
	{
		if(pi->param != -1)
		{
			pi->param = (_KEY_NONE << 8) | (pi->param & 255);

			mListViewSetItemColumnText(p->list, pi, 1, NULL);
		}
	}
}


//=========================
//
//=========================


/* リストビューのイベント */

static int _list_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_KEYDOWN)
	{
		if(ev->key.raw_code < 256)
			_list_keydown((_dialog *)wg->toplevel, ev->key.key, ev->key.raw_code);

		return 1;
	}

	return mListViewHandle_event(wg, ev);
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		switch(ev->notify.id)
		{
			//すべてクリア
			case WID_BTT_CLEAR_ALL:
				_cmd_clear_all((_dialog *)wg);
				break;
			//ヘルプ
			case WID_BTT_HELP:
				AppHelp_message(MLK_WINDOW(wg), HELP_TRGROUP_SINGLE, HELP_TRID_CANVASKEY);
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//===================


/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent)
{
	_dialog *p;
	mWidget *ct;
	mListView *list;
	mListHeader *lh;

	MLK_TRGROUP(TRGROUP_DLG_KEYOPT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_DLG_KEYOPT_TITLE_CANVAS), _event_handle);

	if(!p) return NULL;

	//---- リスト

	list = p->list = mListViewCreate(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_MULTI_COLUMN | MLISTVIEW_S_HAVE_HEADER | MLISTVIEW_S_GRID_COL,
		MSCROLLVIEW_S_HORZVERT_FRAME);

	list->wg.event = _list_event_handle;
	list->wg.facceptkey = MWIDGET_ACCEPTKEY_ENTER;

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 25, 20);

	//ヘッダ

	lh = mListViewGetHeaderWidget(list);

	mListHeaderAddItem(lh, "name", 100, 0, 0);
	mListHeaderAddItem(lh, "key", 100, MLISTHEADER_ITEM_F_EXPAND, 0);

	//リストセット

	_set_list(p);

	mListViewSetColumnWidth_auto(list, 0);

	//----- ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, MLF_EXPAND_W, MLK_MAKE32_4(0,12,0,0));

	mButtonCreate(ct, WID_BTT_CLEAR_ALL, MLF_EXPAND_X, 0, 0, MLK_TR(TRID_DLG_KEYOPT_ALL_CLEAR));

	widget_createHelpButton(ct, WID_BTT_HELP, MLF_MIDDLE, MLK_MAKE32_4(0,0,5,0));
	
	mButtonCreate(ct, MLK_WID_OK, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));
	mButtonCreate(ct, MLK_WID_CANCEL, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	return p;
}

/* データセット */

static void _set_data(_dialog *p)
{
	mColumnItem *pi;
	uint8_t *buf = APPCONF->canvaskey;
	int key;

	mMemset0(buf, 256);

	//

	pi = mListViewGetTopItem(p->list);

	for(; pi; pi = (mColumnItem *)pi->i.next)
	{
		if(pi->param != -1)
		{
			key = pi->param >> 8;

			if(key != _KEY_NONE)
				buf[key] = pi->param & 255;
		}
	}
}

/** キャンバスキーダイアログ実行 */

mlkbool CanvasKeyDlg_run(mWindow *parent)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		_set_data(p);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

