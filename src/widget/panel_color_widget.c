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
 * (パネル)カラーで使うウィジェット
 *
 * [描画色/背景色] [色マスク]
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_menu.h"

#include "def_draw.h"

#include "draw_main.h"
#include "trid.h"


//--------------------

enum
{
	TRID_MENU_SETCOLOR = 100,
	TRID_MENU_TOGGLE,
	TRID_MENU_SPOIT
};

//"MASK" 23x7 1bit
static const uint8_t g_img_mask[]={
0x11,0xe1,0x44,0x9b,0x12,0x25,0x55,0x14,0x14,0x55,0xe4,0x0c,
0xd1,0x07,0x15,0x51,0x14,0x25,0x51,0xe4,0x44 };

//"REV" 17x7 1bit
static const uint8_t g_img_rev[]={
0xcf,0x17,0x01,0x51,0x10,0x01,0x51,0x10,0x01,0xcf,0xa3,0x00,
0x45,0xa0,0x00,0x49,0x40,0x00,0xd1,0x47,0x00 };

//色マスクのボタン:8x8 1bit
static const uint8_t g_img_col_on[] = { 0xff,0x01,0x7d,0x7d,0x7d,0x7d,0x7d,0x01 },
	g_img_col_off[] = { 0xff,0x83,0x45,0x29,0x11,0x29,0x45,0x83};

//--------------------



/**************************************
 * 描画色/背景色
 **************************************/

//色が入れ替わった時、通知する。

#define DRAWCOL_SPACE  10


/* 描画 */

static void _drawcol_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	int w,h;

	w = wg->w, h = wg->h;

	//背景

	mWidgetDrawBkgnd(wg, NULL);

	//背景色

	mPixbufBox(pixbuf, DRAWCOL_SPACE, DRAWCOL_SPACE,
		w - DRAWCOL_SPACE, h - DRAWCOL_SPACE, 0);

	mPixbufFillBox(pixbuf, DRAWCOL_SPACE + 1, DRAWCOL_SPACE + 1,
		w - DRAWCOL_SPACE - 2, h - DRAWCOL_SPACE - 2,
		RGBcombo_to_pixcol(&APPDRAW->col.bkgndcol));

	//描画色

	mPixbufBox(pixbuf, 0, 0, w - DRAWCOL_SPACE, h - DRAWCOL_SPACE, 0);

	mPixbufFillBox(pixbuf, 1, 1, w - DRAWCOL_SPACE - 2, h - DRAWCOL_SPACE - 2,
		RGBcombo_to_pixcol(&APPDRAW->col.drawcol));
}

/* イベントハンドラ */

static int _drawcol_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& (ev->pt.act == MEVENT_POINTER_ACT_PRESS || ev->pt.act == MEVENT_POINTER_ACT_DBLCLK)
		&& ev->pt.btt == MLK_BTT_LEFT)
	{
		//左ボタンで色を入れ替え

		drawColor_toggleDrawCol(APPDRAW);

		mWidgetRedraw(wg);

		mWidgetEventAdd_notify(wg, NULL, 0, 0, 0);
	}

	return 1;
}

/** 描画色/背景色ウィジェット作成 */

mWidget *DrawColorWidget_new(mWidget *parent,int id)
{
	mWidget *p;

	p = mWidgetNew(parent, 0);

	p->id = id;
	p->fevent |= MWIDGET_EVENT_POINTER;
	p->hintW = p->hintH = 38;
	p->draw = _drawcol_draw_handle;
	p->event = _drawcol_event_handle;

	return p;
}



/**************************************
 * 色マスク
 **************************************/

/*
 - mWidget::param1 = (mPixbuf *)
 - マスク色がスポイトされた時、通知を送る。
*/

