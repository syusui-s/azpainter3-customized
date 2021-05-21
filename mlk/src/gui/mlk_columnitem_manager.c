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
 * mCIManager
 * (mColumnItem 管理)
 *****************************************/

#include <string.h>	//strcmp

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_font.h"
#include "mlk_buf.h"
#include "mlk_str.h"
#include "mlk_string.h"
#include "mlk_widget_def.h"

#include "mlk_columnitem_manager.h"

//-----------------

#define _ITEM_TOP(p)   MLK_COLUMNITEM(p->list.top)

#define _ITEMFLAG_ON(p,f)  (p)->flags |= MCOLUMNITEM_F_ ## f
#define _ITEMFLAG_OFF(p,f) (p)->flags &= ~(MCOLUMNITEM_F_ ## f)

//-----------------

/*
 - 複数選択を有効にする場合は is_multi_sel を TRUE にする。

 [mColumnItem]

 text_col: 複数列時の文字列データ (uint8 *)
	[長さ(可変)][文字列(NULL含む)]...[255(終端)]

	長さ: 1byte単位。※NULL 含まない。
		0〜253 はそのまま長さ。0 の場合は空文字列。
		254 で長さのバイトが続く。次の値は、-254 した値。
		最後の長さが 254 の場合は、254, 0 となる。

	文字列: 長さが 1 以上の場合は、NULL 文字含む。
		長さが 0 の場合は、データなし。
*/


//*******************************
// mColumnItem
//*******************************


/* テキストから、複数列用のデータを作成 */

static uint8_t *_create_multicol_data(const char *text)
{
	mBuf buf;
	char *top,*end;
	int len,n;
	uint8_t b,*pbuf;

	if(!text) return NULL;

	mBufInit(&buf);
	if(!mBufAlloc(&buf, 1024, 1024)) return NULL;

	top = (char *)text;
	
	while(*top)
	{
		end = mStringFindChar(top, '\t');

		len = end - top;

		if(len == 0)
		{
			//文字列なし (長さ 0)
			// 次が終端 ('\t' + 0) の場合、含めない

			if(*end == 0) break;

			if(!mBufAppendByte(&buf, 0)) goto ERR;
		}
		else
		{
			//文字列長さ
			// 254 以上の場合、254 が続く (254 の場合、[254][0])
			// 254 未満の場合、その値。

			for(n = len; n; n -= b)
			{
				b = (n < 254)? n: 254;

				if(!mBufAppend(&buf, &b, 1)) goto ERR;

				//残りが 254 の場合、0 を追加

				if(n == 254 && !mBufAppendByte(&buf, 0)) goto ERR;
			}

			//文字列 (NULL 文字含む)

			if(!mBufAppend(&buf, top, len)
				|| !mBufAppendByte(&buf, 0))
				goto ERR;
		}

		//次へ

		if(*end == 0) break;

		top = end + 1;
	}

	//終端

	if(buf.cursize && !mBufAppendByte(&buf, 255))
		goto ERR;

	//コピー

	pbuf = (uint8_t *)mBufCopyToBuf(&buf);

	mBufFree(&buf);

	return pbuf;

ERR:
	mBufFree(&buf);
	return NULL;
}

/* 複数列テキスト、次のテキストを取得
 *
 * pptext: 空文字列で NULL
 * return: -1 で終了。それ以外で長さ */

static int _next_coltext(uint8_t **ppbuf,char **pptext)
{
	uint8_t *pt = *ppbuf;
	int len,n;

	if(!pt || *pt == 255)
	{
		*pptext = NULL;
		return -1;
	}

	//長さ

	for(len = 0; 1; )
	{
		n = *(pt++);
		len += n;
		if(n < 254) break;
	}

	//

	*pptext = (len == 0)? NULL: (char *)pt;
	*ppbuf = pt + len + (len != 0);

	return len;
}

/**@ アイテム破棄ハンドラ */

void mColumnItemHandle_destroyItem(mList *list,mListItem *item)
{
	mColumnItem *p = (mColumnItem *)item;

	if(p->flags & MCOLUMNITEM_F_COPYTEXT)
		mFree(p->text);

	mFree(p->text_col);
}

