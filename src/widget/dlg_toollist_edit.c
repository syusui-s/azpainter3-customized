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
 * ツールリスト編集ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_listview.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_sysdlg.h"
#include "mlk_menu.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_columnitem.h"
#include "mlk_imagelist.h"

#include "toollist.h"

#include "def_config.h"
#include "def_draw.h"
#include "def_draw_toollist.h"

#include "draw_toollist.h"

#include "appresource.h"

#include "dlg_toollist.h"
#include "widget_func.h"

#include "trid.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mImageList *imglist;

	mListView *list_group,
		*list_item;
}_dialog;

//----------------------

enum
{
	TRID_TITLE,
	TRID_GROUP,
	TRID_ITEM
};

enum
{
	WID_LIST_GROUP = 100,
	WID_LIST_ITEM
};

enum
{
	CMDID_GROUP_TOP = 100,
	CMDID_ITEM_TOP = 200
};

//----------------------

//ボタン
static const uint8_t g_iconbtt_group[] = {
	APPRES_LISTEDIT_ADD, APPRES_LISTEDIT_EDIT, APPRES_LISTEDIT_DEL,
	APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, APPRES_LISTEDIT_OPEN,
	APPRES_LISTEDIT_SAVE, 255
};

static const uint8_t g_iconbtt_item[] = {
	APPRES_LISTEDIT_RENAME, APPRES_LISTEDIT_DEL,
	APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, APPRES_LISTEDIT_MOVE, 255
};

//----------------------



//=========================
// sub
//=========================


/* グループリストセット */

static void _setlist_group(_dialog *p)
{
	ToolListGroupItem *gi;

	mListViewDeleteAllItem(p->list_group);

	TOOLLIST_FOR_GROUP(gi)
	{
		mListViewAddItem_text_static_param(p->list_group, gi->name, (intptr_t)gi);
	}
}

/* アイテムリストセット */

static void _setlist_item(_dialog *p,ToolListGroupItem *group)
{
	ToolListItem *pi;
	mListView *list = p->list_item;

	mListViewDeleteAllItem(list);

	if(!group) return;

	TOOLLIST_FOR_ITEM(group, pi)
	{
		mListViewAddItem_text_static_param(list, pi->name, (intptr_t)pi);
	}
}


//=========================
// コマンド
//=========================


/* [GROUP] 追加/編集
 *
 * group: NULL で追加 */

static void _cmd_group_new_edit(_dialog *p,ToolListGroupItem *group,mColumnItem *ci)
{
	ToolListDlg_groupOpt dat;

	mMemset0(&dat, sizeof(ToolListDlg_groupOpt));

	if(group)
	{
		mStrSetText(&dat.strname, group->name);
		dat.colnum = group->colnum;
	}
	else
		dat.colnum = 2;

	//ダイアログ

	if(ToolListDialog_groupOption(MLK_WINDOW(p), &dat, (group != NULL)))
	{
		if(group)
		{
			//編集

			mStrdup_free(&group->name, dat.strname.buf);
			group->colnum = dat.colnum;

			mListViewSetItemText_static(p->list_group, ci, group->name);
		}
		else
		{
			//追加

			group = ToolList_insertGroup(&APPDRAW->tlist->list_group,
				NULL, dat.strname.buf, dat.colnum);

			if(group)
			{
				ci = mListViewAddItem_text_static_param(p->list_group, group->name, (intptr_t)group);

				mListViewSetFocusItem(p->list_group, ci);

				_setlist_item(p, group);
			}
		}
	}

	mStrFree(&dat.strname);
}

/* [ITEM] 名前変更 */

static void _cmd_item_rename(_dialog *p,ToolListItem *item,mColumnItem *ci)
{
	mStr str = MSTR_INIT;
	const char *text;

	mStrSetText(&str, item->name);

	text = MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME);

	if(mSysDlg_inputText(MLK_WINDOW(p), text, text,
		MSYSDLG_INPUTTEXT_F_NOT_EMPTY | MSYSDLG_INPUTTEXT_F_SET_DEFAULT,
		&str))
	{
		mStrdup_free(&item->name, str.buf);

		mListViewSetItemText_static(p->list_item, ci, item->name);
	}

	mStrFree(&str);
}

/* [GROUP] 削除 */

static void _cmd_group_delete(_dialog *p,ToolListGroupItem *group,mColumnItem *ci)
{
	//削除確認

	if(!widget_message_confirm(MLK_WINDOW(p),
		MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_DELETE_NO_RESTORE)))
		return;

	//削除
	
	drawToolList_deleteGroup(group);

	//リスト

	ci = mListViewDeleteItem_focus(p->list_group);

	group = (ci)? (ToolListGroupItem *)ci->param: NULL;

	_setlist_item(p, group);
}

