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
 * TileImage
 **********************************/

#include <string.h>
#include <math.h>
#include <time.h>

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_pixbuf.h"
#include "mlk_rand.h"
#include "mlk_simd.h"

#include "def_tileimage.h"

#include "tileimage.h"
#include "pv_tileimage.h"
#include "tileimage_drawinfo.h"

#include "imagecanvas.h"
#include "table_data.h"


//--------------------------

TileImageWorkData *g_tileimg_work = NULL;

TileImageDrawInfo g_tileimage_dinfo;

//--------------------------

void __TileImage_init_curve(void);

void __TileImage_setColFunc_RGBA(TileImageColFuncData *p,int bits);
void __TileImage_setColFunc_Gray(TileImageColFuncData *p,int bits);
void __TileImage_setColFunc_alpha(TileImageColFuncData *p,int bits);
void __TileImage_setColFunc_alpha1bit(TileImageColFuncData *p,int bits);

void __TileImage_setBitFunc(void);
void __TileImage_setPixelColorFunc(TileImagePixelColorFunc *p,int bits);

void BlendColorFunc_setTable_8bit(BlendColorFunc *p);
void BlendColorFunc_setTable_16bit(BlendColorFunc *p);

//--------------------------



//==========================
// TileImageDrawInfo
//==========================


/** TileImageDrawInfo の rcdraw の範囲をクリア */

void TileImageDrawInfo_clearDrawRect(void)
{
	g_tileimage_dinfo.rcdraw.x1 = g_tileimage_dinfo.rcdraw.y1 = 1;
	g_tileimage_dinfo.rcdraw.x2 = g_tileimage_dinfo.rcdraw.y2 = 0;
}


//==========================
// TileImage 全体
//==========================


/** 初期化 */

mlkbool TileImage_init(void)
{
	TileImageWorkData *p;

	mMemset0(&g_tileimage_dinfo, sizeof(TileImageDrawInfo));

	//作業用データ確保

	p = (TileImageWorkData *)mMalloc0(sizeof(TileImageWorkData));
	if(!p) return FALSE;

	g_tileimg_work = p;

	//指先用バッファ

	p->finger_buf = (uint8_t *)mMalloc(101 * 101 * 8);
	if(!(p->finger_buf)) return FALSE;

	//乱数初期化

	p->rand = mRandSFMT_new();

	mRandSFMT_init(p->rand, time(NULL));

	//曲線補間用パラメータ

	__TileImage_init_curve();

	return TRUE;
}

/** 終了時 */

void TileImage_finish(void)
{
	TileImageWorkData *p = TILEIMGWORK;

	if(p)
	{
		mRandSFMT_free(p->rand);

		mFree(p->finger_buf);
		mFree(p);
	}
}

/** イメージのビット数をセット */

void TileImage_global_setImageBits(int bits)
{
	TileImageWorkData *p = TILEIMGWORK;

	if(bits != p->bits)
	{
		p->bits = bits;

		p->bits_val = (bits == 8)? 255: 0x8000;

		//ビットごとの関数

		__TileImage_setBitFunc();

		//ピクセルカラー処理関数

		__TileImage_setPixelColorFunc(p->pixcolfunc, bits);

		//カラー別関数

		__TileImage_setColFunc_RGBA(p->colfunc + TILEIMAGE_COLTYPE_RGBA, bits);
		__TileImage_setColFunc_Gray(p->colfunc + TILEIMAGE_COLTYPE_GRAY, bits);
		__TileImage_setColFunc_alpha(p->colfunc + TILEIMAGE_COLTYPE_ALPHA, bits);
		__TileImage_setColFunc_alpha1bit(p->colfunc + TILEIMAGE_COLTYPE_ALPHA1BIT, bits);

		//合成関数

		if(bits == 8)
			BlendColorFunc_setTable_8bit(p->blendfunc);
		else
			BlendColorFunc_setTable_16bit(p->blendfunc);
	}
}

/** イメージのサイズをセット */

void TileImage_global_setImageSize(int w,int h)
{
	TILEIMGWORK->imgw = w;
	TILEIMGWORK->imgh = h;
}

/** DPI 値の変更
 *
 * トーン化レイヤなどで必要になる */

void TileImage_global_setDPI(int dpi)
{
	TILEIMGWORK->dpi = dpi;
}

/** トーン化レイヤをグレイスケール表示 */

void TileImage_global_setToneToGray(int flag)
{
	TILEIMGWORK->is_tone_to_gray = (flag != 0);
}

/** 指定タイプとビット数でのタイルサイズを取得 */

int TileImage_global_getTileSize(int type,int bits)
{
	int size;

	if(type == TILEIMAGE_COLTYPE_ALPHA1BIT)
		size = 64 * 64 / 8;
	else
	{
		switch(type)
		{
			case TILEIMAGE_COLTYPE_RGBA:
				size = 64 * 64 * 4;
				break;
			case TILEIMAGE_COLTYPE_GRAY:
				size = 64 * 64 * 2;
				break;
			default:
				size = 64 * 64;
				break;
		}

		if(bits == 16) size *= 2;
	}

	return size;
}

/** タイルバッファを、現在ビットの最大サイズで確保 */

uint8_t *TileImage_global_allocTileBitMax(void)
{
	int size;

	if(TILEIMGWORK->bits == 8)
		size = 64 * 64 * 4;
	else
		size = 64 * 64 * 8;

	return (uint8_t *)mMallocAlign(size, 16);
}

/** 乱数用 mRandSFMT * を取得 */

void *TileImage_global_getRand(void)
{
	return (void *)TILEIMGWORK->rand;
}

/** 色処理関数を取得 */

TileImagePixelColorFunc TileImage_global_getPixelColorFunc(int type)
{
	return TILEIMGWORK->pixcolfunc[type];
}

/** 描画情報の色処理関数をセット */

void TileImage_global_setPixelColorFunc(int type)
{
	g_tileimage_dinfo.func_pixelcol = TILEIMGWORK->pixcolfunc[type];
}

/** タイルがすべて透明か (A/A1用、共通) */

