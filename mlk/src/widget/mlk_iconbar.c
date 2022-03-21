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
 * mIconBar
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_iconbar.h"
#include "mlk_pixbuf.h"
#include "mlk_imagelist.h"
#include "mlk_list.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_util.h"

#include "mlk_pv_widget.h"


//------------------

#define _BTT_PADDING  4		//アイコン周りの余白
#define _BTT_SEPW     11	//セパレータの幅
#define _BTT_DROPW    17	//ドロップボタンの幅
#define _SEP_BOTTOM_HEIGHT  5	//下セパレータの高さ

//press
#define _PRESS_BUTTON  1
#define _PRESS_DROP    2

#define _TIMERID_TOOLTIP_DELAY 0

#define _IS_VERT(p)    ((p)->ib.fstyle & MICONBAR_S_VERT)
#define _ITEM_TOP(p)   (mIconBarItem *)((p)->ib.list.top)
#define _ITEM_PREV(pi) (mIconBarItem *)((pi)->i.prev)
#define _ITEM_NEXT(pi) (mIconBarItem *)((pi)->i.next)

//------------------

typedef struct _mIconBarItem mIconBarItem;

struct _mIconBarItem
{
	mListItem i;
	int id,
		img,
		tooltip,
		x,y;
	uint16_t w,h;	//ドロップボックスも含む、ボタンの幅と高さ
	uint32_t flags;
};

//------------------

/*-----------------

item_sel
	現在選択されているアイテム (カーソル下にあるアイテム)。
	セパレータ・無効状態のアイテムは除く。

autowrap_width
	自動折り返し時、現在のウィジェットの幅

--------------------*/



//===============================
// sub
//===============================


/* アイテムIDから検索 */

static mIconBarItem *_get_item_by_id(mIconBar *p,int id)
{
	mIconBarItem *pi;

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		if(pi->id == id) return pi;
	}

	return NULL;
}

/* ポインタ位置からアイテム取得 */

static mIconBarItem *_get_item_by_point(mIconBar *p,int x,int y)
{
	mIconBarItem *pi;

	if(x < 0 || y < 0) return NULL;

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		if(pi->x <= x && x < pi->x + pi->w
			&& pi->y <= y && y < pi->y + pi->h)
			return pi;
	}

	return NULL;
}

/* アイテムのチェックがONの状態か */

static mlkbool _is_item_checked(mIconBarItem *pi)
{
	return ((pi->flags & (MICONBAR_ITEMF_CHECKBUTTON | MICONBAR_ITEMF_CHECKGROUP))
		&& (pi->flags & MICONBAR_ITEMF_CHECKED));
}

/* ボタンの通常サイズ取得
 *
 * アイコンと余白を付けた１つのサイズ */

static void _get_button_normal_size(mIconBar *p,mSize *dst)
{
	mImageList *img = p->ib.imglist;
	int w,h;

	if(!img)
		w = h = 16;
	else
	{
		w = img->eachw;
		h = img->h;
	}

	dst->w = w + _BTT_PADDING * 2;
	dst->h = h + _BTT_PADDING * 2;
}

/* 各アイテムのサイズ取得
 *
 * dst: あからじめ通常ボタンのサイズを入れておくこと */

static void _get_item_size(mIconBar *p,mIconBarItem *pi,mSize *dst)
{
	int w,h;

	w = dst->w;
	h = dst->h;

	//セパレータ

	if(pi->flags & MICONBAR_ITEMF_SEP)
	{
		if(_IS_VERT(p))
			h = _BTT_SEPW;
		else
			w = _BTT_SEPW;
	}

	//ドロップボタン

	if(pi->flags & MICONBAR_ITEMF_DROPDOWN)
		w += _BTT_DROPW;

	dst->w = w;
	dst->h = h;
}

/* 各アイテムの位置とサイズ情報をセット */

