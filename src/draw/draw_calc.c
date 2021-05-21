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
 * AppDraw: 計算関連
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mlk_gui.h"
#include "mlk_rectbox.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw.h"

#include "tileimage.h"

#include "draw_calc.h"



/** 現在の倍率&角度から、キャンバス用パラメータセット */

void drawCalc_setCanvasViewParam(AppDraw *p)
{
	double d;

	d = p->canvas_zoom * 0.001;

	p->viewparam.scale = d;
	p->viewparam.scalediv = 1.0 / d;

	d = p->canvas_angle * MLK_MATH_PI / 18000.0;

	p->viewparam.rd = d;
	p->viewparam.cos = cos(d);
	p->viewparam.sin = sin(d);
	p->viewparam.cosrev = p->viewparam.cos;
	p->viewparam.sinrev = -(p->viewparam.sin);
}

/** 指定位置がキャンバスの範囲内か */

mlkbool drawCalc_isPointInCanvasArea(AppDraw *p,mPoint *pt)
{
	return (pt->x >= 0 && pt->x < p->canvas_size.w
		&& pt->y >= 0 && pt->y < p->canvas_size.h);
}


//==============================
// 座標変換
//==============================


/** キャンバス座標 (double) -> イメージ位置 (double) */

void drawCalc_canvas_to_image_double(AppDraw *p,double *dx,double *dy,double x,double y)
{
	double xx,yy;

	x += p->canvas_scroll.x - (p->canvas_size.w >> 1);
	y += p->canvas_scroll.y - (p->canvas_size.h >> 1);

	xx = x * p->viewparam.cosrev - y * p->viewparam.sinrev;
	yy = x * p->viewparam.sinrev + y * p->viewparam.cosrev;

	if(p->canvas_mirror) xx = -xx;

	*dx = xx * p->viewparam.scalediv + p->imgorigin.x;
	*dy = yy * p->viewparam.scalediv + p->imgorigin.y;
}

/** キャンバス座標 (mPoint) -> イメージ位置 (mDoublePoint) */

void drawCalc_canvas_to_image_double_pt(AppDraw *p,mDoublePoint *dst,mPoint *src)
{
	drawCalc_canvas_to_image_double(p, &dst->x, &dst->y, src->x, src->y);
}

/** キャンバス座標 (double) -> イメージ位置 (mPoint) */

void drawCalc_canvas_to_image_pt(AppDraw *p,mPoint *pt,double x,double y)
{
	drawCalc_canvas_to_image_double(p, &x, &y, x, y);

	pt->x = floor(x);
	pt->y = floor(y);
}

/** キャンバス座標における相対値を、イメージ座標における相対値に変換 */

void drawCalc_canvas_to_image_relative(AppDraw *p,mPoint *dst,mPoint *src)
{
	dst->x = (int)((src->x * p->viewparam.cosrev - src->y * p->viewparam.sinrev) * p->viewparam.scalediv);
	dst->y = (int)((src->x * p->viewparam.sinrev + src->y * p->viewparam.cosrev) * p->viewparam.scalediv);

	if(p->canvas_mirror)
		dst->x = -(dst->x);
}


//---------------


/** イメージ位置 (int) -> キャンバス座標 (mPoint) */

void drawCalc_image_to_canvas_pt(AppDraw *p,mPoint *dst,int x,int y)
{
	double dx,dy;

	dx = (x - p->imgorigin.x) * p->viewparam.scale;
	dy = (y - p->imgorigin.y) * p->viewparam.scale;

	if(p->canvas_mirror) dx = -dx;

	dst->x = (int)(dx * p->viewparam.cos - dy * p->viewparam.sin + (p->canvas_size.w >> 1) - p->canvas_scroll.x);
	dst->y = (int)(dx * p->viewparam.sin + dy * p->viewparam.cos + (p->canvas_size.h >> 1) - p->canvas_scroll.y);
}

/** イメージ位置 (double) -> キャンバス座標 (mPoint) */

