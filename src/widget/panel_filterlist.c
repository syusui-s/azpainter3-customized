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
 * [Panel] フィルタ一覧
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_listview.h"
#include "mlk_columnitem.h"
#include "mlk_event.h"
#include "mlk_menu.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_mainwin.h"

#include "panel.h"

#include "trid.h"
#include "trid_mainmenu.h"



/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == 100)
	{
		mColumnItem *pi = (mColumnItem *)ev->notify.param1;
		int send;

		//ヘッダ
		
		if(pi->param == -1) return 1;
		
		//通知タイプチェック

		if(APPCONF->foption & CONFIG_OPTF_FILTERLIST_DBLCLK)
		{
			//ダブルクリック時
			send = (ev->notify.notify_type == MLISTVIEW_N_ITEM_L_DBLCLK);
		}
		else
		{
			//リスト上を左クリックした時 (フォーカスがすでにあっても有効)
			
			send = (ev->notify.notify_type == MLISTVIEW_N_CHANGE_FOCUS
				|| ev->notify.notify_type == MLISTVIEW_N_CLICK_ON_FOCUS);
		}

		//コマンドイベント送る
	
		if(send)
		{
			mWidgetEventAdd_command(MLK_WIDGET(APPWIDGET->mainwin),
				pi->param, 0, MEVENT_COMMAND_FROM_UNKNOWN, 0);
		}
	}

	return 1;
}

/* mPanel 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	mWidget *p;
	mListView *list;
	mMenu *menu;
	mMenuItem *pi,*pi2;
	mMenuItemInfo *info;

	//コンテナ

	p = mContainerCreateVert(parent, 0, MLF_EXPAND_WH, 0);

	p->event = _event_handle;
	p->notify_to = MWIDGET_NOTIFYTO_SELF;

	//リストビュー

	list = mListViewCreate(p, 100, MLF_EXPAND_WH, MLK_MAKE32_4(0,2,0,0), 0, MSCROLLVIEW_S_HORZVERT_FRAME);

	//list->wg.font = mGuiGetDefaultFont();

	//項目リスト (メニューアイテムから)

	menu = mMenuGetItemSubmenu(APPWIDGET->mainwin->menu_main, TRMENU_TOP_FILTER);

	for(pi = mMenuGetTopItem(menu); pi; pi = mMenuItemGetNext(pi))
	{
		info = mMenuItemGetInfo(pi);

		if(!info->text) continue;

		//サブメニューなしの項目、またはサブメニューのヘッダ

		mListViewAddItem(list, info->text, -1,
			(info->submenu)? MCOLUMNITEM_F_HEADER: 0,
			(info->submenu)? -1: info->id);

		//サブメニュー

		if(info->submenu)
		{
			for(pi2 = mMenuGetTopItem(info->submenu); pi2; pi2 = mMenuItemGetNext(pi2))
			{
				info = mMenuItemGetInfo(pi2);

				if(info->text)
					mListViewAddItem_text_static_param(list, info->text, info->id);
			}
		}
	}

	mListViewSetAutoWidth(list, FALSE);
	
	return p;
}

/** 作成 */

void PanelFilterList_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_FILTER_LIST, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}

