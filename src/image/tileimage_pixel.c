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
 * TileImage: ピクセル関連
 *****************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rectbox.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "pv_tileimage.h"

#include "def_brushdraw.h"

#include "imagematerial.h"
#include "dotshape.h"


//-------------------

typedef struct
{
	uint8_t **pptile;	//描画先のタイルバッファポインタ (NULL でタイル範囲外 [!]描画対象ではある)
	int tx,ty,			//タイル位置
		max_alpha;		//マスクレイヤによるアルファ値制限
	uint64_t coldst;	//描画先の色
}_setpixelinfo;

//-------------------



//================================
// sub
//================================


/* 色マスク判定
 *
 * return: TRUE で保護 */

static int _setpixeldraw_colormask(int type,void *col,int bits)
{
	RGBA8 *pmask,c8;
	RGBA16 c16;
	int f,i;

	//描画先が透明の場合、[マスク] 描画 [逆マスク] 保護

	if(bits == 8)
	{
		c8 = *((RGBA8 *)col);

		if(c8.a == 0) return (type == 2);
	}
	else
	{
		//16bit: 8bit 色で比較する
		
		c16 = *((RGBA16 *)col);
		
		if(c16.a == 0) return (type == 2);

		RGBA16_to_RGBA8(&c8, &c16);
	}

	//マスク色の中に描画先の色があるか

	pmask = g_tileimage_dinfo.pmaskcol;

	for(i = g_tileimage_dinfo.maskcol_num, f = 0; i; i--, pmask++)
	{
		if(pmask->a && pmask->r == c8.r && pmask->g == c8.g && pmask->b == c8.b)
		{
			f = 1;
			break;
		}
	}

	//逆マスクの場合は判定を逆にする
	
	if(type == 2) f = !f;

	return f;
}

/* 描画前の元イメージの色を取得
 *
 * _setpixeldraw_dstpixel() 後に使用可能 */

static void _setpixeldraw_get_save_color(TileImage *p,int x,int y,
	_setpixelinfo *info,void *dstcol)
{
	uint8_t **pptile,*buf;

	(TILEIMGWORK->setcol_rgba_transparent)(dstcol);
	
	pptile = info->pptile;

	//描画先と保存イメージのタイル配列構成は同じなので、
	//描画先がタイル範囲外なら、保存イメージでも範囲外

	if(pptile)
	{
		//保存イメージのタイル
		
		buf = TILEIMAGE_GETTILE_PT(g_tileimage_dinfo.img_save, info->tx, info->ty);

		if(!buf)
		{
			//保存イメージのタイルが未確保なら、まだ描画による変更がないので、描画先から取得

			if(*pptile)
				(TILEIMGWORK->colfunc[p->type].getpixel_at_tile)(p, *pptile, x, y, dstcol);
		}
		else if(buf != TILEIMAGE_TILE_EMPTY)
		{
			//タイルがある場合

			(TILEIMGWORK->colfunc[p->type].getpixel_at_tile)(g_tileimage_dinfo.img_save, buf, x, y, dstcol);
		}
	}
}

/* 描画先の情報取得 + マスク処理
 *
 * coldraw: 描画色。テクスチャが適用されて返る。
 * return: 0 以外で描画しない */

static int _setpixeldraw_dstpixel(TileImage *p,int x,int y,void *coldraw,
	_setpixelinfo *dst)
{
	TileImageDrawInfo *dinfo = &g_tileimage_dinfo;
	uint8_t **pptile;
	int tx,ty,maska,bits,n;
	uint64_t col;

	//エラーが起こった時は、それ以降処理しない

	if(dinfo->err) return 1;

	//選択範囲 (点がない場合は描画しない)

	if(dinfo->img_sel)
	{
		if(TileImage_isPixel_transparent(dinfo->img_sel, x, y))
			return 1;
	}

	//

	bits = TILEIMGWORK->bits;
	
	//テクスチャ適用
	//[!] 色を上書きする場合は、透明時も色をセットしなければならないため、
	//    テクスチャの値が 0 でも処理を続ける。

	if(dinfo->texture)
	{
		if(bits == 8)
		{
			*((uint8_t *)coldraw + 3) = (*((uint8_t *)coldraw + 3)
				* ImageMaterial_getPixel_forTexture(dinfo->texture, x, y) + 127) / 255;
		}
		else
		{
			*((uint16_t *)coldraw + 3) = (*((uint16_t *)coldraw + 3)
				* ImageMaterial_getPixel_forTexture(dinfo->texture, x, y) + 127) / 255;
		}
	}

	//レイヤマスク値取得

	if(!dinfo->img_mask)
		//マスクイメージなし
		maska = (bits == 8)? 255: 0x8000;
	else
	{
		TileImage_getPixel(dinfo->img_mask, x, y, &col);

		if(bits == 8)
			maska = *((uint8_t *)&col + 3);
		else
			maska = *((uint16_t *)&col + 3);

		//A=0 の場合、変化なし

		if(maska == 0) return 1;
	}

	//描画先の色とタイル位置取得
	// col = 描画先の色

	(TILEIMGWORK->setcol_rgba_transparent)(&col);

	if(TileImage_pixel_to_tile(p, x, y, &tx, &ty))
	{
		//タイル配列範囲内
		
		pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		if(*pptile)
			(TILEIMGWORK->colfunc[p->type].getpixel_at_tile)(p, *pptile, x, y, &col);

		dst->pptile = pptile;
		dst->tx = tx;
		dst->ty = ty;
	}
	else
	{
		//タイル配列範囲外

		dst->pptile = NULL;

		//キャンバス範囲外なら処理しない

		if(x < 0 || y < 0 || x >= TILEIMGWORK->imgw || y >= TILEIMGWORK->imgh)
			return 1;
	}

	dst->coldst = col;

	//描画前状態の色によるマスク

	if(dinfo->alphamask_type >= 2 || dinfo->maskcol_type)
	{
		//描画前の色取得
	
		_setpixeldraw_get_save_color(p, x, y, dst, &col);

		//アルファマスク

		if(dinfo->alphamask_type)
		{
			if(bits == 8)
				n = *((uint8_t *)&col + 3);
			else
				n = *((uint16_t *)&col + 3);
		
			if(dinfo->alphamask_type == 2 && n == 0)
				//透明色保護
				return 1;
			else if(dinfo->alphamask_type == 3 && n)
				//不透明色保護
				return 1;
		}

		//色マスク判定

		if(dinfo->maskcol_type
			&& _setpixeldraw_colormask(dinfo->maskcol_type, &col, bits))
			return 1;
	}

	//

	dst->max_alpha = maska;

	return 0;
}

