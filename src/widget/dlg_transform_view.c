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
 * 変形ダイアログ:TransformView
 *****************************************/

#include <string.h>
#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_rectbox.h"

#include "def_draw.h"
#include "def_config.h"

#include "imagecanvas.h"
#include "tileimage.h"
#include "appcursor.h"

#include "pv_transformdlg.h"


//--------------

#define _DRAGF_MOVE_CANVAS -1
#define _DRAGF_ZOOM_CANVAS -2

//--------------


//=========================
// sub
//=========================


/* XOR 枠描画
 *
 * 直接 mPixbuf に描画すると点が重なるため、
 * TileImage(A1) に描画して合成する。 */

static void _draw_xorframe(TransformView *p)
{
	TileImage *img = p->imgxor;
	mRect rcclip;
	uint64_t col = (uint64_t)-1;
	int i;

	TileImage_freeAllTiles(img);

	//枠

	rcclip.x1 = rcclip.y1 = 0;
	rcclip.x2 = p->wg.w - 1;
	rcclip.y2 = p->wg.h - 1;

	TileImage_drawLinePoint4(img, p->pt_corner_canv, &rcclip);

	//4隅の点

	for(i = 0; i < 4; i++)
		TileImage_drawBox(img, p->pt_corner_canv[i].x - 4, p->pt_corner_canv[i].y - 4, 9, 9, &col);
}

/* キャンバスの状態変更時 (スクロール/表示倍率) */

static void _change_canvas_param(TransformView *p)
{
	mDoublePoint dpt[2];
	int i;

	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換: ソース座標から、変換後の4隅のキャンバス座標を取得

		dpt[0].x = dpt[0].y = 0;
		dpt[1].x = p->boxsrc.w;
		dpt[1].y = p->boxsrc.h;

		TransformView_affine_source_to_canv_pt(p, p->pt_corner_canv, dpt[0].x, dpt[0].y);
		TransformView_affine_source_to_canv_pt(p, p->pt_corner_canv + 1, dpt[1].x, dpt[0].y);
		TransformView_affine_source_to_canv_pt(p, p->pt_corner_canv + 2, dpt[1].x, dpt[1].y);
		TransformView_affine_source_to_canv_pt(p, p->pt_corner_canv + 3, dpt[0].x, dpt[1].y);
	}
	else
	{
		//射影変換: イメージ座標から、4隅のキャンバス座標を取得

		for(i = 0; i < 4; i++)
		{
			TransformView_image_to_canv_pt(p, p->pt_corner_canv + i,
				p->dpt_corner_dst[i].x + p->pt_mov.x,
				p->dpt_corner_dst[i].y + p->pt_mov.y);
		}
	}
}

/* 変形パラメータ変更時、作業値を更新 */

static void _change_transform_param(TransformView *p)
{
	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換
		
		p->dcos = cos(p->angle);
		p->dsin = sin(p->angle);

		p->scalex_div = 1 / p->scalex;
		p->scaley_div = 1 / p->scaley;
	}
	else
	{
		//射影変換

		getHomographyRevParam(p->homog_param,
			p->dpt_corner_src, p->dpt_corner_dst);
	}

	_change_canvas_param(p);
}


//=======================
// ハンドラ sub
//=======================


/* 移動時 (変形操作) */

