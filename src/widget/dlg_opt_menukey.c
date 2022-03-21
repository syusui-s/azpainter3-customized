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
 * メニューのキー設定
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_button.h"
#include "mlk_combobox.h"
#include "mlk_conflistview.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"
#include "mlk_menu.h"
#include "mlk_accelerator.h"
#include "mlk_tree.h"
#include "mlk_str.h"
#include "mlk_iniwrite.h"

#include "def_macro.h"
#include "def_widget.h"
#include "def_mainwin.h"

#include "widget_func.h"

#include "trid.h"
#include "trid_dialog.h"
#include "trid_mainmenu.h"


/*
  - 起動時に設定ファイルから読み込み、
    メニューにショートカットキーがセットされる。
  - 設定ダイアログでは、メニューからショートカットキーを取得し、
    作業用としてツリーデータを作成する。
  - 確定時、ツリーデータからメニューにショートカットキーを設定し、
    設定ファイルに保存する。
*/

//-------------------------

//ツリーアイテムデータ

typedef struct
{
	mTreeItem i;
	const char *text;	//項目名 (メニューのテキストと同じポインタを使う)
	int type,			//項目タイプ
		cmdid;			//メニューのコマンドID
	uint32_t key,		//現在のキー
		key_old;		//元のキー (ダイアログを開いた時点の値)
}_treeitem;

enum
{
	_ITEM_TYPE_NORMAL,		//キー設定可能なアイテム
	_ITEM_TYPE_SUBMENU_PARENT	//サブメニューの親アイテム (トップ項目含む)
};

//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mTree tree;
	_treeitem *seltop;	//選択されているトップ項目

	mComboBox *cb_top;
	mConfListView *list;
}_dialog;

enum
{
	WID_CB_TOP = 100,
	WID_LIST,
	WID_BTT_CLEAR_KEY,
	WID_BTT_CLEAR_ALL
};

//----------------------


//==========================
// ツリーデータ
//==========================


/* メニューアイテムから、ツリーアイテム追加
 *
 * parent: 親のツリーアイテム (NULL でルート)
 * mi: 元となるメニューアイテム
 * dst_item: NULL 以外で、作成されたツリーアイテムのポインタが入る
 * dst_submenu: NULL 以外で、mi のサブメニューのポインタが入る
 * return: FALSE で作成に失敗 */

static mlkbool _append_treeitem(_dialog *p,_treeitem *parent,mMenuItem *mi,
	_treeitem **dst_item,mMenu **dst_submenu)
{
	_treeitem *pi;
	mMenuItemInfo *info;

	info = mMenuItemGetInfo(mi);

	//リストに追加しないもの (セパレータ, 最近使ったファイル)

	if((info->flags & MMENUITEM_F_SEP)
		|| info->id == TRMENU_FILE_RECENTFILE)
	{
		if(dst_submenu) *dst_submenu = NULL;
		return TRUE;
	}

	//追加

	pi = (_treeitem *)mTreeAppendNew(&p->tree, MTREEITEM(parent), sizeof(_treeitem));
	if(!pi) return FALSE;
	
	pi->cmdid = info->id;
	pi->key = pi->key_old = info->shortcut_key;
	pi->text = info->text;

	if(info->submenu)
		pi->type = _ITEM_TYPE_SUBMENU_PARENT;

	if(dst_item) *dst_item = pi;
	if(dst_submenu) *dst_submenu = info->submenu;

	return TRUE;
}

/* メニューから、ツリーデータ作成 */

static void _create_treedat(_dialog *p)
{
	mMenuItem *mi_top,*mi,*mi2;
	_treeitem *ti_top,*ti;
	mMenu *submenu,*submenu2;

	mi_top = mMenuGetTopItem(APPWIDGET->mainwin->menu_main);

	for(; mi_top; mi_top = mMenuItemGetNext(mi_top))
	{
		//トップ
		
		if(!_append_treeitem(p, NULL, mi_top, &ti_top, &submenu))
			return;

		if(!submenu) continue;

		//メイン項目

		for(mi = mMenuGetTopItem(submenu); mi; mi = mMenuItemGetNext(mi))
		{
			if(!_append_treeitem(p, ti_top, mi, &ti, &submenu2))
				return;

			//サブメニュー

			if(submenu2)
			{
				for(mi2 = mMenuGetTopItem(submenu2); mi2; mi2 = mMenuItemGetNext(mi2))
				{
					if(!_append_treeitem(p, ti, mi2, NULL, NULL))
						return;
				}
			}
		}
	}
}