mlkbool TileImage_tile_isTransparent_forA(uint8_t *tile,int size)
{
	int i;

#if MLK_ENABLE_SSE2 && _TILEIMG_SIMD_ON

	//---- 16byte 単位でゼロ比較

	__m128i v,vcmp;

	vcmp = _mm_setzero_si128();

	for(i = size / 16; i; i--, tile += 16)
	{
		v = _mm_load_si128((__m128i *)tile);
		v = _mm_cmpeq_epi32(v, vcmp);	//0なら全ビットON

		//8bit x 16個の最上位ビット取得
		if(_mm_movemask_epi8(v) != 0xffff)
			return FALSE;
	}

#else

	//---- 8byte 単位でゼロ比較
	
	uint64_t *ps = (uint64_t *)tile;

	for(i = size / 8; i; i--, ps++)
	{
		if(*ps) return FALSE;
	}

#endif

	return TRUE;
}


//==========================
// 解放
//==========================


/** すべて解放 */

void TileImage_free(TileImage *p)
{
	uint8_t **pp;
	uint32_t i;

	if(p)
	{
		if(p->ppbuf)
		{
			pp = p->ppbuf;
			
			for(i = p->tilew * p->tileh; i; i--, pp++)
			{
				if(*pp && *pp != TILEIMAGE_TILE_EMPTY)
					mFree(*pp);
			}

			mFree(p->ppbuf);
		}

		mFree(p);
	}
}

/** 一つのタイルを解放 */

void TileImage_freeTile(uint8_t **pptile)
{
	if(*pptile)
	{
		if(*pptile != TILEIMAGE_TILE_EMPTY)
			mFree(*pptile);

		*pptile = NULL;
	}
}

/** 指定タイル位置のタイルを解放 (アンドゥ用) */

void TileImage_freeTile_atPos(TileImage *p,int tx,int ty)
{
	uint8_t **pp = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

	if(*pp)
		TileImage_freeTile(pp);
}

/** 全タイルとタイルバッファを解放 (TileImage 自体は残す) */

void TileImage_freeTileBuf(TileImage *p)
{
	if(p)
	{
		TileImage_freeAllTiles(p);

		mFree(p->ppbuf);
		p->ppbuf = NULL;
	}
}

/** タイルをすべて解放 (タイル配列は残す) */

void TileImage_freeAllTiles(TileImage *p)
{
	uint8_t **pp;
	int i;

	if(p && p->ppbuf)
	{
		pp = p->ppbuf;

		for(i = p->tilew * p->tileh; i; i--, pp++)
			TileImage_freeTile(pp);
	}
}

/** 空のタイルをすべて解放
 *
 * return: すべて空なら TRUE */

mlkbool TileImage_freeEmptyTiles(TileImage *p)
{
	uint8_t **ppbuf;
	TileImageColFunc_isTransparentTile func;
	uint32_t i;
	mlkbool ret = TRUE;

	if(p && p->ppbuf)
	{
		ppbuf = p->ppbuf;
		func = TILEIMGWORK->colfunc[p->type].is_transparent_tile;
		
		for(i = p->tilew * p->tileh; i; i--, ppbuf++)
		{
			if(*ppbuf)
			{
				if((func)(*ppbuf))
					TileImage_freeTile(ppbuf);
				else
					ret = FALSE;
			}
		}
	}

	return ret;
}

/** 空のタイルをすべて解放 (アンドゥ用イメージでタイルが確保されている部分のみ)
 *
 * imgundo でタイルが確保されている部分は、イメージが変更された部分。
 * それらの中で、消しゴム等ですべて透明になったタイルは解放して、メモリ負担を軽減させる。 */

void TileImage_freeEmptyTiles_byUndo(TileImage *p,TileImage *imgundo)
{
	uint8_t **ppbuf,**ppundo;
	uint32_t i;
	TileImageColFunc_isTransparentTile func;

	if(!imgundo) return;
	
	func = TILEIMGWORK->colfunc[p->type].is_transparent_tile;

	ppbuf = p->ppbuf;
	ppundo = imgundo->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, ppbuf++, ppundo++)
	{
		if(*ppundo && *ppbuf && (func)(*ppbuf))
			TileImage_freeTile(ppbuf);
	}
}


//==========================
// 作成/複製
//==========================


/** 作成 (px サイズ指定) */

TileImage *TileImage_new(int type,int w,int h)
{
	return __TileImage_create(type, (w + 63) / 64, (h + 63) / 64);
}

/** 作成 (TileImageInfo から) */

TileImage *TileImage_newFromInfo(int type,TileImageInfo *info)
{
	TileImage *p;

	if(info->tilew <= 0 || info->tileh <= 0)
		//新規状態
		p = TileImage_new(type, 1, 1);
	else
	{
		p = __TileImage_create(type, info->tilew, info->tileh);
		if(p)
		{
			p->offx = info->offx;
			p->offy = info->offy;
		}
	}

	return p;
}

/** 作成 (ピクセル範囲から) */

TileImage *TileImage_newFromRect(int type,const mRect *rc)
{
	TileImageInfo info;

	info.offx = rc->x1;
	info.offy = rc->y1;
	info.tilew = (rc->x2 - rc->x1 + 1 + 63) >> 6;
	info.tileh = (rc->y2 - rc->y1 + 1 + 63) >> 6;

	return TileImage_newFromInfo(type, &info);
}

/** 作成 (ピクセル範囲から。ファイル読み込み用)
 *
 * rc: x2,y2 は +1。x1 >= x2 or y1 >= y2 で空状態。 */

TileImage *TileImage_newFromRect_forFile(int type,const mRect *rc)
{
	TileImageInfo info;

	if(rc->x1 >= rc->x2 && rc->y1 >= rc->y2)
	{
		//空
		
		info.offx = info.offy = 0;
		info.tilew = info.tileh = 1;
	}
	else
	{
		info.tilew = (rc->x2 - rc->x1) >> 6;
		info.tileh = (rc->y2 - rc->y1) >> 6;

		if(info.tilew <= 0) info.tilew = 1;
		if(info.tileh <= 0) info.tileh = 1;

		info.offx = rc->x1;
		info.offy = rc->y1;
	}

	return TileImage_newFromInfo(type, &info);
}