/* 描画前のタイル関連処理
 *
 * [タイル作成][タイル配列リサイズ][アンドゥ用イメージコピー]
 * 
 * これが終了した時点で、エラー時を除き、info->pptile には必ず NULL 以外が入る。
 * 
 * return: FALSE で確保エラー */

static mlkbool _setpixeldraw_tile(TileImage *p,int x,int y,_setpixelinfo *info)
{
	TileImage *img_save;
	uint8_t **pptile,**pptmp;
	int tx,ty;
	mlkbool is_empty_tile = FALSE;

	pptile = info->pptile;
	tx = info->tx;
	ty = info->ty;

	img_save = g_tileimage_dinfo.img_save;

	//タイル配列が範囲外の場合、リサイズ

	if(!pptile)
	{
		//描画先のタイル配列リサイズ

		if(!TileImage_resizeTileBuf_includeCanvas(p)) return FALSE;

		//描画先のタイル位置を再取得

		TileImage_pixel_to_tile_nojudge(p, x, y, &tx, &ty);

		pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		//ブラシ濃度用イメージのリサイズ

		if(!__TileImage_resizeTileBuf_clone(img_save, p))
			return FALSE;

		if(g_tileimage_dinfo.img_brush_stroke
			&& !__TileImage_resizeTileBuf_clone(g_tileimage_dinfo.img_brush_stroke, p))
			return FALSE;
	}

	//描画先にタイルがない場合、確保

	if(!(*pptile))
	{
		*pptile = TileImage_allocTile_clear(p);
		if(!(*pptile)) return FALSE;

		is_empty_tile = TRUE;  //描画前は空のタイル
	}

	//imgsave に元イメージをコピー

	pptmp = TILEIMAGE_GETTILE_BUFPT(img_save, tx, ty);

	if(!(*pptmp))
	{
		if(is_empty_tile)
			//元が空の場合、1 をセット
			*pptmp = TILEIMAGE_TILE_EMPTY;
		else
		{
			//確保 & コピー

			*pptmp = TileImage_allocTile(img_save);
			if(!(*pptmp)) return FALSE;
		
			TileImage_copyTile(img_save, *pptmp, *pptile);
		}
	}

	//描画先タイル情報

	info->pptile = pptile;
	info->tx = tx;
	info->ty = ty;

	return TRUE;
}

/* 結果の色をセット
 *
 * return: [0] OK [1] 色が変化しない [-1] エラー(タイル配列やタイルの確保失敗) */

static int _setpixeldraw_setcolor(TileImage *p,int x,int y,
	void *colres,_setpixelinfo *info)
{
	uint8_t *buf,colbuf[8];
	int n;

	//アルファ値調整

	if(TILEIMGWORK->bits == 8)
	{
		*((uint32_t *)colbuf) = *((uint32_t *)colres);
	
		n = colbuf[3];

		if(g_tileimage_dinfo.alphamask_type == 1)
			//アルファマスク (アルファ値維持)
			n = *((uint8_t *)&info->coldst + 3);
		else if(n > info->max_alpha)
			//レイヤマスク適用
			n = info->max_alpha;

		colbuf[3] = n;
	}
	else
	{
		*((uint64_t *)colbuf) = *((uint64_t *)colres);
	
		n = *((uint16_t *)colbuf + 3);

		if(g_tileimage_dinfo.alphamask_type == 1)
			//アルファマスク (アルファ値維持)
			n = *((uint16_t *)&info->coldst + 3);
		else if(n > info->max_alpha)
			//レイヤマスク適用
			n = info->max_alpha;

		*((uint16_t *)colbuf + 3) = n;
	}

	//描画前と描画後が同じ色になるなら、セットしない

	if((TILEIMGWORK->colfunc[p->type].is_same_rgba)(colbuf, &info->coldst))
		return 1;

	//タイル処理

	if(!_setpixeldraw_tile(p, x, y, info))
	{
		g_tileimage_dinfo.err = MLKERR_ALLOC;
		return -1;
	}

	//描画先に色をセット

	buf = (TILEIMGWORK->colfunc[p->type].getpixelbuf_at_tile)(p, *(info->pptile), x, y);

	(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, colbuf);

	//描画範囲に追加

	mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);

	return 0;
}


