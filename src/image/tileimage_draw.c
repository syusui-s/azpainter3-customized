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
 * TileImage
 * 図形描画
 *****************************************/

#include <stdlib.h>
#include <math.h>

#include "mlk.h"
#include "mlk_rectbox.h"

#include "tileimage.h"
#include "def_tileimage.h"
#include "tileimage_drawinfo.h"
#include "pv_tileimage.h"

#include "canvasinfo.h"
#include "fillpolygon.h"



//=================================

/* クリッピングフラグ取得 */

static int _get_clip_flags(int x,int y,mRect *rcclip)
{
	int flags = 0;

	if(x < rcclip->x1) flags |= 1;
	else if(x > rcclip->x2) flags |= 2;

	if(y < rcclip->y1) flags |= 4;
	else if(y > rcclip->y2) flags |= 8;

	return flags;
}

//=================================



/** ドット直線描画 (ブレゼンハム)
 *
 * exclude_start: 開始位置は点を打たない */

void TileImage_drawLineB(TileImage *p,
	int x1,int y1,int x2,int y2,void *drawcol,mlkbool exclude_start)
{
	int sx,sy,dx,dy,a,a1,e;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	if(x1 == x2 && y1 == y2)
	{
		if(!exclude_start)
			(setpix)(p, x1, y1, drawcol);
		return;
	}

	//

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;
	sx = (x1 <= x2)? 1: -1;
	sy = (y1 <= y2)? 1: -1;

	if(dx >= dy)
	{
		//横長
		
		a  = 2 * dy;
		a1 = a - 2 * dx;
		e  = a - dx;

		if(exclude_start)
		{
			if(e >= 0) y1 += sy, e += a1;
			else e += a;
			x1 += sx;
		}

		while(x1 != x2)
		{
			(setpix)(p, x1, y1, drawcol);

			if(e >= 0) y1 += sy, e += a1;
			else e += a;
			x1 += sx;
		}
	}
	else
	{
		//縦長
		
		a  = 2 * dx;
		a1 = a - 2 * dy;
		e  = a - dy;

		if(exclude_start)
		{
			if(e >= 0) x1 += sx, e += a1;
			else e += a;
			y1 += sy;
		}

		while(y1 != y2)
		{
			(setpix)(p, x1, y1, drawcol);

			if(e >= 0) x1 += sx, e += a1;
			else e += a;
			y1 += sy;
		}
	}

	(setpix)(p, x1, y1, drawcol);
}

/** ドット直線描画 (細線用)
 *
 * 終点は描画しない */

void TileImage_drawLineSlim(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol)
{
	double d,dadd;
	int inc = 1;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	if(x1 == x2 && y1 == y2) return;

	if(abs(x2 - x1) > abs(y2 - y1))
	{
		//横長
	
		dadd = (double)(y2 - y1) / (x2 - x1);

		if(x1 > x2) inc = -1, dadd = -dadd;

		for(d = y1; x1 != x2; d += dadd, x1 += inc)
			(setpix)(p, x1, (int)(d + 0.5), drawcol);
	}
	else
	{
		//縦長
		
		dadd = (double)(x2 - x1) / (y2 - y1);

		if(y1 > y2) inc = -1, dadd = -dadd;

		for(d = x1; y1 != y2; d += dadd, y1 += inc)
			(setpix)(p, (int)(d + 0.5), y1, drawcol);
	}
}

/** ドット直線描画 (フィルタ描画用)
 *
 * 直線座標はそのままで、点描画時に rcclip の範囲外を除外。 */

static void _drawline_filter_setpixel(TileImage *p,int x,int y,void *col,const mRect *rc)
{
	if(rc->x1 <= x && x <= rc->x2 && rc->y1 <= y && y <= rc->y2)
		(g_tileimage_dinfo.func_setpixel)(p, x, y, col);
}

void TileImage_drawLine_forFilter(TileImage *p,
	int x1,int y1,int x2,int y2,void *drawcol,const mRect *rcclip)
{
	int sx,sy,dx,dy,a,a1,e;

	if(x1 == x2 && y1 == y2)
	{
		_drawline_filter_setpixel(p, x1, y1, drawcol, rcclip);
		return;
	}

	//

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;
	sx = (x1 <= x2)? 1: -1;
	sy = (y1 <= y2)? 1: -1;

	if(dx >= dy)
	{
		//横長
		
		a  = 2 * dy;
		a1 = a - 2 * dx;
		e  = a - dx;

		while(x1 != x2)
		{
			_drawline_filter_setpixel(p, x1, y1, drawcol, rcclip);

			if(e >= 0) y1 += sy, e += a1;
			else e += a;
			x1 += sx;
		}
	}
	else
	{
		//縦長
		
		a  = 2 * dx;
		a1 = a - 2 * dy;
		e  = a - dy;

		while(y1 != y2)
		{
			_drawline_filter_setpixel(p, x1, y1, drawcol, rcclip);

			if(e >= 0) x1 += sx, e += a1;
			else e += a;
			y1 += sy;
		}
	}

	_drawline_filter_setpixel(p, x1, y1, drawcol, rcclip);
}

