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
 * [panel] ツールリスト
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panel.h"
#include "mlk_label.h"
#include "mlk_event.h"
#include "mlk_splitter.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_draw_toollist.h"

#include "toollist.h"

#include "draw_toollist.h"

#include "panel.h"
#include "panel_func.h"
#include "widget_func.h"
#include "valuebar.h"
#include "dlg_toollist.h"
#include "pv_mainwin.h"

#include "trid.h"


//------------------------

typedef struct _ToolListView ToolListView;

typedef struct
{
	MLK_CONTAINER_DEF

	mLabel *lb_selname;
	ValueBar *bar_size,
		*bar_opacity;
	mWidget *wg_btt_save,
		*wg_btt_opt,
		*wg_sizelist;
	
	ToolListView *toollist;
}_topct;

enum
{
	WID_BTT_SAVE = 100,
	WID_BTT_SETTING,
	WID_BTT_EDIT,
	WID_BAR_SIZE,
	WID_BAR_OPACITY,
	WID_SPLITTER
};

//------------------------

ToolListView *ToolListView_new(mWidget *parent,mFont *font);
void ToolListView_update(ToolListView *p);

mWidget *SizeListView_new(mWidget *parent,mBuf *buf);

//------------------------

//設定ボタン (15x15)
static const unsigned char g_img_setting[]={
0xff,0x7f,0xff,0x4f,0xff,0x4f,0xff,0x7f,0x01,0x40,0x01,0x40,0x01,0x40,0x01,0x40,
0x01,0x40,0x01,0x40,0x01,0x40,0x01,0x40,0x01,0x40,0x01,0x40,0xff,0x7f };

//------------------------


//===========================
// sub
//===========================


/* 選択アイテム情報の更新 */

static void _update_iteminfo(_topct *p)
{
	ToolListItem *selitem;
	BrushEditData *brush;
	int fbrush;

	selitem = APPDRAW->tlist->selitem;

	//[!] 選択なし/ツールの場合は NULL
	brush = drawToolList_getBrush();

	fbrush = (brush != NULL);

	//名前 (ツールの場合もセット)
	
	mLabelSetText(p->lb_selname, (selitem)? selitem->name: NULL);

	//有効/無効

	mWidgetEnable(p->wg_btt_save, (fbrush && !(brush->v.flags & BRUSH_FLAGS_AUTO_SAVE)) );
	mWidgetEnable(p->wg_btt_opt, (selitem != NULL));

	mWidgetEnable(MLK_WIDGET(p->bar_size), fbrush);
	mWidgetEnable(MLK_WIDGET(p->bar_opacity), fbrush);

	//ブラシの値

	if(fbrush)
	{
		//サイズ

		ValueBar_setStatus(p->bar_size, brush->v.size_min, brush->v.size_max, brush->v.size);

		//濃度

		ValueBar_setPos(p->bar_opacity, brush->v.opacity);
	}
}


//===========================
// main
//===========================


/* 通知イベント */

static void _event_notify(_topct *p,mEventNotify *ev)
{
	switch(ev->id)
	{
		//ブラシサイズ
		case WID_BAR_SIZE:
			if(ev->param2 & VALUEBAR_ACT_F_ONCE)
			{
				drawBrushParam_setSize(ev->param1);

				PanelBrushOpt_updateBrushSize();
			}
			break;
		//ブラシ濃度
		case WID_BAR_OPACITY:
			if(ev->param2 & VALUEBAR_ACT_F_ONCE)
			{
				drawBrushParam_setOpacity(ev->param1);

				PanelBrushOpt_updateBrushOpacity();
			}
			break;

		//保存
		// :このパネルで操作可能な値のみ保存する。
		case WID_BTT_SAVE:
			drawBrushParam_saveSimple();
			break;
		//設定
		// :ブラシ設定パネルの表示/非表示
		case WID_BTT_SETTING:
			MainWindow_panel_toggle(APPWIDGET->mainwin, PANEL_BRUSHOPT);
			break;
		//編集
		case WID_BTT_EDIT:
			ToolListDialog_edit(p->wg.toplevel);

			ToolListView_update(p->toollist);
			_update_iteminfo(p);
			PanelBrushOpt_setValue();
			break;

		//mSplitter
		// :サイズが変わった時、サイズリストの高さを保存
		case WID_SPLITTER:
			if(ev->notify_type == MSPLITTER_N_MOVED)
			{
				APPCONF->panel.toollist_sizelist_h =
					mWidgetGetHeight_visible(p->wg_sizelist);
			}
			break;
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_topct *p = (_topct *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == MLK_WIDGET(p->toollist))
			{
				//ツールリスト: 情報を更新

				_update_iteminfo(p);

				if(ev->notify.notify_type != 1)
					PanelBrushOpt_setValue();
			}
			else if(ev->notify.widget_from == p->wg_sizelist)
			{
				//サイズリスト
				// param1 = サイズ値

				drawBrushParam_setSize(ev->notify.param1);

				PanelToolList_updateBrushSize();
				PanelBrushOpt_updateBrushSize();
			}
			else
				_event_notify(p, (mEventNotify *)ev);
			break;
	}

	return 1;
}