//================================
// ドットペン
//================================


/* ドットペン形状を使って描画 (通常) */

static void _setpixeldraw_dotpen(TileImage *p,int x,int y,void *drawcol,
	TileImageSetPixelFunc setpix)
{
	uint8_t *ps,f,val;
	int size,ix,iy,dx,dy;

	size = DotShape_getData(&ps);

	dy = y - (size >> 1);
	f = 0x80;

	for(iy = size; iy > 0; iy--, dy++)
	{
		dx = x - (size >> 1);
	
		for(ix = size; ix > 0; ix--, dx++)
		{
			if(f == 0x80) val = *(ps++);

			if(val & f)
				(setpix)(p, dx, dy, drawcol);

			f >>= 1;
			if(!f) f = 0x80;
		}
	}
}

/* ドットペン形状を使って描画 (矩形上書き) */

static void _setpixeldraw_dotpen_overwrite(TileImage *p,int x,int y,void *drawcol,
	TileImageSetPixelFunc setpix)
{
	uint8_t *ps,f,val;
	int size,ix,iy,dx,dy,bits,srca,a;
	RGBAcombo col;

	bits = TILEIMGWORK->bits;

	bitcol_to_RGBAcombo(&col, drawcol, bits);
	
	srca = (bits == 8)? col.c8.a: col.c16.a;

	size = DotShape_getData(&ps);

	dy = y - (size >> 1);
	f = 0x80;

	for(iy = size; iy > 0; iy--, dy++)
	{
		dx = x - (size >> 1);
	
		for(ix = size; ix > 0; ix--, dx++)
		{
			if(f == 0x80) val = *(ps++);

			a = (val & f)? srca: 0;

			if(bits == 8)
			{
				col.c8.a = a;
				(setpix)(p, dx, dy, &col.c8);
			}
			else
			{
				col.c16.a = a;
				(setpix)(p, dx, dy, &col.c16);
			}

			f >>= 1;
			if(!f) f = 0x80;
		}
	}
}

/* パラメータを指定して直接描画 */

static void _setpixel_with_param(TileImage *p,int x,int y,void *colbuf,void *param)
{
	uint64_t colres,colsrc;
	_setpixelinfo info;

	(TILEIMGWORK->copy_color)(&colsrc, colbuf);

	//描画先の情報取得 + マスク処理

	if(_setpixeldraw_dstpixel(p, x, y, &colsrc, &info))
		return;

	//色処理 (dst + src -> res)

	(TILEIMGWORK->copy_color)(&colres, &info.coldst);

	(g_tileimage_dinfo.func_pixelcol)(p, &colres, &colsrc, param);

	//色セット

	_setpixeldraw_setcolor(p, x, y, &colres, &info);
}


//================================
// ぼかし
//================================


/* ぼかしの色取得 (8bit) */

static void _getcolor_blur_8bit(TileImage *p,int x,int y,int range,void *dstcol)
{
	int cnt,ix,iy,yy,rr,r,g,b,a;
	RGBA8 col;

	r = g = b = a = 0;
	cnt = 0;
	rr = range * range;

	for(iy = -range; iy <= range; iy++)
	{
		yy = iy * iy;
		
		for(ix = -range; ix <= range; ix++)
		{
			if(ix * ix + yy <= rr)
			{
				if(__TileImage_getDrawSrcColor(p, x + ix, y + iy, &col) && col.a)
				{
					r += col.r * col.a;
					g += col.g * col.a;
					b += col.b * col.a;
					a += col.a;
				}

				cnt++;
			}
		}
	}

	//

	col.a = (int)((double)a / cnt + 0.5);

	if(!col.a)
		*((uint32_t *)dstcol) = 0;
	else
	{
		col.r = (int)((double)r / a + 0.5);
		col.g = (int)((double)g / a + 0.5);
		col.b = (int)((double)b / a + 0.5);

		*((uint32_t *)dstcol) = col.v32;
	}
}

/* ぼかしの色取得 (16bit) */

