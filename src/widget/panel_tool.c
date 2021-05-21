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
 * [Panel] ツール
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"

#include "def_widget.h"
#include "def_draw.h"

#include "appresource.h"
#include "panel.h"

#include "draw_main.h"

#include "trid.h"


//-------------------

typedef struct
{
	MLK_CONTAINER_DEF

	mIconBar *ib_tool,
		*ib_sub;
}_topct;

#define CMDID_TOOL     100
#define CMDID_TOOLSUB  200

//-------------------

enum
{
	ICON_FREEHAND,
	ICON_LINE,
	ICON_BOX,
	ICON_CIRCLE,
	ICON_CONC_LINE,
	ICON_SUCC_LINE,
	ICON_BEZIER,
	ICON_FILL_BOX,
	ICON_FILL_CIRCLE,
	ICON_FILL_POLYGON,
	ICON_GRAD_RADIAL,
	ICON_MOVE_CUR,
	ICON_MOVE_GRAB,
	ICON_MOVE_CHECK,
	ICON_MOVE_ALL,
	ICON_SPOIT_CANVAS,
	ICON_SPOIT_LAYER,
	ICON_SPOIT_MIDDLE,
	ICON_SPOIT_REPLACE_DRAWCOL,
	ICON_SPOIT_REPLACE_TP,
	ICON_SEL_MOVE,
	ICON_SEL_COPY,
	ICON_SEL_MOVE_SEL,
	ICON_CUTPASTE_COPY,
	ICON_CUTPASTE_CUT,
	ICON_CUTPASTE_PASTE,
	ICON_CUTPASTE_PASTE_FILE,
	ICON_BOXEDIT_TOP
};

enum
{
	TTIP_FREEHAND,
	TTIP_LINE,
	TTIP_BOX,
	TTIP_CIRCLE,
	TTIP_CONC_LINE,
	TTIP_SUCC_LINE,
	TTIP_BEZIER,
	TTIP_GRAD_LINE = 20,
	TTIP_GRAD_CIRCLE,
	TTIP_GRAD_RECT,
	TTIP_GRAD_RADIAL,
	TTIP_MOVE_CUR = 30,
	TTIP_MOVE_GRAB,
	TTIP_MOVE_CHECK,
	TTIP_MOVE_ALL,
	TTIP_SPOIT_CANVAS = 40,
	TTIP_SPOIT_LAYER,
	TTIP_SPOIT_MIDDLE,
	TTIP_POLYGON = 200,
	TTIP_IMAGE_MOVE,
	TTIP_IMAGE_COPY,
	TTIP_SELECT_MOVE,
	TTIP_CUTPASTE_COPY = 220,
	TTIP_CUTPASTE_CUT,
	TTIP_CUTPASTE_PASTE,
	TTIP_CUTPASTE_PASTE_FILE,
	TTIP_BOXEDIT_FLIP_HORZ = 240,
	TTIP_BOXEDIT_FLIP_VERT,
	TTIP_BOXEDIT_ROTATE_LEFT,
	TTIP_BOXEDIT_ROTATE_RIGHT,
	TTIP_BOXEDIT_TRANSFORM,
	TTIP_BOXEDIT_TRIM
};

//図形塗り/消し

static const uint16_t g_icon_fillpoly[] = {
	(TTIP_BOX<<8)|ICON_FILL_BOX, (TTIP_CIRCLE<<8)|ICON_FILL_CIRCLE,
	(TTIP_POLYGON<<8)|ICON_FILL_POLYGON, (TTIP_FREEHAND<<8)|ICON_FREEHAND, 255
};

//グラデーション

static const uint16_t g_icon_grad[] = {
	(TTIP_GRAD_LINE<<8)|ICON_LINE, (TTIP_GRAD_CIRCLE<<8)|ICON_CIRCLE,
	(TTIP_GRAD_RECT<<8)|ICON_BOX, (TTIP_GRAD_RADIAL<<8)|ICON_GRAD_RADIAL, 255
};

//選択範囲

static const uint16_t g_icon_select[] = {
	(TTIP_BOX<<8)|ICON_FILL_BOX, (TTIP_POLYGON<<8)|ICON_FILL_POLYGON, (TTIP_FREEHAND<<8)|ICON_FREEHAND,
	(TTIP_IMAGE_MOVE<<8)|ICON_SEL_MOVE, (TTIP_IMAGE_COPY<<8)|ICON_SEL_COPY,
	(TTIP_SELECT_MOVE<<8)|ICON_SEL_MOVE_SEL, 255
};

//スタンプ

static const uint16_t g_icon_stamp[] = {
	(TTIP_BOX<<8)|ICON_FILL_BOX, (TTIP_POLYGON<<8)|ICON_FILL_POLYGON, (TTIP_FREEHAND<<8)|ICON_FREEHAND, 255
};

//-------------------


//========================
// sub
//========================


/* ツールごとのサブアイコンセット */

