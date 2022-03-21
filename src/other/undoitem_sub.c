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
 * UndoItem
 * サブ関数、レイヤの読み書き実処理
 *****************************************/

#include <stdio.h>
#include <string.h>	//strlen

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_list.h"
#include "mlk_undo.h"
#include "mlk_zlib.h"
#include "mlk_util.h"

#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "def_tileimage.h"
#include "tileimage.h"

#include "undo.h"
#include "undoitem.h"
#include "pv_undo.h"


//-----------------------

/* レイヤ情報 (文字列は除く) */

typedef struct
{
	uint8_t type,	//255 でフォルダ
		opacity,
		alphamask,
		blendmode,
		tone_density;
	int16_t tone_lines,
		tone_angle;
	uint32_t col,
		flags;
}UndoLayerInfo;

/* レイヤ情報 (文字列含む) */

typedef struct
{
	UndoLayerInfo i;
	char *name,
		*texture_path;
}UndoLayerInfo_str;

/* テキスト情報 */

typedef struct
{
	int32_t x,y,datsize;
	mRect rcdraw;
}TextTopInfo;

#define UNDOLAYERINFO_COLTYPE_FOLDER 255

//-----------------------



//==================================
// レイヤ番号など
//==================================


/** レイヤ番号からレイヤ取得 */

LayerItem *UndoItem_getLayerAtIndex(int no)
{
	return LayerList_getItemAtIndex(APPDRAW->layerlist, no);
}

/** 親と相対位置のレイヤ番号から、レイヤ取得 */

LayerItem *UndoItem_getLayerAtIndex_forParent(int parent,int pos)
{
	LayerItem *item[2];

	LayerList_getItems_fromIndex(APPDRAW->layerlist, item, parent, pos);

	return item[1];
}

/** LayerItem と LayerTextItem をインデックスから取得
 *
 * return: FALSE でいずれかが NULL */

mlkbool UndoItem_getLayerText_atIndex(int layerno,int index,LayerItem **pplayer,LayerTextItem **pptext)
{
	LayerItem *layer;
	LayerTextItem *text;

	layer = LayerList_getItemAtIndex(APPDRAW->layerlist, layerno);
	if(!layer) return FALSE;

	text = LayerItem_getTextItem_atIndex(layer, index);
	if(!text) return FALSE;

	*pplayer = layer;
	*pptext = text;

	return TRUE;
}


/** val にカレントレイヤの番号をセット */

void UndoItem_setval_curlayer(UndoItem *p,int valno)
{
	p->val[valno] = LayerList_getItemIndex(APPDRAW->layerlist, APPDRAW->curlayer);
}

/** val に指定レイヤの番号をセット */

void UndoItem_setval_layerno(UndoItem *p,int valno,LayerItem *item)
{
	p->val[valno] = LayerList_getItemIndex(APPDRAW->layerlist, item);
}

/** val[valno] に指定レイヤの親番号、val[valno+1] にレイヤ番号をセット */

void UndoItem_setval_layerno_forParent(UndoItem *p,int valno,LayerItem *item)
{
	LayerList_getItemIndex_forParent(APPDRAW->layerlist, item,
		p->val + valno, p->val + valno + 1);
}


//==============================
// レイヤ読み書き sub
//==============================


/* 文字列書込み (uint16 len + 文字列) */

static mlkerr _write_string(UndoItem *p,const char *text)
{
	uint16_t len;
	mlkerr ret;

	len = (text)? strlen(text): 0;

	ret = UndoItem_write(p, &len, 2);
	if(ret) return ret;
	
	return UndoItem_write(p, text, len);
}

/* 文字列読み込み
 *
 * dst: 確保された文字列バッファが入る */

static mlkerr _read_string(UndoItem *p,char **dst)
{
	char *buf;
	uint16_t len;
	mlkerr ret;

	*dst = NULL;

	//長さ

	ret = UndoItem_read(p, &len, 2);
	if(ret) return ret;

	//空文字列

	if(len == 0) return MLKERR_OK;

	//文字列

	buf = (char *)mMalloc(len + 1);
	if(!buf) return MLKERR_ALLOC;

	ret = UndoItem_read(p, buf, len);
	if(ret) return ret;

	buf[len] = 0;

	*dst = buf;

	return MLKERR_OK;
}

/* UndoLayerInfo_str 解放 */

static void _free_layerinfostr(UndoLayerInfo_str *p)
{
	mFree(p->name);
	mFree(p->texture_path);
}

/* レイヤ情報読み込み (テキストは除く) */