/**@ 複数列テキストの文字列を取得
 *
 * @p:col 列の位置
 * @p:pptop 文字列の先頭のポインタが入る (空なら NULL)
 * @r:文字長さ (0 で空) */

int mColumnItem_getColText(mColumnItem *p,int col,char **pptop)
{
	uint8_t *pt = p->text_col;
	int len = 0,no,n;

	if(pt)
	{
		for(no = 0; *pt != 255; )
		{
			//長さ
			n = *(pt++);
			len += n;
			
			if(n < 254)
			{
				if(no == col) break;

				if(len) pt += len + 1;
				no++;
				len = 0;
			}
		}
	}

	if(len == 0)
	{
		//空
		*pptop = NULL;
		return 0;
	}
	else
	{
		*pptop = (char *)pt;
		return len;
	}
}

/**@ 複数列の指定位置のテキストを置き換え
 *
 * @p:text NULL で空文字列 */

void mColumnItem_setColText(mColumnItem *p,int col,const char *text)
{
	mStr str = MSTR_INIT;
	uint8_t *pt;
	char *pc;
	int i,len;

	pt = p->text_col;

	//指定位置までの各テキストを取得

	for(i = 0; i < col; i++)
	{
		len = _next_coltext(&pt, &pc);

		mStrAppendText_len(&str, pc, len);
		mStrAppendChar(&str, '\t');
	}

	//置換

	_next_coltext(&pt, &pc);

	mStrAppendText(&str, text);
	mStrAppendChar(&str, '\t');

	//残りを追加

	while(1)
	{
		len = _next_coltext(&pt, &pc);
		if(len == -1) break;

		mStrAppendText_len(&str, pc, len);
		mStrAppendChar(&str, '\t');
	}

	//セット

	mFree(p->text_col);
	
	p->text_col = _create_multicol_data(str.buf);

	mStrFree(&str);
}

/**@ リストのソート関数:strcmp (単一列) */

int mColumnItem_sortFunc_strcmp(mListItem *item1,mListItem *item2,void *param)
{
	return strcmp(((mColumnItem *)item1)->text, ((mColumnItem *)item2)->text);
}

/** アイテムのテキストを変更 */

void __mColumnItemSetText(mColumnItem *pi,const char *text,
	mlkbool copy,mlkbool multi_column,mFont *font)
{
	if(!multi_column)
	{
		//----- 単一列
	
		//解放

		if(pi->flags & MCOLUMNITEM_F_COPYTEXT)
			mFree(pi->text);

		//セット

		if(copy)
		{
			pi->text = mStrdup(text);
			pi->flags |= MCOLUMNITEM_F_COPYTEXT;
		}
		else
		{
			pi->text = (char *)text;
			pi->flags &= ~MCOLUMNITEM_F_COPYTEXT;
		}

		//テキスト幅

		pi->text_width = mFontGetTextWidth(font, pi->text, -1);
	}
	else
	{
		//----- 複数列

		mFree(pi->text_col);

		pi->text_col = _create_multicol_data(text);
	}
}


//*******************************
// mCIManager
//*******************************


/**@ 解放 */

void mCIManagerFree(mCIManager *p)
{
	mListDeleteAll(&p->list);
}

/**@ 初期化 */

void mCIManagerInit(mCIManager *p,mlkbool multi_sel)
{
	p->list.item_destroy = mColumnItemHandle_destroyItem;
	p->is_multi_sel = multi_sel;
}


//=====================
// アイテム
//=====================


/**@ アイテム挿入
 *
 * @p:ins  アイテムの挿入位置
 * @p:size アイテムの確保サイズ */

mColumnItem *mCIManagerInsertItem(mCIManager *p,
	mColumnItem *ins,int size,
	const char *text,int icon,uint32_t flags,intptr_t param,
	mFont *font,mlkbool multi_column)
{
	mColumnItem *pi;

	if(size < sizeof(mColumnItem))
		size = sizeof(mColumnItem);
	
	pi = (mColumnItem *)mListInsertNew(&p->list, MLISTITEM(ins), size);
	if(!pi) return NULL;

	__mColumnItemSetText(pi, text,
		((flags & MCOLUMNITEM_F_COPYTEXT) != 0), multi_column, font);

	pi->icon = icon;
	pi->flags = flags;
	pi->param = param;

	return pi;
}