#define _COLMASK_TYPEBTT_W 35  //ボタンの幅。枠含む
#define _COLMASK_ROW_H     19  //一つの高さ。枠含む
#define _COLMASK_COLOR_W   34  //マスク色の幅。枠含む
#define _COLMASK_WIDTH    (_COLMASK_TYPEBTT_W + (_COLMASK_COLOR_W - 1) * 3 + 1)
#define _COLMASK_HEIGHT   (_COLMASK_ROW_H * 2 - 1)

enum
{
	_COLMASK_AREA_MASK,		//マスク
	_COLMASK_AREA_REV,		//逆マスク
	_COLMASK_AREA_COLOR,	//マスク色部分
	_COLMASK_AREA_COLBTT	//マスク色のボタン部分
};


//=======================
// 描画
//=======================


/* タイプボタン描画
 *
 * type: 0=mask, 1=rev */

static void _colmask_draw_typebtt(mPixbuf *pixbuf,int type,mlkbool press)
{
	const uint8_t *pat;
	int y,patw;

	if(type)
		y = _COLMASK_ROW_H, pat = g_img_rev, patw = 17;
	else
		y = 1, pat = g_img_mask, patw = 23;

	if(press)
	{
		//押し時

		mPixbufBox(pixbuf, 1, y, _COLMASK_TYPEBTT_W - 2, _COLMASK_ROW_H - 2,
			MGUICOL_PIX(WHITE));

		mPixbufFillBox(pixbuf, 2, y + 1, _COLMASK_TYPEBTT_W - 4, _COLMASK_ROW_H - 4,
			mRGBtoPix_sep(255,66,104));
	}
	else
	{
		//通常時

		mPixbufDraw3DFrame(pixbuf, 1, y, _COLMASK_TYPEBTT_W - 2, _COLMASK_ROW_H - 2,
			MGUICOL_PIX(WHITE), mRGBtoPix_sep(132,132,132));

		mPixbufFillBox(pixbuf, 2, y + 1, _COLMASK_TYPEBTT_W - 4, _COLMASK_ROW_H - 4,
			mRGBtoPix_sep(230,230,230));
	}

	//文字

	mPixbufDraw1bitPattern(pixbuf,
		(_COLMASK_TYPEBTT_W - patw) / 2 , y + 5,
		pat, patw, 7,
		MPIXBUF_TPCOL, (press)? MGUICOL_PIX(WHITE): 0);
}

/* タイプボタン部分を描画 */

static void _colmask_draw_typebtt_all(mPixbuf *img)
{
	int n;

	n = APPDRAW->col.colmask_type;

	_colmask_draw_typebtt(img, 0, (n == 1));
	_colmask_draw_typebtt(img, 1, (n == 2));
}

/* マスク色部分を描画
 *
 * frame: TRUE で黒枠を描画 */

static void _colmask_draw_color(mPixbuf *img,int no,mlkbool frame)
{
	int x,y,fon;

	fon = APPDRAW->col.maskcol[no].a;

	x = _COLMASK_TYPEBTT_W + (no % 3) * (_COLMASK_COLOR_W - 1);
	y = (no / 3) * (_COLMASK_ROW_H - 1);

	if(frame)
		mPixbufBox(img, x + 1, y + 1, _COLMASK_COLOR_W - 2, _COLMASK_ROW_H - 2, 0);

	mPixbufFillBox(img, x + 2, y + 2, _COLMASK_COLOR_W - 4, _COLMASK_ROW_H - 4,
		RGBA8_to_pixcol(APPDRAW->col.maskcol + no));

	//チェック

	mPixbufDraw1bitPattern(img,
		x + _COLMASK_COLOR_W - 1 - 9, y + _COLMASK_ROW_H - 1 - 9,
		(fon)? g_img_col_on: g_img_col_off, 8, 8, MGUICOL_PIX(WHITE), 0);

	//OFF 線

	if(!fon)
	{
		x += _COLMASK_COLOR_W - 2 - 9 - 2;
		y += 2;
	
		mPixbufLine(img, x, y,
			x - (_COLMASK_ROW_H - 5), y + (_COLMASK_ROW_H - 5), 0);

		mPixbufLine(img, x + 1, y,
			x - (_COLMASK_ROW_H - 5) + 1, y + (_COLMASK_ROW_H - 5), MGUICOL_PIX(WHITE));
	}
}