//==========================
// sub
//==========================


/* トップ項目のリストをセット */

static void _set_cb_top(_dialog *p)
{
	mComboBox *cb = p->cb_top;
	_treeitem *pi;

	for(pi = (_treeitem *)p->tree.top; pi; pi = (_treeitem *)pi->i.next)
		mComboBoxAddItem_static(cb, pi->text, (intptr_t)pi);

	mComboBoxSetSelItem_atIndex(cb, 0);

	p->seltop = (_treeitem *)p->tree.top;
}

/* リスト項目をセット */

static void _set_list(_dialog *p)
{
	mConfListView *list = p->list;
	_treeitem *pi,*pisub;

	mListViewDeleteAllItem(MLK_LISTVIEW(list));

	for(pi = (_treeitem *)p->seltop->i.first; pi; pi = (_treeitem *)pi->i.next)
	{
		if(pi->type == _ITEM_TYPE_SUBMENU_PARENT)
			mConfListView_addItem_header(list, pi->text);
		else
			mConfListView_addItem_accel(list, pi->text, pi->key, (intptr_t)pi);

		if(pi->i.first)
		{
			for(pisub = (_treeitem *)pi->i.first; pisub; pisub = (_treeitem *)pisub->i.next)
				mConfListView_addItem_accel(list, pisub->text, pisub->key, (intptr_t)pisub);
		}
	}

	mConfListView_setColumnWidth_name(list);
}

/* キーの変更時 */

static void _change_key(_dialog *p,mColumnItem *item,_treeitem *titem)
{
	_treeitem *pi;
	uint32_t key;
	mStr str = MSTR_INIT;

	key = mConfListViewItem_getAccelKey(item);

	titem->key = key;
	
	if(!key) return;

	//同じキーが存在するか

	for(pi = (_treeitem *)p->tree.top; pi; pi = (_treeitem *)mTreeItemGetNext(MTREEITEM(pi)))
	{
		if(pi != titem && pi->key == key) break;
	}

	//

	if(pi)
	{
		mStrSetFormat(&str, "%s\n\n[%s]",
			MLK_TR2(TRGROUP_DLG_KEYOPT, TRID_DLG_KEYOPT_MES_EXIST_KEY), pi->text);
	
		mMessageBoxOK(MLK_WINDOW(p), str.buf);

		mStrFree(&str);
	}
}

/* キーをクリア */

static void _clear_key(_dialog *p)
{
	mColumnItem *pi;
	_treeitem *ti;

	pi = mListViewGetFocusItem(MLK_LISTVIEW(p->list));
	if(pi)
	{
		//ツリーデータ
		
		ti = (_treeitem *)mConfListViewItem_getParam(pi);

		if(ti)
			ti->key = 0;

		//リスト
	
		mConfListView_setFocusValue_zero(p->list);
	}
}

/* すべてクリア */