/** 直線描画 (クリッピング付き)
 *
 * 変形ダイアログ用。
 * 直線の座標をクリッピングして描画。 */

void TileImage_drawLine_clip(TileImage *p,int x1,int y1,int x2,int y2,void *col,const mRect *rcclip)
{
	int f,f1,f2,x,y;
	mRect rc;

	rc = *rcclip;

	f1 = _get_clip_flags(x1, y1, &rc);
	f2 = _get_clip_flags(x2, y2, &rc);

	while(f1 | f2)
	{
		//範囲外

		if(f1 & f2) return;

		//クリッピング

		f = (f1)? f1: f2;

		if(f & 8)
		{
			x = x1 + (x2 - x1) * (rc.y2 - y1) / (y2 - y1);
			y = rc.y2;
		}
		else if(f & 4)
		{
			x = x1 + (x2 - x1) * (rc.y1 - y1) / (y2 - y1);
			y = rc.y1;
		}
		else if(f & 2)
		{
			y = y1 + (y2 - y1) * (rc.x2 - x1) / (x2 - x1);
			x = rc.x2;
		}
		else
		{
			y = y1 + (y2 - y1) * (rc.x1 - x1) / (x2 - x1);
			x = rc.x1;
		}

		if(f == f1)
		{
			x1 = x, y1 = y;
			f1 = _get_clip_flags(x, y, &rc);
		}
		else
		{
			x2 = x, y2 = y;
			f2 = _get_clip_flags(x, y, &rc);
		}
	}

	TileImage_drawLineB(p, x1, y1, x2, y2, col, FALSE);
}

/** 4つの点を線でつなぐ (クリッピング付き)
 *
 * 変形ダイアログ用。
 *
 * pt: 4個 */

void TileImage_drawLinePoint4(TileImage *p,const mPoint *pt,const mRect *rcclip)
{
	uint64_t col = (uint64_t)-1;

	TileImage_drawLine_clip(p, pt[0].x, pt[0].y, pt[1].x, pt[1].y, &col, rcclip);
	TileImage_drawLine_clip(p, pt[1].x, pt[1].y, pt[2].x, pt[2].y, &col, rcclip);
	TileImage_drawLine_clip(p, pt[2].x, pt[2].y, pt[3].x, pt[3].y, &col, rcclip);
	TileImage_drawLine_clip(p, pt[3].x, pt[3].y, pt[0].x, pt[0].y, &col, rcclip);
}

/** 楕円枠描画 */

void TileImage_drawEllipse(TileImage *p,double cx,double cy,double xr,double yr,
    void *drawcol,mlkbool dotpen,CanvasViewParam *param,mlkbool mirror)
{
	int i,div;
	double add,rr,xx,yy,x1,y1,sx,sy,nx,ny,dcos,dsin,t = 0;

	//開始位置 (0度)

	x1 = xr * param->cosrev;
	if(mirror) x1 = -x1;

	sx = cx + x1;
	sy = cy + xr * param->sinrev;

	//分割数

	rr = (xr > yr)? xr: yr;

	div = (int)((rr * MLK_MATH_PI * 2) / 4.0);
	if(div < 30) div = 30; else if(div > 400) div = 400;

	//

	add = MLK_MATH_PI * 2 / div;
	rr = add;
	dcos = param->cosrev;
	dsin = param->sinrev;

	for(i = div; i > 0; i--, rr += add)
	{
		xx = xr * cos(rr);
		yy = yr * sin(rr);

		x1 = xx * dcos + yy * dsin; if(mirror) x1 = -x1;
		y1 = xx * dsin - yy * dcos;

		nx = x1 + cx;
		ny = y1 + cy;

		if(dotpen)
		{
			TileImage_drawLineB(p,
				(int)(sx + 0.5), (int)(sy + 0.5),
				(int)(nx + 0.5), (int)(ny + 0.5), drawcol, TRUE);
		}
		else
			t = TileImage_drawBrushLine(p, sx, sy, nx, ny, 1, 1, t);

		sx = nx, sy = ny;
	}
}

/** 楕円 (ドット用)
 *
 * ドットペンで、キャンバス回転が 0 の時用。 */

