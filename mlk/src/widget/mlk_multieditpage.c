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
 * mMultiEditPage
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_multiedit.h"
#include "mlk_scrollbar.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_guicol.h"
#include "mlk_str.h"

#include "mlk_multieditpage.h"
#include "mlk_widgettextedit.h"


//--------------------------

#define _PADDING_X  2
#define _PADDING_Y  1

#define _CHECK_STYLE(p,f)  (MLK_MULTIEDIT((p)->wg.parent)->me.fstyle & (f))

static int _event_handle(mWidget *wg,mEvent *ev);
static void _draw_handle(mWidget *wg,mPixbuf *pixbuf);

//--------------------------


//========================
// sub
//========================


/* フォントの行の高さを取得 */

static int _get_line_height(mMultiEditPage *p)
{
	return mWidgetGetFontHeight(MLK_WIDGET(p));
}

/* ポインタイベント(押し/ドラッグ)時、文字位置を取得 */

static int _getcharpos_ptevent(mMultiEditPage *p,mEvent *ev)
{
	mPoint pt;

	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(p), &pt);

	return mWidgetTextEdit_multi_getPosAtPixel(p->mep.pedit,
		ev->pt.x - _PADDING_X + pt.x, ev->pt.y - _PADDING_Y + pt.y,
		_get_line_height(p));
}

/* スクロール位置調整 */

static void _adjust_scroll(mMultiEditPage *p)
{
	mWidgetTextEdit *edit;
	mScrollBar *horz,*vert;
	int scrx,scry,curx,cury,w,h,lineh;
	
	edit = p->mep.pedit;
	lineh = _get_line_height(p);

	mScrollViewPage_getScrollBar(MLK_SCROLLVIEWPAGE(p), &horz, &vert);

	scrx = mScrollBarGetPos(horz);
	scry = mScrollBarGetPos(vert);
	curx = edit->curx;
	cury = edit->multi_curline * lineh;
	w = p->wg.w - _PADDING_X * 2;
	h = p->wg.h - _PADDING_Y * 2 - lineh;

	if(scrx >= edit->maxwidth)
		scrx = 0;
	else if(curx - scrx < 0)
		//カーソル位置が左に隠れている
		scrx = curx;
	else if(curx - scrx >= w)
		//カーソル位置が右に隠れている
		scrx = curx - w + 1;

	if(scry >= edit->multi_maxlines * lineh)
		scry = 0;
	else if(cury - scry < 0)
		scry = cury;
	else if(cury - scry >= h)
		scry = cury - h;

	mScrollBarSetPos(horz, scrx);
	mScrollBarSetPos(vert, scry);
}

/* テキスト変更時 */

static void _on_change_text(mMultiEditPage *p)
{
	mMultiEditPage_setScrollInfo(p);

	mMultiEditPage_onChangeCursorPos(p);
	
	//スクロールバー再構成 (親にセット)

	mWidgetSetConstruct(p->wg.parent);

	//CHANGE 通知

	if(_CHECK_STYLE(p, MMULTIEDIT_S_NOTIFY_CHANGE))
	{
		mWidgetEventAdd_notify(p->wg.parent, NULL,
			MMULTIEDIT_N_CHANGE, 0, 0);
	}
}

/* (mScrollViewPage) スクロールページ値取得ハンドラ */

static void _getscrollpage_handle(mWidget *p,int horz,int vert,mSize *dst)
{
	dst->w = horz - _PADDING_X * 2;
	dst->h = vert - _PADDING_Y * 2;
}


//========================
// main
//========================


/** 作成 */

mMultiEditPage *mMultiEditPageNew(mWidget *parent,int size,mWidgetTextEdit *pedit)
{
	mMultiEditPage *p;
	
	if(size < sizeof(mMultiEditPage))
		size = sizeof(mMultiEditPage);
	
	p = (mMultiEditPage *)mScrollViewPageNew(parent, size);
	if(!p) return NULL;
	
	p->wg.draw = _draw_handle;
	p->wg.event = _event_handle;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;

	p->svp.getscrollpage = _getscrollpage_handle;

	p->mep.pedit = pedit;
	
	return p;
}

/** スクロール情報セット */

void mMultiEditPage_setScrollInfo(mMultiEditPage *p)
{
	mScrollBar *horz,*vert;

	mScrollViewPage_getScrollBar(MLK_SCROLLVIEWPAGE(p), &horz, &vert);

	//水平:+1 はカーソルの分
	mScrollBarSetStatus(horz, 0, p->mep.pedit->maxwidth + 1, p->wg.w - _PADDING_X * 2);

	//垂直
	mScrollBarSetStatus(vert, 0,
		p->mep.pedit->multi_maxlines * _get_line_height(p), p->wg.h - _PADDING_Y * 2);
}

