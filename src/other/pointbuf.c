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
 * PointBuf
 *
 * 自由線描画用のポイントデータ
 *****************************************/

#include <math.h>

#include "mlk.h"

#include "pointbuf.h"


/*
  - ポイントデータはリング状。
  - 手ブレ補正も行う。
*/

//---------------------

#define _POINT_NUM  64
#define _POINT_AND  (_POINT_NUM - 1)

typedef struct
{
	double x,y,pressure;
}_pointdat;

struct _PointBuf
{
	_pointdat pt[_POINT_NUM];	//ポイントデータ
	PointBufDat ptlast;
	int ptnum,		//現在のポイント数
		sm_type,	//補正タイプ
		sm_val,		//補正強さ
		sm_ptnum,	//補正時に参照するポイントの数 (現在位置含む)
		sm_subtype,	//補正サブタイプ
		nparam[1];
	double dparam[2];
};

enum
{
	_SUBTYPE_NONE,
	_SUBTYPE_AVG,	//平均
	_SUBTYPE_RANGE	//一定距離
};

//---------------------


//============================
// sub
//============================


/* 現在位置からの相対位置でポイント位置取得
 *
 * pos: 0 で最後の点 */

static _pointdat *_get_ptbuf_rel(PointBuf *p,int pos)
{
	return p->pt + ((p->ptnum - 1 + pos) & _POINT_AND);
}

/* 最後の点を取得 */

static void _get_pt_last(PointBuf *p,_pointdat *dst)
{
	*dst = p->pt[(p->ptnum - 1) & _POINT_AND];
}

/* 最後の点を PointBufDat で取得 */

static void _get_pt_last_dat(PointBuf *p,PointBufDat *dst)
{
	_pointdat *ppt;

	ppt = _get_ptbuf_rel(p, 0);

	dst->x = ppt->x;
	dst->y = ppt->y;
	dst->pressure = ppt->pressure;
}

/* ポイントを追加してバッファ位置を取得 */

static _pointdat *_add_point(PointBuf *p)
{
	_pointdat *pt;

	pt = p->pt + (p->ptnum & _POINT_AND);

	p->ptnum++;

	if(p->ptnum >= _POINT_NUM * 2)
		p->ptnum -= _POINT_NUM;

	return pt;
}

/* 最後の点を指定数追加 */

static void _add_point_last(PointBuf *p,int cnt)
{
	_pointdat pt,*ppt;

	_get_pt_last(p, &pt);

	for(; cnt > 0; cnt--)
	{
		ppt = _add_point(p);
		
		*ppt = pt;
	}
}


//============================
// 各タイプごと
//============================


/* 平均: 補正位置を計算 */

static mlkbool _get_drawpoint_avg(PointBuf *p,PointBufDat *dst)
{
	_pointdat *pt;
	int i,pos,len;
	double d,dw,dsum,dx,dy,dpress;

	len = p->sm_ptnum - 1;
	pos = -len;

	dx = dy = dpress = dsum = 0;

	switch(p->sm_type)
	{
		//単純平均
		case SMOOTHING_TYPE_AVG_SIMPLE:
			for(i = len; i >= 0; i--)
			{
				pt = _get_ptbuf_rel(p, pos++);

				dx += pt->x;
				dy += pt->y;
				dpress += pt->pressure;
			}

			dsum = len + 1;
			break;

		//位置による線形重み
		case SMOOTHING_TYPE_AVG_LINEAR:
			d = 1.0 / p->sm_ptnum;
			
			for(i = len; i >= 0; i--)
			{
				pt = _get_ptbuf_rel(p, pos++);

				dw = 1.0 - i * d;

				dx += pt->x * dw;
				dy += pt->y * dw;
				dpress += pt->pressure * dw;

				dsum += dw;
			}
			break;

		//ガウス重み
		default:
			d = (len + 1) * 0.5;
			d = 1.0 / (2 * d * d);

			for(i = len; i >= 0; i--)
			{
				pt = _get_ptbuf_rel(p, pos++);

				dw = exp(-(i * i) * d);

				dx += pt->x * dw;
				dy += pt->y * dw;
				dpress += pt->pressure * dw;

				dsum += dw;
			}
			break;
	}

	dst->x = dx / dsum;
	dst->y = dy / dsum;
	dst->pressure = dpress / dsum;

	return TRUE;
}

/* 平均: 最後の描画 */

static mlkbool _finish_avg(PointBuf *p,PointBufDat *dst)
{
	int num = p->nparam[0];

	if(num == 0)
		//終了
		return FALSE;
	else if(num == 1)
	{
		//最後の点はそのまま返す

		_get_pt_last_dat(p, dst);
	}
	else
	{
		_add_point_last(p, 1);
		
		PointBuf_getPoint(p, dst);
	}

	(p->nparam[0])--;

	return TRUE;
}

