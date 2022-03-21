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
 * TileImage
 *
 * 選択範囲関連
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_rectbox.h"
#include "mlk_rand.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "pv_tileimage.h"

#include "canvasinfo.h"
#include "drawpixbuf.h"
#include "table_data.h"



/** 選択範囲反転
 *
 * キャンバス範囲内のみ反転し、キャンバス範囲外は常にクリアする。 */

void TileImage_inverseSelect(TileImage *p)
{
	int ix,iy,cw,ch,fy;
	mRect rc;
	uint64_t col;

	//描画可能な範囲すべて処理
	TileImage_getCanDrawRect_pixel(p, &rc);

	cw = TILEIMGWORK->imgw;
	ch = TILEIMGWORK->imgh;

	g_tileimage_dinfo.func_pixelcol = TILEIMGWORK->pixcolfunc[TILEIMAGE_PIXELCOL_INVERSE_ALPHA];

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		fy = (iy >= 0 && iy < ch);
		
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			//キャンバス範囲内なら反転、範囲外ならクリア
			
			col = (fy && ix >= 0 && ix < cw)? (uint64_t)-1: 0;

			TileImage_setPixel_work(p, ix, iy, &col);
		}
	}
}

/** src で不透明な部分に点を描画 */

void TileImage_drawPixels_fromImage_opacity(TileImage *dst,TileImage *src,const mRect *rcarea)
{
	int ix,iy;
	mRect rc;
	uint64_t col,coldraw;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	rc = *rcarea;
	coldraw = (uint64_t)-1;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &col);

			if(!(TILEIMGWORK->is_transparent)(&col))
				(setpix)(dst, ix, iy, &coldraw);
		}
	}
}

/** src の指定色部分に点を描画 */

void TileImage_drawPixels_fromImage_color(TileImage *dst,TileImage *src,const RGBcombo *scol,const mRect *rcarea)
{
	int ix,iy,bits,f;
	mRect rc;
	RGBA8 col8;
	RGB8 sc8;
	RGBA16 col16;
	RGB16 sc16;
	uint64_t coldraw;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	bits = TILEIMGWORK->bits;
	rc = *rcarea;
	coldraw = (uint64_t)-1;

	if(bits == 8)
		sc8 = scol->c8;
	else
		sc16 = scol->c16;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(bits == 8)
			{
				TileImage_getPixel(src, ix, iy, &col8);

				f = (col8.a && col8.r == sc8.r && col8.g == sc8.g && col8.b == sc8.b);
			}
			else
			{
				TileImage_getPixel(src, ix, iy, &col16);

				f = (col16.a && col16.r == sc16.r && col16.g == sc16.g && col16.b == sc16.b);
			}

			if(f)
				(setpix)(dst, ix, iy, &coldraw);
		}
	}
}

/** A1 イメージで、点がある px 範囲を正確に取得
 *
 * 空ではない状態であること。
 * 選択範囲イメージのファイル出力時用。 */