static mlkerr _read_layerinfo(UndoItem *p,UndoLayerInfo_str *info)
{
	mlkerr ret;

	memset(info, 0, sizeof(UndoLayerInfo_str));

	//レイヤ名

	ret = _read_string(p, &info->name);
	if(ret) goto ERR;

	//テクスチャパス
	
	ret = _read_string(p, &info->texture_path);
	if(ret) goto ERR;

	//レイヤ情報
	
	ret = UndoItem_read(p, &info->i, sizeof(UndoLayerInfo));
	if(ret) goto ERR;

	return MLKERR_OK;

ERR:
	_free_layerinfostr(info);
	return ret;
}

/* UndoLayerInfo_str から、レイヤに情報セット */

static void _set_layerinfo(LayerItem *item,UndoLayerInfo_str *info)
{
	item->type = (info->i.type == UNDOLAYERINFO_COLTYPE_FOLDER)? 0: info->i.type;
	item->blendmode = info->i.blendmode;
	item->opacity = info->i.opacity;
	item->alphamask = info->i.alphamask;
	item->col = info->i.col;
	item->flags = info->i.flags;

	item->tone_lines = info->i.tone_lines;
	item->tone_angle = info->i.tone_angle;
	item->tone_density = info->i.tone_density;

	mStrdup_free(&item->name, info->name);

	//テクスチャ

	LayerItem_setTexture(item, info->texture_path);
}

/** レイヤ情報のサイズ取得 */

int UndoItem_getLayerInfoSize(LayerItem *li)
{
	return sizeof(UndoLayerInfo) + 2 + mStrlen(li->name) + 2 + mStrlen(li->texture_path);
}


//==============================
// 読み書き 開始/終了
//==============================
/*
 * - 書き込み先は常にファイル。
 * - 読み書きを開始した場合、処理失敗時にも必ず終了させること。
 */


/** 書き込み開始 (ファイル出力 & zlib) */

mlkerr UndoItem_beginWrite_file_zlib(UndoItem *p)
{
	mlkerr ret;

	//書き込み開く

	ret = UndoItem_alloc(p, UNDO_ALLOC_FILE);
	if(ret) return ret;
	
	ret = UndoItem_openWrite(p);
	if(ret) return ret;

	//zlib
	
	APPUNDO->zenc = mZlibEncNew(16 * 1024, 4, -15, 8, 0);
	if(!APPUNDO->zenc)
	{
		UndoItem_closeWrite(p);
		return MLKERR_ALLOC;
	}

	mZlibSetIO_stdio(APPUNDO->zenc, APPUNDO->writefp);

	return MLKERR_OK;
}

/** file/zlib 書き込み終了 */

void UndoItem_endWrite_file_zlib(UndoItem *p)
{
	UndoItem_closeWrite(p);

	mZlibFree(APPUNDO->zenc);
}

/** file/zlib 読み込み開始 */

mlkerr UndoItem_beginRead_file_zlib(UndoItem *p)
{
	mlkerr ret;

	//開く

	ret = UndoItem_openRead(p);
	if(ret) return ret;

	//zlib

	APPUNDO->zdec = mZlibDecNew(16 * 1024, -15);
	if(!APPUNDO->zdec)
	{
		UndoItem_closeRead(p);
		return MLKERR_ALLOC;
	}

	mZlibSetIO_stdio(APPUNDO->zdec, APPUNDO->readfp);

	return MLKERR_OK;
}

/** file/zlib 読み込み終了 */

void UndoItem_endRead_file_zlib(UndoItem *p)
{
	UndoItem_closeRead(p);

	mZlibFree(APPUNDO->zdec);
}


//=====================================
// テキストデータ
//=====================================


/** テキストデータの書き込み */

