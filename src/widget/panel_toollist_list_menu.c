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
 * [panel] ツールリストのメニュー処理
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_menu.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"

#include "def_draw.h"
#include "def_draw_toollist.h"

#include "toollist.h"

#include "draw_toollist.h"

#include "dlg_toollist.h"
#include "pv_toollist_page.h"
#include "widget_func.h"

#include "trid.h"


//---------------------

enum
{
	TRID_MENU_ADD_GROUP,

	//グループメニュ
	TRID_MENU_GROUP_EDIT = 100,
	TRID_MENU_GROUP_INSERT,
	TRID_MENU_GROUP_DELETE,

	//アイテムメニュー
	TRID_MENU_INSERT_BRUSH = 200,
	TRID_MENU_INSERT_TOOL,
	TRID_MENU_COPY,
	TRID_MENU_PASTE,
	TRID_MENU_OPTION,
	TRID_MENU_TOOL,
	TRID_MENU_DELETE,
	TRID_MENU_REGIST,
	TRID_MENU_TOOL_SETOPT,
	TRID_MENU_TOOL_VIEW,
	
	TRID_MENU_REGIST_NONE = 250,
	TRID_MENU_REGIST_RELEASE_ALL
};

//---------------------

//追加対象ツール
static const uint8_t g_add_tool[] = {
	TOOL_DOTPEN, TOOL_DOTPEN_ERASE, TOOL_FINGER, TOOL_FILL_POLYGON, TOOL_FILL_POLYGON_ERASE,
	TOOL_FILL, TOOL_FILL_ERASE, TOOL_GRADATION, TOOL_MOVE, TOOL_SELECT_FILL,
	TOOL_SELECT, TOOL_CANVAS_MOVE, TOOL_CANVAS_ROTATE, TOOL_SPOIT,
	255
};

//グループ:メニューID
static const uint16_t g_menudat_group[] = {
	TRID_MENU_GROUP_EDIT, MMENU_ARRAY16_SEP,
	TRID_MENU_GROUP_INSERT, TRID_MENU_GROUP_DELETE,
	MMENU_ARRAY16_END
};

//アイテム:メニューID
static const uint16_t g_menudat_item[] = {
	TRID_MENU_INSERT_BRUSH, TRID_MENU_INSERT_TOOL, MMENU_ARRAY16_SEP,
	TRID_MENU_COPY, TRID_MENU_PASTE, MMENU_ARRAY16_SEP,
	TRID_MENU_DELETE, MMENU_ARRAY16_SEP,
	TRID_MENU_OPTION,

	TRID_MENU_TOOL,
	MMENU_ARRAY16_SUB_START,
		TRID_MENU_TOOL_SETOPT, TRID_MENU_TOOL_VIEW,
	MMENU_ARRAY16_SUB_END,
	MMENU_ARRAY16_SEP,

	TRID_MENU_REGIST,
	MMENU_ARRAY16_END
};

//---------------------



//======================
// コマンド
//======================


/* グループの追加/挿入
 *
 * ins: NULL で追加 */

static void _cmd_add_group(ToolListPage *p,ToolListGroupItem *ins)
{
	ToolListDlg_groupOpt dat;

	mMemset0(&dat, sizeof(ToolListDlg_groupOpt));

	dat.colnum = 2;

	if(ToolListDialog_groupOption(p->wg.toplevel, &dat, FALSE))
	{
		ToolList_insertGroup(&APPDRAW->tlist->list_group, ins, dat.strname.buf, dat.colnum);
	
		mStrFree(&dat.strname);

		ToolListPage_updateAll(p);
	}
}

/* グループの設定 */

static void _cmd_group_option(ToolListPage *p,ToolListGroupItem *item)
{
	ToolListDlg_groupOpt dat;

	mMemset0(&dat, sizeof(ToolListDlg_groupOpt));

	mStrSetText(&dat.strname, item->name);

	dat.colnum = item->colnum;

	//

	if(ToolListDialog_groupOption(p->wg.toplevel, &dat, TRUE))
	{
		mStrdup_free(&item->name, dat.strname.buf);

		item->colnum = dat.colnum;
		
		ToolListPage_updateAll(p);
	}

	mStrFree(&dat.strname);
}


//==========================
// 終端部分のメニュー
//==========================


/** 終端の空部分でのメニュー */

