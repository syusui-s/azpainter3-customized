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
 * [panel] ツールリストのアイテムリスト
 *****************************************/

#include <stdlib.h> //abs

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_scrollbar.h"
#include "mlk_event.h"
#include "mlk_font.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "def_draw.h"
#include "def_draw_toollist.h"
#include "def_toolicon.h"

#include "toollist.h"
#include "appcursor.h"

#include "draw_main.h"
#include "draw_toollist.h"

#include "pv_toollist_page.h"
#include "pv_toolicon.h"


//---------------------

/* ToolListView */

typedef struct _ToolListView
{
	MLK_CONTAINER_DEF

	ToolListPage *page;
	mScrollBar *scr;
}ToolListView;

#define _PAGE(p)  ((ToolListPage *)(p))

//---------------------

//押し位置情報
enum
{
	POINTINFO_EMPTY,		//終端の空部分
	POINTINFO_GROUP_EMPTY,	//グループ内の空部分
	POINTINFO_ITEM,			//アイテムの上
	POINTINFO_GROUP_HEADER	//グループのヘッダの上
};

//fdrag
enum
{
	_DRAG_DND_FIRST = 1,
	_DRAG_DND_RUN
};

#define _ITEM_MIN_WIDTH  10  //アイテムの最小幅
#define _ITEM_MIN_HEIGHT 18  //アイテムの最小高さ (余白含まない)

#define COL_FRAME        0x999999   //枠の色
#define COL_ITEM_BKGND   0xffffff   //アイテム背景
#define COL_GROUP_HEADER 0xB2CEFB   //グループヘッダの背景
#define COL_SELECT_BKGND 0xffdcdc	//選択アイテムの背景色
#define COL_SELECT_FRAME 0xcc0000	//選択アイテムの枠色
#define COL_SELECT_TEXT  0x880000	//選択アイテムの文字色
#define COL_DND_FRAME    0x0000ff   //D&D先の枠

#define _GROUP_ARROW_X    3			//グループ展開矢印のX
#define _GROUP_ARROW_SIZE 7			//展開矢印のサイズ
#define _GROUP_NAME_X     15		//グループ名のX位置

//---------------------



//************************************
// ToolListPage
//************************************


/* スクロール情報セット */

static void _page_set_scrollinfo(ToolListPage *p)
{
	ToolListGroupItem *gi;
	int h,itemh,headerh;

	itemh = p->itemh;
	headerh = p->headerh;

	//アイテム全体の高さ

	h = 0;

	TOOLLIST_FOR_GROUP(gi)
	{
		h += headerh;
		
		if(gi->fopened)
			h += ToolListGroupItem_getViewVertNum(gi) * itemh;
	}

	//終端には常に一行分の空をセット

	h += headerh;

	//

	mScrollBarSetStatus(p->scr, 0, h, p->wg.h);
}

/* 押された位置の情報取得 */

static int _get_pointinfo(ToolListPage *p,int px,int py,_POINTINFO *dst)
{
	ToolListGroupItem *gi;
	ToolListItem *item;
	int n,y,wg_w,wg_h,eachw,colnum,xno,yno,itemh,headerh;

	wg_w = p->wg.w;
	wg_h = p->wg.h;

	dst->group = NULL;
	dst->item  = NULL;
	dst->width = 0;

	//ドラッグ中:カーソルがウィジェット外の場合は範囲外

	if(p->fdrag && (px < 0 || px >= wg_w || py < 0 || py >= wg_h))
		return POINTINFO_EMPTY;

	//

	itemh = p->itemh;
	headerh = p->headerh;

	//各グループ判定

	y = -mScrollBarGetPos(p->scr);

	TOOLLIST_FOR_GROUP(gi)
	{
		if(y >= wg_h) break;

		//グループヘッダ内

		if(y <= py && py < y + headerh)
		{
			dst->group = gi;

			return POINTINFO_GROUP_HEADER;
		}

		y += headerh;

		//グループが閉じている

		if(!gi->fopened) continue;
		
		//py がこのグループの範囲外なら、次へ

		n = ToolListGroupItem_getViewVertNum(gi) * itemh;

		if(py < y || py >= y + n)
		{
			y += n;
			continue;
		}

		//----- グループ内

		dst->group = gi;

		colnum = gi->colnum;

		//1つの幅

		eachw = wg_w / colnum;
		if(eachw < _ITEM_MIN_WIDTH) eachw = _ITEM_MIN_WIDTH;

		//アイテム取得

		xno = px / eachw;
		yno = (py - y) / itemh;

		if(xno >= colnum) xno = colnum - 1;

		item = ToolListGroupItem_getItemAtIndex(gi, yno * colnum + xno);

		//セット

		dst->item = item;
		dst->x = xno * eachw;
		dst->y = y + yno * itemh;
		dst->width = (xno == colnum - 1)? wg_w - 1 - eachw * (colnum - 1): eachw;

		return (item)? POINTINFO_ITEM: POINTINFO_GROUP_EMPTY;
	}

	return POINTINFO_EMPTY;
}

