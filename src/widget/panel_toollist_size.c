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
 * [Panel] toollist のブラシサイズリスト
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_scrollbar.h"
#include "mlk_event.h"
#include "mlk_font.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_menu.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"
#include "mlk_string.h"

#include "def_config.h"
#include "def_draw.h"

#include "brushsize_list.h"

#include "trid.h"


/*
  [!] データが変更された時、APPDRAW->fmodify_brushsize = TRUE にする。

  - サイズが選択されると、view から通知イベントが送られる。
    param1 = サイズ値。
*/

//---------------------

#define _PAGE(p)  ((_page *)(p))

typedef struct
{
	mWidget wg;

	mBuf *buf;	//サイズデータ
	mScrollBar *scr;
	int fpress,
		hnum,		//水平方向の表示個数
		press_no;	//押し時のデータ位置
}_page;

typedef struct
{
	MLK_CONTAINER_DEF

	_page *page;
	mScrollBar *scr;
}_view;

//---------------------

#define EACHSIZE   27	//1個のサイズ

#define COL_SELBOX 0xff0000
#define COL_NUMBER 0x0000ff
#define COL_FRAME  0x999999

#define _PRESS_NORMAL 1
#define _PRESS_DELETE 2

enum
{
	TRID_MENU_ADD,
	TRID_MENU_DEL,

	TRID_ADDDLG_TITLE = 100,
	TRID_ADDDLG_MESSAGE
};

//---------------------



//**********************************
// _page : リストの中身
//**********************************


/* スクロール情報セット */

static void _page_set_scrollinfo(_page *p)
{
	int max,page;

	max = BrushSizeList_getNum(p->buf);

	max = (max + p->hnum - 1) / p->hnum;

	page = p->wg.h / EACHSIZE;
	if(page <= 0) page = 1;

	mScrollBarSetStatus(p->scr, 0, max, page);
}

/* すべて更新 */

static void _page_update(_page *p)
{
	_page_set_scrollinfo(p);

	mWidgetRedraw(MLK_WIDGET(p));
}

/* ポインタ位置からデータインデックス取得
 *
 * return: -1 で範囲外 */

static int _page_getindex_atpos(_page *p,int x,int y)
{
	int no,num;

	num = BrushSizeList_getNum(p->buf);

	x /= EACHSIZE;
	y /= EACHSIZE;

	if(x >= p->hnum) return -1;

	no = (y + mScrollBarGetPos(p->scr)) * p->hnum + x;

	return (no < num)? no: -1;
}

/* ボタン押し時の選択枠描画 (直接描画) */

static void _draw_selbox(_page *p,mlkbool erase)
{
	mPixbuf *pixbuf;
	int x,y;

	if(p->press_no == -1) return;

	x = (p->press_no % p->hnum) * EACHSIZE;
	y = (p->press_no / p->hnum - mScrollBarGetPos(p->scr)) * EACHSIZE; 

	//描画

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p));
	
	if(pixbuf)
	{
		mPixbufBox(pixbuf, x, y, EACHSIZE + 1, EACHSIZE + 1,
			mRGBtoPix(erase? COL_FRAME: COL_SELBOX));

		mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);

		mWidgetUpdateBox_d(MLK_WIDGET(p), x, y, EACHSIZE + 1, EACHSIZE + 1);
	}
}


//=============================
// コマンド
//=============================


/* 追加 */

static void _cmd_add(_page *p)
{
	mStr str = MSTR_INIT;

	//メニューの直後なので、翻訳グループはそのまま

	if(mSysDlg_inputText(p->wg.toplevel,
		MLK_TR(TRID_ADDDLG_TITLE), MLK_TR(TRID_ADDDLG_MESSAGE),
		MSYSDLG_INPUTTEXT_F_NOT_EMPTY, &str))
	{
		BrushSizeList_addFromText(p->buf, str.buf);
		
		mStrFree(&str);

		_page_update(p);

		//

		APPDRAW->fmodify_brushsize = TRUE;
	}
}

/* 削除 */

static void _cmd_del(_page *p,int no)
{
	BrushSizeList_deleteAtPos(p->buf, no);

	APPDRAW->fmodify_brushsize = TRUE;

	_page_update(p);
}