void ToolListPage_runMenu_empty(ToolListPage *p,int x,int y)
{
	mMenu *menu;
	mMenuItem *mi;
	int id;

	menu = mMenuNew();

	mMenuAppendText(menu, TRID_MENU_ADD_GROUP, MLK_TR2(TRGROUP_PANEL_TOOLLIST_MENU, TRID_MENU_ADD_GROUP));

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(id < 0) return;

	//-------

	_cmd_add_group(p, NULL);
}


//==========================
// グループメニュー
//==========================


/** グループメニュー */

void ToolListPage_runMenu_group(ToolListPage *p,int x,int y,ToolListGroupItem *group)
{
	mMenu *menu;
	mMenuItem *mi;
	int id;

	MLK_TRGROUP(TRGROUP_PANEL_TOOLLIST_MENU);

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menudat_group);

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(id < 0) return;

	//-------

	switch(id)
	{
		//編集
		case TRID_MENU_GROUP_EDIT:
			_cmd_group_option(p, group);
			break;
		//挿入
		case TRID_MENU_GROUP_INSERT:
			_cmd_add_group(p, group);
			break;
		//削除
		case TRID_MENU_GROUP_DELETE:
			if(widget_message_confirm(p->wg.toplevel,
				MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_DELETE_NO_RESTORE)))
			{
				drawToolList_deleteGroup(group);
				
				ToolListPage_updateAll(p);

				//選択アイテムが削除された時
				if(!APPDRAW->tlist->selitem)
					ToolListPage_notify_changeValue(p);
			}
			break;
	}
}


//==========================
// アイテムメニュー
//==========================

enum
{
	_ITEM_MENUID_TOOL = 1000,
	_ITEM_MENUID_REGIST = 1100
};


/* 新規ブラシ */

static mlkbool _cmd_new_brush(ToolListPage *p,_POINTINFO *info)
{
	mStr str = MSTR_INIT;
	const char *text;
	ToolListItem *item;

	//名前

	text = MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME);

	if(!mSysDlg_inputText(p->wg.toplevel, text, text, MSYSDLG_INPUTTEXT_F_NOT_EMPTY, &str))
		return FALSE;

	//挿入

	item = ToolList_insertItem_brush(info->group, info->item, str.buf);

	mStrFree(&str);

	if(!item) return FALSE;

	//

	drawToolList_setSelItem(APPDRAW, item);

	ToolListPage_notify_changeValue(p);

	return TRUE;
}

/* アイテムメニュー実行 */

static int _runmenu_item(ToolListPage *p,int x,int y,_POINTINFO *info)
{
	mMenu *menu,*sub;
	mMenuItem *mi;
	int i,id;
	char m[10];

	menu = mMenuNew();

	MLK_TRGROUP(TRGROUP_PANEL_TOOLLIST_MENU);

	mMenuAppendTrText_array16_radio(menu, g_menudat_item);

	//ツール名

	MLK_TRGROUP(TRGROUP_TOOL);

	sub = mMenuNew();

	for(i = 0; g_add_tool[i] != 255; i++)
	{
		id = g_add_tool[i];
		
		mMenuAppendText(sub, _ITEM_MENUID_TOOL + id, MLK_TR(id));
	}

	mMenuSetItemSubmenu(menu, TRID_MENU_INSERT_TOOL, sub);

	//

	if(!info->item)
	{
		//空部分

		mMenuSetItemEnable(menu, TRID_MENU_COPY, FALSE);
		mMenuSetItemEnable(menu, TRID_MENU_OPTION, FALSE);
		mMenuSetItemEnable(menu, TRID_MENU_TOOL, FALSE);
		mMenuSetItemEnable(menu, TRID_MENU_DELETE, FALSE);
		mMenuSetItemEnable(menu, TRID_MENU_REGIST, FALSE);
	}
	else
	{
		//---- アイテムあり

		mMenuSetItemEnable(menu, TRID_MENU_TOOL,
			(info->item->type == TOOLLIST_ITEMTYPE_TOOL));

		//登録:メニュー

		MLK_TRGROUP(TRGROUP_PANEL_TOOLLIST_MENU);

		sub = mMenuNew();

		mMenuSetItemSubmenu(menu, TRID_MENU_REGIST, sub);

		mMenuAppend(sub, _ITEM_MENUID_REGIST, MLK_TR(TRID_MENU_REGIST_NONE), 0, MMENUITEM_F_RADIO_TYPE);

		for(i = 0; i < DRAW_TOOLLIST_REG_NUM; i++)
		{
			snprintf(m, 10, "tool%d", i + 1);
			
			mMenuAppend(sub, _ITEM_MENUID_REGIST + 1 + i, m, 0,
				MMENUITEM_F_COPYTEXT | MMENUITEM_F_RADIO_TYPE);
		}

		mMenuAppendSep(sub);

		mMenuAppendText(sub, TRID_MENU_REGIST_RELEASE_ALL, MLK_TR(TRID_MENU_REGIST_RELEASE_ALL));

		//登録選択

		i = drawToolList_getRegistItemNo(info->item);

		mMenuSetItemCheck(menu, _ITEM_MENUID_REGIST + i + 1, TRUE);
	}

	//貼り付け

	mMenuSetItemEnable(menu, TRID_MENU_PASTE, (APPDRAW->tlist->copyitem != NULL));

	//----

	ToolListPage_drawItemSelFrame(p, info, FALSE);

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	ToolListPage_drawItemSelFrame(p, info, TRUE);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	return id;
}

