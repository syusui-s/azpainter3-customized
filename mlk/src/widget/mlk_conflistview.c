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
 * mConfListView
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_conflistview.h"
#include "mlk_listheader.h"
#include "mlk_listviewpage.h"
#include "mlk_columnitem.h"
#include "mlk_event.h"
#include "mlk_key.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_string.h"
#include "mlk_pixbuf.h"
#include "mlk_accelerator.h"
#include "mlk_fontinfo.h"

#include "mlk_lineedit.h"
#include "mlk_checkbutton.h"
#include "mlk_menu.h"
#include "mlk_inputaccel.h"

#include "mlk_pv_event.h"


//-------------------

//icon : 値のタイプ (-1 でヘッダ)
//param : 値を判断するためのパラメータ

//数値
typedef struct
{
	mColumnItem i;
	int val,min,max,dig;
}_item_num;

//チェック
typedef struct
{
	mColumnItem i;
	int checked;
}_item_check;

//選択
typedef struct
{
	mColumnItem i;
	char *list;
	int sel,fcopy;
}_item_select;

//色
typedef struct
{
	mColumnItem i;
	uint32_t col;
}_item_color;

//キー
typedef struct
{
	mColumnItem i;
	uint32_t key;
}_item_accel;

//フォント
typedef struct
{
	mColumnItem i;
	uint32_t flags; //ダイアログフラグ
	mFontInfo info;
}_item_font;

//ファイル
typedef struct
{
	mColumnItem i;
	const char *filter;
}_item_file;

//-------------------


//=============================
// sub
//=============================


/* チェックのアイテム描画 */

static int _drawitem_check(mPixbuf *pixbuf,mColumnItem *item,mColumnItemDrawInfo *info)
{
	if(info->column != 1)
		return 1;
	else
	{
		mPixbufDrawCheckBox(pixbuf, info->box.x,
			info->box.y + (info->box.h - mCheckButtonGetCheckBoxSize()) / 2,
			(((_item_check *)item)->checked)? MPIXBUF_DRAWCKBOX_CHECKED: 0);

		return 0;
	}
}

/* カラーのアイテム描画 */

static int _drawitem_color(mPixbuf *pixbuf,mColumnItem *item,mColumnItemDrawInfo *info)
{
	if(info->column != 1)
		return 1;
	else
	{
		mBox box = info->box;
		int w;

		w = box.w;
		if(w > 60) w = 60;
	
		mPixbufBox(pixbuf, box.x, box.y + 1, w, box.h - 2, 0);

		mPixbufFillBox(pixbuf, box.x + 1, box.y + 2, w - 2, box.h - 4,
			mRGBtoPix( ((_item_color *)item)->col ));

		return 0;
	}
}

/* 通知 & ページを再描画 */

static void _notify(mConfListView *p,int type,intptr_t param1,intptr_t param2)
{
	mWidgetEventAdd_notify(MLK_WIDGET(p), MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW,
		type, param1, param2);

	mWidgetRedraw(MLK_WIDGET(p->sv.page));
}

/* 入力ウィジェット削除 */

static void _delete_inputwg(mConfListView *p)
{
	if(p->clv.editwg)
	{
		//ウィジェット削除
		
		mWidgetDestroy(p->clv.editwg);

		p->clv.editwg = NULL;
		p->clv.item = NULL;

		//スクロールバーを有効化

		mScrollViewEnableScrollBar(MLK_SCROLLVIEW(p), 1);
	}
}

/* 入力を終了 */

