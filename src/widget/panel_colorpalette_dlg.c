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
 * (Panel)カラーパレットのダイアログ
 *
 * [中間色設定] [パレット設定] [パレットリスト]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_lineedit.h"
#include "mlk_listview.h"
#include "mlk_iconbar.h"
#include "mlk_event.h"
#include "mlk_columnitem.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_list.h"

#include "def_draw.h"

#include "appresource.h"
#include "palettelist.h"
#include "widget_func.h"

#include "trid.h"
#include "trid_colorpalette.h"


/***************************************
 * 中間色: 設定ダイアログ
 ***************************************/


typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[DRAW_GRADBAR_NUM];
}_grad_optdlg;


/* 作成 */

static _grad_optdlg *_grad_optdlg_create(mWindow *parent)
{
	_grad_optdlg *p;
	mWidget *ct;
	mLineEdit *le;
	const char *label;
	mStr str = MSTR_INIT;
	int i;

	p = (_grad_optdlg *)mDialogNew(parent, sizeof(_grad_optdlg), MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;

	p->wg.event = mDialogEventDefault_okcancel;

	MLK_TRGROUP(TRGROUP_PANEL_COLOR_PALETTE);

	mToplevelSetTitle(MLK_TOPLEVEL(p), MLK_TR(TRID_GRAD_TITLE));

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	//------

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 7, 8, 0, 0);

	label = MLK_TR(TRID_GRAD_STEP);

	for(i = 0; i < DRAW_GRADBAR_NUM; i++)
	{
		mStrSetFormat(&str, "%s(%d)", label, i + 1);
		mLabelCreate(ct, MLF_MIDDLE, 0, MLABEL_S_COPYTEXT, str.buf);

		//

		p->edit[i] = le = mLineEditCreate(ct, 0, 0, 0, MLINEEDIT_S_SPIN);

		mLineEditSetWidth_textlen(le, 6);
		mLineEditSetNumStatus(le, 0, 64, 0);
		mLineEditSetNum(le, APPDRAW->col.gradcol[i][0] >> 24);
	}

	mStrFree(&str);

	mLabelCreate(MLK_WIDGET(p), 0, MLK_MAKE32_4(0,12,0,0), MLABEL_S_BORDER, MLK_TR(TRID_GRAD_STEP_HELP));

	//

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** 中間色設定ダイアログ実行 */

mlkbool PanelColorPalette_dlg_gradopt(mWindow *parent)
{
	_grad_optdlg *p;
	int i,n;
	mlkbool ret;

	p = _grad_optdlg_create(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);
	if(ret)
	{
		for(i = 0; i < DRAW_GRADBAR_NUM; i++)
		{
			n = mLineEditGetNum(p->edit[i]);
			if(n < 3) n = 0;

			APPDRAW->col.gradcol[i][0] &= 0xffffff;
			APPDRAW->col.gradcol[i][0] |= (uint32_t)n << 24;
		}
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


/***************************************
 * パレット設定
 ***************************************/


typedef struct
{
	MLK_DIALOG_DEF

	mLineEdit *edit[4];
}_paloptdlg;


/* 作成 */

static _paloptdlg *_paloptdlg_create(mWindow *parent)
{
	_paloptdlg *p;
	PaletteListItem *pi;
	int i,min,max,val;

	MLK_TRGROUP(TRGROUP_PANEL_COLOR_PALETTE);

	p = (_paloptdlg *)widget_createDialog(parent, sizeof(_paloptdlg),
		MLK_TR(TRID_PALOPT_TITLE), mDialogEventDefault_okcancel);

	if(!p) return NULL;

	p->ct.sep = 5;

	pi = (PaletteListItem *)APPDRAW->col.cur_pallist;

	//---- 項目

	for(i = 0; i < 4; i++)
	{
		switch(i)
		{
			case 0:
				min = 1;
				max = PALETTE_MAX_COLNUM;
				val = pi->palnum;
				break;
			case 1:
			case 2:
				min = 4, max = 100;
				val = (i == 1)? pi->cellw: pi->cellh;
				break;
			case 3:
				min = 0, max = 500;
				val = pi->xmaxnum;
				break;
		}
	
		p->edit[i] = widget_createLabelEditNum(MLK_WIDGET(p), MLK_TR(TRID_PALOPT_COLNUM + i),
			6, min, max, val);
	}

	//--- ボタン

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,10,0,0));

	return p;
}

/* OK 時 */

static int _paloptdlg_ok(_paloptdlg *p)
{
	PaletteListItem *pi = (PaletteListItem *)APPDRAW->col.cur_pallist;
	int i,n[4];

	for(i = 0; i < 4; i++)
		n[i] = mLineEditGetNum(p->edit[i]);

	//色数

	PaletteListItem_resize(pi, n[0]);

	pi->cellw = n[1];
	pi->cellh = n[2];
	pi->xmaxnum = n[3];

	return 0;
}

/** パレット設定ダイアログ実行 */

mlkbool PanelColorPalette_dlg_palopt(mWindow *parent)
{
	_paloptdlg *p;
	mlkbool ret;

	p = _paloptdlg_create(parent);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		_paloptdlg_ok(p);

		APPDRAW->col.pal_fmodify = TRUE;
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}


/***************************************
 * パレット: パレットリスト
 ***************************************/


typedef struct
{
	MLK_DIALOG_DEF

	mListView *list;
}_pallist_dlg;


/* 追加 */

static void _pallist_cmd_add(_pallist_dlg *p)
{
	mStr str = MSTR_INIT;
	const char *text;
	PaletteListItem *pi;
	mColumnItem *item;

	if(APPDRAW->col.list_pal.num >= 50) return;

	text = MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME);

	if(mSysDlg_inputText(MLK_WINDOW(p),
		text, text, MSYSDLG_INPUTTEXT_F_NOT_EMPTY, &str))
	{
		pi = PaletteList_add(&APPDRAW->col.list_pal, str.buf);
		if(pi)
		{
			//リストアイテム
			
			item = mListViewAddItem_text_static_param(p->list, pi->name, (intptr_t)pi);

			mListViewSetFocusItem(p->list, item);

			//選択がない場合、セット

			if(!APPDRAW->col.cur_pallist)
				APPDRAW->col.cur_pallist = (mListItem *)pi;
		}

		mStrFree(&str);
	}
}

