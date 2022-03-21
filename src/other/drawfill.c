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
 * DrawFill
 *
 * 塗りつぶし描画処理
 *****************************************/

#include "mlk.h"

#include "tileimage.h"
#include "def_tileimage.h"
#include "imagecanvas.h"

#include "def_draw.h"

#include "drawfill.h"


/*-------------------------

 - 不透明範囲時、参照レイヤは常に描画先。
 
 - RGB で判定する場合、参照レイヤは最初の一つのみ有効。
 
 - アルファ値で判定する場合は、すべての参照レイヤのアルファ値を合成した値で比較。
 
 - 参照レイヤは imgref が先頭で、以降は TileImage::link でリンクされている。
 
 - rcref は塗りつぶしの判定を行う範囲。
   この範囲外は境界色で囲まれているとみなす。

 - imgtmp_ref は参照レイヤをすべて合成したイメージ。最初は空。
   判定時に色を取得する際にタイルがなければ、その都度タイルを作成していく。
 
 - imgtmp_draw (1bit) は塗りつぶす部分に点を置いておくイメージ。
   直接描画先に描画するのではなく、まずはここに点を置いて、描画する部分を決める。
 
--------------------------*/

//------------------

typedef mlkbool (*func_compare)(DrawFill *p,int x,int y);
typedef void (*func_blend_alpha)(void *dst,void *src);

typedef struct
{
	int32_t lx,rx,y,oy;
}_fillbuf;

typedef struct
{
	int32_t r,g,b,a;
}_intcolor;

struct _DrawFill
{
	_fillbuf *buf;		//塗りつぶし用バッファ

	TileImage *imgdst,		//描画先
			*imgref,		//参照イメージの先頭 (複数の場合はリンクされている)
			*imgtmp_ref,	//作業用 (参照色)
			*imgtmp_draw,	//作業用[A1] (塗りつぶす部分)
			*imgtmp_draw2,	//作業用[A1] (塗りつぶす部分<アンチエイリアス自動用>)
			*imgtarget;		//描画先イメージ (ポインタとして使う)

	int type,				//処理タイプ
		color_diff,			//許容色差 (各ビット値)
		draw_density,		//描画濃度 (各ビット値)
		refimage_multi,		//複数のレイヤを参照するか
		imgbits;			//ビット数
	mPoint pt_start;		//開始点
	mRect rcref;			//色を参照する範囲
	_intcolor start_col;	//開始色

	func_blend_alpha blend_alpha;	//アルファ値合成関数
	func_compare compare;		//ピクセル値比較関数
};

#define _FILLBUF_NUM  8192

//------------------

static void _run_normal(DrawFill *p);
static void _run_auto_horz(DrawFill *p);
static void _run_auto_vert(DrawFill *p);

static int _getpixelref_alpha(DrawFill *p,int x,int y);

static mlkbool _compare_rgb(DrawFill *p,int x,int y);
static mlkbool _compare_alpha(DrawFill *p,int x,int y);
static mlkbool _compare_alpha_0(DrawFill *p,int x,int y);
static mlkbool _compare_opaque(DrawFill *p,int x,int y);

//------------------



//============================
// 関数
//============================


/* 各ビットのRGBAカラー値から、_intcolor にセット */

static void _set_intcolor(DrawFill *p,_intcolor *dst,void *src)
{
	uint8_t *p8;
	uint16_t *p16;

	if(p->imgbits == 8)
	{
		p8 = (uint8_t *)src;

		dst->r = p8[0];
		dst->g = p8[1];
		dst->b = p8[2];
		dst->a = p8[3];
	}
	else
	{
		p16 = (uint16_t *)src;

		dst->r = p16[0];
		dst->g = p16[1];
		dst->b = p16[2];
		dst->a = p16[3];
	}
}

/* (8bit) アルファ値合成 */

static void _8bit_blend_alpha(void *dst,void *src)
{
	int a,b;

	a = *((uint8_t *)dst);
	b = *((uint8_t *)src);

	*((uint8_t *)dst) = a + b - a * b / 255;
}

/* (16bit) アルファ値合成 */

static void _16bit_blend_alpha(void *dst,void *src)
{
	int a,b;

	a = *((uint16_t *)dst);
	b = *((uint16_t *)src);

	*((uint16_t *)dst) = a + b - (a * b >> 15);
}


//============================
// 解放
//============================


/* 作業用バッファ解放 (描画時に不要なもの) */