/* アイテム内枠を描画
 *
 * drawcol: -1 で消去 */

static void _draw_inside_frame(ToolListPage *p,mPixbuf *pixbuf,_POINTINFO *info,int32_t drawcol)
{
	mPixCol col;

	//終端の空 or グループヘッダ
	
	if(!info->width) return;

	//

	if(drawcol >= 0)
		col = mRGBtoPix(drawcol);
	else if(!info->item)
		col = MGUICOL_PIX(FACE);
	else
		col = mRGBtoPix((info->item == APPDRAW->tlist->selitem)? COL_SELECT_FRAME: COL_ITEM_BKGND);

	mPixbufBox(pixbuf, info->x + 1, info->y, info->width - 1, p->itemh - 1, col);

	mWidgetUpdateBox_d(MLK_WIDGET(p), info->x + 1, info->y, info->width - 1, p->itemh - 1);
}

/* D&D 先描画 (移動先変更時。直接描画) */

static void _draw_dnd_frame(ToolListPage *p,_POINTINFO *info)
{
	mPixbuf *pixbuf;

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p));
	if(pixbuf)
	{
		//消去
		_draw_inside_frame(p, pixbuf, &p->dnd_info, -1);

		//新規
		_draw_inside_frame(p, pixbuf, info, COL_DND_FRAME);
	
		mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);
	}
}


//=========================
// イベント
//=========================


/* ボタン押し/ダブルクリック */

static void _event_press(ToolListPage *p,mEventPointer *ev)
{
	_POINTINFO info;
	int ret;

	ret = _get_pointinfo(p, ev->x, ev->y, &info);

	if(ev->btt == MLK_BTT_RIGHT)
	{
		//------ 右クリック

		switch(ret)
		{
			//アイテム/グループの空部分
			case POINTINFO_ITEM:
			case POINTINFO_GROUP_EMPTY:
				ToolListPage_runMenu_item(p, ev->x, ev->y, &info);
				break;
			//グループヘッダ
			case POINTINFO_GROUP_HEADER:
				ToolListPage_runMenu_group(p, ev->x, ev->y, info.group);
				break;
			//終端の空部分
			case POINTINFO_EMPTY:
				ToolListPage_runMenu_empty(p, ev->x, ev->y);
				break;
		}
	}
	else if(ev->btt == MLK_BTT_LEFT)
	{
		//------- 左クリック

		switch(ret)
		{
			//グループヘッダ (展開状態の切り替え)
			case POINTINFO_GROUP_HEADER:
				info.group->fopened ^= 1;

				ToolListPage_updateAll(p);
				break;
			//アイテム
			case POINTINFO_ITEM:
				if(ev->act == MEVENT_POINTER_ACT_DBLCLK)
				{
					//ダブルクリック => 設定

					ToolListPage_cmd_itemOption(p, info.item);
				}
				else
				{
					//現在のツールを変更
					// :他のツール時に、現在選択されているアイテムをクリックした時も対象。

					drawTool_setTool(APPDRAW, TOOL_TOOLLIST);

					//選択変更
					
					if(drawToolList_setSelItem(APPDRAW, info.item))
					{
						//選択が変わった時
						
						mWidgetRedraw(MLK_WIDGET(p));
						
						ToolListPage_notify_changeValue(p);
					}
					else
					{
						//すでに選択してあるアイテム上で押した場合、D&D 開始

						p->fdrag = _DRAG_DND_FIRST;
						p->dnd_srcitem = info.item;
						p->dnd_info = info;
						p->dnd_pt_press.x = ev->x;
						p->dnd_pt_press.y = ev->y;

						mWidgetGrabPointer(MLK_WIDGET(p));
					}
				}
				break;
		}
	}
}