//======================
// 削除
//======================


/**@ アイテムすべて削除 */

void mCIManagerDeleteAllItem(mCIManager *p)
{
	mListDeleteAll(&p->list);

	p->item_focus = NULL;
}

/**@ アイテム削除
 *
 * @r:TRUE でフォーカスアイテムが削除された */

mlkbool mCIManagerDeleteItem(mCIManager *p,mColumnItem *item)
{
	mlkbool ret = FALSE;

	if(item)
	{
		if(item == p->item_focus)
		{
			p->item_focus = NULL;
			ret = TRUE;
		}

		mListDelete(&p->list, MLISTITEM(item));
	}

	return ret;
}

/**@ アイテム削除 (インデックス位置指定)
 *
 * @p:index 負の値で、現在のフォーカスアイテム
 * @r:TRUE でフォーカスアイテムが削除された */

mlkbool mCIManagerDeleteItem_atIndex(mCIManager *p,int index)
{
	return mCIManagerDeleteItem(p, mCIManagerGetItem_atIndex(p, index));
}


//======================
// すべての選択
//======================


/**@ すべてのアイテムを選択 */

void mCIManagerSelectAll(mCIManager *p)
{
	mColumnItem *pi;

	if(p->is_multi_sel)
	{
		for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
			_ITEMFLAG_ON(pi, SELECTED);
	}
}

/**@ すべての選択解除 */

void mCIManagerUnselectAll(mCIManager *p)
{
	mColumnItem *pi;
	
	if(p->is_multi_sel)
	{
		for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
			_ITEMFLAG_OFF(pi, SELECTED);
	}
}


//======================
// 取得
//======================


/**@ アイテムのインデックス番号取得
 *
 * @p:item NULL でフォーカスアイテム */

int mCIManagerGetItemIndex(mCIManager *p,mColumnItem *item)
{
	if(!item) item = p->item_focus;

	return mListItemGetIndex(MLISTITEM(item));
}

/**@ 位置からアイテム取得
 *
 * @p:index 負の値で、現在のフォーカスアイテム */

mColumnItem *mCIManagerGetItem_atIndex(mCIManager *p,int index)
{
	if(index < 0)
		return p->item_focus;
	else if(index >= p->list.num)
		return NULL;
	else
		return (mColumnItem *)mListGetItemAtIndex(&p->list, index);
}

/**@ パラメータ値からアイテム検索 */

mColumnItem *mCIManagerGetItem_fromParam(mCIManager *p,intptr_t param)
{
	mColumnItem *pi;

	for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		if(pi->param == param)
			return pi;
	}

	return NULL;
}

/**@ テキストからアイテム検索 */

mColumnItem *mCIManagerGetItem_fromText(mCIManager *p,const char *text)
{
	mColumnItem *pi;

	for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		if(strcmp(pi->text, text) == 0)
			return pi;
	}

	return NULL;
}


//=========================
// フォーカスアイテム
//=========================


/**@ フォーカスアイテムセット
 *
 * @d:指定アイテム1つだけを選択状態にする。
 *
 * @p:item NULL で選択解除
 * @r:item がすでにフォーカス状態の場合、FALSE */

mlkbool mCIManagerSetFocusItem(mCIManager *p,mColumnItem *item)
{
	if(p->item_focus == item)
		return FALSE;
	else
	{
		//選択解除
		
		if(p->is_multi_sel)
			mCIManagerUnselectAll(p);
		else
		{
			if(p->item_focus)
				_ITEMFLAG_OFF(p->item_focus, SELECTED);
		}

		//選択
		
		if(item)
			_ITEMFLAG_ON(item, SELECTED);
		
		p->item_focus = item;
		
		return TRUE;
	}
}

/**@ 指定位置のアイテムをフォーカスにセット
 *
 * @p:index 負の値で選択を解除 */

mlkbool mCIManagerSetFocusItem_atIndex(mCIManager *p,int index)
{
	mColumnItem *pi;

	if(index < 0)
		pi = NULL;
	else
		pi = (mColumnItem *)mListGetItemAtIndex(&p->list, index);

	return mCIManagerSetFocusItem(p, pi);
}

/**@ 先頭または終端のアイテムをフォーカスにセット */

