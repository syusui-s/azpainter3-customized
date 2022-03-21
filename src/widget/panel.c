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
 * パネル処理
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_panel.h"

#include "def_widget.h"
#include "def_config.h"

#include "panel.h"

#include "trid.h"


//------------------
//各パネルの ID (作成時/配置の設定時に使う)

static const char g_panel_id[] = {'t','s','b','o','l','c','w','p','r','v','i','f',0};

//------------------

/*-------------

pane_layout
	"0123" の4文字が配置順に並ぶ。
	0 = キャンバス、1-3 = ペイン番号。

panel_layout
	ID の文字が配置順に並んでいる。
	':' でペインの区切り。
	
---------------*/



//=========================
// sub
//=========================


/* パネルソート関数 */

static int _sort_handle(mPanel *p,int id1,int id2)
{
	char *pc,*p1,*p2;

	//id の文字のポインタ位置

	p1 = p2 = NULL;

	for(pc = APPCONF->panel.panel_layout; *pc; pc++)
	{
		if(*pc == id1)
			p1 = pc;
		else if(*pc == id2)
			p2 = pc;

		if(p1 && p2) break;
	}

	return (p1 > p2);
}

/* 内容作成時のハンドラ */

static mWidget *_create_handle(mPanel *p,int id,mWidget *parent)
{
	mToplevel *top;
	PanelHandle_create func;

	//ウィンドウモード時は、アクセラレータをセット
	// :パネルのウィンドウにフォーカスがあっても、ショートカットキーが動作するように

	top = mPanelGetWindow(p);
	if(top)
		top->top.accelerator = mToplevelGetAccelerator(MLK_TOPLEVEL(APPWIDGET->mainwin));

	//作成

	func = (PanelHandle_create)mPanelGetParam1(p);
	
	return (func)(p, id, parent);
}

/* パネル番号から格納時の親をセット */

static void _set_store_parent(mPanel *p,int no)
{
	int n;

	n = Panel_getPaneNo(no);

	mPanelSetStoreParent(p, APPWIDGET->pane[n]);
}


//=========================
// main
//=========================


/** パネル作成
 *
 * 作成後、mPanelCreateWidget() でウィジェットを作成すること。
 *
 * no: パネルの番号
 * exsize: 追加するデータサイズ
 * panel_style: 追加するパネルスタイル */

mPanel *Panel_new(int no,int exsize,uint32_t panel_style,
	uint32_t window_style,mPanelState *state,PanelHandle_create create)
{
	mPanel *p;

	//mPanel 作成
	// :ペインの再レイアウトは PANEL イベント時に手動で実行する。

	p = mPanelNew(g_panel_id[no], exsize,
		MPANEL_S_ALL_BUTTONS | MPANEL_S_NO_RELAYOUT | panel_style);

	APPWIDGET->panel[no] = p;

	//設定

	mPanelSetParam1(p, (intptr_t)create);
	mPanelSetTitle(p, MLK_TR2(TRGROUP_PANEL_NAME, no));
	mPanelSetState(p, state);
	mPanelSetFont(p, APPWIDGET->font_panel);
	mPanelSetFunc_create(p, _create_handle);
	mPanelSetFunc_sort(p, _sort_handle);
	mPanelSetNotifyWidget(p, MLK_WIDGET(APPWIDGET->mainwin));

	_set_store_parent(p, no);

	mPanelSetWindowInfo(p, MLK_WINDOW(APPWIDGET->mainwin),
		MTOPLEVEL_S_NORMAL | MTOPLEVEL_S_PARENT | MTOPLEVEL_S_NO_INPUT_METHOD | window_style);

	return p;
}

/** 内容ウィジェットが作成されているか */

mlkbool Panel_isCreated(int no)
{
	return mPanelIsCreated(APPWIDGET->panel[no]);
}

/** 内容ウィジェットが実際に画面上に表示されているか
 *
 * 畳まれている場合は FALSE */

mlkbool Panel_isVisible(int no)
{
	return mPanelIsVisibleContents(APPWIDGET->panel[no]);
}

/** 拡張データのポインタを取得 */

void *Panel_getExPtr(int no)
{
	return mPanelGetExPtr(APPWIDGET->panel[no]);
}

/** 内容ウィジェットを取得
 *
 * 閉じている状態なら NULL */

mWidget *Panel_getContents(int no)
{
	return mPanelGetContents(APPWIDGET->panel[no]);
}

/** ダイアログ表示時の親ウィンドウを取得
 *
 * ウィンドウ状態なら、そのウィンドウ。
 * 格納状態なら、メインウィンドウ。 */

mWindow *Panel_getDialogParent(int no)
{
	mWindow *win;

	win = (mWindow *)mPanelGetWindow(APPWIDGET->panel[no]);
	if(!win) win = MLK_WINDOW(APPWIDGET->mainwin);

	return win;
}

/** パネルの再レイアウト */

void Panel_relayout(int no)
{
	mPanelReLayout(APPWIDGET->panel[no]);
}

/** ID からパネル番号を取得
 *
 * return: -1 で見つからなかった */

int Panel_id_to_no(int id)
{
	const char *pc;
	int no = 0;
	
	for(pc = g_panel_id; *pc; pc++, no++)
	{
		if(*pc == id) return no;
	}

	return -1;
}

/** パネル番号から、配置するペインの番号を取得 */

int Panel_getPaneNo(int no)
{
	char *pc;
	int paneno = 0;

	no = g_panel_id[no];

	for(pc = APPCONF->panel.panel_layout; *pc && *pc != no; pc++)
	{
		if(*pc == ':') paneno++;
	}

	if(paneno >= PANEL_PANE_NUM)
		paneno = PANEL_PANE_NUM - 1;

	return paneno;
}


//============================
// 全パネルの処理
//============================


/** 全パネルの破棄 */

void Panels_destroy(void)
{
	int i;

	for(i = 0; i < PANEL_NUM; i++)
	{
		mPanelDestroy(APPWIDGET->panel[i]);

		APPWIDGET->panel[i] = NULL;
	}
}

/** 初期表示 (ウィンドウの表示) */

void Panels_showInit(void)
{
	int i;

	for(i = 0; i < PANEL_NUM; i++)
		mPanelShowWindow(APPWIDGET->panel[i]);
}

/** 全てウィンドウモードまたは格納に切り替え
 *
 * type: [0]格納 [1]ウィンドウ [負]反転 */

void Panels_toggle_winmode(int type)
{
	int i;

	for(i = 0; i < PANEL_NUM; i++)
		mPanelSetWindowMode(APPWIDGET->panel[i], type);
}

/** パネルを再配置 (パネル配置変更時) */

void Panels_relocate(void)
{
	int i;

	for(i = 0; i < PANEL_NUM; i++)
	{
		//格納時の親を再セット
		_set_store_parent(APPWIDGET->panel[i], i);

		mPanelStoreReArrange(APPWIDGET->panel[i], FALSE);
	}
}

