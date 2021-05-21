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
 * 筆圧カーブ編集ダイアログ
 *****************************************/

#include <stdlib.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_button.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"

#include "def_draw.h"
#include "def_draw_toollist.h"

#include "widget_func.h"
#include "apphelp.h"

#include "trid.h"


//----------------

//グラフ
typedef struct
{
	mWidget wg;
	
	mPixbuf *img;
	uint16_t *tbl;
	int fdrag,
		ptnum;
	mPoint pt[CURVE_SPLINE_MAXNUM];
	uint32_t cpt[CURVE_SPLINE_MAXNUM];
	CurveSpline cparam;
}_graphwg;

//ダイアログ
typedef struct
{
	MLK_DIALOG_DEF

	_graphwg *graph;
}_dialog;

enum
{
	WID_BTT_RESET = 100,
	WID_BTT_COPY,
	WID_BTT_PASTE,
	WID_BTT_HELP
};

enum
{
	TRID_TITLE
};

//----------------

#define _GRAPHWG_SIZE  201

//----------------


//************************************
// グラフ
//************************************


/* グラフ描画 */

static void _draw_graph(_graphwg *p)
{
	mPixbuf *img = p->img;
	uint16_t *tbl;
	int size,i,n;
	mPoint pt,ptlast;
	mPixCol col;

	size = _GRAPHWG_SIZE;

	//枠

	mPixbufBox(img, 0, 0, size, size, 0);

	//背景

	mPixbufFillBox(img, 1, 1, size - 2, size - 2, MGUICOL_PIX(WHITE));

	//中央線

	n = size / 2;
	col = mRGBtoPix_sep(200,200,200);

	mPixbufLineH(img, 1, n, size - 2, col);
	mPixbufLineV(img, n, 1, size - 2, col);

	//基準線

	mPixbufLine(img, 0, size - 1, size - 1, 0, 0);

	//グラフ

	col = mRGBtoPix(0x0000ff);

	if(p->ptnum == 2)
	{
		//カーブなし

		mPixbufLine(img, p->pt[0].x, p->pt[0].y, p->pt[1].x, p->pt[1].y, col);
	}
	else
	{
		//カーブ

		tbl = p->tbl;
		size -= 3;

		ptlast = p->pt[0];

		for(i = 0; i <= size; i++)
		{
			n = tbl[(i << CURVE_SPLINE_POS_BIT) / size];
			n = (n * size) >> CURVE_SPLINE_POS_BIT;

			pt.x = 1 + i;
			pt.y = size - n + 1;
		
			mPixbufLine(img, ptlast.x, ptlast.y, pt.x, pt.y, col);

			ptlast = pt;
		}
	}

	//ポイント

	col = mRGBtoPix(0xFF46BE);

	for(i = p->ptnum - 1; i >= 0; i--)
	{
		mPixbufBox(img, p->pt[i].x - 3, p->pt[i].y - 3, 7, 7, col);
	}
}

/* グラフ更新 */

static void _update_graph(_graphwg *p)
{
	CurveSpline_setCurveTable(&p->cparam, p->tbl, p->cpt, p->ptnum);

	_draw_graph(p);

	mWidgetRedraw(MLK_WIDGET(p));
}

/* 指定位置の周囲に点があるか
 *
 * return: -1 でなし、0〜で点番号 (両端含む) */

static int _get_point_at_pos(_graphwg *p,int x,int y)
{
	int i;

	for(i = 0; i < p->ptnum; i++)
	{
		if(abs(x - p->pt[i].x) <= 3 && abs(y - p->pt[i].y) <= 3)
			return i;
	}

	return -1;
}

/* ポイント位置セット */

static void _set_point(_graphwg *p,int no,int x,int y)
{
	int max;

	//調整

	if(no == 0 || no == p->ptnum - 1)
	{
		//両端の場合、X 位置は移動不可

		x = p->pt[no].x;
	}
	else
	{
		//X 位置は前後の点の間のみ移動可 (同じ位置可)

		if(x < p->pt[no - 1].x)
			x = p->pt[no - 1].x;
		else if(x > p->pt[no + 1].x)
			x = p->pt[no + 1].x;
	}

	//グラフ部分の位置

	max = _GRAPHWG_SIZE - 3;

	x--;
	y--;

	if(x < 0) x = 0; else if(x > max) x = max;
	if(y < 0) y = 0; else if(y > max) y = max;

	//
	
	p->pt[no].x = x + 1;
	p->pt[no].y = y + 1;

	//データ位置

	x = (x << CURVE_SPLINE_POS_BIT) / max;
	y = ((max - y) << CURVE_SPLINE_POS_BIT) / max;
		
	p->cpt[no] = (x << 16) | y;
}

