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
 * ツールバー設定ダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_arrowbutton.h"
#include "mlk_listview.h"
#include "mlk_columnitem.h"
#include "mlk_event.h"

#include "def_toolbar_btt.h"
#include "appresource.h"

#include "widget_func.h"

#include "trid.h"


//----------------------

typedef struct
{
	MLK_DIALOG_DEF

	mListView *list[2];
}_dialog;

enum
{
	TRID_TITLE,
	TRID_ITEM_SEP
};

enum
{
	WID_LIST_LEFT = 100,
	WID_LIST_RIGHT,
	WID_BTT_TO_LEFT,
	WID_BTT_TO_RIGHT,
	WID_BTT_UP,
	WID_BTT_DOWN
};

#define BTTNO_END 255
#define BTTNO_SEP 254

//----------------------


/* 初期リストセット */

static void _set_list(_dialog *p,const uint8_t *buf)
{
	mListView *list;
	const char *txt_sep;
	int i,j,no;

	txt_sep = MLK_TR(TRID_ITEM_SEP);

	MLK_TRGROUP(TRGROUP_TOOLBAR_TOOLTIP);

	if(!buf) buf = AppResource_getToolBarDefaultBtts();

	//現在のリスト

	list = p->list[0];

	for(i = 0; buf[i] != BTTNO_END; i++)
	{
		no = buf[i];

		if(no == BTTNO_SEP)
			mListViewAddItem_text_static_param(list, txt_sep, BTTNO_SEP);
		else
			mListViewAddItem_text_static_param(list, MLK_TR(no), no);
	}

	//残りのリスト

	list = p->list[1];

	mListViewAddItem_text_static_param(list, txt_sep, BTTNO_SEP);

	for(i = 0; i < TOOLBAR_BTT_NUM; i++)
	{
		//現在のリストにあるか

		for(j = 0; buf[j] != BTTNO_END && buf[j] != i; j++);

		//なければ追加

		if(buf[j] == BTTNO_END)
			mListViewAddItem_text_static_param(list, MLK_TR(i), i);
	}
}

/* リストの選択項目を左右に移動
 *
 * listno: 移動元のリスト番号 */

static void _move_list(_dialog *p,int listno)
{
	mListView *listsrc,*listdst;
	mColumnItem *src,*dst;
	int srcno;

	listsrc = p->list[listno];
	listdst = p->list[!listno];

	//アイテム

	src = mListViewGetFocusItem(listsrc);
	if(!src) return;
	
	dst = mListViewGetFocusItem(listdst);

	srcno = src->param;

	//

	if(listno == 0)
	{
		//左 -> 右
		// :区切りは追加しない。また、常に終端へ移動。
		// :左は、削除後、次のアイテムを選択。

		if(srcno != BTTNO_SEP)
			mListViewAddItem_text_static_param(listdst, src->text, srcno);

		mListViewDeleteItem_focus(listsrc);
	}
	else
	{
		//左 <- 右
		// :左の選択位置へ挿入。選択位置がない場合は終端。
		// :次を続けて移動しやすいように、移動後の次のアイテムを選択。
		// :右が区切りの場合は削除しない。
		
		dst = mListViewInsertItem(listdst, dst, src->text, -1, 0, srcno);

		if(dst)
			mListViewSetFocusItem(listdst, MLK_COLUMNITEM(dst->i.next));

		if(srcno != BTTNO_SEP)
			mListViewDeleteItem_focus(listsrc);
	}
}


//==========================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY)
	{
		_dialog *p = (_dialog *)wg;
		
		switch(ev->notify.id)
		{
			//右から左へ
			case WID_BTT_TO_LEFT:
				_move_list(p, 1);
				break;
			//左から右へ
			case WID_BTT_TO_RIGHT:
				_move_list(p, 0);
				break;
			//左のリスト: up/down
			case WID_BTT_UP:
			case WID_BTT_DOWN:
				mListViewMoveItem_updown(p->list[0], NULL, (ev->notify.id == WID_BTT_DOWN));
				break;
		}
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,uint8_t *bttbuf)
{
	_dialog *p;
	mWidget *ct,*ct2;

	MLK_TRGROUP(TRGROUP_DLG_TOOLBAR_CUSTOMIZE);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);

	if(!p) return NULL;

	//---- リスト

	ct = mContainerCreateHorz(MLK_WIDGET(p), 8, MLF_EXPAND_WH, 0);

	//左

	p->list[0] = mListViewCreate(ct, 0, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	//ボタン

	ct2 = mContainerCreateVert(ct, 8, MLF_MIDDLE, 0);

	mArrowButtonCreate_minsize(ct2, WID_BTT_TO_LEFT, 0, 0, MARROWBUTTON_S_LEFT | MARROWBUTTON_S_FONTSIZE, 26);

	mArrowButtonCreate_minsize(ct2, WID_BTT_TO_RIGHT, 0, 0, MARROWBUTTON_S_RIGHT | MARROWBUTTON_S_FONTSIZE, 26);

	//右

	p->list[1] = mListViewCreate(ct, 0, MLF_EXPAND_WH, 0,
		MLISTVIEW_S_AUTO_WIDTH, MSCROLLVIEW_S_HORZVERT_FRAME);

	//---- 下ボタン

	ct = mContainerCreateHorz(MLK_WIDGET(p), 8, 0, MLK_MAKE32_4(0,6,0,0));

	mArrowButtonCreate_minsize(ct, WID_BTT_UP, 0, 0, MARROWBUTTON_S_UP | MARROWBUTTON_S_FONTSIZE, 26);
	mArrowButtonCreate_minsize(ct, WID_BTT_DOWN, 0, 0, MARROWBUTTON_S_DOWN | MARROWBUTTON_S_FONTSIZE, 26);

	//---- OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	//-------
	
	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->list[0]), 15, 20);
	mWidgetSetInitSize_fontHeightUnit(MLK_WIDGET(p->list[1]), 15, 20);

	//リストセット

	_set_list(p, bttbuf);

	return p;
}

/* OK */

static void _set_ok(_dialog *p,uint8_t **ppbuf,int *dst_size)
{
	mColumnItem *pi;
	int num;
	uint8_t *buf,*pd;

	num = mListViewGetItemNum(p->list[0]);

	buf = (uint8_t *)mMalloc(num + 1);
	if(!buf) return;

	//セット

	pd = buf;
	pi = mListViewGetTopItem(p->list[0]);

	for(; pi; pi = MLK_COLUMNITEM(pi->i.next))
		*(pd++) = pi->param;

	*pd = BTTNO_END;

	//

	mFree(*ppbuf);
	
	*ppbuf = buf;
	*dst_size = num + 1;
}

/** ダイアログ実行 */

mlkbool ToolBarCustomizeDlg_run(mWindow *parent,uint8_t **ppbuf,int *dst_size)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent, *ppbuf);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		_set_ok(p, ppbuf, dst_size);

	mWidgetDestroy(MLK_WIDGET(p));
	
	return ret;
}

