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
 * mRect/mBox
 *****************************************/

#include "mlk.h"


//================================
// mRect
//================================


/**@ 範囲がない状態かどうか
 *
 * @g:mRect
 * @r:x1 > x2 or y1 > y2 の場合、TRUE となる */

mlkbool mRectIsEmpty(const mRect *rc)
{
	return (rc->x1 > rc->x2 || rc->y1 > rc->y2);
}

/**@ 範囲がない状態にセットする
 *
 * @d:x1,y1 = 0, x2,y2 = -1 とする */

void mRectEmpty(mRect *rc)
{
	rc->x1 = rc->y1 = 0;
	rc->x2 = rc->y2 = -1;
}

/**@ 値をセット */

void mRectSet(mRect *rc,int x1,int y1,int x2,int y2)
{
	rc->x1 = x1;
	rc->y1 = y1;
	rc->x2 = x2;
	rc->y2 = y2;
}

/**@ 値をセット (バイトパック値から)
 *
 * @d:val の各バイトから、値をセットする
 * @p:val 上位バイトから順に x1, y1, x2, y2 */

void mRectSetPack(mRect *rc,uint32_t val)
{
	rc->x1 = (uint8_t)(val >> 24);
	rc->y1 = (uint8_t)(val >> 16);
	rc->x2 = (uint8_t)(val >> 8);
	rc->y2 = (uint8_t)val;
}

/**@ 位置とサイズから値をセット */

void mRectSetBox_d(mRect *rc,int x,int y,int w,int h)
{
	rc->x1 = x;
	rc->y1 = y;
	rc->x2 = x + w - 1;
	rc->y2 = y + h - 1;
}

/**@ mBox から値をセット */

void mRectSetBox(mRect *rc,const mBox *box)
{
	rc->x1 = box->x;
	rc->y1 = box->y;
	rc->x2 = box->x + box->w - 1;
	rc->y2 = box->y + box->h - 1;
}

/**@ mPoint からセット
 *
 * @d:x1,x2 = x, y1,y2 = y となる */

void mRectSetPoint(mRect *rc,const mPoint *pt)
{
	rc->x1 = rc->x2 = pt->x;
	rc->y1 = rc->y2 = pt->y;
}

/**@ mPoint 2点からセット
 *
 * @d:2つの点の最小値、最大値からそれぞれセットする */

void mRectSetPoint_minmax(mRect *rc,const mPoint *pt1,const mPoint *pt2)
{
	if(pt1->x < pt2->x)
		rc->x1 = pt1->x, rc->x2 = pt2->x;
	else
		rc->x1 = pt2->x, rc->x2 = pt1->x;

	if(pt1->y < pt2->y)
		rc->y1 = pt1->y, rc->y2 = pt2->y;
	else
		rc->y1 = pt2->y, rc->y2 = pt1->y;
}

/**@ x1,y1 が最小値、x2,y2 が最大値になるように入れ替え */

void mRectSwap_minmax(mRect *rc)
{
	int n;
	
	if(rc->x1 > rc->x2)
		n = rc->x1, rc->x1 = rc->x2, rc->x2 = n;
	
	if(rc->y1 > rc->y2)
		n = rc->y1, rc->y1 = rc->y2, rc->y2 = n;
}

/**@ mRect を最小値・最大値に入れ替えて、dst にセット
 *
 * @d:x1,y1 が最小値、x2,y2 が最大値になるように入れ替えて、dst にセット。\
 * src は変化しない。 */

void mRectSwap_minmax_to(mRect *dst,const mRect *src)
{
	*dst = *src;
	mRectSwap_minmax(dst);
}

/**@ mRect の範囲を結合
 *
 * @d:dst に src の範囲を追加する。\
 * src に範囲がなければ、何もしない。\
 * dst に範囲がなければ、src をコピーする。 */

void mRectUnion(mRect *dst,const mRect *src)
{
	if(mRectIsEmpty(src))
		return;
	else if(mRectIsEmpty(dst))
		*dst = *src;
	else
	{
		if(dst->x1 > src->x1) dst->x1 = src->x1;
		if(dst->y1 > src->y1) dst->y1 = src->y1;
		if(dst->x2 < src->x2) dst->x2 = src->x2;
		if(dst->y2 < src->y2) dst->y2 = src->y2;
	}
}

/**@ mBox の範囲を結合
 *
 * @d:dst に box の範囲を追加する */

void mRectUnion_box(mRect *dst,const mBox *box)
{
	mRect rc;

	rc.x1 = box->x, rc.y1 = box->y;
	rc.x2 = box->x + box->w - 1;
	rc.y2 = box->y + box->h - 1;

	mRectUnion(dst, &rc);
}

/**@ クリッピング
 * 
 * @d:指定範囲に収まるようにクリッピングする。\
 * rc が指定範囲の外にある場合、範囲なし状態になる。
 * 
 * @r:rc の一部でも指定範囲内にあるか */