/** カーソル位置変更時 */

void mMultiEditPage_onChangeCursorPos(mMultiEditPage *p)
{
	_adjust_scroll(p);
}

/** テキスト内容の変更時 */

void mMultiEditPage_onChangeText(mMultiEditPage *p)
{
	_on_change_text(p);

	mWidgetRedraw(MLK_WIDGET(p));
}

/** IM 位置取得 (親からの位置) */

void mMultiEditPage_getIMPos(mMultiEditPage *p,mPoint *dst)
{
	mPoint pt;

	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(p), &pt);

	pt.x = p->mep.pedit->curx - pt.x + _PADDING_X;
	pt.y = p->mep.pedit->multi_curline * _get_line_height(p) - pt.y + _PADDING_Y;

	if(pt.x < _PADDING_X) pt.x = _PADDING_X;
	else if(pt.x > p->wg.w) pt.x = p->wg.w;

	if(pt.y < _PADDING_Y) pt.y = _PADDING_Y;
	else if(pt.y > p->wg.h) pt.y = p->wg.h;

	pt.x += p->wg.x;
	pt.y += p->wg.y;

	*dst = pt;
}


//========================
// ハンドラ
//========================


/* キー押し時 */

static void _event_keydown(mMultiEditPage *p,mEvent *ev)
{
	int ret;

	//処理
	
	ret = mWidgetTextEdit_keyProc(p->mep.pedit, ev->key.key, ev->key.state,
		!_CHECK_STYLE(p, MMULTIEDIT_S_READ_ONLY));

	//
	
	if(ret == MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT)
		_on_change_text(p);
	else if(ret == MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR)
		mMultiEditPage_onChangeCursorPos(p);

	//更新
	
	if(ret)
		mWidgetRedraw(MLK_WIDGET(p));
}

/* ボタン押し */

static void _event_press(mMultiEditPage *p,mEvent *ev)
{
	int pos;

	//文字位置

	pos = _getcharpos_ptevent(p, ev);

	//親にフォーカスセット

	mWidgetSetFocus(p->wg.parent);

	//カーソル位置移動 (+Shift で選択範囲拡張)

	mWidgetTextEdit_moveCursorPos(p->mep.pedit, pos, ev->pt.state & MLK_STATE_SHIFT);

	//

	p->mep.fpress = TRUE;
	p->mep.drag_pos = pos;

	mWidgetGrabPointer(MLK_WIDGET(p));
	mWidgetRedraw(MLK_WIDGET(p));
}

/* ポインタ移動 */