/* ポイントを新規追加 */

static int _new_point(_graphwg *p,int x,int y)
{
	int i,no,num;

	num = p->ptnum;

	//追加する位置

	no = num - 1;

	for(i = 1; i < num; i++)
	{
		if(x < p->pt[i].x)
		{
			no = i;
			break;
		}
	}

	//後ろをずらす

	for(i = num; i > no; i--)
	{
		p->pt[i] = p->pt[i - 1];
		p->cpt[i] = p->cpt[i - 1];
	}

	//

	p->ptnum++; //[!]先に追加

	_set_point(p, no, x, y);

	_update_graph(p);

	return no;
}

/* ポイントの削除 */

static void _delete_point(_graphwg *p,int no)
{
	int i;

	if(no == 0 || no == p->ptnum - 1) return;

	//詰める

	for(i = no; i < p->ptnum - 1; i++)
	{
		p->pt[i] = p->pt[i + 1];
		p->cpt[i] = p->cpt[i + 1];
	}

	p->ptnum--;

	_update_graph(p);
}


//-------------


/* 押し */

static void _graph_event_press(_graphwg *p,mEventPointer *ev)
{
	int no;

	no = _get_point_at_pos(p, ev->x, ev->y);

	if(ev->btt == MLK_BTT_RIGHT
		|| (ev->btt == MLK_BTT_LEFT && (ev->state & MLK_STATE_CTRL)))
	{
		//右クリック/Ctrl+左クリック: ポイントの削除

		if(no == -1) return;

		_delete_point(p, no);
	}
	else if(ev->btt == MLK_BTT_LEFT)
	{
		//---- 左ボタン (新規ポイント/点のドラッグ)
		
		//新規ポイント

		if(no == -1)
		{
			if(p->ptnum == CURVE_SPLINE_MAXNUM) return;

			no = _new_point(p, ev->x, ev->y);
		}

		//

		p->fdrag = no + 1;
		mWidgetGrabPointer(MLK_WIDGET(p));
	}
}

/* 移動 */

static void _graph_event_motion(_graphwg *p,mEvent *ev)
{
	int no;

	no = p->fdrag - 1;

	_set_point(p, no, ev->pt.x, ev->pt.y);

	_update_graph(p);
}

/* グラブ解除 */

static void _graph_release_grab(_graphwg *p)
{
	if(p->fdrag)
	{
		mWidgetUngrabPointer();
		p->fdrag = 0;
	}
}

/* イベントハンドラ */

static int _graph_event_handle(mWidget *wg,mEvent *ev)
{
	_graphwg *p = (_graphwg *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fdrag)
					_graph_event_motion(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(!p->fdrag)
					_graph_event_press(p, (mEventPointer *)ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_graph_release_grab(p);
			}
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_graph_release_grab(p);
			break;
	}

	return 1;
}

/* 描画 */

static void _graph_draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBlt(pixbuf, 0, 0,
		((_graphwg *)wg)->img, 0, 0, -1, -1);
}

/* 破棄ハンドラ */

static void _graph_destroy_handle(mWidget *wg)
{
	_graphwg *p = (_graphwg *)wg;

	mFree(p->tbl);
	mPixbufFree(p->img);
}

/* 位置をリセット */

static void _graph_reset_point(_graphwg *p)
{
	p->pt[0].x = 1;
	p->pt[0].y = _GRAPHWG_SIZE - 2;
	p->pt[1].x = _GRAPHWG_SIZE - 2;
	p->pt[1].y = 1;

	p->cpt[0] = 0;
	p->cpt[1] = CURVE_SPLINE_VAL_MAXPOS;

	p->ptnum = 2;
}

/* データから位置をセット */