mlkbool mRectClipBox_d(mRect *rc,int x,int y,int w,int h)
{
	int x2,y2;
	
	x2 = x + w - 1;
	y2 = y + h - 1;

	if(w < 1 || h < 1 || mRectIsEmpty(rc)
		|| rc->x2 < x || rc->y2 < y || rc->x1 > x2 || rc->y1 > y2)
	{
		rc->x1 = rc->y1 = 0;
		rc->x2 = rc->y2 = -1;
	
		return FALSE;
	}
	else
	{
		if(rc->x1 < x) rc->x1 = x;
		if(rc->y1 < y) rc->y1 = y;
		if(rc->x2 > x2) rc->x2 = x2;
		if(rc->y2 > y2) rc->y2 = y2;

		return TRUE;
	}
}

/**@ クリッピング (mRect の範囲内)
 *
 * @d:rc を clip の範囲内にクリッピングする
 * @r:rc の一部でも clip の範囲内にあるか */

mlkbool mRectClipRect(mRect *rc,const mRect *clip)
{
	if(mRectIsEmpty(rc) || mRectIsEmpty(clip))
		return FALSE;

	if(rc->x2 < clip->x1 || rc->y2 < clip->y1
		|| rc->x1 > clip->x2 || rc->y1 > clip->y2)
	{
		rc->x1 = rc->y1 = 0;
		rc->x2 = rc->y2 = -1;
		return FALSE;
	}
	else
	{
		if(rc->x1 < clip->x1) rc->x1 = clip->x1;
		if(rc->y1 < clip->y1) rc->y1 = clip->y1;
		if(rc->x2 > clip->x2) rc->x2 = clip->x2;
		if(rc->y2 > clip->y2) rc->y2 = clip->y2;
		
		return TRUE;
	}
}

/**@ 矩形が、指定した mRect の範囲内に完全に収まっているか */

mlkbool mRectIsInRect(mRect *rc,const mRect *rcin)
{
	if(mRectIsEmpty(rc) || mRectIsEmpty(rcin))
		return FALSE;

	return (rc->x1 >= rcin->x1 && rc->x2 <= rcin->x2
		&& rc->y1 >= rcin->y1 && rc->y2 <= rcin->y2);
}

/**@ 指定位置が、範囲内にあるか */

mlkbool mRectIsPointIn(const mRect *rc,int x,int y)
{
	if(mRectIsEmpty(rc))
		return FALSE;

	return (x >= rc->x1 && x <= rc->x2 && y >= rc->y1 && y <= rc->y2);
}

/**@ rc1 と rc2 の範囲が一部でも重なっているか */

mlkbool mRectIsCross(const mRect *rc1,const mRect *rc2)
{
	if(mRectIsEmpty(rc1) || mRectIsEmpty(rc2))
		return FALSE;

	return !(rc1->x1 > rc2->x2 || rc1->x2 < rc2->x1
		|| rc1->y1 > rc2->y2 || rc1->y2 < rc2->y1);
}

/**@ 指定した位置を範囲に追加
 *
 * @d:rc が x, y の位置を含むように範囲を広げる */

void mRectIncPoint(mRect *rc,int x,int y)
{
	if(mRectIsEmpty(rc))
	{
		rc->x1 = rc->x2 = x;
		rc->y1 = rc->y2 = y;
	}
	else
	{
		if(rc->x1 > x) rc->x1 = x;
		if(rc->y1 > y) rc->y1 = y;
		if(rc->x2 < x) rc->x2 = x;
		if(rc->y2 < y) rc->y2 = y;
	}
}

/**@ 範囲を拡張/縮小する
 *
 * @p:dx,dy 方向と値。正の値で拡張、負の値で縮小 */

void mRectDeflate(mRect *rc,int dx,int dy)
{
	rc->x1 -= dx, rc->y1 -= dy;
	rc->x2 += dx, rc->y2 += dy;
}

/**@ 範囲の位置を相対移動 */

void mRectMove(mRect *rc,int mx,int my)
{
	rc->x1 += mx;
	rc->x2 += mx;
	rc->y1 += my;
	rc->y2 += my;
}


//================================
// mBox
//================================


/**@ 値をセット
 *
 * @g:mBox */

void mBoxSet(mBox *box,int x,int y,int w,int h)
{
	box->x = x;
	box->y = y;
	box->w = w;
	box->h = h;
}

/**@ mPoint 2点から範囲セット
 *
 * @d:2点の最小値・最大値から範囲をセットする */