void TileImage_A1_getHaveRect_real(TileImage *p,mRect *dst)
{
	uint8_t **pp,*ps8,f;
	uint64_t *ps64;
	int i,ix,iy,cur;
	mRect rc;

	//イメージがあるタイル位置の範囲

	mRectEmpty(&rc);

	pp = p->ppbuf;

	for(iy = 0; iy < p->tileh; iy++)
	{
		for(ix = 0; ix < p->tilew; ix++, pp++)
		{
			if(*pp)
				mRectIncPoint(&rc, ix, iy);
		}
	}

	//点がある一番上のタイルから、上端を取得
	// :(x1-x2,y1) の各タイルで、点がある最小のY位置取得

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y1);
	cur = 63;

	for(i = rc.x2 - rc.x1 + 1; i > 0; i--, pp++)
	{
		if(!(*pp)) continue;

		//横の 64px 分は 8byte であるため、uint64 で判定

		ps64 = (uint64_t *)*pp;
		
		for(iy = 0; iy < 64 && iy < cur; iy++, ps64++)
		{
			if(*ps64) cur = iy;
		}
	}

	dst->y1 = p->offy + (rc.y1 << 6) + cur;

	//下

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y2);
	cur = 0;

	for(i = rc.x2 - rc.x1 + 1; i > 0; i--, pp++)
	{
		if(!(*pp)) continue;

		ps64 = (uint64_t *)(*pp + 8 * 63);
		
		for(iy = 63; iy >= 0 && iy > cur; iy--, ps64--)
		{
			if(*ps64) cur = iy;
		}
	}

	dst->y2 = p->offy + (rc.y2 << 6) + cur;

	//左

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y1);
	cur = 63;

	for(i = rc.y2 - rc.y1 + 1; i > 0; i--, pp += p->tilew)
	{
		if(!(*pp)) continue;
	
		for(ix = 0, f = 0x80; ix < 64 && ix < cur; ix++)
		{
			ps8 = *pp + (ix >> 3);
		
			for(iy = 0; iy < 64; iy++, ps8 += 8)
			{
				if(*ps8 & f)
				{
					cur = ix;
					break;
				}
			}

			f >>= 1;
			if(f == 0) f = 0x80;
		}
	}

	dst->x1 = p->offx + (rc.x1 << 6) + cur;

	//右

	pp = TILEIMAGE_GETTILE_BUFPT(p, rc.x2, rc.y1);
	cur = 0;

	for(i = rc.y2 - rc.y1 + 1; i > 0; i--, pp += p->tilew)
	{
		if(!(*pp)) continue;

		for(ix = 63, f = 1; ix >= 0 && ix > cur; ix--)
		{
			ps8 = *pp + (ix >> 3);
		
			for(iy = 0; iy < 64; iy++, ps8 += 8)
			{
				if(*ps8 & f)
				{
					cur = ix;
					break;
				}
			}

			if(f == 0x80)
				f = 1;
			else
				f <<= 1;
		}
	}

	dst->x2 = p->offx + (rc.x2 << 6) + cur;
}


//=====================================
// 選択範囲 編集用
//=====================================


/** 選択範囲内のイメージをコピー or 切り取りして、作成
 *
 * - 色関数は <上書き> をセットしておく。
 * - dst は、rcimg の範囲でタイル配列が作成される。
 *
 * rcimg: イメージ範囲
 * cut: TRUE で、src の範囲内を透明に */

TileImage *TileImage_createSelCopyImage(TileImage *src,TileImage *sel,const mRect *rcimg,mlkbool cut)
{
	TileImage *dst;
	int ix,iy,bits,a;
	uint64_t col,colerase = 0;
	mRect rc;

	//作成

	dst = TileImage_newFromRect(src->type, rcimg);
	if(!dst) return NULL;

	dst->col = src->col;

	//

	rc = *rcimg;
	bits = TILEIMGWORK->bits;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_isPixel_opaque(sel, ix, iy))
			{
				//src の色
				
				TileImage_getPixel(src, ix, iy, &col);

				a = (bits == 8)? *((uint8_t *)&col + 3): *((uint16_t *)&col + 3);

				//

				if(a)
				{
					//コピー
					
					TileImage_setPixel_new(dst, ix, iy, &col);

					//消去

					if(cut)
						TileImage_setPixel_draw_direct(src, ix, iy, &colerase);
				}
			}
		}
	}

	return dst;
}

/** イメージの貼り付け (重ね塗り用)
 *
 * rcimg: 処理する範囲。src の指定範囲を、dst の同じ位置にセット。 */

void TileImage_pasteSelImage(TileImage *dst,TileImage *src,const mRect *rcimg)
{
	int ix,iy;
	uint64_t col;
	mRect rc;

	rc = *rcimg;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &col);

			if(!(TILEIMGWORK->is_transparent)(&col))
			{
				TileImage_setPixel_draw_direct(dst, ix, iy, &col);
			}
		}
	}
}


//=========================
// 矩形選択
//=========================


/** 矩形範囲をコピー/切り取りしてイメージを作成
 *
 * - 切り貼りツール、矩形編集ツール。
 * - 作成後のイメージは、(0,0)-(box->w x box->h) */