/** アイテムメニュー処理 */

void ToolListPage_runMenu_item(ToolListPage *p,int x,int y,_POINTINFO *info)
{
	ToolListItem *item;
	int id,update;

	id = _runmenu_item(p, x, y, info);

	if(id < 0) return;

	//[!] デフォルトで、処理後にリストが更新される。
	//[!] 選択が変更されたときは、ブラシ値の更新のため、通知を送る。

	update = TRUE;

	if(id >= _ITEM_MENUID_TOOL && id < _ITEM_MENUID_TOOL + TOOL_NUM)
	{
		//ツールを挿入

		id = id - _ITEM_MENUID_TOOL;

		item = ToolList_insertItem_tool(info->group, info->item, MLK_TR2(TRGROUP_TOOL, id), id);

		drawToolList_setSelItem(APPDRAW, item);

		ToolListPage_notify_changeValue(p);
	}
	else if(id >= _ITEM_MENUID_REGIST && id <= _ITEM_MENUID_REGIST + DRAW_TOOLLIST_REG_NUM)
	{
		//登録

		drawToolList_setRegistItem(info->item, id - _ITEM_MENUID_REGIST - 1);
	}
	else
	{
		switch(id)
		{
			//新規ブラシ (作成後、選択)
			case TRID_MENU_INSERT_BRUSH:
				update = _cmd_new_brush(p, info);
				break;
			//コピー
			case TRID_MENU_COPY:
				drawToolList_copyItem(APPDRAW, info->item);
				update = FALSE;
				break;
			//貼り付け (挿入)
			// :作成後、選択
			case TRID_MENU_PASTE:
				item = ToolList_pasteItem(info->group, info->item, APPDRAW->tlist->copyitem);

				if(item)
				{
					drawToolList_setSelItem(APPDRAW, item);

					ToolListPage_notify_changeValue(p);
				}
				break;
			//設定
			case TRID_MENU_OPTION:
				ToolListPage_cmd_itemOption(p, info->item);
				update = FALSE;
				break;
			//削除
			case TRID_MENU_DELETE:
				if(drawToolList_deleteItem(info->item))
					ToolListPage_notify_changeValue(p);
				break;

			//ツールオプションの値をセット
			case TRID_MENU_TOOL_SETOPT:
				ToolList_getToolValue(
					((ToolListItem_tool *)info->item)->toolno,
					NULL, NULL,
					&((ToolListItem_tool *)info->item)->toolopt);
				
				update = FALSE;
				break;
			//設定値を表示
			case TRID_MENU_TOOL_VIEW:
				ToolListDialog_toolOptView(p->wg.toplevel, (ToolListItem_tool *)info->item);
				update = FALSE;
				break;

			//登録全て解除
			case TRID_MENU_REGIST_RELEASE_ALL:
				drawToolList_releaseRegistAll();
				break;
		}
	}

	//更新

	if(update)
		ToolListPage_updateAll(p);
}

/** アイテム設定 */

void ToolListPage_cmd_itemOption(ToolListPage *p,ToolListItem *item)
{
	mStr str = MSTR_INIT;
	const char *text;

	mStrSetText(&str, item->name);

	text = MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME);

	if(mSysDlg_inputText(p->wg.toplevel, text, text,
		MSYSDLG_INPUTTEXT_F_NOT_EMPTY | MSYSDLG_INPUTTEXT_F_SET_DEFAULT,
		&str))
	{
		mStrdup_free(&item->name, str.buf);

		ToolListPage_updateAll(p);

		//アイテム名
		
		if(item == APPDRAW->tlist->selitem)
			ToolListPage_notify_updateToolListPanel(p);
	}

	mStrFree(&str);
}