void TileImage_drawEllipse_dot(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol)
{
	int64_t dx,dy,e,e2,a,b,b1;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	if(x1 == x2 && y1 == y2)
	{
		(setpix)(p, x1, y1, drawcol);
		return;
	}

	//

	a = (x1 < x2)? x2 - x1: x1 - x2;
	b = (y1 < y2)? y2 - y1: y1 - y2;

	if(a >= b)
	{
		//横長

		b1 = b & 1;
		dx = 4 * (1 - a) * b * b;
		dy = 4 * (b1 + 1) * a * a;
		e  = dx + dy + b1 * a * a;

		if(x1 > x2) x1 = x2, x2 += a;
		if(y1 > y2) y1 = y2;

		y1 += (b + 1) / 2;
		y2 = y1 - b1;

		a  = 8 * a * a;
		b1 = 8 * b * b;

		do
		{
			(setpix)(p, x2, y1, drawcol);
			if(x1 != x2) (setpix)(p, x1, y1, drawcol);
			if(y1 != y2) (setpix)(p, x1, y2, drawcol);
			if(x1 != x2 && y1 != y2) (setpix)(p, x2, y2, drawcol);

			e2 = 2 * e;

			if(e2 <= dy)
				y1++, y2--, e += dy += a;

			if(e2 >= dx || 2 * e > dy)
				x1++, x2--, e += dx += b1;

		}while(x1 <= x2);
	}
	else
	{
		//縦長

		b1 = a & 1;
		dy = 4 * (1 - b) * a * a;
		dx = 4 * (b1 + 1) * b * b;
		e  = dx + dy + b1 * b * b;

		if(y1 > y2) y1 = y2, y2 += b;
		if(x1 > x2) x1 = x2;

		x1 += (a + 1) / 2;
		x2 = x1 - b1;

		b  = 8 * b * b;
		b1 = 8 * a * a;

		do
		{
			(setpix)(p, x2, y1, drawcol);
			if(x1 != x2) (setpix)(p, x1, y1, drawcol);
			if(y1 != y2) (setpix)(p, x1, y2, drawcol);
			if(x1 != x2 && y1 != y2) (setpix)(p, x2, y2, drawcol);

			e2 = 2 * e;
			
			if(e2 <= dx)
				x1++, x2--, e += dx += b;
			
			if(e2 >= dy || 2 * e > dx)
				y1++, y2--, e += dy += b1;

		}while(y1 <= y2);
	}
}

/** 四角枠描画 */

void TileImage_drawBox(TileImage *p,int x,int y,int w,int h,void *pix)
{
	int i;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	//横線

	for(i = 0; i < w; i++)
	{
		(setpix)(p, x + i, y, pix);
		(setpix)(p, x + i, y + h - 1, pix);
	}

	//縦線

	for(i = 1; i < h - 1; i++)
	{
		(setpix)(p, x, y + i, pix);
		(setpix)(p, x + w - 1, y + i, pix);
	}
}

/* (ベジェ曲線) 8分割で長さを計算し、分割数を取得 */

static int _bezier_get_division(mDoublePoint *pt)
{
	int i,div;
	double sx,sy,t,len,tt,t1,t2,t3,t4,nx,ny;

	//曲線の長さ
	
	sx = pt[0].x, sy = pt[0].y;

	for(i = 8, t = 0.125, len = 0; i; i--, t += 0.125)
	{
		tt = 1 - t;

		t1 = tt * tt * tt;
		t2 = 3 * t * tt * tt;
		t3 = 3 * t * t * tt;
		t4 = t * t * t;

		nx = pt[0].x * t1 + pt[1].x * t2 + pt[2].x * t3 + pt[3].x * t4;
		ny = pt[0].y * t1 + pt[1].y * t2 + pt[2].y * t3 + pt[3].y * t4;

		len += sqrt((nx - sx) * (nx - sx) + (ny - sy) * (ny - sy));

		sx = nx, sy = ny;
	}

	if(len == 0) return -1;

	//分割数 (1つの線が 4px になるように)

	div = (int)(len / 4.0);
	if(div < 10) div = 10; else if(div > 400) div = 400;

	return div;
}

/** 補助線付きの3次ベジェ線描画 (XOR 描画用)
 *
 * drawline2: 補助線2 を描画する */

void TileImage_drawBezier_forXor(TileImage *p,mPoint *pt,mlkbool drawline2)
{
	mDoublePoint dpt[4];
	uint64_t col;
	int div,i;
	double sx,sy,nx,ny,t,tt,t1,t2,t3,t4,inc;

	col = (uint64_t)-1;

	//補助線1

	TileImage_drawLineB(p, pt[0].x, pt[0].y, pt[1].x, pt[1].y, &col, FALSE);
	TileImage_drawBox(p, pt[1].x - 2, pt[1].y - 2, 5, 5, &col);

	//補助線2

	if(drawline2)
	{
		TileImage_drawLineB(p, pt[2].x, pt[2].y, pt[3].x, pt[3].y, &col, FALSE);
		TileImage_drawBox(p, pt[2].x - 2, pt[2].y - 2, 5, 5, &col);
	}

	//------ 曲線

	for(i = 0; i < 4; i++)
	{
		dpt[i].x = pt[i].x;
		dpt[i].y = pt[i].y;
	}

	div = _bezier_get_division(dpt);
	if(div < 0) return;

	inc = 1.0 / div;

	sx = dpt[0].x, sy = dpt[0].y;

	for(i = div, t = inc; i; i--, t += inc)
	{
		tt = 1 - t;

		t1 = tt * tt * tt;
		t2 = 3 * t * tt * tt;
		t3 = 3 * t * t * tt;
		t4 = t * t * t;

		nx = dpt[0].x * t1 + dpt[1].x * t2 + dpt[2].x * t3 + dpt[3].x * t4;
		ny = dpt[0].y * t1 + dpt[1].y * t2 + dpt[2].y * t3 + dpt[3].y * t4;

		TileImage_drawLineB(p, sx, sy, nx, ny, &col, FALSE);

		sx = nx, sy = ny;
	}
}