/* ポインタ移動 (ドラッグ中) */

static void _event_motion(ToolListPage *p,mEventPointer *ev)
{
	_POINTINFO info;

	_get_pointinfo(p, ev->x, ev->y, &info);

	//width = 0 で、終端の空 or グループヘッダ上

	//D&D 開始状態
	// :押し位置から別のアイテムに移動し、かつ押し位置から指定px以上移動した場合、D&D 開始

	if(p->fdrag == _DRAG_DND_FIRST)
	{
		if((info.width && info.item != p->dnd_srcitem)
			&& (abs(ev->x - p->dnd_pt_press.x) > 16 || abs(ev->y - p->dnd_pt_press.y) > 16))
		{
			p->fdrag = _DRAG_DND_RUN;

			mWidgetSetCursor(MLK_WIDGET(p), AppCursor_getForDrag(APPCURSOR_DND_ITEM));
		}
	}

	//D&D 中

	if(p->fdrag == _DRAG_DND_RUN)
	{
		//移動先変更

		if(info.group != p->dnd_info.group || info.item != p->dnd_info.item
			|| info.width != p->dnd_info.width
			|| info.x != p->dnd_info.x)
		{
			//挿入線

			_draw_dnd_frame(p, &info);

			p->dnd_info = info;
		}
	}
}

/* グラブ解放 */

static void _release_grab(ToolListPage *p)
{
	if(p->fdrag)
	{
		//D&D 実行

		if(p->fdrag == _DRAG_DND_RUN)
		{
			if(p->dnd_info.width)
				ToolList_moveItem(p->dnd_srcitem, p->dnd_info.group, p->dnd_info.item);
		
			mWidgetSetCursor(MLK_WIDGET(p), 0);

			ToolListPage_updateAll(p);
		}
		
		//

		p->fdrag = 0;
		mWidgetUngrabPointer();
	}
}

/* イベント */

static int _page_event_handle(mWidget *wg,mEvent *ev)
{
	ToolListPage *p = _PAGE(wg);
	
	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				if(p->fdrag)
					_event_motion(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS
				|| ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
			{
				if(!p->fdrag)
					_event_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
			}
			break;

		//ホイールで垂直スクロール
		case MEVENT_SCROLL:
			if(ev->scroll.vert_step)
			{
				if(mScrollBarAddPos(p->scr, p->itemh * ev->scroll.vert_step))
					mWidgetRedraw(wg);
			}
			break;
		
		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_release_grab(p);
			break;
	}

	return 1;
}


//=========================
// 描画 sub
//=========================

static const uint8_t g_pat_arrow_close[] = {0x01,0x03,0x07,0x0f,0x07,0x03,0x01},
	g_pat_arrow_expand[] = {0x7f,0x3e,0x1c,0x08};


/* グループヘッダ描画 */

static void _draw_group_header(ToolListPage *p,mPixbuf *pixbuf,
	ToolListGroupItem *gi,int y,int w,mFont *font)
{
	int h;

	h = p->headerh - 1;

	//背景

	mPixbufFillBox(pixbuf, 0, y, w, h, mRGBtoPix(COL_GROUP_HEADER));

	//下枠

	mPixbufLineH(pixbuf, 0, y + h, w, mRGBtoPix(COL_FRAME));

	//展開矢印

	if(gi->fopened)
	{
		mPixbufDraw1bitPattern(pixbuf,
			_GROUP_ARROW_X, y + (h - 4) / 2,
			g_pat_arrow_expand, 7, 4, MPIXBUF_TPCOL, 0);
	}
	else
	{
		mPixbufDraw1bitPattern(pixbuf,
			_GROUP_ARROW_X + 3, y + (h - 7) / 2,
			g_pat_arrow_close, 4, 7, MPIXBUF_TPCOL, 0);
	}

	//名前

	if(mPixbufClip_setBox_d(pixbuf, _GROUP_NAME_X, y, w - _GROUP_NAME_X - 3, h))
	{
		mFontDrawText_pixbuf(font, pixbuf, _GROUP_NAME_X,
			y + (h - mFontGetHeight(font)) / 2, gi->name, -1, 0);
	}

	mPixbufClip_clear(pixbuf);
}