/** 複製を新規作成 (タイルイメージも含む)
 *
 * return: すべてのタイルが確保できなかった場合、NULL */

TileImage *TileImage_newClone(TileImage *src)
{
	TileImage *p;
	uint8_t **ppsrc,**ppdst;
	uint32_t i;

	//作成

	p = __TileImage_create(src->type, src->tilew, src->tileh);
	if(!p) return NULL;

	p->offx = src->offx;
	p->offy = src->offy;
	p->col  = src->col;

	//タイルコピー

	ppsrc = src->ppbuf;
	ppdst = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, ppsrc++, ppdst++)
	{
		if(*ppsrc)
		{
			*ppdst = TileImage_allocTile(p);
			if(!(*ppdst))
			{
				TileImage_free(p);
				return NULL;
			}

			TileImage_copyTile(p, *ppdst, *ppsrc);
		}
	}

	return p;
}

/** ビット数を指定して、複製を作成
 *
 * コピーイメージなどで、コピー後にビット数が変更された場合など。
 *
 * return: すべてのタイルが確保できなかった場合、NULL */

TileImage *TileImage_newClone_bits(TileImage *src,int srcbits,int dstbits)
{
	TileImage *p;
	uint8_t **ppsrc,**ppdst;
	void *tblbuf = NULL;
	uint32_t i;

	//ビット数が同じ、またはA1bit

	if(srcbits == dstbits
		|| src->type == TILEIMAGE_COLTYPE_ALPHA1BIT)
	{
		return TileImage_newClone(src);
	}

	//作成

	p = __TileImage_create(src->type, src->tilew, src->tileh);
	if(!p) return NULL;

	p->offx = src->offx;
	p->offy = src->offy;
	p->col  = src->col;

	//タイルを変換してコピー

	tblbuf = TileImage_createToBits_table(dstbits);
	if(!tblbuf) goto ERR;

	ppsrc = src->ppbuf;
	ppdst = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, ppsrc++, ppdst++)
	{
		if(*ppsrc)
		{
			*ppdst = TileImage_allocTile(p);
			if(!(*ppdst)) goto ERR;

			TileImage_copyTile_convert(p, *ppdst, *ppsrc, tblbuf, dstbits);
		}
	}

	mFree(tblbuf);

	return p;

ERR:
	mFree(tblbuf);
	TileImage_free(p);
	return NULL;
}


//==========================
//
//==========================


/** p を、src と同じ構成になるように作成または配列リサイズ
 *
 * 描画時の元イメージ保存用、または、ストローク用イメージなどの作成に使う。
 * [!] 途中でイメージビットが変わる場合がある。
 *
 * p が NULL なら新規作成。
 * すでに構成が同じなら、そのまま p が返る。
 * エラーなら、解放されて NULL が返る。
 *
 * type: カラータイプ。負の値で src と同じ */

TileImage *TileImage_createSame(TileImage *p,TileImage *src,int type)
{
	if(type < 0) type = src->type;

	if(!p)
	{
		//------ 新規作成
		
		TileImageInfo info;

		TileImage_getInfo(src, &info);

		return TileImage_newFromInfo(type, &info);
	}
	else
	{
		//------ 作成済み。構成が同じならそのまま返す

		//移動ツールで移動した場合があるので、常にコピー

		p->offx = src->offx;
		p->offy = src->offy;

		//構成が異なる

		if(p->type != type
			|| p->tilesize != src->tilesize  //ビット数が異なる場合
			|| p->tilew != src->tilew || p->tileh != src->tileh)
		{
			//タイプ

			p->type = type;

			__TileImage_setTileSize(p);
		
			//配列再作成
			
			TileImage_freeTileBuf(p);

			p->tilew = src->tilew;
			p->tileh = src->tileh;

			if(!__TileImage_allocTileBuf(p))
			{
				TileImage_free(p);
				p = NULL;
			}
		}

		return p;
	}
}

/** イメージをクリアする
 *
 * タイルを削除して、タイル配列を 1x1 にする。 */

void TileImage_clear(TileImage *p)
{
	TileImage_freeTileBuf(p);

	p->offx = p->offy = 0;
	p->tilew = p->tileh = 1;

	__TileImage_allocTileBuf(p);
}


//===============================
// タイル配列
//===============================


/** タイル配列リサイズ (キャンバス全体を含むように) */

mlkbool TileImage_resizeTileBuf_includeCanvas(TileImage *p)
{
	mRect rc;

	//描画可能な px 範囲

	TileImage_getCanDrawRect_pixel(p, &rc);

	//px -> タイル位置

	TileImage_pixel_to_tile_rect(p, &rc);

	//リサイズ

	return __TileImage_resizeTileBuf(p, rc.x1, rc.y1,
			rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);
}

/** タイル配列リサイズ
 *
 * キャンバス全体と、指定位置を含む範囲 (キャンバス範囲外含む)。
 * 指定位置描画時に、制限無しでリサイズする用。  */

mlkbool TileImage_resizeTileBuf_canvas_point(TileImage *p,int x,int y)
{
	mRect rc;

	//描画可能な px 範囲

	TileImage_getCanDrawRect_pixel(p, &rc);

	//x,y を含む

	mRectIncPoint(&rc, x, y);

	//px -> タイル位置

	TileImage_pixel_to_tile_rect(p, &rc);

	//リサイズ

	return __TileImage_resizeTileBuf(p, rc.x1, rc.y1,
			rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1);
}

/** タイル配列リサイズ (最大サイズ指定)
 *
 * (0,0)-(w x h) が最大となるようにする。
 * すでにそれ以上のサイズなら何もしない。 */

mlkbool TileImage_resizeTileBuf_maxsize(TileImage *p,int w,int h)
{
	w = (w + 63) / 64;
	h = (h + 63) / 64;

	if(p->tilew >= w && p->tileh >= h)
		return TRUE;

	return __TileImage_resizeTileBuf(p, 0, 0, w, h);
}

/** p と src の範囲を結合し、p のタイル配列をリサイズ */