static void _event_motion(TransformView *p,mEvent *ev)
{
	mDoublePoint dpt;
	int x,y,update = 1;
	double d1,d2;

	x = ev->pt.x;
	y = ev->pt.y;

	//

	if(p->fdrag == TRANSFORM_DSTAREA_OUT_IMAGE)
	{
		//アフィン変換: 回転

		d1 = TransformView_affine_get_canvas_angle(p, x, y);
		
		p->angle += d1 - p->dtmp[0];

		p->dtmp[0] = d1;

		update |= 2;
	}
	else if(p->fdrag == TRANSFORM_DSTAREA_IN_IMAGE)
	{
		//共通: 平行移動

		if(!(ev->pt.state & MLK_STATE_CTRL))
			p->pt_mov.x += round((x - p->pt_last_canv.x) * p->viewparam.scalediv);

		if(!(ev->pt.state & MLK_STATE_SHIFT))
			p->pt_mov.y += round((y - p->pt_last_canv.y) * p->viewparam.scalediv);
	}
	else if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換: 4隅の点移動 (拡大縮小)
		// 押し時の位置とカーソル位置のソース座標から倍率計算

		TransformView_affine_canv_to_source_for_drag(p, &dpt, x, y);

		dpt.x -= p->centerx;
		dpt.y -= p->centery;

		d1 = fabs(dpt.x / p->dtmp[0]);
		d2 = fabs(dpt.y / p->dtmp[1]);

		if(d1 == 0) d1 = 0.001;
		if(d2 == 0) d2 = 0.001;

		if(p->keep_aspect)
		{
			if(d1 > d2)
				d2 = d1;
			else
				d1 = d2;
		}

		p->scalex = d1;
		p->scaley = d2;

		update |= 2;
	}
	else
	{
		//射影変換: 4隅の点 移動

		update = TransformView_onMotion_homog(p, x, y, ev->pt.state);
	}

	//更新 (タイマー)

 	if(update & 1)
		mWidgetTimerAdd_ifnothave(MLK_WIDGET(p), 0, 5, 0);

	//エディットの値更新

	if(update & 2)
		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, TRANSFORM_NOTIFY_CHANGE_AFFINE_PARAM, 0, 0);
	
	//座標

	p->pt_last_canv.x = x;
	p->pt_last_canv.y = y;
}

/* 移動時 (キャンバス移動) */

static void _event_motion_canvasmove(TransformView *p,int x,int y)
{
	int sx,sy;

	sx = p->canv_scrx + p->pt_last_canv.x - x;
	sy = p->canv_scry + p->pt_last_canv.y - y;

	//セット

	if(sx != p->canv_scrx || sy != p->canv_scry)
	{
		p->canv_scrx = sx;
		p->canv_scry = sy;

		TransformView_update(p, TRANSFORM_UPDATE_F_CANVAS);
	}

	//

	p->pt_last_canv.x = x;
	p->pt_last_canv.y = y;
}

/* 移動時 (キャンバス拡大縮小) */

static void _event_motion_canvaszoom(TransformView *p,int y)
{
	int val,n;

	if(y == p->pt_last_canv.y) return;

	val = p->canv_zoom;

	if(y < p->pt_last_canv.y)
	{
		//拡大

		n = (int)(val * 1.1);
		if(n == val) n++;

		if(n > TRANSFORM_ZOOM_MAX) n = TRANSFORM_ZOOM_MAX;
	}
	else
	{
		n = (int)(val * 0.9);
		if(n == val) n--;

		if(n < 1) n = 1;
	}

	//

	if(n != val)
	{
		TransformView_setCanvasZoom(p, n);

		mWidgetEventAdd_notify(MLK_WIDGET(p), NULL, TRANSFORM_NOTIFY_CHANGE_CANVAS_ZOOM, 0, 0);
	}

	p->pt_last_canv.y = y;
}

/* 押し時 */

