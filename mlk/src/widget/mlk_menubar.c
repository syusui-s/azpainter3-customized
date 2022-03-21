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
 * mMenuBar [メニューバー]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_menubar.h"
#include "mlk_window.h"
#include "mlk_menu.h"
#include "mlk_font.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"
#include "mlk_pv_menu.h"


//------------------

#define _PADDING_X   6  //各項目テキストごとの余白
#define _PADDING_Y   4

#define _MENUITEM_TOP(p)  ((mMenuItem *)(p)->menubar.menu->list.top)

//------------------



//=======================
// sub
//=======================


/** 16bit 配列からメニューアイテムを新規作成
 *
 * radio: TRUE でラジオタイプあり */

static void _create_menu_array16(mMenuBar *p,const uint16_t *buf,mlkbool radio)
{
	mMenu *top;

	//すでに存在する

	if(p->menubar.menu) return;

	//トップメニュー作成

	top = mMenuNew();
	if(!top) return;

	p->menubar.menu = top;

	//項目追加

	if(radio)
		mMenuAppendTrText_array16_radio(top, buf);
	else
		mMenuAppendTrText_array16(top, buf);
}

/** メニューバーによるメニューウィンドウが実行中か */

static mlkbool _is_running_menuwindow(mMenuBar *p)
{
	mAppRunData *run = MLKAPP->run_current;

	return (run && run->type == MAPPRUN_TYPE_POPUP && run->widget_sub == MLK_WIDGET(p));
}

/** ポインタ位置からアイテム取得 */

static mMenuItem *_get_item_point(mMenuBar *p,int x)
{
	mMenuItem *pi;
	int left = 0,w;
	
	for(pi = _MENUITEM_TOP(p); pi && left <= x; pi = MLK_MENUITEM(pi->i.next))
	{
		w = pi->text_w + _PADDING_X * 2;

		if(x < left + w) return pi;
		
		left += w;
	}
	
	return NULL;
}

/** アイテムのサブメニュー表示時の矩形を取得 */

static void _get_item_submenu_box(mMenuBar *p,mMenuItem *item,mBox *box)
{
	mMenuItem *pi;
	int x = 0;

	for(pi = _MENUITEM_TOP(p); pi && pi != item; pi = MLK_MENUITEM(pi->i.next))
		x += pi->text_w + _PADDING_X * 2;
	
	box->x = x;
	box->y = 0;
	box->w = item->text_w + _PADDING_X * 2;
	box->h = p->wg.h;
}

/** サブメニュー表示 */

static void _show_submenu(mMenuBar *p,mMenuItem *item)
{
	mBox box;

	do
	{
		//選択アイテム

		p->menubar.item_sel = item;

		mWidgetRedraw(MLK_WIDGET(p));

		//ポップアップ表示
		
		_get_item_submenu_box(p, item, &box);

		item = __mMenuWindowRunMenuBar(p, &box, item->info.submenu, item);

		//ポップアップの表示中、
		//mMenuBar 内でのポインタ移動や、左右キーにより、選択項目が変わった場合、
		//移動先のアイテムが返る。
		//戻り値が NULL で、ポップアップ終了。

	} while(item);
	
	//終了
	
	p->menubar.item_sel = NULL;

	mWidgetRedraw(MLK_WIDGET(p));
}


//=======================
// 他から呼ばれる関数
//=======================


/** Alt+? のホットキー押しによるメニュー表示
 *
 * トップレベルウィンドウのキー押しイベント処理内で呼ばれる。
 * (mlk_pv_event.c)
 *
 * return: メニューを表示した場合、TRUE */