mlkbool TileImage_resizeTileBuf_combine(TileImage *p,TileImage *src)
{
	mRect rc,rc2;
	int tx1,ty1,tx2,ty2;

	//タイル配列全体での px 範囲を結合

	TileImage_getAllTilesPixelRect(p, &rc);
	TileImage_getAllTilesPixelRect(src, &rc2);

	mRectUnion(&rc, &rc2);

	//px 範囲から、p におけるタイル位置に変換

	TileImage_pixel_to_tile_nojudge(p, rc.x1, rc.y1, &tx1, &ty1);
	TileImage_pixel_to_tile_nojudge(p, rc.x2, rc.y2, &tx2, &ty2);

	//タイル配列リサイズ

	return __TileImage_resizeTileBuf(p, tx1, ty1, tx2 - tx1 + 1, ty2 - ty1 + 1);
}

/** タイル配列リサイズ (アンドゥ時用) */

mlkbool TileImage_resizeTileBuf_forUndo(TileImage *p,TileImageInfo *info)
{
	uint8_t **ppnew,**ppd;
	int ttx,tty,ix,iy,tx,ty,sw,sh;

	//配列は変更なし

	if(info->tilew == p->tilew && info->tileh == p->tileh)
		return TRUE;

	//新規確保

	ppnew = __TileImage_allocTileBuf_new(info->tilew, info->tileh);
	if(!ppnew) return FALSE;

	//データコピー

	ppd = ppnew;
	ttx = (info->offx - p->offx) >> 6;
	tty = (info->offy - p->offy) >> 6;
	sw = p->tilew;
	sh = p->tileh;

	for(iy = info->tileh, ty = tty; iy > 0; iy--, ty++)
	{
		for(ix = info->tilew, tx = ttx; ix > 0; ix--, tx++, ppd++)
		{
			if(tx >= 0 && tx < sw && ty >= 0 && ty < sh)
				*ppd = *(p->ppbuf + ty * sw + tx);
		}
	}

	//入れ替え

	mFree(p->ppbuf);

	p->ppbuf = ppnew;
	p->tilew = info->tilew;
	p->tileh = info->tileh;
	p->offx  = info->offx;
	p->offy  = info->offy;

	return TRUE;
}


//==========================
// タイル
//==========================


/** 一つのタイルを確保 */

uint8_t *TileImage_allocTile(TileImage *p)
{
	return (uint8_t *)mMallocAlign(p->tilesize, 16);
}

/** タイルを確保してクリア */

uint8_t *TileImage_allocTile_clear(TileImage *p)
{
	uint8_t *buf;

	buf = (uint8_t *)mMallocAlign(p->tilesize, 16);
	if(!buf) return NULL;

	TileImage_clearTile(p, buf);

	return buf;
}

/** ポインタの位置にタイルがなければ確保
 *
 * return: FALSE で、タイルの新規確保に失敗 */

mlkbool TileImage_allocTile_atptr(TileImage *p,uint8_t **ppbuf)
{
	if(!(*ppbuf))
	{
		*ppbuf = TileImage_allocTile(p);
		if(!(*ppbuf)) return FALSE;
	}

	return TRUE;
}

/** ポインタの位置にタイルがなければ確保して、クリア */

mlkbool TileImage_allocTile_atptr_clear(TileImage *p,uint8_t **ppbuf)
{
	if(!(*ppbuf))
	{
		*ppbuf = TileImage_allocTile(p);
		if(!(*ppbuf)) return FALSE;

		TileImage_clearTile(p, *ppbuf);
	}

	return TRUE;
}

/** 指定タイル位置のタイルを取得 (タイルが確保されていなければ確保)
 *
 * return: タイルポインタ。位置が範囲外や、確保失敗の場合は NULL */

uint8_t *TileImage_getTileAlloc_atpos(TileImage *p,int tx,int ty,mlkbool clear)
{
	uint8_t **pp;

	if(tx < 0 || ty < 0 || tx >= p->tilew || ty >= p->tileh)
		return NULL;
	else
	{
		pp = TILEIMAGE_GETTILE_BUFPT(p, tx, ty);

		if(!(*pp))
		{
			if(clear)
				*pp = TileImage_allocTile_clear(p);
			else
				*pp = TileImage_allocTile(p);
		}

		return *pp;
	}
}

/** タイルを透明にクリア */

void TileImage_clearTile(TileImage *p,uint8_t *tile)
{
#if MLK_ENABLE_SSE2 && _TILEIMG_SIMD_ON

	int i;
	__m128i v,*pd;

	pd = (__m128i *)tile;
	v = _mm_setzero_si128();
	
	for(i = p->tilesize / 16; i; i--, pd++)
		_mm_store_si128(pd, v);

#else

	memset(tile, 0, p->tilesize);

#endif
}

/** タイル内容をコピー
 *
 * [!] src も 16byte 境界であること */

void TileImage_copyTile(TileImage *p,uint8_t *dst,const uint8_t *src)
{
#if MLK_ENABLE_SSE2 && _TILEIMG_SIMD_ON

	int i;
	__m128i v1,v2,*pd,*ps;

	pd = (__m128i *)dst;
	ps = (__m128i *)src;
	
	for(i = p->tilesize / 32; i; i--, pd += 2, ps += 2)
	{
		v1 = _mm_load_si128(ps);
		v2 = _mm_load_si128(ps + 1);
		
		_mm_store_si128(pd, v1);
		_mm_store_si128(pd + 1, v2);
	}

#else

	memcpy(dst, src, p->tilesize);

#endif
}


//==========================
// セット/変更
//==========================


/** オフセット位置をセット */

void TileImage_setOffset(TileImage *p,int offx,int offy)
{
	p->offx = offx;
	p->offy = offy;
}

/** オフセット位置を相対移動 */

void TileImage_moveOffset_rel(TileImage *p,int x,int y)
{
	p->offx += x;
	p->offy += y;
}

/** 線の色セット */

void TileImage_setColor(TileImage *p,uint32_t col)
{
	if(p)
		RGB32bit_to_RGBcombo(&p->col, col);
}


//==========================
// 計算など
//==========================


/** px 位置からタイル位置を取得
 *
 * return: FALSE で範囲外 */

