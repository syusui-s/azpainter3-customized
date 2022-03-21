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
 * mTab
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_tab.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_key.h"
#include "mlk_list.h"

#include "mlk_pv_widget.h"


//--------------------

#define _PADDING_TABX  4	//テキストの左右の余白
#define _PADDING_TABY  3	//テキストの上下の余白
#define _MINSPACE_HORZ 12	//詰める際の最小タブ幅 (水平)
#define _MINSPACE_VERT 14	//詰める際の最小タブ幅 (垂直)

#define _IS_VERT_TAB(p) (p->tab.fstyle & (MTAB_S_LEFT | MTAB_S_RIGHT))
#define _IS_HAVE_SEP(p) (p->tab.fstyle & MTAB_S_HAVE_SEP)
#define _IS_SHORTEN(p)  (p->tab.fstyle & MTAB_S_SHORTEN)

//--------------------


//========================
// sub
//========================


/* 各タブの位置を計算 (水平) */

static void _calc_tabpos_horz(mTab *p)
{
	mTabItem *pi,*pi2,*pitop;
	int pos,remain,flag,f;

	//通常時の位置をセット

	pos = 0;

	for(pi = MLK_TABITEM(p->tab.list.top); pi; pi = MLK_TABITEM(pi->i.next))
	{
		pi->pos = pos;
		pos += pi->width - 1; //右の枠は重ねる
	}

	//詰めるか

	if(!_IS_SHORTEN(p)) return;

	//ウィジェット幅内に収まっていればそのまま
	//+1 は、最後の枠分

	remain = (pos + 1) - p->wg.w;
	if(remain <= 0) return;

	//------ 詰める

	//2番目のアイテムから開始
	
	pitop = (mTabItem *)mListGetItemAtIndex(&p->tab.list, 1);
	flag = TRUE;

	while(remain > 0 && flag)
	{
		for(pi = pitop, flag = FALSE; pi && remain; pi = MLK_TABITEM(pi->i.next))
		{
			pi2 = MLK_TABITEM(pi->i.prev);

			//左のタブの左端〜現在のタブの左端との間に、詰められる隙間があるか

			if(pi->pos - pi2->pos <= _MINSPACE_HORZ) continue;

			//現在のタブの右端と、左側の全タブの右端との間に、詰められる隙間があるか

			pos = pi->pos + pi->width;

			for(f = 0; pi2; pi2 = MLK_TABITEM(pi2->i.prev))
			{
				if(pos - pi2->pos - pi2->width <= _MINSPACE_HORZ)
				{
					f = 1;
					break;
				}
			}

			if(f) continue;

			//隙間があれば、右側の全タブ位置を 1px 左へ

			for(pi2 = pi; pi2; pi2 = MLK_TABITEM(pi2->i.next))
				pi2->pos--;

			remain--;
			flag = TRUE;
		}
	}
}

/* 各タブの位置を計算 (垂直) */

static void _calc_tabpos_vert(mTab *p)
{
	mTabItem *pi,*pi2,*pitop;
	int pos,max,remain,itemh,n,i;

	itemh = mWidgetGetFontHeight(MLK_WIDGET(p)) + _PADDING_TABY * 2;

	//通常時の位置をセット

	pos = 0;

	for(pi = MLK_TABITEM(p->tab.list.top); pi; pi = MLK_TABITEM(pi->i.next))
	{
		pi->pos = pos;
		pos += itemh - 1;
	}

	//2個以上の時、詰めるか

	if(p->tab.list.num < 2 || !_IS_SHORTEN(p)) return;

	remain = (pos + 1) - p->wg.h;
	if(remain <= 0) return;

	//------ 詰める

	pitop = (mTabItem *)mListGetItemAtIndex(&p->tab.list, 1);

	//各タブのずらせる最大幅

	max = itemh - _MINSPACE_VERT;
	if(max <= 0) return;

	//各タブを均等にずらす

	n = remain / (p->tab.list.num - 1);
	if(n > max) n = max;

	for(pi = pitop, i = n; pi; pi = MLK_TABITEM(pi->i.next), i += n)
		pi->pos -= i;

	//残り

	remain -= n * (p->tab.list.num - 1);

	if(remain && n < max)
	{
		for(pi = pitop; pi && remain > 0; pi = MLK_TABITEM(pi->i.next))
		{
			for(pi2 = pi; pi2; pi2 = MLK_TABITEM(pi2->i.next))
				pi2->pos--;
		
			remain--;
		}
	}
}

