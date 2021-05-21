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
 * mComboBox
 *****************************************/

#include <string.h>  //strlen

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_combobox.h"
#include "mlk_listviewpage.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_string.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_key.h"

#include "mlk_pv_widget.h"
#include "mlk_columnitem_manager.h"


//-----------------

#define _FRAME_W      1   //枠太さ
#define _PADDING_X    5   //X余白 (枠は含まない)
#define _PADDING_Y    3   //Y余白 (枠は含まない)
#define _ARROW_W      9   //矢印の幅 + 余白

//-----------------

/*-------------

item_height:
	アイテムの高さ (余白含まない)。0 以下でフォントの高さ。

---------------*/



//======================
// sub
//======================


/** アイテム高さ取得 */

static int _get_item_height(mComboBox *p)
{
	if(p->cb.item_height <= 0)
		return mWidgetGetFontHeight(MLK_WIDGET(p));
	else
		return p->cb.item_height;
}

/** 通知＆再描画 */

static void _notify_update(mComboBox *p)
{
	mColumnItem *item = p->cb.manager.item_focus;

	mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
		MCOMBOBOX_N_CHANGE_SEL,
		(intptr_t)item, item->param);

	mWidgetRedraw(MLK_WIDGET(p));
}

/** 選択を上下に移動 */

static void _select_updown(mComboBox *p,mlkbool down)
{
	if(mCIManagerFocusItem_updown(&p->cb.manager, down))
		_notify_update(p);
}

/** アイテム追加 */

static mColumnItem *_add_item(mComboBox *p,int size,const char *text,uint32_t flags,intptr_t param)
{
	return mCIManagerInsertItem(&p->cb.manager, NULL, size,
		text, 0, flags, param,
		mWidgetGetFont(MLK_WIDGET(p)), FALSE);
}

/** ポップアップ表示 */

static void _run_popup(mComboBox *p)
{
	mBox box;

	mWidgetGetBox_rel(MLK_WIDGET(p), &box);
	
	if(mPopupListView_run(MLK_WIDGET(p),
		0, 0, &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_RIGHT | MPOPUP_F_GRAVITY_BOTTOM | MPOPUP_F_FLIP_Y,
		&p->cb.manager, _get_item_height(p) + 2, p->wg.w, -1))
	{
		//選択が変わった場合

		_notify_update(p);
	}
}


//======================
// main
//======================


/**@ mComboBox データ解放 */

void mComboBoxDestroy(mWidget *p)
{
	mCIManagerFree(&MLK_COMBOBOX(p)->cb.manager);
}

/**@ 作成 */