mlkbool TileImage_pixel_to_tile(TileImage *p,int x,int y,int *tilex,int *tiley)
{
	x = (x - p->offx) >> 6;
	y = (y - p->offy) >> 6;

	*tilex = x;
	*tiley = y;

	return (x >= 0 && x < p->tilew && y >= 0 && y < p->tileh);
}

/** px 位置からタイル位置を取得 (範囲外判定を行わない) */

void TileImage_pixel_to_tile_nojudge(TileImage *p,int x,int y,int *tilex,int *tiley)
{
	*tilex = (x - p->offx) >> 6;
	*tiley = (y - p->offy) >> 6;
}

/** px -> タイル位置 (mRect) */

void TileImage_pixel_to_tile_rect(TileImage *p,mRect *rc)
{
	TileImage_pixel_to_tile_nojudge(p, rc->x1, rc->y1, &rc->x1, &rc->y1);
	TileImage_pixel_to_tile_nojudge(p, rc->x2, rc->y2, &rc->x2, &rc->y2);
}

/** タイル位置から左上の px 位置取得 */

void TileImage_tile_to_pixel(TileImage *p,int tx,int ty,int *px,int *py)
{
	*px = (tx << 6) + p->offx;
	*py = (ty << 6) + p->offy;
}

/** すべてのタイルの範囲を px で取得 */

void TileImage_getAllTilesPixelRect(TileImage *p,mRect *rc)
{
	int x1,y1,x2,y2;

	TileImage_tile_to_pixel(p, 0, 0, &x1, &y1);
	TileImage_tile_to_pixel(p, p->tilew - 1, p->tileh - 1, &x2, &y2);

	rc->x1 = x1, rc->y1 = y1;
	rc->x2 = x2 + 63, rc->y2 = y2 + 63;
}


//======================
// 情報取得
//======================


/** オフセット位置を取得 */

void TileImage_getOffset(TileImage *p,mPoint *pt)
{
	pt->x = p->offx;
	pt->y = p->offy;
}

/** イメージ情報取得 */

void TileImage_getInfo(TileImage *p,TileImageInfo *info)
{
	info->tilew = p->tilew;
	info->tileh = p->tileh;
	info->offx  = p->offx;
	info->offy  = p->offy;
}

/** 現在描画可能な範囲を px で取得
 *
 * タイル全体とキャンバス範囲を含む。 */

void TileImage_getCanDrawRect_pixel(TileImage *p,mRect *rc)
{
	mRect rc1,rc2;

	TileImage_getAllTilesPixelRect(p, &rc1);

	rc2.x1 = rc2.y1 = 0;
	rc2.x2 = TILEIMGWORK->imgw - 1;
	rc2.y2 = TILEIMGWORK->imgh - 1;

	//範囲を合成

	mRectUnion(&rc1, &rc2);

	*rc = rc1;
}

/** box の px 範囲に相当するタイル範囲 (mRect) を取得
 *
 * rc:  タイル範囲が入る
 * box: イメージの範囲 (px)
 * return: FALSE で、全体がタイル範囲外 */

mlkbool TileImage_getTileRect_fromPixelBox(TileImage *p,mRect *rcdst,const mBox *box)
{
	int tx1,ty1,tx2,ty2,f;

	//box の四隅のタイル位置

	TileImage_pixel_to_tile_nojudge(p, box->x, box->y, &tx1, &ty1);
	TileImage_pixel_to_tile_nojudge(p, box->x + box->w - 1, box->y + box->h - 1, &tx2, &ty2);

	//各位置がタイル範囲外かどうかのフラグ

	f = (tx1 < 0) | ((tx2 < 0) << 1)
		| ((tx1 >= p->tilew) << 2) | ((tx2 >= p->tilew) << 3)
		| ((ty1 < 0) << 4) | ((ty2 < 0) << 5)
		| ((ty1 >= p->tileh) << 6) | ((ty2 >= p->tileh) << 7);

	//全体がタイル範囲外か

	if((f & 0x03) == 0x03 || (f & 0x30) == 0x30
		|| (f & 0x0c) == 0x0c || (f & 0xc0) == 0xc0)
		return FALSE;

	//タイル位置調整

	if(f & 1) tx1 = 0;
	if(f & 8) tx2 = p->tilew - 1;
	if(f & 0x10) ty1 = 0;
	if(f & 0x80) ty2 = p->tileh - 1;

	//セット

	rcdst->x1 = tx1, rcdst->y1 = ty1;
	rcdst->x2 = tx2, rcdst->y2 = ty2;

	return TRUE;
}

/** box の px 範囲内のタイルを処理する際の情報を取得
 *
 * return: タイルの開始位置のポインタ (NULL で範囲外) */

uint8_t **TileImage_getTileRectInfo(TileImage *p,TileImageTileRectInfo *info,const mBox *box)
{
	mRect rc;

	//タイル範囲

	if(!TileImage_getTileRect_fromPixelBox(p, &rc, box))
		return NULL;

	info->rctile = rc;

	//左上タイルの px 位置

	TileImage_tile_to_pixel(p, rc.x1, rc.y1, &info->pxtop.x, &info->pxtop.y);

	//box -> mRect (クリッピング用)

	info->rcclip.x1 = box->x;
	info->rcclip.y1 = box->y;
	info->rcclip.x2 = box->x + box->w;
	info->rcclip.y2 = box->y + box->h;

	//

	info->tilew = rc.x2 - rc.x1 + 1;
	info->tileh = rc.y2 - rc.y1 + 1;
	info->pitch_tile = p->tilew - info->tilew;

	//タイル位置

	return TILEIMAGE_GETTILE_BUFPT(p, rc.x1, rc.y1);
}

/** タイルが存在する部分の px 範囲取得
 *
 * 透明な部分は除く。イメージの範囲外部分も含む。
 *
 * rcdst: すべて透明の場合は、(0,0)-(-1,-1)となる。
 * tilenum: NULL 以外で、確保されているタイル数が入る。
 * return: FALSE で範囲なし */

