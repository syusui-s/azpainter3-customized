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
 * mMenuWindow
 * メニューポップアップウィンドウ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_menubar.h"
#include "mlk_event.h"
#include "mlk_menu.h"
#include "mlk_guicol.h"
#include "mlk_key.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_window.h"
#include "mlk_pv_menu.h"


//------------------------

#define MLK_MENUWINDOW(p) ((mMenuWindow *)(p))

typedef struct _mMenuWindow mMenuWindow;

struct _mMenuWindow
{
	MLK_POPUP_DEF

	mMenu *menu;
	mMenuBar *menubar;

	mMenuItem *item_sel,		//現在選択されているアイテム
		*item_ret;				//[TOP] 終了時に返すアイテム
	mMenuWindow *menuwin_top,	//トップのウィンドウ
		*menuwin_sub,			//現在表示されているサブメニュー (NULL でなし)
		*menuwin_parent,		//親のウィンドウ (サブメニュー時)
		*menuwin_focus,			//[TOP] 現在フォーカスがあるウィンドウ
		*menuwin_scroll;		//[TOP] スクロール中の場合、スクロール対象のポップアップ
	mWidget *widget_send;		//イベントの送り先

	mSize winsize;		//ウィンドウサイズ (アイテムから計算したサイズ)
	int item_full_h,	//全アイテムの高さ
		scry;
	uint32_t flags;
};

//------------------------

enum
{
	MMENUWINDOW_F_HAVE_SCROLL = 1<<0,	//スクロールありの場合
	MMENUWINDOW_F_SCROLL_DOWN = 1<<1,	//スクロール操作、下方向か
	MMENUWINDOW_F_MENUBAR_CHANGE = 1<<2	//メニューバーからの表示時、ポインタ操作などにより表示項目変更
};

//------------------------

#define _FRAME_X     1		//横枠幅
#define _FRAME_Y     1		//縦枠幅
#define _LEFT_SPACE  18		//左枠とテキストの間の余白
#define _RIGHT_SPACE 18		//テキストと右枠の間の余白
#define _LABEL_SCKEY_SPACE 11    //ラベルとショートカットキーテキストの間の余白
#define _SCROLL_H    15    //スクロール部分の高さ 

#define _MENUITEM_TOP(p)        ((mMenuItem *)(p)->menu->list.top)
#define _IS_ITEM_CHECKRADIO(p)  ((p)->info.flags & (MMENUITEM_F_CHECK_TYPE | MMENUITEM_F_RADIO_TYPE))
#define _IS_ITEMFLAG_ON(p,name) (((p)->info.flags & MMENUITEM_F_ ## name) != 0)

#define _TIMERID_SUBMENU  0
#define _TIMERID_SCROLL   1

#define _SETSELITEM_F_SCROLL_TO     1	//アイテムが見える位置までスクロールする
#define _SETSELITEM_F_SUBMENU_DELAY 2	//サブメニュー遅延あり

//------------------------

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//------------------------



//==============================
// sub
//==============================


/** メニューウィンドウ破棄 */

static void _destroy_menuwindow(mMenuWindow *p)
{
	mMenuWindow *sub,*next;

	//サブメニューのウィンドウもすべて削除
	//[Wayland] 最後のサブウィンドウから順に削除しなければならない

	//最下層のサブメニュー

	for(sub = p; sub->menuwin_sub; sub = sub->menuwin_sub);

	//サブメニュー削除

	for(; sub != p; sub = next)
	{
		next = sub->menuwin_parent;
		
		mWidgetDestroy(MLK_WIDGET(sub));
	}

	//自身を削除

	mWidgetDestroy(MLK_WIDGET(p));
}

/** resize ハンドラ */

static void _resize_handle(mWidget *wg)
{
	mMenuWindow *p = MLK_MENUWINDOW(wg);

	//高さが小さい時はスクロールあり

	if(p->winsize.h != wg->h)
		p->flags |= MMENUWINDOW_F_HAVE_SCROLL;
}

/** ポップアップ終了 */

static void _end_popup(mMenuWindow *p,mMenuItem *ret)
{
	(p->menuwin_top)->item_ret = ret;
	
	mGuiQuit();
}

/** アイテムの表示 Y 位置取得 */