mlkbool mCIManagerSetFocus_toHomeEnd(mCIManager *p,mlkbool home)
{
	return mCIManagerSetFocusItem(p,
		(home)? MLK_COLUMNITEM(p->list.top): MLK_COLUMNITEM(p->list.bottom));
}


//=========================
// 操作
//=========================


/**@ アイテムの位置を上下に移動
 *
 * @p:item  NULL でフォーカスアイテム */

mlkbool mCIManagerMoveItem_updown(mCIManager *p,mColumnItem *item,mlkbool down)
{
	if(!item) item = p->item_focus;

	return mListMoveUpDown(&p->list, MLISTITEM(item), !down);
}

/**@ フォーカスアイテムを上下に選択移動 */

mlkbool mCIManagerFocusItem_updown(mCIManager *p,mlkbool down)
{
	mColumnItem *item;
	
	item = p->item_focus;
	
	if(!item)
		//フォーカスがなければ先頭
		item = _ITEM_TOP(p);
	else if(down)
	{
		//下

		if(item->i.next)
			item = MLK_COLUMNITEM(item->i.next);
	}
	else
	{
		//上

		if(item->i.prev)
			item = MLK_COLUMNITEM(item->i.prev);
	}
	
	return mCIManagerSetFocusItem(p, item);
}

/**@ クリックによるアイテム選択処理
 *
 * @d:ポップアップ時は、ポインタ移動中の選択にも使う。
 * 
 * @p:item  常に NULL 以外を指定
 * @r:操作に関するフラグ。\
 *  [bit0] フォーカスアイテムが変更された。\
 *  [bit1] 通常の単一選択操作において、item がフォーカスアイテムだった。\
 *  [bit2] 複数選択時、+Shift/+Ctrl による操作が行われた。 */

int mCIManagerOnClick(mCIManager *p,uint32_t key_state,mColumnItem *item)
{
	mColumnItem *focus,*pi,*top,*end;
	
	focus = p->item_focus;

	if(!p->is_multi_sel)
	{
		//----- 単一選択

		if(focus == item)
			return MCIMANAGER_ONCLICK_ON_FOCUS;
		else
		{
			if(focus)
				_ITEMFLAG_OFF(focus, SELECTED);
			
			_ITEMFLAG_ON(item, SELECTED);

			p->item_focus = item;

			return MCIMANAGER_ONCLICK_CHANGE_FOCUS;
		}
	}
	else
	{
		//----- 複数選択

		if(key_state & MLK_STATE_CTRL)
		{
			//+Ctrl: 選択反転
			
			item->flags ^= MCOLUMNITEM_F_SELECTED;

			p->item_focus = item;

			return MCIMANAGER_ONCLICK_KEY_SEL | ((focus != item)? MCIMANAGER_ONCLICK_CHANGE_FOCUS: 0);
		}
		else if((key_state & MLK_STATE_SHIFT) && focus)
		{
			//+Shift: フォーカス位置から指定位置までを選択
			// [!] フォーカス位置は変更しない

			mCIManagerUnselectAll(p);
			
			if(mListItemGetDir(MLISTITEM(focus), MLISTITEM(item)) < 0)
				top = focus, end = item;
			else
				top = item, end = focus;
			
			for(pi = top; pi; pi = MLK_COLUMNITEM(pi->i.next))
			{
				_ITEMFLAG_ON(pi, SELECTED);
			
				if(pi == end) break;
			}

			return MCIMANAGER_ONCLICK_KEY_SEL;
		}
		else
		{
			//通常 (単一選択)
			
			mCIManagerUnselectAll(p);
			
			_ITEMFLAG_ON(item, SELECTED);

			p->item_focus = item;
			
			return (focus != item)? MCIMANAGER_ONCLICK_CHANGE_FOCUS: MCIMANAGER_ONCLICK_ON_FOCUS;
		}
	}
}


//==========================
// ほか
//==========================


/**@ アイテムテキストの最大幅取得 (単一列時) */

int mCIManagerGetMaxWidth(mCIManager *p)
{
	mColumnItem *pi;
	int max = 0;

	for(pi = _ITEM_TOP(p); pi; pi = MLK_COLUMNITEM(pi->i.next))
	{
		if(pi->text_width > max)
			max = pi->text_width;
	}

	return max;
}