mlkbool TileImage_getHaveImageRect_pixel(TileImage *p,mRect *rcdst,uint32_t *tilenum)
{
	uint8_t **pp;
	mRect rc;
	int x,y;
	uint32_t num = 0;

	mRectEmpty(&rc);

	//確保されているタイルの最小/最大位置

	if(p)
	{
		pp = p->ppbuf;

		for(y = 0; y < p->tileh; y++)
		{
			for(x = 0; x < p->tilew; x++, pp++)
			{
				if(*pp)
				{
					mRectIncPoint(&rc, x, y);
					num++;
				}
			}
		}
	}

	if(tilenum) *tilenum = num;

	//タイル位置 -> px

	if(mRectIsEmpty(&rc))
	{
		*rcdst = rc;
		return FALSE;
	}
	else
	{
		TileImage_tile_to_pixel(p, 0, 0, &x, &y);

		rcdst->x1 = x + (rc.x1 << 6);
		rcdst->y1 = y + (rc.y1 << 6);
		rcdst->x2 = x + (rc.x2 << 6) + 63;
		rcdst->y2 = y + (rc.y2 << 6) + 63;
		
		return TRUE;
	}
}

/** 確保されているタイルの数を取得 */

int TileImage_getHaveTileNum(TileImage *p)
{
	uint8_t **pp;
	int i,num = 0;

	pp = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pp++)
	{
		if(*pp) num++;
	}

	return num;
}

/** rc を描画可能な範囲内にクリッピング
 *
 * return: FALSE で全体が範囲外 */

mlkbool TileImage_clipCanDrawRect(TileImage *p,mRect *rc)
{
	mRect rc1;

	TileImage_getCanDrawRect_pixel(p, &rc1);

	//範囲外

	if(rc->x1 > rc1.x2 || rc->y1 > rc1.y2
		|| rc->x2 < rc1.x1 || rc->y2 < rc1.y1)
		return FALSE;

	//調整

	if(rc->x1 < rc1.x1) rc->x1 = rc1.x1;
	if(rc->y1 < rc1.y1) rc->y1 = rc1.y1;
	if(rc->x2 > rc1.x2) rc->x2 = rc1.x2;
	if(rc->y2 > rc1.y2) rc->y2 = rc1.y2;

	return TRUE;
}


//=============================
// いろいろ
//=============================


/* ImageCanvas 合成: トーン化の情報セット */

static void _blendcanvas_set_tone_info(TileImageBlendInfo *dst,const TileImageBlendSrcInfo *sinfo)
{
	int n;
	double dcell,dcos,dsin;

	//固定濃度 (0 でなし)

	if(sinfo->tone_density)
		dst->tone_density = __TileImage_density_to_colval(sinfo->tone_density, FALSE);

	//セル幅 (px)

	dcell = TILEIMGWORK->dpi / (sinfo->tone_lines * 0.1);

	//sin/cos
	
	n = sinfo->tone_angle * 512 / 360;

	dcos = TABLEDATA_GET_COS(n);
	dsin = TABLEDATA_GET_SIN(n);

	//1px進むごとに加算する値
	// :本来は dcos / dcell だが、モアレ防止の為、セル1周期がピクセル単位になるように調整。

	dst->tone_fcos = (int64_t)round(1.0 / round(dcell / dcos) * TILEIMG_TONE_FIX_VAL);
	dst->tone_fsin = (int64_t)round(1.0 / round(dcell / dsin) * TILEIMG_TONE_FIX_VAL);
}

/** ImageCanvas に合成
 *
 * - トーン化レイヤ対象外のカラータイプでは、トーン化は常に OFF に指定されていること。
 *
 * boxdst: キャンバスの描画範囲 */

void TileImage_blendToCanvas(TileImage *p,ImageCanvas *dst,
	const mBox *boxdst,const TileImageBlendSrcInfo *sinfo)
{
	TileImageTileRectInfo info;
	TileImageBlendInfo binfo;
	uint8_t **pptile;
	int ix,iy,px,py;
	int64_t fxx,fxy,fyx,fyy,fsin64,fcos64;
	TileImageColFunc_blendTile func;

	if(sinfo->opacity == 0) return;

	if(!(pptile = TileImage_getTileRectInfo(p, &info, boxdst)))
		return;

	func = TILEIMGWORK->colfunc[p->type].blend_tile;

	//[!] トーン化の値はゼロクリア

	memset(&binfo, 0, sizeof(TileImageBlendInfo));

	binfo.opacity = sinfo->opacity;
	binfo.imgtex = sinfo->img_texture;
	binfo.func_blend = TILEIMGWORK->blendfunc[sinfo->blendmode];
	binfo.tone_repcol = -1;

	//トーン化を行うか

	if(sinfo->tone_lines)
	{
		if(!TILEIMGWORK->is_tone_to_gray)
		{
			//通常のトーン表示

			binfo.is_tone = TRUE;
			binfo.is_tone_bkgnd_tp = !(sinfo->ftone_white);
		}
		else
		{
			//トーンをグレイスケール表示
			
			if(sinfo->tone_density)
			{
				//固定濃度ありの場合、濃度から色を取得
				binfo.tone_repcol = __TileImage_density_to_colval(sinfo->tone_density, TRUE);
			}
			else if(p->type == TILEIMAGE_COLTYPE_ALPHA1BIT)
			{
				//固定濃度なしの場合、GRAYタイプなら通常のグレイスケール表示。
				//A1タイプの場合、黒で固定。

				binfo.tone_repcol = 0;
			}
		}
	}

	//タイルごとに合成
	// :px,py = タイル左上の px 位置

	if(binfo.is_tone)
	{
		//----- トーン化

		//情報セット

		_blendcanvas_set_tone_info(&binfo, sinfo);

		//イメージ (0,0) 時点での初期位置
		// :位置によって微妙に形が変わるので、適当な値でずらす。

		fyx = (int64_t)(0.5 * TILEIMG_TONE_FIX_VAL);
		fyy = (int64_t)(0.7 * TILEIMG_TONE_FIX_VAL);

		//左上の位置 (先頭タイルの(0,0)位置)
		// :どのタイルから描画しても同じ位置になるようにする。

		fyx += info.pxtop.x * binfo.tone_fcos - info.pxtop.y * binfo.tone_fsin;
		fyy += info.pxtop.x * binfo.tone_fsin + info.pxtop.y * binfo.tone_fcos;

		fcos64 = binfo.tone_fcos << 6;
		fsin64 = binfo.tone_fsin << 6;

		//

		py = info.pxtop.y;

		for(iy = info.tileh; iy; iy--, py += 64)
		{
			fxx = fyx;
			fxy = fyy;
		
			for(ix = info.tilew, px = info.pxtop.x; ix; ix--, px += 64, pptile++)
			{
				if(*pptile)
				{
					__TileImage_setBlendInfo(&binfo, px, py, &info.rcclip);

					binfo.tile = *pptile;
					binfo.dstbuf = dst->ppbuf + binfo.dy;
					binfo.tone_fx = fxx;
					binfo.tone_fy = fxy;

					//タイル内の開始位置を加算

					if(binfo.sx || binfo.sy)
					{
						binfo.tone_fx += binfo.sx * binfo.tone_fcos - binfo.sy * binfo.tone_fsin;
						binfo.tone_fy += binfo.sx * binfo.tone_fsin + binfo.sy * binfo.tone_fcos;
					}

					(func)(p, &binfo);
				}

				fxx += fcos64;
				fxy += fsin64;
			}

			pptile += info.pitch_tile;

			fyx -= fsin64;
			fyy += fcos64;
		}
	}
	else
	{
		//----- 通常
		
		py = info.pxtop.y;

		for(iy = info.tileh; iy; iy--, py += 64)
		{
			for(ix = info.tilew, px = info.pxtop.x; ix; ix--, px += 64, pptile++)
			{
				if(!(*pptile)) continue;

				__TileImage_setBlendInfo(&binfo, px, py, &info.rcclip);

				binfo.tile = *pptile;
				binfo.dstbuf = dst->ppbuf + binfo.dy;

				(func)(p, &binfo);
			}

			pptile += info.pitch_tile;
		}
	}
}