static void _end_input(mConfListView *p)
{
	mColumnItem *item;
	mWidget *wg;
	mStr str = MSTR_INIT;

	wg = p->clv.editwg;
	item = p->clv.item;

	//値を取得

	switch(item->icon)
	{
		//テキスト
		case MCONFLISTVIEW_VALTYPE_TEXT:
			mLineEditGetTextStr(MLK_LINEEDIT(wg), &str);
			mColumnItem_setColText(item, 1, str.buf);
			break;
		//数値
		case MCONFLISTVIEW_VALTYPE_NUM:
			{
			char m[32];
			
			((_item_num *)item)->val = mLineEditGetNum(MLK_LINEEDIT(wg));

			mIntToStr_float(m, ((_item_num *)item)->val, ((_item_num *)item)->dig);
			mColumnItem_setColText(item, 1, m);
			}
			break;
		//キー
		case MCONFLISTVIEW_VALTYPE_ACCEL:
			((_item_accel *)item)->key = mInputAccelGetKey(MLK_INPUTACCEL(wg));

			mAcceleratorGetKeyText(&str, ((_item_accel *)item)->key);
			mColumnItem_setColText(item, 1, str.buf);
			break;
	}

	mStrFree(&str);

	//ウィジェット削除

	_delete_inputwg(p);

	//フォーカス

	mWidgetSetFocus(MLK_WIDGET(p));

	//通知&再描画

	_notify(p, MCONFLISTVIEW_N_CHANGE_VAL, (intptr_t)item, item->param);
}

/* 入力ウィジェットから、入力終了 */

static void _inputwg_end(mWidget *wg)
{
	//新規ウィジェットと区別するため、
	//削除対象の場合、param2 を 1 にする

	wg->param2 = 1;
	
	mWidgetEventAdd_notify(wg, NULL, -1, 0, 0);
}

/* 入力ウィジェットの共通ハンドラ */

static int _inputwg_event(mWidget *wg,mEvent *ev)
{
	//フォーカスアウトで削除

	if(ev->type == MEVENT_FOCUS && ev->focus.is_out)
	{
		_inputwg_end(wg);
		return 1;
	}

	//元のハンドラ

	if(wg->param1)
		return ((mFuncWidgetHandle_event)wg->param1)(wg, ev);
	else
		return 0;
}

/* 選択のメニュー実行 */

static void _run_select(mConfListView *p,mWidget *parent,_item_select *pi)
{
	mMenu *menu;
	mMenuItem *mi;
	mListHeader *lh;
	const char *top,*end;
	mBox box;
	int no;

	if(!pi->list) return;

	menu = mMenuNew();

	//追加

	top = pi->list;
	no = 0;

	while(mStringGetNextSplit(&top, &end, '\t'))
	{
		mMenuAppendText_copy(menu, no++, top, end - top);

		top = end;
	}

	//選択

	mMenuSetItemCheck(menu, pi->sel, 1);

	//実行

	lh = mListViewGetHeaderWidget(MLK_LISTVIEW(p));

	box.x = mListHeaderGetItemX(lh, 1);
	box.y = mListViewPage_getItemY(MLK_LISTVIEWPAGE(parent), MLK_COLUMNITEM(pi));
	box.w = mListHeaderGetItemWidth(lh, 1);
	box.h = p->lv.item_height;

	mi = mMenuPopup(menu, parent, 0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_FLIP_XY, NULL);

	//セット

	if(mi)
	{
		pi->sel = mMenuItemGetID(mi);

		mColumnItem_setColText(MLK_COLUMNITEM(pi), 1, mMenuItemGetText(mi));
		
		_notify(p, MCONFLISTVIEW_N_CHANGE_VAL, (intptr_t)pi, pi->i.param);
	}

	mMenuDestroy(menu);
}

/* フォント選択 */

static void _select_font(mConfListView *p,_item_font *pi)
{
	if(mSysDlg_selectFont(p->wg.toplevel, pi->flags, &pi->info))
	{
		mStr str = MSTR_INIT;

		//値のテキスト

		mFontInfoGetText_family_size(&str, &pi->info);
		mColumnItem_setColText(MLK_COLUMNITEM(pi), 1, str.buf);

		mStrFree(&str);

		//通知
		_notify(p, MCONFLISTVIEW_N_CHANGE_VAL, (intptr_t)pi, pi->i.param);
	}
}