void mBoxSetPoint_minmax(mBox *box,const mPoint *pt1,const mPoint *pt2)
{
	int x1,y1,x2,y2;

	if(pt1->x < pt2->x)
		x1 = pt1->x, x2 = pt2->x;
	else
		x1 = pt2->x, x2 = pt1->x;

	if(pt1->y < pt2->y)
		y1 = pt1->y, y2 = pt2->y;
	else
		y1 = pt2->y, y2 = pt1->y;

	box->x = x1;
	box->y = y1;
	box->w = x2 - x1 + 1;
	box->h = y2 - y1 + 1;
}

/**@ mRect からセット
 *
 * @d:左上を x1,y1、右下を x2,y2 とする。\
 * 範囲が正しくない場合でも、そのままセットする。 */

void mBoxSetRect(mBox *box,const mRect *rc)
{
	box->x = rc->x1;
	box->y = rc->y1;
	box->w = rc->x2 - rc->x1 + 1;
	box->h = rc->y2 - rc->y1 + 1;
}

/**@ mRect からセット (範囲がない場合はサイズを 0 に)
 *
 * @d:rc の範囲がない状態の場合は、w,h を 0 にする */

void mBoxSetRect_empty(mBox *box,const mRect *rc)
{
	box->x = rc->x1;
	box->y = rc->y1;

	if(mRectIsEmpty(rc))
		box->w = box->h = 0;
	else
	{
		box->w = rc->x2 - rc->x1 + 1;
		box->h = rc->y2 - rc->y1 + 1;
	}
}

/**@ すべての点を含む範囲をセット
 *
 * @d:指定数の mPoint から、すべての点を含む最小範囲を box にセット。\
 * 現在の box の範囲は含まず、リセットされる。 */

void mBoxSetPoints(mBox *box,const mPoint *pt,int num)
{
	int x1,y1,x2,y2,xx,yy;

	if(num <= 0) return;

	x1 = x2 = pt->x;
	y1 = y2 = pt->y;

	for(pt++; num > 1; num--, pt++)
	{
		xx = pt->x, yy = pt->y;

		if(xx < x1) x1 = xx;
		if(yy < y1) y1 = yy;
		if(xx > x2) x2 = xx;
		if(yy > y2) y2 = yy;
	}

	box->x = x1, box->y = y1;
	box->w = x2 - x1 + 1, box->h = y2 - y1 + 1;
}

/**@ 範囲結合
 *
 * @d:dst に src の範囲を追加する */

void mBoxUnion(mBox *dst,const mBox *src)
{
	mRect rc1,rc2;

	mRectSetBox(&rc1, dst);
	mRectSetBox(&rc2, src);

	mRectUnion(&rc1, &rc2);

	dst->x = rc1.x1;
	dst->y = rc1.y1;
	dst->w = rc1.x2 - rc1.x1 + 1;
	dst->h = rc1.y2 - rc1.y1 + 1;
}

/**@ 縦横比を維持して指定サイズ内に拡大縮小する
 *
 * @d:box の範囲を、boxw,boxh の大きさに収まるように拡大縮小して、セットする
 * 
 * @p:noup TRUE で、拡大はしない (拡大する代わりに、x,y 位置に余白を追加) */

void mBoxResize_keepaspect(mBox *box,int boxw,int boxh,mlkbool noup)
{
	int x,y,w,h,dw,dh;

	x = box->x, y = box->y;
	w = box->w, h = box->h;

	if(noup && w <= boxw && h <= boxh)
	{
		//ボックスサイズより小さければ拡大せず余白を付ける

		x = (boxw - w) >> 1;
		y = (boxh - h) >> 1;
	}
	else
	{
		//縦横比維持で拡大縮小

		dw = (int)((double)boxh / h * w + 0.5);
		dh = (int)((double)boxw / w * h + 0.5);

		if(dw < boxw)
		{
			//縦長

			x = (boxw - dw) >> 1, y = 0;
			w = dw, h = boxh;
		}
		else
		{
			//横長

			x = 0, y = (boxh - dh) >> 1;
			w = boxw, h = dh;
		}
	}

	box->x = x, box->y = y;
	box->w = w, box->h = h;
}

/**@ 2つの範囲が重なっているかどうか
 *
 * @d:2つの範囲が一部でも重なっているかどうか */

mlkbool mBoxIsCross(const mBox *box1,const mBox *box2)
{
	if(box1->w == 0 || box1->h == 0 || box2->w == 0 || box2->h == 0)
		return FALSE;

	return (box1->x < box2->x + box2->w
		&& box1->y < box2->y + box2->h
		&& box1->x + box1->w - 1 >= box2->x
		&& box1->y + box1->h - 1 >= box2->y);
}

/**@ 指定位置が範囲内にあるか
 *
 * @d:x,y の位置が box の範囲内にあるか */

mlkbool mBoxIsPointIn(const mBox *box,int x,int y)
{
	return (box->w && box->h
		&& box->x <= x && x < box->x + box->w
		&& box->y <= y && y < box->y + box->h);
}

