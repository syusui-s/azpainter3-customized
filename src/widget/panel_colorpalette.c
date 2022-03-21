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
 * [panel] カラーパレット
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_imgbutton.h"
#include "mlk_arrowbutton.h"
#include "mlk_iconbar.h"
#include "mlk_pager.h"
#include "mlk_event.h"

#include "def_widget.h"
#include "def_config.h"

#include "appresource.h"
#include "panel.h"
#include "panel_func.h"
#include "draw_main.h"

#include "pv_panel_colpal.h"


//------------------------

typedef struct
{
	MLK_CONTAINER_DEF

	mPager *pager;
	mIconBar *ib_type;
	mWidget *ct_top; //ボタンのコンテナ
}_topct;

enum
{
	CMDID_PALETTE,
	CMDID_HSL,
	CMDID_GRADATION
};

enum
{
	WID_MENUBTT = 100,
	WID_MOVE_LEFT,
	WID_MOVE_RIGHT
};

//------------------------

mlkbool PanelColorPalette_createPage_palette(mPager *p,mPagerInfo *info);
mlkbool PanelColorPalette_createPage_HSL(mPager *p,mPagerInfo *info);
mlkbool PanelColorPalette_createPage_grad(mPager *p,mPagerInfo *info);

static const mFuncPagerCreate g_pagefunc[] = {
	PanelColorPalette_createPage_palette,
	PanelColorPalette_createPage_HSL, PanelColorPalette_createPage_grad
};

//------------------------



//===========================
// sub
//===========================


/* 通知イベント */

static void _event_notify(_topct *p,mEventNotify *ev)
{
	if(ev->widget_from->parent == p->ct_top)
	{
		//通知元の親が ct_top = 各ボタンのウィジェット
		// :各ページごとに処理を行うため、mPager に通知を送る。
		
		switch(ev->id)
		{
			//移動ボタン
			case WID_MOVE_LEFT:
			case WID_MOVE_RIGHT:
				mWidgetEventAdd_notify_id(ev->widget_from, MLK_WIDGET(p->pager), -2, 0,
					(ev->id == WID_MOVE_RIGHT), 0);
				break;
			//メニューボタン
			case WID_MENUBTT:
				mWidgetEventAdd_notify_id(ev->widget_from, MLK_WIDGET(p->pager), -1, 0, 0, 0);
				break;
		}
	}
	else
	{
		//ほかは、各ページからの通知 (色変更)
		// :param1=RGB色, param2=CHANGECOL

		if(drawColor_setDrawColor(ev->param1))
			PanelColor_setColor_fromOther(ev->param2);
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_topct *p = (_topct *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			_event_notify(p, (mEventNotify *)ev);
			break;

		//(mIconBar) タイプ変更 -> ページ切り替え
		case MEVENT_COMMAND:
			PanelColorPalette_changeType(ev->cmd.id - CMDID_PALETTE);
			break;
	}

	return 1;
}


//===========================
// ウィジェット作成
//===========================


/* 通常モードで作成 */

static void _create_normal_mode(_topct *p)
{
	mIconBar *ib;
	mWidget *wg,*ct;
	int i;

	mContainerSetType_vert(MLK_CONTAINER(p), 0);

	//---- ボタン

	ct = p->ct_top = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, 0);

	//mIconBar

	p->ib_type = ib = mIconBarCreate(ct, 0, MLF_EXPAND_X | MLF_MIDDLE, 0, 0);

	mIconBarSetImageList(ib, APPRES->imglist_panelcolpal_type);

	for(i = 0; i < 3; i++)
		mIconBarAdd(ib, CMDID_PALETTE + i, i, -1, MICONBAR_ITEMF_CHECKGROUP);

	mIconBarSetCheck(ib, CMDID_PALETTE + APPCONF->panel.colpal_type, 1);

	//矢印ボタン

	mArrowButtonCreate_minsize(ct, WID_MOVE_LEFT, MLF_MIDDLE, 0,
		MARROWBUTTON_S_LEFT, 19);

	mArrowButtonCreate_minsize(ct, WID_MOVE_RIGHT, MLF_MIDDLE, 0,
		MARROWBUTTON_S_RIGHT, 19);

	//メニューボタン
	
	wg = (mWidget *)mImgButtonCreate(ct, WID_MENUBTT, MLF_MIDDLE, 0, 0);

	wg->hintRepW = wg->hintRepH = APPRES_MENUBTT_SIZE + 12;

	mImgButton_setBitImage(MLK_IMGBUTTON(wg), MIMGBUTTON_TYPE_1BIT_TP_TEXT,
		AppResource_get1bitImg_menubtt(), APPRES_MENUBTT_SIZE, APPRES_MENUBTT_SIZE);

	//--- mPager

	p->pager = mPagerCreate(MLK_WIDGET(p), MLF_EXPAND_WH, 0);
}