/** ベジェ曲線描画 */

void TileImage_drawBezier(TileImage *p,mDoublePoint *pt,void *drawcol,
	mlkbool dotpen,uint32_t headtail)
{
	int div,i,head,tail,nt;
	double sx,sy,nx,ny,t,tt,t1,t2,t3,t4,inc;
	double in,out,press_s,press_n,tbrush = 0;

	//(ブラシ) 入り抜き

	if(!dotpen)
	{
		head = headtail >> 16;
		tail = headtail & 0xffff;

		if(tail > 1000 - head)
			tail = 1000 - head;

		in  = (head)? 1.0 / (head * 0.001): 0;
		out = (tail)? 1.0 / (tail * 0.001): 0;

		press_s = (head)? 0: 1.0;
	}

	//

	div = _bezier_get_division(pt);
	if(div < 0) return;

	inc = 1.0 / div;

	sx = pt[0].x, sy = pt[0].y;

	for(i = 0, t = inc; i < div; i++, t += inc)
	{
		tt = 1 - t;

		t1 = tt * tt * tt;
		t2 = 3 * t * tt * tt;
		t3 = 3 * t * t * tt;
		t4 = t * t * t;

		nx = pt[0].x * t1 + pt[1].x * t2 + pt[2].x * t3 + pt[3].x * t4;
		ny = pt[0].y * t1 + pt[1].y * t2 + pt[2].y * t3 + pt[3].y * t4;

		if(dotpen)
			TileImage_drawLineB(p, sx, sy, nx, ny, drawcol, (i != 0));
		else
		{
			nt = (int)(t * 1000 + 0.5);

			if(nt < head)
				press_n = t * in;
			else if(nt > 1000 - tail)
				press_n = (1.0 - t) * out;
			else
				press_n = 1.0;
			
			tbrush = TileImage_drawBrushLine(p, sx, sy, nx, ny,
				press_s, press_n, tbrush);

			press_s = press_n;
		}

		sx = nx, sy = ny;
	}
}


//================================
// 塗りつぶし
//================================


/** 四角形塗りつぶし */

void TileImage_drawFillBox(TileImage *p,int x,int y,int w,int h,void *col)
{
	int ix,iy;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	for(iy = 0; iy < h; iy++)
	{
		for(ix = 0; ix < w; ix++)
			(setpix)(p, x + ix, y + iy, col);
	}
}

/** 楕円塗りつぶし (キャンバスに対する) */

void TileImage_drawFillEllipse(TileImage *p,
	double cx,double cy,double xr,double yr,void *drawcol,mlkbool antialias,
	CanvasViewParam *param,mlkbool mirror)
{
	int nx,ny,i,j,c,flag,srca,bits;
	double xx,yy,rr,x1,y1,mx,my,xx2,yy2,xt,dcos,dsin;
	mDoublePoint pt[4];
	mRect rc;
	RGBAcombo col;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	if(!antialias)
	{
		cx = floor(cx) + 0.5;
		cy = floor(cy) + 0.5;
		xr = round(xr);
		yr = round(yr);
	}

	//--------- 描画範囲計算 (10度単位)

	dcos = param->cosrev;
	dsin = param->sinrev;

	for(i = 0, rr = 0; i < 36; i++, rr += MLK_MATH_PI / 18)
	{
		xx = xr * cos(rr);
		yy = yr * sin(rr);

		x1 = xx * dcos + yy * dsin; if(mirror) x1 = -x1;
		y1 = xx * dsin - yy * dcos;

		nx = floor(x1 + cx);
		ny = floor(y1 + cy);

		if(i == 0)
		{
			rc.x1 = rc.x2 = nx;
			rc.y1 = rc.y2 = ny;
		}
		else
			mRectIncPoint(&rc, nx, ny);
	}

	//念の為拡張

	rc.x1 -= 2, rc.y1 -= 2;
	rc.x2 += 2, rc.y2 += 2;

	//描画可能範囲内に調整

	if(!TileImage_clipCanDrawRect(p, &rc)) return;

	//-------------

	if(xr < yr)
	{
		//縦長
		rr = xr * xr;
		mx = 1.0, my = xr / yr;
	}
	else
	{
		//横長
		rr = yr * yr;
		mx = yr / xr, my = 1.0;
	}

	//--------- 描画

	bits = TILEIMGWORK->bits;

	bitcol_to_RGBAcombo(&col, drawcol, bits);

	srca = (bits == 8)? col.c8.a: col.c16.a;

	dcos = param->cos;
	dsin = param->sin;

	yy = rc.y1 - cy;

	for(ny = rc.y1; ny <= rc.y2; ny++, yy += 1.0)
	{
		xx = rc.x1 - cx;

		for(nx = rc.x1; nx <= rc.x2; nx++, xx += 1.0)
		{
			if(antialias)
			{
				//-------- アンチエイリアス (5x5)

				//内外判定

				pt[0].x = pt[3].x = xx;
				pt[0].y = pt[1].y = yy;
				pt[1].x = pt[2].x = xx + 1;
				pt[2].y = pt[3].y = yy + 1;

				for(i = 0, flag = 0; i < 4; i++)
				{
					xt = pt[i].x;
					if(mirror) xt = -xt;

					x1 = (xt * dcos - pt[i].y * dsin) * mx;
					y1 = (xt * dsin + pt[i].y * dcos) * my;

					if(x1 * x1 + y1 * y1 <= rr)
						flag |= (1 << i);
				}

				if(flag == 0)
					//円の範囲外
					continue;
				else if(flag == 15)
				{
					//四隅すべて範囲内 = 濃度最大
					(setpix)(p, nx, ny, drawcol);
					continue;
				}

				//5x5 平均

				c = 0;

				for(i = 0, yy2 = yy; i < 5; i++, yy2 += 0.2)
				{
					for(j = 0, xx2 = xx; j < 5; j++, xx2 += 0.2)
					{
						xt = xx2;
						if(mirror) xt = -xt;

						x1 = (xt * dcos - yy2 * dsin) * mx;
						y1 = (xt * dsin + yy2 * dcos) * my;

						if(x1 * x1 + y1 * y1 < rr)
							c++;
					}
				}

				c = srca * c / 25;

				if(c)
				{
					if(bits == 8)
					{
						col.c8.a = c;
						(setpix)(p, nx, ny, &col.c8);
					}
					else
					{
						col.c16.a = c;
						(setpix)(p, nx, ny, &col.c16);
					}
				}
			}
			else
			{
				//--------- 非アンチエイリアス

				xt = xx;
				if(mirror) xt = -xt;

				x1 = (xt * dcos - yy * dsin) * mx;
				y1 = (xt * dsin + yy * dcos) * my;

				if(x1 * x1 + y1 * y1 < rr)
					(setpix)(p, nx, ny, drawcol);
			}
		}
	}
}

