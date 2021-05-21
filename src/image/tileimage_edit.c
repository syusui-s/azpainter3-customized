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

/**********************************
 * TileImage: 編集
 **********************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_pixbuf.h"
#include "mlk_popup_progress.h"
#include "mlk_rectbox.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "pv_tileimage.h"

#include "canvasinfo.h"


/** カラータイプ変換
 *
 * lum_to_alpha: 色の輝度をアルファ値へ変換 */

void TileImage_convertColorType(TileImage *p,int newtype,mlkbool lum_to_alpha)
{
	uint8_t *tilebuf,**pp;
	uint32_t i;
	int srctype;
	TileImageColFunc_getTileRGBA func_gettile;
	TileImageColFunc_setTileRGBA func_settile;
	TileImageColFunc_isTransparentTile func_is_trans;

	//作業用タイルバッファ (RGBA 分)

	tilebuf = TileImage_global_allocTileBitMax();
	if(!tilebuf) return;

	//タイプ変更

	srctype = p->type;

	p->type = newtype;

	__TileImage_setTileSize(p);

	//関数

	func_gettile = TILEIMGWORK->colfunc[srctype].gettile_rgba;
	func_settile = TILEIMGWORK->colfunc[newtype].settile_rgba;
	func_is_trans = TILEIMGWORK->colfunc[newtype].is_transparent_tile;

	//タイル変換

	pp = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pp++)
	{
		if(!(*pp)) continue;
		
		//元タイプで、RGBA タイルとして取得
		
		(func_gettile)(p, tilebuf, *pp);

		//タイル再確保

		mFree(*pp);

		*pp = TileImage_allocTile(p);
		if(!(*pp))
		{
			mFree(tilebuf);
			return;
		}

		//RGBA タイルからセット

		(func_settile)(*pp, tilebuf, lum_to_alpha);

		//すべて透明なら解放

		if((func_is_trans)(*pp))
			TileImage_freeTile(pp);
	}

	mFree(tilebuf);
}

/** タイルを指定ビット数に変換してコピー
 *
 * tilesize は新しいビット数の値になっていること。 */

void TileImage_copyTile_convert(TileImage *p,uint8_t *dst,uint8_t *src,void *tblbuf,int bits)
{
	uint8_t *ps8,*pd8;
	uint16_t *ps16,*pd16;
	int i;

	i = p->tilesize;

	if(bits == 8)
	{
		//16->8bit
		
		ps16 = (uint16_t *)src;
		pd8 = dst;
		
		for(; i; i--, ps16++)
			*(pd8++) = *((uint8_t *)tblbuf + *ps16);
	}
	else
	{
		//8->16bit
		
		ps8 = src;
		pd16 = (uint16_t *)dst;
		
		for(i >>= 1; i; i--, ps8++)
			*(pd16++) = *((uint16_t *)tblbuf + *ps8);
	}
}

/** ビット数の変換 (8bit <-> 16bit)
 *
 * bits: 変換後のビット数
 * tblbuf: 変換用テーブル */

void TileImage_convertBits(TileImage *p,int bits,void *tblbuf,mPopupProgress *prog)
{
	uint8_t **pptile,*newbuf;
	uint32_t i;

	//A1bit はそのまま
	
	if(p->type == TILEIMAGE_COLTYPE_ALPHA1BIT)
	{
		mPopupProgressThreadAddPos(prog, 5);
		return;
	}

	//タイルサイズ変更 (タイル確保前に行うこと)

	p->tilesize = TileImage_global_getTileSize(p->type, bits);

	//タイル変換

	mPopupProgressThreadSubStep_begin(prog, 5, p->tilew * p->tileh);

	pptile = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pptile++)
	{
		if(*pptile)
		{
			//新しいタイル確保

			newbuf = TileImage_allocTile(p);

			//変換

			if(newbuf)
				TileImage_copyTile_convert(p, newbuf, *pptile, tblbuf, bits);

			//置き換え (NULL の場合も常に)

			mFree(*pptile);

			*pptile = newbuf;
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/** src のイメージを dst の同じ位置にコピー
 * 
 * フィルタのプレビューイメージ用。
 * [!] dst は空状態である。 */

void TileImage_copyImage_rect(TileImage *dst,TileImage *src,const mRect *rc)
{
	int ix,iy;
	uint64_t col;
	mRect rc1 = *rc;

	for(iy = rc1.y1; iy <= rc1.y2; iy++)
	{
		for(ix = rc1.x1; ix <= rc1.x2; ix++)
		{
			TileImage_getPixel(src, ix, iy, &col);

			TileImage_setPixel_new_notp(dst, ix, iy, &col);
		}
	}
}


//===============================
// 色置き換え
//===============================


/** 色置き換え (透明 -> 指定色) */

void TileImage_replaceColor_tp_to_col(TileImage *p,void *dstcol)
{
	mRect rc;
	int ix,iy;

	TileImage_getCanDrawRect_pixel(p, &rc);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_isPixel_transparent(p, ix, iy))
				TileImage_setPixel_draw_direct(p, ix, iy, dstcol);
		}
	}
}