/* A1 タイルを mPixbuf に XOR 合成 */

static void _blend_xor_pixbuf(mPixbuf *pixbuf,const TileImageBlendInfo *info)
{
	int ix,iy,pitchd,bpp;
	uint8_t *ps,*pd,*psY,f,fleft;
	mFuncPixbufSetBuf setbuf;

	psY = info->tile + (info->sy << 3) + (info->sx >> 3);

	pd = mPixbufGetBufPtFast(pixbuf, info->dx, info->dy);
	bpp = pixbuf->pixel_bytes;
	pitchd = pixbuf->line_bytes - info->w * bpp;

	mPixbufGetFunc_setbuf(pixbuf, &setbuf);

	fleft = 1 << (7 - (info->sx & 7));

	for(iy = info->h; iy; iy--)
	{
		ps = psY;
		f  = fleft;
	
		for(ix = info->w; ix; ix--, pd += bpp)
		{
			if(*ps & f)
				(setbuf)(pd, 0);
			
			f >>= 1;
			if(!f) f = 0x80, ps++;
		}

		psY += 8;
		pd += pitchd;
	}
}

/** A1 イメージを mPixbuf に XOR 合成 */

void TileImage_blendXor_pixbuf(TileImage *p,mPixbuf *pixbuf,mBox *boxdst)
{
	TileImageTileRectInfo info;
	TileImageBlendInfo binfo;
	uint8_t **pptile;
	int ix,iy,px,py;
	mBox box;

	if(!mPixbufClip_getBox(pixbuf, &box, boxdst)) return;

	//描画先に相当するタイル範囲

	if(!(pptile = TileImage_getTileRectInfo(p, &info, &box)))
		return;

	//タイル毎に合成

	for(iy = info.tileh, py = info.pxtop.y; iy; iy--, py += 64, pptile += info.pitch_tile)
	{
		for(ix = info.tilew, px = info.pxtop.x; ix; ix--, px += 64, pptile++)
		{
			if(*pptile)
			{
				__TileImage_setBlendInfo(&binfo, px, py, &info.rcclip);

				binfo.tile = *pptile;

				_blend_xor_pixbuf(pixbuf, &binfo);
			}
		}
	}
}

/** プレビュー画像を mPixbuf に描画
 *
 * レイヤ一覧用。背景はチェック柄。 */

void TileImage_drawPreview(TileImage *p,mPixbuf *pixbuf,
	int x,int y,int boxw,int boxh,int sw,int sh)
{
	uint8_t *pd;
	int ix,iy,pitchd,bpp,finc,finc2,fy,fx,fxleft,n,jx,jy,ff;
	int ytbl[3],xtbl[3],ckx,cky,rr,gg,bb,aa;
	uint32_t colex;
	double dscale,d;
	mBox box;
	RGB8 rgb8;
	mFuncPixbufSetBuf setpix;

	if(!mPixbufClip_getBox_d(pixbuf, &box, x, y, boxw, boxh))
		return;

	//範囲外の色
	colex = mRGBtoPix(0xb0b0b0);

	//倍率 (倍率の低い方。拡大はしない)

	dscale = (double)boxw / sw;
	d = (double)boxh / sh;

	if(d < dscale) dscale = d;
	if(d > 1.0) d = 1.0;

	//

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->pixel_bytes;
	pitchd = pixbuf->line_bytes - box.w * bpp;

	mPixbufGetFunc_setbuf(pixbuf, &setpix);

	x = box.x - x;
	y = box.y - y;

	finc = (int)((1<<16) / dscale + 0.5);
	finc2 = finc / 3;

	fxleft = (int)(((x - boxw * 0.5) / dscale + sw * 0.5) * (1<<16));
	fy = (int)(((y - boxh * 0.5) / dscale + sh * 0.5) * (1<<16));

	cky = y & 7;

	//

	for(iy = box.h; iy; iy--, fy += finc, cky = (cky + 1) & 7)
	{
		n = fy >> 16;

		//Y 範囲外
		
		if(n < 0 || n >= sh)
		{
			mPixbufBufLineH(pixbuf, pd, box.w, colex);
			pd += pixbuf->line_bytes;
			continue;
		}

		//Yテーブル

		for(jy = 0, ff = fy; jy < 3; jy++, ff += finc2)
		{
			n = ff >> 16;
			if(n >= sh) n = sh - 1;
			
			ytbl[jy] = n;
		}
	
		//----- X
	
		ckx = x & 7;

		for(ix = box.w, fx = fxleft; ix; ix--, fx += finc, pd += bpp, ckx = (ckx + 1) & 7)
		{
			n = fx >> 16;

			//X 範囲外

			if(n < 0 || n >= sw)
			{
				(setpix)(pd, colex);
				continue;
			}

			//Xテーブル

			for(jx = 0, ff = fx; jx < 3; jx++, ff += finc2)
			{
				n = ff >> 16;
				if(n >= sw) n = sw - 1;

				xtbl[jx] = n;
			}

			//オーバーサンプリング

			rr = gg = bb = aa = 0;

			for(jy = 0; jy < 3; jy++)
			{
				for(jx = 0; jx < 3; jx++)
				{
					TileImage_getPixel_oversamp(p, xtbl[jx], ytbl[jy],
						&rr, &gg, &bb, &aa);
				}
			}

			//チェック柄と合成

			TileImage_getColor_oversamp_blendPlaid(p, &rgb8,
				rr, gg, bb, aa, 9, (ckx >> 2) ^ (cky >> 2));

			(setpix)(pd, mRGBtoPix_sep(rgb8.r, rgb8.g, rgb8.b));
		}

		pd += pitchd;
	}
}