/** 楕円塗りつぶし (ドット単位。キャンバス回転が 0 の場合)
 *
 * [!] 同じ位置に複数回点を描画する場合があるので、ドットストローク塗りで点を打つ。 */

static void _fillellipse_lineh(TileImage *p,int x,int y,int x2,void *drawcol)
{
	for(; x <= x2; x++)
		TileImage_setPixel_draw_dot_stroke(p, x, y, drawcol);
}

void TileImage_drawFillEllipse_dot(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol)
{
	int x,y,dx,dy,e,cx,cy,xx,yy,xadd,yadd;
	double rate = 1.0;

	dx = x2 - x1 + 1;
	dy = y2 - y1 + 1;

	if(dx <= 1 && dy <= 1)
	{
		TileImage_setPixel_draw_dot_stroke(p, x1, y1, drawcol);
		return;
	}

	xadd = !(dx & 1);
	yadd = !(dy & 1);
	cx = x1 + dx / 2 - xadd;
	cy = y1 + dy / 2 - yadd;

	if(dx > dy)
	{
		if(cx != x1) rate = (double)(cy - y1) / (cx - x1);
	}
	else
	{
		if(cy != y1) rate = (double)(cx - x1) / (cy - y1);
	}

	x = 0;
	y = abs(dx >= dy ? cx - x1: cy - y1);
	e = 3 - 2 * y;

	//

	while(x <= y)
	{
		xx = (int)(x * rate);
		yy = (int)(y * rate);

		if(dx >= dy)
		{
			_fillellipse_lineh(p, cx - x, cy - yy, cx + x + xadd, drawcol);
			_fillellipse_lineh(p, cx - y, cy - xx, cx + y + xadd, drawcol);
			_fillellipse_lineh(p, cx - y, cy + xx + yadd, cx + y + xadd, drawcol);
			_fillellipse_lineh(p, cx - x, cy + yy + yadd, cx + x + xadd, drawcol);
		}
		else
		{
			_fillellipse_lineh(p, cx - xx, cy - y, cx + xx + xadd, drawcol);
			_fillellipse_lineh(p, cx - yy, cy - x, cx + yy + xadd, drawcol);
			_fillellipse_lineh(p, cx - yy, cy + x + yadd, cx + yy + xadd, drawcol);
			_fillellipse_lineh(p, cx - xx, cy + y + yadd, cx + xx + xadd, drawcol);
		}

		if(e < 0)
			e += 4 * x + 6;
		else
		{
			e += 4 * (x - y) + 10;
			y--;
		}

		x++;
	}
}

/** 多角形塗りつぶし */

