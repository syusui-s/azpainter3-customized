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
 * 環境設定のページ: ボタン操作
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_pager.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_listview.h"
#include "mlk_listheader.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"
#include "mlk_columnitem.h"
#include "mlk_str.h"

#include "def_config.h"
#include "def_canvaskey.h"
#include "def_tool.h"

#include "widget_func.h"

#include "trid.h"
#include "trid_dialog.h"

#include "pv_envoptdlg.h"


//--------------------

#define _REGTOOL_NUM  8

//登録ツール名
static const char *g_regtool_name[] = {
	"tool1","tool2","tool3","tool4","tool5","tool6","tool7","tool8"
};

//--------------------


//**********************************
// コマンド選択ダイアログ
//**********************************


/* リストのセット */

static void _cmdsel_setlist(mListView *list)
{
	int i;

	//指定なし

	mListViewAddItem_text_static_param(list, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_CMD_NONE), 0);

	//ツール動作
	// [!] 一部ツールは除く

	mListViewAddItem(list, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_CMD_GROUP_TOOL), -1, MCOLUMNITEM_F_HEADER, -1);

	MLK_TRGROUP(TRGROUP_TOOL);

	for(i = 0; i < TOOL_NUM; i++)
	{
		if(IS_TOOL_OPCMD_EXCLUDE(i)) continue;

		mListViewAddItem_text_static_param(list, MLK_TR(i), CANVASKEY_OP_TOOL + i);
	}

	//登録ツール動作

	mListViewAddItem(list, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_CMD_GROUP_REGIST), -1, MCOLUMNITEM_F_HEADER, -1);

	for(i = 0; i < _REGTOOL_NUM; i++)
		mListViewAddItem_text_static_param(list, g_regtool_name[i], CANVASKEY_OP_REGIST + i);

	//ほか動作

	mListViewAddItem(list, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_CMD_GROUP_OTHER_OP), -1, MCOLUMNITEM_F_HEADER, -1);

	MLK_TRGROUP(TRGROUP_DLG_KEYOPT);

	for(i = 0; i < CANVASKEY_OP_OTHER_NUM; i++)
		mListViewAddItem_text_static_param(list, MLK_TR(TRID_DLG_KEYOPT_OP_OTHER_TOP + i), CANVASKEY_OP_OTHER + i);

	//コマンド他

	mListViewAddItem(list, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_CMD_GROUP_OTHER_CMD), -1, MCOLUMNITEM_F_HEADER, -1);

	for(i = 0; i < CANVASKEY_CMD_OTHER_NUM; i++)
		mListViewAddItem_text_static_param(list, MLK_TR(TRID_DLG_KEYOPT_CMD_OTHER_TOP + i), CANVASKEY_CMD_OTHER + i);
}

/* ダイアログ作成 */

static mDialog *_cmdsel_create(mWindow *parent,int defsel)
{
	mDialog *p;
	mListView *list;
	mColumnItem *item;

	p = widget_createDialog(parent, 0, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_SELECT_COMMAND),
		mDialogEventDefault_okcancel);

	if(!p) return NULL;

	//list

	list = mListViewCreate(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0,
		0, MSCROLLVIEW_S_VERT | MSCROLLVIEW_S_FRAME);

	p->wg.param1 = (intptr_t)list;

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 0, 20);

	_cmdsel_setlist(list);

	mListViewSetAutoWidth(list, TRUE);

	item = mListViewSetFocusItem_param(list, defsel);

	if(item)
		mListViewScrollToItem(list, item, 0);

	//OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,10,0,0));

	return p;
}

/* コマンド選択ダイアログ
 *
 * return: コマンド番号。-1 でキャンセル */

static int _cmdsel_run(mWindow *parent,int defsel)
{
	mDialog *p;
	int ret;

	p = _cmdsel_create(parent, defsel);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(!ret)
		ret = -1;
	else
	{
		ret = mListViewGetItemParam((mListView *)p->wg.param1, -1);

		if(ret < 0) ret = 0;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


//**********************************
// ページ
//**********************************

/* <mPager>
 * param1 = mListView *
 * param2 = デバイスタイプ */

enum
{
	WID_CK_DEVICE_NORMAL = 100,
	WID_CK_DEVICE_PENTAB,
	WID_LIST,
	WID_BTT_SEL_CMD,
	WID_BTT_HELP
};


/* 番号から、コマンド名の取得 */

static const char *_get_cmdname(int no)
{
	if(no == CANVASKEY_NONE)
		return "----";
	else if(no >= CANVASKEY_OP_TOOL && no < CANVASKEY_OP_TOOL + TOOL_NUM)
	{
		//ツール動作
		return MLK_TR2(TRGROUP_TOOL, no - CANVASKEY_OP_TOOL);
	}
	else if(no >= CANVASKEY_OP_REGIST && no < CANVASKEY_OP_REGIST + _REGTOOL_NUM)
	{
		//登録ツール動作
		return g_regtool_name[no - CANVASKEY_OP_REGIST];
	}
	else if(no >= CANVASKEY_OP_OTHER && no < CANVASKEY_OP_OTHER + CANVASKEY_OP_OTHER_NUM)
	{
		//ほか動作
		return MLK_TR2(TRGROUP_DLG_KEYOPT, TRID_DLG_KEYOPT_OP_OTHER_TOP + no - CANVASKEY_OP_OTHER);
	}
	else if(no >= CANVASKEY_CMD_OTHER && no < CANVASKEY_CMD_OTHER + CANVASKEY_CMD_OTHER_NUM)
	{
		//コマンド
		return MLK_TR2(TRGROUP_DLG_KEYOPT, TRID_DLG_KEYOPT_CMD_OTHER_TOP + no - CANVASKEY_CMD_OTHER);
	}
	else
		return NULL;
}

/* リストのコマンド名セット */

static void _set_list_cmd(mListView *list,int type,EnvOptData *dat)
{
	mColumnItem *pi;
	uint8_t *ps;

	ps = (type)? dat->btt_pentab: dat->btt_default;

	pi = mListViewGetTopItem(list);

	for(; pi; pi = MLK_COLUMNITEM(pi->i.next), ps++)
	{
		mListViewSetItemColumnText(list, pi, 1, _get_cmdname(*ps));
	}
}

/* コマンド選択 */

static void _select_cmd(mListView *list,int type,EnvOptData *dat)
{
	mColumnItem *pi;
	uint8_t *buf;
	int no;

	pi = mListViewGetFocusItem(list);
	if(!pi) return;

	buf = (type)? dat->btt_pentab: dat->btt_default;

	no = _cmdsel_run(list->wg.toplevel, buf[pi->param]);
	if(no < 0) return;

	buf[pi->param] = no;

	mListViewSetItemColumnText(list, pi, 1, _get_cmdname(no));
}

/* mLabel のイベント */

static int _label_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& ev->pt.act == MEVENT_POINTER_ACT_PRESS)
	{
		if(ev->pt.btt <= CONFIG_POINTERBTT_MAXNO)
		{
			mListViewSetFocusItem_index((mListView *)wg->param1, ev->pt.btt);
			mListViewScrollToFocus((mListView *)wg->param1);
		}
	}

	return 1;
}