TileImage *TileImage_createBoxSelCopyImage(TileImage *src,const mBox *box,mlkbool cut)
{
	TileImage *dst;
	mRect rc;
	int ix,iy;
	uint64_t col,colerase = 0;

	dst = TileImage_new(src->type, box->w, box->h);
	if(!dst) return NULL;

	dst->col = src->col;

	//

	mRectSetBox(&rc, box);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &col);

			if(!(TILEIMGWORK->is_transparent)(&col))
			{
				if(cut)
					TileImage_setPixel_draw_direct(src, ix, iy, &colerase);

				TileImage_setPixel_new(dst, ix - rc.x1, iy - rc.y1, &col);
			}
		}
	}

	return dst;
}

/** 貼り付け (上書きあり) */

void TileImage_pasteBoxSelImage(TileImage *dst,TileImage *src,const mBox *box)
{
	int ix,iy;
	uint64_t col;
	mRect rc;

	mRectSetBox(&rc, box);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &col);

			TileImage_setPixel_draw_direct(dst, ix, iy, &col);
		}
	}
}


//=========================
// スタンプ
//=========================


/** スタンプ用イメージ作成
 *
 * src の rcimg 範囲内のイメージで、sel に点がある部分をコピー。
 * (rcimg->x1, rcimg->y1) から (0,0) の位置にコピーされる。 */

TileImage *TileImage_createStampImage(TileImage *src,TileImage *sel,const mRect *rcimg)
{
	TileImage *dst;
	int ix,iy;
	mRect rc;
	uint64_t col;

	rc = *rcimg;

	//作成

	dst = TileImage_new(src->type, rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);
	if(!dst) return NULL;

	dst->col = src->col;

	//コピー

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_isPixel_opaque(sel, ix, iy))
			{
				TileImage_getPixel(src, ix, iy, &col);

				TileImage_setPixel_new_notp(dst, ix - rc.x1, iy - rc.y1, &col);
			}
		}
	}

	return dst;
}

/* 貼り付け (回転以外) */

static void _stamp_paste_normal(TileImage *dst,int x,int y,int trans,
	TileImage *src,int srcw,int srch)
{
	int ix,iy,sx,sy,frand = 0;
	uint64_t col;

	x -= srcw / 2;
	y -= srch / 2;

	if(trans >= 3)
		frand = mRandSFMT_getIntRange(TILEIMGWORK->rand, 0, 1);

	for(iy = 0; iy < srch; iy++)
	{
		for(ix = 0; ix < srcw; ix++)
		{
			sx = ix;
			sy = iy;
		
			switch(trans)
			{
				case 0:
					break;
				//左右反転
				case 1:
					sx = srcw - 1 - ix;
					break;
				//上下反転
				case 2:
					sy = srch - 1 - iy;
					break;
				//ランダム左右反転
				case 3:
					if(frand)
						sx = srcw - 1 - ix;
					break;
				//ランダム上下反転
				case 4:
					if(frand)
						sy = srch - 1 - iy;
					break;
			}
			
			TileImage_getPixel(src, sx, sy, &col);

			if(!(TILEIMGWORK->is_transparent)(&col))
				TileImage_setPixel_draw_direct(dst, x + ix, y + iy, &col);
		}
	}
}

/* 貼り付け (ランダム回転) */

static void _stamp_paste_rotate(TileImage *dst,int x,int y,
	TileImage *src,int srcw,int srch)
{
	mRect rc;
	double dcos,dsin,dxx,dxy,dyx,dyy;
	int ix,iy;
	uint64_t col;

	//角度

	ix = mRandSFMT_getIntRange(TILEIMGWORK->rand, 0, 511);

	dcos = TABLEDATA_GET_COS(ix);
	dsin = TABLEDATA_GET_SIN(ix);

	//回転後の範囲

	__TileImage_getRotateRect(&rc, srcw, srch, -dcos, -dsin);

	//描画先左上のソース位置

	dyx = rc.x1 * dcos - rc.y1 * dsin + (srcw / 2);
	dyy = rc.x1 * dsin + rc.y1 * dcos + (srch / 2);

	//

	mRectMove(&rc, x, y);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dxx = dyx;
		dxy = dyy;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			//FALSE = 範囲外 or 透明
			
			if(TileImage_getColor_bicubic(src, dxx, dxy, srcw, srch, &col))
				TileImage_setPixel_draw_direct(dst, ix, iy, &col);
		
			dxx += dcos;
			dxy += dsin;
		}

		dyx -= dsin;
		dyy += dcos;
	}
}

