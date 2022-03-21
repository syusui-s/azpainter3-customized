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
 * mWidget 内部関数
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_rectbox.h"
#include "mlk_key.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_widget.h"
#include "mlk_pv_window.h"


//==========================
// mWidgetRect
//==========================


/** mWidgetRect に同じ値をセット */

void __mWidgetRectSetSame(mWidgetRect *p,int val)
{
	p->left = p->top = p->right = p->bottom = val;
}

/** mWidgetRect に４つの値セット (left,top,right,bottom) */

void __mWidgetRectSetPack4(mWidgetRect *p,uint32_t val)
{
	p->left   = (uint8_t)(val >> 24);
	p->top    = (uint8_t)(val >> 16);
	p->right  = (uint8_t)(val >> 8);
	p->bottom = (uint8_t)val;
}


//==========================
// mWidget
//==========================


/** ウィジェット作成時の初期化 */

void __mWidgetCreateInit(mWidget *p,int id,uint32_t flayout,uint32_t margin_pack)
{
	p->id = id;
	p->flayout |= flayout;
	//作成時にデフォルトで付くフラグがある場合があるため、OR

	mWidgetSetMargin_pack4(p, margin_pack);
}


//==========================
// ツリー/リスト
//==========================


/** ツリーの次のウィジェット取得 (全体が対象)
 * 
 * return: NULL で終了 */

mWidget *__mWidgetGetTreeNext(mWidget *p)
{
	if(p->first)
		return p->first;
	else
	{
		do
		{
			if(p->next)
				return p->next;
			else
				p = p->parent;
		} while(p);

		return p;
	}
}

/** ツリーの次のウィジェット取得
 * 
 * p の子はスキップする。 */

mWidget *__mWidgetGetTreeNextPass(mWidget *p)
{
	do{
		if(p->next)
			return p->next;
		else
			p = p->parent;
	} while(p);

	return p;
}

/** ツリーの次のウィジェット取得
 * 
 * p の子はスキップする。root 以下のみ取得。 */

mWidget *__mWidgetGetTreeNextPass_root(mWidget *p,mWidget *root)
{
	if(p == root) return NULL;
	
	do{
		if(p->next)
			return p->next;
		else
		{
			p = p->parent;
			if(p == root) return NULL;
		}
	}while(p);
	
	return p;
}

/** ツリーの次のウィジェット取得
 * 
 * root 以下のみ。p の子も含む。
 *
 * p: NULL で root から開始 */

mWidget *__mWidgetGetTreeNext_root(mWidget *p,mWidget *root)
{
	if(!p) p = root;

	if(p->first)
		return p->first;
	else
		return __mWidgetGetTreeNextPass_root(p, root);
}

/** 指定方向に指定数進むか、戻った先のウィジェットを取得 (同じ親内)
 *
 * グリッドレイアウトの移動などで使う。 */

mWidget *__mWidgetGetListSkipNum(mWidget *p,int dir)
{
	if(dir > 0)
	{
		//進む
		for(; p && dir > 0; dir--, p = p->next);
	}
	else
	{
		//戻る
		for(; p && dir < 0; dir++, p = p->prev);
	}

	return p;
}


//==========================
// UI 関連
//==========================


/** p を含む親方向すべてで UI フラグを ON にする
 *
 * ルートウィジェットも含む。 */

void __mWidgetSetUIFlags_upper(mWidget *p,uint32_t flags)
{
	for(; p; p = p->parent)
		p->fui |= flags;
}

/** p に対する描画フラグを ON にする (ウィジェット再描画時) */

void __mWidgetSetUIFlags_draw(mWidget *p)
{
	mWidget *p2;

	//[!] p の draw_rect は変更しないため、以下のループで処理しない
	p->fui |= MWIDGET_UI_FOLLOW_DRAW | MWIDGET_UI_DRAW;

	//親方向に FOLLOW フラグ ON
	
	__mWidgetSetUIFlags_upper(p, MWIDGET_UI_FOLLOW_DRAW);

	//子ウィジェットをすべて再描画

	p2 = p;

	while(1)
	{
		p2 = __mWidgetGetTreeNext_root(p2, p);
		if(!p2) break;
		
		p2->fui |= MWIDGET_UI_FOLLOW_DRAW | MWIDGET_UI_DRAW;

		//ウィジェットの描画範囲を全体へ
		mRectSetBox_d(&p2->draw_rect, 0, 0, p2->w, p2->h);
	}
}

