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

/***********************************************
 * 変形ダイアログ:サブ関数
 ***********************************************/

#include <math.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_rectbox.h"

#include "def_draw.h"
#include "appcursor.h"

#include "pv_transformdlg.h"


//==============================
// イメージ/キャンバス座標変換
//==============================


/** イメージ座標 -> キャンバス座標変換 */

void TransformView_image_to_canv_pt(TransformView *p,mPoint *pt,double x,double y)
{
	pt->x = floor((x - p->canv_origx) * p->viewparam.scale - p->canv_scrx + (p->wg.w >> 1));
	pt->y = floor((y - p->canv_origy) * p->viewparam.scale - p->canv_scry + (p->wg.h >> 1));
}

/** キャンバス座標 -> イメージ座標変換 */

void TransformView_canv_to_image_dpt(TransformView *p,mDoublePoint *pt,double x,double y)
{
	pt->x = (x + p->canv_scrx - (p->wg.w >> 1)) * p->viewparam.scalediv + p->canv_origx;
	pt->y = (y + p->canv_scry - (p->wg.h >> 1)) * p->viewparam.scalediv + p->canv_origy;
}


//==============================
// アフィン変換の計算
//==============================


/** ソース画像座標 -> アフィン変換 -> キャンバス座標 */

void TransformView_affine_source_to_canv_pt(TransformView *p,mPoint *pt,double x,double y)
{
	double xx,yy;

	x = (x - p->centerx) * p->scalex;
	y = (y - p->centery) * p->scaley;

	xx = (x * p->dcos - y * p->dsin) + p->centerx + p->boxsrc.x + p->pt_mov.x;
	yy = (x * p->dsin + y * p->dcos) + p->centery + p->boxsrc.y + p->pt_mov.y;

	TransformView_image_to_canv_pt(p, pt, xx, yy);
}

/** キャンバス座標 -> ソース画像座標 (mDoublePoint) */

void TransformView_affine_canv_to_source_dpt(TransformView *p,mDoublePoint *ptdst,int x,int y)
{
	mDoublePoint pt;
	double xx,yy;

	//キャンバス -> イメージ座標変換

	TransformView_canv_to_image_dpt(p, &pt, x, y);

	//イメージ座標から逆アフィン変換

	xx = pt.x - p->boxsrc.x - p->pt_mov.x - p->centerx;
	yy = pt.y - p->boxsrc.y - p->pt_mov.y - p->centery;

	ptdst->x = ( xx * p->dcos + yy * p->dsin) * p->scalex_div + p->centerx;
	ptdst->y = (-xx * p->dsin + yy * p->dcos) * p->scaley_div + p->centery;
}

/** キャンバス座標 -> ソース画像座標 (mPoint) */

void TransformView_affine_canv_to_source_pt(TransformView *p,mPoint *ptdst,int x,int y)
{
	mDoublePoint dpt;

	TransformView_affine_canv_to_source_dpt(p, &dpt, x, y);

	ptdst->x = floor(dpt.x);
	ptdst->y = floor(dpt.y);
}

/** カーソル位置とイメージの中央からなる角度を取得 */

double TransformView_affine_get_canvas_angle(TransformView *p,int x,int y)
{
	mDoublePoint dpt;

	//イメージ座標で計算

	TransformView_canv_to_image_dpt(p, &dpt, x, y);

	return atan2(dpt.y - p->pt_mov.y - (p->boxsrc.y + p->centery),
		dpt.x - p->pt_mov.x - (p->boxsrc.x + p->centerx));
}

/** キャンバス座標 -> ソース座標 (4隅の点ドラッグ時) */

void TransformView_affine_canv_to_source_for_drag(TransformView *p,mDoublePoint *ptdst,int x,int y)
{
	mDoublePoint pt;
	double xx,yy;

	//キャンバス -> イメージ座標変換

	TransformView_canv_to_image_dpt(p, &pt, x, y);

	//イメージ座標から逆アフィン変換
	// :ドラッグ開始時の倍率での座標が必要なので、倍率は保存した値を使う

	xx = pt.x - p->boxsrc.x - p->pt_mov.x - p->centerx;
	yy = pt.y - p->boxsrc.y - p->pt_mov.y - p->centery;

	ptdst->x = ( xx * p->dcos + yy * p->dsin) * p->dtmp[2] + p->centerx;
	ptdst->y = (-xx * p->dsin + yy * p->dcos) * p->dtmp[3] + p->centery;
}


//==============================
// 射影変換の計算
//==============================


/* 点と辺のベクトル外積 */