void drawCalc_image_to_canvas_pt_double(AppDraw *p,mPoint *dst,double x,double y)
{
	x = (x - p->imgorigin.x) * p->viewparam.scale;
	y = (y - p->imgorigin.y) * p->viewparam.scale;

	if(p->canvas_mirror) x = -x;

	dst->x = (int)(x * p->viewparam.cos - y * p->viewparam.sin + (p->canvas_size.w >> 1) - p->canvas_scroll.x);
	dst->y = (int)(x * p->viewparam.sin + y * p->viewparam.cos + (p->canvas_size.h >> 1) - p->canvas_scroll.y);
}


//------------


/** キャンバス中央におけるイメージ位置取得 */

void drawCalc_getImagePos_atCanvasCenter(AppDraw *p,double *px,double *py)
{
	drawCalc_canvas_to_image_double(p, px, py,
		p->canvas_size.w * 0.5, p->canvas_size.h * 0.5);
}

/** イメージ座標 (mBox) -> キャンバス座標 (mBox)
 *
 * return: FALSE で範囲外 */

mlkbool drawCalc_image_to_canvas_box(AppDraw *p,mBox *dst,const mBox *src)
{
	mPoint pt[4];
	int x1,y1,x2,y2,i;

	//右・下端は+1

	x1 = src->x, y1 = src->y;
	x2 = x1 + src->w;
	y2 = y1 + src->h;

	//最小・最大値

	if(p->canvas_angle == 0)
	{
		drawCalc_image_to_canvas_pt(p, pt, x1, y1);
		drawCalc_image_to_canvas_pt(p, pt + 1, x2, y2);

		if(pt[0].x < pt[1].x)
			x1 = pt[0].x, x2 = pt[1].x;
		else
			x1 = pt[1].x, x2 = pt[0].x;

		if(pt[0].y < pt[1].y)
			y1 = pt[0].y, y2 = pt[1].y;
		else
			y1 = pt[1].y, y2 = pt[0].y;
	}
	else
	{
		drawCalc_image_to_canvas_pt(p, pt    , x1, y1);
		drawCalc_image_to_canvas_pt(p, pt + 1, x2, y1);
		drawCalc_image_to_canvas_pt(p, pt + 2, x1, y2);
		drawCalc_image_to_canvas_pt(p, pt + 3, x2, y2);

		x1 = x2 = pt[0].x;
		y1 = y2 = pt[0].y;

		for(i = 1; i < 4; i++)
		{
			if(x1 > pt[i].x) x1 = pt[i].x;
			if(y1 > pt[i].y) y1 = pt[i].y;
			if(x2 < pt[i].x) x2 = pt[i].x;
			if(y2 < pt[i].y) y2 = pt[i].y;
		}
	}

	//念のため範囲拡張

	x1--, y1--;
	x2++, y2++;

	//範囲外判定

	if(x2 < 0 || y2 < 0 || x1 >= p->canvas_size.w || y1 >= p->canvas_size.h)
		return FALSE;

	//調整

	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= p->canvas_size.w) x2 = p->canvas_size.w - 1;
	if(y2 >= p->canvas_size.h) y2 = p->canvas_size.h - 1;

	//

	dst->x = x1, dst->y = y1;
	dst->w = x2 - x1 + 1;
	dst->h = y2 - y1 + 1;

	return TRUE;
}

/** キャンバス座標 (mBox) -> イメージ座標 (mBox)
 *
 * return: FALSE でイメージの範囲外 */

mlkbool drawCalc_canvas_to_image_box(AppDraw *p,mBox *dst,const mBox *src)
{
	mPoint pt[4];
	mRect rc;
	int i;

	//四隅をイメージ位置に変換

	drawCalc_canvas_to_image_pt(p, pt    , src->x - 1, src->y - 1);
	drawCalc_canvas_to_image_pt(p, pt + 1, src->x + src->w, src->y - 1);
	drawCalc_canvas_to_image_pt(p, pt + 2, src->x - 1, src->y + src->h);
	drawCalc_canvas_to_image_pt(p, pt + 3, src->x + src->w, src->y + src->h);

	//範囲

	rc.x1 = rc.x2 = pt[0].x;
	rc.y1 = rc.y2 = pt[0].y;

	for(i = 1; i < 4; i++)
	{
        if(pt[i].x < rc.x1) rc.x1 = pt[i].x;
        if(pt[i].y < rc.y1) rc.y1 = pt[i].y;
        if(pt[i].x > rc.x2) rc.x2 = pt[i].x;
        if(pt[i].y > rc.y2) rc.y2 = pt[i].y;
	}

	//イメージ範囲調整

	return drawCalc_image_rect_to_box(p, dst, &rc);
}