static void _set_item_info(mIconBar *p)
{
	mIconBarItem *pi;
	int x,y,padding;
	mlkbool is_vert,is_autowrap;
	mSize szbtt,sznorm;

	is_vert = (_IS_VERT(p) != 0);
	is_autowrap = (!is_vert && (p->ib.fstyle & MICONBAR_S_AUTO_WRAP));

	_get_button_normal_size(p, &sznorm);

	//

	padding = p->ib.padding;
	x = y = padding;

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		//サイズ取得
		
		szbtt = sznorm;
		_get_item_size(p, pi, &szbtt);

		//[水平自動折り返し時] 幅を超える場合は折り返す

		if(is_autowrap
			&& x != padding
			&& x + szbtt.w + padding > p->wg.w)
		{
			x = padding;
			y += szbtt.h;
		}

		//セット
	
		pi->x = x;
		pi->y = y;
		pi->w = szbtt.w;
		pi->h = szbtt.h;

		//次の位置へ

		if(is_vert)
			y += szbtt.h;
		else
		{
			if(pi->flags & MICONBAR_ITEMF_WRAP)
			{
				//手動折り返し
				x = padding;
				y += szbtt.h;
			}
			else
				x += szbtt.w;
		}
	}
}

/* レイアウトサイズ取得
 *
 * areaw: レイアウトの最大幅。0 で、自動折り返しでない。 */

static void _get_layout_size(mIconBar *p,int areaw,mSize *dst)
{
	mIconBarItem *pi;
	mSize szbtt,sznorm;
	int w,maxw,maxh,padding;
	mlkbool is_vert,is_autowrap;

	is_vert = (_IS_VERT(p) != 0);
	is_autowrap = (!is_vert && (p->ib.fstyle & MICONBAR_S_AUTO_WRAP));
	padding = p->ib.padding;

	_get_button_normal_size(p, &sznorm);

	maxw = maxh = w = 0;

	if(!is_vert) maxh = sznorm.h;

	areaw -= padding * 2;
	if(areaw < 1) areaw = 1;

	//計算

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		//アイテムサイズ
		
		szbtt = sznorm;
		_get_item_size(p, pi, &szbtt);

		//自動折り返し時、幅を超える場合は折り返し

		if(is_autowrap && w && w + szbtt.w > areaw)
		{
			w = 0;
			maxh += szbtt.h;
		}

		//
		
		if(is_vert)
		{
			//垂直
			
			if(maxw < szbtt.w) maxw = szbtt.w;
			maxh += szbtt.h;
		}
		else
		{
			//水平
			
			w += szbtt.w;
			if(maxw < w) maxw = w;

			if(pi->flags & MICONBAR_ITEMF_WRAP)
			{
				w = 0;
				maxh += szbtt.h;
			}
		}
	}

	//下セパレータ

	if(p->ib.fstyle & MICONBAR_S_SEP_BOTTOM)
		maxh += _SEP_BOTTOM_HEIGHT;

	//

	dst->w = maxw + padding * 2;
	dst->h = maxh + padding * 2;
}

/* チェックグループをすべて OFF にする
 *
 * item 自体は変更されない */

static void _checkgroup_off(mIconBarItem *item)
{
	mIconBarItem *p;

	//前方向 OFF

	for(p = _ITEM_PREV(item); p; p = _ITEM_PREV(p))
	{
		if(!(p->flags & MICONBAR_ITEMF_CHECKGROUP)) break;

		p->flags &= ~MICONBAR_ITEMF_CHECKED;
	}

	//後方向 OFF

	for(p = _ITEM_NEXT(item); p; p = _ITEM_NEXT(p))
	{
		if(!(p->flags & MICONBAR_ITEMF_CHECKGROUP)) break;

		p->flags &= ~MICONBAR_ITEMF_CHECKED;
	}
}

/* ツールチップ表示 */

static void _show_tooltip(mIconBar *p,mIconBarItem *item)
{
	mBox box;
	uint32_t flags;

	box.x = item->x;
	box.y = item->y;
	box.w = item->w;
	box.h = item->h;

	if(_IS_VERT(p))
		flags = MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_GRAVITY_LEFT | MPOPUP_F_GRAVITY_BOTTOM | MPOPUP_F_FLIP_X;
	else
		flags = MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_GRAVITY_RIGHT | MPOPUP_F_GRAVITY_TOP | MPOPUP_F_FLIP_XY;

	p->ib.tooltip = mTooltipShow(MLK_TOOLTIP(p->ib.tooltip),
		MLK_WIDGET(p), 0, 0, &box, flags,
		MLK_TR2(p->ib.trgroup, item->tooltip), 0);
}

