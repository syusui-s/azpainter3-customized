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
 * FillPolygon
 *
 * 多角形塗りつぶし処理
 *****************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_buf.h"


//--------------------

//交点データ

typedef struct
{
	int32_t x,dir;
}EdgeDat;

//処理用データ

typedef struct _FillPolygon
{
	mBuf buf_pt,	//ポイントバッファ
		buf_edge;	//交点バッファ
	uint16_t *aabuf;	//アンチエイリアス用バッファ
	
	int ptnum,		//頂点の数
		edgenum,	//現在の交点数
		xmin,xmax,ymin,ymax,	//全頂点の最小/最大値 (int)
		width,			//描画のX幅
		edge_curpos,	//現在の交点位置
		edge_curparam;	//塗りつぶし規則用
}FillPolygon;


#define _EXPAND_PTNUM  1
#define _SUBPIXEL_Y    8
#define _GETPT_POS(p,pos)   ((mDoublePoint *)(p)->buf_pt.buf + pos)

//--------------------


//===========================
// main
//===========================


/** 解放 */

void FillPolygon_free(FillPolygon *p)
{
	if(p)
	{
		mBufFree(&p->buf_pt);
		mBufFree(&p->buf_edge);
		mFree(p->aabuf);
		
		mFree(p);
	}
}

/** 作成 */