/* ファイル選択 */

static void _select_file(mConfListView *p,_item_file *pi)
{
	mStr str = MSTR_INIT;

	mListViewGetItemColumnText(MLK_LISTVIEW(p), MLK_COLUMNITEM(pi), 1, &str);

	if(mSysDlg_openFile(p->wg.toplevel, ((_item_file *)pi)->filter, 0, NULL, 0, &str))
	{
		mColumnItem_setColText(MLK_COLUMNITEM(pi), 1, str.buf);
	
		_notify(p, MCONFLISTVIEW_N_CHANGE_VAL, (intptr_t)pi, pi->i.param);
	}

	mStrFree(&str);
}

/* 値の入力開始
 *
 * item: NULL で現在のフォーカス */

static void _start_input(mConfListView *p,mColumnItem *item)
{
	mWidget *wg,*parent;
	mListHeader *lh;
	char *pc;
	int itemh;

	//入力状態の時は終了だけ行う
	//(入力状態で mConfListView の2番目の列がクリックされた時)

	if(p->clv.editwg)
	{
		_end_input(p);
		return;
	}

	//

	if(!item) item = mListViewGetFocusItem(MLK_LISTVIEW(p));
	if(!item) return;

	if(item->flags & MCOLUMNITEM_F_HEADER) return;

	//入力ウィジェット作成 or 値変更処理

	wg = NULL;
	parent = MLK_WIDGET(p->sv.page);

	switch(item->icon)
	{
		//テキスト入力
		case MCONFLISTVIEW_VALTYPE_TEXT:
			wg = (mWidget *)mLineEditNew(parent, 0, MLINEEDIT_S_NOTIFY_ENTER);

			mColumnItem_getColText(item, 1, &pc);

			mLineEditSetText(MLK_LINEEDIT(wg), pc);
			break;
		//数値入力
		case MCONFLISTVIEW_VALTYPE_NUM:
			{
			_item_num *pi = (_item_num *)item;
			
			wg = (mWidget *)mLineEditNew(parent, 0, MLINEEDIT_S_NOTIFY_ENTER | MLINEEDIT_S_SPIN);

			mLineEditSetNumStatus(MLK_LINEEDIT(wg), pi->min, pi->max, pi->dig);
			mLineEditSetNum(MLK_LINEEDIT(wg), pi->val);
			}
			break;
		//チェック
		case MCONFLISTVIEW_VALTYPE_CHECK:
			((_item_check *)item)->checked ^= 1;

			//通知
			_notify(p, MCONFLISTVIEW_N_CHANGE_VAL, (intptr_t)item, item->param);
			break;
		//選択
		case MCONFLISTVIEW_VALTYPE_SELECT:
			_run_select(p, parent, (_item_select *)item);
			break;
		//色
		case MCONFLISTVIEW_VALTYPE_COLOR:
			if(mSysDlg_selectColor(p->wg.toplevel, &((_item_color *)item)->col))
			{
				//通知
				_notify(p, MCONFLISTVIEW_N_CHANGE_VAL, (intptr_t)item, item->param);
			}
			break;
		//キー
		case MCONFLISTVIEW_VALTYPE_ACCEL:
			wg = (mWidget *)mInputAccelNew(parent, 0, MINPUTACCEL_S_NOTIFY_ENTER);

			mInputAccelSetKey(MLK_INPUTACCEL(wg), ((_item_accel *)item)->key);
			break;
		//フォント
		case MCONFLISTVIEW_VALTYPE_FONT:
			_select_font(p, (_item_font *)item);
			break;
		//ファイル
		case MCONFLISTVIEW_VALTYPE_FILE:
			_select_file(p, (_item_file *)item);
			break;
	}

	p->clv.editwg = wg;

	if(!wg) return;

	//----------

	p->clv.item = item;

	//ウィジェット設定

	lh = mListViewGetHeaderWidget(MLK_LISTVIEW(p));
	itemh = mListViewGetItemHeight(MLK_LISTVIEW(p));

	wg->notify_to = MWIDGET_NOTIFYTO_PARENT2; //通知は mConfListView へ
	wg->param1 = (intptr_t)wg->event;
	wg->event = _inputwg_event;

	(wg->calc_hint)(wg);

	mWidgetMoveResize(wg,
		mListHeaderGetItemX(lh, 1),
		mListViewPage_getItemY(MLK_LISTVIEWPAGE(parent), item) + (itemh - wg->hintH) / 2,
		mListHeaderGetItemWidth(lh, 1), wg->hintH);

	mWidgetSetFocus(wg);
	mWidgetRedraw(parent);

	//ウィジェット上にカーソルがあれば ENTER 処理
	//(カーソルを動かさないと ENTER が来ないため、
	// 入力ウィジェット上でホイール操作が行えない)

	__mEventReEnter();

	//スクロールバーを無効化

	mScrollViewEnableScrollBar(MLK_SCROLLVIEW(p), 0);
}

