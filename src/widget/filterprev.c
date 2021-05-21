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
 * FilterPrev
 *
 * フィルタダイアログのプレビュー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_pixbuf.h"
#include "mlk_event.h"
#include "mlk_guicol.h"
#include "mlk_rectbox.h"

#include "tileimage.h"
#include "drawpixbuf.h"

#include "filterprev.h"


//-------------------------

struct _FilterPrev
{
	mWidget wg;

	mPixbuf *pixbuf;	//表示用イメージ

	TileImage *imgsrc;		//元イメージ (ドラッグでの範囲指定時用)
	int fullw,fullh;		//イメージの最大サイズ
	mlkbool enable_scroll,	//スクロールが有効か
		fpress;
	
	mBox boxview,		//プレビュー内で表示しているイメージ範囲
		boxframe;		//プレビュー内における、イメージ全体の枠の範囲
	mSize size_view;	//イメージ全体枠における、現在の表示範囲サイズ
};

//-------------------------



/* ドラッグ時、表示範囲位置セット */

static mlkbool _change_pos(FilterPrev *p,int x,int y)
{
	x = x - 1 - p->boxframe.x - p->size_view.w / 2;
	y = y - 1 - p->boxframe.y - p->size_view.h / 2;

	x = (int)((double)x / p->boxframe.w * p->fullw + 0.5);
	y = (int)((double)y / p->boxframe.h * p->fullh + 0.5);

	//調整

	if(x < 0)
		x = 0;
	else if(x > p->fullw - p->boxview.w)
		x = p->fullw - p->boxview.w;
	
	if(y < 0)
		y = 0;
	else if(y > p->fullh - p->boxview.h)
		y = p->fullh - p->boxview.h;

	//変更

	if(x == p->boxview.x && y == p->boxview.y)
		return FALSE;
	else
	{
		p->boxview.x = x;
		p->boxview.y = y;
		return TRUE;
	}
}

/* ドラッグ中イメージの描画 */

static void _drawimg_in_drag(FilterPrev *p)
{
	mPixbuf *pixbuf = p->pixbuf;
	int x,y,n;
	mBox boxf;

	boxf = p->boxframe;

	//ソースイメージ

	TileImage_drawFilterPreview(p->imgsrc, pixbuf, &p->boxview);

	//全体の枠

	mPixbufBox(pixbuf, boxf.x + 1, boxf.y + 1,
		boxf.w, boxf.h, mRGBtoPix(0xff0000));

	//表示範囲の枠

	x = (int)((double)p->boxview.x / p->fullw * boxf.w + boxf.x + 0.5);
	y = (int)((double)p->boxview.y / p->fullh * boxf.h + boxf.y + 0.5);

	n = boxf.x + boxf.w - p->size_view.w;
	if(x > n) x = n;

	n = boxf.y + boxf.h - p->size_view.h;
	if(y > n) y = n;

	drawpixbuf_dashBox_mono(pixbuf, x + 1, y + 1, p->size_view.w, p->size_view.h);
}

/* グラブ解除 */

static void _release_grab(FilterPrev *p)
{
	if(p->fpress)
	{
		p->fpress = 0;
		mWidgetUngrabPointer();

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, 0, 0, 0);
	}
}

/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	FilterPrev *p = (FilterPrev *)wg;

	switch(ev->type)
	{
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				//移動

				if(p->fpress == 1)
				{
					if(_change_pos(p, ev->pt.x, ev->pt.y))
						mWidgetRedraw(wg);
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				//押し

				if(ev->pt.btt == MLK_BTT_LEFT && !p->fpress && p->enable_scroll)
				{
					p->fpress = 1;
					mWidgetGrabPointer(MLK_WIDGET(p));

					_change_pos(p, ev->pt.x, ev->pt.y);

					mWidgetRedraw(wg);
				}
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				//離し
				
				if(ev->pt.btt == MLK_BTT_LEFT)
					_release_grab(p);
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

/* 描画 */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	FilterPrev *p = (FilterPrev *)wg;

	if(p->fpress)
		_drawimg_in_drag(p);

	mPixbufBlt(pixbuf, 0, 0, p->pixbuf, 0, 0, -1, -1);
}

/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	mPixbufFree(((FilterPrev *)wg)->pixbuf);
}


//====================


/** 作成 */

FilterPrev *FilterPrev_new(mWidget *parent,int id,int w,int h,
	TileImage *imgsrc,int fullw,int fullh)
{
	FilterPrev *p;

	//ウィジェットサイズ調整
	// :全体サイズの方が小さい場合

	if(fullw + 2 < w) w = fullw + 2;
	if(fullh + 2 < h) h = fullh + 2;

	//作成

	p = (FilterPrev *)mWidgetNew(parent, sizeof(FilterPrev));
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.flayout = MLF_FIX_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.destroy = _destroy_handle;
	p->wg.w = w;
	p->wg.h = h;
	p->wg.margin.right = 12;

	p->imgsrc = imgsrc;
	p->fullw = fullw;
	p->fullh = fullh;

	//スクロールが有効か

	p->enable_scroll = (fullw + 2 > w || fullh + 2 > h);

	//表示範囲

	p->boxview.w = w - 2;
	p->boxview.h = h - 2;

	//イメージ全体の枠の範囲

	p->boxframe.w = fullw;
	p->boxframe.h = fullh;

	mBoxResize_keepaspect(&p->boxframe, w - 2, h - 2, TRUE);

	//イメージ全体の枠における、表示範囲部分のサイズ

	p->size_view.w = (int)((double)p->boxview.w / fullw * p->boxframe.w + 0.5);
	p->size_view.h = (int)((double)p->boxview.h / fullh * p->boxframe.h + 0.5);

	//イメージ

	p->pixbuf = mPixbufCreate(w, h, 0);
	if(!p->pixbuf)
	{
		mWidgetDestroy(MLK_WIDGET(p));
		return NULL;
	}

	mPixbufBox(p->pixbuf, 0, 0, w, h, 0); //枠

	return p;
}

/** プレビュー時の描画対象のイメージ範囲取得 */

void FilterPrev_getDrawArea(FilterPrev *p,mRect *rc,mBox *box)
{
	if(p)
	{
		rc->x1 = p->boxview.x;
		rc->y1 = p->boxview.y;
		rc->x2 = rc->x1 + p->boxview.w - 1;
		rc->y2 = rc->y1 + p->boxview.h - 1;

		mBoxSetRect(box, rc);
	}
}

/** イメージ描画 */

void FilterPrev_drawImage(FilterPrev *p,TileImage *img)
{
	if(p)
	{
		TileImage_drawFilterPreview(img, p->pixbuf, &p->boxview);

		mWidgetRedraw(MLK_WIDGET(p));
	}
}