static int _get_item_y(mMenuWindow *p,mMenuItem *item)
{
	mMenuItem *pi;
	int y;
	
	y = _FRAME_Y;

	if(p->flags & MMENUWINDOW_F_HAVE_SCROLL)
		y += _SCROLL_H - p->scry;
	
	for(pi = _MENUITEM_TOP(p); pi && pi != item; pi = MLK_MENUITEM(pi->i.next))
		y += pi->info.height;

	return y;
}

/** MEVENT_MENU_POPUP : ポップアップ表示イベント (直接実行) */

static void _run_event_menu_popup(mMenuWindow *p,int submenu_id,mlkbool is_menubar)
{
	mEventMenuPopup ev;

	if(p->widget_send)
	{
		ev.type = MEVENT_MENU_POPUP;
		ev.widget = p->widget_send;
		ev.menu = p->menu;
		ev.submenu_id = submenu_id;
		ev.is_menubar = is_menubar;

		(ev.widget->event)(ev.widget, (mEvent *)&ev);
	}
}


//=============================
// sub - 初期化
//=============================


/** アイテムからサイズ情報をセット */

static void _init_menuitem_size(mMenuWindow *p)
{
	mMenuItem *pi;
	int tw = 0,h = 0,draw_w = 0,w;

	//アイテムのサイズ
	// tw : テキストの最大幅
	// draw_w : 独自描画時の最大幅
	// h : 全アイテムの高さ
	
	for(pi = _MENUITEM_TOP(p); pi; pi = MLK_MENUITEM(pi->i.next))
	{
		w = pi->text_w;
		if(pi->sckey_w) w += _LABEL_SCKEY_SPACE + pi->sckey_w;

		if(w > tw) tw = w;
	
		if(pi->info.draw && draw_w < pi->info.width) draw_w = pi->info.width;
		
		h += pi->info.height;
	}
	
	//全体の幅

	w = tw;
	
	if(w) w += _LEFT_SPACE + _RIGHT_SPACE;
	
	if(w < draw_w) w = draw_w;

	//

	p->item_full_h = h;

	p->winsize.w = w + _FRAME_X * 2;
	p->winsize.h = h + _FRAME_Y * 2;
}

/** メニューウィンドウ作成
 *
 * parent: NULL でトップ。サブメニューの場合は親ウィンドウ。 */

static mMenuWindow *_create_menuwindow(mMenuWindow *parent,mMenu *menu,mWidget *send)
{
	mMenuWindow *p;

	p = (mMenuWindow *)mPopupNew(sizeof(mMenuWindow), (parent)? MPOPUP_S_NO_GRAB: 0);
	if(!p) return NULL;

	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.resize = _resize_handle;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY;
	p->wg.fstate |= MWIDGET_STATE_ENABLE_KEY_REPEAT;

	p->menu = menu;
	p->widget_send = send;
	p->menuwin_parent = parent;

	if(!parent)
		//トップ
		p->menuwin_top = p->menuwin_focus = p;
	else
	{
		//サブメニュー
		
		p->win.parent = MLK_WIDGET(parent);

		p->menuwin_top = parent->menuwin_top;
		
		parent->menuwin_sub = p;
	}

	//アイテムのテキストサイズ初期化
	
	__mMenuInitTextSize(menu, MLKAPP->font_default);

	//ウィンドウサイズ

	_init_menuitem_size(p);

	return p;
}

/** サブメニューウィンドウを作成して表示
 * 
 * parent で現在選択されているアイテムのサブメニューを表示する */

static void _show_submenu(mMenuWindow *parent)
{
	mMenuWindow *p;
	mMenuItem *item;
	mBox box;
	
	item = parent->item_sel;

	//mMenuWindow 作成
	
	p = _create_menuwindow(parent, item->info.submenu, parent->widget_send);
	if(!p) return;

	//MEVENT_MENU_POPUP イベント
	
	_run_event_menu_popup(p, item->info.id, ((p->menuwin_top)->menubar != 0));

	//表示 (p->win.parent は作成時にセット済み)

	box.x = 2;
	box.w = parent->wg.w - 4;
	box.y = _get_item_y(parent, item) - _FRAME_Y - 1;
	box.h = 1;

	(MLKAPP->bkend.popup_show)(MLK_POPUP(p),
		0, 0, p->winsize.w, p->winsize.h, &box,
		MPOPUP_F_RIGHT | MPOPUP_F_TOP | MPOPUP_F_FLIP_X);
}


//=============================
// main
//=============================


/** 手動でメニューポップアップを表示
 *
 * send: イベントの通知先
 * return: 選択された項目 (NULL でキャンセルされた) */