//==============================
// 範囲
//==============================


/** mRect をキャンバスの範囲内にクリッピングして mBox へ
 *
 * dst: 範囲がない場合、w = h = 0 となる。
 * return: FALSE で範囲外 */

mlkbool drawCalc_clipCanvas_toBox(AppDraw *p,mBox *dst,mRect *src)
{
	int x1,y1,x2,y2;

	if(src->x1 > src->x2
		|| src->y1 > src->y2
		|| src->x2 < 0 || src->y2 < 0
		|| src->x1 >= p->canvas_size.w || src->y1 >= p->canvas_size.h)
	{
		//範囲外
		dst->w = dst->h = 0;
		return FALSE;
	}
	else
	{
		x1 = (src->x1 < 0)? 0: src->x1;
		y1 = (src->y1 < 0)? 0: src->y1;
		x2 = (src->x2 >= p->canvas_size.w)? p->canvas_size.w - 1: src->x2;
		y2 = (src->y2 >= p->canvas_size.h)? p->canvas_size.h - 1: src->y2;

		dst->x = x1, dst->y = y1;
		dst->w = x2 - x1 + 1;
		dst->h = y2 - y1 + 1;

		return TRUE;
	}
}

/** mRect をイメージ範囲内にクリッピング
 *
 * return: FALSE で範囲外 */

mlkbool drawCalc_clipImageRect(AppDraw *p,mRect *rc)
{
	int x1,y1,x2,y2;

	if(mRectIsEmpty(rc)) return FALSE;

	x1 = rc->x1, y1 = rc->y1;
	x2 = rc->x2, y2 = rc->y2;

	//範囲外

	if(x2 < 0 || y2 < 0 || x1 >= p->imgw || y1 >= p->imgh)
		return FALSE;

	//調整

	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= p->imgw) x2 = p->imgw - 1;
	if(y2 >= p->imgh) y2 = p->imgh - 1;

	//セット

	rc->x1 = x1, rc->y1 = y1;
	rc->x2 = x2, rc->y2 = y2;

	return TRUE;
}

/** mRect (イメージ範囲) を、範囲内に収めて mBox へ
 *
 * return: FALSE で範囲外、または範囲が空 */

mlkbool drawCalc_image_rect_to_box(AppDraw *p,mBox *dst,const mRect *rc)
{
	mRect rc2;

	rc2 = *rc;

	if(!drawCalc_clipImageRect(p, &rc2))
	{
		dst->w = dst->h = 0;
		return FALSE;
	}
	else
	{
		dst->x = rc2.x1;
		dst->y = rc2.y1;
		dst->w = rc2.x2 - rc2.x1 + 1;
		dst->h = rc2.y2 - rc2.y1 + 1;

		return TRUE;
	}
}

/** mBox (イメージ座標) の範囲を結合
 *
 * x < 0 の場合は範囲なしとする。
 * 
 * return: FALSE で両方共範囲なし */

mlkbool drawCalc_unionImageBox(mBox *dst,mBox *src)
{
	if(dst->x < 0 && src->x < 0)
		//両方共範囲なし
		return FALSE;
	else if(src->x < 0)
		//src が範囲なし => dst は変化なし
		return TRUE;
	else if(dst->x < 0)
	{
		//dst が範囲なし => src をコピー

		*dst = *src;
		return TRUE;
	}
	else
	{
		//範囲合成

		mBoxUnion(dst, src);
		return TRUE;
	}
}

/** [src] + [src を mx,my 分相対移動した範囲] を合成して dst にセット */