/* mListViewPage イベントハンドラ */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	mConfListView *p = MLK_CONFLISTVIEW(wg->parent);

	switch(ev->type)
	{
		//キー (SPACE で入力開始)
		case MEVENT_KEYDOWN:
			if(ev->key.key == MKEY_SPACE || ev->key.key == MKEY_KP_SPACE)
				_start_input(p, NULL);
			break;
	
		//入力中はホイール無効
		case MEVENT_SCROLL:
			if(p->clv.editwg) return 1;
			break;
	}

	return mListViewPageHandle_event(wg, ev);
}


//=============================
// main
//=============================


/* アイテム破棄ハンドラ */

static void _item_destroy(mList *list,mListItem *item)
{
	mColumnItem *pi = MLK_COLUMNITEM(item);

	if(pi->icon == MCONFLISTVIEW_VALTYPE_SELECT)
	{
		//(選択) リスト文字列解放

		if(((_item_select *)pi)->fcopy)
			mFree(((_item_select *)pi)->list);
	}
	else if(pi->icon == MCONFLISTVIEW_VALTYPE_FONT)
	{
		//フォント

		mFontInfoFree(&((_item_font *)pi)->info);
	}

	//mColumnItem

	mColumnItemHandle_destroyItem(list, item);
}

/**@ mConfListView データ解放 */

void mConfListViewDestroy(mWidget *wg)
{
	//リストビュー

	mListViewDestroy(wg);
}

/**@ 作成
 *
 * @d:レイアウトフラグは、デフォルトで EXPAND_W/H。\
 *  アイテム追加後、カラム幅をセットすること。 */

mConfListView *mConfListViewNew(mWidget *parent,int size,uint32_t fstyle)
{
	mConfListView *p;
	mListHeader *header;

	//mListView
	
	if(size < sizeof(mConfListView))
		size = sizeof(mConfListView);
	
	p = (mConfListView *)mListViewNew(parent, size,
			MLISTVIEW_S_MULTI_COLUMN | MLISTVIEW_S_HAVE_HEADER | MLISTVIEW_S_GRID_COL | MLISTVIEW_S_GRID_ROW,
			MSCROLLVIEW_S_HORZVERT_FRAME);

	if(!p) return NULL;

	p->wg.destroy = mConfListViewDestroy;
	p->wg.event = mConfListViewHandle_event;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.notify_to_rep = MWIDGET_NOTIFYTOREP_SELF; //通知は自身で受ける

	mListViewSetItemHeight_min(MLK_LISTVIEW(p), mCheckButtonGetCheckBoxSize());

	//リストアイテム破棄ハンドラ

	(p->lv.manager).list.item_destroy = _item_destroy;

	//mListViewPage

	(p->sv.page)->wg.event = _page_event_handle;

	//mListHeader アイテム

	header = mListViewGetHeaderWidget(MLK_LISTVIEW(p));

	mListHeaderAddItem(header, "name", 20, 0, 0);
	mListHeaderAddItem(header, "value", 10, MLISTHEADER_ITEM_F_EXPAND, 0);

	return p;
}