/* ツールチップを消す */

static void _end_tooltip(mIconBar *p)
{
	mWidgetTimerDelete(MLK_WIDGET(p), _TIMERID_TOOLTIP_DELAY);

	mWidgetDestroy(MLK_WIDGET(p->ib.tooltip));
	p->ib.tooltip = NULL;
}

/* ツールチップ表示準備 */

static void _ready_tooltip(mIconBar *p,mIconBarItem *item)
{
	if(p->ib.fstyle & MICONBAR_S_TOOLTIP)
	{
		if(!item || item->tooltip < 0)
			//消す
			_end_tooltip(p);
		else
		{
			if(p->ib.tooltip)
				//すでに表示されている場合、変更
				_show_tooltip(p, item);
			else
				//タイマーで遅延
				mWidgetTimerAdd(MLK_WIDGET(p), _TIMERID_TOOLTIP_DELAY, 500, 0);
		}
	}
}


//===============================
// ハンドラ
//===============================


/**@ calc_hint ハンドラ関数 */

void mIconBarHandle_calcHint(mWidget *p)
{
	//自動折り返しでない場合、hintW,H = 全ボタン分のサイズ
	
	if(!(MLK_ICONBAR(p)->ib.fstyle & MICONBAR_S_AUTO_WRAP))
	{
		mSize size;

		_get_layout_size(MLK_ICONBAR(p), 0, &size);

		p->hintW = size.w;
		p->hintH = size.h;
	}
}

/** construct ハンドラ関数
 *
 * アイテム増減後、または自動折り返し状態の変更後に、
 * まとめて一度呼ばれる */

static void _construct_handle(mWidget *p)
{
	_set_item_info(MLK_ICONBAR(p));

	mWidgetRedraw(p);
}

/** calc_layout_size ハンドラ関数
 *
 * 自動折り返し時に、動的にレイアウトサイズを指定する。
 * dstw に拡張時の最大幅が入っている。 */

static void _calc_layout_size_handle(mWidget *wg,int *dstw,int *dsth)
{
	mIconBar *p = MLK_ICONBAR(wg);
	mSize size;
	int areaw = *dstw;

	//最大幅に合わせてボタンを配置した時のサイズ

	_get_layout_size(p, areaw, &size);

	//幅が変わった時

	if(p->ib.autowrap_width != size.w)
	{
		p->ib.autowrap_width = size.w;
		mWidgetSetConstruct(wg);
	}

	//幅拡張時は最大幅にセット
	//(セパレータの線を全体に伸ばすため)

	if(wg->flayout & MLF_EXPAND_W)
		size.w = areaw;

	//

	*dstw = size.w;
	*dsth = size.h;
}


//===============================
// main
//===============================


/**@ mIconBar データ解放 */

void mIconBarDestroy(mWidget *wg)
{
	mIconBar *p = (mIconBar *)wg;

	mListDeleteAll(&p->ib.list);

	//イメージリスト破棄

	if(p->ib.fstyle & MICONBAR_S_DESTROY_IMAGELIST)
		mImageListFree(p->ib.imglist);

	//ツールチップウィンドウ破棄

	mWidgetDestroy(MLK_WIDGET(p->ib.tooltip));
}

/**@ 作成 */

mIconBar *mIconBarNew(mWidget *parent,int size,uint32_t fstyle)
{
	mIconBar *p;
	
	if(size < sizeof(mIconBar))
		size = sizeof(mIconBar);
	
	p = (mIconBar *)mWidgetNew(parent, size);
	if(!p) return NULL;
	
	p->wg.destroy = mIconBarDestroy;
	p->wg.draw = mIconBarHandle_draw;
	p->wg.calc_hint = mIconBarHandle_calcHint;
	p->wg.event = mIconBarHandle_event;
	p->wg.construct = _construct_handle;

	if(fstyle & MICONBAR_S_AUTO_WRAP)
		p->wg.calc_layout_size = _calc_layout_size_handle;
	
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->ib.fstyle = fstyle;
	
	return p;
}