/** スタンプイメージの貼り付け (重ね塗り)
 *
 * src: (0,0)-(srcw x srch) の範囲 */

void TileImage_pasteStampImage(TileImage *dst,int x,int y,int trans,
	TileImage *src,int srcw,int srch)
{
	if(trans == 5)
		_stamp_paste_rotate(dst, x, y, src, srcw, srch);
	else
		_stamp_paste_normal(dst, x, y, trans, src, srcw, srch);
}


//=====================================
// 共通
//=====================================

/* 64x64 タイルの左上を(0,0)とした時のバッファバイト位置
 * (x が -1 の時、8で割った値が -1 になるように、"/ 8" ではなく、">> 3" とする) */

#define _TILEEX2_BUFPOS(x,y)  (((y) + 2) * 10 + ((x) >> 3) + 1)


/** 指定タイルの周囲 2px を含む範囲の 1bit データを取得
 *
 * return: セットされたデータに一つでも点があるか */

static mlkbool _get_tilebuf_ex2(TileImage *p,uint8_t *dstbuf,uint8_t **pptile,int tx,int ty)
{
	uint8_t *ps,*pd,flags;
	uint64_t *p64;
	int i;

	//クリア

	memset(dstbuf, 0, 10 * 68);

	//タイルのデータをコピー (8byte 単位)

	ps = *pptile;
	pd = dstbuf + _TILEEX2_BUFPOS(0,0);

	for(i = 64; i; i--)
	{
		*((uint64_t *)pd) = *((uint64_t *)ps);

		ps += 8;
		pd += 10;
	}

	//上下左右のタイルが範囲内かどうかのフラグ

	flags = (tx != 0) | ((tx < p->tilew - 1) << 1)
		| ((ty != 0) << 2) | ((ty < p->tileh - 1) << 3);

	//左のタイルの右端 2px

	if((flags & 1) && *(pptile - 1))
	{
		ps = *(pptile - 1) + 7;
		pd = dstbuf + _TILEEX2_BUFPOS(-1,0);

		for(i = 64; i; i--, ps += 8, pd += 10)
			*pd = *ps;
	}

	//右のタイルの左端 2px

	if((flags & 2) && *(pptile + 1))
	{
		ps = *(pptile + 1);
		pd = dstbuf + _TILEEX2_BUFPOS(64,0);

		for(i = 64; i; i--, ps += 8, pd += 10)
			*pd = *ps;
	}

	//上のタイルの下端 2px

	if((flags & 4) && *(pptile - p->tilew))
	{
		ps = *(pptile - p->tilew) + 62 * 8;
		pd = dstbuf + _TILEEX2_BUFPOS(0,-2);

		for(i = 2; i; i--, ps += 8, pd += 10)
			*((uint64_t *)pd) = *((uint64_t *)ps);
	}

	//下のタイルの上端 2px

	if((flags & 8) && *(pptile + p->tilew))
	{
		ps = *(pptile + p->tilew);
		pd = dstbuf + _TILEEX2_BUFPOS(0,64);

		for(i = 2; i; i--, ps += 8, pd += 10)
			*((uint64_t *)pd) = *((uint64_t *)ps);
	}

	//左上 (-1,-1)

	if((flags & 5) == 5)
	{
		ps = *(pptile - 1 - p->tilew);

		if(ps && (ps[8 * 64 - 1] & 1))
			dstbuf[_TILEEX2_BUFPOS(-1,-1)] |= 1;
	}

	//左下 (-1,64)

	if((flags & 9) == 9)
	{
		ps = *(pptile - 1 + p->tilew);

		if(ps && (ps[7] & 1))
			dstbuf[_TILEEX2_BUFPOS(-1,64)] |= 1;
	}

	//右上 (64,-1)

	if((flags & 6) == 6)
	{
		ps = *(pptile + 1 - p->tilew);

		if(ps && (ps[8 * 63] & 0x80))
			dstbuf[_TILEEX2_BUFPOS(64,-1)] |= 0x80;
	}

	//右下 (64,64)

	if((flags & 10) == 10)
	{
		ps = *(pptile + 1 + p->tilew);

		if(ps && (*ps & 0x80))
			dstbuf[_TILEEX2_BUFPOS(64,64)] |= 0x80;
	}

	//一つでも点があるか判定

	p64 = (uint64_t *)dstbuf;

	for(i = 10 * 68 / 8; i && *p64 == 0; i--, p64++);

	return (i != 0);
}