/* [ITEM] グループ移動 */

static void _cmd_item_move_group(_dialog *p,ToolListItem *item,mColumnItem *ci,mIconBar *ib)
{
	mMenu *menu;
	mMenuItemInfo *info;
	mMenuItem *mi;
	ToolListGroupItem *gi;
	mBox box;

	//グループが一つしかない場合は移動不可

	if(APPDRAW->tlist->list_group.num == 1) return;

	//メニュー

	menu = mMenuNew();

	TOOLLIST_FOR_GROUP(gi)
	{
		if(gi != item->group)
		{
			info = mMenuAppendText(menu, 0, gi->name);
			if(info) info->param1 = (intptr_t)gi;
		}
	}

	mIconBarGetItemBox(ib, CMDID_ITEM_TOP + APPRES_LISTEDIT_MOVE, &box);

	mi = mMenuPopup(menu, MLK_WIDGET(ib), 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);
	
	if(!mi)
		gi = NULL;
	else
	{
		info = mMenuItemGetInfo(mi);
		gi = (ToolListGroupItem *)info->param1;
	}

	mMenuDestroy(menu);

	//移動 & アイテム削除

	if(gi)
	{
		ToolList_moveItem(item, gi, NULL);

		mListViewDeleteItem_focus(p->list_item);
	}
}

/* [GROUP] 開く */

static void _cmd_group_open(_dialog *p,ToolListGroupItem *group)
{
	mStr str = MSTR_INIT;

	if(mSysDlg_openFile(MLK_WINDOW(p),
		"AzPainter toollist (*.aptl)\t*.aptl\tAll files\t*", 0,
		APPCONF->strFileDlgDir.buf, 0, &str))
	{
		mStrPathGetDir(&APPCONF->strFileDlgDir, str.buf);

		ToolList_loadFile(str.buf, TRUE);
		
		mStrFree(&str);

		//グループリスト

		_setlist_group(p);

		//以前の選択グループを選択

		if(group)
			mListViewSetFocusItem_param(p->list_group, (intptr_t)group);
	}
}

/* [GROUP] 保存 */

static void _cmd_group_save(_dialog *p,ToolListGroupItem *group)
{
	mStr str = MSTR_INIT;

	if(mSysDlg_saveFile(MLK_WINDOW(p),
		"AzPainter toollist (*.aptl)\taptl", 0,
		APPCONF->strFileDlgDir.buf, 0, &str, NULL))
	{
		mStrPathGetDir(&APPCONF->strFileDlgDir, str.buf);

		ToolList_saveFile(str.buf, group);
		
		mStrFree(&str);
	}
}


//=========================
// イベント
//=========================


/* COMMAND イベント */