static void _free_tmp(DrawFill *p)
{
	TileImage_free(p->imgtmp_ref);
	TileImage_free(p->imgtmp_draw2);

	mFree(p->buf);

	//

	p->imgtmp_ref = p->imgtmp_draw2 = NULL;
	p->buf = NULL;
}

/** DrawFill 解放 */

void DrawFill_free(DrawFill *p)
{
	if(p)
	{
		_free_tmp(p);
		
		TileImage_free(p->imgtmp_draw);

		mFree(p);
	}
}


//============================
// 初期化
//============================


/* 開始点の色を取得 */

static void _get_start_pixel(DrawFill *p,int x,int y)
{
	TileImage *img = p->imgref;
	uint8_t *pdst;
	uint64_t col;
	uint16_t a;

	if(p->type == DRAWFILL_TYPE_CANVAS)
	{
		//キャンバス色

		ImageCanvas_getPixel_rgba(APPDRAW->imgcanvas, x, y, &col);
	}
	else
	{
		//レイヤの色

		TileImage_getPixel(img, x, y, &col);

		if(p->imgbits == 8)
			pdst = (uint8_t *)&col + 3;
		else
			pdst = (uint8_t *)((uint16_t *)&col + 3);

		for(img = img->link; img; img = img->link)
		{
			TileImage_getPixel_alpha_buf(img, x, y, &a);
			
			(p->blend_alpha)(pdst, &a);
		}
	}

	_set_intcolor(p, &p->start_col, &col);
}

/* 各タイプ別の初期化 (開始点のピクセル判定 + 比較関数セット)
 *
 * return: TRUE で描画する必要がない */

static mlkbool _init_type(DrawFill *p)
{
	switch(p->type)
	{
		//RGB/キャンバス色
		case DRAWFILL_TYPE_RGB:
		case DRAWFILL_TYPE_CANVAS:
			p->compare = _compare_rgb;
			return FALSE;

		//アルファ値
		case DRAWFILL_TYPE_ALPHA:
			p->compare = _compare_alpha;
			return FALSE;
	
		//透明(アンチエイリアス自動)
		case DRAWFILL_TYPE_TRANSPARENT_AUTO:
			p->compare = _compare_alpha_0;
			return (p->start_col.a != 0);

		//完全透明
		case DRAWFILL_TYPE_TRANSPARENT:
			p->compare = _compare_alpha_0;
			return (p->start_col.a != 0);

		//不透明範囲
		default:
			p->compare = _compare_opaque;
			return (p->start_col.a == 0);
	}
}

/* 初期化 */

static mlkbool _init(DrawFill *p)
{
	//複数のレイヤを参照するか
	// :RGB で判定する場合は、常に一つのみ

	p->refimage_multi = (p->imgref->link != 0
		&& p->type != DRAWFILL_TYPE_RGB && p->type != DRAWFILL_TYPE_CANVAS);

	//単体参照なら、リンクを一つに

	if(!p->refimage_multi)
		p->imgref->link = NULL;

	//色を参照する範囲

	if(p->type == DRAWFILL_TYPE_OPAQUE)
	{
		//[不透明範囲] 描画先の描画可能範囲
		
		TileImage_getCanDrawRect_pixel(p->imgdst, &p->rcref);
	}
	else
	{
		//[通常塗りつぶし]
		// イメージ範囲外に余白部分があると、その部分も塗りつぶされてしまうので、
		// イメージ範囲内に限定する。

		p->rcref.x1 = p->rcref.y1 = 0;
		p->rcref.x2 = APPDRAW->imgw - 1;
		p->rcref.y2 = APPDRAW->imgh - 1;
	}

	//開始点が色の参照範囲内にあるか
	// :範囲外なら描画の必要なし

	if(p->pt_start.x < p->rcref.x1 || p->pt_start.y < p->rcref.y1
		|| p->pt_start.x > p->rcref.x2 || p->pt_start.y > p->rcref.y2)
		return FALSE;

	//開始点の色を取得
	
	_get_start_pixel(p, p->pt_start.x, p->pt_start.y);

	//タイプ別の初期化と開始点判定

	if(_init_type(p))
		return FALSE;

	//------- 確保

	//バッファ確保

	p->buf = (_fillbuf *)mMalloc(sizeof(_fillbuf) * _FILLBUF_NUM);
	if(!p->buf) return FALSE;

	//描画用イメージ (A1)

	p->imgtmp_draw = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1BIT, &p->rcref);
	if(!p->imgtmp_draw) return FALSE;

	//参照色合成イメージ (アルファ値複数判定時)

	if(p->refimage_multi)
	{
		p->imgtmp_ref = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA, &p->rcref);
		if(!p->imgtmp_ref) return FALSE;
	}

	//アンチエイリアス自動用 (A1)

	if(p->type == DRAWFILL_TYPE_TRANSPARENT_AUTO)
	{
		p->imgtmp_draw2 = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1BIT, &p->rcref);
		if(!p->imgtmp_draw2) return FALSE;
	}

	return TRUE;
}