mMenuItem *__mMenuWindowRunPopup(mWidget *parent,int x,int y,mBox *box,
	uint32_t flags,mMenu *menu,mWidget *send)
{
	mMenuWindow *p;
	mMenuItem *item;

	//作成

	p = _create_menuwindow(NULL, menu, send);
	if(!p) return NULL;

	//実行

	if(!mPopupRun(MLK_POPUP(p), parent, x, y, p->winsize.w, p->winsize.h, box, flags))
	{
		_destroy_menuwindow(p);
		return NULL;
	}
	
	//----- 終了
	
	item = p->item_ret;
	
	//COMMAND イベント送る
	
	if(send && item && (flags & MPOPUP_F_MENU_SEND_COMMAND))
	{
		mWidgetEventAdd_command(send, item->info.id,
			item->info.param1, MEVENT_COMMAND_FROM_MENU, menu);
	}
	
	_destroy_menuwindow(p);
	
	return item;
}

/** メニュバーから表示
 *
 * return: mMenuBar 上の操作やキー操作により、選択が変更される場合、変更先のアイテム。
 *  NULL でポップアップ終了。 */

mMenuItem *__mMenuWindowRunMenuBar(mMenuBar *menubar,mBox *box,mMenu *menu,mMenuItem *item)
{
	mMenuWindow *p;
	int is_change;

	//作成

	p = _create_menuwindow(NULL, menu, mWidgetGetNotifyWidget(MLK_WIDGET(menubar)));
	if(!p) return NULL;

	p->menubar = menubar;
	
	//MEVENT_MENU_POPUP イベント
	
	_run_event_menu_popup(p, item->info.id, TRUE);
	
	//実行

	p->win.parent = MLK_WIDGET(menubar);

	if(!(MLKAPP->bkend.popup_show)(MLK_POPUP(p), 0, 0, p->winsize.w, p->winsize.h, box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY))
	{
		_destroy_menuwindow(p);
		return NULL;
	}

	mGuiRunPopup(MLK_POPUP(p), p->win.parent); //mMenuBar 上のイベントを受け付ける
	
	//------- 終了
	
	item = p->item_ret;
	is_change = p->flags & MMENUWINDOW_F_MENUBAR_CHANGE;
		
	//MEVENT_COMMAND
	
	if(item && !is_change)
	{
		mWidgetEventAdd_command(p->widget_send,
			item->info.id, item->info.param1, MEVENT_COMMAND_FROM_MENU,
			menu);
	}
	
	//削除
	
	_destroy_menuwindow(p);
	
	return (is_change)? item: NULL;
}

/** メニューバーからの表示中、メニューバー上でポインタが移動して項目が移動する時 */

void __mMenuWindowMoveMenuBarItem(mMenuItem *item)
{
	mMenuWindow *p;

	p = MLK_MENUWINDOW(MLKAPP->run_current->window);

	p->flags |= MMENUWINDOW_F_MENUBAR_CHANGE;

	_end_popup(p, item);
}


//=============================
// sub - 操作
//=============================


/** スクロールあり時、アイテム部分の高さ取得 */

static int _get_scroll_area_height(mMenuWindow *p)
{
	return p->wg.h - _FRAME_Y * 2 - _SCROLL_H * 2;
}

/** 選択アイテム変更
 *
 * item が選択不可能なアイテムの場合は、解除扱いとなる。
 * 
 * item: 選択させるアイテム。NULL で、現在の選択を解除。 */