static void _event_command(_dialog *p,int id,mIconBar *iconbar)
{
	mColumnItem *ci_group,*ci_item;
	ToolListGroupItem *group = NULL;
	ToolListItem *item = NULL;
	int fgroup,fup;

	if(id >= CMDID_ITEM_TOP)
		id -= CMDID_ITEM_TOP, fgroup = FALSE;
	else
		id -= CMDID_GROUP_TOP, fgroup = TRUE;

	//アイテム

	ci_group = mListViewGetFocusItem(p->list_group);
	ci_item = mListViewGetFocusItem(p->list_item);

	if(ci_group) group = (ToolListGroupItem *)ci_group->param;
	if(ci_item) item = (ToolListItem *)ci_item->param;
	
	//選択がない

	if(id != APPRES_LISTEDIT_ADD && id != APPRES_LISTEDIT_OPEN)
	{
		if((fgroup && !group) || (!fgroup && !item))
			return;
	}

	//

	switch(id)
	{
		//追加 (GROUP)
		case APPRES_LISTEDIT_ADD:
			_cmd_group_new_edit(p, NULL, NULL);
			break;
		//編集 (GROUP)
		case APPRES_LISTEDIT_EDIT:
			_cmd_group_new_edit(p, group, ci_group);
			break;
		//名前変更 (ITEM)
		case APPRES_LISTEDIT_RENAME:
			_cmd_item_rename(p, item, ci_item);
			break;
		//削除 (GROUP/ITEM)
		case APPRES_LISTEDIT_DEL:
			if(fgroup)
				_cmd_group_delete(p, group, ci_group);
			else
			{
				drawToolList_deleteItem(item);

				mListViewDeleteItem_focus(p->list_item);
			}
			break;
		//上/下へ移動 (GROUP/ITEM)
		case APPRES_LISTEDIT_UP:
		case APPRES_LISTEDIT_DOWN:
			fup = (id == APPRES_LISTEDIT_UP);
			
			if(fgroup)
			{
				mListMoveUpDown(&APPDRAW->tlist->list_group, MLISTITEM(group), fup);

				mListViewMoveItem_updown(p->list_group, ci_group, !fup);
			}
			else
			{
				mListMoveUpDown(&item->group->list, MLISTITEM(item), fup);

				mListViewMoveItem_updown(p->list_item, ci_item, !fup);
			}
			break;
		//グループ移動 (ITEM)
		case APPRES_LISTEDIT_MOVE:
			_cmd_item_move_group(p, item, ci_item, iconbar);
			break;
		//開く (GROUP)
		case APPRES_LISTEDIT_OPEN:
			_cmd_group_open(p, group);
			break;
		//保存 (GROUP)
		case APPRES_LISTEDIT_SAVE:
			_cmd_group_save(p, group);
			break;
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.id == WID_LIST_GROUP
				&& ev->notify.notify_type == MLISTVIEW_N_CHANGE_FOCUS)
			{
				//グループ選択変更 -> アイテムリストセット

				_setlist_item((_dialog *)wg,
					(ToolListGroupItem *)MLK_COLUMNITEM(ev->notify.param1)->param);
			}
			break;
	
		case MEVENT_COMMAND:
			_event_command((_dialog *)wg, ev->cmd.id, (mIconBar *)ev->cmd.from_ptr);
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}


//=======================


/* リストとボタン作成 */

static mListView *_create_list(_dialog *p,mWidget *parent,int no)
{
	mWidget *ctv,*cth;
	mListView *lv;
	mIconBar *ib;
	const uint8_t *pbtt;
	int n,top;

	ctv = mContainerCreateVert(parent, 2, MLF_EXPAND_WH, 0);

	mLabelCreate(ctv, 0, 0, 0, MLK_TR(TRID_GROUP + no));

	//------------

	cth = mContainerCreateHorz(ctv, 3, MLF_EXPAND_WH, 0);

	//リスト

	lv = mListViewCreate(cth, WID_LIST_GROUP + no, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(lv), 13, 18);

	//mIconBar

	ib = mIconBarCreate(cth, 0, 0, 0, MICONBAR_S_VERT | MICONBAR_S_TOOLTIP);

	mIconBarSetImageList(ib, p->imglist);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_LISTEDIT);

	if(no == 0)
	{
		pbtt = g_iconbtt_group;
		top = CMDID_GROUP_TOP;
	}
	else
	{
		pbtt = g_iconbtt_item;
		top = CMDID_ITEM_TOP;
	}

	for(; *pbtt != 255; pbtt++)
	{
		n = *pbtt;
		mIconBarAdd(ib, top + n, n, n, 0);
	}

	return lv;
}

/* 作成 */

static _dialog *_create_dialog(mWindow *parent)
{
	_dialog *p;
	mWidget *ct;

	MLK_TRGROUP(TRGROUP_DLG_TOOLLIST_EDIT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);

	if(!p) return NULL;

	//イメージリスト

	p->imglist = AppResource_createImageList_listedit();

	//----- リスト

	ct = mContainerCreateHorz(MLK_WIDGET(p), 12, MLF_EXPAND_WH, 0);

	p->list_group = _create_list(p, ct, 0);
	p->list_item = _create_list(p, ct, 1);

	//---- OK

	ct = (mWidget *)mButtonCreate(MLK_WIDGET(p), MLK_WID_OK, MLF_RIGHT, MLK_MAKE32_4(0,12,0,0),
		0, MLK_TR_SYS(MLK_TRSYS_OK));

	ct->fstate |= MWIDGET_STATE_ENTER_SEND;

	//-------

	//リストセット

	_setlist_group(p);

	_setlist_item(p, (ToolListGroupItem *)APPDRAW->tlist->list_group.top);

	//グループ選択

	mListViewSetFocusItem_index(p->list_group, 0);

	return p;
}

/** ツールリスト編集ダイアログ */

void ToolListDialog_edit(mWindow *parent)
{
	_dialog *p;

	p = _create_dialog(parent);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	mDialogRun(MLK_DIALOG(p), FALSE);

	//

	mImageListFree(p->imglist);

	mWidgetDestroy(MLK_WIDGET(p));
}