/* メニュー実行 */

static void _run_menu(_page *p,int x,int y,int no)
{
	mMenu *menu;
	mMenuItem *mi;
	int id;

	p->press_no = no;

	_draw_selbox(p, FALSE);

	//メニュー

	MLK_TRGROUP(TRGROUP_PANEL_TOOLLIST_BRUSHSIZE);

	menu = mMenuNew();

	mMenuAppendTrText(menu, TRID_MENU_ADD, 2);

	if(no == -1)
		mMenuSetItemEnable(menu, TRID_MENU_DEL, 0);

	mi = mMenuPopup(menu, MLK_WIDGET(p), x, y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	//選択枠消去

	_draw_selbox(p, TRUE);

	//処理

	if(id == -1) return;

	switch(id)
	{
		//追加
		case TRID_MENU_ADD:
			_cmd_add(p);
			break;
		//削除
		case TRID_MENU_DEL:
			_cmd_del(p, no);
			break;
	}
}


//=============================
// イベント
//=============================


/* ボタン押し時 */

static void _event_press(_page *p,mEventPointer *ev)
{
	int no;

	no = _page_getindex_atpos(p, ev->x, ev->y);

	if(ev->btt == MLK_BTT_RIGHT)
	{
		//右クリックメニュー

		_run_menu(p, ev->x, ev->y, no);
	}
	else if(ev->btt == MLK_BTT_LEFT && no != -1)
	{
		//----- 左ボタン

		p->fpress = (ev->state & MLK_STATE_SHIFT)? _PRESS_DELETE: _PRESS_NORMAL;
		p->press_no = no;

		//通知

		if(p->fpress == _PRESS_NORMAL)
		{
			mWidgetEventAdd_notify(p->wg.parent, NULL, 0,
				BrushSizeList_getValue(p->buf, no), 0);
		}

		_draw_selbox(p, FALSE);

		mWidgetGrabPointer(MLK_WIDGET(p));
	}
}

/* グラブ解放 */

static void _release_grab(_page *p)
{
	if(p->fpress)
	{
		if(p->fpress == _PRESS_DELETE)
			_cmd_del(p, p->press_no);
		else
			_draw_selbox(p, TRUE);
	
		mWidgetUngrabPointer();
		p->fpress = 0;
	}
}

/* イベント */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	_page *p = _PAGE(wg);
	
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				if(!p->fpress)
					_event_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
	}

	return 1;
}


//=============================
// 描画
//=============================


/* ブラシサイズ プレビュー描画 */

static void _draw_brushprev(mPixbuf *pixbuf,
	int x,int y,int w,int h,int size)
{
	uint8_t *pd;
	int bpp,pitch,ix,iy,jx,jy,xtbl[4],ytbl[4],f,fcx,fcy,frr,val;
	mBox box;
	mFuncPixbufSetBuf setbuf;

	if(!mPixbufClip_getBox_d(pixbuf, &box, x, y, w, h)) return;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->pixel_bytes;
	pitch = pixbuf->line_bytes - bpp * box.w;

	mPixbufGetFunc_setbuf(pixbuf, &setbuf);

	//[!] 位置は 2bit 固定小数点数で計算

	fcx = w << 1; //w * 4 / 2
	fcy = h << 1;

	frr = (size << 1) / 10;
	frr *= frr;

	//box.x, box.y を相対位置に

	box.x -= x;
	box.y -= y;

	//4x4 オーバーサンプリング

	for(iy = 0; iy < box.h; iy++, pd += pitch)
	{
		//y テーブル

		f = ((box.y + iy) << 2) - fcy;

		for(jy = 0; jy < 4; jy++, f++)
			ytbl[jy] = f * f;
	
		//
	
		for(ix = 0; ix < box.w; ix++, pd += bpp)
		{
			//x テーブル

			f = ((box.x + ix) << 2) - fcx;

			for(jx = 0; jx < 4; jx++, f++)
				xtbl[jx] = f * f;

			//4x4

			val = 0;

			for(jy = 0; jy < 4; jy++)
			{
				for(jx = 0; jx < 4; jx++)
				{
					if(xtbl[jx] + ytbl[jy] < frr)
						val += 255;
				}
			}

			val >>= 4;

			//文字にかかる部分は色を薄くする

			if(box.y + iy >= EACHSIZE - 11)
				val >>= 3;

			//セット

			val = 255 - val;

			(setbuf)(pd, mRGBtoPix_sep(val,val,val));
		}
	}
}