static mlkbool _vector_product(mPoint *pt,mPoint *pt1,mPoint *pt2)
{
	return (((pt2->x - pt1->x) * (pt->y - pt2->y) - (pt->x - pt2->x) * (pt2->y - pt1->y)) >= 0);
}

/* 点と辺のベクトル外積 */

static mlkbool _homog_judge(mDoublePoint *pt,mDoublePoint *pt1,mDoublePoint *pt2)
{
	return (((pt2->x - pt1->x) * (pt->y - pt2->y) - (pt->x - pt2->x) * (pt2->y - pt1->y)) < 0);
}

/* ポイントが変形後イメージの範囲内か */

static mlkbool _homog_isPointInImage(TransformView *p,mPoint *pt)
{
	//0-1-3 の三角形の内側か

	if(_vector_product(pt, p->pt_corner_canv, p->pt_corner_canv + 1)
		&& _vector_product(pt, p->pt_corner_canv + 1, p->pt_corner_canv + 3)
		&& _vector_product(pt, p->pt_corner_canv + 3, p->pt_corner_canv))
		return TRUE;

	//1-2-3 の三角形の内側か

	return (_vector_product(pt, p->pt_corner_canv + 1, p->pt_corner_canv + 2)
		&& _vector_product(pt, p->pt_corner_canv + 2, p->pt_corner_canv + 3)
		&& _vector_product(pt, p->pt_corner_canv + 3, p->pt_corner_canv + 1));
}

/** 変形後の描画範囲を取得 (キャンバス座標) */

mlkbool TransformView_homog_getDstCanvasBox(TransformView *p,mBox *box)
{
	mRect rc;
	int i,x,y;

	rc.x1 = rc.x2 = p->pt_corner_canv[0].x;
	rc.y1 = rc.y2 = p->pt_corner_canv[0].y;

	for(i = 1; i < 4; i++)
	{
		x = p->pt_corner_canv[i].x;
		y = p->pt_corner_canv[i].y;

		if(x < rc.x1) rc.x1 = x;
		else if(x > rc.x2) rc.x2 = x;

		if(y < rc.y1) rc.y1 = y;
		else if(y > rc.y2) rc.y2 = y;
	}

	rc.x1--, rc.y1--;
	rc.x2++, rc.y2++;

	//クリッピング

	if(!mRectClipBox_d(&rc, 0, 0, p->wg.w, p->wg.h))
		return FALSE;
	else
	{
		mBoxSetRect(box, &rc);
		return TRUE;
	}
}

/** 変換後の描画範囲取得 (イメージ座標) */

void TransformView_homog_getDstImageRect(TransformView *p,mRect *rcdst)
{
	mRect rc;
	int i,x,y;

	rc.x1 = rc.x2 = floor(p->dpt_corner_dst[0].x + p->pt_mov.x);
	rc.y1 = rc.y2 = floor(p->dpt_corner_dst[0].y + p->pt_mov.y);

	for(i = 1; i < 4; i++)
	{
		x = floor(p->dpt_corner_dst[i].x + p->pt_mov.x);
		y = floor(p->dpt_corner_dst[i].y + p->pt_mov.y);

		if(x < rc.x1) rc.x1 = x;
		else if(x > rc.x2) rc.x2 = x;

		if(y < rc.y1) rc.y1 = y;
		else if(y > rc.y2) rc.y2 = y;
	}

	rc.x1--, rc.y1--;
	rc.x2++, rc.y2++;

	*rcdst = rc;
}


//=========================
// 操作関連
//=========================


/* カーソル位置が変形後のどの範囲か */

static int _get_point_dstarea(TransformView *p,int x,int y)
{
	int i;
	mPoint pt;

	//4隅の点 (共通)

	for(i = 0; i < 4; i++)
	{
		pt = p->pt_corner_canv[i];

		if(pt.x - 4 <= x && x <= pt.x + 4
			&& pt.y - 4 <= y && y <= pt.y + 4)
			return TRANSFORM_DSTAREA_LEFT_TOP + i;
	}

	//変形後のイメージ範囲内/外

	if(p->type == TRANSFORM_TYPE_NORMAL)
	{
		//アフィン変換
		
		TransformView_affine_canv_to_source_pt(p, &pt, x, y);

		if(pt.x < 0 || pt.y < 0 || pt.x >= p->boxsrc.w || pt.y >= p->boxsrc.h)
			return TRANSFORM_DSTAREA_OUT_IMAGE;
		else
			return TRANSFORM_DSTAREA_IN_IMAGE;
	}
	else
	{
		//射影変換

		pt.x = x;
		pt.y = y;

		return _homog_isPointInImage(p, &pt)? TRANSFORM_DSTAREA_IN_IMAGE: TRANSFORM_DSTAREA_NONE;
	}
}