/* 削除 */

static void _pallist_cmd_del(_pallist_dlg *p,mColumnItem *item)
{
	if(!item) return;
	
	//確認
	
	if(mMessageBox(MLK_WINDOW(p),
		MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_TITLE_CONFIRM),
		MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_DELETE_NO_RESTORE),
		MMESBOX_YES | MMESBOX_CANCEL, MMESBOX_YES) != MMESBOX_YES)
		return;

	//

	mListDelete(&APPDRAW->col.list_pal, (mListItem *)item->param);

	item = mListViewDeleteItem_focus(p->list);

	APPDRAW->col.cur_pallist = (item)? (mListItem *)item->param: NULL;
}

/* 上/下へ */

static void _pallist_cmd_updown(_pallist_dlg *p,mColumnItem *item,mlkbool down)
{
	if(!item) return;

	if(mListViewMoveItem_updown(p->list, item, down))
		mListMoveUpDown(&APPDRAW->col.list_pal, (mListItem *)item->param, !down);
}

/* 名前変更 */

static void _pallist_cmd_rename(_pallist_dlg *p,mColumnItem *item)
{
	mStr str = MSTR_INIT;
	const char *text;
	PaletteListItem *pi;

	if(!item) return;

	pi = (PaletteListItem *)item->param;

	text = MLK_TR2(TRGROUP_WORD, TRID_WORD_NAME);

	mStrSetText(&str, pi->name);

	if(mSysDlg_inputText(MLK_WINDOW(p),
		text, text,
		MSYSDLG_INPUTTEXT_F_NOT_EMPTY | MSYSDLG_INPUTTEXT_F_SET_DEFAULT, &str))
	{
		mFree(pi->name);
		pi->name = mStrdup(str.buf);

		//リスト

		mListViewSetItemText_static(p->list, item, pi->name);
	}

	mStrFree(&str);
}

/* COMMAND イベント */

static void _pallist_event_command(_pallist_dlg *p,int id)
{
	mColumnItem *item;

	item = mListViewGetFocusItem(p->list);

	switch(id)
	{
		//追加
		case APPRES_LISTEDIT_ADD:
			_pallist_cmd_add(p);
			break;
		//削除
		case APPRES_LISTEDIT_DEL:
			_pallist_cmd_del(p, item);
			break;
		//上へ/下へ
		case APPRES_LISTEDIT_UP:
		case APPRES_LISTEDIT_DOWN:
			_pallist_cmd_updown(p, item, (id == APPRES_LISTEDIT_DOWN));
			break;
		//名前変更
		case APPRES_LISTEDIT_RENAME:
			_pallist_cmd_rename(p, item);
			break;
	}
}

/* イベント */

static int _pallist_event_handle(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_COMMAND:
			_pallist_event_command((_pallist_dlg *)wg, ev->cmd.id);
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* 作成 */

static _pallist_dlg *_pallist_dlg_create(mWindow *parent)
{
	_pallist_dlg *p;
	mListView *list;
	PaletteListItem *pi;
	const uint8_t bttno[] = {
		APPRES_LISTEDIT_ADD, APPRES_LISTEDIT_DEL,
		APPRES_LISTEDIT_UP, APPRES_LISTEDIT_DOWN, APPRES_LISTEDIT_RENAME, 255
	};

	p = (_pallist_dlg *)widget_createDialog(parent, sizeof(_pallist_dlg),
		MLK_TR2(TRGROUP_PANEL_COLOR_PALETTE, TRID_PALLIST_TITLE), _pallist_event_handle);

	if(!p) return NULL;

	//リスト

	p->list = list = mListViewCreate(MLK_WIDGET(p), 0, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH,	MSCROLLVIEW_S_HORZVERT_FRAME);

	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(list), 15, 12);

	MLK_LIST_FOR(APPDRAW->col.list_pal, pi, PaletteListItem)
	{
		mListViewAddItem_text_static_param(list, pi->name, (intptr_t)pi);
	}

	mListViewSetFocusItem_param(list, (intptr_t)APPDRAW->col.cur_pallist);

	//ボタン

	widget_createListEdit_iconbar_btt(MLK_WIDGET(p), bttno, FALSE);

	return p;
}

/** ダイアログ実行 (データは直接変更する) */

void PanelColorPalette_dlg_pallist(mWindow *parent)
{
	_pallist_dlg *p;

	p = _pallist_dlg_create(parent);
	if(!p) return;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	mDialogRun(MLK_DIALOG(p), TRUE);
}