//=====================================
// 選択範囲 拡張/縮小
//=====================================


/* タイル 1px 拡張 */

static void _expand_tile_inflate(TileImage *p,uint8_t *buf,int px,int py)
{
	uint8_t *ps,f;
	int ix,iy,n;
	uint64_t col;

	col = (uint64_t)-1;

	//周囲1px分を含む範囲で、点がなくて上下左右いずれかに点がある場合、点を追加

	ps = buf + _TILEEX2_BUFPOS(-1, -1);

	for(iy = -1; iy <= 64; iy++, ps++)
	{
		for(ix = -1, f = 1; ix <= 64; ix++)
		{
			if(!(*ps & f))
			{
				//上下

				n = (ps[-10] & f) | (ps[10] & f);

				//左
			
				n |= (f == 0x80)? ps[-1] & 1: *ps & (f << 1);

				//右

				n |= (f == 1)? ps[1] & 0x80: *ps & (f >> 1);

				//上下左右いずれかに点があればセット

				if(n)
					TileImage_setPixel_work(p, px + ix, py + iy, &col);
			}

			f >>= 1;
			if(!f) f = 0x80, ps++;
		}
	}
}

/* タイル 1px 縮小 */

static void _expand_tile_deflate(TileImage *p,uint8_t *buf,int px,int py)
{
	uint8_t *ps,f;
	int ix,iy,n;
	uint64_t col = 0;

	//64x64 の範囲で、点があって上下左右いずれかに点がない場合、点を消去

	ps = buf + _TILEEX2_BUFPOS(0, 0);

	for(iy = 0; iy < 64; iy++, ps += 2)
	{
		for(ix = 0, f = 0x80; ix < 64; ix++)
		{
			if(*ps & f)
			{
				//上下

				n = !(ps[-10] & f) | !(ps[10] & f);

				//左
			
				n |= !((f == 0x80)? ps[-1] & 1: *ps & (f << 1));

				//右

				n |= !((f == 1)? ps[1] & 0x80: *ps & (f >> 1));

				//上下左右いずれかに点がない場合消去

				if(n)
					TileImage_setPixel_work(p, px + ix, py + iy, &col);
			}

			f >>= 1;
			if(!f) f = 0x80, ps++;
		}
	}
}

/** 選択範囲 拡張/縮小
 *
 * pxcnt: 正で拡張、負で縮小  */

void TileImage_expandSelect(TileImage *p,int pxcnt,mPopupProgress *prog)
{
	TileImage *tmp;
	uint8_t **pptile,*buf;
	int i,ix,iy,w,h,px,py;

	//作業用タイルバッファ

	buf = (uint8_t *)mMalloc(10 * 68);
	if(!buf) return;

	//

	i = (pxcnt < 0)? -pxcnt: pxcnt;

	mPopupProgressThreadSetMax(prog, i * 5);

	g_tileimage_dinfo.func_pixelcol = TILEIMGWORK->pixcolfunc[TILEIMAGE_PIXELCOL_OVERWRITE];

	//

	for(; i > 0; i--)
	{
		//判定元用にコピー

		tmp = TileImage_newClone(p);
		if(!tmp) break;

		//tmp をソースとして、タイルごとに処理し、p に点セット

		pptile = tmp->ppbuf;
		w = tmp->tilew;
		h = tmp->tileh;

		mPopupProgressThreadSubStep_begin(prog, 5, h);

		for(iy = 0, py = tmp->offy; iy < h; iy++, py += 64)
		{
			for(ix = 0, px = tmp->offx; ix < w; ix++, px += 64, pptile++)
			{
				if(*pptile && _get_tilebuf_ex2(tmp, buf, pptile, ix, iy))
				{
					if(pxcnt > 0)
						_expand_tile_inflate(p, buf, px, py);
					else
						_expand_tile_deflate(p, buf, px, py);
				}
			}

			mPopupProgressThreadSubStep_inc(prog);
		}

		//

		TileImage_free(tmp);
	}

	mFree(buf);
}