/** UI フラグが ON になっている一番先頭のウィジェットを取得
 * 
 * p: NULL でルートから
 * follow_mask: 辿るフラグ (ON の場合のみ下位を辿る)
 * run_mask: 実行用のフラグ (ON になっていれば、それが返る) */

mWidget *__mWidgetGetTreeNext_uiflag(mWidget *p,
		uint32_t follow_mask,uint32_t run_mask)
{
	if(!p) p = MLKAPP->widget_root;
	
	while(1)
	{
		//次へ

		if(p->fui & follow_mask)
		{
			p->fui &= ~follow_mask;

			p = __mWidgetGetTreeNext(p);
		}
		else
			p = __mWidgetGetTreeNextPass(p);
		
		if(!p) break;
		
		//

		if(p->fui & run_mask) break;
	}
	
	return p;
}

/** UI の辿るフラグが ON で、かつ一番下位にあるウィジェットを取得
 *
 * - mGuiCalcHintSize() やウィジェットの削除時、下位から順に実行させたい時に使う。
 * - フラグ値は外さない。
 *
 * p: NULL の場合はルートウィジェットから。
 *    それ以外は、ルートとなるウィジェット (ツリーから取り外されたウィジェットなど)。
 * return: 見つからなければ NULL */

mWidget *__mWidgetGetTreeBottom_uiflag(mWidget *p,uint32_t follow_mask)
{
	mWidget *p2,*root;

	root = p;
	if(!p) p = MLKAPP->widget_root;

	while(1)
	{
		//最後の子から順にフラグが ON のものを探し、あればその子をさらに検索。
		//子の中で見つからなければ、親が対象。

		for(p2 = p->last; p2 && !(p2->fui & follow_mask); p2 = p2->prev);

		if(!p2) break;

		p = p2;
	}

	//ルートからの検索時、ルート以外にフラグが ON のものがない場合は、なしとする
	
	if(!root && p == MLKAPP->widget_root)
		return NULL;

	return p;
}

/** UI フラグが ON になっている、一つ前のウィジェットを取得
 *
 * - 下位から順に処理していく時の、次のウィジェット取得時に使う。
 * - p を含め、辿る時に follow_mask は OFF にセットされる。
 *   (戻り値のウィジェットでは ON のまま)
 * - run_mask のフラグはそのまま (実際の実行時に OFF にする)。
 * - 最後はルートウィジェットの follow_mask が OFF になって、NULL が返る。 */

mWidget *__mWidgetGetTreePrev_uiflag(mWidget *p,
	uint32_t follow_mask,uint32_t run_mask)
{
	mWidget *p2;

	while(1)
	{
		p->fui &= ~follow_mask;

		//前方の follow ウィジェット検索

		for(p2 = p->prev; p2 && !(p2->fui & follow_mask); p2 = p2->prev);

		if(!p2)
		{
			//ない場合は親へ

			p = p->parent;
			if(!p) break;
		}
		else
		{
			//子の一番下位の follow ウィジェット検索。
			//ない場合は、親自体を対象とする。

			p = p2;
			
			while(1)
			{
				for(p2 = p->last; p2 && !(p2->fui & follow_mask); p2 = p2->prev);

				if(!p2) break;

				p = p2;
			}
		}

		if(p->fui & run_mask) break;
	}
	
	return p;
}