/** フィルタのダイアログプレビュー時の描画
 *
 * チェック柄背景にイメージを合成。
 *
 * pixbuf: 外側の 1px は枠として描画済み
 * box: イメージ座標 */

void TileImage_drawFilterPreview(TileImage *p,mPixbuf *pixbuf,const mBox *box)
{
	uint8_t *pd,*ps8;
	uint16_t *ps16;
	int ix,iy,pitch,bpp,bits,w,h,fck,fy,bkgnd,r,g,b,a;
	uint64_t col;
	uint16_t bkcol16[2] = {28784,24672};
	uint8_t bkcol8[2] = {224,192};
	mFuncPixbufSetBuf setbuf;

	bits = TILEIMGWORK->bits;

	if(bits == 8)
		ps8 = (uint8_t *)&col;
	else
		ps16 = (uint16_t *)&col;

	w = box->w;
	h = box->h;

	pd = mPixbufGetBufPtFast(pixbuf, 1, 1);
	bpp = pixbuf->pixel_bytes;
	pitch = pixbuf->line_bytes - bpp * w;

	mPixbufGetFunc_setbuf(pixbuf, &setbuf);

	//

	for(iy = 0; iy < h; iy++, pd += pitch)
	{
		fy = (iy >> 3) & 1;
		
		for(ix = 0; ix < w; ix++, pd += bpp)
		{
			TileImage_getPixel(p, box->x + ix, box->y + iy, &col);

			fck = fy ^ ((ix >> 3) & 1);

			if(bits == 8)
			{
				//8bit
				
				bkgnd = bkcol8[fck];
				a = ps8[3];

				if(!a)
					r = g = b = bkgnd;
				else
				{
					r = ps8[0];
					g = ps8[1];
					b = ps8[2];

					if(a != 255)
					{
						r = ((r - bkgnd) * a / 255) + bkgnd;
						g = ((g - bkgnd) * a / 255) + bkgnd;
						b = ((b - bkgnd) * a / 255) + bkgnd;
					}
				}
				
			}
			else
			{
				//16bit
				
				a = ps16[3];

				if(!a)
					r = g = b = bkcol8[fck];
				else
				{
					r = ps16[0];
					g = ps16[1];
					b = ps16[2];

					if(a != 0x8000)
					{
						bkgnd = bkcol16[fck];

						r = ((r - bkgnd) * a >> 15) + bkgnd;
						g = ((g - bkgnd) * a >> 15) + bkgnd;
						b = ((b - bkgnd) * a >> 15) + bkgnd;
					}

					r = r * 255 >> 15;
					g = g * 255 >> 15;
					b = b * 255 >> 15;
				}
			}

			(setbuf)(pd, mRGBtoPix_sep(r, g, b));
		}
	}
}

/** ヒストグラム取得
 *
 * 256個 x 4byte。
 * RGBA または GRAY タイプのみ。 */

uint32_t *TileImage_getHistogram(TileImage *p)
{
	uint32_t *buf;
	uint8_t **pptile,*ps8;
	uint16_t *ps16;
	int bits,n;
	uint32_t i,j;
	mlkbool is_rgba;

	buf = (uint32_t *)mMalloc0(256 * 4);
	if(!buf) return NULL;

	bits = TILEIMGWORK->bits;
	is_rgba = (p->type == TILEIMAGE_COLTYPE_RGBA);

	//タイルごと

	pptile = p->ppbuf;

	for(i = p->tilew * p->tileh; i; i--, pptile++)
	{
		if(!(*pptile)) continue;

		if(is_rgba)
		{
			//RGBA

			if(bits == 8)
			{
				ps8 = *pptile;

				for(j = 64 * 64; j; j--, ps8 += 4)
				{
					if(ps8[3])
					{
						n = CONV_RGB_TO_LUM(ps8[0], ps8[1], ps8[2]);
						buf[n]++;
					}
				}
			}
			else
			{
				ps16 = (uint16_t *)*pptile;

				for(j = 64 * 64; j; j--, ps16 += 4)
				{
					if(ps16[3])
					{
						n = CONV_RGB_TO_LUM(ps16[0], ps16[1], ps16[2]);
						buf[n * 255 >> 15]++;
					}
				}
			}
		}
		else
		{
			//GRAY

			if(bits == 8)
			{
				ps8 = *pptile;

				for(j = 64 * 64; j; j--, ps8 += 2)
				{
					if(ps8[1])
						buf[ps8[0]]++;
				}
			}
			else
			{
				ps16 = (uint16_t *)*pptile;

				for(j = 64 * 64; j; j--, ps16 += 2)
				{
					if(ps16[1])
						buf[ps16[0] * 255 >> 15]++;
				}
			}
		}
	}

	return buf;
}