//=========================
// main
//=========================


/* 描画 */

static void _page_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	ToolListPage *p = _PAGE(wg);
	ToolListGroupItem *gi;
	ToolListItem *item,*selitem;
	mFont *font;
	int x,y,n,ynext,hcnt,eachw,lastw,itemw,itemh,headerh,fonth;
	mPixCol col_frame,col_bkgnd;
	mlkbool fselect,fregist;
	char m[2] = {0,0};

	font = mWidgetGetFont(wg);

	fonth = mFontGetHeight(font);
	itemh = p->itemh;
	headerh = p->headerh;

	selitem = APPDRAW->tlist->selitem;

	col_frame = mRGBtoPix(COL_FRAME);

	//グループ

	y = -mScrollBarGetPos(p->scr);

	TOOLLIST_FOR_GROUP(gi)
	{
		if(y >= wg->h) break;
		
		//グループヘッダ

		if(y > -headerh)
			_draw_group_header(p, pixbuf, gi, y, wg->w, font);

		//

		y += headerh;

		if(!gi->fopened) continue;

		//アイテム全体が上に隠れている場合、位置を進める

		hcnt = ToolListGroupItem_getViewVertNum(gi) * itemh;

		if(y + hcnt <= 0)
		{
			y += hcnt;
			continue;
		}

		ynext = y + hcnt;

		//------- 各アイテム

		//背景 (グループ内の空白部分)

		mPixbufFillBox(pixbuf, 0, y, wg->w, ynext - y, MGUICOL_PIX(FACE));

		//1つの幅

		eachw = wg->w / gi->colnum;

		if(eachw < _ITEM_MIN_WIDTH) eachw = _ITEM_MIN_WIDTH;

		//右端のアイテムの幅 (右に 1px 余白を作る)

		lastw = wg->w - 1 - eachw * (gi->colnum - 1);

		//グループ内に登録アイテムがあるか

		fregist = drawToolList_isRegistItem_inGroup(gi);

		//

		x = 0;
		hcnt = gi->colnum;

		TOOLLIST_FOR_ITEM(gi, item)
		{
			itemw = (hcnt == 1)? lastw: eachw;
			fselect = (item == selitem);

			//全体のクリッピング

			mPixbufClip_setBox_d(pixbuf, x, y, itemw + 1, itemh);

			//背景

			col_bkgnd = (fselect)? mRGBtoPix(COL_SELECT_BKGND): MGUICOL_PIX(WHITE);

			mPixbufFillBox(pixbuf, x + 1, y, itemw - 1, itemh - 1, col_bkgnd);

			//アイコン

			mPixbufBlt_1bit(pixbuf, x + 3, y + (itemh - 16) / 2,
				g_img_toolicon, item->icon * 16, 0, 16, 16,
				TOOLICON_NUM * 16, col_bkgnd, 0);

			//登録ツール番号

			if(fregist)
			{
				n = drawToolList_getRegistItemNo(item);

				if(n != -1)
				{
					m[0] = '1' + n;

					mPixbufFillBox(pixbuf, x + 3, y + 2, 6, 9, mRGBtoPix(0x0000ff));

					mPixbufDrawNumberPattern_5x7(pixbuf, x + 4, y + 3, m, MGUICOL_PIX(WHITE)); 
				}
			}

			//枠
			// :上の枠はグループヘッダの下端と重なる

			mPixbufBox(pixbuf, x, y - 1, itemw + 1, itemh + 1, col_frame);
			
			//選択枠

			if(fselect)
				mPixbufBox(pixbuf, x + 1, y, itemw - 1, itemh - 1, mRGBtoPix(COL_SELECT_FRAME));

			//名前

			if(mPixbufClip_setBox_d(pixbuf, x + 3 + 16 + 3, y, itemw - (3+16+3+2), itemh))
			{
				mFontDrawText_pixbuf(font, pixbuf,
					x + 3 + 16 + 3, y + (itemh - fonth) / 2, item->name, -1,
					(fselect)? COL_SELECT_TEXT: 0);
			}

			mPixbufClip_clear(pixbuf);
		
			//次へ

			hcnt--;
			x += itemw;
			
			if(hcnt == 0)
			{
				x = 0;
				hcnt = gi->colnum;
				y += itemh;
			}
		}

		//次のグループへ

		y = ynext;
	}

	//末尾の空白部分

	if(y < wg->h)
		mPixbufFillBox(pixbuf, 0, y, wg->w, wg->h - y, MGUICOL_PIX(FACE_DARK));
}