static void _set_selitem(mMenuWindow *p,mMenuItem *item,uint8_t flags)
{
	//選択不可能
	
	if(item && !__mMenuItemIsSelectable(item))
		item = NULL;

	//選択が変わらない

	if(p->item_sel == item) return;
	
	//サブメニュー用タイマー削除
	
	mWidgetTimerDelete(MLK_WIDGET(p->menuwin_top), _TIMERID_SUBMENU);
	
	//現在表示中のサブメニューウィンドウを削除
	
	if(p->menuwin_sub)
	{
		_destroy_menuwindow(p->menuwin_sub);
		p->menuwin_sub = NULL;
	}
	
	//選択変更
	
	p->item_sel = item;
	
	mWidgetRedraw(MLK_WIDGET(p));

	//新しいアイテムが選択された時

	if(item)
	{
		//項目が見えるようにスクロール

		if((flags & _SETSELITEM_F_SCROLL_TO)
			&& (p->flags & MMENUWINDOW_F_HAVE_SCROLL))
		{
			int y;

			y = _get_item_y(p, item) - _FRAME_Y - _SCROLL_H;
			
			if(y < 0)
				//上に隠れている場合
				p->scry += y;
			else if(y > _get_scroll_area_height(p) - item->info.height)
				p->scry += y - _get_scroll_area_height(p) + item->info.height;
		}

		//サブメニューがある場合
		
		if(__mMenuItemIsShowableSubmenu(item))
		{
			if(flags & _SETSELITEM_F_SUBMENU_DELAY)
			{
				//遅延後に表示
				mWidgetTimerAdd(MLK_WIDGET(p->menuwin_top),
					_TIMERID_SUBMENU, 200, (intptr_t)p);
			}
			else
				//すぐに表示
				_show_submenu(p);
		}
	}
}

/** アイテムの決定時
 *
 * - 選択不可能な場合、何もしない。
 * - ボタン押し or ホットキー入力 or 決定キー押し時 */

static void _enter_item(mMenuWindow *p,mMenuItem *item)
{
	if(item
		&& !item->info.submenu
		&& __mMenuItemIsSelectable(item))
	{
		//チェック処理
		
		if(_IS_ITEM_CHECKRADIO(item))
		{
			if(_IS_ITEMFLAG_ON(item, RADIO_TYPE))
				__mMenuItemSetCheckRadio(item);
			else
				item->info.flags ^= MMENUITEM_F_CHECKED;
		}
		
		//終了
		
		_end_popup(p, item);
	}
}

/** 一段階スクロール処理 */

static void _move_scroll(mMenuWindow *p)
{
	int scr = p->scry,n;
	
	if(p->flags & MMENUWINDOW_F_SCROLL_DOWN)
	{
		//下方向
		
		scr += 20;

		n = p->item_full_h - _get_scroll_area_height(p);
		if(scr > n) scr = n;
	}
	else
	{
		scr -= 20;
		if(scr < 0) scr = 0;
	}
	
	if(scr != p->scry)
	{
		p->scry = scr;
		mWidgetRedraw(MLK_WIDGET(p));
	}
}


//=============================
// キーボード関連
//=============================


/** メニューバー項目の左右移動 */

static void _move_menubar(mMenuWindow *p,int dir)
{
	mMenuItem *item,*item_sel;
	mMenu *menu;
	
	p = p->menuwin_top;

	if(!p->menubar) return;

	//メニューバーの選択アイテム

	item = item_sel = p->menubar->menubar.item_sel;
	menu = p->menubar->menubar.menu;

	//選択可能な次のアイテム取得

	do
	{
		if(dir < 0)
		{
			item = MLK_MENUITEM(item->i.prev);
			if(!item) item = mMenuGetBottomItem(menu);
		}
		else
		{
			item = MLK_MENUITEM(item->i.next);
			if(!item) item = mMenuGetTopItem(menu);
		}
	} while(!__mMenuItemIsShowableSubmenu(item));

	//アイテムが変化する場合、一度ポップアップを終了

	if(item != item_sel)
	{
		p->flags |= MMENUWINDOW_F_MENUBAR_CHANGE;
		
		_end_popup(p, item);
	}
}

/** 選択位置を上下に移動 */

static void _movesel_updown(mMenuWindow *p,int up)
{
	mMenuItem *item = p->item_sel;

	if(up)
		item = __mMenuGetPrevSelectableItem(p->menu, item);
	else
		item = __mMenuGetNextSelectableItem(p->menu, item);
	
	if(item)
		_set_selitem(p, item, _SETSELITEM_F_SCROLL_TO | _SETSELITEM_F_SUBMENU_DELAY);
}

/** キー押しイベント時 */