mlkbool TileImage_drawFillPolygon(TileImage *p,FillPolygon *fillpolygon,
	void *drawcol,mlkbool antialias)
{
	mRect rc;
	int x,y,ymin,ymax,x1,x2,xmin,width,bits,srca;
	uint16_t *aabuf,*ps;
	TileImageSetPixelFunc setpix;
	RGBAcombo col;

	//描画する y の範囲
	// :描画可能な Y 範囲内のみ

	TileImage_getCanDrawRect_pixel(p, &rc);

	FillPolygon_getMinMaxY(fillpolygon, &ymin, &ymax);

	if(ymin > rc.y2 || ymax < rc.y1) return FALSE;

	if(ymin < rc.y1) ymin = rc.y1;
	if(ymax > rc.y2) ymax = rc.y2;

	//開始

	if(!FillPolygon_beginDraw(fillpolygon, antialias))
		return FALSE;

	if(antialias)
		aabuf = FillPolygon_getAABuf(fillpolygon, &xmin, &width);

	//交点間描画

	bits = TILEIMGWORK->bits;
	setpix = g_tileimage_dinfo.func_setpixel;

	bitcol_to_RGBAcombo(&col, drawcol, bits);

	srca = (bits == 8)? col.c8.a: col.c16.a;

	for(y = ymin; y <= ymax; y++)
	{
		if(antialias)
		{
			//アンチエイリアス

			if(!FillPolygon_setXBuf_AA(fillpolygon, y))
				continue;

			for(x = 0, ps = aabuf; x < width; x++, ps++)
			{
				x1 = (*ps) >> 3; //Y オーバーサンプリング分を割る

				if(x1 == 255)
					(setpix)(p, xmin + x, y, drawcol);
				else if(x1)
				{
					//濃度を適用
					
					if(bits == 8)
					{
						col.c8.a = srca * x1 / 255;
						(setpix)(p, xmin + x, y, &col.c8);
					}
					else
					{
						col.c16.a = srca * x1 / 255;
						(setpix)(p, xmin + x, y, &col.c16);
					}
				}
			}
		}
		else
		{
			//非アンチエイリアス

			if(!FillPolygon_getIntersection_noAA(fillpolygon, y))
				continue;

			while(FillPolygon_getNextLine_noAA(fillpolygon, &x1, &x2))
			{
				for(x = x1; x <= x2; x++)
					(setpix)(p, x, y, drawcol);
			}
		}
	}

	return TRUE;
}


//===========================
// グラデーション
//===========================


/* 色取得 (8bit) */

static void _grad_getcolor_8bit(uint8_t *dst,const TileImageDrawGradInfo *info,int npos)
{
	uint8_t *ps,*psl,*psr;
	int lpos,rpos,n1,n2;

	//---- RGB

	//位置

	for(ps = info->buf + 2; *((uint16_t *)(ps + 5)) < npos; ps += 5);

	//左右の位置と色

	lpos = *((uint16_t *)ps);
	psl = ps + 2;
	
	rpos = *((uint16_t *)(ps + 5));
	psr = ps + 5 + 2;

	//色計算

	if(info->flags & TILEIMAGE_DRAWGRAD_F_SINGLE_COL)
	{
		//単色

		dst[0] = psl[0];
		dst[1] = psl[1];
		dst[2] = psl[2];
	}
	else
	{
		//グラデーション

		n1 = npos - lpos;
		n2 = rpos - lpos;

		dst[0] = (psr[0] - psl[0]) * n1 / n2 + psl[0];
		dst[1] = (psr[1] - psl[1]) * n1 / n2 + psl[1];
		dst[2] = (psr[2] - psl[2]) * n1 / n2 + psl[2];
	}

	//------- A

	ps = info->buf + 2 + *(info->buf) * 5;

	for(; *((uint16_t *)(ps + 3)) < npos; ps += 3);

	lpos = *((uint16_t *)ps);
	rpos = *((uint16_t *)(ps + 3));
	psl = ps + 2;
	psr = ps + 3 + 2;

	n1 = (*psr - *psl) * (npos - lpos) / (rpos - lpos) + *psl;

	dst[3] = n1 * info->density >> 8;
}

/* 色取得 (16bit) */

static void _grad_getcolor_16bit(uint16_t *dst,const TileImageDrawGradInfo *info,int npos)
{
	uint16_t *ps,*psl,*psr;
	int lpos,rpos,n1,n2;

	//---- RGB

	//位置

	for(ps = (uint16_t *)(info->buf + 2); ps[4] < npos; ps += 4);

	//左右の位置と色

	lpos = *ps;
	rpos = ps[4];
	psl = ps + 1;
	psr = ps + 4 + 1;

	//色計算

	if(info->flags & TILEIMAGE_DRAWGRAD_F_SINGLE_COL)
	{
		//単色

		dst[0] = psl[0];
		dst[1] = psl[1];
		dst[2] = psl[2];
	}
	else
	{
		//グラデーション

		n1 = npos - lpos;
		n2 = rpos - lpos;

		dst[0] = (psr[0] - psl[0]) * n1 / n2 + psl[0];
		dst[1] = (psr[1] - psl[1]) * n1 / n2 + psl[1];
		dst[2] = (psr[2] - psl[2]) * n1 / n2 + psl[2];
	}

	//------- A

	ps = (uint16_t *)(info->buf + 2 + *(info->buf) * 8);

	for(; ps[2] < npos; ps += 2);

	lpos = *ps;
	rpos = ps[2];
	psl = ps + 1;
	psr = ps + 2 + 1;

	n1 = (*psr - *psl) * (npos - lpos) / (rpos - lpos) + *psl;

	dst[3] = n1 * info->density >> 8;
}