static void _getcolor_blur_16bit(TileImage *p,int x,int y,int range,void *dstcol)
{
	int cnt,ix,iy,yy,rr;
	double r,g,b,a,d;
	RGBA16 col;

	r = g = b = a = 0;
	cnt = 0;
	rr = range * range;

	for(iy = -range; iy <= range; iy++)
	{
		yy = iy * iy;
		
		for(ix = -range; ix <= range; ix++)
		{
			if(ix * ix + yy <= rr)
			{
				if(__TileImage_getDrawSrcColor(p, x + ix, y + iy, &col) && col.a)
				{
					d = (double)col.a / 0x8000;
					
					r += col.r * d;
					g += col.g * d;
					b += col.b * d;
					a += d;
				}

				cnt++;
			}
		}
	}

	//

	col.a = (int)(a / cnt * 0x8000 + 0.5);

	if(!col.a)
		*((uint64_t *)dstcol) = 0;
	else
	{
		col.r = (int)(r / a + 0.5);
		col.g = (int)(g / a + 0.5);
		col.b = (int)(b / a + 0.5);

		*((uint64_t *)dstcol) = col.v64;
	}
}


//================================
// 描画用、色のセット
//================================
/*
 * - x,y がタイル配列内 & タイルがない場合は作成される。
 * - タイル配列の範囲外の場合は、キャンバス範囲内であれば、配列がリサイズされる。
 *
 * img_save : 描画前の元イメージ保存先 (アンドゥ用)。
 *     元のタイルが空の場合は、TILEIMAGE_TILE_EMPTY がポインタにセットされる。
 *
 * img_brush_stroke : ブラシ描画時の描画済みの濃度値 (type = A のみ)
 *     常に描画先と同じ配列状態。
 */


/** 直接描画で色セット */

void TileImage_setPixel_draw_direct(TileImage *p,int x,int y,void *colbuf)
{
	uint64_t colres,colsrc;
	_setpixelinfo info;

	(TILEIMGWORK->copy_color)(&colsrc, colbuf);

	//描画先の情報取得 + マスク処理

	if(_setpixeldraw_dstpixel(p, x, y, &colsrc, &info))
		return;

	//色処理 (dst + src -> res)

	(TILEIMGWORK->copy_color)(&colres, &info.coldst);

	(g_tileimage_dinfo.func_pixelcol)(p, &colres, &colsrc, NULL);

	//色セット

	_setpixeldraw_setcolor(p, x, y, &colres, &info);
}

/** ストローク重ね塗り描画 (ドット用)
 *
 * ドット描画時は描画濃度が一定なので、作業用イメージは必要ない。 */

void TileImage_setPixel_draw_dot_stroke(TileImage *p,int x,int y,void *colbuf)
{
	_setpixelinfo info;
	uint64_t colres,colsrc;

	//描画先の情報取得 + マスク処理

	(TILEIMGWORK->copy_color)(&colsrc, colbuf);

	if(_setpixeldraw_dstpixel(p, x, y, &colsrc, &info))
		return;

	//描画前の色を取得

	_setpixeldraw_get_save_color(p, x, y, &info, &colres);

	//描画前+描画色で色処理

	(g_tileimage_dinfo.func_pixelcol)(p, &colres, &colsrc, NULL);

	//色セット

	_setpixeldraw_setcolor(p, x, y, &colres, &info);
}

/** ストローク重ね塗り描画 (ブラシ用)
 *
 * ストローク中の最大濃度を記録するための作業用イメージが必要。 */

void TileImage_setPixel_draw_brush_stroke(TileImage *p,int x,int y,void *colbuf)
{
	TileImage *img_stroke = g_tileimage_dinfo.img_brush_stroke;
	uint8_t **pptile,*tilebuf,*buf;
	uint64_t colres,colsrc;
	_setpixelinfo info;
	int bit,stroke_a,draw_a;
	mlkbool fupdate;

	//描画先の情報取得 + マスク処理

	(TILEIMGWORK->copy_color)(&colsrc, colbuf);

	if(_setpixeldraw_dstpixel(p, x, y, &colsrc, &info))
		return;

	//濃度用イメージから、ストローク中の現在濃度を取得

	bit = TILEIMGWORK->bits;

	pptile = NULL; //濃度描画時に使用する
	tilebuf = NULL;
	stroke_a = 0;

	if(info.pptile)
	{
		pptile = TILEIMAGE_GETTILE_BUFPT(img_stroke, info.tx, info.ty);

		if(*pptile)
		{
			tilebuf = (TILEIMGWORK->colfunc[TILEIMAGE_COLTYPE_ALPHA].getpixelbuf_at_tile)(img_stroke, *pptile, x, y);

			if(bit == 8)
				stroke_a = *tilebuf;
			else
				stroke_a = *((uint16_t *)tilebuf);
		}
	}

	//描画濃度の方が大きければ、濃度更新。
	//小さければ、現在の濃度値を使う。

	fupdate = FALSE;

	if(bit == 8)
	{
		buf = (uint8_t *)&colsrc + 3;
		draw_a = *buf;

		if(draw_a > stroke_a)
			fupdate = TRUE;
		else
			*buf = stroke_a;
	}
	else
	{
		buf = (uint8_t *)&colsrc + 6;
		draw_a = *((uint16_t *)buf);

		if(draw_a > stroke_a)
			fupdate = TRUE;
		else
			*((uint16_t *)buf) = stroke_a;
	}

	//描画前の色を取得

	_setpixeldraw_get_save_color(p, x, y, &info, &colres);

	//色処理

	(g_tileimage_dinfo.func_pixelcol)(p, &colres, &colsrc, NULL);

	//色セット
	// :描画先のタイル配列がリサイズされた時は、濃度イメージも同時にリサイズされる。

	if(_setpixeldraw_setcolor(p, x, y, &colres, &info) == 0
		&& fupdate
		&& !g_tileimage_dinfo.err) //前回含めエラーが出た場合は処理しない
	{
		//描画先に正しく色がセットされた時、濃度イメージに描画
		// :タイル配列のリサイズ後にセットする必要がある。
		// :描画後に色が変化しない場合、この処理はしない。

		//取得時にタイル範囲外だった場合は、タイル位置再取得
		
		if(!pptile)
			pptile = TILEIMAGE_GETTILE_BUFPT(img_stroke, info.tx, info.ty);

		//タイル未確保なら作成

		if(!tilebuf)
		{
			*pptile = TileImage_allocTile_clear(img_stroke);
			if(!(*pptile))
			{
				g_tileimage_dinfo.err = 1;
				return;
			}

			tilebuf = (TILEIMGWORK->colfunc[TILEIMAGE_COLTYPE_ALPHA].getpixelbuf_at_tile)(img_stroke, *pptile, x, y);
		}

		//セット

		if(bit == 8)
			*tilebuf = draw_a;
		else
			*((uint16_t *)tilebuf) = draw_a;
	}
}