static void _graph_set_data(_graphwg *p,const uint32_t *pt)
{
	int i,max,x,y;

	//点の数はデータから取得

	max = _GRAPHWG_SIZE - 3;

	for(i = 0; i < CURVE_SPLINE_MAXNUM; i++)
	{
		if(i && pt[i] == 0) break;
		
		p->cpt[i] = pt[i];

		x = pt[i] >> 16;
		y = pt[i] & 0xffff;

		p->pt[i].x = (x * max >> CURVE_SPLINE_POS_BIT) + 1;
		p->pt[i].y = max - (y * max >> CURVE_SPLINE_POS_BIT) + 1;
	}

	p->ptnum = i;
}

/* グラフ作成 */

static _graphwg *_create_graph(mWidget *parent,uint32_t *pt)
{
	_graphwg *p;

	p = (_graphwg *)mWidgetNew(parent, sizeof(_graphwg));
	if(!p) return NULL;

	p->wg.flayout = MLF_FIX_WH;
	p->wg.w = p->wg.h = _GRAPHWG_SIZE;
	p->wg.fevent = MWIDGET_EVENT_POINTER;
	p->wg.destroy = _graph_destroy_handle;
	p->wg.event = _graph_event_handle;
	p->wg.draw = _graph_draw_handle;

	//

	p->tbl = (uint16_t *)mMalloc((CURVE_SPLINE_POS_VAL + 1) * 2);

	p->img = mPixbufCreate(_GRAPHWG_SIZE, _GRAPHWG_SIZE, 0);

	//

	_graph_set_data(p, pt);

	CurveSpline_setCurveTable(&p->cparam, p->tbl, p->cpt, p->ptnum);

	_draw_graph(p);

	return p;
}

/* 曲線リセット */

static void _graph_reset(_graphwg *p)
{
	_graph_reset_point(p);

	_update_graph(p);
}

/* データを取得 */

static void _graph_get_data(_graphwg *p,uint32_t *dst)
{
	int i;

	//未使用の値は 0

	for(i = p->ptnum; i < CURVE_SPLINE_MAXNUM; i++)
		p->cpt[i] = 0;

	//コピー

	memcpy(dst, p->cpt, 4 * CURVE_SPLINE_MAXNUM);
}


//************************************
// ダイアログ
//************************************


/* イベントハンドラ */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//リセット
				case WID_BTT_RESET:
					_graph_reset(p->graph);
					break;
				//コピー
				case WID_BTT_COPY:
					_graph_get_data(p->graph, APPDRAW->tlist->pt_press_copy);
					break;
				//貼り付け
				case WID_BTT_PASTE:
					_graph_set_data(p->graph, APPDRAW->tlist->pt_press_copy);
					_update_graph(p->graph);
					break;
				//ヘルプ
				case WID_BTT_HELP:
					AppHelp_message(MLK_WINDOW(wg), HELP_TRGROUP_SINGLE, HELP_TRID_BRUSH_PRESSURE);
					break;
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,uint32_t *pt)
{
	_dialog *p;
	mWidget *ct,*ct2,*ct3;

	MLK_TRGROUP(TRGROUP_DLG_PRESSURE);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);

	if(!p) return NULL;

	//-----

	ct = mContainerCreateHorz(MLK_WIDGET(p), 15, MLF_EXPAND_W, 0);

	//グラフ

	p->graph = _create_graph(ct, pt);

	//---- ボタン

	ct2 = mContainerCreateVert(ct, 6, MLF_EXPAND_W, 0);

	//リセット

	mButtonCreate(ct2, WID_BTT_RESET, MLF_RIGHT, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_RESET));

	//コピー/貼り付け

	ct3 = mContainerCreateHorz(ct2, 5, MLF_RIGHT, 0);

	widget_createCopyButton(ct3, WID_BTT_COPY, 0, 0, 25);

	widget_createPasteButton(ct3, WID_BTT_PASTE, 0, 0, 25);

	//"?"

	widget_createHelpButton(ct2, WID_BTT_HELP, MLF_RIGHT, 0);

	//---- OK/Cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** 筆圧カーブダイアログ実行 */

mlkbool BrushDialog_pressure(mWindow *parent,uint32_t *pt)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent, pt);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		_graph_get_data(p->graph, pt);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