static void _event_press(TransformView *p,mEvent *ev)
{
	int is_right,type,cursor = -1;

	is_right = (ev->pt.btt == MLK_BTT_RIGHT);

	if(is_right && (ev->pt.state & MLK_STATE_CTRL))
	{
		//Ctrl+右: キャンバス拡大縮小

		type = _DRAGF_ZOOM_CANVAS;
		cursor = APPCURSOR_ZOOM_DRAG;
		
		TransformView_startCanvasZoomDrag(p);
	}
	else if(is_right || ev->pt.btt == MLK_BTT_MIDDLE)
	{
		//右ボタン or 中ボタン: キャンバス移動 (ドラッグ中低品質)

		type = _DRAGF_MOVE_CANVAS;
		cursor = APPCURSOR_HAND_GRAB;

		p->low_quality = TRUE;
	}
	else if(ev->pt.btt == MLK_BTT_LEFT && p->cur_area != TRANSFORM_DSTAREA_NONE)
	{
		//変形用の操作

		type = p->cur_area;
		
		switch(type)
		{
			//平行移動
			case TRANSFORM_DSTAREA_IN_IMAGE:
				cursor = APPCURSOR_HAND_GRAB;
				break;
			//アフィン変換:回転 (イメージ範囲外時)
			// dtmp[0] : 前回の角度
			case TRANSFORM_DSTAREA_OUT_IMAGE:
				p->dtmp[0] = TransformView_affine_get_canvas_angle(p, ev->pt.x, ev->pt.y);
				break;
			//4隅の点 (アフィン変換)
			// dtmp[0,1] : 操作点のソース座標(x,y)
			// dtmp[2,3] : 開始時の逆倍率
			default:
				if(p->type == TRANSFORM_TYPE_NORMAL)
					TransformView_onPress_affineZoom(p, ev->pt.x, ev->pt.y);
				break;
		}
	}
	else
		return;

	//グラブ

	p->fdrag = type;
	p->dragbtt = ev->pt.btt;

	p->pt_last_canv.x = ev->pt.x;
	p->pt_last_canv.y = ev->pt.y;

	mWidgetGrabPointer(MLK_WIDGET(p));

	//カーソル
	
	p->cursor_restore = p->wg.cursor;

	if(cursor != -1)
		mWidgetSetCursor(MLK_WIDGET(p), AppCursor_getForDrag(cursor));
}

/** グラブ解放
 *
 * ev: フォーカスアウト時は NULL */

static void _ungrab(TransformView *p,mEvent *ev)
{
	if(p->fdrag)
	{
		//カーソル戻す

		mWidgetSetCursor(MLK_WIDGET(p), p->cursor_restore);
	
		//キャンバス移動/ズーム時
	
		if(p->fdrag == _DRAGF_MOVE_CANVAS || p->fdrag == _DRAGF_ZOOM_CANVAS)
		{
			p->low_quality = FALSE;
			
			mWidgetRedraw(MLK_WIDGET(p));

			//カーソル下のエリア再セット

			if(ev)
				TransformView_onMotion_nodrag(p, ev->pt.x, ev->pt.y);
		}

		//グラブ解除

		p->fdrag = 0;
		
		mWidgetUngrabPointer();

		//タイマーが残っている場合、更新

		if(mWidgetTimerDelete(MLK_WIDGET(p), 0))
			TransformView_update(p, TRANSFORM_UPDATE_F_TRANSPARAM);
	}
}


//=======================
// ハンドラ main
//=======================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	TransformView *p = (TransformView *)wg;

	switch(ev->type)
	{
		//ポインタ
		case MEVENT_POINTER:
			if(ev->pt.act == MEVENT_POINTER_ACT_MOTION)
			{
				if(p->fdrag == TRANSFORM_DSTAREA_NONE)
					//非ドラッグ時の移動 (カーソル変更)
					TransformView_onMotion_nodrag(p, ev->pt.x, ev->pt.y);
				else if(p->fdrag == _DRAGF_MOVE_CANVAS)
					//キャンバススクロール
					_event_motion_canvasmove(p, ev->pt.x, ev->pt.y);
				else if(p->fdrag == _DRAGF_ZOOM_CANVAS)
					//キャンバス拡大縮小
					_event_motion_canvaszoom(p, ev->pt.y);
				else
					//イメージ操作
					_event_motion(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_PRESS)
			{
				if(!p->fdrag)
					_event_press(p, ev);
			}
			else if(ev->pt.act == MEVENT_POINTER_ACT_RELEASE)
			{
				if(p->dragbtt == ev->pt.btt)
					_ungrab(p, ev);
			}
			break;
	
		//タイマー (変形操作中)
		case MEVENT_TIMER:
			mWidgetTimerDelete(wg, 0);

			TransformView_update(p, TRANSFORM_UPDATE_F_TRANSPARAM);
			break;

		case MEVENT_FOCUS:
			if(ev->focus.is_out)
				_ungrab(p, NULL);
			break;
	}

	return 1;
}