/** ドットペン形状を使って、直接描画 */

void TileImage_setPixel_draw_dotpen_direct(TileImage *p,int x,int y,void *pix)
{
	_setpixeldraw_dotpen(p, x, y, pix, TileImage_setPixel_draw_direct);
}

/** ドットペン形状を使って、ストローク重ね塗り描画 */

void TileImage_setPixel_draw_dotpen_stroke(TileImage *p,int x,int y,void *pix)
{
	_setpixeldraw_dotpen(p, x, y, pix, TileImage_setPixel_draw_dot_stroke);
}

/** ドットペン形状を使って、矩形上書き描画 */

void TileImage_setPixel_draw_dotpen_overwrite_square(TileImage *p,int x,int y,void *pix)
{
	_setpixeldraw_dotpen_overwrite(p, x, y, pix, TileImage_setPixel_draw_direct);
}

/** ぼかし描画 */

void TileImage_setPixel_draw_blur(TileImage *p,int x,int y,void *colbuf)
{
	uint64_t colres,colsrc;
	_setpixelinfo info;

	(TILEIMGWORK->copy_color)(&colsrc, colbuf);

	//描画先の情報取得 + マスク処理

	if(_setpixeldraw_dstpixel(p, x, y, &colsrc, &info))
		return;

	//テキスチャ適用で透明の場合、除外

	if((TILEIMGWORK->is_transparent)(&colsrc))
		return;

	//色処理

	if(TILEIMGWORK->bits == 8)
		_getcolor_blur_8bit(p, x, y, g_tileimage_dinfo.brushdp->blur_range, &colres);
	else
		_getcolor_blur_16bit(p, x, y, g_tileimage_dinfo.brushdp->blur_range, &colres);

	//色セット

	_setpixeldraw_setcolor(p, x, y, &colres, &info);
}

/** 指先描画 */

void TileImage_setPixel_draw_finger(TileImage *p,int x,int y,void *pix)
{
	uint8_t *ps,f,val;
	int size,ix,iy,dx,dy,bufpos;
	TileImageFingerPixelParam param;

	size = DotShape_getData(&ps);

	//

	dy = y - (size >> 1);
	f = 0x80;
	bufpos = 0;

	for(iy = size; iy > 0; iy--, dy++)
	{
		dx = x - (size >> 1);

		for(ix = size; ix > 0; ix--, dx++, bufpos++)
		{
			if(f == 0x80) val = *(ps++);

			if(val & f)
			{
				param.x = dx;
				param.y = dy;
				param.bufpos = bufpos;
				
				_setpixel_with_param(p, dx, dy, pix, &param);
			}

			f >>= 1;
			if(!f) f = 0x80;
		}
	}
}

/** 元の色 (RGB) に指定値を足して描画 (フィルタ用) */

void TileImage_setPixel_draw_addsub(TileImage *p,int x,int y,int v)
{
	int i,n;
	uint8_t col8[4];
	uint16_t col16[4];

	if(TILEIMGWORK->bits == 8)
	{
		TileImage_getPixel(p, x, y, col8);

		if(col8[3])
		{
			for(i = 0; i < 3; i++)
			{
				n = col8[i] + v;

				if(n < 0) n = 0;
				else if(n > 255) n = 255;

				col8[i] = n;
			}

			TileImage_setPixel_draw_direct(p, x, y, col8);
		}
	}
	else
	{
		TileImage_getPixel(p, x, y, col16);

		if(col16[3])
		{
			for(i = 0; i < 3; i++)
			{
				n = col16[i] + v;

				if(n < 0) n = 0;
				else if(n > 0x8000) n = 0x8000;

				col16[i] = n;
			}

			TileImage_setPixel_draw_direct(p, x, y, col16);
		}
	}
}


//================================
// 色のセット (描画用以外)
//================================
/*
 * pix : 8bit なら RGBA8, 16bit なら RGBA16
 */