/* 描画 */

static void _page_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	_page *p = _PAGE(wg);
	uint16_t *pbuf;
	int num,pos,ix,iy,xx,size;
	char m[10];
	mPixCol col_frame,col_num,col_bkgnd;

	//背景

	mPixbufFillBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FACE_DARK));

	//

	col_frame = mRGBtoPix(COL_FRAME);
	col_num = mRGBtoPix(COL_NUMBER);
	col_bkgnd = MGUICOL_PIX(WHITE);

	pos = mScrollBarGetPos(p->scr) * p->hnum;
	num = BrushSizeList_getNum(p->buf);
	pbuf = (uint16_t *)p->buf->buf + pos;

	//

	for(iy = 0; iy < wg->h; iy += EACHSIZE)
	{
		for(ix = 0, xx = 0; ix < p->hnum; ix++, pos++, pbuf++, xx += EACHSIZE)
		{
			if(pos >= num) return;

			size = *pbuf;

			mPixbufBox(pixbuf, xx, iy, EACHSIZE + 1, EACHSIZE + 1, col_frame);
			
			mPixbufBox(pixbuf, xx + 1, iy + 1, EACHSIZE - 1, EACHSIZE - 1, col_bkgnd);

			_draw_brushprev(pixbuf,
				xx + 2, iy + 2, EACHSIZE - 3, EACHSIZE - 3, (size > 240)? 240: size);

			//数値

			mIntToStr_float(m, size, 1);

			mPixbufDrawNumberPattern_5x7(pixbuf, xx + 2, iy + EACHSIZE - 8, m, col_num);
		}
	}
}


//=============================


/* サイズ変更時 */

static void _page_resize_handle(mWidget *wg)
{
	int n;

	//水平方向の個数

	n = (wg->w - 1) / EACHSIZE;
	if(n < 1) n = 1;

	_PAGE(wg)->hnum = n;

	//スクロール

	_page_set_scrollinfo(_PAGE(wg));
}

/* page 作成 */

static _page *_page_new(mWidget *parent,mBuf *buf)
{
	_page *p;

	p = (_page *)mWidgetNew(parent, sizeof(_page));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.event = _page_event_handle;
	p->wg.draw = _page_draw_handle;
	p->wg.resize = _page_resize_handle;

	p->buf = buf;
	p->hnum = 1;

	return p;
}


//**********************************
// _sizelist : リストメイン
//**********************************


/* 描画 */

static void _view_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	//枠
	
	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FRAME));
}

/* スクロールバー ハンドル */

static void _scroll_handle(mWidget *p,int pos,uint32_t flags)
{
	if(flags & MSCROLLBAR_ACTION_F_CHANGE_POS)
		mWidgetRedraw(MLK_WIDGET(p->param1));
}

/** 作成 */

mWidget *SizeListView_new(mWidget *parent,mBuf *buf)
{
	_view *p;
	mScrollBar *scr;
	int h;

	h = APPCONF->panel.toollist_sizelist_h;

	p = (_view *)mContainerNew(parent, sizeof(_view));

	p->wg.flayout = MLF_EXPAND_W | MLF_FIX_H;
	p->wg.h = (h < 1)? 1: h;
	p->wg.draw = _view_draw_handle;

	mContainerSetType_horz(MLK_CONTAINER(p), 0);
	mContainerSetPadding_same(MLK_CONTAINER(p), 1); //枠分

	//高さ 0 の場合は非表示

	if(h == 0)
		p->wg.fstate &= ~MWIDGET_STATE_VISIBLE;

	//page

	p->page = _page_new(MLK_WIDGET(p), buf);

	//スクロールバー

	p->scr = scr = mScrollBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0, MSCROLLBAR_S_VERT);

	scr->wg.param1 = (intptr_t)p->page;
	
	mScrollBarSetHandle_action(scr, _scroll_handle);

	//

	p->page->scr = scr;

	return (mWidget *)p;
}