/* イベント */

static int _page_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		EnvOptData *dat = (EnvOptData *)mPagerGetDataPtr(p);

		switch(ev->notify.id)
		{
			//リスト
			case WID_LIST:
				if(ev->notify.notify_type == MLISTVIEW_N_ITEM_L_DBLCLK)
					_select_cmd((mListView *)p->wg.param1, p->wg.param2, dat);
				break;
			//デバイス
			case WID_CK_DEVICE_NORMAL:
			case WID_CK_DEVICE_PENTAB:
				p->wg.param2 = ev->notify.id - WID_CK_DEVICE_NORMAL;
				
				_set_list_cmd((mListView *)p->wg.param1, p->wg.param2, dat);
				break;
			//コマンドの選択
			case WID_BTT_SEL_CMD:
				_select_cmd((mListView *)p->wg.param1, p->wg.param2, dat);
				break;
			//ヘルプ
			case WID_BTT_HELP:
				mMessageBoxOK(p->wg.toplevel, MLK_TR2(TRGROUP_DLG_ENVOPT, TRID_BTT_HELP_GETBTT));
				break;
		}
	}

	return 1;
}

/** "ボタン操作" ページ作成 */

mlkbool EnvOpt_createPage_btt(mPager *p,mPagerInfo *info)
{
	EnvOptData *dat;
	mWidget *ct,*wg;
	mListView *list;
	mListHeader *lh;
	int i;
	mStr str = MSTR_INIT;

	info->event = _page_event;

	dat = (EnvOptData *)mPagerGetDataPtr(p);

	//-------

	//デバイス

	mCheckButtonCreate(MLK_WIDGET(p), WID_CK_DEVICE_NORMAL, 0, 0, MCHECKBUTTON_S_RADIO,
		MLK_TR(TRID_BTT_DEVICE_DEFAULT), TRUE);

	mCheckButtonCreate(MLK_WIDGET(p), WID_CK_DEVICE_PENTAB, 0, MLK_MAKE32_4(0,3,0,0), MCHECKBUTTON_S_RADIO,
		MLK_TR(TRID_BTT_DEVICE_PENTAB), FALSE);

	//リスト

	list = mListViewCreate(MLK_WIDGET(p), WID_LIST, MLF_EXPAND_WH, MLK_MAKE32_4(0,6,0,0),
		MLISTVIEW_S_MULTI_COLUMN | MLISTVIEW_S_HAVE_HEADER | MLISTVIEW_S_GRID_COL,
		MSCROLLVIEW_S_HORZVERT_FRAME);

	p->wg.param1 = (intptr_t)list;

	lh = mListViewGetHeaderWidget(list);

	mListHeaderAddItem(lh, MLK_TR(TRID_BTT_BTTNAME), 100, 0, 0);
	mListHeaderAddItem(lh, MLK_TR(TRID_BTT_COMMAND), 100, MLISTHEADER_ITEM_F_EXPAND, 0);

	//---- 下

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,6,0,0));

	//コマンドの選択

	mButtonCreate(ct, WID_BTT_SEL_CMD, MLF_EXPAND_X, 0, 0, MLK_TR(TRID_BTT_SELECT_COMMAND));

	//ボタンの取得

	wg = (mWidget *)mLabelCreate(ct, MLF_EXPAND_H, 0, MLABEL_S_BORDER | MLABEL_S_MIDDLE, MLK_TR(TRID_BTT_GET_BUTTON));

	wg->param1 = (intptr_t)list;
	wg->fevent |= MWIDGET_EVENT_POINTER;
	wg->event = _label_event;

	widget_createHelpButton(ct, WID_BTT_HELP, MLF_MIDDLE, 0);

	//---- リストのセット

	for(i = 0; i < CONFIG_POINTERBTT_NUM; i++)
	{
		if(i <= 7)
			mStrSetText(&str, MLK_TR(TRID_BUTTON_NAME + i));
		else
			mStrSetFormat(&str, "btt%d", i);
	
		mStrAppendFormat(&str, "\t%s",  _get_cmdname(dat->btt_default[i]));
	
		mListViewAddItem_text_copy_param(list, str.buf, i);
	}

	mStrFree(&str);

	mListViewSetColumnWidth_auto(list, 0);

	return TRUE;
}