mlkbool __mMenuBarShowHotkey(mMenuBar *p,uint32_t key,uint32_t state)
{
	mMenuItem *item;

	//装飾キーに Alt 以外も押されている
	
	if((state & MLK_STATE_MASK_MODS) != MLK_STATE_ALT)
		return FALSE;

	//0-9,A-Z,a-z のみ有効

	if(key >= 'a' && key <= 'z')
		key -= 0x20;
	
	if(!((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z')))
		return FALSE;

	//キーから対象アイテム検索
	
	item = __mMenuFindItem_hotkey(p->menubar.menu, (char)key);
	
	if(!item || !__mMenuItemIsShowableSubmenu(item))
		return FALSE;

	//表示
	
	_show_submenu(p, item);
	
	return TRUE;
}


//=======================
// main
//=======================


/**@ データ削除 */

void mMenuBarDestroy(mWidget *p)
{
	//mMenu 削除
	
	mMenuDestroy(MLK_MENUBAR(p)->menubar.menu);

	//トップレベルとの関連付けを解除

	mToplevelAttachMenuBar(MLK_TOPLEVEL(p->toplevel), NULL);
}

/**@ メニューバー作成
 * 
 * @d:作成と同時に、親のトップレベルに関連付けられる。\
 * レイアウトフラグは、MLF_EXPAND_W がデフォルトで付く。 */

mMenuBar *mMenuBarNew(mWidget *parent,int size,uint32_t fstyle)
{
	mMenuBar *p;
	
	if(size < sizeof(mMenuBar))
		size = sizeof(mMenuBar);
	
	p = (mMenuBar *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mMenuBarDestroy;
	p->wg.calc_hint = mMenuBarHandle_calcHint;
	p->wg.draw = mMenuBarHandle_draw;
	p->wg.event = mMenuBarHandle_event;

	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.flayout = MLF_EXPAND_W;

	p->menubar.fstyle = fstyle;
	
	//トップレベルと関連付け

	mToplevelAttachMenuBar(MLK_TOPLEVEL(p->wg.toplevel), p);
	
	return p;
}

/**@ 作成 */

mMenuBar *mMenuBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mMenuBar *p;

	p = mMenuBarNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);
	
	return p;
}

/**@ メニューデータ取得 */

mMenu *mMenuBarGetMenu(mMenuBar *p)
{
	return p->menubar.menu;
}

/**@ メニューデータをセット
 *
 * @d:※メニューバーの破棄時は、セットされたデータも破棄される。 */

void mMenuBarSetMenu(mMenuBar *p,mMenu *menu)
{
	p->menubar.menu = menu;
}

/**@ メニューアイテムにサブメニューをセット */

void mMenuBarSetSubmenu(mMenuBar *p,int id,mMenu *submenu)
{
	mMenuSetItemSubmenu(p->menubar.menu, id, submenu);
}

/**@ 16bit 配列からメニューアイテムを新規作成
 *
 * @d:※すでに項目がセットされている場合は、何もしない。\
 *  16380 未満の値は、項目の文字列 ID。\
 *  MMENUBAR_ARRAY16_* で、特殊な値。CHECK/RADIO は、フラグとして使う。 */

void mMenuBarCreateMenuTrArray16(mMenuBar *p,const uint16_t *buf)
{
	_create_menu_array16(p, buf, FALSE);
}

/**@ 16bit 配列からメニューアイテムを新規作成 (ラジオタイプあり) */

void mMenuBarCreateMenuTrArray16_radio(mMenuBar *p,const uint16_t *buf)
{
	_create_menu_array16(p, buf, TRUE);
}


//=======================
// ハンドラ
//=======================


/**@ calc_hint ハンドラ関数 */

void mMenuBarHandle_calcHint(mWidget *wg)
{
	mMenuBar *p = MLK_MENUBAR(wg);
	mFont *font;
	
	font = mWidgetGetFont(wg);

	wg->hintW = 1;
	wg->hintH = mFontGetHeight(font) + _PADDING_Y * 2;

	if(p->menubar.fstyle & MMENUBAR_S_BORDER_BOTTOM)
		wg->hintH++;

	//テキストサイズ計算
	
	__mMenuInitTextSize(p->menubar.menu, font);
}

/* メニュー表示中のポインタ移動時
 *
 * mMenuBar からのポップアップ表示中は、
 * mMenuBar 上の POINTER#MOTION を受け付けるようになっている。 */

static void _event_motion(mMenuBar *p,int x)
{
	mMenuItem *item;

	if(_is_running_menuwindow(p))
	{
		//ポインタ下のアイテム

		item = _get_item_point(p, x);

		//選択変更
		// 選択不可能なアイテムの場合は何もしない。
		// ポップアップのメインループを一度終了させてから表示する。

		if(item
			&& item != p->menubar.item_sel
			&& __mMenuItemIsShowableSubmenu(item))
			__mMenuWindowMoveMenuBarItem(item);
	}
}

/* ボタン押し時
 *
 * [!] ポップアップ表示中は、ボタン押しイベントは来ない */

static void _event_press(mMenuBar *p,int x)
{
	mMenuItem *item;

	item = _get_item_point(p, x);
	
	if(item && __mMenuItemIsShowableSubmenu(item))
		_show_submenu(p, item);
}

/**@ event ハンドラ関数 */

int mMenuBarHandle_event(mWidget *wg,mEvent *ev)
{
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
				//MOTION
				_event_motion(MLK_MENUBAR(wg), ev->pt.x);
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//PRESS
				
				if(ev->pt.btt == MLK_BTT_LEFT || ev->pt.btt == MLK_BTT_RIGHT)
					_event_press(MLK_MENUBAR(wg), ev->pt.x);
			}
			break;

		case MEVENT_ENTER:
			_event_motion(MLK_MENUBAR(wg), ev->enter.x);
			break;
	}
	
	return 1;
}

/**@ draw ハンドラ関数 */

void mMenuBarHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mMenuBar *p = MLK_MENUBAR(wg);
	mFont *font;
	mMenuItem *pi;
	int x,w,itemh;
	uint32_t col;

	itemh = wg->h;
	
	//背景
	
	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(MENU_FACE));

	//下境界線

	if(p->menubar.fstyle & MMENUBAR_S_BORDER_BOTTOM)
	{
		itemh--;
		mPixbufLineH(pixbuf, 0, itemh, wg->w, MGUICOL_PIX(FRAME));
	}
	
	//メニュートップ項目

	if(!p->menubar.menu) return;
	
	font = mWidgetGetFont(wg);
	x = 0;
	
	for(pi = _MENUITEM_TOP(p); pi; pi = MLK_MENUITEM(pi->i.next))
	{
		w = pi->text_w + _PADDING_X * 2;
	
		if(pi == p->menubar.item_sel)
		{
			//選択時
			
			mPixbufFillBox(pixbuf, x, 0, w, itemh, MGUICOL_PIX(MENU_SELECT));
			
			col = MGUICOL_RGB(MENU_TEXT);
		}
		else
		{
			//通常時
			
			col = (pi->info.flags & MMENUITEM_F_DISABLE)
					? MGUICOL_RGB(MENU_TEXT_DISABLE): MGUICOL_RGB(MENU_TEXT);
		}
		
		//テキスト
		
		mFontDrawText_pixbuf_hotkey(font, pixbuf,
			_PADDING_X + x, _PADDING_Y, pi->info.text, -1, col);

		//次へ
		
		x += w;
	}
}