/** 色をセット
 *
 * タイル作成。透明の場合もセット */

void TileImage_setPixel_new(TileImage *p,int x,int y,void *pix)
{
	uint8_t *buf;

	if((buf = __TileImage_getPixelBuf_new(p, x, y)))
		(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, pix);
}

/** 色をセット
 *
 * タイル作成。透明の場合はセットしない。 */

void TileImage_setPixel_new_notp(TileImage *p,int x,int y,void *pix)
{
	uint8_t *buf;
	int a;

	if(TILEIMGWORK->bits == 8)
		a = *((uint8_t *)pix + 3);
	else
		a = *((uint16_t *)pix + 3);

	if(a)
	{
		if((buf = __TileImage_getPixelBuf_new(p, x, y)))
			(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, pix);
	}
}

/** 色をセット
 *
 * TileImage_setPixel_new() + rcdraw 範囲追加 */

void TileImage_setPixel_new_drawrect(TileImage *p,int x,int y,void *pix)
{
	uint8_t *buf;

	if((buf = __TileImage_getPixelBuf_new(p, x, y)))
	{
		(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, pix);

		mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);
	}
}

/** 色をセット
 *
 * [ 色計算/配列拡張なし] */

void TileImage_setPixel_new_colfunc(TileImage *p,int x,int y,void *pix)
{
	uint64_t col,coldst;
	uint8_t *buf;

	//描画後の色

	TileImage_getPixel(p, x, y, &coldst);

	col = coldst;

	(g_tileimage_dinfo.func_pixelcol)(p, &col, pix, NULL);

	//色が変わらない

	if((TILEIMGWORK->colfunc[p->type].is_same_rgba)(&col, &coldst))
		return;

	//セット
	
	if((buf = __TileImage_getPixelBuf_new(p, x, y)))
		(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, &col);
}

/** 色をセット (作業用イメージなどへの描画時。マスク類は処理しない)
 *
 * [ 配列拡張/色計算/描画範囲セット ] */

void TileImage_setPixel_work(TileImage *p,int x,int y,void *pix)
{
	uint64_t coldst,colres;
	int tx,ty;
	uint8_t **pptile,*buf;

	//colres = 描画後の色

	TileImage_getPixel(p, x, y, &coldst);

	colres = coldst;

	(g_tileimage_dinfo.func_pixelcol)(p, &colres, pix, NULL);

	//色が変わらない

	if((TILEIMGWORK->colfunc[p->type].is_same_rgba)(&colres, &coldst))
		return;

	//タイル位置

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
	{
		//キャンバス範囲外なら何もしない

		if(x < 0 || y < 0 || x >= TILEIMGWORK->imgw || y >= TILEIMGWORK->imgh)
			return;

		//配列リサイズ & タイル位置再計算

		if(!TileImage_resizeTileBuf_includeCanvas(p))
			return;

		TileImage_pixel_to_tile(p, x, y, &tx, &ty);
	}

	//タイル確保

	pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(!(*pptile))
	{
		*pptile = TileImage_allocTile_clear(p);
		if(!(*pptile)) return;
	}

	//セット

	buf = (TILEIMGWORK->colfunc[p->type].getpixelbuf_at_tile)(p, *pptile, x, y);

	(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, &colres);

	//描画範囲追加

	mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);
}

/** 色をセット (テキストレイヤ描画用)
 *
 * - マスク類は処理しない。
 * - 配列拡張は制限なし。
 *
 * [ 配列拡張/色計算/描画範囲セット ] */

void TileImage_setPixel_nolimit(TileImage *p,int x,int y,void *pix)
{
	uint64_t coldst,colres;
	int tx,ty;
	uint8_t **pptile,*buf;

	//colres = 描画後の色

	TileImage_getPixel(p, x, y, &coldst);

	colres = coldst;

	(g_tileimage_dinfo.func_pixelcol)(p, &colres, pix, NULL);

	//色が変わらない

	if((TILEIMGWORK->colfunc[p->type].is_same_rgba)(&colres, &coldst))
		return;

	//タイル位置

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
	{
		//配列リサイズ & タイル位置再計算

		if(!TileImage_resizeTileBuf_canvas_point(p, x, y))
			return;

		TileImage_pixel_to_tile(p, x, y, &tx, &ty);
	}

	//タイル確保

	pptile = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(!(*pptile))
	{
		*pptile = TileImage_allocTile_clear(p);
		if(!(*pptile)) return;
	}

	//セット

	buf = (TILEIMGWORK->colfunc[p->type].getpixelbuf_at_tile)(p, *pptile, x, y);

	(TILEIMGWORK->colfunc[p->type].setpixel)(p, buf, x, y, &colres);

	//描画範囲追加

	mRectIncPoint(&g_tileimage_dinfo.rcdraw, x, y);
}


//================================
// 取得
//================================


/** 指定位置のピクセルバッファ位置取得
 *
 * その位置にタイルがない場合は作成する。ただし、タイル配列の拡張は行わない。
 *
 * return: 範囲外または、タイル確保失敗で NULL */