/** 色置き換え (不透明 -> 指定色 or 透明) */

void TileImage_replaceColor_col_to_col(TileImage *p,void *srccol,void *dstcol)
{
	int ix,iy,bits;
	mRect rc;
	RGBA8 c8,src8,dst8;
	RGBA16 c16,src16,dst16;

	bits = TILEIMGWORK->bits;

	if(bits == 8)
	{
		src8.v32 = *((uint32_t *)srccol);
		dst8.v32 = *((uint32_t *)dstcol);
	}
	else
	{
		src16.v64 = *((uint64_t *)srccol);
		dst16.v64 = *((uint64_t *)dstcol);
	}

	TileImage_getCanDrawRect_pixel(p, &rc);

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(bits == 8)
			{
				//8bit
				
				TileImage_getPixel(p, ix, iy, &c8);

				if(c8.a && c8.r == src8.r && c8.g == src8.g && c8.b == src8.b)
				{
					if(dst8.a)
						dst8.a = c8.a;

					TileImage_setPixel_draw_direct(p, ix, iy, &dst8);
				}
			}
			else
			{
				//16bit

				TileImage_getPixel(p, ix, iy, &c16);

				if(c16.a && c16.r == src16.r && c16.g == src16.g && c16.b == src16.b)
				{
					if(dst16.a)
						dst16.a = c16.a;

					TileImage_setPixel_draw_direct(p, ix, iy, &dst16);
				}
			}
		}
	}
}


//===============================
// イメージ結合
//===============================


/* イメージ結合: 8bit */