/** グラデーションの色取得
 *
 * [!] フィルタのグラデーションマップでも使う。
 *
 * pos: 0.0〜1.0 (範囲外の場合もあり) */

void TileImage_getGradationColor(void *dstcol,double pos,const TileImageDrawGradInfo *info)
{
	double dtmp;
	int npos;

	//位置 [0.0-1.0] => 15bit 固定少数

	if(info->flags & TILEIMAGE_DRAWGRAD_F_LOOP)
	{
		//繰り返し
		
		pos = modf(pos, &dtmp);
		if(pos < 0) pos += 1.0;
	}
	else
	{
		//0.0-1.0 に収める
	
		if(pos < 0) pos = 0;
		else if(pos > 1) pos = 1;
	}

	npos = (int)(pos * (1<<15) + 0.5);

	//位置反転

	if(info->flags & TILEIMAGE_DRAWGRAD_F_REVERSE)
		npos = (1<<15) - npos;

	//色取得

	if(TILEIMGWORK->bits == 8)
		_grad_getcolor_8bit((uint8_t *)dstcol, info, npos);
	else
		_grad_getcolor_16bit((uint16_t *)dstcol, info, npos);
}

/* 色をセット */

static void _grad_setpixel(TileImage *p,int x,int y,double pos,const TileImageDrawGradInfo *info)
{
	uint64_t col;

	TileImage_getGradationColor(&col, pos, info);

	TileImage_setPixel_draw_direct(p, x, y, &col);
}

/** グラデーション:線形
 *
 * rc: 描画する範囲 */

void TileImage_drawGradation_line(TileImage *p,
	int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info)
{
	double abx,aby,abab,abap,yy;
	int ix,iy;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcdraw;

	abx = x2 - x1;
	aby = y2 - y1;
	abab = abx * abx + aby * aby;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		yy = aby * (iy - y1);
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			abap = abx * (ix - x1) + yy;
		
			_grad_setpixel(p, ix, iy, abap / abab, info);
		}
	}
}

/** グラデーション:円形 */

void TileImage_drawGradation_circle(TileImage *p,
	int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info)
{
	int ix,iy;
	double dx,dy,len;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcdraw;

	dx = x2 - x1;
	dy = y2 - y1;
	
	len = sqrt(dx * dx + dy * dy);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dy = iy - y1;
		dy *= dy;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			dx = ix - x1;
		
			_grad_setpixel(p, ix, iy, sqrt(dx * dx + dy) / len, info);
		}
	}
}

/** グラデーション:矩形 */

void TileImage_drawGradation_box(TileImage *p,
	int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info)
{
	int ix,iy,abs_y;
	double xx,yy,dx,dy,len,tmpx,tmp;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcdraw;

	xx = abs(x2 - x1);
	yy = abs(y2 - y1);

	len = sqrt(xx * xx + yy * yy);

	dx = (y1 == y2)? 0: xx / yy;
	dy = (x1 == x2)? 0: yy / xx;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		abs_y = (y1 < iy)? iy - y1: y1 - iy;
		tmpx = abs_y * dx;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			xx = (x1 < ix)? ix - x1: x1 - ix;
			yy = abs_y;

			if(y1 == y2)
				yy = 0;
			else if(xx < tmpx)
				xx = tmpx;

			if(x1 == x2)
				xx = 0;
			else
			{
				tmp = xx * dy;
				if(yy < tmp) yy = tmp;
			}
		
			_grad_setpixel(p, ix, iy, sqrt(xx * xx + yy * yy) / len, info);
		}
	}
}

/** グラデーション:放射状 */

void TileImage_drawGradation_radial(TileImage *p,
	int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info)
{
	int ix,iy;
	double dy,top,pos,tmp;
	mRect rc;

	if(x1 == x2 && y1 == y2) return;

	rc = *rcdraw;

	top = -atan2(y2 - y1, x2 - x1);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dy = iy - y1;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			pos = (-atan2(dy, ix - x1) - top) * 0.5 / MLK_MATH_PI;
			pos = modf(pos, &tmp);

			if(pos < 0) pos += 1.0;
		
			_grad_setpixel(p, ix, iy, pos, info);
		}
	}
}


//==========================
// 塗りつぶし処理用
//==========================


/** src (A1) でイメージがある部分に点を描画する */