void drawCalc_unionRect_relmove(mRect *dst,mRect *src,int mx,int my)
{
	*dst = *src;
	mRectMove(dst, mx, my);

	mRectUnion(dst, src);
}


//=============================
// 描画関連
//=============================


/** 直線を45度単位に調整
 *
 * pt: 対象となる点。結果はここに入る。
 * ptstart: 始点 */

void drawCalc_fitLine45(mPoint *pt,mPoint *ptstart)
{
	int dx,dy,adx,ady,n;

	dx = pt->x - ptstart->x;
	dy = pt->y - ptstart->y;

	adx = abs(dx);
	ady = abs(dy);

	n = (adx > ady)? adx: ady;

	if(abs(adx - ady) < n / 2)
	{
		//45度線

		if(adx < ady)
			pt->y = ptstart->y + ((dy < 0)? -adx: adx);
		else
			pt->x = ptstart->x + ((dx < 0)? -ady: ady);
	}
	else if(adx > ady)
		//水平
		pt->y = ptstart->y;
	else
		//垂直
		pt->x = ptstart->x;
}

/** pttmp[0,1] から、線の角度をラジアンで取得
 *
 * イメージ位置に対する角度。反時計回り */

double drawCalc_getLineRadian_forImage(AppDraw *p)
{
	double x1,y1,x2,y2;

	drawCalc_canvas_to_image_double(p, &x1, &y1, p->w.pttmp[0].x, p->w.pttmp[0].y);
	drawCalc_canvas_to_image_double(p, &x2, &y2, p->w.pttmp[1].x, p->w.pttmp[1].y);

	return atan2(y2 - y1, x2 - x1);
}

/** イメージ移動時の計算
 *
 * img: 相対移動数を計算するためのイメージ
 * state: +Shift で X 移動、+Ctrl で Y 移動。
 * ptret: 相対移動数が入る
 * return: 1px でも移動するか */

mlkbool drawCalc_moveImage_onMotion(AppDraw *p,TileImage *img,uint32_t state,mPoint *ptret)
{
	mPoint pt,pt2;
	double x,y;

	//カーソル移動距離

	x = p->w.dpt_canv_cur.x - p->w.dpt_canv_last.x;
	y = p->w.dpt_canv_cur.y - p->w.dpt_canv_last.y;

	if(state & MLK_STATE_CTRL)  x = 0;
	if(state & MLK_STATE_SHIFT) y = 0;

	//総移動数に加算

	p->w.ptd_tmp[0].x += x;
	p->w.ptd_tmp[0].y += y;

	//総移動数を キャンバス -> イメージ 変換

	pt2.x = (int)p->w.ptd_tmp[0].x;
	pt2.y = (int)p->w.ptd_tmp[0].y;

	drawCalc_canvas_to_image_relative(p, &pt, &pt2);

	//移動開始時のオフセット位置を加算して絶対値に

	pt.x += p->w.pttmp[0].x;
	pt.y += p->w.pttmp[0].y;

	//現在位置からの相対移動数

	TileImage_getOffset(img, &pt2);

	pt.x -= pt2.x;
	pt.y -= pt2.y;

	if(pt.x == 0 && pt.y == 0)
		return FALSE;
	else
	{
		//総相対移動数
		p->w.pttmp[1].x += pt.x;
		p->w.pttmp[1].y += pt.y;
	
		*ptret = pt;

		return TRUE;
	}
}


//==============================
// ほか
//==============================


/** 1段階上げ/下げした表示倍率を取得 */

int drawCalc_getZoom_step(AppDraw *p,mlkbool zoomup)
{
	int n,val,step;

	n = p->canvas_zoom / 10;

	if((!zoomup && n <= 100) || (zoomup && n < 100))
	{
		//----- 100% 以下時
		// 拡大時は現在値の 115%、縮小時は 85%

		val = p->canvas_zoom;

		if(zoomup)
		{
			n = (int)(val * 1.15 + 0.5);
			if(n > 1000) n = 1000;

			if(n == val) n = val + 1;
		}
		else
		{
			n = (int)(val * 0.85 + 0.5);
			if(n == val) n = val - 1;
		}
	}
	else
	{
		//---- 100% 以上時

		step = APPCONF->canvas_zoom_step_hi;

		if(zoomup)
			n = (n + step) / step * step;
		else
		{
			n = (n - 1) / step * step;
			if(n < 100) n = 100;
		}

		n *= 10;
	}

	//

	if(n < CANVAS_ZOOM_MIN)
		n = CANVAS_ZOOM_MIN;
	else if(n > CANVAS_ZOOM_MAX)
		n = CANVAS_ZOOM_MAX;

	return n;
}