static void _combine_8bit(TileImage *dst,TileImage *src,mRect *rc,int opasrc,int opadst,
	int blendmode,mPopupProgress *prog)
{
	int ix,iy,dst_rawa,ret;
	int32_t csrc[3],cdst[3];
	RGBA8 colsrc,coldst;
	BlendColorFunc func_blend;
	TileImagePixelColorFunc func_pix;

	func_blend = TILEIMGWORK->blendfunc[blendmode];
	func_pix = TILEIMGWORK->pixcolfunc[TILEIMAGE_PIXELCOL_NORMAL];

	for(iy = rc->y1; iy <= rc->y2; iy++)
	{
		for(ix = rc->x1; ix <= rc->x2; ix++)
		{
			//----- src 色
			
			TileImage_getPixel(src, ix, iy, &colsrc);

			colsrc.a = colsrc.a * opasrc >> 7;

			//------ dst

			TileImage_getPixel(dst, ix, iy, &coldst);

			//両方透明なら処理なし
			
			if(colsrc.a == 0 && coldst.a == 0) continue;

			dst_rawa = coldst.a;

			coldst.a = coldst.a * opadst >> 7;

			//色合成 (colsrc にセット)
			// ret = アルファ合成を行うか

			if(blendmode == 0)
				ret = TRUE;
			else
			{
				csrc[0] = colsrc.r;
				csrc[1] = colsrc.g;
				csrc[2] = colsrc.b;
				
				if(coldst.a == 0)
					cdst[0] = cdst[1] = cdst[2] = 255;
				else
				{
					cdst[0] = coldst.r;
					cdst[1] = coldst.g;
					cdst[2] = coldst.b;
				}

				ret = (func_blend)(csrc, cdst, colsrc.a);

				colsrc.r = csrc[0];
				colsrc.g = csrc[1];
				colsrc.b = csrc[2];
			}

			//coldst に結果

			if(ret)
				//アルファ合成
				(func_pix)(dst, &coldst, &colsrc, NULL);
			else
			{
				coldst.r = csrc[0];
				coldst.g = csrc[1];
				coldst.b = csrc[2];
			}

			//色をセット
			// :結合前の dst-A と結合後の dst-A が両方透明なら、何もしない。
			// :opadst の値によっては、結合後の dst-A が透明になる場合があるので、
			// :[結合前=不透明、結合後=透明] なら、色をセットしなければならない。

			if(coldst.a || dst_rawa)
				TileImage_setPixel_new(dst, ix, iy, &coldst);
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/* イメージ結合: 16bit */

static void _combine_16bit(TileImage *dst,TileImage *src,mRect *rc,int opasrc,int opadst,
	int blendmode,mPopupProgress *prog)
{
	int ix,iy,dst_rawa,ret;
	int32_t csrc[3],cdst[3];
	RGBA16 colsrc,coldst;
	BlendColorFunc func_blend;
	TileImagePixelColorFunc func_pix;

	func_blend = TILEIMGWORK->blendfunc[blendmode];
	func_pix = TILEIMGWORK->pixcolfunc[TILEIMAGE_PIXELCOL_NORMAL];

	for(iy = rc->y1; iy <= rc->y2; iy++)
	{
		for(ix = rc->x1; ix <= rc->x2; ix++)
		{
			//----- src 色
			
			TileImage_getPixel(src, ix, iy, &colsrc);

			colsrc.a = colsrc.a * opasrc >> 7;

			//------ dst

			TileImage_getPixel(dst, ix, iy, &coldst);

			//両方透明なら処理なし
			
			if(colsrc.a == 0 && coldst.a == 0) continue;

			dst_rawa = coldst.a;

			coldst.a = coldst.a * opadst >> 7;

			//色合成 (colsrc にセット)
			// ret = アルファ合成を行うか

			if(blendmode == 0)
				ret = TRUE;
			else
			{
				csrc[0] = colsrc.r;
				csrc[1] = colsrc.g;
				csrc[2] = colsrc.b;
				
				if(coldst.a == 0)
					cdst[0] = cdst[1] = cdst[2] = 0x8000;
				else
				{
					cdst[0] = coldst.r;
					cdst[1] = coldst.g;
					cdst[2] = coldst.b;
				}

				ret = (func_blend)(csrc, cdst, colsrc.a);

				colsrc.r = csrc[0];
				colsrc.g = csrc[1];
				colsrc.b = csrc[2];
			}

			//coldst に結果

			if(ret)
				//アルファ合成
				(func_pix)(dst, &coldst, &colsrc, NULL);
			else
			{
				coldst.r = csrc[0];
				coldst.g = csrc[1];
				coldst.b = csrc[2];
			}

			//色をセット

			if(coldst.a || dst_rawa)
				TileImage_setPixel_new(dst, ix, iy, &coldst);
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/** イメージ結合 */

void TileImage_combine(TileImage *dst,TileImage *src,mRect *rc,int opasrc,int opadst,
	int blendmode,mPopupProgress *prog)
{
	//配列リサイズ

	if(!TileImage_resizeTileBuf_combine(dst, src)) return;

	mPopupProgressThreadSubStep_begin_onestep(prog, 20, rc->y2 - rc->y1 + 1);

	//

	if(TILEIMGWORK->bits == 8)
		_combine_8bit(dst, src, rc, opasrc, opadst, blendmode, prog);
	else
		_combine_16bit(dst, src, rc, opasrc, opadst, blendmode, prog);

	//透明タイルを解放

	TileImage_freeEmptyTiles(dst);
}


//===============================
// イメージ全体の編集
//===============================


/** キャンバスサイズ変更で範囲外を切り取る場合: 切り取り後のイメージ作成 */

TileImage *TileImage_createCropImage(TileImage *src,int offx,int offy,int w,int h)
{
	TileImage *dst;
	int ix,iy;
	RGBA8 c8;
	RGBA16 c16;

	//作成

	dst = TileImage_new(src->type, w, h);
	if(!dst) return NULL;

	dst->col = src->col;

	//コピー

	if(TILEIMGWORK->bits == 8)
	{
		//8bit

		for(iy = 0; iy < h; iy++)
		{
			for(ix = 0; ix < w; ix++)
			{
				TileImage_getPixel(src, ix - offx, iy - offy, &c8);

				if(c8.a)
					TileImage_setPixel_new(dst, ix, iy, &c8);
			}
		}
	}
	else
	{
		//16bit
		
		for(iy = 0; iy < h; iy++)
		{
			for(ix = 0; ix < w; ix++)
			{
				TileImage_getPixel(src, ix - offx, iy - offy, &c16);

				if(c16.a)
					TileImage_setPixel_new(dst, ix, iy, &c16);
			}
		}
	}

	return dst;
}

/** イメージ全体を左右反転 */

void TileImage_flipHorz_full(TileImage *p)
{
	uint8_t **pptile,**pp2,*ptmp;
	TileImageColFunc_editTile func;
	int ix,iy,cnt;

	ptmp = TileImage_allocTile(p);
	if(!ptmp) return;

	//offset

	p->offx = TILEIMGWORK->imgw - p->offx - p->tilew * 64;

	//各タイルのイメージを左右反転

	func = TILEIMGWORK->colfunc[p->type].flip_horz;

	pptile = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pptile++)
	{
		if(*pptile)
		{
			TileImage_copyTile(p, ptmp, *pptile);
		
			(func)(*pptile, ptmp);
		}
	}

	mFree(ptmp);

	//タイル配列のポインタを左右反転

	cnt = p->tilew / 2;
	pptile = p->ppbuf;
	pp2 = pptile + p->tilew - 1;

	for(iy = p->tileh; iy; iy--)
	{
		for(ix = cnt; ix; ix--, pptile++, pp2--)
		{
			ptmp = *pptile;
			*pptile = *pp2;
			*pp2 = ptmp;
		}

		pptile += p->tilew - cnt;
		pp2 += p->tilew + cnt;
	}
}

/** イメージ全体を上下反転 */

void TileImage_flipVert_full(TileImage *p)
{
	uint8_t **pp,**pp2,*ptmp;
	TileImageColFunc_editTile func;
	int ix,iy,ww;

	ptmp = TileImage_allocTile(p);
	if(!ptmp) return;

	//offset

	p->offy = TILEIMGWORK->imgh - p->offy - p->tileh * 64;

	//各タイルを上下反転

	func = TILEIMGWORK->colfunc[p->type].flip_vert;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
		{
			TileImage_copyTile(p, ptmp, *pp);
		
			(func)(*pp, ptmp);
		}
	}

	mFree(ptmp);

	//タイル配列を上下反転

	ww = p->tilew * 2;
	pp  = p->ppbuf;
	pp2 = pp + (p->tileh - 1) * p->tilew;

	for(iy = p->tileh / 2; iy; iy--)
	{
		for(ix = p->tilew; ix; ix--, pp++, pp2++)
		{
			ptmp = *pp;
			*pp  = *pp2;
			*pp2 = ptmp;
		}

		pp2 -= ww;
	}
}

/** イメージ全体を左に90度回転
 *
 * 元の (imgw - 1,0) が、回転後の (0,0) となる */

void TileImage_rotateLeft_full(TileImage *p)
{
	uint8_t **ppnew,**pp,**pp2,*tilebuf;
	TileImageColFunc_editTile func;
	int ix,iy,n;

	tilebuf = TileImage_allocTile(p);
	if(!tilebuf) return;

	//新規配列

	ppnew = __TileImage_allocTileBuf_new(p->tileh, p->tilew);
	if(!ppnew)
	{
		mFree(tilebuf);
		return;
	}

	//オフセット

	ix = p->offx;
	iy = p->offy;
	
	p->offx = iy;
	p->offy = TILEIMGWORK->imgw - p->tilew * 64 - ix;

	//各タイルを回転

	func = TILEIMGWORK->colfunc[p->type].rotate_left;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
		{
			TileImage_copyTile(p, tilebuf, *pp);
		
			(func)(*pp, tilebuf);
		}
	}

	mFree(tilebuf);

	//配列を回転して配置

	pp = ppnew;
	pp2 = p->ppbuf + p->tilew - 1;
	n = p->tilew * p->tileh + 1;

	for(iy = p->tilew; iy; iy--, pp2 -= n)
	{
		for(ix = p->tileh; ix; ix--, pp2 += p->tilew)
			*(pp++) = *pp2;
	}

	//置き換え

	mFree(p->ppbuf);

	n = p->tilew;

	p->ppbuf = ppnew;
	p->tilew = p->tileh;
	p->tileh = n;
}

/** イメージ全体を右に90度回転
 *
 * 元の (0, imgh - 1) が、回転後の (0,0) となる */

void TileImage_rotateRight_full(TileImage *p)
{
	uint8_t **ppnew,**pp,**pp2,*tilebuf;
	int ix,iy,n;
	TileImageColFunc_editTile func;

	tilebuf = TileImage_allocTile(p);
	if(!tilebuf) return;

	//新規配列

	ppnew = __TileImage_allocTileBuf_new(p->tileh, p->tilew);
	if(!ppnew)
	{
		mFree(tilebuf);
		return;
	}

	//オフセット

	ix = p->offx;
	iy = p->offy;
	
	p->offx = TILEIMGWORK->imgh - p->tileh * 64 - iy;
	p->offy = ix;

	//各タイルを回転

	func = TILEIMGWORK->colfunc[p->type].rotate_right;

	pp = p->ppbuf;

	for(ix = p->tilew * p->tileh; ix; ix--, pp++)
	{
		if(*pp)
		{
			TileImage_copyTile(p, tilebuf, *pp);
		
			(func)(*pp, tilebuf);
		}
	}

	mFree(tilebuf);

	//配列を回転して配置

	pp = ppnew;
	pp2 = p->ppbuf + (p->tileh - 1) * p->tilew;
	n = p->tilew * p->tileh + 1;

	for(iy = p->tilew; iy; iy--, pp2 += n)
	{
		for(ix = p->tileh; ix; ix--, pp2 -= p->tilew)
			*(pp++) = *pp2;
	}

	//置き換え

	mFree(p->ppbuf);

	n = p->tilew;

	p->ppbuf = ppnew;
	p->tilew = p->tileh;
	p->tileh = n;
}


//===========================
// 矩形編集
//===========================


/** 矩形範囲の左右反転 */

void TileImage_flipHorz_box(TileImage *p,const mBox *box)
{
	int x1,x2,ix,iy,y,cnt;
	uint64_t col1,col2;

	x1 = box->x;
	x2 = box->x + box->w - 1;
	cnt = box->w >> 1;

	for(iy = box->h, y = box->y; iy > 0; iy--, y++)
	{
		for(ix = 0; ix < cnt; ix++)
		{
			//両端の色を入れ替えてセット

			TileImage_getPixel(p, x1 + ix, y, &col1);
			TileImage_getPixel(p, x2 - ix, y, &col2);

			TileImage_setPixel_draw_direct(p, x1 + ix, y, &col2);
			TileImage_setPixel_draw_direct(p, x2 - ix, y, &col1);
		}
	}
}

/** 矩形範囲の上下反転 */

void TileImage_flipVert_box(TileImage *p,const mBox *box)
{
	int y1,y2,ix,iy,x;
	uint64_t col1,col2;

	y1 = box->y;
	y2 = box->y + box->h - 1;

	for(iy = box->h >> 1; iy > 0; iy--, y1++, y2--)
	{
		for(ix = box->w, x = box->x; ix > 0; ix--, x++)
		{
			TileImage_getPixel(p, x, y1, &col1);
			TileImage_getPixel(p, x, y2, &col2);

			TileImage_setPixel_draw_direct(p, x, y1, &col2);
			TileImage_setPixel_draw_direct(p, x, y2, &col1);
		}
	}
}

/** 範囲を左に90度回転 (左上が原点) */

void TileImage_rotateLeft_box(TileImage *p,TileImage *src,const mBox *box)
{
	mRect rc;
	int sx,sy,ix,iy;
	uint64_t col;

	if(!src) return;

	rc.x1 = box->x, rc.y1 = box->y;
	rc.x2 = box->x + box->h - 1;
	rc.y2 = box->y + box->w - 1;

	sx = box->w - 1;

	for(iy = rc.y1; iy <= rc.y2; iy++, sx--)
	{
		for(ix = rc.x1, sy = 0; ix <= rc.x2; ix++, sy++)
		{
			TileImage_getPixel(src, sx, sy, &col);
			TileImage_setPixel_draw_direct(p, ix, iy, &col);
		}
	}
}

/** 範囲を右に90度回転 */

void TileImage_rotateRight_box(TileImage *p,TileImage *src,const mBox *box)
{
	mRect rc;
	int sx,sy,ix,iy;
	uint64_t col;

	if(!src) return;

	rc.x1 = box->x, rc.y1 = box->y;
	rc.x2 = box->x + box->h - 1;
	rc.y2 = box->y + box->w - 1;

	sx = 0;

	for(iy = rc.y1; iy <= rc.y2; iy++, sx++)
	{
		sy = box->h - 1;
		
		for(ix = rc.x1; ix <= rc.x2; ix++, sy--)
		{
			TileImage_getPixel(src, sx, sy, &col);
			TileImage_setPixel_draw_direct(p, ix, iy, &col);
		}
	}
}

/** 拡大(補間なし)
 *
 * zoom: 2 で2倍 */

void TileImage_scaleup_nearest(TileImage *p,TileImage *src,const mBox *box,int zoom)
{
	mRect rc;
	int ix,iy,sy,isx,isy;
	uint64_t col;

	if(!src) return;

	rc.x1 = box->x, rc.y1 = box->y;
	rc.x2 = box->x + box->w * zoom - 1;
	rc.y2 = box->y + box->h * zoom - 1;

	if(rc.x2 > TILEIMGWORK->imgw) rc.x2 = TILEIMGWORK->imgw - 1;
	if(rc.y2 > TILEIMGWORK->imgh) rc.y2 = TILEIMGWORK->imgh - 1;

	//

	for(iy = rc.y1, isy = 0; iy <= rc.y2; iy++, isy++)
	{
		sy = isy / zoom;
	
		for(ix = rc.x1, isx = 0; ix <= rc.x2; ix++, isx++)
		{
			TileImage_getPixel(src, isx / zoom, sy, &col);
			
			TileImage_setPixel_draw_direct(p, ix, iy, &col);
		}
	}
}

/** タイル状に並べる (全体) */

void TileImage_putTiles_full(TileImage *p,TileImage *src,const mBox *box)
{
	int ix,iy,imgw,imgh,sxleft,sx,sy,boxw,boxh;
	uint64_t col;

	imgw = TILEIMGWORK->imgw;
	imgh = TILEIMGWORK->imgh;

	boxw = box->w;
	boxh = box->h;

	sxleft = box->x % boxw;
	if(sxleft) sxleft = boxw - sxleft;

	sy = box->y % boxh;
	if(sy) sy = boxh - sy;

	//

	for(iy = 0; iy < imgh; iy++)
	{
		for(ix = 0, sx = sxleft; ix < imgw; ix++)
		{
			TileImage_getPixel(src, sx, sy, &col);
			
			TileImage_setPixel_draw_direct(p, ix, iy, &col);

			sx++;
			if(sx == boxw) sx = 0;
		}

		sy++;
		if(sy == boxh) sy = 0;
	}
}

/** タイル状に並べる (横一列) */

void TileImage_putTiles_horz(TileImage *p,TileImage *src,const mBox *box)
{
	int ix,iy,imgw,bottom,sx,sy,boxw;
	uint64_t col;

	imgw = TILEIMGWORK->imgw;
	bottom = box->y + box->h - 1;

	boxw = box->w;

	sx = box->x % boxw;
	if(sx) sx = boxw - sx;

	//

	for(ix = 0; ix < imgw; ix++)
	{
		for(iy = box->y, sy = 0; iy <= bottom; iy++, sy++)
		{
			TileImage_getPixel(src, sx, sy, &col);
			
			TileImage_setPixel_draw_direct(p, ix, iy, &col);
		}

		sx++;
		if(sx == boxw) sx = 0;
	}
}

/** タイル状に並べる (縦一列) */

void TileImage_putTiles_vert(TileImage *p,TileImage *src,const mBox *box)
{
	int ix,iy,right,imgh,sx,sy,boxh;
	uint64_t col;

	imgh = TILEIMGWORK->imgh;
	right = box->x + box->w - 1;

	boxh = box->h;

	sy = box->y % boxh;
	if(sy) sy = boxh - sy;

	//

	for(iy = 0; iy < imgh; iy++)
	{
		for(ix = box->x, sx = 0; ix <= right; ix++, sx++)
		{
			TileImage_getPixel(src, sx, sy, &col);
			
			TileImage_setPixel_draw_direct(p, ix, iy, &col);
		}

		sy++;
		if(sy == boxh) sy = 0;
	}
}


//=========================
// 変形用
//=========================


/** 矩形内のイメージをコピー & 範囲内消去
 *
 * ダイアログ前に行われる。
 * 消去するのは、合成イメージで元のイメージを非表示にさせるため。
 * 選択範囲の外は透明にする。
 * 
 * sel: NULL の場合あり */

TileImage *TileImage_createCopyImage_forTransform(TileImage *src,TileImage *sel,const mBox *box)
{
	TileImage *dst;
	int w,h,ix,iy,sx,sy;
	uint64_t col,colerase = 0;

	w = box->w;
	h = box->h;

	//作成

	dst = TileImage_new(src->type, w, h);
	if(!dst) return NULL;

	dst->col = src->col;

	//

	for(iy = 0, sy = box->y; iy < h; iy++, sy++)
	{
		for(ix = 0, sx = box->x; ix < w; ix++, sx++)
		{
			if(!sel || TileImage_isPixel_opaque(sel, sx, sy))
			{
				TileImage_getPixel(src, sx, sy, &col);

				if(!(TILEIMGWORK->is_transparent)(&col))
				{
					//コピー
					
					TileImage_setPixel_new(dst, ix, iy, &col);

					//消去

					TileImage_setPixel_new(src, sx, sy, &colerase);
				}
			}
		}
	}

	return dst;
}

/** 最初に消去した範囲を元に戻す
 *
 * 変形ダイアログの終了時。
 * src の (0,0)-(w x h) の範囲を、dst の (x,y)-(w x h) に戻す。 */

void TileImage_restoreCopyImage_forTransform(TileImage *dst,TileImage *src,const mBox *box)
{
	int ix,iy;
	mBox boxd;
	uint64_t col;

	boxd = *box;

	for(iy = 0; iy < boxd.h; iy++)
	{
		for(ix = 0; ix < boxd.w; ix++)
		{
			TileImage_getPixel(src, ix, iy, &col);

			if(!(TILEIMGWORK->is_transparent)(&col))
				TileImage_setPixel_new(dst, boxd.x + ix, boxd.y + iy, &col);
		}
	}
}

/** 変形の描画前に元の範囲内を消去
 *
 * 範囲内でかつ選択範囲の内側のみ */

void TileImage_clearBox_forTransform(TileImage *p,TileImage *sel,const mBox *box)
{
	int ix,iy;
	mRect rc;
	uint64_t col = 0;

	mRectSetBox(&rc, box);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(!sel || TileImage_isPixel_opaque(sel, ix, iy))
				TileImage_setPixel_draw_direct(p, ix, iy, &col);
		}
	}
}

/* ソース座標から mPixbuf に点を描画 (プレビュー用) */

static void _transform_setpixel_pixbuf(mPixbuf *pixbuf,uint8_t *dstbuf,
	TileImage *p,double dx,double dy,int srcw,int srch)
{
	int nx,ny;
	uint32_t pixcol;
	uint64_t col;
	uint8_t *pcol8;
	uint16_t *pcol16;

	if(dx < 0 || dy < 0) return;

	nx = (int)dx;
	ny = (int)dy;

	if(nx >= srcw || ny >= srch) return;

	//ソースの色

	TileImage_getPixel(p, nx, ny, &col);

	//キャンバスに合成 (A=0-128)

	if(TILEIMGWORK->bits == 8)
	{
		pcol8 = (uint8_t *)&col;

		if(!pcol8[3]) return;
		
		pixcol = (pcol8[0] << 16) | (pcol8[1] << 8) | pcol8[2];
		pixcol |= (uint32_t)(pcol8[3] * 128 / 255) << 24;
	}
	else
	{
		pcol16 = (uint16_t *)&col;

		if(!pcol16[3]) return;
	
		pixcol = (pcol16[0] * 255 >> 15) << 16;
		pixcol |= (pcol16[1] * 255 >> 15) << 8;
		pixcol |= pcol16[2] * 255 >> 15;
		pixcol |= pcol16[3] >> (15 - 7) << 24;
	}

	mPixbufBlendPixel_a128_buf(pixbuf, dstbuf, pixcol);
}


//=============================
// 変形: アフィン変換
//=============================


/* アフィン変換の描画先範囲取得 */

static void _get_affine_drawrect(mRect *rcdst,const mBox *box,
	double scalex,double scaley,double dcos,double dsin,double movx,double movy)
{
	double cx,cy,xx,yy;
	mDoublePoint pt[4];
	mPoint ptmin,ptmax;
	int i,x,y;

	cx = box->w * 0.5;
	cy = box->h * 0.5;

	//4隅の座標 (ソース座標)

	pt[0].x = pt[3].x = -cx;
	pt[0].y = pt[1].y = -cy;
	pt[1].x = pt[2].x = -cx + box->w;
	pt[2].y = pt[3].y = -cy + box->h;

	//4隅の点を変形して、最小/最大値取得

	for(i = 0; i < 4; i++)
	{
		xx = pt[i].x * scalex;
		yy = pt[i].y * scaley;
	
		x = (int)(xx * dcos - yy * dsin + cx);
		y = (int)(xx * dsin + yy * dcos + cy);

		if(i == 0)
		{
			ptmin.x = ptmax.x = x;
			ptmin.y = ptmax.y = y;
		}
		else
		{
			if(x < ptmin.x) ptmin.x = x;
			if(y < ptmin.y) ptmin.y = y;
			if(x > ptmax.x) ptmax.x = x;
			if(y > ptmax.y) ptmax.y = y;
		}
	}

	//セット (少し拡張)

	rcdst->x1 = box->x + ptmin.x + movx - 2;
	rcdst->y1 = box->y + ptmin.y + movy - 2;
	rcdst->x2 = box->x + ptmax.x + movx + 2;
	rcdst->y2 = box->y + ptmax.y + movy + 2;
}

/** アフィン変換
 *
 * src: 元イメージ (0,0)-(w x h) */

void TileImage_transformAffine(TileImage *dst,TileImage *src,const mBox *box,
	double scalex,double scaley,double dcos,double dsin,double movx,double movy,
	mPopupProgress *prog)
{
	mRect rc;
	int ix,iy,sw,sh;
	double cx,cy,dx,dy,dxx,dyy,scalex_div,scaley_div,addxx,addxy,addyx,addyy;
	uint64_t col;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	//全体の描画先範囲

	_get_affine_drawrect(&rc, box, scalex, scaley, dcos, dsin, movx, movy);

	//描画可能範囲でクリッピング

	if(!TileImage_clipCanDrawRect(dst, &rc)) return;

	//

	sw = box->w, sh = box->h;

	cx = sw * 0.5;
	cy = sh * 0.5;

	scalex_div = 1 / scalex;
	scaley_div = 1 / scaley;

	//描画先左上におけるソース座標

	dx = rc.x1 - box->x - movx - cx;
	dy = rc.y1 - box->y - movy - cy;

	dxx = (dx *  dcos + dy * dsin) * scalex_div + cx;
	dyy = (dx * -dsin + dy * dcos) * scaley_div + cy;

	//

	addxx = dcos * scalex_div;
	addxy = -dsin * scaley_div;
	addyx = dsin * scalex_div;
	addyy = dcos * scaley_div;

	//

	mPopupProgressThreadSubStep_begin_onestep(prog, 50, rc.y2 - rc.y1 + 1);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dx = dxx;
		dy = dyy;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, dx += addxx, dy += addxy)
		{
			if(TileImage_getColor_bicubic(src, dx, dy, sw, sh, &col))
				(setpix)(dst, ix, iy, &col);
		}

		dxx += addyx;
		dyy += addyy;

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/** アフィン変換 + mPixbuf に合成 (プレビュー用)
 *
 * p: ソース画像
 * boxsrc: ソース画像の範囲
 * scalex,scaley: 拡大率(逆)
 * dcos,dsin: 逆回転の値
 * ptmov: スクロール位置 */

void TileImage_transformAffine_preview(TileImage *p,mPixbuf *pixbuf,const mBox *boxsrc,
	double scalex,double scaley,double dcos,double dsin,
	mPoint *ptmov,CanvasDrawInfo *canvinfo)
{
	int ix,iy,sw,sh,pitchd,bpp;
	double cx,cy,scalec,dx,dy,dx_xx,dx_yx,dy_xy,dy_yy,dinc_cos,dinc_sin,dxx,dyy;
	mRect rc;
	mBox box;
	uint8_t *pd;

	//クリッピング
	// :box, rc = mPixbuf の描画範囲

	if(!mPixbufClip_getBox(pixbuf, &box, &canvinfo->boxdst)) return;

	mRectSetBox(&rc, &box);

	//

	sw = boxsrc->w;
	sh = boxsrc->h;
	cx = sw * 0.5;
	cy = sh * 0.5;

	scalec = canvinfo->param->scalediv;

	//描画先左上のソース座標

	dx = (rc.x1 + canvinfo->scrollx) * scalec + canvinfo->originx - boxsrc->x - cx - ptmov->x;
	dy = (rc.y1 + canvinfo->scrolly) * scalec + canvinfo->originy - boxsrc->y - cy - ptmov->y;

	//逆アフィン変換
	// (sx *  dcos + sy * dsin) * scalex
	// (sx * -dsin + sy * dcos) * scaley

	dx_xx = dx * dcos;
	dx_yx = dx * -dsin;
	dy_xy = dy * dsin;
	dy_yy = dy * dcos;

	dinc_cos = scalec * dcos;
	dinc_sin = scalec * dsin;

	//pixbuf

	pd = mPixbufGetBufPtFast(pixbuf, rc.x1, rc.y1);
	bpp = pixbuf->pixel_bytes;
	pitchd = pixbuf->line_bytes - box.w * bpp;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dxx = dx_xx;
		dyy = dx_yx;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += bpp)
		{
			dx = (dxx + dy_xy) * scalex + cx;
			dy = (dyy + dy_yy) * scaley + cy;

			_transform_setpixel_pixbuf(pixbuf, pd, p, dx, dy, sw, sh);

			dxx += dinc_cos;
			dyy -= dinc_sin;
		}

		pd += pitchd;
		dy_xy += dinc_sin;
		dy_yy += dinc_cos;
	}
}