/**@ 1番目(名前)のカラム幅を、テキスト幅からセット
 *
 * @d:レイアウト済みの場合は、値のカラム幅を現在のウィジェット幅から再セット。 */

void mConfListView_setColumnWidth_name(mConfListView *p)
{
	mListViewSetColumnWidth_auto(MLK_LISTVIEW(p), 0);

	if(p->wg.fui & MWIDGET_UI_LAYOUTED)
		mListHeaderSetExpandItemWidth(mListViewGetHeaderWidget(MLK_LISTVIEW(p)));
}

/**@ ヘッダアイテム追加 */

void mConfListView_addItem_header(mConfListView *p,const char *text)
{
	mListViewAddItem(MLK_LISTVIEW(p), text, -1, MCOLUMNITEM_F_HEADER, 0);
}

/**@ アイテム追加 (値=テキスト) */

void mConfListView_addItem_text(mConfListView *p,const char *name,const char *val,intptr_t param)
{
	mStr str = MSTR_INIT;

	mStrSetFormat(&str, "%s\t%s", name, val);

	mListViewAddItem(MLK_LISTVIEW(p), str.buf,
		MCONFLISTVIEW_VALTYPE_TEXT, 0, param);

	mStrFree(&str);
}

/**@ アイテム追加 (値=数値)
 *
 * @p:dig 小数点以下桁数 */

void mConfListView_addItem_num(mConfListView *p,
	const char *name,int val,int min,int max,int dig,intptr_t param)
{
	mStr str = MSTR_INIT;
	_item_num *pi;
	char m[32];

	mIntToStr_float(m, val, dig);
	mStrSetFormat(&str, "%s\t%s", name, m);

	pi = (_item_num *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_num), str.buf, MCONFLISTVIEW_VALTYPE_NUM, 0, param);

	if(pi)
	{
		pi->val = val;
		pi->min = min;
		pi->max = max;
		pi->dig = dig;
	}

	mStrFree(&str);
}

/**@ アイテム追加 (値=ON/OFF) */

void mConfListView_addItem_check(mConfListView *p,const char *name,int checked,intptr_t param)
{
	_item_check *pi;

	pi = (_item_check *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_check), name, MCONFLISTVIEW_VALTYPE_CHECK, 0, param);

	if(pi)
	{
		pi->checked = (checked != 0);
		pi->i.draw = _drawitem_check;
	}
}

/**@ アイテム追加 (値=選択)
 *
 * @p:def 初期選択
 * @p:list 選択リストの文字列。'\\t' で区切る。間が空文字列の場合は無視される。
 * @p:copy TRUE でリスト文字列を複製する。FALSE の場合は、ポインタがそのまま使われる。 */

void mConfListView_addItem_select(mConfListView *p,const char *name,int def,const char *list,mlkbool copy,intptr_t param)
{
	_item_select *pi;
	char *pc;
	int len;
	mStr str = MSTR_INIT;

	mStrSetFormat(&str, "%s\t", name);

	len = mStringGetSplitText_atIndex(list, '\t', def, &pc);
	if(len > 0)
		mStrAppendText_len(&str, pc, len);

	//

	pi = (_item_select *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_select), str.buf, MCONFLISTVIEW_VALTYPE_SELECT, 0, param);

	mStrFree(&str);

	if(pi)
	{
		pi->sel = def;

		if(copy)
		{
			pi->list = mStrdup(list);
			pi->fcopy = 1;
		}
		else
			pi->list = (char *)list;
	}
}

/**@ アイテム追加 (値=色) */