/* 通常 -> コンパクトモードにセット */

static void _set_compact_mode(_topct *p)
{
	mWidget *ct = p->ct_top;

	//mIconBar 非表示

	mWidgetShow(MLK_WIDGET(p->ib_type), 0);

	//

	mContainerSetType_horz(MLK_CONTAINER(p), 5);

	//ボタンコンテナ

	mContainerSetType_vert(MLK_CONTAINER(ct), 3);
	ct->flayout = MLF_EXPAND_H;

	//ボタンの順を入れ替え (メニューを先頭に)

	mWidgetTreeMove(ct->last, ct, ct->first);
}

/* コンパクト -> 通常モードにセット */

static void _set_normal_mode(_topct *p)
{
	mWidget *ct = p->ct_top;

	//mIconBar 表示

	mWidgetShow(MLK_WIDGET(p->ib_type), 1);

	//
	
	mContainerSetType_vert(MLK_CONTAINER(p), 0);

	//ボタンコンテナ

	mContainerSetType_horz(MLK_CONTAINER(ct), 4);
	ct->flayout = MLF_EXPAND_W;

	//ボタンの順を入れ替え (メニューを終端に)

	mWidgetTreeMove(ct->first, ct, NULL);
}

/* mPanel 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	_topct *p;

	//コンテナ (トップ)

	p = (_topct *)mContainerNew(parent, sizeof(_topct));

	p->wg.event = _event_handle;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,2,3,0));

	//

	_create_normal_mode(p);

	if(APPCONF->panel.colpal_flags & CONFIG_PANEL_COLPAL_F_COMPACT)
		_set_compact_mode(p);

	mPagerSetPage(p->pager, g_pagefunc[APPCONF->panel.colpal_type]);

	return (mWidget *)p;
}


//===========================
// main
//===========================


/* _topct 取得 */

static _topct *_get_topct(void)
{
	return (_topct *)Panel_getContents(PANEL_COLOR_PALETTE);
}

/** カラーパレット作成 */

void PanelColorPalette_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_COLOR_PALETTE, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}

/** 選択タイプの変更 */

void PanelColorPalette_changeType(int no)
{
	_topct *p = _get_topct();

	if(p)
	{
		APPCONF->panel.colpal_type = no;

		if(p->ib_type)
			mIconBarSetCheck(p->ib_type, CMDID_PALETTE + no, 1);
		
		mPagerSetPage(p->pager, g_pagefunc[no]);

		mWidgetReLayout(MLK_WIDGET(p));
	}
}

/** UI モードの切り替え */

void PanelColorPalette_ToggleMode(void)
{
	_topct *p = _get_topct();

	APPCONF->panel.colpal_flags ^= CONFIG_PANEL_COLPAL_F_COMPACT;

	if(p)
	{
		if(APPCONF->panel.colpal_flags & CONFIG_PANEL_COLPAL_F_COMPACT)
			_set_compact_mode(p);
		else
			_set_normal_mode(p);

		Panel_relayout(PANEL_COLOR_PALETTE);
	}
}