//=============================
// 変形: 射影変換
//=============================


/** 射影変換
 *
 * param: [0..8] 射影変換用パラメータ [9,10] 平行移動 */

void TileImage_transformHomography(TileImage *dst,TileImage *src,const mRect *rcdst,
	double *param,int sx,int sy,int sw,int sh,mPopupProgress *prog)
{
	mRect rc;
	int ix,iy;
	double dp_y,dx_y,dy_y,dx,dy,dp;
	uint64_t col;
	TileImageSetPixelFunc setpix = g_tileimage_dinfo.func_setpixel;

	//描画可能範囲にクリッピング

	rc = *rcdst;

	if(!TileImage_clipCanDrawRect(dst, &rc)) return;

	//描画先左上のパラメータ

	dx = rc.x1 - param[9];
	dy = rc.y1 - param[10];

	dp_y = dx * param[6] + dy * param[7] + param[8];
	dx_y = dx * param[0] + dy * param[1] + param[2];
	dy_y = dx * param[3] + dy * param[4] + param[5];

	//

	mPopupProgressThreadSubStep_begin_onestep(prog, 50, rc.y2 - rc.y1 + 1);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dp = dp_y;
		dx = dx_y;
		dy = dy_y;
	
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			if(TileImage_getColor_bicubic(src, dx / dp - sx, dy / dp - sy, sw, sh, &col))
				(setpix)(dst, ix, iy, &col);

			dp += param[6];
			dx += param[0];
			dy += param[3];
		}

		dp_y += param[7];
		dx_y += param[1];
		dy_y += param[4];

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/** 射影変換 + mPixbuf に合成 (プレビュー用)
 *
 * boxdst: 描画先範囲 */