/* 描画ハンドラ */

static void _draw_handle(mWidget *wg,mPixbuf *pixbuf)
{
	TransformView *p = (TransformView *)wg;
	CanvasDrawInfo *di = &p->canvinfo;
	mBox box;

	//キャンバスイメージ描画

	di->boxdst.w = wg->w;
	di->boxdst.h = wg->h;
	di->scrollx = p->canv_scrx - (wg->w >> 1);
	di->scrolly = p->canv_scry - (wg->h >> 1);
	di->originx = p->canv_origx;
	di->originy = p->canv_origy;

	if(!p->low_quality && p->canv_zoom != 1000 && p->canv_zoom < 2000)
		//200%以下 (100% は除く) 時
		ImageCanvas_drawPixbuf_oversamp(APPDRAW->imgcanvas, pixbuf, di);
	else 
		ImageCanvas_drawPixbuf_nearest(APPDRAW->imgcanvas, pixbuf, di);

	//変形後イメージ

	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		TileImage_transformAffine_preview(p->imgcopy, pixbuf, &p->boxsrc,
			p->scalex_div, p->scaley_div, p->dcos, p->dsin, &p->pt_mov, di);
	}
	else
	{
		if(TransformView_homog_getDstCanvasBox(p, &box))
		{
			TileImage_transformHomography_preview(p->imgcopy, pixbuf, &box,
				p->homog_param, &p->pt_mov,
				p->boxsrc.x, p->boxsrc.y, p->boxsrc.w, p->boxsrc.h, di);
		}
	}

	//XOR 枠

	_draw_xorframe(p);

	TileImage_blendXor_pixbuf(p->imgxor, pixbuf, &di->boxdst);

	//外枠

	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, 0);
}

/* サイズ変更時 */

static void _resize_handle(mWidget *wg)
{
	TransformView *p = (TransformView *)wg;

	TileImage_resizeTileBuf_maxsize(p->imgxor, wg->w, wg->h);

	_change_canvas_param(p);
}


//=========================
// 初期化
//=========================


/* 射影変換の src 位置セット */

static void _set_corner_src(TransformView *p)
{
	mDoublePoint *ppt = p->dpt_corner_src;
	double dx1,dy1;
	mRect rc;

	mRectSetBox(&rc, &p->boxsrc);
	rc.x2++;
	rc.y2++;

	//[!] 座標値が 0 だと正しく計算できないので、少しずらす

	dx1 = (rc.x1 == 0)? 0.001: rc.x1;
	dy1 = (rc.y1 == 0)? 0.001: rc.y1;

	ppt[0].x = dx1;
	ppt[0].y = dy1;
	ppt[1].x = rc.x2;
	ppt[1].y = dy1;
	ppt[2].x = rc.x2;
	ppt[2].y = rc.y2;
	ppt[3].x = dx1;
	ppt[3].y = rc.y2;
}

/* 破棄ハンドラ */

static void _destroy_handle(mWidget *wg)
{
	TileImage_free(((TransformView *)wg)->imgxor);
}

/** 作成 */