/* 選択アイテム変更 */

static void _change_selitem(mTab *p,mTabItem *pi)
{
	if(pi && pi != p->tab.focusitem)
	{
		p->tab.focusitem = pi;

		mWidgetRedraw(MLK_WIDGET(p));

		//通知

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
			MTAB_N_CHANGE_SEL,
			mListItemGetIndex(MLISTITEM(pi)), (intptr_t)pi);
	}
}

/* カーソル位置からアイテム取得 */

static mTabItem *_getitem_point(mTab *p,int x,int y)
{
	mTabItem *pi,*focus,*next;
	int itemh;

	//フォーカスがない場合、終端をフォーカスとする

	focus = p->tab.focusitem;
	if(!focus) focus = MLK_TABITEM(p->tab.list.bottom);
	if(!focus) return NULL;

	//

	if(_IS_VERT_TAB(p))
	{
		//------ 垂直

		itemh = mWidgetGetFontHeight(MLK_WIDGET(p)) + _PADDING_TABY * 2;

		//フォーカスアイテムを優先判定

		if(focus && focus->pos <= y && y < focus->pos + itemh)
			return focus;

		if(y < focus->pos)
		{
			//フォーカスより前

			for(pi = focus; pi; pi = next)
			{
				next = MLK_TABITEM(pi->i.prev);
				if(next && y >= next->pos && y < pi->pos)
					return next;
			}
		}
		else
		{
			//フォーカスより後

			for(pi = focus; pi; pi = next)
			{
				next = MLK_TABITEM(pi->i.next);
				if(next && y >= pi->pos + itemh && y < next->pos + itemh)
					return next;
			}
		}
	}
	else
	{
		//------ 水平

		//フォーカスアイテムを優先判定

		if(focus->pos <= x && x < focus->pos + focus->width)
			return focus;

		if(x < focus->pos)
		{
			//フォーカスより前

			for(pi = focus; pi; pi = next)
			{
				next = MLK_TABITEM(pi->i.prev);
				if(next && x >= next->pos && x < pi->pos)
					return next;
			}
		}
		else
		{
			//フォーカスより後

			for(pi = focus; pi; pi = next)
			{
				next = MLK_TABITEM(pi->i.next);
				if(next && x >= pi->pos + pi->width && x < next->pos + next->width)
					return next;
			}
		}
	}

	return NULL;
}

/* 全アイテムの中で最大の幅を取得 */

static int _get_maxitemwidth(mTab *p)
{
	mTabItem *pi;
	int max = 0;

	for(pi = MLK_TABITEM(p->tab.list.top); pi; pi = MLK_TABITEM(pi->i.next))
	{
		if(max < pi->width) max = pi->width;
	}

	return max;
}

/* 水平時、全アイテム分のウィジェット幅を取得 */

static int _get_horz_widget_width(mTab *p)
{
	mTabItem *pi;
	int w = 0;

	MLK_LIST_FOR(p->tab.list, pi, mTabItem)
		w += pi->width - 1;

	return w + 1;
}

/* construct ハンドラ関数 */

static void _construct_handle(mWidget *wg)
{
	mTab *p = MLK_TAB(wg);

	if(_IS_VERT_TAB(p))
		_calc_tabpos_vert(p);
	else
		_calc_tabpos_horz(p);

	mWidgetRedraw(wg);
}

/* アイテム削除ハンドラ */

static void _item_destroy(mList *list,mListItem *item)
{
	mTabItem *p = (mTabItem *)item;

	if(p->flags & MTABITEM_F_COPYTEXT)
		mFree(MLK_TABITEM(p)->text);
}


//========================
// メイン
//========================


/**@ mTab データ解放 */

void mTabDestroy(mWidget *wg)
{
	mListDeleteAll(&MLK_TAB(wg)->tab.list);
}

/**@ 作成 */