uint8_t *__TileImage_getPixelBuf_new(TileImage *p,int x,int y)
{
	int tx,ty;
	uint8_t **pp;

	//タイル位置

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
		return NULL;

	//タイルがない場合は確保

	pp = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(!(*pp))
	{
		*pp = TileImage_allocTile_clear(p);
		if(!(*pp)) return NULL;
	}

	//ピクセルのバッファ位置

	return (TILEIMGWORK->colfunc[p->type].getpixelbuf_at_tile)(p, *pp, x, y);
}

/** (描画中) 指定位置から、描画前の元イメージの色を取得
 *
 * ぼかし/水彩時
 *
 * return: FALSE で透明 (色はセットされない) */

mlkbool __TileImage_getDrawSrcColor(TileImage *p,int x,int y,void *dstcol)
{
	int tx,ty;
	uint8_t **pptile;
	TileImage *src;

	//イメージ範囲外はクリッピング

	if(x < 0) x = 0;
	else if(x >= TILEIMGWORK->imgw) x = TILEIMGWORK->imgw - 1;

	if(y < 0) y = 0;
	else if(y >= TILEIMGWORK->imgh) y = TILEIMGWORK->imgh - 1;
	
	//

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
		return FALSE;

	//保存イメージでタイルが確保されていれば、保存イメージから。
	//確保されていなければ、描画先イメージから。

	src = g_tileimage_dinfo.img_save;
	pptile = TILEIMAGE_GETTILE_BUFPT(src, tx, ty);

	if(*pptile)
	{
		//元が空タイルの場合
		if(*pptile == TILEIMAGE_TILE_EMPTY) return FALSE;
	}
	else
	{
		src = p;
		pptile = TILEIMAGE_GETTILE_BUFPT(src, tx, ty);
	}

	if(!(*pptile)) return FALSE;

	//タイルがある場合

	(TILEIMGWORK->colfunc[p->type].getpixel_at_tile)(src, *pptile, x, y, dstcol);

	return TRUE;
}

/** 指定位置の色が不透明か (A=0 でないか) */

mlkbool TileImage_isPixel_opaque(TileImage *p,int x,int y)
{
	RGBA16 c16;
	RGBA8 c8;

	if(TILEIMGWORK->bits == 8)
	{
		TileImage_getPixel(p, x, y, &c8);

		return (c8.a != 0);
	}
	else
	{
		TileImage_getPixel(p, x, y, &c16);

		return (c16.a != 0);
	}
}

/** 指定位置の色が透明か */

mlkbool TileImage_isPixel_transparent(TileImage *p,int x,int y)
{
	RGBA16 c16;
	RGBA8 c8;

	if(TILEIMGWORK->bits == 8)
	{
		TileImage_getPixel(p, x, y, &c8);

		return (c8.a == 0);
	}
	else
	{
		TileImage_getPixel(p, x, y, &c16);

		return (c16.a == 0);
	}
}

/** 指定位置の色を取得 (範囲外は透明)
 *
 * dst: 現在のビットでの値 (8bit:RGBA8, 16bit:RGBA16) */

void TileImage_getPixel(TileImage *p,int x,int y,void *dst)
{
	uint8_t *ptile;
	int tx,ty;

	if(!TileImage_pixel_to_tile(p, x, y, &tx, &ty))
	{
		//タイル範囲外

		(TILEIMGWORK->setcol_rgba_transparent)(dst);
	}
	else
	{
		ptile = TILEIMAGE_GETTILE_PT(p, tx, ty);

		if(ptile)
			(TILEIMGWORK->colfunc[p->type].getpixel_at_tile)(p, ptile, x, y, dst);
		else
			//タイル未確保
			(TILEIMGWORK->setcol_rgba_transparent)(dst);
	}
}

/** 指定位置のアルファ値をバッファに取得 */

void TileImage_getPixel_alpha_buf(TileImage *p,int x,int y,void *dst)
{
	uint64_t col;

	TileImage_getPixel(p, x, y, &col);

	if(TILEIMGWORK->bits == 8)
		*((uint8_t *)dst) = *((uint8_t *)&col + 3);
	else
		*((uint16_t *)dst) = *((uint16_t *)&col + 3);
}

/** 指定位置の色を取得 (イメージ範囲外はクリッピング) */

void TileImage_getPixel_clip(TileImage *p,int x,int y,void *dst)
{
	if(x < 0)
		x = 0;
	else if(x >= TILEIMGWORK->imgw)
		x = TILEIMGWORK->imgw - 1;

	if(y < 0)
		y = 0;
	else if(y >= TILEIMGWORK->imgh)
		y = TILEIMGWORK->imgh - 1;

	TileImage_getPixel(p, x, y, dst);
}

/** RGBAcombo で色を取得 (8bit/16bit 両方の値) */

void TileImage_getPixel_combo(TileImage *p,int x,int y,RGBAcombo *dst)
{
	if(TILEIMGWORK->bits == 8)
	{
		TileImage_getPixel(p, x, y, &dst->c8);

		RGBA8_to_RGBA16(&dst->c16, &dst->c8);
	}
	else
	{
		TileImage_getPixel(p, x, y, &dst->c16);

		RGBA16_to_RGBA8(&dst->c8, &dst->c16);
	}
}