FillPolygon *FillPolygon_new(void)
{
	FillPolygon *p;

	p = (FillPolygon *)mMalloc0(sizeof(FillPolygon));
	if(!p) return NULL;

	//ポイントバッファ

	if(!mBufAlloc(&p->buf_pt, sizeof(mDoublePoint) * 20, sizeof(mDoublePoint) * 50))
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/** 頂点を追加
 *
 * return: FALSE で失敗 */

mlkbool FillPolygon_addPoint(FillPolygon *p,double x,double y)
{
	mDoublePoint *pbuf,pt;
	int nx,ny;

	//前の位置と同じ場合、追加しない

	if(p->ptnum)
	{
		pbuf = _GETPT_POS(p, p->ptnum - 1);

		if(pbuf->x == x && pbuf->y == y) return TRUE;
	}

	//追加

	pt.x = x, pt.y = y;

	if(!mBufAppend(&p->buf_pt, &pt, sizeof(mDoublePoint)))
		return FALSE;

	p->ptnum++;

	//最小/最大値 (int)

	nx = floor(x);
	ny = floor(y);

	if(p->ptnum == 1)
	{
		p->xmin = p->xmax = nx;
		p->ymin = p->ymax = ny;
	}
	else
	{
		if(nx < p->xmin) p->xmin = nx;
		if(nx > p->xmax) p->xmax = nx;

		if(ny < p->ymin) p->ymin = ny;
		if(ny > p->ymax) p->ymax = ny;
	}

	return TRUE;
}

/** 頂点を閉じる
 *
 * return: FALSE で頂点が足りない */

mlkbool FillPolygon_closePoint(FillPolygon *p)
{
	mDoublePoint *pbuf;

	if(p->ptnum < 3) return FALSE;

	//始点を追加

	pbuf = (mDoublePoint *)p->buf_pt.buf;

	FillPolygon_addPoint(p, pbuf->x, pbuf->y);

	//X 範囲を拡張

	p->xmin--;
	p->xmax++;

	return TRUE;
}

/** 4つの点を追加して、閉じる */

mlkbool FillPolygon_addPoint4_close(FillPolygon *p,mDoublePoint *pt)
{
	int i;

	for(i = 0; i < 4; i++)
		FillPolygon_addPoint(p, pt[i].x, pt[i].y);

	return FillPolygon_closePoint(p);
}


//================================
// sub
//================================


/* 交点追加 */

static mlkbool _add_edge(FillPolygon *p,int x,int dir)
{
	EdgeDat dat;

	dat.x = x;
	dat.dir = dir;

	if(!mBufAppend(&p->buf_edge, &dat, sizeof(EdgeDat)))
		return FALSE;

	p->edgenum++;

	return TRUE;
}

/* 交点を X の小さい順に並び替え */

static void _sort_edge(FillPolygon *p)
{
	EdgeDat *pe,*pe2,*pemin,dat;
	int i,j;

	pe = (EdgeDat *)p->buf_edge.buf;

	for(i = 0; i < p->edgenum - 1; i++, pe++)
	{
		pemin = pe;

		for(j = i + 1, pe2 = pe + 1; j < p->edgenum; j++, pe2++)
		{
			if(pe2->x < pemin->x)
				pemin = pe2;
		}

		//入れ替え

		if(pe != pemin)
		{
			dat = *pe;
			*pe = *pemin;
			*pemin = dat;
		}
	}
}


//================================
// 描画用
//================================


/** 描画開始
 *
 * return: FALSE で失敗 */

mlkbool FillPolygon_beginDraw(FillPolygon *p,mlkbool antialias)
{
	//描画X幅
	
	p->width = p->xmax - p->xmin + 1;

	//交点バッファ確保

	if(!mBufAlloc(&p->buf_edge, sizeof(EdgeDat) * 20, sizeof(EdgeDat) * 20))
		return FALSE;

	//

	if(antialias)
	{
		//アンチエイリアス用バッファ

		p->aabuf = (uint16_t *)mMalloc(p->width * 2);
		if(!p->aabuf) return FALSE;
	}
	else
	{
		//[非アンチエイリアス時] 位置の小数点を切り捨て
		// :切り捨て後に同じ位置が続く場合、除外

		int i,num = 0;
		mDoublePoint *pd,*ps;

		pd = ps = (mDoublePoint *)p->buf_pt.buf;

		for(i = 0; i < p->ptnum; i++, ps++)
		{
			pd->x = floor(ps->x);
			pd->y = floor(ps->y);

			//前の位置と同じなら削除

			if(!(i && pd->x == pd[-1].x && pd->y == pd[-1].y))
			{
				pd++;
				num++;
			}
		}

		if(num < 3 + _EXPAND_PTNUM) return FALSE;

		p->ptnum = num;
	}

	return TRUE;
}

/** 描画先 Y の最小/最大値取得 */

void FillPolygon_getMinMaxY(FillPolygon *p,int *ymin,int *ymax)
{
	*ymin = p->ymin;
	*ymax = p->ymax;
}

/** 描画先の矩形範囲取得 */

void FillPolygon_getDrawRect(FillPolygon *p,mRect *rc)
{
	rc->x1 = p->xmin;
	rc->x2 = p->xmax;
	rc->y1 = p->ymin;
	rc->y2 = p->ymax;
}


//================================
// 非アンチエイリアス
//================================


/** 描画 Y 位置の交点取得 (非アンチエイリアス用) */

mlkbool FillPolygon_getIntersection_noAA(FillPolygon *p,int y)
{
	mDoublePoint *ptbuf,*pt1,*pt2;
	int i,x,dir;
	double dy;

	//交点クリア

	mBufReset(&p->buf_edge);

	p->edgenum = 0;
	p->edge_curpos = 0;
	p->edge_curparam = 0;

	//各辺から交点取得

	ptbuf = (mDoublePoint *)p->buf_pt.buf;

	dy = y + 0.1;

	for(i = p->ptnum - _EXPAND_PTNUM; i > 0; i--, ptbuf++)
	{
		pt1 = ptbuf;
		pt2 = ptbuf + 1;
	
		//水平線は除外

		if(pt1->y == pt2->y) continue;

		//方向
	
		dir = (pt1->y < pt2->y)? 1: -1;

		if(dir < 0)
			pt1 = ptbuf + 1, pt2 = ptbuf;

		//dy が辺の範囲外

		if(dy < pt1->y || dy > pt2->y) continue;

		//交点 X

		x = round(pt1->x + (dy - pt1->y) / (pt2->y - pt1->y) * (pt2->x - pt1->x));

		if(!_add_edge(p, x, dir))
			return FALSE;
	}

	_sort_edge(p);

	return TRUE;
}

/** (非アンチエイリアス用) 描画する次の水平線を取得
 *
 * return: FALSE で終了 */

mlkbool FillPolygon_getNextLine_noAA(FillPolygon *p,int *left,int *right)
{
	EdgeDat *pe;

	if(p->edge_curpos >= p->edgenum - 1) return FALSE;

	pe = (EdgeDat *)p->buf_edge.buf + p->edge_curpos;

	for(; p->edge_curpos < p->edgenum - 1; p->edge_curpos++, pe++)
	{
		//規則カウント

		p->edge_curparam += pe->dir;

		//カウント値が0以外なら交点間を描画

		if(p->edge_curparam)
		{
			*left = pe->x;
			*right = pe[1].x;

			p->edge_curpos++;

			return TRUE;
		}
	}

	return FALSE;
}


//================================
// アンチエイリアス
//================================


/* 交点リスト作成 */

static mlkbool _get_intersection_aa(FillPolygon *p,double y)
{
	mDoublePoint *ptbuf,pt1,pt2,pttmp;
	int i,x,dir;

	//交点クリア

	mBufReset(&p->buf_edge);
	
	p->edgenum = 0;

	//各辺から交点取得

	ptbuf = (mDoublePoint *)p->buf_pt.buf;

	for(i = p->ptnum - _EXPAND_PTNUM; i > 0; i--, ptbuf++)
	{
		pt1 = *ptbuf;
		pt2 = ptbuf[1];
	
		//水平線は除外

		if(pt1.y == pt2.y) continue;

		//方向
	
		dir = (pt1.y < pt2.y)? 1: -1;

		if(dir < 0)
			pttmp = pt1, pt1 = pt2, pt2 = pttmp;

		//y が辺の範囲外

		if(y < pt1.y || y > pt2.y) continue;

		//交点 X (8bit 固定小数点。xmin からの相対位置)

		x = round(((y - pt1.y) / (pt2.y - pt1.y) * (pt2.x - pt1.x) + pt1.x - p->xmin) * (1<<8));

		if(!_add_edge(p, x, dir))
			return FALSE;
	}

	_sort_edge(p);

	return TRUE;
}

/** (アンチエイリアス用) 描画Y位置のバッファをセット
 *
 * return: FALSE で失敗 */

mlkbool FillPolygon_setXBuf_AA(FillPolygon *p,int y)
{
	uint16_t *xbuf,*px;
	EdgeDat *pe;
	double dy;
	int iy,i,cnt,x1,x2,nx1,nx2;

	xbuf = p->aabuf;

	mMemset0(xbuf, p->width * 2);

	dy = y + 0.011;  //少しずらす

	for(iy = 0; iy < _SUBPIXEL_Y; iy++, dy += 1.0 / _SUBPIXEL_Y)
	{
		//交点取得

		if(!_get_intersection_aa(p, dy)) return FALSE;

		//交点間

		pe = (EdgeDat *)p->buf_edge.buf;
		cnt = 0;

		for(i = p->edgenum - 1; i > 0; i--, pe++)
		{
			cnt += pe->dir;

			if(!cnt) continue;

			//交点計算時のX小数部分を濃度に反映
			// :x1〜x2 の範囲
		
			x1 = pe->x;
			x2 = pe[1].x;

			nx1 = x1 >> 8;
			nx2 = x2 >> 8;

			px = xbuf + nx1;

			if(nx1 == nx2)
				*px += (x2 - x1) & 255;
			else
			{
				*(px++) += 255 - (x1 & 255);

				for(nx1++; nx1 < nx2; nx1++)
					*(px++) += 255;

				*px += x2 & 255;
			}
		}
	}

	return TRUE;
}

/** X描画分のバッファを取得
 *
 * Y をオーバーサンプリングした濃度値 (MAX=255) となっているため、
 * オーバーサンプリング分を割って、濃度 0-255 を出す。
 *
 * xmin: 開始X位置
 * width: 描画幅 */

uint16_t *FillPolygon_getAABuf(FillPolygon *p,int *xmin,int *width)
{
	*xmin = p->xmin;
	*width = p->width;

	return p->aabuf;
}