mTab *mTabNew(mWidget *parent,int size,uint32_t fstyle)
{
	mTab *p;
	
	if(size < sizeof(mTab))
		size = sizeof(mTab);
	
	p = (mTab *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mTabDestroy;
	p->wg.calc_hint = mTabHandle_calcHint;
	p->wg.draw = mTabHandle_draw;
	p->wg.event = mTabHandle_event;
	p->wg.resize = mTabHandle_resize;
	p->wg.construct = _construct_handle;
	
	p->wg.fstate |= MWIDGET_STATE_TAKE_FOCUS;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_KEY;

	p->tab.fstyle = fstyle;

	p->tab.list.item_destroy = _item_destroy;
	
	return p;
}

/**@ 作成 */

mTab *mTabCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mTab *p;

	p = mTabNew(parent, 0, fstyle);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ calc_hint ハンドラ関数 */

void mTabHandle_calcHint(mWidget *wg)
{
	mTab *p = MLK_TAB(wg);

	if(_IS_VERT_TAB(p))
	{
		//垂直

		wg->hintW = _get_maxitemwidth(p) + 1; //フォーカスタブ用に +1

		if(_IS_HAVE_SEP(p)) wg->hintW++;
	}
	else
	{
		//水平
		
		wg->hintH = mWidgetGetFontHeight(wg) + _PADDING_TABY * 2;

		if(_IS_HAVE_SEP(p)) wg->hintH++;
	}
}

/**@ resize ハンドラ関数 */

void mTabHandle_resize(mWidget *wg)
{
	mTab *p = MLK_TAB(wg);

	//詰める場合、位置を再セット

	if(_IS_SHORTEN(p))
	{
		if(_IS_VERT_TAB(p))
			_calc_tabpos_vert(p);
		else
			_calc_tabpos_horz(p);
	}
}

/**@ アイテム追加
 *
 * @p:w タブの幅。負の値でテキストから自動計算。 */

void mTabAddItem(mTab *p,const char *text,int w,uint32_t flags,intptr_t param)
{
	mTabItem *pi;

	//幅

	if(w < 0)
		w = mFontGetTextWidth(mWidgetGetFont(MLK_WIDGET(p)), text, -1) + _PADDING_TABX * 2;

	//追加

	pi = (mTabItem *)mListAppendNew(&p->tab.list, sizeof(mTabItem));
	if(pi)
	{
		if(flags & MTABITEM_F_COPYTEXT)
			pi->text = mStrdup(text);
		else
			pi->text = (char *)text;

		pi->width = w;
		pi->flags = flags;
		pi->param = param;

		mWidgetSetConstruct(MLK_WIDGET(p));
	}
}

/**@ アイテム追加 (静的テキストのみ指定) */

void mTabAddItem_text_static(mTab *p,const char *text)
{
	mTabAddItem(p, text, -1, 0, 0);
}

/**@ インデックス位置からタブアイテム削除
 *
 * @r:削除したか */

mlkbool mTabDelItem_atIndex(mTab *p,int index)
{
	mTabItem *pi = mTabGetItem_atIndex(p, index);

	if(!pi)
		return FALSE;
	else
	{
		mListDelete(&p->tab.list, MLISTITEM(pi));

		if(pi == p->tab.focusitem)
			p->tab.focusitem = MLK_TABITEM(p->tab.list.top);

		mWidgetSetConstruct(MLK_WIDGET(p));

		return TRUE;
	}
}

/**@ インデックス位置からタブアイテム取得
 *
 * @p:index 負の値で現在の選択タブ */

mTabItem *mTabGetItem_atIndex(mTab *p,int index)
{
	if(index < 0)
		return p->tab.focusitem;
	else
		return (mTabItem *)mListGetItemAtIndex(&p->tab.list, index);
}

/**@ 現在の選択タブのインデックス位置を取得
 *
 * @r:-1 で選択なし */

int mTabGetSelItemIndex(mTab *p)
{
	return mListItemGetIndex(MLISTITEM(p->tab.focusitem));
}

/**@ インデックス位置からアイテムのパラメータ値取得
 *
 * @r:アイテムがない場合、0 */

intptr_t mTabGetItemParam_atIndex(mTab *p,int index)
{
	mTabItem *pi = mTabGetItem_atIndex(p, index);

	return (pi)? pi->param: 0;
}

/**@ インデックスから選択タブをセット */

void mTabSetSel_atIndex(mTab *p,int index)
{
	mTabItem *pi = mTabGetItem_atIndex(p, index);

	if(pi != p->tab.focusitem)
	{
		p->tab.focusitem = pi;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ アイテム数から推奨サイズをセット
 *
 * @d:アイテム追加後、ウィジェットサイズをその分の固定サイズにしたい時に使う。\
 *  水平タブなら、hintRepW に幅をセット。\
 *  垂直タブなら、hintRepH に高さをセット。 */

void mTabSetHintSize(mTab *p)
{
	if(_IS_VERT_TAB(p))
		p->wg.hintRepH = (mWidgetGetFontHeight(MLK_WIDGET(p)) + _PADDING_TABY * 2 - 1) * p->tab.list.num + 1;
	else
		p->wg.hintRepW = _get_horz_widget_width(p);
}


//========================
// イベント
//========================


/* キー押し時 */

static void _event_key(mTab *p,mEventKey *ev)
{
	mListItem *pi;
	uint32_t key;
	int dir = 0;

	if(!p->tab.focusitem) return;

	//移動方向

	key = ev->key;

	if(_IS_VERT_TAB(p))
	{
		if(key == MKEY_UP || key == MKEY_KP_UP)
			dir = -1;
		else if(key == MKEY_DOWN || key == MKEY_KP_DOWN)
			dir = 1;
	}
	else
	{
		if(key == MKEY_LEFT || key == MKEY_KP_LEFT)
			dir = -1;
		else if(key == MKEY_RIGHT || key == MKEY_KP_RIGHT)
			dir = 1;
	}

	if(dir == 0) return;

	//移動

	pi = MLISTITEM(p->tab.focusitem);
	pi = (dir < 0)? pi->prev: pi->next;

	if(pi) _change_selitem(p, MLK_TABITEM(pi));
}

/**@ event ハンドラ関数 */

int mTabHandle_event(mWidget *wg,mEvent *ev)
{
	mTab *p = MLK_TAB(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				&& ev->pt.btt == MLK_BTT_LEFT)
			{
				//フォーカスセット

				mWidgetSetFocus_redraw(wg, FALSE);

				//選択アイテム変更

				_change_selitem(p, _getitem_point(p, ev->pt.x, ev->pt.y));
			}
			break;

		//キー
		case MEVENT_KEYDOWN:
			_event_key(p, &ev->key);
			break;
		
		case MEVENT_FOCUS:
			mWidgetRedraw(wg);
			break;
		default:
			return FALSE;
	}

	return TRUE;
}


//==========================
// 描画
//==========================


/* 水平のタブ描画 */

static void _draw_horz_tab(mTab *p,mPixbuf *pixbuf)
{
	mFont *font;
	mTabItem *pi,*focusitem;
	int step,texty,x,h;
	uint32_t col_frame;
	mlkbool sep,nofocus,bottom,enable;

	font = mWidgetGetFont(MLK_WIDGET(p));

	h = p->wg.h;
	texty = (h - mFontGetHeight(font)) >> 1;
	focusitem = p->tab.focusitem;
	
	sep = (_IS_HAVE_SEP(p) != 0);
	bottom = ((p->tab.fstyle & MTAB_S_BOTTOM) != 0);
	enable = mWidgetIsEnable(MLK_WIDGET(p));

	col_frame = MGUICOL_PIX(FRAME);

	//セパレータ

	if(sep)
		mPixbufLineH(pixbuf, 0, (bottom)? 0: h - 1, p->wg.w, col_frame);

	//step0 = 先頭〜フォーカス
	//step1 = 終端〜フォーカス (フォーカスがない場合なし)
	//step2 = フォーカス (フォーカスがない場合なし)

	for(step = 0; step < 3; step++)
	{
		if(step == 0)
			pi = MLK_TABITEM(p->tab.list.top);
		else if(step == 1)
			pi = MLK_TABITEM(p->tab.list.bottom);
		else
			pi = focusitem;
		
		nofocus = (step != 2);

		//
	
		while(pi)
		{
			//step0,1 の場合、フォーカス位置まで
			if(step != 2 && pi == focusitem) break;

			x = pi->pos;

			//枠

			mPixbufBox(pixbuf, x, (bottom)? sep: nofocus,
				pi->width, h - sep - nofocus,
				(!nofocus && (p->wg.fstate & MWIDGET_STATE_FOCUSED))? MGUICOL_PIX(FRAME_FOCUS): col_frame);

			//背景
			//(フォーカスアイテム時、セパレータの線は消す)

			mPixbufFillBox(pixbuf, x + 1, (bottom)? (nofocus && sep): 1 + nofocus,
				pi->width - 2, h - 1 - nofocus - (nofocus && sep),
				(!nofocus || !enable)? MGUICOL_PIX(FACE): MGUICOL_PIX(FACE_DARK));

			//テキスト

			if(mPixbufClip_setBox_d(pixbuf,
				x + _PADDING_TABX, 0, pi->width - _PADDING_TABX * 2, h))
			{
				mFontDrawText_pixbuf(font, pixbuf,
					x + _PADDING_TABX,
					texty + (bottom? !nofocus: nofocus),
					pi->text, -1,
					(enable)? MGUICOL_RGB(TEXT): MGUICOL_RGB(TEXT_DISABLE));
			}

			mPixbufClip_clear(pixbuf);

			//次

			if(step == 0)
				pi = MLK_TABITEM(pi->i.next);
			else if(step == 1)
				pi = MLK_TABITEM(pi->i.prev);
			else
				break;
		}

		//フォーカスがない場合、step0 で終了
		if(!focusitem) break;
	}
}

/* 垂直のタブ描画 */

static void _draw_vert_tab(mTab *p,mPixbuf *pixbuf)
{
	mFont *font;
	mTabItem *pi,*focusitem;
	int step,y,w,itemh;
	uint32_t col_frame;
	mlkbool sep,nofocus,right,enable;

	font = mWidgetGetFont(MLK_WIDGET(p));
	w = p->wg.w;
	itemh = mFontGetHeight(font) + _PADDING_TABY * 2;
	focusitem = p->tab.focusitem;

	enable = mWidgetIsEnable(MLK_WIDGET(p));
	sep = (_IS_HAVE_SEP(p) != 0);
	right = ((p->tab.fstyle & MTAB_S_RIGHT) != 0);

	col_frame = MGUICOL_PIX(FRAME);

	//セパレータ

	if(sep)
		mPixbufLineV(pixbuf, (right)? 0: w - 1, 0, p->wg.h, col_frame);

	//タブ

	for(step = 0; step < 3; step++)
	{
		if(step == 0)
			pi = MLK_TABITEM(p->tab.list.top);
		else if(step == 1)
			pi = MLK_TABITEM(p->tab.list.bottom);
		else
			pi = focusitem;
		
		nofocus = (step != 2);

		//

		while(pi)
		{
			//step0,1 の場合、フォーカス位置まで
			if(step != 2 && pi == focusitem) break;

			y = pi->pos;

			//枠

			mPixbufBox(pixbuf, (right)? sep: nofocus, y, w - sep - nofocus, itemh,
				(!nofocus && (p->wg.fstate & MWIDGET_STATE_FOCUSED))? MGUICOL_PIX(FRAME_FOCUS): col_frame);

			//背景

			mPixbufFillBox(pixbuf,
				(right)? (nofocus && sep): 1 + nofocus, y + 1,
				w - 1 - nofocus - (nofocus && sep), itemh - 2,
				(!nofocus || !enable)? MGUICOL_PIX(FACE): MGUICOL_PIX(FACE_DARK));

			//テキスト

			if(mPixbufClip_setBox_d(pixbuf,
				_PADDING_TABX, y, w - _PADDING_TABX * 2, itemh))
			{
				mFontDrawText_pixbuf(font, pixbuf,
					(right? sep + !nofocus: nofocus) + _PADDING_TABX,
					y + _PADDING_TABY,
					pi->text, -1,
					(enable)? MGUICOL_RGB(TEXT): MGUICOL_RGB(TEXT_DISABLE));
			}

			mPixbufClip_clear(pixbuf);

			//次

			if(step == 0)
				pi = MLK_TABITEM(pi->i.next);
			else if(step == 1)
				pi = MLK_TABITEM(pi->i.prev);
			else
				break;
		}

		//フォーカスがない場合、step0 で終了
		if(!focusitem) break;
	}
}

/**@ draw ハンドラ関数 */

void mTabHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mTab *p = MLK_TAB(wg);

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//

	if(_IS_VERT_TAB(p))
		_draw_vert_tab(p, pixbuf);
	else
		_draw_horz_tab(p, pixbuf);
}