/** オーバーサンプリング用、指定位置の色を加算 */

void TileImage_getPixel_oversamp(TileImage *p,int x,int y,int *pr,int *pg,int *pb,int *pa)
{
	if(TILEIMGWORK->bits == 8)
	{
		RGBA8 c8;

		TileImage_getPixel(p, x, y, &c8);

		if(c8.a)
		{
			*pr += c8.r * c8.a / 255;
			*pg += c8.g * c8.a / 255;
			*pb += c8.b * c8.a / 255;
			*pa += c8.a;
		}
	}
	else
	{
		RGBA16 c16;

		TileImage_getPixel(p, x, y, &c16);

		if(c16.a)
		{
			*pr += c16.r * c16.a >> 15;
			*pg += c16.g * c16.a >> 15;
			*pb += c16.b * c16.a >> 15;
			*pa += c16.a;
		}
	}
}

/** オーバーサンプリングの色値をチェック柄と合成して 8bitRGB 取得 */

void TileImage_getColor_oversamp_blendPlaid(TileImage *p,RGB8 *dst,
	int r,int g,int b,int a,int div,int colno)
{
	const uint8_t col8[] = {250,220};
	const uint16_t col16[] = {(250<<15)/255, (220<<15)/255};
	int c;

	if(a == 0)
	{
		r = g = b = col8[colno];
	}
	else if(TILEIMGWORK->bits == 8)
	{
		c = col8[colno];

		r = r * 255 / a;
		g = g * 255 / a;
		b = b * 255 / a;
		a /= div;

		r = (r - c) * a / 255 + c;
		g = (g - c) * a / 255 + c;
		b = (b - c) * a / 255 + c;
	}
	else
	{
		c = col16[colno];

		r = ((int64_t)r << 15) / a;
		g = ((int64_t)g << 15) / a;
		b = ((int64_t)b << 15) / a;
		a /= div;

		r = (((r - c) * a >> 15) + c) * 255 >> 15;
		g = (((g - c) * a >> 15) + c) * 255 >> 15;
		b = (((b - c) * a >> 15) + c) * 255 >> 15;
	}

	dst->r = r;
	dst->g = g;
	dst->b = b;
}

/* Bicubic 重み取得 */

static double _get_bicubic(double d)
{
	d = fabs(d);

	if(d < 1.0)
		return (d - 2) * d * d + 1;
	else if(d < 2.0)
		return ((5 - d) * d - 8) * d + 4;
	else
		return 0;
}

/** Bicubic で補間した色を取得
 *
 * p の (0,0)-(sw x sh) の範囲で色を取得する。
 *
 * dx,dy: double でのソース位置
 * sw,sh: ソース画像のサイズ
 * return: FALSE で範囲外、または透明 */

mlkbool TileImage_getColor_bicubic(TileImage *p,double dx,double dy,int sw,int sh,uint64_t *dstcol)
{
	int i,j,x,y,nx,ny;
	double wx[4],wy[4];
	RGBAdouble dcol;
	uint64_t col;

	nx = floor(dx);
	ny = floor(dy);

	//ソース範囲外

	if(nx < 0 || ny < 0 || nx >= sw || ny >= sh)
		return FALSE;

	//参照する左上位置

	nx--;
	ny--;

	//Bicubic 重みテーブル

	for(i = 0; i < 4; i++)
	{
		wx[i] = _get_bicubic(dx - (nx + i + 0.5));
		wy[i] = _get_bicubic(dy - (ny + i + 0.5));
	}

	//ソースの色 (4x4近傍)

	dcol.r = dcol.g = dcol.b = dcol.a = 0;

	for(i = 0; i < 4; i++)
	{
		y = ny + i;
		if(y < 0) y = 0;
		else if(y >= sh) y = sh - 1;
	
		for(j = 0; j < 4; j++)
		{
			x = nx + j;
			if(x < 0) x = 0;
			else if(x >= sw) x = sw - 1;
		
			TileImage_getPixel(p, x, y, &col);

			(TILEIMGWORK->add_weight_color)(&dcol, &col, wx[j] * wy[i]);
		}
	}

	return (TILEIMGWORK->get_weight_color)(dstcol, &dcol);
}


//================================
// ほか
//================================


/** (指先用) 描画開始時、始点の位置の色をバッファにセット */

void TileImage_setFingerBuf(TileImage *p,int x,int y)
{
	uint8_t *ps,*pd,f,val;
	int size,ix,iy,dx,dy,incdst;

	size = DotShape_getData(&ps);

	pd = TILEIMGWORK->finger_buf;
	incdst = (TILEIMGWORK->bits == 8)? 4: 8;

	//

	dy = y - size / 2;
	f = 0x80;

	for(iy = size; iy > 0; iy--, dy++)
	{
		dx = x - size / 2;
	
		for(ix = size; ix > 0; ix--, dx++, pd += incdst)
		{
			if(f == 0x80) val = *(ps++);

			if(val & f)
				TileImage_getPixel_clip(p, dx, dy, pd);

			f >>= 1;
			if(!f) f = 0x80;
		}
	}
}

