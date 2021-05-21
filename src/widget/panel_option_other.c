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

/********************************************
 * PanelOption
 *
 * [テクスチャ] [定規] [入り抜き]
 ********************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_pager.h"
#include "mlk_label.h"
#include "mlk_iconbar.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_menu.h"
#include "mlk_str.h"

#include "def_draw.h"

#include "sel_material.h"
#include "imgmatprev.h"
#include "valuebar.h"
#include "widget_func.h"

#include "appresource.h"
#include "apphelp.h"

#include "draw_main.h"
#include "draw_rule.h"

#include "trid.h"


//---------------

enum
{
	TRID_OPT_LOAD = 100,
	TRID_OPT_SAVE,
	TRID_OPT_LINE,
	TRID_OPT_BEZIER,
	TRID_OPT_HEAD,
	TRID_OPT_TAIL
};

//---------------


//******************************
// テクスチャ
//******************************


typedef struct
{
	SelMaterial *sel;
	ImgMatPrev *prev;
}_pagedat_texture;


/* イベント */

static int _texture_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->notify.id == 100)
	{
		//素材選択: パスが変更された

		_pagedat_texture *pd = (_pagedat_texture *)pagedat;

		SelMaterial_getPath(pd->sel, &APPDRAW->strOptTexturePath);

		drawTexture_loadOptionTextureImage(APPDRAW);

		ImgMatPrev_setImage(pd->prev, APPDRAW->imgmat_opttex);
	}

	return TRUE;
}

/** "テクスチャ" ページ作成 */

mlkbool PanelOption_createPage_texture(mPager *p,mPagerInfo *info)
{
	_pagedat_texture *pd;

	pd = (_pagedat_texture *)mMalloc0(sizeof(_pagedat_texture));

	info->pagedat = pd;
	info->event = _texture_event;

	mContainerSetType_vert(MLK_CONTAINER(p), 5);
	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,0,4,0));

	//素材選択

	pd->sel = SelMaterial_new(MLK_WIDGET(p), 100, SELMATERIAL_TYPE_TEXTURE);

	SelMaterial_setPath(pd->sel, APPDRAW->strOptTexturePath.buf);

	//プレビュー

	pd->prev = ImgMatPrev_new(MLK_WIDGET(p), 4, 4, MLF_EXPAND_WH);

	ImgMatPrev_setImage(pd->prev, APPDRAW->imgmat_opttex);

	return TRUE;
}


//******************************
// 定規
//******************************

//mWidget::param1 = (mIconBar *)

enum
{
	WID_RULE_BTT_CALL = 100,
	WID_RULE_BTT_REGIST,
	WID_RULE_HELP
};


/* 登録メニュー実行
 *
 * return: 選択番号。-1 でキャンセル */

static int _rule_runmenu(mWidget *bttwg)
{
	mMenu *menu;
	mMenuItem *mi;
	RuleRecord *rec;
	mBox box;
	mStr str = MSTR_INIT;
	int i,n;

	MLK_TRGROUP(TRGROUP_RULE);

	//メニュー

	menu = mMenuNew();

	rec = APPDRAW->rule.record;

	for(i = 0; i < DRAW_RULE_RECORD_NUM; i++, rec++)
	{
		mStrSetFormat(&str, "[%d] ", i + 1);

		if(rec->type == 0)
			mStrAppendText(&str, "---");
		else
		{
			mStrAppendFormat(&str, "%s : ", MLK_TR(rec->type));
		
			switch(rec->type)
			{
				//角度
				case DRAW_RULE_TYPE_LINE:
				case DRAW_RULE_TYPE_GRID:
					n = round(-rec->dval[0] * 18000 / MLK_MATH_PI);
					mStrAppendFormat(&str, "angle %d.%d", n / 100, abs(n % 100));
					break;
				//位置
				case DRAW_RULE_TYPE_CONCLINE:
				case DRAW_RULE_TYPE_CIRCLE:
					mStrAppendFormat(&str, "(%d,%d)", (int)rec->dval[0], (int)rec->dval[1]);
					break;
				//位置と角度
				case DRAW_RULE_TYPE_ELLIPSE:
				case DRAW_RULE_TYPE_SYMMETRY:
					n = round(-rec->dval[2] * 18000 / MLK_MATH_PI);
					mStrAppendFormat(&str, "(%d,%d) angle %d.%d",
						(int)rec->dval[0], (int)rec->dval[1], n / 100, abs(n % 100));
					break;
			}
		}

		mMenuAppendText_copy(menu, i, str.buf, str.len);
	}

	mStrFree(&str);

	//

	mWidgetGetBox_rel(bttwg, &box);

	mi = mMenuPopup(menu, bttwg, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	n = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	return n;
}

/* イベント */

static int _rule_event(mPager *p,mEvent *ev,void *pagedat)
{
	int no;

	switch(ev->type)
	{
		//mIconBar
		case MEVENT_COMMAND:
			if(ev->cmd.id == 100)
				//設定モード
				APPDRAW->rule.setting_mode ^= 1;
			else
				//タイプ変更
				drawRule_setType(APPDRAW, ev->cmd.id);
			break;

		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//読込
				case WID_RULE_BTT_CALL:
					no = _rule_runmenu(ev->notify.widget_from);
					if(no != -1)
					{
						drawRule_readRecord(APPDRAW, no);

						//タイプ選択
						mIconBarSetCheck(MLK_ICONBAR(p->wg.param1), APPDRAW->rule.type, TRUE);
					}
					break;
				//保存
				case WID_RULE_BTT_REGIST:
					no = _rule_runmenu(ev->notify.widget_from);
					if(no != -1)
						drawRule_setRecord(APPDRAW, no);
					break;
				//ヘルプ
				case WID_RULE_HELP:
					AppHelp_message(p->wg.toplevel, HELP_TRGROUP_SINGLE, HELP_TRID_RULE);
					break;
			}
			break;
	}

	return 1;
}