/** 次の描画対象ウィジェット取得
 *
 * - mGuiUpdateWidgets() 時に使われる。
 * - 上位ウィジェットから順に検索する。
 * - 非表示状態の場合はスキップ。
 * - MWIDGET_UI_FOLLOW_DRAW で対象を辿り、MWIDGET_UI_DRAW が描画対象。
 * - 辿る際に MWIDGET_UI_FOLLOW_DRAW は OFF になる。
 *   (戻り値のウィジェットのフラグはそのまま)
 * - 戻り値のウィジェットの MWIDGET_UI_DRAW は OFF になる。
 * - draw ハンドラで __mWidgetDrawBkgnd() による背景描画が行われた時、
 *   ウィジェット全体が描画された場合は、MWIDGET_UI_DRAWED_BKGND が ON になっている。
 *   (これにより、その下位では背景描画を行う必要がなくなる)
 *   次を検索する段階で、不要になった MWIDGET_UI_DRAWED_BKGND は OFF にしていく。
 * 
 * param: p  NULL で先頭から */

mWidget *__mWidgetGetTreeNext_draw(mWidget *p)
{
	int follow;

	if(!p) p = MLKAPP->widget_root;
	
	while(1)
	{
		//---- p をスキップして、次の辿るウィジェットを取得
		//---- また、同時に、不要になった MWIDGET_UI_DRAWED_BKGND フラグを OFF にする。

		//下位を辿るか
	
		follow = p->fui & MWIDGET_UI_FOLLOW_DRAW;

		p->fui &= ~MWIDGET_UI_FOLLOW_DRAW;
	
		//次へ

		if(follow && p->first)
			//辿る＆子がある場合は、子へ
			//(親の MWIDGET_UI_DRAWED_BKGND は必要なので、残す)
			p = p->first;
		else
		{
			//辿らない or 子がない場合、next か親へ
			//(子が描画対象でない場合、親 (=p) の MWIDGET_UI_DRAWED_BKGND は必要ないので、消す)

			p->fui &= ~MWIDGET_UI_DRAWED_BKGND;
			
			while(1)
			{
				if(p->next)
				{
					p = p->next;
					break;
				}
				else
				{
					//終端なら親へ
					//(p の子はすべて処理したということなので、p の MWIDGET_UI_DRAWED_BKGND は消す)
					
					p = p->parent;
					if(!p) return NULL;

					p->fui &= ~MWIDGET_UI_DRAWED_BKGND;
				}
			}
		}
		
		//---- 処理対象か

		if(p->fui & MWIDGET_UI_DRAW)
		{
			p->fui &= ~MWIDGET_UI_DRAW;

			if(mWidgetIsVisible(p)) break;

			//非表示の場合は検索を続ける
		}
	}
	
	return p;
}

/** ウィジェット全体の範囲を、ウィンドウ座標でクリッピングして取得
 *
 * ウィンドウの表示範囲 & 親ウィジェットの領域でクリッピングされる。
 *
 * ptoffset: NULL 以外で、ウィジェットの左上位置が入る (描画時のオフセット位置)
 * root: TRUE で、ウィンドウ装飾を含む座標
 * return: FALSE でウィンドウ範囲外 */

mlkbool __mWidgetGetWindowRect(mWidget *p,mRect *dst,mPoint *ptoffset,mlkbool root)
{
	mRect rc;
	mWindow *win;

	win = p->toplevel;

	//ウィジェット全体の範囲

	rc.x1 = p->absX;
	rc.y1 = p->absY;
	rc.x2 = rc.x1 + p->w - 1;
	rc.y2 = rc.y1 + p->h - 1;

	//左上位置

	if(ptoffset)
	{
		ptoffset->x = p->absX;
		ptoffset->y = p->absY;

		if(root)
		{
			ptoffset->x += win->win.deco.w.left;
			ptoffset->y += win->win.deco.w.top;
		}
	}

	//親の範囲を適用していく
	//(最終的に、ウィンドウの範囲でクリッピングされる)

	for(p = p->parent; p->parent; p = p->parent)
	{
		if(!mRectClipBox_d(&rc, p->absX, p->absY, p->w, p->h))
			return FALSE;
	}

	//ウィンドウ装飾含む範囲
	//(実際のウィンドウ範囲でクリッピング)

	if(root)
	{
		mRectMove(&rc, win->win.deco.w.left, win->win.deco.w.top);

		if(!mRectClipBox_d(&rc, 0, 0, win->win.win_width, win->win.win_height))
			return FALSE;
	}

	*dst = rc;

	return TRUE;
}