void mConfListView_addItem_color(mConfListView *p,const char *name,mRgbCol col,intptr_t param)
{
	_item_color *pi;

	pi = (_item_color *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_color), name, MCONFLISTVIEW_VALTYPE_COLOR, 0, param);

	if(pi)
	{
		pi->col = col;
		pi->i.draw = _drawitem_color;
	}
}

/**@ アイテム追加 (値=アクセラレータキー)
 *
 * @d:ENTER キーは、キー入力でセットできない (確定用として使われるため)。 */

void mConfListView_addItem_accel(mConfListView *p,const char *name,uint32_t key,intptr_t param)
{
	_item_accel *pi;
	mStr str = MSTR_INIT;

	mAcceleratorGetKeyText(&str, key);
	mStrPrependText(&str, "\t");
	mStrPrependText(&str, name);

	pi = (_item_accel *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_accel), str.buf, MCONFLISTVIEW_VALTYPE_ACCEL, 0, param);

	mStrFree(&str);

	if(pi)
		pi->key = key;
}

/**@ アイテム追加 (値=フォント情報)
 *
 * @p:infotext フォント情報の文字列フォーマット
 * @p:flags フォント選択ダイアログのフラグ */

void mConfListView_addItem_font(mConfListView *p,const char *name,const char *infotext,uint32_t flags,intptr_t param)
{
	_item_font *pi;
	mFontInfo info;
	mStr str = MSTR_INIT;

	mFontInfoInit(&info);
	mFontInfoSetFromText(&info, infotext);

	mFontInfoGetText_family_size(&str, &info);
	mStrPrependText(&str, "\t");
	mStrPrependText(&str, name);

	pi = (_item_font *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_font), str.buf, MCONFLISTVIEW_VALTYPE_FONT, 0, param);

	mStrFree(&str);

	if(pi)
	{
		pi->info = info;
		pi->flags = flags;
	}
}

/**@ アイテム追加 (値=ファイルパス)
 *
 * @p:filter 開くダイアログ時のフィルタ文字列 (静的文字列) */

void mConfListView_addItem_file(mConfListView *p,const char *name,const char *path,const char *filter,intptr_t param)
{
	_item_file *pi;
	mStr str = MSTR_INIT;

	mStrSetFormat(&str, "%s\t%s", name, path);

	pi = (_item_file *)mListViewAddItem_size(MLK_LISTVIEW(p),
		sizeof(_item_file), str.buf, MCONFLISTVIEW_VALTYPE_FILE, 0, param);

	mStrFree(&str);

	if(pi)
		pi->filter = filter;
}

/**@ アイテムに値をセット
 *
 * @p:item  NULL でフォーカスアイテム */

void mConfListView_setItemValue(mConfListView *p,mColumnItem *item,mConfListViewValue *val)
{
	mStr str = MSTR_INIT;
	char *pc;
	int n;

	if(!item)
	{
		item = mListViewGetFocusItem(MLK_LISTVIEW(p));
		if(!item) return;
	}

	if(item->icon < 0) return;

	switch(item->icon)
	{
		//テキスト、ファイル
		case MCONFLISTVIEW_VALTYPE_TEXT:
		case MCONFLISTVIEW_VALTYPE_FILE:
			mStrSetText(&str, val->text);
			break;
		//数値
		case MCONFLISTVIEW_VALTYPE_NUM:
			{
			_item_num *pi = (_item_num *)item;
			
			n = val->num;

			if(n < pi->min)
				n = pi->min;
			else if(n > pi->max)
				n = pi->max;
		
			pi->val = n;

			mStrSetIntDig(&str, n, pi->dig);
			}
			break;
		//チェック
		case MCONFLISTVIEW_VALTYPE_CHECK:
			((_item_check *)item)->checked = val->checked;
			break;
		//選択
		case MCONFLISTVIEW_VALTYPE_SELECT:
			((_item_select *)item)->sel = val->select;

			n = mStringGetSplitText_atIndex(((_item_select *)item)->list, '\t', val->select, &pc);
			if(n > 0)
				mStrSetText_len(&str, pc, n);
			break;
		//色
		case MCONFLISTVIEW_VALTYPE_COLOR:
			((_item_color *)item)->col = val->color;
			break;
		//キー
		case MCONFLISTVIEW_VALTYPE_ACCEL:
			((_item_accel *)item)->key = val->key;

			mAcceleratorGetKeyText(&str, val->key);
			break;
		//フォント
		case MCONFLISTVIEW_VALTYPE_FONT:
			mFontInfoSetFromText(&((_item_font *)item)->info, val->text);

			mFontInfoGetText_family_size(&str, &((_item_font *)item)->info);
			break;
	}

	mListViewSetItemColumnText(MLK_LISTVIEW(p), item, 1, str.buf);

	mStrFree(&str);
}