/** "定規" ページ作成 */

mlkbool PanelOption_createPage_rule(mPager *p,mPagerInfo *info)
{
	mWidget *ct;
	mIconBar *ib;
	int i;

	info->event = _rule_event;

	mContainerSetType_vert(MLK_CONTAINER(p), 15);
	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,3,4,0));

	//mIconBar

	ib = mIconBarCreate(MLK_WIDGET(p), 0, 0, 0,
		MICONBAR_S_TOOLTIP | MICONBAR_S_BUTTON_FRAME | MICONBAR_S_AUTO_WRAP | MICONBAR_S_DESTROY_IMAGELIST);

	p->wg.param1 = (intptr_t)ib;

	mIconBarSetImageList(ib, AppResource_createImageList_ruleicon());
	mIconBarSetTooltipTrGroup(ib, TRGROUP_RULE);

	mIconBarAdd(ib, 100, 0, 100, MICONBAR_ITEMF_CHECKBUTTON);
	mIconBarAddSep(ib);

	for(i = 0; i < DRAW_RULE_TYPE_NUM; i++)
		mIconBarAdd(ib, i, 1 + i, i, MICONBAR_ITEMF_CHECKGROUP);

	if(APPDRAW->rule.setting_mode)
		mIconBarSetCheck(ib, 100, TRUE);

	mIconBarSetCheck(ib, APPDRAW->rule.type, TRUE);

	//ボタン

	MLK_TRGROUP(TRGROUP_PANEL_OPTION);

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, 0);

	mButtonCreate(ct, WID_RULE_BTT_CALL, 0, 0, 0, MLK_TR(TRID_OPT_LOAD));
	mButtonCreate(ct, WID_RULE_BTT_REGIST, MLF_EXPAND_X, 0, 0, MLK_TR(TRID_OPT_SAVE));

	widget_createHelpButton(ct, WID_RULE_HELP, 0, 0);

	return TRUE;
}


//******************************
// 入り抜き
//******************************


typedef struct
{
	ValueBar *bar[2];
}_pagedat_headtail;

enum
{
	WID_HEADTAIL_LINE = 100,
	WID_HEADTAIL_BEZIER,
	WID_HEADTAIL_BAR_HEAD,
	WID_HEADTAIL_BAR_TAIL,
	WID_HEADTAIL_BTT_LOAD,
	WID_HEADTAIL_BTT_SAVE,
	WID_HEADTAIL_BTT_HELP
};


/* 登録メニュー実行 */