/* サイズ変更時 */

static void _page_resize_handle(mWidget *wg)
{
	//スクロール情報

	_page_set_scrollinfo(_PAGE(wg));
}

/* ToolListPage 作成 */

static ToolListPage *_create_page(mWidget *parent)
{
	ToolListPage *p;

	p = (ToolListPage *)mWidgetNew(parent, sizeof(ToolListPage));

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER | MWIDGET_EVENT_SCROLL;
	p->wg.event = _page_event_handle;
	p->wg.draw = _page_draw_handle;
	p->wg.resize = _page_resize_handle;

	return p;
}

/** 更新 (状態や数の変更時) */

void ToolListPage_updateAll(ToolListPage *p)
{
	_page_set_scrollinfo(p);
	
	mWidgetRedraw(MLK_WIDGET(p));
}

/** 通知を送る (選択変更時など、ブラシの情報を更新する必要がある場合) */

void ToolListPage_notify_changeValue(ToolListPage *p)
{
	mWidgetEventAdd_notify(p->wg.parent, NULL, 0, 0, 0);
}

/** 通知を送る (ツールリストパネルの値のみ変更) */

void ToolListPage_notify_updateToolListPanel(ToolListPage *p)
{
	mWidgetEventAdd_notify(p->wg.parent, NULL, 1, 0, 0);
}

/** メニュー用アイテム選択枠描画 (直接描画) */

void ToolListPage_drawItemSelFrame(ToolListPage *p,_POINTINFO *info,mlkbool erase)
{
	mPixbuf *pixbuf;

	pixbuf = mWidgetDirectDraw_begin(MLK_WIDGET(p));
	if(pixbuf)
	{
		_draw_inside_frame(p, pixbuf, info, (erase)? -1: 0x0000ff);
	
		mWidgetDirectDraw_end(MLK_WIDGET(p), pixbuf);
	}
}


//************************************
// ToolListView
//************************************


/* 描画 */

static void _view_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	//枠描画
	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, MGUICOL_PIX(FRAME));
}

/* mScrollBar 操作ハンドラ */

static void _view_scroll_handle(mWidget *p,int pos,uint32_t flags)
{
	if(flags & MSCROLLBAR_ACTION_F_CHANGE_POS)
		mWidgetRedraw(MLK_WIDGET(p->param1));
}

/** ToolListView 作成 */

ToolListView *ToolListView_new(mWidget *parent,mFont *font)
{
	ToolListView *p;
	ToolListPage *page;
	mScrollBar *scr;
	int n;

	p = (ToolListView *)mContainerNew(parent, sizeof(ToolListView));

	mContainerSetType_horz(MLK_CONTAINER(p), 0);
	mContainerSetPadding_same(MLK_CONTAINER(p), 1); //枠分

	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.draw = _view_draw_handle;

	//page

	p->page = page = _create_page(MLK_WIDGET(p));

	page->wg.font = font;

	//アイテム高さ

	n = mFontGetHeight(font);
	if(n < _ITEM_MIN_HEIGHT) n = _ITEM_MIN_HEIGHT;

	page->itemh = n + 7;

	n = mFontGetHeight(font) + 6;	//fonth + padding + bottom(1px)
	if(n < 10) n = 10;		//矢印ボタンを含めた最小サイズ

	page->headerh = n;

	//スクロールバー

	p->scr = scr = mScrollBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_H, 0, MSCROLLBAR_S_VERT);

	scr->wg.param1 = (intptr_t)page;

	mScrollBarSetHandle_action(scr, _view_scroll_handle);

	//

	page->scr = scr;

	return p;
}

/** リストを更新 */

void ToolListView_update(ToolListView *p)
{
	ToolListPage_updateAll(p->page);
}