//=====================================
// キャンバスに輪郭を描画
//=====================================
/*
  作業用バッファは、64x64 + 周囲2px の 1bit データ。
  左端は 2bit 分余白。
  [2bit余白][一つ左2px]+[タイル64]+[一つ右2px][2bit余白] で、横は 10 byte。
*/

typedef struct
{
	CanvasDrawInfo *cdinfo;
	mRect rcclip_img,	//クリッピング範囲 (イメージ)
		rcclip_canv;	//クリッピング範囲 (キャンバス)
	double dadd[4];
	int fpixelbox;		//拡大表示時、ピクセルごとに枠を描画
}_drawselinfo;



/* 拡大表示時の1pxの輪郭描画 */

static void _drawsel_drawedge_point(mPixbuf *pixbuf,
	double dx,double dy,double *padd,int flags,const mRect *rcclip)
{
	dx += 0.5;
	dy += 0.5;

	if(flags & 1)
		drawpixbuf_line_selectEdge(pixbuf, dx, dy, dx + padd[0], dy + padd[1], rcclip);

	if(flags & 2)
		drawpixbuf_line_selectEdge(pixbuf, dx + padd[2], dy + padd[3], dx + padd[0] + padd[2], dy + padd[1] + padd[3], rcclip);

	if(flags & 4)
		drawpixbuf_line_selectEdge(pixbuf, dx, dy, dx + padd[2], dy + padd[3], rcclip);

	if(flags & 8)
		drawpixbuf_line_selectEdge(pixbuf, dx + padd[0], dy + padd[1], dx + padd[0] + padd[2], dy + padd[1] + padd[3], rcclip);
}

/* 輪郭描画
 *
 * px,py: 描画先の px 位置 (実際のタイルの (-1,-1)) */