//============================
// レイアウト関連
//============================


/** リサイズ時の処理
 *
 * mWidgetResize() と CONFIGURE イベント時 (ウィンドウのサイズ変更時) */

void __mWidgetOnResize(mWidget *p)
{
	//resize ハンドラはレイアウトの前に実行する。
	//子アイテムをレイアウトする前に、レイアウト状態を変更したい場合があるため。

	if(p->resize)
		(p->resize)(p);

	//固定サイズの場合、親の推奨サイズを再計算対象に

	if(p->flayout & MLF_FIX_WH)
		mWidgetSetRecalcHint(p);

	//レイアウト

	mWidgetLayout(p);

	mWidgetRedraw(p);
}

/** レイアウト時の位置とサイズをセット
 *
 * mWidgetMoveResize() 時、レイアウト中の場合に使われる。
 *
 * return: サイズが変わったか */

mlkbool __mWidgetLayoutMoveResize(mWidget *p,int x,int y,int w,int h)
{
	mlkbool resize;

	p->x = x;
	p->y = y;

	//絶対位置
	// :下位ウィジェットは、上位から順番にすべてレイアウトが実行されるので、
	// :下位の絶対位置をここでセットする必要はない。
	
	p->absX = p->parent->absX + x;
	p->absY = p->parent->absY + y;

	//サイズ

	if(w < 1) w = 1;
	if(h < 1) h = 1;

	resize = (w != p->w || h != p->h);

	p->w = w;
	p->h = h;

	p->fui |= MWIDGET_UI_LAYOUTED;

	//resize ハンドラ

	if(resize && p->resize)
		(p->resize)(p);

	return resize;
}

/** レイアウトサイズを計算する
 *
 * mGuiCalcHintSize() 時と、コンテナの推奨サイズ計算時に使う。
 * MWIDGET_UI_CALC フラグは OFF にセットされる。 */

void __mWidgetCalcHint(mWidget *p)
{
	if(p->fui & MWIDGET_UI_CALC)
	{
		if(p->calc_hint)
			//通常ハンドラ
			(p->calc_hint)(p);
		else if(p->ftype & MWIDGET_TYPE_CONTAINER)
			//コンテナ時: calc_hint が NULL の場合は、コンテナ用の関数を使う
			(MLK_CONTAINER(p)->ct.calc_hint)(p);

		p->fui &= ~MWIDGET_UI_CALC;
	}
}

/** レイアウト時の幅取得 */

int __mWidgetGetLayoutW(mWidget *p)
{
	if(p->flayout & MLF_FIX_W)
		//固定
		return p->w;
	else if(p->initW && !(p->fui & MWIDGET_UI_LAYOUTED))
		//初期サイズ (レイアウト未実行時)
		return p->initW;
	else if(p->hintRepW)
		//hint 置き換え
		return p->hintRepW;
	else if(p->hintMinW)
		//hint 最小値補正
		return (p->hintW < p->hintMinW)? p->hintMinW: p->hintW;
	else
		//hint
		return p->hintW;
}

/** レイアウト時の高さ取得 */

int __mWidgetGetLayoutH(mWidget *p)
{
	if(p->flayout & MLF_FIX_H)
		return p->h;
	else if(p->initH && !(p->fui & MWIDGET_UI_LAYOUTED))
		return p->initH;
	else if(p->hintRepH)
		return p->hintRepH;
	else if(p->hintMinH)
		return (p->hintH < p->hintMinH)? p->hintMinH: p->hintH;
	else
		return p->hintH;
}

/** 余白を含めたレイアウト幅取得 */

int __mWidgetGetLayoutW_space(mWidget *p)
{
	return __mWidgetGetLayoutW(p) + p->margin.left + p->margin.right;
}