/* 初期描画 */

static void _colmask_init_image(mPixbuf *img)
{
	int i;

	//背景

	mPixbufFillBox(img, 0, 0, _COLMASK_WIDTH, _COLMASK_HEIGHT, MGUICOL_PIX(WHITE));

	//---- タイプボタン

	//枠

	mPixbufBox(img, 0, 0, _COLMASK_TYPEBTT_W, _COLMASK_HEIGHT, 0);
	mPixbufLineH(img, 0, _COLMASK_ROW_H - 1, _COLMASK_TYPEBTT_W, 0);

	//MASK/REV

	_colmask_draw_typebtt_all(img);

	//---- マスク色

	for(i = 0; i < DRAW_COLORMASK_NUM; i++)
		_colmask_draw_color(img, i, TRUE);
}


//=======================
// sub
//=======================


/* カーソル位置からエリアを取得 */

static int _colmask_get_area(mWidget *wg,int x,int y,int *dst_no)
{
	int ret,xpos,ypos;

	if(x < _COLMASK_TYPEBTT_W)
	{
		//MASK/REV

		ret = (y < _COLMASK_ROW_H)? _COLMASK_AREA_MASK: _COLMASK_AREA_REV;

		*dst_no = 0;
	}
	else
	{
		//色

		x -= _COLMASK_TYPEBTT_W;

		xpos = x / (_COLMASK_COLOR_W - 1);
		if(xpos > 2) xpos = 2;

		ypos = (y >= _COLMASK_ROW_H);

		x -= xpos * (_COLMASK_COLOR_W - 1);
		y -= ypos * (_COLMASK_ROW_H -1);

		ret = (x >= _COLMASK_COLOR_W - 1 - 9 && y >= _COLMASK_ROW_H - 1 - 9)
			? _COLMASK_AREA_COLBTT: _COLMASK_AREA_COLOR;

		*dst_no = ypos * 3 + xpos;
	}

	return ret;
}

/* マスク色 ON/OFF */

static void _colmask_cmd_color_onoff(mWidget *wg,int no)
{
	APPDRAW->col.maskcol[no].a = !APPDRAW->col.maskcol[no].a;

	_colmask_draw_color((mPixbuf *)wg->param1, no, FALSE);
	mWidgetRedraw(wg);
}

/* 描画色をセット */

static void _colmask_cmd_setcol(mWidget *wg,int no)
{
	//セット時は常に ON
	RGB8_to_RGBA8(APPDRAW->col.maskcol + no, &APPDRAW->col.drawcol.c8, 1);

	_colmask_draw_color((mPixbuf *)wg->param1, no, FALSE);
	mWidgetRedraw(wg);
}

/* マスク色をスポイト */

static void _colmask_cmd_spoit(mWidget *wg,int no)
{
	drawColor_setDrawColor(RGBA8_to_32bit(APPDRAW->col.maskcol + no));

	mWidgetEventAdd_notify(wg, NULL, 0, 0, 0);
}

/* 通常左クリック時 */

static void _colmask_press_left(mWidget *wg,int area,int no)
{
	switch(area)
	{
		//マスク/逆マスク
		case _COLMASK_AREA_MASK:
		case _COLMASK_AREA_REV:
			drawColorMask_setType(APPDRAW, (area == _COLMASK_AREA_MASK)? -1: -2);

			_colmask_draw_typebtt_all((mPixbuf *)wg->param1);
			mWidgetRedraw(wg);
			break;
		//描画色セット
		case _COLMASK_AREA_COLOR:
			_colmask_cmd_setcol(wg, no);
			break;
		//ON/OFF
		case _COLMASK_AREA_COLBTT:
			_colmask_cmd_color_onoff(wg, no);
			break;
	}
}