mlkerr UndoItem_writeLayerText(UndoItem *p,LayerItem *li)
{
	LayerTextItem *text;
	TextTopInfo info;
	mlkerr ret;
	uint16_t num;

	//個数

	num = li->list_text.num;

	ret = UndoItem_write(p, &num, 2);
	if(ret) return ret;

	//データ

	MLK_LIST_FOR(li->list_text, text, LayerTextItem)
	{
		info.x = text->x;
		info.y = text->y;
		info.rcdraw = text->rcdraw;
		info.datsize = text->datsize;

		ret = UndoItem_write(p, &info, sizeof(TextTopInfo));
		if(ret) return ret;

		ret = UndoItem_write(p, text->dat, text->datsize);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/** テキストデータの読み込み (上書き置き換え) */

mlkerr UndoItem_readLayerText(UndoItem *p,LayerItem *li)
{
	TextTopInfo info;
	LayerTextItem *text;
	uint16_t num;
	mlkerr ret;

	mListDeleteAll(&li->list_text);

	//個数

	ret = UndoItem_read(p, &num, 2);
	if(ret) return ret;

	//データ

	for(; num; num--)
	{
		ret = UndoItem_read(p, &info, sizeof(TextTopInfo));
		if(ret) return ret;

		//追加

		text = LayerItem_addText_undo(li, info.x, info.y, info.datsize, NULL);
		if(!text) return MLKERR_ALLOC;

		text->rcdraw = info.rcdraw;

		//データ読込

		ret = UndoItem_read(p, text->dat, info.datsize);
		if(ret) return ret;
	}

	return MLKERR_OK;
}


//==================================
// レイヤデータ書き込み
//==================================


/** レイヤ情報とイメージを書き込み */

mlkerr UndoItem_writeLayerInfoAndImage(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	//情報

	ret = UndoItem_writeLayerInfo(p, item);
	if(ret) return ret;

	//テキスト

	ret = UndoItem_writeLayerText(p, item);
	if(ret) return ret;

	//イメージ
	
	return UndoItem_writeTileImage(p, item->img);
}

/** レイヤ情報の書き込み (テキスト含まない) */

mlkerr UndoItem_writeLayerInfo(UndoItem *p,LayerItem *li)
{
	UndoLayerInfo info;
	mlkerr ret;

	//レイヤ名

	ret = _write_string(p, li->name);
	if(ret) return ret;

	//テクスチャパス
	
	ret = _write_string(p, li->texture_path);
	if(ret) return ret;

	//レイヤ情報

	memset(&info, 0, sizeof(UndoLayerInfo));

	info.type = (li->img)? li->type: UNDOLAYERINFO_COLTYPE_FOLDER;
	info.opacity = li->opacity;
	info.blendmode = li->blendmode;
	info.alphamask = li->alphamask;
	info.col = li->col;
	info.flags = li->flags;
	info.tone_lines = li->tone_lines;
	info.tone_angle = li->tone_angle;
	info.tone_density = li->tone_density;

	return UndoItem_write(p, &info, sizeof(UndoLayerInfo));
}

/** タイルイメージ書き込み */

/* [TileImageInfo]
 * [uint32:圧縮サイズ]
 * [タイルイメージ]
 *   uint16: タイルX位置 (X,Y が 0xffff で終了)
 *   uint16: タイルY位置
 *   タイルデータ
 */

mlkerr UndoItem_writeTileImage(UndoItem *p,TileImage *img)
{
	TileImageInfo info;
	mZlib *enc = APPUNDO->zenc;
	uint8_t **pptile;
	int ix,iy,tw,th;
	uint16_t tpos[2];
	mlkerr ret;

	//img == NULL の場合フォルダなので、成功とする
	
	if(!img) return MLKERR_OK;

	//タイルイメージ情報

	TileImage_getInfo(img, &info);

	ret = UndoItem_write(p, &info, sizeof(TileImageInfo));
	if(ret) return ret;

	//圧縮サイズ (仮)

	ret = UndoItem_writeEncSize_temp();
	if(ret) return ret;

	//各タイル

	mZlibEncReset(enc);

	pptile = img->ppbuf;
	tw = img->tilew;
	th = img->tileh;

	for(iy = 0; iy < th; iy++)
	{
		for(ix = 0; ix < tw; ix++, pptile++)
		{
			if(*pptile)
			{
				//タイル位置

				tpos[0] = ix;
				tpos[1] = iy;

				ret = mZlibEncSend(enc, tpos, 4);
				if(ret) return ret;

				//タイルデータ
				
				ret = mZlibEncSend(enc, *pptile, img->tilesize);
				if(ret) return ret;
			}
		}
	}

	//終了

	tpos[0] = 0xffff;
	tpos[1] = 0xffff;

	ret = mZlibEncSend(enc, tpos, 4);
	if(ret) return ret;
	
	ret = mZlibEncFinish(enc);
	if(ret) return ret;

	//圧縮サイズ

	UndoItem_writeEncSize_real(mZlibEncGetSize(enc));

	return MLKERR_OK;
}


//==================================
// レイヤ読み込み
//==================================


/** レイヤ情報読み込み & レイヤ新規復元 (テキスト含まない)
 *
 * is_folder: フォルダかどうか */

mlkerr UndoItem_readLayerInfo_new(UndoItem *p,int parent_pos,int rel_pos,LayerItem **ppdst,int *is_folder)
{
	UndoLayerInfo_str info;
	LayerItem *item;
	mlkerr ret;

	//レイヤ情報読み込み

	ret = _read_layerinfo(p, &info);
	if(ret) return ret;

	//レイヤ作成

	item = LayerList_addLayer_index(APPDRAW->layerlist, parent_pos, rel_pos);
	if(!item)
	{
		_free_layerinfostr(&info);
		return MLKERR_ALLOC;
	}

	//情報をセット

	_set_layerinfo(item, &info);

	_free_layerinfostr(&info);

	//

	*ppdst = item;

	*is_folder = (info.i.type == UNDOLAYERINFO_COLTYPE_FOLDER);

	return MLKERR_OK;
}

/** レイヤを一つ復元 (情報+イメージ)
 *
 * 指定位置に挿入追加される。
 *
 * rcupdate: 更新範囲が追加される */

mlkerr UndoItem_readLayer(UndoItem *p,int parent_pos,int rel_pos,mRect *rcupdate)
{
	LayerItem *item;
	mRect rc;
	int is_folder;
	mlkerr ret;

	//レイヤ情報読み込み & 作成

	ret = UndoItem_readLayerInfo_new(p, parent_pos, rel_pos, &item, &is_folder);
	if(ret) return ret;

	//テキスト

	ret = UndoItem_readLayerText(p, item);
	if(ret) return ret;

	//イメージ作成 & 読み込み

	if(!is_folder)
	{
		ret = UndoItem_readLayerImage(p, item);
		if(ret) return ret;

		//更新範囲
		// :複数レイヤの復元時は、
		// :レイヤが非表示の状態でも範囲として必要な場合があるので (結合時など)、
		// :LayerItem_getVisibleImageRect() で取得しないこと。

		if(TileImage_getHaveImageRect_pixel(item->img, &rc, NULL))
			mRectUnion(rcupdate, &rc);
	}

	return MLKERR_OK;
}

/** 既存のレイヤに、情報とイメージを上書き復元 (テキスト含む)
 *
 * rcupdate: イメージの範囲を追加 */

mlkerr UndoItem_readLayer_overwrite(UndoItem *p,LayerItem *item,mRect *rcupdate)
{
	UndoLayerInfo_str info;
	mRect rc;
	mlkerr ret;

	//レイヤ情報:読み込み＆セット

	ret = _read_layerinfo(p, &info);
	if(ret) return ret;

	_set_layerinfo(item, &info);

	_free_layerinfostr(&info);

	//テキスト

	ret = UndoItem_readLayerText(p, item);
	if(ret) return ret;

	//イメージ置き換え

	ret = UndoItem_readLayerImage(p, item);
	if(ret) return ret;

	//更新範囲を追加

	if(TileImage_getHaveImageRect_pixel(item->img, &rc, NULL))
		mRectUnion(rcupdate, &rc);

	return MLKERR_OK;
}

/** レイヤイメージの復元
 *
 * - item の各情報は設定済みであること。
 * - 既存のイメージを置き換える場合もあり。 */

mlkerr UndoItem_readLayerImage(UndoItem *p,LayerItem *item)
{
	TileImageInfo info;
	TileImage *img;
	mZlib *dec = APPUNDO->zdec;
	uint8_t *tile;
	uint16_t tpos[2];
	uint32_t encsize;
	mlkerr ret;

	if(!item) return MLKERR_INVALID_VALUE;

	//TileImageInfo

	ret = UndoItem_read(p, &info, sizeof(TileImageInfo));
	if(ret) return ret;

	//既存イメージは削除

	TileImage_free(item->img);
	item->img = NULL;

	//イメージ作成

	img = TileImage_newFromInfo(item->type, &info);
	if(!img) return MLKERR_ALLOC;

	item->img = img;

	TileImage_setColor(img, item->col);

	//------ タイル読み込み

	//圧縮サイズ

	ret = UndoItem_read(p, &encsize, 4);
	if(ret) return ret;
	
	mZlibDecReset(dec);
	mZlibDecSetSize(dec, encsize);

	//

	while(1)
	{
		//タイル位置

		ret = mZlibDecRead(dec, tpos, 4);
		if(ret) return ret;

		if(tpos[0] == 0xffff && tpos[1] == 0xffff) break;

		//タイル作成

		tile = TileImage_getTileAlloc_atpos(img, tpos[0], tpos[1], FALSE);
		if(!tile) return MLKERR_ALLOC;

		//タイルデータ

		ret = mZlibDecRead(dec, tile, img->tilesize);
		if(ret) return ret;
	}

	return mZlibDecFinish(dec);
}