/** 余白を含めたレイアウト高さ取得 */

int __mWidgetGetLayoutH_space(mWidget *p)
{
	return __mWidgetGetLayoutH(p) + p->margin.top + p->margin.bottom;
}

/** 推奨サイズ計算時の、推奨サイズと初期サイズを取得 */

void __mWidgetGetLayoutCalcSize(mWidget *p,mSize *hint,mSize *init)
{
	int hs,is,margin;

	//hs = hint size, is = init size

	//------- 幅

	margin = p->margin.left + p->margin.right;

	if(p->flayout & MLF_FIX_W)
		//固定
		hs = is = p->w;
	else
	{
		//初期サイズは、0 なら推奨サイズ。
		//推奨サイズは、置き換えが 0 以外ならその値。
		
		is = p->initW;
		hs = p->hintW;

		if(p->hintRepW)
			hs = p->hintRepW;
		else if(p->hintMinW && hs < p->hintMinW)
			hs = p->hintMinW;

		if(!is) is = hs;
	}

	hint->w = hs + margin;
	init->w = is + margin;

	//------- 高さ

	margin = p->margin.top + p->margin.bottom;

	if(p->flayout & MLF_FIX_H)
		hs = is = p->h;
	else
	{
		is = p->initH;
		hs = p->hintH;

		if(p->hintRepH)
			hs = p->hintRepH;
		else if(p->hintMinH && hs < p->hintMinH)
			hs = p->hintMinH;

		if(!is) is = hs;
	}

	hint->h = hs + margin;
	init->h = is + margin;
}


//===============================
// 描画関連
//===============================


/** mPixbuf に背景を描画
 *
 * 描画は親から順に行われるため、親で背景全体が描画されていれば、
 * 子では背景を描画しなくて済む。
 * 
 * box:   描画範囲。NULL でウィジェット全体。
 * force: 親によって背景が描画されていても強制的に描画 (クリアする時など) */

void __mWidgetDrawBkgnd(mWidget *wg,mBox *box,mlkbool force)
{
	mPixbuf *pixbuf = wg->toplevel->win.pixbuf;
	mBox box1;
	mPoint pt;
	mWidget *p;
	
	//描画範囲 (mPixbuf の絶対位置)
	
	if(box)
		box1 = *box;
	else
	{
		box1.x = box1.y = 0;
		box1.w = wg->w, box1.h = wg->h;
	}
	
	box1.x += wg->toplevel->win.deco.w.left + wg->absX;
	box1.y += wg->toplevel->win.deco.w.top  + wg->absY;
	
	//draw_bkgnd ハンドラが NULL なら親の値を使うため、
	//親方向に検索

	for(p = wg; p; p = p->parent)
	{
		//<背景を描画しない>
		// draw_bkend == NULL は親と同じ背景を描画するため、
		// 背景描画なしはフラグで指定する。
		
		if(p->foption & MWIDGET_OPTION_NO_DRAW_BKGND) break;

		//<親によってすでに全体が描画されている>
		// [!] 子が draw_bkgnd == NULL で p が親である状態。
		// draw_bkend が NULL だった場合、背景は親と同じにするということなので、
		// p の親がすでに描画済みの状態であれば、背景描画は行わない。
		// (強制描画時は除く)

		if(!force && (p->fui & MWIDGET_UI_DRAWED_BKGND)) break;

		//背景描画 (NULL なら親を検索)
	
		if(p->draw_bkgnd)
		{
			mPixbufSetOffset(pixbuf, 0, 0, &pt);

			(p->draw_bkgnd)(p, pixbuf, &box1);

			mPixbufSetOffset(pixbuf, pt.x, pt.y, NULL);

			//全体を描画した時、背景描画済みフラグ ON
			//(子がない場合は除く)

			if(!box && wg->first)
				wg->fui |= MWIDGET_UI_DRAWED_BKGND;

			break;
		}
	}
}


//===============================
// フォーカス関連
//===============================