/**@ 指定位置のアイテムの値をセット
 *
 * @p:index アイテムのインデックス位置 (負の値でフォーカス) */

void mConfListView_setItemValue_atIndex(mConfListView *p,int index,mConfListViewValue *val)
{
	mColumnItem *item;

	item = mListViewGetItem_atIndex(MLK_LISTVIEW(p), index);

	if(item)
		mConfListView_setItemValue(p, item, val);
}

/**@ すべてのアイテムの値を取得
 *
 * @p:func コールバック関数。戻り値で 0 以外を返すと、中断させる。\
 *  type は値のタイプ (MCONFLISTVIEW_VALTYPE_*)。\
 *  item_param は、アイテム追加時に指定したパラメータ値。\
 *  val に値が入る。
 * @p:param 関数に渡すパラメータ値
 * @r:すべてのアイテムを処理したか。FALSE で、途中で中断した。 */

mlkbool mConfListView_getValues(mConfListView *p,mFuncConfListView_getValue func,void *param)
{
	mColumnItem *pi;
	mConfListViewValue val;
	mStr str = MSTR_INIT;
	char *pc;
	int len,ret;

	ret = 0;
	pi = mListViewGetTopItem(MLK_LISTVIEW(p));

	for(; pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		//ヘッダは除外
		if(pi->icon == -1) continue;
	
		switch(pi->icon)
		{
			//テキスト、ファイル
			case MCONFLISTVIEW_VALTYPE_TEXT:
			case MCONFLISTVIEW_VALTYPE_FILE:
				len = mColumnItem_getColText(pi, 1, &pc);
				if(!len)
					val.text = NULL;
				else
				{
					mStrSetText_len(&str, pc, len);
					val.text = str.buf;
				}
				break;
			//数値
			case MCONFLISTVIEW_VALTYPE_NUM:
				val.num = ((_item_num *)pi)->val;
				break;
			//チェック
			case MCONFLISTVIEW_VALTYPE_CHECK:
				val.checked = ((_item_check *)pi)->checked;
				break;
			//選択
			case MCONFLISTVIEW_VALTYPE_SELECT:
				val.select = ((_item_select *)pi)->sel;
				break;
			//色
			case MCONFLISTVIEW_VALTYPE_COLOR:
				val.color = ((_item_color *)pi)->col;
				break;
			//キー
			case MCONFLISTVIEW_VALTYPE_ACCEL:
				val.key = ((_item_accel *)pi)->key;
				break;
			//フォント
			case MCONFLISTVIEW_VALTYPE_FONT:
				mFontInfoToText(&str, &((_item_font *)pi)->info);
				val.text = str.buf;
				break;
		}

		ret = (func)(pi->icon, pi->param, &val, param);
		if(ret) break;
	}

	mStrFree(&str);

	return (ret == 0);
}

/**@ フォーカスアイテムの値を 0 にする */