/** 作成と初期化
 *
 * imgref: 参照レイヤイメージの先頭
 * return: NULL で確保に失敗、または描画する必要なし */

DrawFill *DrawFill_new(TileImage *imgdst,TileImage *imgref,mPoint *ptstart,
	int type,int color_diff,int draw_density)
{
	DrawFill *p;

	//確保

	p = (DrawFill *)mMalloc0(sizeof(DrawFill));
	if(!p) return NULL;

	//値セット

	p->imgbits = APPDRAW->imgbits;
	p->imgdst = imgdst;
	p->imgref = imgref;
	p->pt_start = *ptstart;
	p->type = type;
	p->color_diff = Density100_to_bitval(color_diff, p->imgbits);
	p->draw_density = Density100_to_bitval(draw_density, p->imgbits);

	p->blend_alpha = (p->imgbits == 8)? _8bit_blend_alpha: _16bit_blend_alpha;

	//初期化

	if(_init(p))
		return p;
	else
	{
		DrawFill_free(p);
		return NULL;
	}
}


//===============================
// 実行
//===============================


/** 塗りつぶし実行 */

void DrawFill_run(DrawFill *p,void *drawcol)
{
	//----- 実行
	// imgtmp_draw に塗りつぶす点をセット

	p->imgtarget = p->imgtmp_draw;

	if(p->type != DRAWFILL_TYPE_TRANSPARENT_AUTO)
		//通常塗りつぶし
		_run_normal(p);
	else
	{
		//---- アンチエイリアス自動

		//水平: imgtmp_draw に描画
		
		_run_auto_horz(p);

		//垂直: imgtmp_draw2 に描画
		
		p->imgtarget = p->imgtmp_draw2;

		_run_auto_vert(p);

		//結合 (draw = draw + draw2)

		TileImage_combine_forA1(p->imgtmp_draw, p->imgtmp_draw2);
	}

	//作業用バッファ解放

	_free_tmp(p);

	//imgtmp_draw を元に、実際に描画

	TileImage_drawPixels_fromA1(p->imgdst, p->imgtmp_draw, drawcol);
}


//===============================
// スキャン
//===============================


/* 水平スキャン */

static void _scan_horz(DrawFill *p,
	int lx,int rx,int y,int oy,_fillbuf **pped,func_compare compfunc)
{
	while(lx <= rx)
	{
		//左の非境界点

		for(; lx < rx; lx++)
		{
			if(!(compfunc)(p, lx, y)) break;
		}

		if((compfunc)(p, lx, y)) break;

		(*pped)->lx = lx;

		//右の境界点

		for(; lx <= rx; lx++)
		{
			if((compfunc)(p, lx, y)) break;
		}

		(*pped)->rx = lx - 1;
		(*pped)->y  = y;
		(*pped)->oy = oy;

		if(++(*pped) == p->buf + _FILLBUF_NUM)
			(*pped) = p->buf;
	}
}

/* 垂直スキャン */

static void _scan_vert(DrawFill *p,
	int ly,int ry,int x,int ox,_fillbuf **pped,func_compare compfunc)
{
	while(ly <= ry)
	{
		//開始点

		for(; ly < ry; ly++)
		{
			if(!(compfunc)(p, x, ly)) break;
		}

		if((compfunc)(p, x, ly)) break;

		(*pped)->lx = ly;

		//終了点

		for(; ly <= ry; ly++)
		{
			if((compfunc)(p, x, ly)) break;
		}

		(*pped)->rx = ly - 1;
		(*pped)->y  = x;
		(*pped)->oy = ox;

		if(++(*pped) == p->buf + _FILLBUF_NUM)
			(*pped) = p->buf;
	}
}


//============================
// 通常塗りつぶし処理
//============================


/* 通常塗りつぶし:実行 */

