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
 * UndoItem
 *
 * タイルイメージ
 *****************************************/

#include <stdio.h>	//FILE*
#include <string.h>

#include "mlk.h"
#include "mlk_undo.h"

#include "def_draw.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "layeritem.h"

#include "undo.h"
#include "undoitem.h"
#include "pv_undo.h"


//--------------------

#define _FLAGS_EMPTY		1	//空タイル
#define _FLAGS_FIRST_EMPTY  2	//最初の書き込み時、元が空タイル

//--------------------

typedef mlkerr (*write_tile_func)(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
typedef mlkerr (*revwrite_tile_func)(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
typedef mlkerr (*restore_tile_func)(UndoItem *p,uint8_t *tile);

//A1bit
static mlkerr _write_tile_a1(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_a1(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_a1(UndoItem *p,uint8_t *tile);

//8bit:A
static mlkerr _write_tile_8bit_a(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_8bit_a(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_8bit_a(UndoItem *p,uint8_t *tile);

//16bit:A
static mlkerr _write_tile_16bit_a(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_16bit_a(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_16bit_a(UndoItem *p,uint8_t *tile);

//8bit:GRAY
static mlkerr _write_tile_8bit_gray(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_8bit_gray(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_8bit_gray(UndoItem *p,uint8_t *tile);

//16bit:GRAY
static mlkerr _write_tile_16bit_gray(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_16bit_gray(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_16bit_gray(UndoItem *p,uint8_t *tile);

//8bit:RGBA
static mlkerr _write_tile_8bit_rgba(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_8bit_rgba(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_8bit_rgba(UndoItem *p,uint8_t *tile);

//16bit:RGBA
static mlkerr _write_tile_16bit_rgba(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first);
static mlkerr _revwrite_tile_16bit_rgba(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty);
static mlkerr _restore_tile_16bit_rgba(UndoItem *p,uint8_t *tile);

//--------------------

//書き込み
static write_tile_func g_write_tile_funcs[2][4] = {
	{_write_tile_8bit_rgba, _write_tile_8bit_gray, _write_tile_8bit_a, _write_tile_a1},
	{_write_tile_16bit_rgba, _write_tile_16bit_gray, _write_tile_16bit_a, _write_tile_a1}
};

//逆書き込み
static revwrite_tile_func g_revwrite_tile_funcs[2][4] = {
	{_revwrite_tile_8bit_rgba, _revwrite_tile_8bit_gray, _revwrite_tile_8bit_a, _revwrite_tile_a1},
	{_revwrite_tile_16bit_rgba, _revwrite_tile_16bit_gray, _revwrite_tile_16bit_a, _revwrite_tile_a1}
};

//復元
static restore_tile_func g_restore_tile_funcs[2][4] = {
	{_restore_tile_8bit_rgba, _restore_tile_8bit_gray, _restore_tile_8bit_a, _restore_tile_a1},
	{_restore_tile_16bit_rgba, _restore_tile_16bit_gray, _restore_tile_16bit_a, _restore_tile_a1}
};

//--------------------

int undo_encode8(uint8_t *dst,uint8_t *src,int size);
void undo_decode8(uint8_t *dst,uint8_t *src,int size);
int undo_encode16(uint8_t *dst,uint8_t *src,int size);
void undo_decode16(uint8_t *dst,uint8_t *src,int size);

//--------------------


//==================================
// sub
//==================================


/* 対象のレイヤイメージ取得 */

static TileImage *_get_layerimg(UndoItem *p)
{
	LayerItem *item;

	item = UndoItem_getLayerAtIndex(p->val[0]);

	return (item)? item->img: NULL;
}


//==================================
// 書き込み・復元
//==================================
/*
 * [TileImageInfo] 描画前のイメージ情報
 * [タイルデータ]
 *   uint16 x 2: タイル位置。両方 0xffff で終了。
 *   1byte: フラグ (空タイルか)
 *   <各カラータイプ別のデータ>
 */


/** アンドゥ時、最初の書き込み
 *
 * info: 描画前のイメージ情報 */

mlkerr UndoItem_setdat_tileimage(UndoItem *p,TileImageInfo *info)
{
	TileImage *imgsrc,*imgdst;
	uint16_t tpos[2];
	int tw,th,ix,iy,bittype;
	uint8_t **pptile,*tiledst,flags;
	mlkerr ret;

	bittype = (APPDRAW->imgbits == 16);

	//イメージ

	imgsrc = APPDRAW->tileimg_tmp_save;
	imgdst = APPDRAW->curlayer->img;

	//書き込み開始

	ret = UndoItem_allocOpenWrite_variable(p);
	if(ret) return ret;
	
	//イメージ情報

	ret = UndoItem_write(p, info, sizeof(TileImageInfo));
	if(ret) goto ERR;

	//タイルデータ

	pptile = imgsrc->ppbuf;
	tw = imgsrc->tilew;
	th = imgsrc->tileh;

	for(iy = 0; iy < th; iy++)
	{
		for(ix = 0; ix < tw; ix++, pptile++)
		{
			if(!(*pptile)) continue;

			//タイル位置

			tpos[0] = ix;
			tpos[1] = iy;

			ret = UndoItem_write(p, tpos, 4);
			if(ret) goto ERR;

			//フラグ

			flags = (*pptile == TILEIMAGE_TILE_EMPTY)? _FLAGS_EMPTY | _FLAGS_FIRST_EMPTY: 0;

			ret = UndoItem_write(p, &flags, 1);
			if(ret) goto ERR;

			//タイル (空の場合はなし)
			// :保存用イメージと描画イメージのタイル配列構成は同じのため、
			// :同じタイル位置で取得可能。

			if(!(flags & _FLAGS_EMPTY))
			{
				tiledst = TILEIMAGE_GETTILE_PT(imgdst, ix, iy);

				ret = (g_write_tile_funcs[bittype][imgsrc->type])(p, *pptile, tiledst, TRUE);
				if(ret) goto ERR;
			}
		}
	}

	//タイル終了

	tpos[0] = tpos[1] = 0xffff;

	ret = UndoItem_write(p, tpos, 4);

	//可変書き込み時は、閉じる際にメモリを確保する場合がある
ERR:
	return UndoItem_closeWrite_variable(p, ret);
}

/** 反転イメージを書き込み */

mlkerr UndoItem_setdat_tileimage_reverse(UndoItem *dst,UndoItem *src,int settype)
{
	TileImage *img;
	TileImageInfo info;
	int toffx,toffy,bittype;
	uint16_t tpos[2];
	uint8_t *ptile,flags_src,flags_dst;
	mlkerr ret;

	bittype = (APPDRAW->imgbits == 16);

	//対象イメージ

	img = _get_layerimg(dst);
	if(!img) return MLKERR_INVALID_VALUE;

	//--------- 書き込み

	//src 開く

	ret = UndoItem_openRead(src);
	if(ret) return ret;

	ret = UndoItem_read(src, &info, sizeof(TileImageInfo));
	if(ret) goto ERR;

	//REDO->UNDO 時のタイル位置調整

	toffx = (info.offx - img->offx) >> 6;
	toffy = (info.offy - img->offy) >> 6;

	//書き込み開始

	ret = UndoItem_allocOpenWrite_variable(dst);
	if(ret) goto ERR;
	
	//イメージ情報書き込み

	TileImage_getInfo(img, &info);

	ret = UndoItem_write(dst, &info, sizeof(TileImageInfo));
	if(ret) goto ERR;

	//----- タイル

	while(1)
	{
		//src タイル位置取得

		ret = UndoItem_read(src, tpos, 4);
		if(ret) goto ERR;

		//タイル終了

		if(tpos[0] == 0xffff && tpos[1] == 0xffff)
			break;
	
		//src フラグ読み込み

		ret = UndoItem_read(src, &flags_src, 1);
		if(ret) goto ERR;

		//保存対象のタイルを取得

		if(settype == MUNDO_TYPE_REDO)
		{
			//src:UNDO -> dst:REDO
			//現在のタイル状態を保存。

			ptile = TILEIMAGE_GETTILE_PT(img, tpos[0], tpos[1]);
		}
		else
		{
			//src:REDO -> dst:UNDO

			if(flags_src & _FLAGS_FIRST_EMPTY)
				ptile = NULL;
			else
				ptile = TILEIMAGE_GETTILE_PT(img, tpos[0] + toffx, tpos[1] + toffy);
		}

		//フラグ

		flags_dst = flags_src & _FLAGS_FIRST_EMPTY;

		if(!ptile)
			flags_dst |= _FLAGS_EMPTY;

		//タイル位置 + フラグ書き込み

		ret = UndoItem_write(dst, tpos, 4);
		if(ret) goto ERR;
		
		ret = UndoItem_write(dst, &flags_dst, 1);
		if(ret) goto ERR;

		//タイルデータ
		// :src のデータをスキップする。
		// :ptile == NULL の場合は書き込まない。

		ret = (g_revwrite_tile_funcs[bittype][img->type])(src, dst, ptile, !(flags_src & _FLAGS_EMPTY));
		if(ret) goto ERR;
	}

	//タイル終了

	tpos[0] = tpos[1] = 0xffff;

	ret = UndoItem_write(dst, tpos, 4);

	//

ERR:
	UndoItem_closeRead(src);

	return UndoItem_closeWrite_variable(dst, ret);
}

/** タイルイメージ復元 */

mlkerr UndoItem_restore_tileimage(UndoItem *p,int runtype)
{
	TileImage *img;
	TileImageInfo info;
	uint16_t tpos[2];
	int bittype;
	uint8_t *ptile,flags;
	mlkerr ret;

	bittype = (APPDRAW->imgbits == 16);

	//対象イメージ

	img = _get_layerimg(p);
	if(!img) return MLKERR_INVALID_VALUE;

	//読み込み開始

	ret = UndoItem_openRead(p);
	if(ret) return ret;

	//イメージ情報

	ret = UndoItem_read(p, &info, sizeof(TileImageInfo));
	if(ret) goto ERR;

	//配列リサイズ (リドゥ時)

	if(runtype == MUNDO_TYPE_REDO
		&& !TileImage_resizeTileBuf_forUndo(img, &info))
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//タイル

	while(1)
	{
		//タイル位置

		ret = UndoItem_read(p, tpos, 4);
		if(ret) goto ERR;

		//タイル終了

		if(tpos[0] == 0xffff && tpos[1] == 0xffff)
			break;
	
		//フラグ

		ret = UndoItem_read(p, &flags, 1);
		if(ret) goto ERR;

		//タイル復元

		if(flags & _FLAGS_EMPTY)
		{
			//空タイルにする

			TileImage_freeTile_atPos(img, tpos[0], tpos[1]);
		}
		else
		{
			//読み込み

			ptile = TileImage_getTileAlloc_atpos(img, tpos[0], tpos[1], TRUE);
			if(!ptile) goto ERR;

			ret = (g_restore_tile_funcs[bittype][img->type])(p, ptile);
			if(ret) goto ERR;
		}
	}

	ret = MLKERR_OK;

	//配列リサイズ (アンドゥ時)

	if(runtype == MUNDO_TYPE_UNDO
		&& !TileImage_resizeTileBuf_forUndo(img, &info))
		ret = MLKERR_ALLOC;

	//

ERR:
	UndoItem_closeRead(p);

	return ret;
}


/*************************************
 * タイルデータ処理
 *************************************/


//============================
// sub
//============================


/* 8bit 圧縮
 *
 * return: 圧縮後のサイズ (無圧縮時は、size) */

static uint16_t _encode_8bit(uint8_t *dst,uint8_t *src,int size)
{
	int ret;

	ret = undo_encode8(dst, src, size);

	if(ret == -1)
	{
		//無圧縮
		memcpy(dst, src, size);
		return size;
	}
	else
		return ret;
}

/* 8bit 展開 */

static void _decode_8bit(uint8_t *dst,uint8_t *src,int encsize,int rawsize)
{
	if(encsize == rawsize)
		memcpy(dst, src, rawsize);
	else
		undo_decode8(dst, src, encsize);
}

/* データを読み込んで展開 (8bit) */

static mlkerr _read_decode_8bit(UndoItem *p,uint8_t *dst,int encsize,int rawsize,uint8_t *workbuf)
{
	mlkerr ret;

	if(encsize == rawsize)
	{
		//無圧縮

		return UndoItem_read(p, dst, rawsize);
	}
	else
	{
		//展開

		ret = UndoItem_read(p, workbuf, encsize);
		if(ret) return ret;

		undo_decode8(dst, workbuf, encsize);

		return MLKERR_OK;
	}
}

/* 16bit 圧縮
 *
 * return: 圧縮後のサイズ (無圧縮時は、size) */

static uint16_t _encode_16bit(uint8_t *dst,uint8_t *src,int size)
{
	int ret;

	ret = undo_encode16(dst, src, size);

	if(ret == -1)
	{
		//無圧縮
		memcpy(dst, src, size);
		return size;
	}
	else
		return ret;
}

/* 16bit 展開 */

static void _decode_16bit(uint8_t *dst,uint8_t *src,int encsize,int rawsize)
{
	if(encsize == rawsize)
		memcpy(dst, src, rawsize);
	else
		undo_decode16(dst, src, encsize);
}


//============================
// A1bit
//============================


/* (A1bit) タイルデータを書き込み */

mlkerr _write_tile_a1(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint16_t encsize;
	mlkerr ret;

	//圧縮

	encsize = _encode_8bit(APPUNDO->workbuf1, tilesrc, 64 * 64 / 8);

	//圧縮サイズ

	ret = UndoItem_write(p, &encsize, 2);
	if(ret) return ret;

	//タイルデータ

	return UndoItem_write(p, APPUNDO->workbuf1, encsize);
}

/* (A1bit) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_a1(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t encsize;
	mlkerr ret;

	//src タイルデータスキップ

	if(isnot_empty)
	{
		ret = UndoItem_read(src, &encsize, 2);
		if(ret) return ret;

		UndoItem_readSeek(src, encsize);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_a1(dst, tilesrc, NULL, FALSE);
}

/* (A1bit) タイルデータ復元 */

mlkerr _restore_tile_a1(UndoItem *p,uint8_t *tile)
{
	uint16_t encsize;
	mlkerr ret;

	//圧縮サイズ

	ret = UndoItem_read(p, &encsize, 2);
	if(ret) return ret;

	//タイルデータ

	return _read_decode_8bit(p, tile, encsize, 64 * 64 / 8, APPUNDO->workbuf1);
}


//============================
// 8bit:A
//============================
/*
  [uint16 x 3]
    0: フラグ圧縮後サイズ
    1: Aデータ圧縮前サイズ
    2: Aデータ圧縮後サイズ
  [フラグ圧縮データ]
    (64x64x1bit = 512 byte)
    0:変化なし、1:変化あり
  [Aデータ圧縮データ]
    変化ありの点のみ
*/

/* データをセット
 *
 * return: Aデータサイズ */

static int _8bit_a_set_data(uint8_t *dstbuf,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	int i,j;
	uint8_t fval,f,srca,dsta,revval;
	uint8_t *pd_flags,*pd_a;

	pd_flags = dstbuf;
	pd_a = dstbuf + 512;

	for(i = 64 * 64 / 8; i; i--)
	{
		fval = 0;
		f = 0x80;

		if(!is_first && tiledst)
			revval = *(tiledst++);
		
		for(j = 8; j; j--, f >>= 1)
		{
			srca = *(tilesrc++);
		
			//比較する値
			
			if(!tiledst)
				dsta = 0;
			else if(is_first)
				dsta = *(tiledst++);
			else
			{
				//逆書き込み時: tiledst はフラグのバッファ

				if(revval & f)
					dsta = !srca; //変化あり
				else
					dsta = srca; //変化なし
			}

			//変化あり

			if(srca != dsta)
			{
				fval |= f;

				*(pd_a++) = srca;
			}
		}

		*(pd_flags++) = fval;
	}

	return pd_a - (dstbuf + 512);
}

/* (8bit:A) データからタイル復元 */

static void _8bit_a_data_to_tile(uint8_t *dst,uint8_t *src)
{
	uint8_t *ps_flags,*ps_a;
	int i,j;
	uint8_t f,fval;

	ps_flags = src;
	ps_a = src + 512;

	for(i = 64 * 64 / 8; i; i--)
	{
		fval = *(ps_flags++);
		f = 0x80;
		
		for(j = 8; j; j--, f >>= 1)
		{
			if(fval & f)
				*dst = *(ps_a++);

			dst++;
		}
	}
}

/* (8bit:A) タイルデータを書き込み */

mlkerr _write_tile_8bit_a(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint8_t *datbuf,*encbuf;
	uint16_t val[3];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//データセット

	val[1] = _8bit_a_set_data(datbuf, tilesrc, tiledst, is_first);

	//圧縮

	val[0] = _encode_8bit(encbuf, datbuf, 512);
	val[2] = _encode_8bit(encbuf + val[0], datbuf + 512, val[1]);

	//書き込み

	ret = UndoItem_write(p, val, 2 * 3);
	if(ret) return ret;

	return UndoItem_write(p, encbuf, val[0] + val[2]);
}

/* (8bit:A) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_8bit_a(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t val[3];
	mlkerr ret;

	//src タイルデータ (フラグのみ読み込む)

	if(isnot_empty)
	{
		ret = UndoItem_read(src, val, 2 * 3);
		if(ret) return ret;

		ret = _read_decode_8bit(src, APPUNDO->workbuf2, val[0], 512, APPUNDO->workbuf1);
		if(ret) return ret;

		UndoItem_readSeek(src, val[2]);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_8bit_a(dst, tilesrc, (isnot_empty)? APPUNDO->workbuf2: NULL, FALSE);
}

/* (8bit:A) タイルデータ復元 */

mlkerr _restore_tile_8bit_a(UndoItem *p,uint8_t *tile)
{
	uint8_t *encbuf,*datbuf;
	uint16_t val[3];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//読み込み

	ret = UndoItem_read(p, val, 2 * 3);
	if(ret) return ret;

	ret = UndoItem_read(p, encbuf, val[0] + val[2]);
	if(ret) return ret;

	//展開

	_decode_8bit(datbuf, encbuf, val[0], 512);
	_decode_8bit(datbuf + 512, encbuf + val[0], val[2], val[1]);

	//復元

	_8bit_a_data_to_tile(tile, datbuf);

	return MLKERR_OK;
}


//============================
// 16bit:A
//============================
/*
  [uint16 x 3]
    0: フラグ圧縮後サイズ
    1: Aデータ圧縮前サイズ
    2: Aデータ圧縮後サイズ
  [フラグ圧縮データ]
    (64x64x1bit = 512 byte)
    0:変化なし、1:変化あり
  [Aデータ圧縮データ]
    変化ありの点のみ
*/

/* データをセット
 *
 * return: Aデータサイズ */

static int _16bit_a_set_data(uint8_t *dstbuf,uint16_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	int i,j;
	uint8_t fval,f,revval;
	uint8_t *pd_flags;
	uint16_t *pd_a,srca,dsta;

	pd_flags = dstbuf;
	pd_a = (uint16_t *)(dstbuf + 512);

	for(i = 64 * 64 / 8; i; i--)
	{
		fval = 0;
		f = 0x80;

		if(!is_first && tiledst)
			revval = *(tiledst++);
		
		for(j = 8; j; j--, f >>= 1)
		{
			srca = *(tilesrc++);
		
			//比較する値
			
			if(!tiledst)
				dsta = 0;
			else if(is_first)
			{
				dsta = *((uint16_t *)tiledst);
				tiledst += 2;
			}
			else
			{
				//逆書き込み時: tiledst はフラグのバッファ

				if(revval & f)
					dsta = !srca; //変化あり
				else
					dsta = srca; //変化なし
			}

			//変化あり

			if(srca != dsta)
			{
				fval |= f;

				*(pd_a++) = srca;
			}
		}

		*(pd_flags++) = fval;
	}

	return (uint8_t *)pd_a - (dstbuf + 512);
}

/* (16bit:A) データからタイル復元 */

static void _16bit_a_data_to_tile(uint16_t *dst,uint8_t *src)
{
	uint8_t *ps_flags,f,fval;
	uint16_t *ps_a;
	int i,j;

	ps_flags = src;
	ps_a = (uint16_t *)(src + 512);

	for(i = 64 * 64 / 8; i; i--)
	{
		fval = *(ps_flags++);
		f = 0x80;
		
		for(j = 8; j; j--, f >>= 1)
		{
			if(fval & f)
				*dst = *(ps_a++);

			dst++;
		}
	}
}

/* (16bit:A) タイルデータを書き込み */

mlkerr _write_tile_16bit_a(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint8_t *datbuf,*encbuf;
	uint16_t val[3];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//データセット

	val[1] = _16bit_a_set_data(datbuf, (uint16_t *)tilesrc, tiledst, is_first);

	//圧縮

	val[0] = _encode_8bit(encbuf, datbuf, 512);
	val[2] = _encode_16bit(encbuf + val[0], datbuf + 512, val[1]);

	//書き込み

	ret = UndoItem_write(p, val, 2 * 3);
	if(ret) return ret;

	return UndoItem_write(p, encbuf, val[0] + val[2]);
}

/* (16bit:A) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_16bit_a(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t val[3];
	mlkerr ret;

	//src タイルデータ (フラグのみ読み込む)

	if(isnot_empty)
	{
		ret = UndoItem_read(src, val, 2 * 3);
		if(ret) return ret;

		ret = _read_decode_8bit(src, APPUNDO->workbuf2, val[0], 512, APPUNDO->workbuf1);
		if(ret) return ret;

		UndoItem_readSeek(src, val[2]);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_16bit_a(dst, tilesrc, (isnot_empty)? APPUNDO->workbuf2: NULL, FALSE);
}

/* (16bit:A) タイルデータ復元 */

mlkerr _restore_tile_16bit_a(UndoItem *p,uint8_t *tile)
{
	uint8_t *encbuf,*datbuf;
	uint16_t val[3];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//読み込み

	ret = UndoItem_read(p, val, 2 * 3);
	if(ret) return ret;

	ret = UndoItem_read(p, encbuf, val[0] + val[2]);
	if(ret) return ret;

	//展開

	_decode_8bit(datbuf, encbuf, val[0], 512);
	_decode_16bit(datbuf + 512, encbuf + val[0], val[2], val[1]);

	//復元

	_16bit_a_data_to_tile((uint16_t *)tile, datbuf);

	return MLKERR_OK;
}


//============================
// 8bit:GRAY
//============================
/*
  [uint16 x 4]
    0: GRAY 色データサイズ
    1: 色データ圧縮前サイズ
    2: フラグ圧縮後サイズ
    3: 色データ圧縮後サイズ
  [フラグ圧縮データ]
    (1bit:512byte) 色の変化フラグ
    (1bit:512byte) Aの変化フラグ
  [変化色圧縮データ]
    GRAY 色、Aの順で各チャンネル別に
*/

/* (8bit:GRAY) データをセット
 *
 * dstval: [0] 色(GRAY)データサイズ [1] 全体の色データサイズ */

static void _8bit_gray_set_data(uint8_t *dstbuf,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first,uint16_t *dstval)
{
	int i,j;
	uint8_t *pd_flags_col,*pd_flags_a,*pd_col,*pd_a,*prev_col,*prev_a;
	uint8_t fval_col,fval_a,f,srcc,dstc,srca,dsta,rev_col,rev_a;

	pd_flags_col = dstbuf;
	pd_flags_a = dstbuf + 512;
	pd_col = dstbuf + 1024;
	pd_a = pd_col + 64 * 64;

	if(!is_first && tiledst)
	{
		prev_col = tiledst;
		prev_a = tiledst + 512;
	}

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = fval_a = 0;
		f = 0x80;

		if(!is_first && tiledst)
		{
			rev_col = *(prev_col++);
			rev_a = *(prev_a++);
		}
		
		for(j = 8; j; j--, f >>= 1)
		{
			srcc = tilesrc[0];
			srca = tilesrc[1];
			tilesrc += 2;
		
			//比較する値
			
			if(!tiledst)
				dstc = dsta = 0;
			else if(is_first)
			{
				dstc = tiledst[0];
				dsta = tiledst[1];
				tiledst += 2;
			}
			else
			{
				//逆書き込み時: tiledst はフラグのバッファ

				dstc = (rev_col & f)? !srcc: srcc;
				dsta = (rev_a & f)? !srca: srca;
			}

			//色変化

			if(srcc != dstc)
			{
				fval_col |= f;
				*(pd_col++) = srcc;
			}

			//A変化

			if(srca != dsta)
			{
				fval_a |= f;
				*(pd_a++) = srca;
			}
		}

		*(pd_flags_col++) = fval_col;
		*(pd_flags_a++) = fval_a;
	}

	//Aデータを移動

	i = pd_col - (dstbuf + 1024);
	j = pd_a - (dstbuf + 1024 + 64 * 64);

	if(i != 64 * 64)
		memmove(dstbuf + 1024 + i, dstbuf + 1024 + 64 * 64, j);

	//

	dstval[0] = i;
	dstval[1] = i + j;
}

/* (8bit:GRAY) データからタイル復元 */

static void _8bit_gray_data_to_tile(uint8_t *dst,uint8_t *src,int colsize)
{
	uint8_t *ps_flags_col,*ps_flags_a,*ps_col,*ps_a;
	int i,j;
	uint8_t f,fval_col,fval_a;

	ps_flags_col = src;
	ps_flags_a = src + 512;
	ps_col = src + 1024;
	ps_a = src + 1024 + colsize;

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = *(ps_flags_col++);
		fval_a = *(ps_flags_a++);
		f = 0x80;
		
		for(j = 8; j; j--, f >>= 1)
		{
			if(fval_col & f)
				dst[0] = *(ps_col++);

			if(fval_a & f)
				dst[1] = *(ps_a++);

			dst += 2;
		}
	}
}

/* (8bit:GRAY) タイルデータを書き込み */

mlkerr _write_tile_8bit_gray(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint8_t *datbuf,*encbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//データセット

	_8bit_gray_set_data(datbuf, tilesrc, tiledst, is_first, val);

	//圧縮

	val[2] = _encode_8bit(encbuf, datbuf, 1024);
	val[3] = _encode_8bit(encbuf + val[2], datbuf + 1024, val[1]);

	//書き込み

	ret = UndoItem_write(p, val, 2 * 4);
	if(ret) return ret;

	return UndoItem_write(p, encbuf, val[2] + val[3]);
}

/* (8bit:GRAY) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_8bit_gray(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t val[4];
	mlkerr ret;

	//src タイルデータ (フラグのみ読み込む)

	if(isnot_empty)
	{
		ret = UndoItem_read(src, val, 2 * 4);
		if(ret) return ret;

		ret = _read_decode_8bit(src, APPUNDO->workbuf2, val[2], 1024, APPUNDO->workbuf1);
		if(ret) return ret;

		UndoItem_readSeek(src, val[3]);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_8bit_gray(dst, tilesrc, (isnot_empty)? APPUNDO->workbuf2: NULL, FALSE);
}

/* (8bit:GRAY) タイルデータ復元 */

mlkerr _restore_tile_8bit_gray(UndoItem *p,uint8_t *tile)
{
	uint8_t *encbuf,*datbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//読み込み

	ret = UndoItem_read(p, val, 2 * 4);
	if(ret) return ret;

	ret = UndoItem_read(p, encbuf, val[2] + val[3]);
	if(ret) return ret;

	//展開

	_decode_8bit(datbuf, encbuf, val[2], 1024);
	_decode_8bit(datbuf + 1024, encbuf + val[2], val[3], val[1]);

	//復元

	_8bit_gray_data_to_tile(tile, datbuf, val[0]);

	return MLKERR_OK;
}


//============================
// 16bit:GRAY
//============================
/*
  [uint16 x 4]
    0: GRAY 色データサイズ
    1: 色データ圧縮前サイズ
    2: フラグ圧縮後サイズ
    3: 色データ圧縮後サイズ
  [フラグ圧縮データ]
    (1bit:512byte) 色の変化フラグ
    (1bit:512byte) Aの変化フラグ
  [変化色圧縮データ]
    GRAY 色、Aの順で各チャンネル別に
*/

/* (16bit:GRAY) データをセット
 *
 * dstval: [0] 色(GRAY)データサイズ [1] 全体の色データサイズ */

static void _16bit_gray_set_data(uint8_t *dstbuf,uint16_t *tilesrc,uint8_t *tiledst,mlkbool is_first,uint16_t *dstval)
{
	int i,j;
	uint8_t *pd_flags_col,*pd_flags_a,*prev_col,*prev_a;
	uint16_t *pd_col,*pd_a,srcc,dstc,srca,dsta;
	uint8_t fval_col,fval_a,f,rev_col,rev_a;

	pd_flags_col = dstbuf;
	pd_flags_a = dstbuf + 512;
	pd_col = (uint16_t *)(dstbuf + 1024);
	pd_a = (uint16_t *)(dstbuf + 1024 + 64 * 64 * 2);

	if(!is_first && tiledst)
	{
		prev_col = tiledst;
		prev_a = tiledst + 512;
	}

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = fval_a = 0;
		f = 0x80;

		if(!is_first && tiledst)
		{
			rev_col = *(prev_col++);
			rev_a = *(prev_a++);
		}
		
		for(j = 8; j; j--, f >>= 1)
		{
			srcc = tilesrc[0];
			srca = tilesrc[1];
			tilesrc += 2;
		
			//比較する値
			
			if(!tiledst)
				dstc = dsta = 0;
			else if(is_first)
			{
				dstc = *((uint16_t *)tiledst);
				dsta = *((uint16_t *)tiledst + 1);
				tiledst += 4;
			}
			else
			{
				//逆書き込み時: tiledst はフラグのバッファ

				dstc = (rev_col & f)? !srcc: srcc;
				dsta = (rev_a & f)? !srca: srca;
			}

			//色変化

			if(srcc != dstc)
			{
				fval_col |= f;
				*(pd_col++) = srcc;
			}

			//A変化

			if(srca != dsta)
			{
				fval_a |= f;
				*(pd_a++) = srca;
			}
		}

		*(pd_flags_col++) = fval_col;
		*(pd_flags_a++) = fval_a;
	}

	//Aデータを移動

	i = (uint8_t *)pd_col - (dstbuf + 1024);
	j = (uint8_t *)pd_a - (dstbuf + 1024 + 64 * 64 * 2);

	if(i != 64 * 64 * 2)
		memmove(dstbuf + 1024 + i, dstbuf + 1024 + 64 * 64 * 2, j);

	//

	dstval[0] = i;
	dstval[1] = i + j;
}

/* (16bit:GRAY) データからタイル復元 */

static void _16bit_gray_data_to_tile(uint16_t *dst,uint8_t *src,int colsize)
{
	uint8_t *ps_flags_col,*ps_flags_a;
	uint16_t *ps_col,*ps_a;
	int i,j;
	uint8_t f,fval_col,fval_a;

	ps_flags_col = src;
	ps_flags_a = src + 512;
	ps_col = (uint16_t *)(src + 1024);
	ps_a = (uint16_t *)(src + 1024 + colsize);

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = *(ps_flags_col++);
		fval_a = *(ps_flags_a++);
		f = 0x80;
		
		for(j = 8; j; j--, f >>= 1)
		{
			if(fval_col & f)
				dst[0] = *(ps_col++);

			if(fval_a & f)
				dst[1] = *(ps_a++);

			dst += 2;
		}
	}
}

/* (16bit:GRAY) タイルデータを書き込み */

mlkerr _write_tile_16bit_gray(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint8_t *datbuf,*encbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//データセット

	_16bit_gray_set_data(datbuf, (uint16_t *)tilesrc, tiledst, is_first, val);

	//圧縮

	val[2] = _encode_8bit(encbuf, datbuf, 1024);
	val[3] = _encode_16bit(encbuf + val[2], datbuf + 1024, val[1]);

	//書き込み

	ret = UndoItem_write(p, val, 2 * 4);
	if(ret) return ret;

	return UndoItem_write(p, encbuf, val[2] + val[3]);
}

/* (16bit:GRAY) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_16bit_gray(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t val[4];
	mlkerr ret;

	//src タイルデータ (フラグのみ読み込む)

	if(isnot_empty)
	{
		ret = UndoItem_read(src, val, 2 * 4);
		if(ret) return ret;

		ret = _read_decode_8bit(src, APPUNDO->workbuf2, val[2], 1024, APPUNDO->workbuf1);
		if(ret) return ret;

		UndoItem_readSeek(src, val[3]);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_16bit_gray(dst, tilesrc, (isnot_empty)? APPUNDO->workbuf2: NULL, FALSE);
}

/* (16bit:GRAY) タイルデータ復元 */

mlkerr _restore_tile_16bit_gray(UndoItem *p,uint8_t *tile)
{
	uint8_t *encbuf,*datbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//読み込み

	ret = UndoItem_read(p, val, 2 * 4);
	if(ret) return ret;

	ret = UndoItem_read(p, encbuf, val[2] + val[3]);
	if(ret) return ret;

	//展開

	_decode_8bit(datbuf, encbuf, val[2], 1024);
	_decode_16bit(datbuf + 1024, encbuf + val[2], val[3], val[1]);

	//復元

	_16bit_gray_data_to_tile((uint16_t *)tile, datbuf, val[0]);

	return MLKERR_OK;
}


//============================
// 8bit:RGBA
//============================
/*
  [uint16 x 4]
    0: RGB 変化色データ、各チャンネルごとの元サイズ
    1: 変化色データ圧縮前サイズ
    2: フラグ圧縮後サイズ
    3: 変化色データ圧縮後サイズ
  [フラグ圧縮データ]
    (1bit:512byte) RGBの変化フラグ
    (1bit:512byte) Aの変化フラグ
  [変化色圧縮データ]
    R,G,B,A の順で各チャンネル別に
*/

/* (8bit:RGBA) データをセット
 *
 * dstval: [0] RGBデータサイズ [1] 全体の色データサイズ */

static void _8bit_rgba_set_data(uint8_t *dstbuf,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first,uint16_t *dstval)
{
	int i,j;
	uint8_t *pd_flags_col,*pd_flags_a,*pd_r,*pd_g,*pd_b,*pd_a,*prev_col,*prev_a;
	uint8_t fval_col,fval_a,f,srca,dsta,rev_col,rev_a;
	uint32_t srcc,dstc;

	pd_flags_col = dstbuf;
	pd_flags_a = dstbuf + 512;
	pd_r = dstbuf + 1024;
	pd_g = pd_r + 64 * 64;
	pd_b = pd_g + 64 * 64;
	pd_a = pd_b + 64 * 64;

	if(!is_first && tiledst)
	{
		prev_col = tiledst;
		prev_a = tiledst + 512;
	}

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = fval_a = 0;
		f = 0x80;

		if(!is_first && tiledst)
		{
			rev_col = *(prev_col++);
			rev_a = *(prev_a++);
		}
		
		for(j = 8; j; j--, f >>= 1)
		{
			srcc = (tilesrc[0] << 16) | (tilesrc[1] << 8) | tilesrc[2];
			srca = tilesrc[3];
			tilesrc += 4;
		
			//比較する値
			
			if(!tiledst)
				dstc = 0, dsta = 0;
			else if(is_first)
			{
				dstc = (tiledst[0] << 16) | (tiledst[1] << 8) | tiledst[2];
				dsta = tiledst[3];
				tiledst += 4;
			}
			else
			{
				//逆書き込み時: tiledst はフラグのバッファ

				dstc = (rev_col & f)? !srcc: srcc;
				dsta = (rev_a & f)? !srca: srca;
			}

			//色変化

			if(srcc != dstc)
			{
				fval_col |= f;
				*(pd_r++) = (uint8_t)(srcc >> 16);
				*(pd_g++) = (uint8_t)(srcc >> 8);
				*(pd_b++) = (uint8_t)srcc;
			}

			//A変化

			if(srca != dsta)
			{
				fval_a |= f;
				*(pd_a++) = srca;
			}
		}

		*(pd_flags_col++) = fval_col;
		*(pd_flags_a++) = fval_a;
	}

	//G,B,A データを移動

	i = pd_r - (dstbuf + 1024);
	j = pd_a - (dstbuf + 1024 + 64 * 64 * 3);

	if(i != 64 * 64)
	{
		//G
		memmove(dstbuf + 1024 + i, dstbuf + 1024 + 64 * 64, i);

		//B
		memmove(dstbuf + 1024 + i * 2, dstbuf + 1024 + 64 * 64 * 2, i);

		//A
		memmove(dstbuf + 1024 + i * 3, dstbuf + 1024 + 64 * 64 * 3, j);
	}

	//

	dstval[0] = i;
	dstval[1] = i * 3 + j;
}

/* (8bit:RGBA) データからタイル復元 */

static void _8bit_rgba_data_to_tile(uint8_t *dst,uint8_t *src,int colsize)
{
	uint8_t *ps_flags_col,*ps_flags_a,*ps_r,*ps_g,*ps_b,*ps_a;
	int i,j;
	uint8_t f,fval_col,fval_a;

	ps_flags_col = src;
	ps_flags_a = src + 512;
	ps_r = src + 1024;
	ps_g = ps_r + colsize;
	ps_b = ps_g + colsize;
	ps_a = ps_b + colsize;

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = *(ps_flags_col++);
		fval_a = *(ps_flags_a++);
		f = 0x80;
		
		for(j = 8; j; j--, f >>= 1)
		{
			if(fval_col & f)
			{
				dst[0] = *(ps_r++);
				dst[1] = *(ps_g++);
				dst[2] = *(ps_b++);
			}

			if(fval_a & f)
				dst[3] = *(ps_a++);

			dst += 4;
		}
	}
}

/* (8bit:RGBA) タイルデータを書き込み */

mlkerr _write_tile_8bit_rgba(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint8_t *datbuf,*encbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//データセット

	_8bit_rgba_set_data(datbuf, tilesrc, tiledst, is_first, val);

	//圧縮

	val[2] = _encode_8bit(encbuf, datbuf, 1024);
	val[3] = _encode_8bit(encbuf + val[2], datbuf + 1024, val[1]);

	//書き込み

	ret = UndoItem_write(p, val, 2 * 4);
	if(ret) return ret;

	return UndoItem_write(p, encbuf, val[2] + val[3]);
}

/* (8bit:RGBA) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_8bit_rgba(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t val[4];
	mlkerr ret;

	//src タイルデータ (フラグのみ読み込む)

	if(isnot_empty)
	{
		ret = UndoItem_read(src, val, 2 * 4);
		if(ret) return ret;

		ret = _read_decode_8bit(src, APPUNDO->workbuf2, val[2], 1024, APPUNDO->workbuf1);
		if(ret) return ret;

		UndoItem_readSeek(src, val[3]);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_8bit_rgba(dst, tilesrc, (isnot_empty)? APPUNDO->workbuf2: NULL, FALSE);
}

/* (8bit:RGBA) タイルデータ復元 */

mlkerr _restore_tile_8bit_rgba(UndoItem *p,uint8_t *tile)
{
	uint8_t *encbuf,*datbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//読み込み

	ret = UndoItem_read(p, val, 2 * 4);
	if(ret) return ret;

	ret = UndoItem_read(p, encbuf, val[2] + val[3]);
	if(ret) return ret;

	//展開

	_decode_8bit(datbuf, encbuf, val[2], 1024);
	_decode_8bit(datbuf + 1024, encbuf + val[2], val[3], val[1]);

	//復元

	_8bit_rgba_data_to_tile(tile, datbuf, val[0]);

	return MLKERR_OK;
}


//============================
// 16bit:RGBA
//============================
/*
  [uint16 x 4]
    0: RGB 変化色データ、各チャンネルごとの元サイズ
    1: 変化色データ圧縮前サイズ
    2: フラグ圧縮後サイズ
    3: 変化色データ圧縮後サイズ
  [フラグ圧縮データ]
    (1bit:512byte) RGBの変化フラグ
    (1bit:512byte) Aの変化フラグ
  [変化色圧縮データ]
    R,G,B,A の順で各チャンネル別に
*/

/* (16bit:RGBA) データをセット
 *
 * dstval: [0] RGBデータサイズ [1] 全体の色データサイズ */

static void _16bit_rgba_set_data(uint8_t *dstbuf,uint16_t *tilesrc,uint8_t *tiledst,mlkbool is_first,uint16_t *dstval)
{
	int i,j;
	uint8_t *pd_flags_col,*pd_flags_a,*prev_col,*prev_a;
	uint16_t *pd_r,*pd_g,*pd_b,*pd_a;
	uint16_t srcr,srcg,srcb,srca,dstr,dstg,dstb,dsta;
	uint8_t fval_col,fval_a,f,rev_col,rev_a;

	pd_flags_col = dstbuf;
	pd_flags_a = dstbuf + 512;
	pd_r = (uint16_t *)(dstbuf + 1024);
	pd_g = pd_r + 64 * 64;
	pd_b = pd_g + 64 * 64;
	pd_a = pd_b + 64 * 64;

	if(!is_first && tiledst)
	{
		prev_col = tiledst;
		prev_a = tiledst + 512;
	}

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = fval_a = 0;
		f = 0x80;

		if(!is_first && tiledst)
		{
			rev_col = *(prev_col++);
			rev_a = *(prev_a++);
		}
		
		for(j = 8; j; j--, f >>= 1)
		{
			srcr = tilesrc[0];
			srcg = tilesrc[1];
			srcb = tilesrc[2];
			srca = tilesrc[3];
			tilesrc += 4;
		
			//比較する値
			
			if(!tiledst)
			{
				dstr = dstg = dstb = dsta = 0;
			}
			else if(is_first)
			{
				dstr = *((uint16_t *)tiledst);
				dstg = *((uint16_t *)tiledst + 1);
				dstb = *((uint16_t *)tiledst + 2);
				dsta = *((uint16_t *)tiledst + 3);
				tiledst += 8;
			}
			else
			{
				//逆書き込み時: tiledst はフラグのバッファ

				if(rev_col & f)
					dstr = !srcr;
				else
				{
					dstr = srcr;
					dstg = srcg;
					dstb = srcb;
				}

				dsta = (rev_a & f)? !srca: srca;
			}

			//色変化

			if(srcr != dstr || srcg != dstg || srcb != dstb)
			{
				fval_col |= f;
				*(pd_r++) = srcr;
				*(pd_g++) = srcg;
				*(pd_b++) = srcb;
			}

			//A変化

			if(srca != dsta)
			{
				fval_a |= f;
				*(pd_a++) = srca;
			}
		}

		*(pd_flags_col++) = fval_col;
		*(pd_flags_a++) = fval_a;
	}

	//G,B,A データを移動

	i = (uint8_t *)pd_r - (dstbuf + 1024);
	j = (uint8_t *)pd_a - (dstbuf + 1024 + 64 * 64 * 3 * 2);

	if(i != 64 * 64 * 2)
	{
		//G
		memmove(dstbuf + 1024 + i, dstbuf + 1024 + 64 * 64 * 2, i);

		//B
		memmove(dstbuf + 1024 + i * 2, dstbuf + 1024 + 64 * 64 * 2 * 2, i);

		//A
		memmove(dstbuf + 1024 + i * 3, dstbuf + 1024 + 64 * 64 * 3 * 2, j);
	}

	//

	dstval[0] = i;
	dstval[1] = i * 3 + j;
}

/* (16bit:RGBA) データからタイル復元 */

static void _16bit_rgba_data_to_tile(uint16_t *dst,uint8_t *src,int colsize)
{
	uint8_t *ps_flags_col,*ps_flags_a;
	uint16_t *ps_r,*ps_g,*ps_b,*ps_a;
	int i,j;
	uint8_t f,fval_col,fval_a;

	ps_flags_col = src;
	ps_flags_a = src + 512;
	ps_r = (uint16_t *)(src + 1024);
	ps_g = (uint16_t *)((uint8_t *)ps_r + colsize);
	ps_b = (uint16_t *)((uint8_t *)ps_g + colsize);
	ps_a = (uint16_t *)((uint8_t *)ps_b + colsize);

	for(i = 64 * 64 / 8; i; i--)
	{
		fval_col = *(ps_flags_col++);
		fval_a = *(ps_flags_a++);
		f = 0x80;
		
		for(j = 8; j; j--, f >>= 1)
		{
			if(fval_col & f)
			{
				dst[0] = *(ps_r++);
				dst[1] = *(ps_g++);
				dst[2] = *(ps_b++);
			}

			if(fval_a & f)
				dst[3] = *(ps_a++);

			dst += 4;
		}
	}
}

/* (16bit:RGBA) タイルデータを書き込み */

mlkerr _write_tile_16bit_rgba(UndoItem *p,uint8_t *tilesrc,uint8_t *tiledst,mlkbool is_first)
{
	uint8_t *datbuf,*encbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//データセット

	_16bit_rgba_set_data(datbuf, (uint16_t *)tilesrc, tiledst, is_first, val);

	//圧縮

	val[2] = _encode_8bit(encbuf, datbuf, 1024);
	val[3] = _encode_16bit(encbuf + val[2], datbuf + 1024, val[1]);

	//書き込み

	ret = UndoItem_write(p, val, 2 * 4);
	if(ret) return ret;

	return UndoItem_write(p, encbuf, val[2] + val[3]);
}

/* (16bit:RGBA) タイルデータ 逆書き込み */

mlkerr _revwrite_tile_16bit_rgba(UndoItem *src,UndoItem *dst,uint8_t *tilesrc,mlkbool isnot_empty)
{
	uint16_t val[4];
	mlkerr ret;

	//src タイルデータ (フラグのみ読み込む)

	if(isnot_empty)
	{
		ret = UndoItem_read(src, val, 2 * 4);
		if(ret) return ret;

		ret = _read_decode_8bit(src, APPUNDO->workbuf2, val[2], 1024, APPUNDO->workbuf1);
		if(ret) return ret;

		UndoItem_readSeek(src, val[3]);
	}

	//書き込み

	if(!tilesrc)
		return MLKERR_OK;
	else
		return _write_tile_16bit_rgba(dst, tilesrc, (isnot_empty)? APPUNDO->workbuf2: NULL, FALSE);
}

/* (16bit:RGBA) タイルデータ復元 */

mlkerr _restore_tile_16bit_rgba(UndoItem *p,uint8_t *tile)
{
	uint8_t *encbuf,*datbuf;
	uint16_t val[4];
	mlkerr ret;

	datbuf = APPUNDO->workbuf1;
	encbuf = APPUNDO->workbuf2;

	//読み込み

	ret = UndoItem_read(p, val, 2 * 4);
	if(ret) return ret;

	ret = UndoItem_read(p, encbuf, val[2] + val[3]);
	if(ret) return ret;

	//展開

	_decode_8bit(datbuf, encbuf, val[2], 1024);
	_decode_16bit(datbuf + 1024, encbuf + val[2], val[3], val[1]);

	//復元

	_16bit_rgba_data_to_tile((uint16_t *)tile, datbuf, val[0]);

	return MLKERR_OK;
}