void mConfListView_setFocusValue_zero(mConfListView *p)
{
	mConfListViewValue val;

	mMemset0(&val, sizeof(mConfListViewValue));
	
	mConfListView_setItemValue_atIndex(p, -1, &val);
}

/**@ すべてのアクセラレータキーの値を 0 にする */

void mConfListView_clearAll_accel(mConfListView *p)
{
	mColumnItem *pi;

	pi = mListViewGetTopItem(MLK_LISTVIEW(p));

	for(; pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		if(pi->icon == MCONFLISTVIEW_VALTYPE_ACCEL)
		{
			((_item_accel *)pi)->key = 0;

			mListViewSetItemColumnText(MLK_LISTVIEW(p), pi, 1, NULL);
		}
	}
}


/**@ アイテムから、パラメータ値を取得 */

intptr_t mConfListViewItem_getParam(mColumnItem *pi)
{
	return pi->param;
}

/**@ アイテムから、アクセラレータキーの値を取得
 *
 * @r:タイプがアクセラレータキーではない場合、0 */

uint32_t mConfListViewItem_getAccelKey(mColumnItem *pi)
{
	if(pi->icon == MCONFLISTVIEW_VALTYPE_ACCEL)
		return ((_item_accel *)pi)->key;
	else
		return 0;
}


//==============================
// ハンドラ
//==============================


/* 入力ウィジェットの通知 */

static void _notify_editwg(mConfListView *p,mWidget *wg,mEventNotify *ev)
{
	switch((p->clv.item)->icon)
	{
		//mLineEdit
		case MCONFLISTVIEW_VALTYPE_TEXT:
		case MCONFLISTVIEW_VALTYPE_NUM:
			if(ev->notify_type == MLINEEDIT_N_ENTER)
				_inputwg_end(wg);
			break;
		//mInputAccel
		case MCONFLISTVIEW_VALTYPE_ACCEL:
			if(ev->notify_type == MINPUTACCEL_N_ENTER)
				_inputwg_end(wg);
			break;
	}
}

/**@ event ハンドラ関数 */

int mConfListViewHandle_event(mWidget *wg,mEvent *ev)
{
	mConfListView *p = MLK_CONFLISTVIEW(wg);

	if(ev->type == MEVENT_NOTIFY)
	{
		if(ev->notify.widget_from == wg)
		{
			//---- mListView

			switch(ev->notify.notify_type)
			{
				//2番目の列が左クリックされた時
				case MLISTVIEW_N_CHANGE_FOCUS:
				case MLISTVIEW_N_CLICK_ON_FOCUS:
				case MLISTVIEW_N_ITEM_L_DBLCLK:
					if(ev->notify.param2 == 1)
						_start_input(p, (mColumnItem *)ev->notify.param1);
					break;
			}
		
			return TRUE;
		}
		else if(ev->notify.widget_from == p->clv.editwg)
		{
			//---- 入力ウィジェット

			//入力状態で mConfListView の2番目の列をクリックした時は、
			//"[editwg] FOCUSOUT -> clv に終了通知追加 -> clv の通知 -> _start_input
			// -> 新しい入力状態 -> 終了通知"、という順で実行される。
			//editwg の値が変化していれば、widget_from != editwg となるので、通知は無効化される。
			//仮に、前回の editwg と新しい editwg でポインタが同じになった場合でも、
			//param2 の値で、新しい入力ウィジェットか、削除対象のウィジェットかを判定できる。
			//これにより、入力状態でリストをクリックした時は、_start_input で終了処理だけ行い、
			//後に来る終了通知を無効化できる。

			if(ev->notify.notify_type == -1)
			{
				//終了

				if((p->clv.editwg)->param2)
					_end_input(p);
			}
			else
				//入力ウィジェットの通知
				_notify_editwg(p, ev->notify.widget_from, (mEventNotify *)ev);

			return TRUE;
		}
	}

	return mListViewHandle_event(wg, ev);
}