/** 非ドラッグ時の移動中
 *
 * 位置に合わせてカーソル変更 */

void TransformView_onMotion_nodrag(TransformView *p,int x,int y)
{
	int n,cur;

	n = _get_point_dstarea(p, x, y);

	//エリアが変わったら、カーソル変更

	if(n != p->cur_area)
	{
		p->cur_area = n;

		if(n == TRANSFORM_DSTAREA_NONE)
			//操作対象外
			cur = -1;
		else if(n == TRANSFORM_DSTAREA_IN_IMAGE)
			//移動
			cur = APPCURSOR_HAND;
		else if(n == TRANSFORM_DSTAREA_OUT_IMAGE)
			//回転
			cur = APPCURSOR_ROTATE;
		else
		{
			//4隅
			
			if(p->type == TRANSFORM_TYPE_TRAPEZOID)
				//(射影) 点の移動
				cur = APPCURSOR_MOVE;
			else
			{
				//(アフィン) 拡大縮小
				
				if(n == TRANSFORM_DSTAREA_LEFT_TOP || n == TRANSFORM_DSTAREA_RIGHT_BOTTOM)
					cur = APPCURSOR_LEFT_TOP;
				else
					cur = APPCURSOR_RIGHT_TOP;
			}
		}
		
		mWidgetSetCursor(MLK_WIDGET(p), (cur == -1)? 0: AppCursor_getForDialog(cur));
	}
}

/** [アフィン変換] 4隅の点ドラッグ開始時 */

void TransformView_onPress_affineZoom(TransformView *p,int cx,int cy)
{
	mDoublePoint dpt;

	//押し位置のソース座標

	TransformView_affine_canv_to_source_dpt(p, &dpt, cx, cy);

	p->dtmp[0] = (dpt.x - p->centerx) / p->scalex;
	p->dtmp[1] = (dpt.y - p->centery) / p->scaley;

	//開始時の逆倍率

	p->dtmp[2] = p->scalex_div;
	p->dtmp[3] = p->scaley_div;
}

/** [射影変換] 4隅の点ドラッグ時 */

mlkbool TransformView_onMotion_homog(TransformView *p,int x,int y,uint32_t state)
{
	mDoublePoint pt,*ppt;
	int n;
	mlkbool f = TRUE;

	TransformView_canv_to_image_dpt(p, &pt, x, y);

	pt.x -= p->pt_mov.x;
	pt.y -= p->pt_mov.y;

	ppt = p->dpt_corner_dst;

	//調整

	switch(p->fdrag)
	{
		//左上
		case TRANSFORM_DSTAREA_LEFT_TOP:
			f = (_homog_judge(&pt, ppt + 3, ppt + 1)
				&& _homog_judge(&pt, ppt + 2, ppt + 1)
				&& _homog_judge(&pt, ppt + 3, ppt + 2));
			break;
		//右上
		case TRANSFORM_DSTAREA_RIGHT_TOP:
			f = (_homog_judge(&pt, ppt, ppt + 2)
				&& _homog_judge(&pt, ppt, ppt + 3)
				&& _homog_judge(&pt, ppt + 3, ppt + 2));
			break;
		//右下
		case TRANSFORM_DSTAREA_RIGHT_BOTTOM:
			f = (_homog_judge(&pt, ppt + 1, ppt + 3)
				&& _homog_judge(&pt, ppt + 1, ppt)
				&& _homog_judge(&pt, ppt, ppt + 3));
			break;
		//左下
		default:
			f = (_homog_judge(&pt, ppt + 2, ppt)
				&& _homog_judge(&pt, ppt + 1, ppt)
				&& _homog_judge(&pt, ppt + 2, ppt + 1));
			break;
	}

	//変更

	if(f)
	{
		//座標値が 0 の場合は少しずらす

		if(pt.x == 0) pt.x = 0.001;
		if(pt.y == 0) pt.y = 0.001;

		//セット

		n = p->fdrag - TRANSFORM_DSTAREA_LEFT_TOP;

		if(!(state & MLK_STATE_CTRL))
			p->dpt_corner_dst[n].x = pt.x;
		
		if(!(state & MLK_STATE_SHIFT))
			p->dpt_corner_dst[n].y = pt.y;
	}

	return f;
}


//=========================
// 射影変換の計算
//=========================


/* 4x4 逆行列にする */