/** 自身にフォーカスがある場合、取り除く
 *
 * ウィジェットの削除時や、
 * 親ウィジェットが無効状態に変化した場合に、フォーカスを解除する時に使う。
 * 
 * FOCUS_OUT イベントは追加される。 */

void __mWidgetRemoveSelfFocus(mWidget *p)
{
	if(p->fstate & MWIDGET_STATE_FOCUSED)
		__mWindowSetFocus(p->toplevel, NULL, MEVENT_FOCUS_FROM_UNKNOWN);
}

/** ウィジェットが非表示/無効状態になった時のフォーカス解除処理
 *
 * p が非表示または無効になった時、p 以下にフォーカスがあれば、
 * フォーカスを解除する。 */

void __mWidgetRemoveFocus_forDisabled(mWidget *p)
{
	mWidget *focus;

	if(!(p->fstate & MWIDGET_STATE_VISIBLE)
		|| !(p->fstate & MWIDGET_STATE_ENABLED))
	{
		focus = p->toplevel->win.focus;

		//ウィンドウにフォーカスがあり、フォーカスが p 以下にある場合
	
		if(focus && mWidgetTreeIsUnder(focus, p))
			__mWidgetRemoveSelfFocus(focus);
	}
}

/** ウィジェットが非表示/無効状態になった時、ENTER 状態であれば、LEAVE 状態にする */

void __mWidgetLeave_forDisable(mWidget *p)
{
	if(MLKAPP->widget_enter == p
		&& (!(p->fstate & MWIDGET_STATE_VISIBLE) || !(p->fstate & MWIDGET_STATE_ENABLED)))
	{
		MLKAPP->widget_enter = NULL;
	
		mEventListAdd_base(p, MEVENT_LEAVE);
	}
}

/** 矢印キーが押された時のフォーカス移動を行う
 *
 * 親コンテナのタイプにより、有効なキーは異なる。
 * フォーカスを受け取り可能な次 or 前のウィジェットへ移動する。
 *
 * p: フォーカスのあるウィジェット
 * return: 移動したか */

mlkbool __mWidgetMoveFocus_arrowkey(mWidget *p,uint32_t key)
{
	mContainer *ct;
	int dir,col;
	uint32_t state;

	//方向

	dir = mKeyGetArrowDir(key);
	if(dir == -1) return FALSE;

	//親のコンテナ

	ct = MLK_CONTAINER(p->parent);

	if(!(ct->wg.ftype & MWIDGET_TYPE_CONTAINER))
		return FALSE;

	//各タイプごとに p に移動先セット

	state = MWIDGET_STATE_VISIBLE | MWIDGET_STATE_ENABLED | MWIDGET_STATE_TAKE_FOCUS;

	switch(ct->ct.type)
	{
		//垂直/水平
		case MCONTAINER_TYPE_VERT:
		case MCONTAINER_TYPE_HORZ:
			//無効キー

			if(ct->ct.type == MCONTAINER_TYPE_HORZ)
			{
				if(dir & 1) return FALSE;
			}
			else
			{
				if(!(dir & 1)) return FALSE;
			}

			//移動 (無効状態のものなどはスキップ)

			do
			{
				p = (dir & 2)? p->next: p->prev;
			} while(p && (p->fstate & state) != state);
		
			break;

		//グリッド
		case MCONTAINER_TYPE_GRID:
			if(!(dir & 1))
			{
				//左右

				do
				{
					p = (dir & 2)? p->next: p->prev;
				} while(p && (p->fstate & state) != state);
			}
			else
			{
				//上下

				col = ct->ct.grid_cols;
				if(!(dir & 2)) col = -col;

				do
				{
					p = __mWidgetGetListSkipNum(p, col);
				} while(p && (p->fstate & state) != state);
			}
			break;

		default:
			return FALSE;
	}

	//移動

	if(p)
		return __mWindowSetFocus(p->toplevel, p, MEVENT_FOCUS_FROM_KEY_MOVE);
	else
		return FALSE;
}