/**@ 作成 */

mIconBar *mIconBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mIconBar *p;

	p = mIconBarNew(parent, 0, fstyle);
	if(!p) return NULL;

	__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);
	
	return p;
}

/**@ ウィジェットとボタンの間の余白幅をセット */

void mIconBarSetPadding(mIconBar *p,int padding)
{
	p->ib.padding = padding;
}

/**@ イメージリストセット
 *
 * @d:元のイメージからサイズが変わった場合は、
 * サイズ変更の影響を受ける最上位の親ウィジェットを再レイアウトすること。 */

void mIconBarSetImageList(mIconBar *p,mImageList *img)
{
	p->ib.imglist = img;

	if(p->wg.fui & MWIDGET_UI_LAYOUTED)
	{
		mWidgetSetRecalcHint(MLK_WIDGET(p));
		mWidgetSetConstruct(MLK_WIDGET(p));
	}
}

/**@ ツールチップの翻訳文字列のグループIDセット */

void mIconBarSetTooltipTrGroup(mIconBar *p,uint16_t gid)
{
	p->ib.trgroup = gid;
}


//==============================
// アイテム関連
//==============================


/**@ ボタンをすべて削除 */

void mIconBarDeleteAll(mIconBar *p)
{
	mListDeleteAll(&p->ib.list);

	mWidgetRedraw(MLK_WIDGET(p));
}

/**@ ボタン追加
 *
 * @p:id アイテム ID
 * @p:img イメージのインデックス番号 (負の値でなし)
 * @p:tooltip ツールチップの文字列ID (負の値でなし)
 * @p:flags  フラグ */

void mIconBarAdd(mIconBar *p,int id,int img,int tooltip,uint32_t flags)
{
	mIconBarItem *pi;

	pi = (mIconBarItem *)mListAppendNew(&p->ib.list, sizeof(mIconBarItem));
	if(pi)
	{
		pi->id = id;
		pi->img = img;
		pi->tooltip = tooltip;
		pi->flags = flags;

		mWidgetSetConstruct(MLK_WIDGET(p));
	}
}

/**@ セパレータ追加 */

void mIconBarAddSep(mIconBar *p)
{
	mIconBarAdd(p, -1, 0, -1, MICONBAR_ITEMF_SEP);
}

/**@ アイテムのチェック状態をセット
 *
 * @p:type 正:ON、0:OFF、負:反転 */