static void _mat_reverse(double *mat)
{
	int i,j,k,n,ni;
	double d,src[16];

	memcpy(src, mat, sizeof(double) * 16);

	for(i = 0, n = 0; i < 4; i++)
		for(j = 0; j < 4; j++, n++)
			mat[n] = (i == j);

	for(i = 0; i < 4; i++)
	{
		ni = i << 2;

		//[!] ここで、src の値が 0 だと正しく計算できないので、
		// 座標値が 0 にならないようにすること。

		d = 1 / src[ni + i];

		for(j = 0, n = ni; j < 4; j++, n++)
		{
			src[n] *= d;
			mat[n] *= d;
		}

		for(j = 0; j < 4; j++)
		{
			if(i != j)
			{
				d = src[(j << 2) + i];

				for(k = 0, n = j << 2; k < 4; k++, n++)
				{
					src[n] -= src[ni + k] * d;
					mat[n] -= mat[ni + k] * d;
				}
			}
		}
	}
}

/* xy 共通計算 */

static void _calc_xy(double *dst,double *t,
	double d1,double d2,double d3,double d4)
{
	_mat_reverse(t);

	dst[0] = t[0] * d1 + t[1] * d2 + t[2] * d3 + t[3] * d4;
	dst[1] = t[0] + t[1] + t[2] + t[3];

	dst[2] = t[4] * d1 + t[5] * d2 + t[6] * d3 + t[7] * d4;
	dst[3] = t[4] + t[5] + t[6] + t[7];

	dst[4] = t[8] * d1 + t[9] * d2 + t[10] * d3 + t[11] * d4;
	dst[5] = t[8] + t[9] + t[10] + t[11];

	dst[6] = t[12] * d1 + t[13] * d2 + t[14] * d3 + t[15] * d4;
	dst[7] = t[12] + t[13] + t[14] + t[15];
}

/* 射影変換 逆変換用パラメータ取得
 *
 * dst: 9個の値が入る
 * s,d: 4個の点。s は変形前の4隅の座標。d は変形後の4隅の座標。 */

void getHomographyRevParam(double *dst,const mDoublePoint *s,const mDoublePoint *d)
{
	double t[16],xp[8],yp[8],m,c,f;

	//x

	t[0] = s[0].x, t[1] = s[0].y;
	t[2] = -s[0].x * d[0].x, t[3] = -s[0].y * d[0].x;

	t[4] = s[1].x, t[5] = s[1].y;
	t[6] = -s[1].x * d[1].x, t[7] = -s[1].y * d[1].x;

	t[8] = s[2].x, t[9] = s[2].y;
	t[10] = -s[2].x * d[2].x, t[11] = -s[2].y * d[2].x;

	t[12] = s[3].x, t[13] = s[3].y;
	t[14] = -s[3].x * d[3].x, t[15] = -s[3].y * d[3].x;

	_calc_xy(xp, t, d[0].x, d[1].x, d[2].x, d[3].x);

	//y

	t[0] = s[0].x, t[1] = s[0].y;
	t[2] = -s[0].x * d[0].y, t[3] = -s[0].y * d[0].y;

	t[4] = s[1].x, t[5] = s[1].y;
	t[6] = -s[1].x * d[1].y, t[7] = -s[1].y * d[1].y;

	t[8] = s[2].x, t[9] = s[2].y;
	t[10] = -s[2].x * d[2].y, t[11] = -s[2].y * d[2].y;

	t[12] = s[3].x, t[13] = s[3].y;
	t[14] = -s[3].x * d[3].y, t[15] = -s[3].y * d[3].y;

	_calc_xy(yp, t, d[0].y, d[1].y, d[2].y, d[3].y);

	//

	m = xp[5] * -yp[7] - (-yp[5] * xp[7]);
	m = 1 / m;

	//変換用行列

	c = (-yp[7] * m) * (xp[4] - yp[4]) + (yp[5] * m) * (xp[6] - yp[6]);
	f = (-xp[7] * m) * (xp[4] - yp[4]) + (xp[5] * m) * (xp[6] - yp[6]);

	memset(t, 0, sizeof(double) * 16);

	t[0] = xp[0] - c * xp[1];
	t[1] = xp[2] - c * xp[3];
	t[2] = c;

	t[4] = yp[0] - f * yp[1];
	t[5] = yp[2] - f * yp[3];
	t[6] = f;

	t[8] = xp[4] - c * xp[5];
	t[9] = xp[6] - c * xp[7];
	t[10] = 1;

	t[15] = 1;

	//逆変換用パラメータ

	_mat_reverse(t);

	dst[0] = t[0];
	dst[1] = t[1];
	dst[2] = t[2];
	dst[3] = t[4];
	dst[4] = t[5];
	dst[5] = t[6];
	dst[6] = t[8];
	dst[7] = t[9];
	dst[8] = t[10];
}