static int _headtail_runmenu_regist(mWidget *bttwg)
{
	mMenu *menu;
	mMenuItem *mi;
	uint32_t *pdat;
	int i,n1,n2;
	mBox box;
	char m[32];

	menu = mMenuNew();

	pdat = APPDRAW->headtail.regist;

	for(i = 0; i < DRAW_HEADTAIL_REGIST_NUM; i++, pdat++)
	{
		n1 = *pdat >> 16;
		n2 = *pdat & 0xffff;
	
		snprintf(m, 32, "[%d] %d.%d : %d.%d", i + 1,
			n1 / 10, n1 % 10, n2 / 10, n2 % 10);
		
		mMenuAppendText_copy(menu, i, m, -1);
	}

	mWidgetGetBox_rel(bttwg, &box);

	mi = mMenuPopup(menu, bttwg, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	n1 = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	return n1;
}

/* バーの値セット */

static void _headtail_set_bar_value(_pagedat_headtail *pd)
{
	uint32_t val;

	val = APPDRAW->headtail.curval[APPDRAW->headtail.selno];

	ValueBar_setPos(pd->bar[0], val >> 16);
	ValueBar_setPos(pd->bar[1], val & 0xffff);
}

/* イベント */

static int _headtail_event(mPager *p,mEvent *ev,void *pagedat)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		int no,selno;

		selno = APPDRAW->headtail.selno;
	
		switch(ev->notify.id)
		{
			//タイプ選択
			case WID_HEADTAIL_LINE:
			case WID_HEADTAIL_BEZIER:
				APPDRAW->headtail.selno = ev->notify.id - WID_HEADTAIL_LINE;

				_headtail_set_bar_value((_pagedat_headtail *)pagedat);
				break;
			//bar:入り
			case WID_HEADTAIL_BAR_HEAD:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
				{
					APPDRAW->headtail.curval[selno] &= 0xffff;
					APPDRAW->headtail.curval[selno] |= (uint32_t)ev->notify.param1 << 16;
				}
				break;
			//bar:抜き
			case WID_HEADTAIL_BAR_TAIL:
				if(ev->notify.param2 & VALUEBAR_ACT_F_ONCE)
				{
					APPDRAW->headtail.curval[selno] &= 0xffff0000;
					APPDRAW->headtail.curval[selno] |= ev->notify.param1;
				}
				break;
			//読込
			case WID_HEADTAIL_BTT_LOAD:
				no = _headtail_runmenu_regist(ev->notify.widget_from);
				if(no != -1)
				{
					APPDRAW->headtail.curval[selno] = APPDRAW->headtail.regist[no];

					_headtail_set_bar_value((_pagedat_headtail *)pagedat);
				}
				break;
			//保存
			case WID_HEADTAIL_BTT_SAVE:
				no = _headtail_runmenu_regist(ev->notify.widget_from);
				if(no != -1)
				{
					APPDRAW->headtail.regist[no] = APPDRAW->headtail.curval[selno];
				}
				break;
			//ヘルプ
			case WID_HEADTAIL_BTT_HELP:
				AppHelp_message(p->wg.toplevel, HELP_TRGROUP_SINGLE, HELP_TRID_HEADTAIL);
				break;
		}
	}

	return 1;
}

/** "入り抜き" ページ作成 */

mlkbool PanelOption_createPage_headtail(mPager *p,mPagerInfo *info)
{
	_pagedat_headtail *pd;
	mWidget *ct;
	int i;

	pd = (_pagedat_headtail *)mMalloc(sizeof(_pagedat_headtail));

	info->pagedat = pd;
	info->event = _headtail_event;

	//

	mContainerSetType_vert(MLK_CONTAINER(p), 0);
	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(3,3,4,0));

	MLK_TRGROUP(TRGROUP_PANEL_OPTION);

	//タイプ選択

	ct = mContainerCreateHorz(MLK_WIDGET(p), 5, 0, 0);

	for(i = 0; i < 2; i++)
	{
		mCheckButtonCreate(ct, WID_HEADTAIL_LINE + i, 0, 0, MCHECKBUTTON_S_RADIO,
			MLK_TR(TRID_OPT_LINE + i), (APPDRAW->headtail.selno == i));
	}

	//バー

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 4, 5, MLF_EXPAND_W, MLK_MAKE32_4(0,8,0,0));

	for(i = 0; i < 2; i++)
	{
		mLabelCreate(ct, MLF_MIDDLE, 0, 0, MLK_TR(TRID_OPT_HEAD + i));

		pd->bar[i] = ValueBar_new(ct, WID_HEADTAIL_BAR_HEAD + i, MLF_EXPAND_W | MLF_MIDDLE,
			1, 0, 1000, 0);
	}

	_headtail_set_bar_value(pd);

	//ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 4, MLF_EXPAND_W, MLK_MAKE32_4(0,10,0,0));

	for(i = 0; i < 2; i++)
		mButtonCreate(ct, WID_HEADTAIL_BTT_LOAD + i, 0, 0, 0, MLK_TR(TRID_OPT_LOAD + i));

	widget_createHelpButton(ct, WID_HEADTAIL_BTT_HELP, MLF_RIGHT | MLF_EXPAND_X, 0); 

	return TRUE;
}