static void _event_keydown(mMenuWindow *p,uint32_t key)
{
	mMenuItem *item;
	mMenuWindow *sub;

	//ホットキーによるアイテム選択

	if(key >= 'a' && key <= 'z')
		key -= 0x20;
	
	if((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z'))
	{
		item = __mMenuFindItem_hotkey(p->menu, (char)key);
		if(item)
		{
			if(item->info.submenu)
				//サブメニューありならすぐ表示
				_set_selitem(p, item, _SETSELITEM_F_SCROLL_TO);
			else
				//決定
				_enter_item(p, item);
		}
		
		return;
	}
	
	//操作キー

	switch(key)
	{
		//SPACE/ENTER (アイテム決定)
		case MKEY_SPACE:
		case MKEY_ENTER:
		case MKEY_KP_SPACE:
		case MKEY_KP_ENTER:
			_enter_item(p, p->item_sel);
			break;

		//上
		case MKEY_UP:
		case MKEY_KP_UP:
			_movesel_updown(p, TRUE);
			break;
		//下
		case MKEY_DOWN:
		case MKEY_KP_DOWN:
			_movesel_updown(p, FALSE);
			break;

		//左
		// サブメニューあり: サブメニューはそのままで、親メニューに選択を移す
		// サブメニューなし: mMenuBar の場合、項目を移動
		// [!] ポップアップの表示位置が反転している場合、向きは逆になるが、キー対応はそのまま
		case MKEY_LEFT:
		case MKEY_KP_LEFT:
			if(p->menuwin_parent)
			{
				//サブメニューの場合、フォーカスを親へ移動
			
				(p->menuwin_top)->menuwin_focus = p->menuwin_parent;
				
				_set_selitem(p, NULL, 0);
			}
			else
				//メニューバーありの場合、一つ左へ
				_move_menubar(p, -1);
			break;

		//右
		// サブメニューあり: サブメニュー内の選択可能な先頭アイテムへ移動
		// サブメニューなし: mMenuBar の場合、項目移動
		case MKEY_RIGHT:
		case MKEY_KP_RIGHT:
			sub = p->menuwin_sub;
			if(!sub)
				_move_menubar(p, 1);
			else
			{
				//サブメニュー
				
				item = __mMenuGetSelectableTopItem(sub->menu);

				if(item)
				{
					(p->menuwin_top)->menuwin_focus = sub;
					
					_set_selitem(sub, item, _SETSELITEM_F_SCROLL_TO | _SETSELITEM_F_SUBMENU_DELAY);
				}
			}
			break;

		//HOME (先頭アイテムへ)
		case MKEY_HOME:
		case MKEY_KP_HOME:
			item = __mMenuGetSelectableTopItem(p->menu);
			_set_selitem(p, item, _SETSELITEM_F_SCROLL_TO | _SETSELITEM_F_SUBMENU_DELAY);
			break;
		//END (終端アイテムへ)
		case MKEY_END:
		case MKEY_KP_END:
			item = __mMenuGetSelectableBottomItem(p->menu);
			_set_selitem(p, item, _SETSELITEM_F_SCROLL_TO | _SETSELITEM_F_SUBMENU_DELAY);
			break;
	}
}


//=============================
// ポインタ関連
//=============================


/** ポインタ位置からアイテム取得 */

static mMenuItem *_get_item_point(mMenuWindow *p,int y)
{
	mMenuItem *pi;
	int top,bottom,itemh;
	
	top = _FRAME_Y;
	bottom = p->wg.h - _FRAME_Y;

	//スクロールあり: スクロールボタン上
	
	if(p->flags & MMENUWINDOW_F_HAVE_SCROLL)
	{
		top += _SCROLL_H - p->scry;
		bottom -= _SCROLL_H;

		if(y < _FRAME_Y + _SCROLL_H || y >= bottom)
			return NULL;
	}
	
	//
	
	for(pi = _MENUITEM_TOP(p); pi; pi = MLK_MENUITEM(pi->i.next))
	{
		if(top > y || top >= bottom) break;

		itemh = pi->info.height;
		
		if(y < top + itemh) return pi;
		
		top += itemh;
	}
	
	return NULL;
}

/** ポインタ位置がスクロール操作上にあるか
 *
 * return: [0] スクロール操作外 [1] スクロール操作可能 [-1] 上にはあるがスクロール不可 */

static int _is_point_scroll(mMenuWindow *p,int y)
{
	if(p->flags & MMENUWINDOW_F_HAVE_SCROLL)
	{
		if(y < _FRAME_Y + _SCROLL_H)
		{
			//上

			if(p->scry == 0) return -1;

			p->flags &= ~MMENUWINDOW_F_SCROLL_DOWN;
			return 1;
		}
		else if(y >= p->wg.h - _FRAME_Y - _SCROLL_H)
		{
			//下

			if(p->scry >= p->item_full_h - _get_scroll_area_height(p))
				return -1;

			p->flags |= MMENUWINDOW_F_SCROLL_DOWN;
			return 1;
		}
	}
	
	return 0;
}

/** スクロール解除 */

static void _release_scroll(mMenuWindow *p)
{
	p = p->menuwin_top;

	if(p->menuwin_scroll)
	{
		p->menuwin_scroll = NULL;
		
		mWidgetTimerDelete(MLK_WIDGET(p), _TIMERID_SCROLL);
		mWidgetUngrabPointer();
	}
}

/** ボタン押し時 */

static void _event_btt_press(mMenuWindow *p,mEventPointer *ev)
{
	int ret;

	if(ev->btt == MLK_BTT_LEFT && !p->menuwin_top->menuwin_scroll)
	{
		//スクロール操作判定
		
		ret = _is_point_scroll(p, ev->y);

		if(ret == 1)
		{
			//スクロール開始

			p->menuwin_top->menuwin_scroll = p;

			mWidgetTimerAdd(MLK_WIDGET(p->menuwin_top), _TIMERID_SCROLL, 60, 0);
			mWidgetGrabPointer(MLK_WIDGET(p));
		}
	}
}

/** 左ボタン離し時 */

static void _event_btt_release(mMenuWindow *p)
{
	if(p->menuwin_top->menuwin_scroll)
		//スクロール時は解除
		_release_scroll(p);
	else
		//項目決定
		_enter_item(p, p->item_sel);
}

/** ポインタ移動時 (ENTER 含む) */

static void _event_motion(mMenuWindow *p,int y)
{
	if(!p->menuwin_top->menuwin_scroll)
	{
		//ポインタ位置からアイテム選択

		p->menuwin_top->menuwin_focus = p;

		_set_selitem(p, _get_item_point(p, y), _SETSELITEM_F_SUBMENU_DELAY);
	}
}

/** イベントハンドラ */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mMenuWindow *p = MLK_MENUWINDOW(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
				_event_motion(p, ev->pt.y);
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
				_event_btt_press(p, (mEventPointer *)ev);
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_event_btt_release(p);
			}
			break;

		//ENTER
		case MEVENT_ENTER:
			_event_motion(p, ev->enter.y);
			break;

		//LEAVE
		// カーソルが離れた場合、選択を解除する。
		// (サブメニューが表示中の親は除く)
		case MEVENT_LEAVE:
			if(!p->menuwin_sub)
				_set_selitem(p, NULL, 0);
			break;
	
		//キー押し [top]
		case MEVENT_KEYDOWN:
			if(!ev->key.is_grab_pointer && p->menuwin_focus)
				_event_keydown(p->menuwin_focus, ev->key.key);
			break;

		//タイマー [top]
		case MEVENT_TIMER:
			if(ev->timer.id == _TIMERID_SUBMENU)
			{
				//サブメニュー表示遅延
				
				mWidgetTimerDelete(wg, _TIMERID_SUBMENU);
				
				_show_submenu(MLK_MENUWINDOW(ev->timer.param));
			}
			else if(ev->timer.id == _TIMERID_SCROLL)
			{
				//スクロール
				
				if(p->menuwin_top->menuwin_scroll)
					_move_scroll(p->menuwin_top->menuwin_scroll);
			}
			break;

		//フォーカス [top]
		case MEVENT_FOCUS:
			if(!ev->focus.is_out)
				_release_scroll(p);
			break;
	}

	return mPopupEventDefault(wg, ev);
}