static void _set_subtype(_topct *p)
{
	int i,toolno,subno,img_top,ttip_top,bttnum;
	uint32_t flags,flags_base;
	const uint16_t *picon;

	mIconBarDeleteAll(p->ib_sub);

	toolno = APPDRAW->tool.no;
	subno = APPDRAW->tool.subno[toolno];

	picon = NULL;
	ttip_top = -1;
	bttnum = 0;
	flags_base = MICONBAR_ITEMF_CHECKGROUP;

	//

	switch(toolno)
	{
		//描画
		case TOOL_TOOLLIST:
		case TOOL_DOTPEN:
		case TOOL_DOTPEN_ERASE:
		case TOOL_FINGER:
			bttnum = TOOLSUB_DRAW_NUM;
			img_top = ttip_top = 0;
			break;
		//図形塗りつぶし/消去
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			picon = g_icon_fillpoly;
			break;
		//グラデーション
		case TOOL_GRADATION:
			picon = g_icon_grad;
			break;
		//移動
		case TOOL_MOVE:
			bttnum = 4;
			img_top = ICON_MOVE_CUR;
			ttip_top = TTIP_MOVE_CUR;
			break;
		//選択範囲
		case TOOL_SELECT:
			picon = g_icon_select;
			break;
		//切り貼り
		case TOOL_CUTPASTE:
			bttnum = 4;
			img_top = ICON_CUTPASTE_COPY;
			ttip_top = TTIP_CUTPASTE_COPY;
			flags_base = MICONBAR_ITEMF_BUTTON;
			break;
		//矩形編集
		case TOOL_BOXEDIT:
			bttnum = 6;
			img_top = ICON_BOXEDIT_TOP;
			ttip_top = TTIP_BOXEDIT_FLIP_HORZ;
			flags_base = MICONBAR_ITEMF_BUTTON;
			break;
		//スタンプ
		case TOOL_STAMP:
			picon = g_icon_stamp;
			break;
		//スポイト
		case TOOL_SPOIT:
			bttnum = 5;
			img_top = ICON_SPOIT_CANVAS;
			ttip_top = TTIP_SPOIT_CANVAS;
			break;
	}

	//

	if(picon)
	{
		//配列データからセット

		for(i = 0; *picon != 255; i++, picon++)
		{
			mIconBarAdd(p->ib_sub, CMDID_TOOLSUB + i,
				*picon & 255, *picon >> 8,
				flags_base | (i == subno? MICONBAR_ITEMF_CHECKED: 0));
		}
	}
	else
	{
		//値が連続しているもの
	
		for(i = 0; i < bttnum; i++)
		{
			flags = flags_base;

			if(flags == MICONBAR_ITEMF_CHECKGROUP && i == subno)
				flags |= MICONBAR_ITEMF_CHECKED;
		
			mIconBarAdd(p->ib_sub, CMDID_TOOLSUB + i, img_top + i,
				(ttip_top < 0)? -1: ttip_top + i, flags);
		}
	}
}


//========================
// main
//========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_COMMAND)
	{
		int id = ev->cmd.id;

		if(id >= CMDID_TOOL && id < CMDID_TOOL + TOOL_NUM)
		{
			//ツール

			drawTool_setTool(APPDRAW, id - CMDID_TOOL);
		}
		else if(id >= CMDID_TOOLSUB && id < CMDID_TOOLSUB + 100)
		{
			//サブタイプ

			id -= CMDID_TOOLSUB;

			if(APPDRAW->tool.no == TOOL_CUTPASTE)
				//切り貼り
				drawBoxSel_run_cutpaste(APPDRAW, id);
			else if(APPDRAW->tool.no == TOOL_BOXEDIT)
				//矩形編集
				drawBoxSel_run_edit(APPDRAW, id);
			else
				drawTool_setTool_subtype(APPDRAW, id);
		}
	}

	return 1;
}

/* mPanel: 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	_topct *p;
	mIconBar *ib;
	int i;

	//コンテナ (トップ)

	p = (_topct *)mContainerNew(parent, sizeof(_topct));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.event = _event_handle;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(2,3,2,0));

	//---- ツール

	p->ib_tool = ib = mIconBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0,
		MICONBAR_S_TOOLTIP | MICONBAR_S_AUTO_WRAP | MICONBAR_S_SEP_BOTTOM);

	mIconBarSetImageList(ib, APPRES->imglist_paneltool_tool);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_TOOL);

	for(i = 0; i < TOOL_NUM; i++)
		mIconBarAdd(ib, CMDID_TOOL + i, i, i, MICONBAR_ITEMF_CHECKGROUP);

	mIconBarSetCheck(ib, CMDID_TOOL + APPDRAW->tool.no, 1);

	//---- サブ

	p->ib_sub = ib = mIconBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0,
		MICONBAR_S_TOOLTIP | MICONBAR_S_AUTO_WRAP);

	mIconBarSetImageList(ib, APPRES->imglist_paneltool_sub);
	mIconBarSetTooltipTrGroup(ib, TRGROUP_TOOL_SUB);

	_set_subtype(p);

	return (mWidget *)p;
}

/* _topct 取得 */

static _topct *_get_topct(void)
{
	return (_topct *)Panel_getContents(PANEL_TOOL);
}


/** 作成 */

void PanelTool_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_TOOL, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}

/** ツール変更時 */

void PanelTool_changeTool(void)
{
	_topct *p = _get_topct();

	if(p)
	{
		//ツール

		mIconBarSetCheck(p->ib_tool, CMDID_TOOL + APPDRAW->tool.no, 1);

		//サブタイプ

		_set_subtype(p);
	}
}

/** サブタイプ変更時 */

void PanelTool_changeToolSub(void)
{
	_topct *p = _get_topct();

	if(p)
	{
		mIconBarSetCheck(p->ib_sub,
			CMDID_TOOLSUB + APPDRAW->tool.subno[APPDRAW->tool.no], 1);
	}
}