void _run_normal(DrawFill *p)
{
	int lx,rx,ly,oy,_lx,_rx;
	mRect rcref;
	_fillbuf *pst,*ped;
	func_compare compfunc = p->compare;

	pst = p->buf;
	ped = p->buf + 1;

	pst->lx = pst->rx = p->pt_start.x;
	pst->y  = pst->oy = p->pt_start.y;

	rcref = p->rcref;

	//

	do
	{
		lx = pst->lx;
		rx = pst->rx;
		ly = pst->y;
		oy = pst->oy;

		_lx = lx - 1;
		_rx = rx + 1;

		if(++pst == p->buf + _FILLBUF_NUM)
			pst = p->buf;

		//現在の点が境界点か

		if(compfunc(p, lx, ly)) continue;

		//右方向の境界点を探す

		for(; rx < rcref.x2; rx++)
		{
			if(compfunc(p, rx + 1, ly)) break;
		}

		//左方向の境界点を探す

		for(; lx > rcref.x1; lx--)
		{
			if(compfunc(p, lx - 1, ly)) break;
		}

		//lx-rx の水平線描画
		// :タイル確保失敗時はエラー

		if(!TileImage_drawLineH_forA1(p->imgtmp_draw, lx, rx, ly))
			return;

		//真上の走査

		if(ly - 1 >= rcref.y1)
		{
			if(ly - 1 == oy)
			{
				_scan_horz(p, lx, _lx, ly - 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx, ly - 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx, rx, ly - 1, ly, &ped, compfunc);
		}

		//真下の走査

		if(ly + 1 <= rcref.y2)
		{
			if(ly + 1 == oy)
			{
				_scan_horz(p, lx, _lx, ly + 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx, ly + 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx, rx, ly + 1, ly, &ped, compfunc);
		}

	} while(pst != ped);
}


//===============================
// アンチエイリアス自動判定
//===============================
/*
  境界を見つける時に最大アルファ値を記憶しておき、アルファ値が下がった点を境界とする。
  これを水平方向、垂直方向の両方で実行する。
  結果をそれぞれ別のイメージに描画して、最後に2つのイメージを結合する。
*/


/* 水平走査 */

void _run_auto_horz(DrawFill *p)
{
	int lx,rx,ly,oy,_lx,_rx,a,max,lx2,rx2,flag,draw_density;
	mRect rcref;
	_fillbuf *pst,*ped;
	func_compare compfunc = p->compare;

	pst = p->buf;
	ped = p->buf + 1;

	pst->lx = pst->rx = p->pt_start.x;
	pst->y  = pst->oy = p->pt_start.y;

	rcref = p->rcref;
	draw_density = p->draw_density;

	//

	do
	{
		lx = pst->lx;
		rx = pst->rx;
		ly = pst->y;
		oy = pst->oy;

		_lx = lx - 1;
		_rx = rx + 1;

		if(++pst == p->buf + _FILLBUF_NUM)
			pst = p->buf;

		//透明ではないか

		if(compfunc(p, lx, ly)) continue;

		//右方向

		rx2 = rx;

		for(max = 0, flag = 1; rx < rcref.x2; rx++)
		{
			if(TileImage_isPixel_opaque(p->imgtarget, rx + 1, ly)) break;

			a = _getpixelref_alpha(p, rx + 1, ly);

			if(a) flag = 0;
			if(a >= draw_density || a < max) break;

			max = a;
			if(flag) rx2++;
		}

		//左方向

		lx2 = lx;

		for(max = 0, flag = 1; lx > rcref.x1; lx--)
		{
			if(TileImage_isPixel_opaque(p->imgtarget, lx - 1, ly)) break;

			a = _getpixelref_alpha(p, lx - 1, ly);

			if(a) flag = 0;
			if(a >= draw_density || a < max) break;

			max = a;
			if(flag) lx2--;
		}

		//lx-rx の水平線描画

		if(!TileImage_drawLineH_forA1(p->imgtarget, lx, rx, ly))
			return;

		//真上の走査

		if(ly - 1 >= rcref.y1)
		{
			if(ly - 1 == oy)
			{
				_scan_horz(p, lx2, _lx, ly - 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx2, ly - 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx2, rx2, ly - 1, ly, &ped, compfunc);
		}

		//真下の走査

		if(ly + 1 <= rcref.y2)
		{
			if(ly + 1 == oy)
			{
				_scan_horz(p, lx2, _lx, ly + 1, ly, &ped, compfunc);
				_scan_horz(p, _rx, rx2, ly + 1, ly, &ped, compfunc);
			}
			else
				_scan_horz(p, lx2, rx2, ly + 1, ly, &ped, compfunc);
		}

	} while(pst != ped);
}

/* 垂直走査 */

void _run_auto_vert(DrawFill *p)
{
	int ly,ry,xx,ox,_ly,_ry,a,max,ly2,ry2,flag,draw_density;
	mRect rcref;
	_fillbuf *pst,*ped;
	func_compare compfunc = p->compare;

	pst = p->buf;
	ped = p->buf + 1;

	pst->lx = pst->rx = p->pt_start.y;
	pst->y  = pst->oy = p->pt_start.x;

	rcref = p->rcref;
	draw_density = p->draw_density;

	//

	do
	{
		ly = pst->lx;
		ry = pst->rx;
		xx = pst->y;
		ox = pst->oy;

		_ly = ly - 1;
		_ry = ry + 1;

		if(++pst == p->buf + _FILLBUF_NUM)
			pst = p->buf;

		//透明ではないか

		if(compfunc(p, xx, ly)) continue;

		//下方向

		ry2 = ry;

		for(max = 0, flag = 1; ry < rcref.y2; ry++)
		{
			if(TileImage_isPixel_opaque(p->imgtarget, xx, ry + 1)) break;

			a = _getpixelref_alpha(p, xx, ry + 1);

			if(a) flag = 0;
			if(a >= draw_density || a < max) break;

			max = a;
			if(flag) ry2++;
		}

		//上方向

		ly2 = ly;

		for(max = 0, flag = 1; ly > rcref.y1; ly--)
		{
			if(TileImage_isPixel_opaque(p->imgtarget, xx, ly - 1)) break;

			a = _getpixelref_alpha(p, xx, ly - 1);

			if(a) flag = 0;
			if(a >= draw_density || a < max) break;

			max = a;
			if(flag) ly2--;
		}

		//ly-ry の垂直線描画

		if(!TileImage_drawLineV_forA1(p->imgtarget, ly, ry, xx))
			return;

		//左の走査

		if(xx - 1 >= rcref.x1)
		{
			if(xx - 1 == ox)
			{
				_scan_vert(p, ly2, _ly, xx - 1, xx, &ped, compfunc);
				_scan_vert(p, _ry, ry2, xx - 1, xx, &ped, compfunc);
			}
			else
				_scan_vert(p, ly2, ry2, xx - 1, xx, &ped, compfunc);
		}

		//右の走査

		if(xx + 1 <= rcref.x2)
		{
			if(xx + 1 == ox)
			{
				_scan_vert(p, ly2, _ly, xx + 1, xx, &ped, compfunc);
				_scan_vert(p, _ry, ry2, xx + 1, xx, &ped, compfunc);
			}
			else
				_scan_vert(p, ly2, ry2, xx + 1, xx, &ped, compfunc);
		}

	} while(pst != ped);
}


//=============================
// 参照レイヤのアルファ値取得
//=============================


/* 参照色イメージのタイルをセット
 *
 * return: 作成されたタイルのポインタ。TILEIMAGE_TILE_EMPTY で透明タイル */

static uint8_t *_set_tile_ref(DrawFill *p,int topx,int topy,uint8_t **pptile)
{
	TileImage *imgsrc;
	uint8_t *pd;
	int ix,iy,dstinc;
	uint16_t sa;

	dstinc = (p->imgbits == 8)? 1: 2;

	//先頭のレイヤは、アルファ値をそのままセット

	imgsrc = p->imgref;
	pd = *pptile;

	for(iy = 0; iy < 64; iy++)
	{
		for(ix = 0; ix < 64; ix++, pd += dstinc)
			TileImage_getPixel_alpha_buf(imgsrc, topx + ix, topy + iy, pd);
	}

	//以降はアルファ値を合成

	for(imgsrc = imgsrc->link; imgsrc; imgsrc = imgsrc->link)
	{
		pd = *pptile;

		for(iy = 0; iy < 64; iy++)
		{
			for(ix = 0; ix < 64; ix++, pd += dstinc)
			{
				TileImage_getPixel_alpha_buf(imgsrc, topx + ix, topy + iy, &sa);

				(p->blend_alpha)(pd, &sa);
			}
		}
	}

	//タイルがすべて透明なら、解放して TILEIMAGE_TILE_EMPTY をセット

	if(TileImage_tile_isTransparent_forA(*pptile, (p->imgbits == 8)? 64 * 64: 64 * 64 * 2))
	{
		TileImage_freeTile(pptile);

		*pptile = TILEIMAGE_TILE_EMPTY;
	}

	return *pptile;
}

/* [アルファ値判定時] 参照レイヤをすべて合成したアルファ値を取得
 *
 * imgtmp_ref にタイルがなければ作成。
 *
 * return: アルファ値 */

int _getpixelref_alpha(DrawFill *p,int x,int y)
{
	TileImage *img;
	uint8_t **pptile,*tile;
	int tx,ty,topx,topy;
	uint16_t a;

	//単体なら直接取得

	if(!p->refimage_multi)
	{
		TileImage_getPixel_alpha_buf(p->imgref, x, y, &a);

		return (p->imgbits == 8)? *((uint8_t *)&a): a;
	}

	//------- 複数時

	img = p->imgtmp_ref;

	//タイル配列位置

	if(!TileImage_pixel_to_tile(img, x, y, &tx, &ty))
		return 0;

	pptile = TILEIMAGE_GETTILE_BUFPT(img, tx, ty);

	//タイル

	tile = *pptile;

	if(tile == TILEIMAGE_TILE_EMPTY)
		//透明タイル
		return 0;
	else if(!tile)
	{
		//タイル作成

		*pptile = TileImage_allocTile(img);
		if(!(*pptile)) return 0;

		TileImage_tile_to_pixel(img, tx, ty, &topx, &topy);
		
		tile = _set_tile_ref(p, topx, topy, pptile);
		
		if(tile == TILEIMAGE_TILE_EMPTY) return 0;
	}

	//存在するタイルからピクセル値取得

	x = (x - img->offx) & 63;
	y = (y - img->offy) & 63;

	if(p->imgbits == 8)
		return *((uint8_t *)tile + y * 64 + x);
	else
		return *((uint16_t *)tile + y * 64 + x);
}


//===================================
// ピクセル値の比較関数
//===================================
/*
  開始点と異なる色 (塗りつぶさない部分) なら TRUE を返す。
  imgtarget にすでに点がある場合も境界とみなす。
*/


/* RGB */

mlkbool _compare_rgb(DrawFill *p,int x,int y)
{
	uint64_t col;
	int diff;
	_intcolor ic;

	if(TileImage_isPixel_opaque(p->imgtarget, x, y))
		return TRUE;

	//参照する色

	if(p->type == DRAWFILL_TYPE_RGB)
		TileImage_getPixel(p->imgref, x, y, &col);
	else
		ImageCanvas_getPixel_rgba(APPDRAW->imgcanvas, x, y, &col);

	_set_intcolor(p, &ic, &col);

	//判定

	if(p->start_col.a == 0)
	{
		//開始点が透明なら、不透明が境界
		
		return (ic.a != 0);
	}
	else if(ic.a == 0)
	{
		//開始点が透明でなく、対象が透明なら、境界

		return TRUE;
	}
	else
	{
		//色差が範囲外なら境界

		diff = p->color_diff;

		if(diff == 0)
		{
			return (ic.r != p->start_col.r || ic.g != p->start_col.g || ic.b != p->start_col.b);
		}
		else
		{
			return (ic.r < p->start_col.r - diff || ic.r > p->start_col.r + diff
				|| ic.g < p->start_col.g - diff || ic.g > p->start_col.g + diff
				|| ic.b < p->start_col.b - diff || ic.b > p->start_col.b + diff);
		}
	}
}

/* アルファ値 */

mlkbool _compare_alpha(DrawFill *p,int x,int y)
{
	int a;

	if(TileImage_isPixel_opaque(p->imgtarget, x, y))
		return TRUE;
	
	a = _getpixelref_alpha(p, x, y);

	return (a < p->start_col.a - p->color_diff || a > p->start_col.a + p->color_diff);
}

/* A=0 */

mlkbool _compare_alpha_0(DrawFill *p,int x,int y)
{
	return (TileImage_isPixel_opaque(p->imgtarget, x, y)
		|| _getpixelref_alpha(p, x, y) != 0);
}

/* 不透明 */

mlkbool _compare_opaque(DrawFill *p,int x,int y)
{
	return (TileImage_isPixel_opaque(p->imgtarget, x, y)
		|| _getpixelref_alpha(p, x, y) == 0);
}