void TileImage_transformHomography_preview(TileImage *p,mPixbuf *pixbuf,const mBox *boxdst,
	double *param,mPoint *ptmov,int sx,int sy,int sw,int sh,CanvasDrawInfo *canvinfo)
{
	int ix,iy,pitchd,bpp;
	double scalec,dx,dy,dp,dp_y,dx_y,dy_y,addxp,addxx,addxy,addyp,addyx,addyy;
	mRect rc;
	mBox box;
	uint8_t *pd;

	//クリッピング

	if(!mPixbufClip_getBox(pixbuf, &box, boxdst)) return;

	mRectSetBox(&rc, &box);

	//

	scalec = canvinfo->param->scalediv;

	//描画先左上の状態

	dx = (rc.x1 + canvinfo->scrollx) * scalec + canvinfo->originx - ptmov->x;
	dy = (rc.y1 + canvinfo->scrolly) * scalec + canvinfo->originy - ptmov->y;

	dp_y = dx * param[6] + dy * param[7] + param[8];
	dx_y = dx * param[0] + dy * param[1] + param[2];
	dy_y = dx * param[3] + dy * param[4] + param[5];

	//加算値

	addxp = scalec * param[6];
	addxx = scalec * param[0];
	addxy = scalec * param[3];

	addyp = scalec * param[7];
	addyx = scalec * param[1];
	addyy = scalec * param[4];

	//pixbuf

	pd = mPixbufGetBufPtFast(pixbuf, rc.x1, rc.y1);
	bpp = pixbuf->pixel_bytes;
	pitchd = pixbuf->line_bytes - box.w * bpp;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		dp = dp_y;
		dx = dx_y;
		dy = dy_y;

		//

		for(ix = rc.x1; ix <= rc.x2; ix++, pd += bpp)
		{
			//逆変換
			// d = x * param[6] + y * param[7] + param[8]
			// sx = (x * param[0] + y * param[1] + param[2]) / d
			// sy = (x * param[3] + y * param[4] + param[5]) / d

			_transform_setpixel_pixbuf(pixbuf, pd, p,
				dx / dp - sx, dy / dp - sy, sw, sh);

			dp += addxp;
			dx += addxx;
			dy += addxy;
		}

		pd += pitchd;

		dp_y += addyp;
		dx_y += addyx;
		dy_y += addyy;
	}
}