TransformView *TransformView_create(mWidget *parent,int wid)
{
	TransformView *p;
	CanvasDrawInfo *di;

	//作成 (初期サイズは前回のサイズ)

	p = (TransformView *)mWidgetNew(parent, sizeof(TransformView));

	p->wg.id = wid;
	p->wg.flayout = MLF_EXPAND_WH;
	p->wg.fevent |= MWIDGET_EVENT_POINTER;
	p->wg.destroy = _destroy_handle;
	p->wg.event = _event_handle;
	p->wg.draw = _draw_handle;
	p->wg.resize = _resize_handle;
	p->wg.initW = APPCONF->transform.view_w;
	p->wg.initH = APPCONF->transform.view_h;

	//

	p->canv_zoom = 1000;
	p->boxsrc = APPDRAW->boxsel.box;

	p->centerx = p->boxsrc.w * 0.5;
	p->centery = p->boxsrc.h * 0.5;

	p->canv_origx = p->boxsrc.x + p->centerx;
	p->canv_origy = p->boxsrc.y + p->centery;

	p->keep_aspect = ((APPCONF->transform.flags & CONFIG_TRANSFORM_F_KEEP_ASPECT) != 0);

	p->viewparam.scale = p->viewparam.scalediv = 1;

	//XOR 用イメージ作成

	p->imgxor = TileImage_new(TILEIMAGE_COLTYPE_ALPHA1BIT, p->wg.initW, p->wg.initH);
	if(!(p->imgxor))
	{
		mWidgetDestroy(MLK_WIDGET(p));
		return NULL;
	}

	//CanvasDrawInfo (キャンバス描画情報)

	di = &p->canvinfo;

	mMemset0(di, sizeof(CanvasDrawInfo));

	di->imgw = APPDRAW->imgw;
	di->imgh = APPDRAW->imgh;
	di->bkgndcol = APPCONF->canvasbkcol;
	di->param = &p->viewparam;

	//射影変換ソース位置

	_set_corner_src(p);

	//パラメータ値リセット

	TransformView_resetParam(p);

	return p;
}

/** 初期化
 *
 * 矩形範囲に合わせて、表示倍率セット */

void TransformView_init(TransformView *p)
{
	double dh,dv;
	int n;

	//表示倍率: 矩形範囲全体が表示できる状態に (縮小のみ)

	dh = (double)p->wg.initW / (p->boxsrc.w * 2);
	dv = (double)p->wg.initH / (p->boxsrc.h * 2);

	if(dh > dv) dh = dv;
	if(dh > 1) dh = 1;

	n = (int)(dh * 1000 + 0.5);
	if(n < 1) n = 1;

	TransformView_setCanvasZoom(p, n);

	//変形パラメータ作業用

	_change_transform_param(p);
}


//============================
// ダイアログからも使われる
//============================


/** 表示倍率セット */

void TransformView_setCanvasZoom(TransformView *p,int zoom)
{
	p->canv_zoom = zoom;

	p->viewparam.scale = zoom * 0.001;
	p->viewparam.scalediv = 1.0 / p->viewparam.scale;

	TransformView_update(p, TRANSFORM_UPDATE_F_CANVAS);
}

/** 変形パラメータ値をリセット */

void TransformView_resetParam(TransformView *p)
{
	p->pt_mov.x = p->pt_mov.y = 0;

	//アフィン変換

	p->scalex = p->scaley = 1;
	p->angle = 0;

	//射影変換: 4隅のイメージ位置

	memcpy(p->dpt_corner_dst, p->dpt_corner_src, sizeof(mDoublePoint) * 4);
}

/** パラメータ更新 + ウィジェット更新 */

void TransformView_update(TransformView *p,uint8_t flags)
{
	//キャンバス更新時
	
	if(flags & TRANSFORM_UPDATE_F_CANVAS)
		_change_canvas_param(p);

	//変形パラメータ変更時

	if(flags & TRANSFORM_UPDATE_F_TRANSPARAM)
		_change_transform_param(p);

	mWidgetRedraw(MLK_WIDGET(p));
}

/** キャンバス拡大縮小のドラッグ開始時 */

void TransformView_startCanvasZoomDrag(TransformView *p)
{
	mDoublePoint pt;

	p->low_quality = 1;

	//イメージ原点の変更

	TransformView_canv_to_image_dpt(p, &pt, p->wg.w * 0.5, p->wg.h * 0.5);

	p->canv_origx = pt.x;
	p->canv_origy = pt.y;
	p->canv_scrx = p->canv_scry = 0;

	_change_canvas_param(p);
}