/* mSplitter 対象取得関数*/

static int _splitter_target(mSplitter *p,mSplitterTarget *info)
{
	info->wgprev = (mWidget *)info->param;
	info->wgnext = p->wg.next;

	info->prev_cur = mWidgetGetHeight_visible(info->wgprev);
	info->next_cur = mWidgetGetHeight_visible(info->wgnext);

	info->prev_min = 10;
	info->next_min = 0;

	info->prev_max = info->next_max = info->prev_cur + info->next_cur;

	return 1;
}

/* label + ValueBar 作成 */

static ValueBar *_create_label_bar(mWidget *parent,const char *label,int wid,int dig,int min,int max)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, label);
	
	return ValueBar_new(parent, wid, MLF_EXPAND_W, dig, min, max, min);
}

/* mPanel 内容作成 */

static mWidget *_panel_create_handle(mPanel *panel,int id,mWidget *parent)
{
	_topct *p;
	mWidget *ct;
	mSplitter *sp;

	//コンテナ (トップ)

	p = (_topct *)mContainerNew(parent, sizeof(_topct));

	p->wg.event = _event_handle;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.notify_to = MWIDGET_NOTIFYTO_SELF;

	mContainerSetPadding_pack4(MLK_CONTAINER(p), MLK_MAKE32_4(0,2,0,0));

	//---- アイテムリスト

	p->toollist = ToolListView_new(MLK_WIDGET(p), APPWIDGET->font_panel);

	//------ 選択名、ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 3, MLF_EXPAND_W, 0);

	mContainerSetPadding_pack4(MLK_CONTAINER(ct), MLK_MAKE32_4(2,2,2,0));

	//保存

	p->wg_btt_save = widget_createSaveButton(ct, WID_BTT_SAVE, MLF_MIDDLE, 0);

	//設定ボタン

	p->wg_btt_opt = widget_createImgButton_1bitText(ct, WID_BTT_SETTING, MLF_MIDDLE, 0, g_img_setting, 15, 0);

	//アイテム名

	p->lb_selname = mLabelCreate(ct, MLF_EXPAND_W | MLF_MIDDLE, 0, MLABEL_S_BORDER, NULL);

	//編集ボタン

	widget_createEditButton(ct, WID_BTT_EDIT, MLF_MIDDLE, 0);

	//------ ブラシ簡易設定

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 4, 5, MLF_EXPAND_W, MLK_MAKE32_4(2,6,4,4));

	p->bar_size = _create_label_bar(ct, MLK_TR2(TRGROUP_WORD, TRID_WORD_SIZE),
		WID_BAR_SIZE, 1, 0, 100);
	
	p->bar_opacity = _create_label_bar(ct, MLK_TR2(TRGROUP_WORD, TRID_WORD_DENSITY),
		WID_BAR_OPACITY, 0, 1, 100);

	//---- サイズリスト

	//splitter

	sp = mSplitterNew(MLK_WIDGET(p), 0, MSPLITTER_S_HORZ);

	sp->wg.id = WID_SPLITTER;

	mSplitterSetFunc_getTarget(sp, _splitter_target, (intptr_t)p->toollist);

	//

	p->wg_sizelist = SizeListView_new(MLK_WIDGET(p), &APPDRAW->tlist->buf_sizelist);

	//------

	_update_iteminfo(p);

	return (mWidget *)p;
}

/** ツールリスト作成 */

void PanelToolList_new(mPanelState *state)
{
	mPanel *p;

	p = Panel_new(PANEL_TOOLLIST, 0, 0, 0, state, _panel_create_handle);

	mPanelCreateWidget(p);
}


//===========================
// 外部から呼ばれる関数
//===========================


/* _topct 取得 */

static _topct *_get_topct(void)
{
	return (_topct *)Panel_getContents(PANEL_TOOLLIST);
}

/** ツールリストの更新 */

void PanelToolList_updateList(void)
{
	_topct *p = _get_topct();

	if(p)
		ToolListView_update(p->toollist);
}

/** ブラシサイズの更新 */

void PanelToolList_updateBrushSize(void)
{
	_topct *p = _get_topct();

	if(p)
	{
		BrushEditData *pb = APPDRAW->tlist->brush;
	
		ValueBar_setStatus(p->bar_size, pb->v.size_min, pb->v.size_max, pb->v.size);
	}
}

/** ブラシ濃度の更新 */

void PanelToolList_updateBrushOpacity(void)
{
	_topct *p = _get_topct();

	if(p)
		ValueBar_setPos(p->bar_opacity, APPDRAW->tlist->brush->v.opacity);
}