/* 一定距離: 点追加後 */

static void _range_addpoint(PointBuf *p)
{
	_pointdat *pt;
	double len,xx,yy;

	pt = _get_ptbuf_rel(p, 0);

	//前回からの距離

	xx = pt->x - pt[-1].x;
	yy = pt->y - pt[-1].y;
	
	len = sqrt(xx * xx + yy * yy);

	//距離が一定以上なら適用

	len += p->dparam[0];

	if(len < p->dparam[1])
	{
		p->dparam[0] = len;
		p->nparam[0] = 0;
	}
	else
	{
		p->dparam[0] = 0;
		p->nparam[0] = 1;

		p->ptlast.x = pt->x;
		p->ptlast.y = pt->y;
		p->ptlast.pressure = pt->pressure;
	}
}


//============================
// main
//============================


/** PointBuf 確保 */

PointBuf *PointBuf_new(void)
{
	return (PointBuf *)mMalloc(sizeof(PointBuf));
}

/** 手ぶれ補正のセット
 *
 * [!] ポイントをセットする前に実行すること。 */

void PointBuf_setSmoothing(PointBuf *p,int type,int val)
{
	p->sm_type = type;
	p->sm_val = val;

	switch(type)
	{
		//なし
		case SMOOTHING_TYPE_NONE:
			p->sm_subtype = _SUBTYPE_NONE;
			p->sm_ptnum = 0;
			break;
		//平均
		case SMOOTHING_TYPE_AVG_SIMPLE:
		case SMOOTHING_TYPE_AVG_LINEAR:
		case SMOOTHING_TYPE_AVG_GAUSS:
			p->sm_subtype = _SUBTYPE_AVG;
			p->sm_ptnum = 2 + val * 2;
			p->nparam[0] = 1 + val * 2; //最後に描画する点の数
			break;
		//一定距離
		// dparam[0]: 現在の距離
		// dparam[1]: 距離値
		// nparam[0]: 0=最後の点は描画対象外、1=最後の点を描画
		case SMOOTHING_TYPE_RANGE:
			p->sm_subtype = _SUBTYPE_RANGE;
			p->dparam[0] = 0;
			p->dparam[1] = 2 + val;
			p->nparam[0] = 0;
			break;
	}
}

/** 最初のポイントをセット */

void PointBuf_setFirstPoint(PointBuf *p,double x,double y,double pressure)
{
	p->pt[0].x = x;
	p->pt[0].y = y;
	p->pt[0].pressure = pressure;

	p->ptnum = 1;

	//平均時は過去参照分を追加

	if(p->sm_subtype == _SUBTYPE_AVG)
		_add_point_last(p, p->sm_ptnum - 1);
}

/** ポイントを追加 */

void PointBuf_addPoint(PointBuf *p,double x,double y,double pressure)
{
	_pointdat *pt;

	//追加

	pt = _add_point(p);

	pt->x = x;
	pt->y = y;
	pt->pressure = pressure;

	//

	switch(p->sm_subtype)
	{
		case _SUBTYPE_RANGE:
			_range_addpoint(p);
			break;
	}
}

/** 最後の位置から、補正後の位置を取得
 *
 * return: FALSE で描画なし */

mlkbool PointBuf_getPoint(PointBuf *p,PointBufDat *dst)
{
	switch(p->sm_subtype)
	{
		//補正無し
		// :最後の点をそのまま返す
		case _SUBTYPE_NONE:
			_get_pt_last_dat(p, dst);
			return TRUE;

		//平均
		case _SUBTYPE_AVG:
			return _get_drawpoint_avg(p, dst);

		//一定距離
		case _SUBTYPE_RANGE:
			if(!p->nparam[0])
				return FALSE;
			else
			{
				*dst = p->ptlast;
				return TRUE;
			}
	}

	return FALSE;
}

/** 描画終了時の描画位置を取得
 *
 * FALSE が返るまで複数回行う。
 *
 * return: FALSE で描画する点がない */

mlkbool PointBuf_getFinishPoint(PointBuf *p,PointBufDat *dst)
{
	switch(p->sm_subtype)
	{
		case _SUBTYPE_AVG:
			return _finish_avg(p, dst);

		//一定距離
		// :最後の点が描画されていなければ、最後の点
		case _SUBTYPE_RANGE:
			if(p->nparam[0])
				return FALSE;
			else
			{
				_get_pt_last_dat(p, dst);
				p->nparam[0] = 1;
				return TRUE;
			}
	}

	return FALSE;
}