static void _drawsel_drawedge(mPixbuf *pixbuf,uint8_t *tilebuf,int px,int py,_drawselinfo *info)
{
	int sx,sy,w,h,img_right,img_bottom,ix,iy,x,y;
	uint8_t *ps,*psY,stf,f,flags,fpixelbox,flags_y,tmp;
	double ddx,ddy,ddx2,ddy2,*dadd;
	mRect rc;

	//イメージ座標からクリッピング

	rc = info->rcclip_img;	//x2,y2 は +1
	sx = sy = 0;
	w = h = 66;

	if(px < rc.x1) w += px - rc.x1, sx += rc.x1 - px, px = rc.x1;
	if(py < rc.y1) h += py - rc.y1, sy += rc.y1 - py, py = rc.y1;
	if(px + w > rc.x2) w = rc.x2 - px;
	if(py + h > rc.y2) h = rc.y2 - py;

	if(w <= 0 || h <= 0) return;

	//イメージの右端/下端

	img_right = TILEIMGWORK->imgw - 1;
	img_bottom = TILEIMGWORK->imgh - 1;

	//タイルデータの開始位置
	//(周囲1px分も描画するので、位置は -1)

	sx--, sy--;

	ps = tilebuf + _TILEEX2_BUFPOS(sx, sy);

	stf = 1 << (7 - (sx & 7));

	//

	CanvasDrawInfo_image_to_canvas(info->cdinfo, px, py, &ddx2, &ddy2);

	fpixelbox = info->fpixelbox;
	dadd = info->dadd;

	//---------

	y = py;

	for(iy = 0; iy < h; iy++, y++)
	{
		x = px;
		f = stf;
		psY = ps;

		//上端/下端のフラグ
		flags_y = (y == 0) | ((y == img_bottom) << 1);

		ddx = ddx2;
		ddy = ddy2;

		for(ix = 0; ix < w; ix++, x++)
		{
			if(*ps & f)
			{
				//------- 点がある場合
				
				//端の位置かどうかのフラグ
				// :イメージ全体の端だった場合は輪郭描画
				
				flags = flags_y | ((x == 0) << 2) | ((x == img_right) << 3);

				//

				if(flags)
				{
					if(fpixelbox)
						_drawsel_drawedge_point(pixbuf, ddx, ddy, dadd, flags, &info->rcclip_canv);
					else
						drawpixbuf_setPixel_selectEdge(pixbuf, (int)(ddx + 0.5), (int)(ddy + 0.5), &info->rcclip_canv);
				}
			}
			else
			{
				//------ 点がない場合、上下左右いずれかに点があれば輪郭描画

				flags = 0;

				//上

				if(ps[-10] & f) flags |= 1;

				//下

				if(ps[10] & f) flags |= 2;

				//左

				tmp = (f == 0x80)? ps[-1] & 1: *ps & (f << 1);

				if(tmp) flags |= 4;

				//右

				tmp = (f == 1)? ps[1] & 0x80: *ps & (f >> 1);

				if(tmp) flags |= 8;

				//描画

				if(flags)
				{
					if(fpixelbox)
						_drawsel_drawedge_point(pixbuf, ddx, ddy, dadd, flags, &info->rcclip_canv);
					else
						drawpixbuf_setPixel_selectEdge(pixbuf, (int)(ddx + 0.5), (int)(ddy + 0.5), &info->rcclip_canv);
				}
			}

			f >>= 1;
			if(!f) f = 0x80, ps++;

			ddx += dadd[0];
			ddy += dadd[1];
		}

		ps = psY + 10;

		ddx2 += dadd[2];
		ddy2 += dadd[3];
	}
}

/** キャンバスに選択範囲の輪郭を描画
 *
 * cdinfo: キャンバスの描画情報
 * boximg: 描画するキャンバス範囲に相当するイメージ範囲 */

void TileImage_drawSelectEdge(TileImage *p,mPixbuf *pixbuf,CanvasDrawInfo *cdinfo,const mBox *boximg)
{
	_drawselinfo info;
	TileImageTileRectInfo tinfo;
	uint8_t **pptile,*buf;
	int ix,iy,px,py;

	if(!p->ppbuf) return;

	//イメージ範囲 -> タイル範囲

	pptile = TileImage_getTileRectInfo(p, &tinfo, boximg);
	if(!pptile) return;

	//作業用バッファ

	buf = (uint8_t *)mMalloc(10 * 68);
	if(!buf) return;

	//情報

	info.cdinfo = cdinfo;
	info.rcclip_img = tinfo.rcclip;

	mRectSetBox(&info.rcclip_canv, &cdinfo->boxdst);

	info.dadd[0] = cdinfo->param->scale * cdinfo->param->cos;
	info.dadd[1] = cdinfo->param->scale * cdinfo->param->sin;
	info.dadd[2] = -info.dadd[1];
	info.dadd[3] = info.dadd[0];

	if(cdinfo->mirror)
	{
		info.dadd[0] = -info.dadd[0];
		info.dadd[1] = -info.dadd[1];
	}

	info.fpixelbox = (cdinfo->param->scale > 1);

	//タイルごとに処理

	for(iy = tinfo.rctile.y1, py = tinfo.pxtop.y; iy <= tinfo.rctile.y2; iy++, py += 64, pptile += tinfo.pitch_tile)
	{
		for(ix = tinfo.rctile.x1, px = tinfo.pxtop.x; ix <= tinfo.rctile.x2; ix++, px += 64, pptile++)
		{
			if(*pptile)
			{
				//一つも点がない場合は何もしない
			
				if(_get_tilebuf_ex2(p, buf, pptile, ix, iy))
					_drawsel_drawedge(pixbuf, buf, px - 1, py - 1, &info);
			}
		}
	}
	
	//

	mFree(buf);
}