static void _clear_all_key(_dialog *p)
{
	_treeitem *pi;

	//キーをすべて 0 に

	for(pi = (_treeitem *)p->tree.top; pi; pi = (_treeitem *)mTreeItemGetNext(MTREEITEM(pi)))
		pi->key = 0;

	//リスト

	mConfListView_clearAll_accel(p->list);
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//リスト
			case WID_LIST:
				if(ev->notify.notify_type == MCONFLISTVIEW_N_CHANGE_VAL)
					_change_key(p, (mColumnItem *)ev->notify.param1, (_treeitem *)ev->notify.param2);
				break;
			//トップ項目
			case WID_CB_TOP:
				if(ev->notify.notify_type == MCOMBOBOX_N_CHANGE_SEL)
				{
					p->seltop = (_treeitem *)ev->notify.param2;

					_set_list(p);
				}
				break;
			//キーをクリア
			case WID_BTT_CLEAR_KEY:
				_clear_key(p);
				break;
			//すべてクリア
			case WID_BTT_CLEAR_ALL:
				_clear_all_key(p);
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//=====================


/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent)
{
	_dialog *p;
	mWidget *ct;

	MLK_TRGROUP(TRGROUP_DLG_KEYOPT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_DLG_KEYOPT_TITLE_MENU), _event_handle);

	if(!p) return NULL;
		
	//------- ウィジェット

	//トップ項目

	p->cb_top = mComboBoxCreate(MLK_WIDGET(p), WID_CB_TOP, MLF_EXPAND_W, 0, 0);

	//リスト

	p->list = mConfListViewNew(MLK_WIDGET(p), 0, 0);

	p->list->wg.id = WID_LIST;
	
	mWidgetSetMargin_pack4(MLK_WIDGET(p->list), MLK_MAKE32_4(0,5,0,0));
	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->list), 28, 18);

	//キークリア

	mButtonCreate(MLK_WIDGET(p), WID_BTT_CLEAR_KEY, MLF_RIGHT, MLK_MAKE32_4(0,5,0,0), 0,
		MLK_TR(TRID_DLG_KEYOPT_KEY_CLEAR));

	//すべてクリア/OK/cancel

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, MLF_EXPAND_W, MLK_MAKE32_4(0,12,0,0));

	mButtonCreate(ct, WID_BTT_CLEAR_ALL, MLF_EXPAND_X, MLK_MAKE32_4(0,0,10,0), 0, MLK_TR(TRID_DLG_KEYOPT_ALL_CLEAR));
	mButtonCreate(ct, MLK_WID_OK, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));
	mButtonCreate(ct, MLK_WID_CANCEL, 0, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	//------- 初期化

	_create_treedat(p);

	//トップ項目

	_set_cb_top(p);

	//リスト

	_set_list(p);

	return p;
}

/* アクセラレータ/メニューにキーをセット */

static void _set_acckey(mTree *tree)
{
	mAccelerator *accel;
	mMenu *mainmenu;
	_treeitem *pi;

	accel = mToplevelGetAccelerator(MLK_TOPLEVEL(APPWIDGET->mainwin));
	mainmenu = APPWIDGET->mainwin->menu_main;

	mAcceleratorClear(accel);

	for(pi = (_treeitem *)tree->top; pi; pi = (_treeitem *)mTreeItemGetNext(MTREEITEM(pi)))
	{
		if(pi->type == _ITEM_TYPE_NORMAL)
		{
			//アクセラレータ
			
			if(pi->key)
				mAcceleratorAdd(accel, pi->cmdid, pi->key, NULL);

			//メニュー (キーが変更された場合のみ)

			if(pi->key != pi->key_old)
				mMenuSetItemShortcutKey(mainmenu, pi->cmdid, pi->key);
		}
	}
}

/* 設定ファイルに書き込み */

static void _write_config(mTree *tree)
{
	FILE *fp;
	_treeitem *pi;

	fp = mIniWrite_openFile_join(mGuiGetPath_config_text(), CONFIG_FILENAME_MENUKEY);
	if(!fp) return;

	mIniWrite_putGroup(fp, "menukey");

	for(pi = (_treeitem *)tree->top; pi; pi = (_treeitem *)mTreeItemGetNext(MTREEITEM(pi)))
	{
		if(pi->type == _ITEM_TYPE_NORMAL && pi->key)
			mIniWrite_putHex_keyno(fp, pi->cmdid, pi->key);
	}

	fclose(fp);
}

/** 実行 */

mlkbool MenuKeyOptDlg_run(mWindow *parent)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		_set_acckey(&p->tree);

		_write_config(&p->tree);
	}

	mTreeDeleteAll(&p->tree);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

