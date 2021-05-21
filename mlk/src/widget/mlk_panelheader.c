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
 * mPanelHeader
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_panelheader.h"
#include "mlk_pixbuf.h"
#include "mlk_font.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_util.h"


//-------------

#define _MIN_TEXT_HEIGHT  11	//テキストの最小高さ
#define _PADDING_VERT  1	//垂直余白
#define _BUTTON_PADX   2	//ボタンのX余白

#define _TITLE_CH_W  6
#define _TITLE_CH_H  8

static const uint8_t g_pat_shut_close[7] = {0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04},
	g_pat_shut_open[7] = {0x00,0x00,0x7f,0x3e,0x1c,0x08,0x00},
	g_pat_close[7] = {0x22,0x77,0x3e,0x1c,0x3e,0x77,0x22},
	g_pat_store_up[7] = {0x08,0x1c,0x36,0x6b,0x1c,0x36,0x63},
	g_pat_store_down[7] = {0x63,0x36,0x1c,0x6b,0x36,0x1c,0x08},
	g_pat_resize[7] = {0x08,0x1c,0x3e,0x08,0x3e,0x1c,0x08};

//-------------



/* ボタンのサイズ取得 */

static int _get_button_size(mPanelHeader *p)
{
	return p->wg.h - _PADDING_VERT * 2;
}


//============================
// main
//============================


/**@ mPanelHeader データ解放 */

void mPanelHeaderDestroy(mWidget *wg)
{

}

/**@ 作成
 *
 * @d:レイアウトは、デフォルトで MLF_EXPAND_W。 */

mPanelHeader *mPanelHeaderNew(mWidget *parent,int size,uint32_t fstyle)
{
	mPanelHeader *p;
	
	if(size < sizeof(mPanelHeader))
		size = sizeof(mPanelHeader);
	
	p = (mPanelHeader *)mWidgetNew(parent, size);
	if(!p) return NULL;

	p->wg.destroy = mPanelHeaderDestroy;
	p->wg.draw = mPanelHeaderHandle_draw;
	p->wg.event = mPanelHeaderHandle_event;
	p->wg.calc_hint = mPanelHeaderHandle_calcHint;
	
	p->wg.flayout = MLF_EXPAND_W;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;

	p->ph.fstyle = fstyle;
	
	return p;
}

/**@ calc_hint ハンドラ */

void mPanelHeaderHandle_calcHint(mWidget *wg)
{
	int h;

	h = mWidgetGetFontHeight(wg);
	if(h < _MIN_TEXT_HEIGHT) h = _MIN_TEXT_HEIGHT;

	if(!(h & 1)) h++;

	wg->hintH = h + _PADDING_VERT * 2;
}

/**@ タイトルのセット (静的文字列) */

void mPanelHeaderSetTitle(mPanelHeader *p,const char *title)
{
	p->ph.text = title;
}

/**@ 折りたたみ状態の変更
 *
 * @p:type [0] 折りたたむ [1] 開く [負] 切り替え */

void mPanelHeaderSetShut(mPanelHeader *p,int type)
{
	if((p->ph.fstyle & MPANELHEADER_S_HAVE_SHUT)
		&& mIsChangeFlagState(type, !(p->ph.fstyle & MPANELHEADER_S_SHUT_CLOSED)))
	{
		p->ph.fstyle ^= MPANELHEADER_S_SHUT_CLOSED;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

/**@ リサイズのボタン状態を変更 */

void mPanelHeaderSetResize(mPanelHeader *p,int type)
{
	if((p->ph.fstyle & MPANELHEADER_S_HAVE_RESIZE)
		&& mIsChangeFlagState(type, !(p->ph.fstyle & MPANELHEADER_S_RESIZE_DISABLED)))
	{
		p->ph.fstyle ^= MPANELHEADER_S_RESIZE_DISABLED;

		mWidgetRedraw(MLK_WIDGET(p));
	}
}


//========================
// ハンドラ
//========================


/* x が bx を左端とするボタンの範囲にあるか */

static mlkbool _is_in_button(mPanelHeader *p,int x,int bx)
{
	return (bx <= x && x < bx + _get_button_size(p));
}

/* 位置からボタン取得
 *
 * return: MPANELHEADER_BTT_*。範囲外で 0 */

static int _get_button_at_point(mPanelHeader *p,int x,int y)
{
	int bx,bttsize;

	if(y >= _PADDING_VERT && y < p->wg.h - _PADDING_VERT)
	{
		//(左端) 折りたたみ
		
		if((p->ph.fstyle & MPANELHEADER_S_HAVE_SHUT)
			&& _is_in_button(p, x, _BUTTON_PADX))
			return MPANELHEADER_BTT_SHUT;

		//---- 右端から順に
		
		bttsize = _get_button_size(p);

		bx = p->wg.w - _BUTTON_PADX - bttsize;

		//閉じる

		if(p->ph.fstyle & MPANELHEADER_S_HAVE_CLOSE)
		{
			if(_is_in_button(p, x, bx)) return MPANELHEADER_BTT_CLOSE;
			bx -= bttsize + _BUTTON_PADX;
		}

		//格納

		if(p->ph.fstyle & MPANELHEADER_S_HAVE_STORE)
		{
			if(_is_in_button(p, x, bx)) return MPANELHEADER_BTT_STORE;
			bx -= bttsize + _BUTTON_PADX;
		}

		//リサイズ

		if((p->ph.fstyle & MPANELHEADER_S_HAVE_RESIZE) && _is_in_button(p, x, bx))
			return MPANELHEADER_BTT_RESIZE;
	}

	return 0;
}

/* グラブ解除 */

static void _release_grab(mPanelHeader *p,mlkbool send)
{
	if(p->ph.fpress)
	{
		if(send)
		{
			mWidgetEventAdd_notify(MLK_WIDGET(p), NULL,
				MPANELHEADER_N_PRESS_BUTTON, p->ph.fpress, 0);
		}
	
		mWidgetUngrabPointer();
		mWidgetRedraw(MLK_WIDGET(p));

		p->ph.fpress = 0;
	}
}

/**@ event ハンドラ関数 */

int mPanelHeaderHandle_event(mWidget *wg,mEvent *ev)
{
	mPanelHeader *p = MLK_PANELHEADER(wg);

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				//押し
			
				if(ev->pt.btt == MLK_BTT_LEFT && p->ph.fpress == 0)
				{
					p->ph.fpress = _get_button_at_point(p, ev->pt.x, ev->pt.y);

					if(p->ph.fpress)
					{
						mWidgetGrabPointer(wg);
						mWidgetRedraw(wg);
					}
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
			
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p, TRUE);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p, FALSE);
			break;

		default:
			return FALSE;
	}

	return TRUE;
}