void TileImage_drawPixels_fromA1(TileImage *dst,TileImage *src,void *drawcol)
{
	uint8_t **pptile,*ps,f;
	int x,y,ix,iy,jx,jy;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	pptile = src->ppbuf;
	y = src->offy;

	//src の全タイルを参照

	for(iy = src->tileh; iy; iy--, y += 64)
	{
		x = src->offx;

		for(ix = src->tilew; ix; ix--, x += 64, pptile++)
		{
			if(!(*pptile)) continue;

			//src のタイルの点を判定して描画

			ps = *pptile;
			f  = 0x80;

			for(jy = 0; jy < 64; jy++)
			{
				for(jx = 0; jx < 64; jx++)
				{
					if(*ps & f)
						(setpix)(dst, x + jx, y + jy, drawcol);

					f >>= 1;
					if(f == 0) f = 0x80, ps++;
				}
			}
		}
	}
}

/** 2つの A1 タイプをタイルごとに結合
 *
 * [!] タイル配列構成は同じであること。 */

void TileImage_combine_forA1(TileImage *dst,TileImage *src)
{
	uint8_t **ppdst,**ppsrc;
	uint64_t *ps,*pd;
	uint32_t i,j;

	ppdst = dst->ppbuf;
	ppsrc = src->ppbuf;

	for(i = dst->tilew * dst->tileh; i; i--, ppsrc++, ppdst++)
	{
		if(*ppsrc)
		{
			if(*ppdst)
			{
				//両方あり: 8byte 単位で OR 結合

				ps = (uint64_t *)*ppsrc;
				pd = (uint64_t *)*ppdst;

				for(j = 8 * 64 / 8; j; j--)
					*(pd++) |= *(ps++);
			}
			else
			{
				//dst が空: 確保してコピー

				*ppdst = TileImage_allocTile(dst);
				if(*ppdst)
					TileImage_copyTile(dst, *ppdst, *ppsrc);
			}
		}
	}
}

/** (A1) 水平線描画
 *
 * タイルに直接セットする。
 *
 * return: FALSE で確保エラー */

mlkbool TileImage_drawLineH_forA1(TileImage *p,int x1,int x2,int y)
{
	TileImageTileRectInfo info;
	mBox box;
	uint8_t **pptile,*pd,f;
	int xx,yy,tw;

	//描画対象のタイル範囲

	box.x = x1, box.y = y;
	box.w = x2 - x1 + 1, box.h = 1;

	pptile = TileImage_getTileRectInfo(p, &info, &box);
	if(!pptile) return TRUE;	//NULL で範囲外

	tw = info.rctile.x2 - info.rctile.x1 + 1;

	//クリッピング

	if(x1 < info.pxtop.x) x1 = info.pxtop.x;
	if(x2 >= info.pxtop.x + tw * 64) x2 = info.pxtop.x + tw * 64 - 1;

	//先頭タイル確保

	if(!TileImage_allocTile_atptr_clear(p, pptile))
		return FALSE;

	//タイルごとに描画

	xx = (x1 - p->offx) & 63;
	yy = ((y - p->offy) & 63) << 3;
	f  = 1 << (7 - (xx & 7));
	pd = *pptile + yy + (xx >> 3);

	for(; tw; tw--)
	{
		//タイル一つ分横線を引く

		for(; xx < 64; xx++, x1++)
		{
			*pd |= f;

			//終了
			if(x1 == x2) return TRUE;

			f >>= 1;
			if(f == 0) f = 0x80, pd++;
		}

		//次のタイル

		pptile++;

		if(!TileImage_allocTile_atptr_clear(p, pptile))
			return FALSE;

		pd = *pptile + yy;
		xx = 0;
	}

	return TRUE;
}

/** (A1) 垂直線描画 */

mlkbool TileImage_drawLineV_forA1(TileImage *p,int y1,int y2,int x)
{
	TileImageTileRectInfo info;
	mBox box;
	uint8_t **pptile,*pd,f;
	int xx,yy,th;

	//描画タイル範囲

	box.x = x, box.y = y1;
	box.w = 1, box.h = y2 - y1 + 1;

	pptile = TileImage_getTileRectInfo(p, &info, &box);
	if(!pptile) return TRUE;	//NULL で範囲外

	th = info.rctile.y2 - info.rctile.y1 + 1;

	//クリッピング

	if(y1 < info.pxtop.y) y1 = info.pxtop.y;
	if(y2 >= info.pxtop.y + th * 64) y2 = info.pxtop.y + th * 64 - 1;

	//先頭タイル確保

	if(!TileImage_allocTile_atptr_clear(p, pptile))
		return FALSE;

	//

	xx = (x - p->offx) & 63;
	yy = (y1 - p->offy) & 63;
	f  = 1 << (7 - (xx & 7));
	xx >>= 3;
	pd = *pptile + (yy << 3) + xx;

	for(; th; th--)
	{
		//タイル一つ分縦線を引く

		for(; yy < 64; yy++, y1++, pd += 8)
		{
			*pd |= f;

			//終了
			if(y1 == y2) return TRUE;
		}

		//次のタイル

		pptile += p->tilew;

		if(!TileImage_allocTile_atptr_clear(p, pptile))
			return FALSE;

		pd = *pptile + xx;
		yy = 0;
	}

	return TRUE;
}