mComboBox *mComboBoxNew(mWidget *parent,int size,uint32_t fstyle)
{
	mComboBox *p;
	
	if(size < sizeof(mComboBox))
		size = sizeof(mComboBox);
	
	p = (mComboBox *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mComboBoxDestroy;
	p->wg.calc_hint = mComboBoxHandle_calcHint;
	p->wg.draw = mComboBoxHandle_draw;
	p->wg.event = mComboBoxHandle_event;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS | MWIDGET_STATE_ENABLE_KEY_REPEAT;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY | MWIDGET_EVENT_SCROLL;

	p->cb.fstyle = fstyle;

	mCIManagerInit(&p->cb.manager, FALSE);

	return p;
}

/**@ 作成 */

mComboBox *mComboBoxCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mComboBox *p;

	p = mComboBoxNew(parent, 0, fstyle);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ calc_hint ハンドラ関数 */

void mComboBoxHandle_calcHint(mWidget *p)
{
	//幅

	p->hintW = (_FRAME_W + _PADDING_X) * 2 + _ARROW_W;

	//高さ

	p->hintH = (_FRAME_W + _PADDING_Y) * 2 + _get_item_height(MLK_COMBOBOX(p));
	if(p->hintH < 9) p->hintH = 9; 
}


/**@ アイテム数取得 */

int mComboBoxGetItemNum(mComboBox *p)
{
	return p->cb.manager.list.num;
}

/**@ アイテムの高さセット
 *
 * @d:主に独自描画時用。
 *
 * @p:height 0 以下で、フォントの高さとなる。 */

void mComboBoxSetItemHeight(mComboBox *p,int height)
{
	p->cb.item_height = height;
}

/**@ アイテムの高さセット (最小値)
 *
 * @d:指定値がフォントの高さより大きい場合は、セットする。 */

void mComboBoxSetItemHeight_min(mComboBox *p,int height)
{
	int h;

	h = mWidgetGetFontHeight(MLK_WIDGET(p));

	if(height > h)
		p->cb.item_height = height;
}

/**@ アイテムの最大幅を推奨サイズとしてセット
 *
 * @d:全アイテムの追加後に実行する。\
 * 最大幅が hintRepW にセットされ、推奨サイズが置き換わる。 */

void mComboBoxSetAutoWidth(mComboBox *p)
{
	p->wg.hintRepW = mCIManagerGetMaxWidth(&p->cb.manager)
		+ (_FRAME_W + _PADDING_X) * 2 + _ARROW_W;
}


//========================
// アイテム追加
//========================


/**@ アイテム追加 */

mColumnItem *mComboBoxAddItem(mComboBox *p,const char *text,uint32_t flags,intptr_t param)
{
	return _add_item(p, 0, text, flags, param);
}

/**@ アイテム追加 (静的文字列) */

mColumnItem *mComboBoxAddItem_static(mComboBox *p,const char *text,intptr_t param)
{
	return _add_item(p, 0, text, 0, param);
}

/**@ アイテム追加 (文字列複製) */

mColumnItem *mComboBoxAddItem_copy(mComboBox *p,const char *text,intptr_t param)
{
	return _add_item(p, 0, text, MCOLUMNITEM_F_COPYTEXT, param);
}

/**@ すでに確保されたアイテムをリストに追加 */

void mComboBoxAddItem_ptr(mComboBox *p,mColumnItem *item)
{
	mListAppendItem(&p->cb.manager.list, MLISTITEM(item));
}


/**@ ヌル文字で区切られた文字列から、複数アイテム追加
 *
 * @d:各文字列は静的文字列として扱う。
 *
 * @p:text 終端は、ヌル文字が2つ続くこと。
 * @p:param アイテムのパラメータ値の開始値。\
 *  アイテムが追加するごとに +1 される。 */

void mComboBoxAddItems_sepnull(mComboBox *p,const char *text,intptr_t param)
{
	for(; *text; text += strlen(text) + 1)
		mComboBoxAddItem_static(p, text, param++);

	//静的文字列としてセットする場合、ヌル文字で終わっている必要があるため、
	//ヌル文字区切り以外の文字列ではセットできない。
}

/**@ ヌル文字で区切られた文字列から追加 (パラメータ配列指定)
 *
 * @p:array アイテムのパラメータ値の配列。アイテムの数だけ用意すること。 */

void mComboBoxAddItems_sepnull_arrayInt(mComboBox *p,const char *text,const int *array)
{
	for(; *text; text += strlen(text) + 1)
		mComboBoxAddItem_static(p, text, *(array++));
}

/**@ 指定文字で区切られた文字列から、複数アイテム追加
 *
 * @d:各文字列は、複製される。
 *
 * @p:split 区切り文字 */

void mComboBoxAddItems_sepchar(mComboBox *p,const char *text,char split,intptr_t param)
{
	const char *pc,*end;
	mStr str = MSTR_INIT;

	pc = text;

	while(mStringGetNextSplit(&pc, &end, split))
	{
		mStrSetText_len(&str, pc, end - pc);
	
		mComboBoxAddItem_copy(p, str.buf, param++);
		
		pc = end;
	}

	mStrFree(&str);
}

/**@ 文字列 ID から複数アイテム追加
 *
 * @d:翻訳グループはあらかじめ設定しておくこと。
 *
 * @p:idtop 文字列IDの開始値
 * @p:num  アイテム数
 * @p:param 先頭のパラメータ値。追加するごとに +1 される。 */

void mComboBoxAddItems_tr(mComboBox *p,int idtop,int num,intptr_t param)
{
	int i;

	for(i = 0; i < num; i++)
		mComboBoxAddItem_static(p, MLK_TR(idtop + i), param + i);
}


//==================
// 削除
//==================


/**@ アイテムをすべて削除 */

void mComboBoxDeleteAllItem(mComboBox *p)
{
	mCIManagerDeleteAllItem(&p->cb.manager);

	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ アイテムをポインタから削除 */

void mComboBoxDeleteItem(mComboBox *p,mColumnItem *item)
{
	if(mCIManagerDeleteItem(&p->cb.manager, item))
		mWidgetRedraw(MLK_WIDGET(p));
}

/**@ インデックス位置からアイテム削除
 *
 * @p:index 負の値で、現在の選択アイテム */

void mComboBoxDeleteItem_index(mComboBox *p,int index)
{
	if(mCIManagerDeleteItem_atIndex(&p->cb.manager, index))
		mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 現在の選択アイテムを削除した後、その前後のアイテムを選択する
 *
 * @r:新しく選択されたアイテム。\
 *   NULL で、選択アイテムがない or 削除後にアイテムが一つもない。*/

mColumnItem *mComboBoxDeleteItem_select(mComboBox *p)
{
	mColumnItem *focus,*sel;

	focus = p->cb.manager.item_focus;
	if(!focus) return NULL;

	sel = (mColumnItem *)((focus->i.next)? focus->i.next: focus->i.prev);

	mComboBoxDeleteItem(p, focus);
	mComboBoxSetSelItem(p, sel);

	return sel;
}


//======================
// アイテム取得
//======================


/**@ 先頭アイテム取得 */

mColumnItem *mComboBoxGetTopItem(mComboBox *p)
{
	return (mColumnItem *)p->cb.manager.list.top;
}

/**@ 選択アイテム取得 */

mColumnItem *mComboBoxGetSelectItem(mComboBox *p)
{
	return p->cb.manager.item_focus;
}

/**@ インデックス位置からアイテム取得
 *
 * @p:index 負の値で、現在の選択アイテム */

mColumnItem *mComboBoxGetItem_atIndex(mComboBox *p,int index)
{
	return mCIManagerGetItem_atIndex(&p->cb.manager, index);
}

/**@ パラメータ値からアイテム検索
 *
 * @r:NULL で見つからなかった */

mColumnItem *mComboBoxGetItem_fromParam(mComboBox *p,intptr_t param)
{
	return mCIManagerGetItem_fromParam(&p->cb.manager, param);
}

/**@ テキストからアイテム検索 */

mColumnItem *mComboBoxGetItem_fromText(mComboBox *p,const char *text)
{
	return mCIManagerGetItem_fromText(&p->cb.manager, text);
}


//======================
// アイテム
//======================


/**@ アイテムポインタからインデックス番号取得 */

int mComboBoxGetItemIndex(mComboBox *p,mColumnItem *item)
{
	return mCIManagerGetItemIndex(&p->cb.manager, item);
}

/**@ アイテムのテキスト取得 */

void mComboBoxGetItemText(mComboBox *p,int index,mStr *str)
{
	mColumnItem *pi = mComboBoxGetItem_atIndex(p, index);

	if(pi)
		mStrSetText(str, pi->text);
	else
		mStrEmpty(str);
}

/**@ アイテムのパラメータ値取得
 *
 * @p:index 負の値で現在の選択アイテム
 * @r:アイテムがなかった場合、0 */

intptr_t mComboBoxGetItemParam(mComboBox *p,int index)
{
	mColumnItem *pi;

	pi = mComboBoxGetItem_atIndex(p, index);

	return (pi)? pi->param: 0;
}

/**@ アイテムのテキスト変更 */

void mComboBoxSetItemText_static(mComboBox *p,mColumnItem *item,const char *text)
{
	__mColumnItemSetText(item, text, FALSE, FALSE, mWidgetGetFont(MLK_WIDGET(p)));

	if(item == p->cb.manager.item_focus)
		mWidgetRedraw(MLK_WIDGET(p));
}

/**@ アイテムのテキスト変更 (複製) */

void mComboBoxSetItemText_copy(mComboBox *p,int index,const char *text)
{
	mColumnItem *pi = mComboBoxGetItem_atIndex(p, index);

	if(pi)
	{
		__mColumnItemSetText(pi, text, TRUE, FALSE, mWidgetGetFont(MLK_WIDGET(p)));

		if(pi == p->cb.manager.item_focus)
			mWidgetRedraw(MLK_WIDGET(p));
	}
}


//======================
// 選択
//======================


/**@ 選択アイテムのインデックス番号取得 */

int mComboBoxGetSelIndex(mComboBox *p)
{
	return mCIManagerGetItemIndex(&p->cb.manager, NULL);
}

/**@ 選択アイテム変更
 *
 * @p:item NULL で選択をなしに */

void mComboBoxSetSelItem(mComboBox *p,mColumnItem *item)
{
	if(mCIManagerSetFocusItem(&p->cb.manager, item))
		mWidgetRedraw(MLK_WIDGET(p));
}

/**@ 選択アイテム変更 (インデックス位置)
 *
 * @p:index 負の値で選択なしに */

void mComboBoxSetSelItem_atIndex(mComboBox *p,int index)
{
	if(mCIManagerSetFocusItem_atIndex(&p->cb.manager, index))
		mWidgetRedraw(MLK_WIDGET(p));
}

/**@ パラメータ値を検索し、見つかったアイテムを選択
 *
 * @r:見つかったか */

mlkbool mComboBoxSetSelItem_fromParam(mComboBox *p,intptr_t param)
{
	mColumnItem *pi;

	pi = mCIManagerGetItem_fromParam(&p->cb.manager, param);
	if(pi) mComboBoxSetSelItem(p, pi);

	return (pi != NULL);
}

/**@ パラメータ値を検索し、見つかったアイテムを選択
 *
 * @d:見つからなかった場合、notfound_index の位置を選択 */

void mComboBoxSetSelItem_fromParam_notfound(mComboBox *p,intptr_t param,int notfound_index)
{
	if(!mComboBoxSetSelItem_fromParam(p, param))
		mComboBoxSetSelItem_atIndex(p, notfound_index);
}

/**@ テキストから検索し、見つかったアイテムを選択
 *
 * @r:見つかったか*/

mlkbool mComboBoxSetSelItem_fromText(mComboBox *p,const char *text)
{
	mColumnItem *pi;

	pi = mCIManagerGetItem_fromText(&p->cb.manager, text);
	if(pi) mComboBoxSetSelItem(p, pi);

	return (pi != NULL);
}


//========================
// ハンドラ
//========================


/** キー押し時 */

static void _event_key(mComboBox *p,uint32_t key)
{
	switch(key)
	{
		//上
		case MKEY_UP:
		case MKEY_KP_UP:
			_select_updown(p, FALSE);
			break;
		//下
		case MKEY_DOWN:
		case MKEY_KP_DOWN:
			_select_updown(p, TRUE);
			break;
		//スペース
		case MKEY_SPACE:
		case MKEY_KP_SPACE:
			_run_popup(p);
			break;
		//HOME
		case MKEY_HOME:
		case MKEY_KP_HOME:
			if(mCIManagerSetFocus_toHomeEnd(&p->cb.manager, TRUE))
				mWidgetRedraw(MLK_WIDGET(p));
			break;
		//END
		case MKEY_END:
		case MKEY_KP_END:
			if(mCIManagerSetFocus_toHomeEnd(&p->cb.manager, FALSE))
				mWidgetRedraw(MLK_WIDGET(p));
			break;
	}
}

/**@ イベントハンドラ関数 */

int mComboBoxHandle_event(mWidget *wg,mEvent *ev)
{
	mComboBox *p = MLK_COMBOBOX(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				&& ev->pt.btt == MLK_BTT_LEFT)
			{
				//左ボタン押しでポップアップ表示
				
				mWidgetSetFocus_redraw(wg, FALSE);
				_run_popup(p);
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			_event_key(p, ev->key.key);
			break;

		//スクロール (選択を上下)
		case MEVENT_SCROLL:
			if(ev->scroll.vert_step)
				_select_updown(p, (ev->scroll.vert_step > 0));
			break;

		//フォーカス
		case MEVENT_FOCUS:
			mWidgetRedraw(wg);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mComboBoxHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mComboBox *p = MLK_COMBOBOX(wg);
	mFont *font;
	mColumnItem *pi;
	int n;
	mlkbool is_disable;
	
	font = mWidgetGetFont(wg);

	is_disable = !mWidgetIsEnable(wg);

	//ボタン

	n = 0;
	if(wg->fstate & MWIDGET_STATE_FOCUSED) n |= MPIXBUF_DRAWBTT_FOCUSED;
	if(is_disable) n |= MPIXBUF_DRAWBTT_DISABLED;
	
	mPixbufDrawButton(pixbuf, 0, 0, wg->w, wg->h, n);

	//矢印

	mPixbufDrawArrowDown(pixbuf,
		wg->w - _FRAME_W - _PADDING_X - _ARROW_W + 4, (wg->h - 4) / 2,
		4,
		(is_disable)? MGUICOL_PIX(TEXT_DISABLE): MGUICOL_PIX(TEXT));

	//選択アイテム

	pi = p->cb.manager.item_focus;
	if(!pi) return;

	//描画関数

	n = 1;

	if(pi->draw)
	{
		//描画関数

		mColumnItemDrawInfo dinfo;

		dinfo.widget = wg;
		dinfo.box.x = _FRAME_W + _PADDING_X;
		dinfo.box.y = _FRAME_W + _PADDING_Y;
		dinfo.box.w = wg->w - (_FRAME_W + _PADDING_X) * 2 - _ARROW_W;
		dinfo.box.h = wg->h - (_FRAME_W + _PADDING_Y) * 2;
		dinfo.column = -1;
		dinfo.flags = 0;

		if(is_disable)
			dinfo.flags |= MCOLUMNITEM_DRAWF_DISABLED;

		if(wg->fstate & MWIDGET_STATE_FOCUSED)
			dinfo.flags |= MCOLUMNITEM_DRAWF_FOCUSED | MCOLUMNITEM_DRAWF_ITEM_SELECTED | MCOLUMNITEM_DRAWF_ITEM_FOCUS;

		if(mPixbufClip_setBox(pixbuf, &dinfo.box))
			n = (pi->draw)(pixbuf, pi, &dinfo);
		else
			n = 0;
	}

	//通常テキスト

	if(n)
	{
		mPixbufClip_setBox_d(pixbuf,
			_FRAME_W + _PADDING_X, 0,
			wg->w - (_FRAME_W + _PADDING_X) * 2 - _ARROW_W, wg->h);
		
		mFontDrawText_pixbuf(font, pixbuf,
			_FRAME_W + _PADDING_X, (wg->h - mFontGetHeight(font)) >> 1,
			pi->text, -1,
			(is_disable)? MGUICOL_RGB(TEXT_DISABLE): MGUICOL_RGB(TEXT));
	}
}