void mIconBarSetCheck(mIconBar *p,int id,int type)
{
	mIconBarItem *pi = _get_item_by_id(p, id);

	if(pi
		&& (pi->flags & (MICONBAR_ITEMF_CHECKBUTTON | MICONBAR_ITEMF_CHECKGROUP))
		&& mGetChangeFlagState(type, pi->flags & MICONBAR_ITEMF_CHECKED, &type))
	{
		//チェックグループで ON に変更時、他のアイテムは OFF にする

		if((pi->flags & MICONBAR_ITEMF_CHECKGROUP) && type)
			_checkgroup_off(pi);

		pi->flags ^= MICONBAR_ITEMF_CHECKED;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ アイテムがチェックされているか */

mlkbool mIconBarIsChecked(mIconBar *p,int id)
{
	mIconBarItem *pi = _get_item_by_id(p, id);

	if(pi)
		return ((pi->flags & MICONBAR_ITEMF_CHECKED) != 0);
	else
		return FALSE;
}

/**@ アイテムの有効/無効セット
 *
 * @p:type 0:無効、正:有効、負:反転 */

void mIconBarSetEnable(mIconBar *p,int id,int type)
{
	mIconBarItem *pi = _get_item_by_id(p, id);

	if(pi
		&& mIsChangeFlagState(type, !(pi->flags & MICONBAR_ITEMF_DISABLE)) )
	{
		pi->flags ^= MICONBAR_ITEMF_DISABLE;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ アイテムのボタンの位置とサイズ取得
 *
 * @d:mIconBar からの相対位置。 */

mlkbool mIconBarGetItemBox(mIconBar *p,int id,mBox *box)
{
	mIconBarItem *pi = _get_item_by_id(p, id);

	if(!pi)
		return FALSE;
	else
	{
		box->x = pi->x;
		box->y = pi->y;
		box->w = pi->w;
		box->h = pi->h;
		return TRUE;
	}
}


//========================
// ハンドラ sub
//========================


/* ポインタ移動時 */

static void _event_motion(mIconBar *p,int x,int y)
{
	mIconBarItem *pi;

	//アイテム取得
	//[!] ダイアログ中は、他のウィンドウ上の mIconBar の移動処理を行わない

	if(mWindowIsOtherThanModal(p->wg.toplevel))
		pi = NULL;
	else
		pi = _get_item_by_point(p, x, y);

	//セパレータ上の場合はなし
	//(無効状態でもツールチップは表示するので選択する)

	if(pi && (pi->flags & MICONBAR_ITEMF_SEP))
		pi = NULL;

	//変更

	if(pi != p->ib.item_sel)
	{
		p->ib.item_sel = pi;

		mWidgetRedraw(MLK_WIDGET(p));

		//ツールチップ

		_ready_tooltip(p, pi);
	}
}

/* 左ボタン押し時 */

static void _event_press(mIconBar *p,mEvent *ev)
{
	mIconBarItem *pi;

	pi = p->ib.item_sel;

	//アイテム無効時

	if(pi->flags & MICONBAR_ITEMF_DISABLE) return;

	//ツールチップ消去

	_end_tooltip(p);

	//

	if((pi->flags & MICONBAR_ITEMF_DROPDOWN)
		&& ev->pt.x >= pi->x + pi->w - _BTT_DROPW)
		//---- ドロップボタン側
		p->ib.press = _PRESS_DROP;
	else
	{
		//---- アイコンボタン側
	
		p->ib.press = _PRESS_BUTTON;

		//チェック処理

		if(pi->flags & (MICONBAR_ITEMF_CHECKBUTTON | MICONBAR_ITEMF_CHECKGROUP))
		{
			if(pi->flags & MICONBAR_ITEMF_CHECKGROUP)
			{
				_checkgroup_off(pi);

				pi->flags |= MICONBAR_ITEMF_CHECKED;
			}
			else
				pi->flags ^= MICONBAR_ITEMF_CHECKED;
		}
	}

	mWidgetRedraw(MLK_WIDGET(p));
	mWidgetGrabPointer(MLK_WIDGET(p));
}

/* グラブ解放
 *
 * ev: NULL 以外で、ボタンが離された時 */

static void _release_grab(mIconBar *p,mEventPointer *ev)
{
	if(p->ib.press)
	{
		//通知

		if(ev)
		{
			mWidgetEventAdd_command(mWidgetGetNotifyWidget(MLK_WIDGET(p)),
				(p->ib.item_sel)->id, 0,
				(p->ib.press == _PRESS_DROP)
					? MEVENT_COMMAND_FROM_ICONBAR_DROP: MEVENT_COMMAND_FROM_ICONBAR_BUTTON,
				p);
		}

		//

		mWidgetUngrabPointer();
		mWidgetRedraw(MLK_WIDGET(p));

		p->ib.press = 0;

		//ボタン離し時はその位置で判定し直し

		if(ev)
			_event_motion(p, ev->x, ev->y);
		else
			_event_motion(p, -1, -1);
	}
}


//========================
// ハンドラ
//========================


/**@ イベントハンドラ関数 */

int mIconBarHandle_event(mWidget *wg,mEvent *ev)
{
	mIconBar *p = MLK_ICONBAR(wg);

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(!p->ib.press)
					_event_motion(p, ev->pt.x, ev->pt.y);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS ||
				ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && !p->ib.press && p->ib.item_sel)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し

				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p, (mEventPointer *)ev);
			}
			break;

		//ENTER
		case MEVENT_ENTER:
			_event_motion(p, ev->enter.x, ev->enter.y);
			break;
		//LEAVE
		case MEVENT_LEAVE:
			_event_motion(p, -1, -1);
			break;

		//タイマー (ツールチップ遅延)
		case MEVENT_TIMER:
			if(p->ib.item_sel)
				_show_tooltip(p, p->ib.item_sel);

			mWidgetTimerDelete(wg, ev->timer.id);
			break;

		//FOCUS
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p, NULL);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

/**@ draw ハンドラ関数 */

void mIconBarHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mIconBar *p = MLK_ICONBAR(wg);
	mIconBarItem *pi;
	mImageList *imglist;
	int x,y,w,h,iconw;
	mPixCol col_frame;
	mlkbool is_disable, is_sel, is_press, wg_disable, is_bttframe;

	imglist = p->ib.imglist;
	wg_disable = !mWidgetIsEnable(wg);
	is_bttframe = ((p->ib.fstyle & MICONBAR_S_BUTTON_FRAME) != 0);

	col_frame = MGUICOL_PIX(FRAME);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FACE));

	//ボタン

	for(pi = _ITEM_TOP(p); pi; pi = _ITEM_NEXT(pi))
	{
		x = pi->x, y = pi->y;
		w = pi->w, h = pi->h;

		if(pi->flags & MICONBAR_ITEMF_SEP)
		{
			//[セパレータ]

			if(_IS_VERT(p))
				mPixbufLineH(pixbuf, x, y + h / 2, w, col_frame);
			else
				mPixbufLineV(pixbuf, x + w / 2, y, h, col_frame);
		}
		else
		{
			//[ボタン]

			is_sel = (pi == p->ib.item_sel);
			is_disable = ((pi->flags & MICONBAR_ITEMF_DISABLE) || wg_disable);
			is_press = ((is_sel && p->ib.press == _PRESS_BUTTON) || _is_item_checked(pi));

			iconw = w;
			if(pi->flags & MICONBAR_ITEMF_DROPDOWN) iconw -= _BTT_DROPW;

			if(!is_disable)
			{
				//背景
				
				if(is_press)
					//アイコンボタン押し時
					mPixbufFillBox(pixbuf, x, y, iconw, h, MGUICOL_PIX(BUTTON_FACE_PRESS));
				else if(is_sel)
					//選択時
					mPixbufFillBox(pixbuf, x, y, iconw, h, MGUICOL_PIX(BUTTON_FACE_DEFAULT));
			}

			//枠

			if((is_sel || is_press) && !is_disable)
				//選択 or 押し時
				mPixbufBox(pixbuf, x, y, iconw, h, (is_sel)? MGUICOL_PIX(FRAME_FOCUS): col_frame);
			else if(is_bttframe)
				//ボタン枠
				mPixbufBox(pixbuf, x, y, iconw, h, col_frame);

			//アイコン

			mImageListPutPixbuf(imglist, pixbuf,
				x + _BTT_PADDING + is_press, y + _BTT_PADDING + is_press,
				pi->img,
				(is_disable)? MIMAGELIST_PUTF_DISABLE: 0);

			//--- ドロップボックス

			if(pi->flags & MICONBAR_ITEMF_DROPDOWN)
			{
				x += iconw;

				is_press = (is_sel && p->ib.press == _PRESS_DROP);

				//背景

				if(!is_disable)
				{
					if(is_press)
						mPixbufFillBox(pixbuf, x, y, _BTT_DROPW, h, MGUICOL_PIX(BUTTON_FACE_PRESS));
					else if(is_sel)
						mPixbufFillBox(pixbuf, x, y, _BTT_DROPW, h, MGUICOL_PIX(BUTTON_FACE_DEFAULT));
				}

				//枠
				
				if((is_sel && !is_disable) || is_bttframe)
					mPixbufBox(pixbuf, x, y, _BTT_DROPW, h, col_frame);

				//矢印

				mPixbufDrawArrowDown(pixbuf,
					x + (_BTT_DROPW - 7) / 2 + is_press, y + (h - 4) / 2 + is_press,
					4,
					(is_disable)? MGUICOL_PIX(TEXT_DISABLE): MGUICOL_PIX(TEXT));
			}
		}
	}

	//下セパレータ

	if(p->ib.fstyle & MICONBAR_S_SEP_BOTTOM)
		mPixbufLineH(pixbuf, 0, wg->h - 3, wg->w, col_frame);
}