//========================
// 描画
//========================


/* ボタン描画 (7x7) */

static void _draw_button(mPixbuf *pixbuf,
	int x,int y,const uint8_t *pat,int bttsize,int press,int disable)
{
	mPixbufBox(pixbuf, x, y, bttsize, bttsize, MGUICOL_PIX(FRAME));

	mPixbufFillBox(pixbuf, x + 1, y + 1, bttsize - 2, bttsize - 2,
		mGuiCol_getPix(press? MGUICOL_BUTTON_FACE_PRESS: MGUICOL_BUTTON_FACE));

	mPixbufDraw1bitPattern(pixbuf,
		x + (bttsize - 7) / 2 + press, y + (bttsize - 7) / 2 + press,
		pat, 7, 7,
		MPIXBUF_TPCOL, mGuiCol_getPix(disable? MGUICOL_TEXT_DISABLE: MGUICOL_TEXT));
}

/**@ draw ハンドラ関数 */

void mPanelHeaderHandle_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mPanelHeader *p = MLK_PANELHEADER(wg);
	int x,x2,bttsize;
	uint32_t fstyle;

	fstyle = p->ph.fstyle;
	bttsize = _get_button_size(p);

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FACE_DARK));

	//タイトル

	if(p->ph.text)
	{
		//タイトルX位置
		
		x = _BUTTON_PADX;
		if(fstyle & MPANELHEADER_S_HAVE_SHUT) x += bttsize + _BUTTON_PADX;

		//右側のボタンの左位置 (余白含む)
		
		x2 = p->wg.w - _BUTTON_PADX;
		if(fstyle & MPANELHEADER_S_HAVE_CLOSE) x2 -= bttsize + _BUTTON_PADX;
		if(fstyle & MPANELHEADER_S_HAVE_STORE) x2 -= bttsize + _BUTTON_PADX;
		if(fstyle & MPANELHEADER_S_HAVE_RESIZE) x2 -= bttsize + _BUTTON_PADX;

		if(mPixbufClip_setBox_d(pixbuf, x, _PADDING_VERT, x2 - x, bttsize))
		{
			mFontDrawText_pixbuf(mWidgetGetFont(wg), pixbuf,
				x, _PADDING_VERT, p->ph.text, -1, MGUICOL_RGB(TEXT));
		}

		mPixbufClip_clear(pixbuf);
	}

	//折りたたみボタン

	if(fstyle & MPANELHEADER_S_HAVE_SHUT)
	{
		_draw_button(pixbuf, _BUTTON_PADX, _PADDING_VERT,
			(fstyle & MPANELHEADER_S_SHUT_CLOSED)? g_pat_shut_close: g_pat_shut_open,
			bttsize, (p->ph.fpress == MPANELHEADER_BTT_SHUT), 0);
	}

	//----- 右側ボタン

	x = wg->w - _BUTTON_PADX - bttsize;

	//閉じるボタン

	if(fstyle & MPANELHEADER_S_HAVE_CLOSE)
	{
		_draw_button(pixbuf, x, _PADDING_VERT, g_pat_close,
			bttsize, (p->ph.fpress == MPANELHEADER_BTT_CLOSE), 0);

		x -= bttsize + _BUTTON_PADX;
	}

	//格納ボタン

	if(fstyle & MPANELHEADER_S_HAVE_STORE)
	{
		_draw_button(pixbuf, x, _PADDING_VERT,
			(fstyle & MPANELHEADER_S_STORE_LEAVED)? g_pat_store_down: g_pat_store_up,
			bttsize, (p->ph.fpress == MPANELHEADER_BTT_STORE), 0);

		x -= bttsize + _BUTTON_PADX;
	}

	//リサイズボタン

	if(fstyle & MPANELHEADER_S_HAVE_RESIZE)
	{
		_draw_button(pixbuf, x, _PADDING_VERT, g_pat_resize,
			bttsize, (p->ph.fpress == MPANELHEADER_BTT_RESIZE),
			fstyle & MPANELHEADER_S_RESIZE_DISABLED);
	}
}