static void _event_motion(mMultiEditPage *p,mEvent *ev)
{
	int pos;

	pos = _getcharpos_ptevent(p, ev);

	if(pos != p->mep.drag_pos)
	{
		mWidgetTextEdit_dragExpandSelect(p->mep.pedit, pos);

		p->mep.drag_pos = pos;

		_adjust_scroll(p);
		
		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/* 右クリック時 */

static void _press_rbtt(mWidget *wg,mEvent *ev)
{
	mPoint pt;

	pt.x = ev->pt.x;
	pt.y = ev->pt.y;

	mWidgetMapPoint(wg, wg->parent, &pt);

	mWidgetEventAdd_notify(wg->parent, NULL, MMULTIEDIT_N_R_CLICK, pt.x, pt.y);
}

/* グラブ解放 */

static void _release_grab(mMultiEditPage *p)
{
	if(p->mep.fpress)
	{
		p->mep.fpress = 0;
		mWidgetUngrabPointer();
	}
}

/** イベント */

int _event_handle(mWidget *wg,mEvent *ev)
{
	mMultiEditPage *p = MLK_MULTIEDITPAGE(wg);

	switch(ev->type)
	{
		case MEVENT_KEYDOWN:
			if(!p->mep.fpress)
				_event_keydown(p, ev);
			break;

		//文字
		case MEVENT_CHAR:
		case MEVENT_STRING:
			if(!p->mep.fpress && !_CHECK_STYLE(p, MMULTIEDIT_S_READ_ONLY))
			{
				if(mWidgetTextEdit_insertText(p->mep.pedit,
					(ev->type == MEVENT_CHAR)? NULL: (char *)ev->str.text,
					(ev->type == MEVENT_CHAR)? ev->ch.ch: 0))
				{
					_on_change_text(p);
				
					mWidgetRedraw(wg);
				}
			}
			break;

		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				if(p->mep.fpress)
					_event_motion(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				if(!p->mep.fpress)
				{
					if(ev->pt.btt == MLK_BTT_LEFT)
						_event_press(p, ev);
					else if(ev->pt.btt == MLK_BTT_RIGHT)
					{
						//右ボタン

						_press_rbtt(wg, ev);
					}
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//通知 (スクロールバー)
		case MEVENT_NOTIFY:
			if(ev->notify.widget_from == wg->parent
				&& (ev->notify.notify_type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ
					|| ev->notify.notify_type == MSCROLLVIEWPAGE_N_SCROLL_ACTION_VERT)
				&& (ev->notify.param2 & MSCROLLBAR_ACTION_F_CHANGE_POS))
			{
				//位置変更時、カーソル位置は変わらないため再描画のみ

				mWidgetRedraw(wg);
			}
			break;

		//ホイール (上下で垂直スクロール)
		case MEVENT_SCROLL:
			if(!p->mep.fpress)
			{
				if( mScrollBarAddPos(mScrollViewPage_getScrollBar_vert(MLK_SCROLLVIEWPAGE(p)),
						ev->scroll.vert_step * 3 * _get_line_height(p)) )
					mWidgetRedraw(wg);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
		
		default:
			return FALSE;
	}

	return TRUE;
}

/** 描画 */

void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mMultiEditPage *p = MLK_MULTIEDITPAGE(wg);
	mWidgetTextEdit *edit;
	mFont *font;
	uint32_t *pctop,*pcend;
	mPoint ptscr;
	int x,y,lineh,areaw,areah,num,left;
	uint32_t col_bksel;
	mWidgetTextEditState state[3],*pstate;

	areaw = wg->w - _PADDING_X * 2;
	areah = wg->h - _PADDING_Y * 2;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h,
		(mWidgetIsEnable(wg) && !(_CHECK_STYLE(p, MMULTIEDIT_S_READ_ONLY)))
			? MGUICOL_PIX(FACE_TEXTBOX): MGUICOL_PIX(FACE));

	//テキスト領域クリッピング

	if(!mPixbufClip_setBox_d(pixbuf, _PADDING_X, _PADDING_Y, areaw, areah))
		return;

	//----- テキスト描画

	font = mWidgetGetFont(wg);
	lineh = mWidgetGetFontHeight(wg);

	mScrollViewPage_getScrollPos(MLK_SCROLLVIEWPAGE(p), &ptscr);

	edit = p->mep.pedit;
	pctop = edit->unibuf;
	left = -ptscr.x + _PADDING_X;
	y = -ptscr.y + _PADDING_Y;

	col_bksel = mGuiCol_getPix(
		(wg->parent->fstate & MWIDGET_STATE_FOCUSED)? MGUICOL_FACE_SELECT: MGUICOL_FACE_SELECT_UNFOCUS);

	while(*pctop && y < areah)
	{
		//1行の範囲取得 (pcend = 次行先頭か終端)

		for(pcend = pctop; *pcend && *pcend != '\n'; pcend++);

		if(*pcend) pcend++;

		//スクロールで行が完全に隠れている場合は除く

		x = left;

		if(x < areaw && y > -lineh)
		{
			num = mWidgetTextEdit_getLineState(edit,
				pctop - edit->unibuf, pcend - edit->unibuf, state);

			for(pstate = state; num > 0; num--, pstate++)
			{
				//左側に隠れている場合は除く
				
				if(x + pstate->width > 0)
				{
					//選択背景

					if(pstate->is_sel)
						mPixbufFillBox(pixbuf, x, y, pstate->width, lineh, col_bksel);

					//テキスト

					mFontDrawText_pixbuf_utf32(font, pixbuf, x, y,
						pstate->text, pstate->len,
						mGuiCol_getRGB((pstate->is_sel)? MGUICOL_TEXT_SELECT: MGUICOL_TEXT));
				}

				x += pstate->width;
			}
		}

		//

		pctop = pcend;
		y += lineh;
	}

	//カーソル

	if(wg->parent->fstate & MWIDGET_STATE_FOCUSED)
	{
		mPixbufSetPixelModeXor(pixbuf);
		
		mPixbufLineV(pixbuf,
			edit->curx - ptscr.x + _PADDING_X,
			edit->multi_curline * lineh - ptscr.y + _PADDING_Y,
			lineh, 0);

		mPixbufSetPixelModeCol(pixbuf);
	}
}