/** 描画ハンドラ */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mMenuWindow *p = MLK_MENUWINDOW(wg);
	mFont *font;
	mMenuItem *pi;
	int w,h,y,itemh,fonth;
	uint32_t col,itemf;
	mlkbool have_scroll,is_sel,is_disable;
	mMenuItemDraw drawi;
	
	font = mWidgetGetFont(wg);
	fonth = mFontGetHeight(font);
	w = wg->w;
	h = wg->h;
	
	have_scroll = ((p->flags & MMENUWINDOW_F_HAVE_SCROLL) != 0);

	//枠
	
	mPixbufBox(pixbuf, 0, 0, w, h, MGUICOL_PIX(MENU_FRAME));
	
	//背景
	
	mPixbufFillBox(pixbuf, 1, 1, w - 2, h - 2, MGUICOL_PIX(MENU_FACE));
	
	//スクロール矢印

	if(have_scroll)
	{
		mPixbufDrawArrowUp(pixbuf,
			(w - 9) >> 1, _FRAME_Y + (_SCROLL_H - 5) / 2, 5,
			(p->scry == 0)? MGUICOL_PIX(MENU_TEXT_DISABLE): MGUICOL_PIX(MENU_TEXT));

		mPixbufDrawArrowDown(pixbuf,
			(w - 9) >> 1, h - _FRAME_Y - _SCROLL_H + (_SCROLL_H - 5) / 2, 5,
			(p->scry >= p->item_full_h - (h - _FRAME_Y * 2 - _SCROLL_H * 2))
				? MGUICOL_PIX(MENU_TEXT_DISABLE): MGUICOL_PIX(MENU_TEXT));
	}

	//--------- メニュー項目
	
	y = _FRAME_Y;

	//スクロール時

	if(have_scroll)
	{
		y += _SCROLL_H - p->scry;
		
		//クリッピング

		mPixbufClip_setBox_d(pixbuf,
			0, _FRAME_Y + _SCROLL_H, w, h - _FRAME_Y * 2 - _SCROLL_H * 2);
	}

	//各アイテム
	
	for(pi = _MENUITEM_TOP(p); pi;
			y += pi->info.height, pi = MLK_MENUITEM(pi->i.next))
	{
		itemh = pi->info.height;
	
		if(have_scroll)
		{
			if(y + itemh <= _FRAME_Y + _SCROLL_H) continue;
			if(y >= h - _FRAME_Y - _SCROLL_H) break;
		}

		//
	
		is_sel = (pi == p->item_sel);
		itemf = pi->info.flags;

		//
	
		if(itemf & MMENUITEM_F_SEP)
		{
			//セパレータ
			
			mPixbufLineH(pixbuf,
				_FRAME_X + 1, y + itemh / 2,
				w - _FRAME_X * 2 - 2, MGUICOL_PIX(MENU_SEP));
		}
		else if(pi->info.draw)
		{
			//独自描画

			drawi.flags = 0;
			if(is_sel) drawi.flags |= MMENUITEM_DRAW_F_SELECTED;
			
			drawi.box.x = _FRAME_X;
			drawi.box.y = y;
			drawi.box.w = w - _FRAME_X * 2;
			drawi.box.h = itemh;
			
			(pi->info.draw)(pixbuf, &pi->info, &drawi);
		}
		else if(pi->info.text)
		{
			//------- 通常、ラベルあり

			is_disable = ((itemf & MMENUITEM_F_DISABLE) != 0);
		
			//選択カーソル
			
			if(is_sel && !is_disable)
			{
				mPixbufFillBox(pixbuf,
					_FRAME_X, y, w - _FRAME_X * 2, itemh,
					MGUICOL_PIX(MENU_SELECT));
			}
			
			//テキスト色 (pix)

			col = (is_disable)? MGUICOL_PIX(MENU_TEXT_DISABLE): MGUICOL_PIX(MENU_TEXT);
			
			//チェック
		
			if(itemf & MMENUITEM_F_CHECKED)
			{
				if(itemf & MMENUITEM_F_RADIO_TYPE)
				{
					mPixbufDrawMenuRadio(pixbuf,
						_FRAME_X + 6, y + (itemh - 5) / 2, col);
				}
				else
				{
					mPixbufDrawMenuChecked(pixbuf,
						_FRAME_X + 5, y + (itemh - 7) / 2, col);
				}
			}

			//サブメニュー矢印
			
			if(pi->info.submenu)
			{
				mPixbufDrawArrowRight(pixbuf,
					w - _FRAME_X - 11, y + (itemh - 7) / 2, 5, col);
			}

			//ラベル
			
			mFontDrawText_pixbuf_hotkey(font, pixbuf,
				_FRAME_X + _LEFT_SPACE,
				y + ((itemh - fonth) >> 1),
				pi->info.text, -1,
				(is_disable)? MGUICOL_RGB(MENU_TEXT_DISABLE): MGUICOL_RGB(MENU_TEXT));
			
			//ショートカットキー文字列
			
			if(pi->sckeytxt)
			{
				mFontDrawText_pixbuf(font, pixbuf,
					w - _FRAME_X - _RIGHT_SPACE - pi->sckey_w,
					y + ((itemh - fonth) >> 1),
					pi->sckeytxt, -1,
					(is_sel && !is_disable)? MGUICOL_RGB(MENU_TEXT): MGUICOL_RGB(MENU_TEXT_SHORTCUT));
			}
		}
	}
}