/** ウィンドウに収まる表示倍率を取得
 *
 * 最大で 100% */

int drawCalc_getZoom_fitWindow(AppDraw *p)
{
	mBox box;
	int w,h;

	//領域の余白 10px 分を引く

	if(p->canvas_size.w < 20 || p->canvas_size.h < 20)
		w = h = 21;
	else
	{
		w = p->canvas_size.w - 20;
		h = p->canvas_size.h - 20;
	}

	//

	box.x = box.y = 0;
	box.w = p->imgw, box.h = p->imgh;

	mBoxResize_keepaspect(&box, w, h, TRUE);

	//倍率

	return (int)((double)box.w / p->imgw * 1000 + 0.5);
}

/** キャンバスのスクロールバーの最大値取得
 *
 * 四隅を[イメージ座標 -> キャンバス座標]変換し、一番大きい半径の値+余白 */

void drawCalc_getCanvasScrollMax(AppDraw *p,mSize *size)
{
	mPoint pt[4];
	int i,r,rmax = 0;
	double x,y;

	//イメージの四隅をキャンバス座標に変換。
	//キャンバス中央からの距離の最大値を取得。

	mMemset0(pt, sizeof(mPoint) * 4);

	pt[1].x = pt[3].x = p->imgw;
	pt[2].y = pt[3].y = p->imgh;

	for(i = 0; i < 4; i++)
	{
		drawCalc_image_to_canvas_pt(p, pt + i, pt[i].x, pt[i].y);

		x = pt[i].x - (p->canvas_size.w >> 1);
		y = pt[i].y - (p->canvas_size.h >> 1);

		r = (int)(sqrt(x * x + y * y) + 0.5);
		if(rmax < r) rmax = r;
	}

	//

	size->w = rmax * 2 + p->canvas_size.w;
	size->h = rmax * 2 + p->canvas_size.h;
}


//===============================
// CanvasDrawInfo
//===============================


/** イメージ座標からキャンバス座標に変換 */

void CanvasDrawInfo_image_to_canvas(CanvasDrawInfo *p,double x,double y,double *px,double *py)
{
	x = (x - p->originx) * p->param->scale;
	y = (y - p->originy) * p->param->scale;
	if(p->mirror) x = -x;

	*px = (x * p->param->cos - y * p->param->sin) - p->scrollx;
	*py = (x * p->param->sin + y * p->param->cos) - p->scrolly;
}

/** イメージ座標からキャンバス座標に変換 (=> mPoint) */

void CanvasDrawInfo_image_to_canvas_pt(CanvasDrawInfo *p,double x,double y,mPoint *pt)
{
	x = (x - p->originx) * p->param->scale;
	y = (y - p->originy) * p->param->scale;
	if(p->mirror) x = -x;

	pt->x = floor((x * p->param->cos - y * p->param->sin) - p->scrollx);
	pt->y = floor((x * p->param->sin + y * p->param->cos) - p->scrolly);
}

/** イメージ座標を +1 した時のキャンバス座標の増加幅を取得
 *
 * dst: [0]x-x, [1]x-y, [2]y-x, [3]y-y */

void CanvasDrawInfo_getImageIncParam(CanvasDrawInfo *p,double *dst)
{
	dst[0] = p->param->scale * p->param->cos;
	dst[1] = p->param->scale * p->param->sin;
	dst[2] = -dst[1];
	dst[3] = dst[0];

	if(p->mirror)
	{
		dst[0] = -dst[0];
		dst[1] = -dst[1];
	}
}