/* メニュー */

static void _colmask_run_menu(mWidget *wg,int colno,int x,int y)
{
	mMenu *menu;
	mMenuItem *mi;
	int id;

	//メニュー

	MLK_TRGROUP(TRGROUP_PANEL_COLOR);

	menu = mMenuNew();

	mMenuAppendTrText(menu, TRID_MENU_SETCOLOR, 3);

	mi = mMenuPopup(menu, wg, x, y, NULL, MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);
	id = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	if(id == -1) return;

	//コマンド

	switch(id)
	{
		//描画色をセット
		case TRID_MENU_SETCOLOR:
			_colmask_cmd_setcol(wg, colno);
			break;
		//ON/OFF
		case TRID_MENU_TOGGLE:
			_colmask_cmd_color_onoff(wg, colno);
			break;
		//スポイト
		case TRID_MENU_SPOIT:
			_colmask_cmd_spoit(wg, colno);
			break;
	}
}


//=======================
// ハンドラ
//=======================


/* イベント */

static int _colmask_event_handle(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_POINTER
		&& (ev->pt.act == MEVENT_POINTER_ACT_PRESS || ev->pt.act == MEVENT_POINTER_ACT_DBLCLK))
	{
		int area,no;

		area = _colmask_get_area(wg, ev->pt.x, ev->pt.y, &no);
	
		if(ev->pt.btt == MLK_BTT_RIGHT)
		{
			//右ボタン:メニュー

			if(area == _COLMASK_AREA_COLOR || area == _COLMASK_AREA_COLBTT)
				_colmask_run_menu(wg, no, ev->pt.x, ev->pt.y);
		}
		else if(ev->pt.btt == MLK_BTT_LEFT)
		{
			//左ボタン

			if(ev->pt.state & MLK_STATE_CTRL)
			{
				//+Ctrl: ON/OFF

				_colmask_cmd_color_onoff(wg, no);
			}
			else if(ev->pt.state & MLK_STATE_SHIFT)
			{
				//+Shift: スポイト

				_colmask_cmd_spoit(wg, no);
			}
			else
				_colmask_press_left(wg, area, no);
		}
	}
	
	return 1;
}

/* 描画ハンドラ */

static void _colmask_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBlt(pixbuf, 0, 0, (mPixbuf *)wg->param1, 0, 0, -1, -1);
}

/* 破棄ハンドラ */

static void _colmask_destroy(mWidget *wg)
{
	mPixbufFree((mPixbuf *)wg->param1);
}

/** 色マスクウィジェット作成 */

mWidget *ColorMaskWidget_new(mWidget *parent,int id)
{
	mWidget *p;
	mPixbuf *img;

	p = mWidgetNew(parent, 0);
	if(!p) return NULL;

	p->id = id;
	p->fevent |= MWIDGET_EVENT_POINTER;
	p->flayout = MLF_EXPAND_X | MLF_FIX_WH | MLF_RIGHT;
	p->w = _COLMASK_WIDTH;
	p->h = _COLMASK_HEIGHT;
	p->draw = _colmask_draw_handle;
	p->event = _colmask_event_handle;
	p->destroy = _colmask_destroy;

	img = mPixbufCreate(_COLMASK_WIDTH, _COLMASK_HEIGHT, 0);

	p->param1 = (intptr_t)img;

	_colmask_init_image(img);

	return p;
}

/** マスクの状態が変更された時 */

void ColorMaskWidget_update(mWidget *wg,int type)
{
	switch(type)
	{
		//マスク色1番目の色が変更
		case 0:
			_colmask_draw_color((mPixbuf *)wg->param1, 0, FALSE);
			mWidgetRedraw(wg);
			break;
	}
}


