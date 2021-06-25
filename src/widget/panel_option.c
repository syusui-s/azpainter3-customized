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
 * [Panel] オプション
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_iconbar.h"
#include "mlk_pager.h"
#include "mlk_event.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"

#include "appresource.h"
#include "panel.h"

#include "trid.h"


// mWidget::param1 = mPager *

//--------------------

mlkbool PanelOption_createPage_tool(mPager *p,mPagerInfo *info);
mlkbool PanelOption_createPage_rule(mPager *p,mPagerInfo *info);
mlkbool PanelOption_createPage_texture(mPager *p,mPagerInfo *info);
mlkbool PanelOption_createPage_headtail(mPager *p,mPagerInfo *info);

static const mFuncPagerCreate g_pagefunc[] = {
	PanelOption_createPage_tool, PanelOption_createPage_rule,
	PanelOption_createPage_texture, PanelOption_createPage_headtail
};

//

void PanelOption_toolStamp_changeImage(mWidget *wg);
void PanelOption_rule_setType(mPager *p);

enum
{
	_PAGE_TOOL,
	_PAGE_RULE,
	_PAGE_TEXTURE,
	_PAGE_HEADTAIL
};

//--------------------


//========================
// main
//========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_COMMAND)
	{
		//タブ選択変更
			
		APPCONF->panel.option_type = ev->cmd.id;

		mPagerSetPage((mPager *)wg->param1, g_pagefunc[APPCONF->panel.option_type]);

		mWidgetReLayout(wg);
	}

	return 1;
}

/* mPanel: 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	mWidget *ct;
	mIconBar *ib;
	mPager *pager;
	int i;

	//メインコンテナ

	ct = mContainerCreateVert(parent, 5, MLF_EXPAND_WH, 0);

	ct->event = _event_handle;
	ct->notify_to = MWIDGET_NOTIFYTO_SELF;

	//mIconBar

	ib = mIconBarCreate(ct, 0, MLF_EXPAND_W, MLK_MAKE32_4(3,3,3,0), MICONBAR_S_TOOLTIP | MICONBAR_S_SEP_BOTTOM);

	mIconBarSetImageList(ib, APPRES->imglist_panelopt_type);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_PANEL_OPTION);

	for(i = 0; i < 4; i++)
		mIconBarAdd(ib, i, i, i, MICONBAR_ITEMF_CHECKGROUP);

	mIconBarSetCheck(ib, APPCONF->panel.option_type, 1);

	//mPager
	// :各ページごとに padding をセット

	pager = mPagerCreate(ct, MLF_EXPAND_WH, 0);

	ct->param1 = (intptr_t)pager;

	mPagerSetPage(pager, g_pagefunc[APPCONF->panel.option_type]);

	return ct;
}

/** 作成 */

void PanelOption_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_OPTION, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}


//========================
// 外部からの呼び出し
//========================


/* 内容ウィジェット取得 */

static mWidget *_get_topct(void)
{
	return Panel_getContents(PANEL_OPTION);
}

/** ツール変更時 */

void PanelOption_changeTool(void)
{
	mWidget *p = _get_topct();

	//"ツール" が選択されている場合、各ツールの内容に変更

	if(p && APPCONF->panel.option_type == _PAGE_TOOL)
	{
		mPagerSetPage((mPager *)p->param1, g_pagefunc[0]);

		mWidgetReLayout(p);
	}
}

/** 定規タイプ変更時 */

void PanelOption_changeRuleType(void)
{
	mWidget *p = _get_topct();

	if(p && APPCONF->panel.option_type == _PAGE_RULE)
		PanelOption_rule_setType((mPager *)p->param1);
}

/** スタンプイメージの変更時 */

void PanelOption_changeStampImage(void)
{
	mWidget *p = _get_topct();

	//"ツール" 選択 & スタンプツール時

	if(p
		&& APPCONF->panel.option_type == _PAGE_TOOL
		&& APPDRAW->tool.no == TOOL_STAMP)
	{
		//mPager の mWidget::param1 に、中身のコンテナ
	
		PanelOption_toolStamp_changeImage((mWidget *)MLK_WIDGET(p->param1)->param1);
	}
}

