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
 * データの読み書き
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_undo.h"

#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "tileimage.h"

#include "undo.h"
#include "undoitem.h"

#include "draw_layer.h"



//==============================
// 複数レイヤの順番移動
//==============================


/** 複数レイヤの順番移動 */

mlkerr UndoItem_setdat_movelayerlist_multi(UndoItem *p,int num,int16_t *buf)
{
	p->val[0] = num;

	return UndoItem_writeFull_buf(p, buf, num * 4 * 2);
}


//==============================
// リンクレイヤのレイヤ番号
//==============================


static mlkerr _linklayerno_write(UndoItem *p,void *param)
{
	LayerItem *pi = (LayerItem *)param;
	uint16_t no;
	mlkerr ret;

	for(; pi; pi = pi->link)
	{
		no = LayerList_getItemIndex(APPDRAW->layerlist, pi);
		
		ret = UndoItem_write(p, &no, 2);
		if(ret) return ret;
	}

	return MLKERR_OK;
}

/** リンクレイヤのレイヤ番号を書き込み */

mlkerr UndoItem_setdat_link_layerno(UndoItem *p,LayerItem *item,int num)
{
	return UndoItem_writeFixSize(p, num * 2, _linklayerno_write, item);
}

/** リンクレイヤの番号から、リンクをセット */

mlkerr UndoItem_restore_link_layerno(UndoItem *p,int num,LayerItem **pptop)
{
	mlkerr ret;
	uint16_t *buf,*ps;
	LayerItem *li,*top = NULL,*last;

	ret = UndoItem_readTop(p, num * 2, (void **)&buf);
	if(ret) return ret;

	for(ps = buf; num > 0; num--, ps++)
	{
		li = LayerList_getItemAtIndex(APPDRAW->layerlist, *ps);
		if(!li)
		{
			ret = MLKERR_INVALID_VALUE;
			break;
		}

		LayerItem_setLink(li, &top, &last);
	}

	mFree(buf);

	*pptop = top;

	return ret;
}


//==============================
// 単体レイヤ (情報+イメージ)
//==============================


/** 単体レイヤ 書き込み */

mlkerr UndoItem_setdat_layer(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_writeLayerInfoAndImage(p, item);

	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** 単体レイヤを復元 (指定位置に挿入追加)
 *
 * rc: 更新範囲が追加される */

mlkerr UndoItem_restore_layer(UndoItem *p,int parent,int pos,mRect *rc)
{
	mlkerr ret;

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_readLayer(p, parent, pos, rc);

	UndoItem_endRead_file_zlib(p);

	return ret;
}

/** 単体レイヤの情報とイメージを上書き復元 */

mlkerr UndoItem_restore_layer_overwrite(UndoItem *p,LayerItem *item,mRect *rc)
{
	mlkerr ret;

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_readLayer_overwrite(p, item, rc);

	UndoItem_endRead_file_zlib(p);

	return ret;
}


//=====================================
// 新規レイヤ(空)
//=====================================
/*
  val[0,1]: レイヤ番号
  val[2]: 新規時にイメージがあるか
  
  UNDO: なし
  REDO: レイヤ情報 (テキスト含まない)
*/


static mlkerr _newlayer_redo_write(UndoItem *p,void *param)
{
	return UndoItem_writeLayerInfo(p, (LayerItem *)param);
}

/** [REDO 時] レイヤ情報書き込み */

mlkerr UndoItem_setdat_newLayer_redo(UndoItem *p)
{
	LayerItem *li;

	li = UndoItem_getLayerAtIndex_forParent(p->val[0], p->val[1]);
	if(!li) return MLKERR_INVALID_VALUE;

	//書き込み

	return UndoItem_writeFixSize(p, UndoItem_getLayerInfoSize(li), _newlayer_redo_write, li);
}

/** [REDO 時] 空レイヤの復元 */

mlkerr UndoItem_restore_newLayerEmpty(UndoItem *p)
{
	LayerItem *item;
	TileImage *img;
	int is_folder;
	mlkerr ret;

	//レイヤ情報を読み込んで、レイヤ復元

	ret = UndoItem_openRead(p);
	if(ret) return ret;

	ret = UndoItem_readLayerInfo_new(p, p->val[0], p->val[1], &item, &is_folder);

	UndoItem_closeRead(p);

	if(ret) return ret;

	//新規イメージ作成

	if(!is_folder)
	{
		img = TileImage_new(item->type, 1, 1);
		if(!img) return MLKERR_ALLOC;

		LayerItem_replaceImage(item, img, -1);

		TileImage_setColor(img, item->col);
	}

	return MLKERR_OK;
}


//==============================
// 単体レイヤのイメージ
//==============================


/** 単体レイヤのイメージを書き込み */

mlkerr UndoItem_setdat_layerImage(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_writeTileImage(p, item->img);

	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** 単体レイヤのイメージを復元 */

mlkerr UndoItem_restore_layerImage(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_readLayerImage(p, item);

	UndoItem_endRead_file_zlib(p);

	return ret;
}


//===================================
// 複数レイヤのイメージ
//===================================
/*
  [int16:レイヤ番号][イメージ] x レイヤ数 ... [(int16)-1]
*/


/** 単体またはフォルダ下のイメージを書き込み (テキストレイヤは除く) */

mlkerr UndoItem_setdat_layerImage_folder(UndoItem *p,LayerItem *item)
{
	LayerItem *li;
	int16_t lno;
	mlkerr ret;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	for(li = item; li; li = LayerItem_getNextRoot(li, item))
	{
		if(li->img && !LAYERITEM_IS_TEXT(li))
		{
			//レイヤ番号
		
			lno = LayerList_getItemIndex(APPDRAW->layerlist, li);

			ret = UndoItem_write(p, &lno, 2);
			if(ret) goto ERR;

			//イメージ
			
			ret = UndoItem_writeTileImage(p, li->img);
			if(ret) goto ERR;
		}
	}

	//終了

	lno = -1;

	ret = UndoItem_write(p, &lno, 2);

ERR:
	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** すべてのレイヤのイメージ (レイヤ情報は除く)
 *
 * type: [0] テキストレイヤは除く [1] A1bit は除く(ビット変更時) */

mlkerr UndoItem_setdat_layerImage_all(UndoItem *p,int type)
{
	LayerItem *li;
	int16_t lno;
	mlkerr ret;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	for(li = LayerList_getTopItem(APPDRAW->layerlist); li; li = LayerItem_getNext(li))
	{
		if(li->img)
		{
			//除外

			if((type == 0 && LAYERITEM_IS_TEXT(li))
				|| (type == 1 && li->type == LAYERTYPE_ALPHA1BIT))
				continue;
		
			//レイヤ番号
		
			lno = LayerList_getItemIndex(APPDRAW->layerlist, li);

			ret = UndoItem_write(p, &lno, 2);
			if(ret) goto ERR;

			//イメージ
			
			ret = UndoItem_writeTileImage(p, li->img);
			if(ret) goto ERR;
		}
	}

	//終了

	lno = -1;

	ret = UndoItem_write(p, &lno, 2);

ERR:
	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** 複数レイヤのイメージを復元 */

mlkerr UndoItem_restore_layerImage_multi(UndoItem *p,mRect *rcupdate)
{
	LayerItem *li;
	int16_t lno;
	mRect rc;
	mlkerr ret;

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	while(1)
	{
		//レイヤ番号
		
		ret = UndoItem_read(p, &lno, 2);
		if(ret) break;

		if(lno == -1)
		{
			ret = MLKERR_OK;
			break;
		}

		li = UndoItem_getLayerAtIndex(lno);

		//イメージ復元

		ret = UndoItem_readLayerImage(p, li);
		if(ret) break;

		//更新範囲

		if(LayerItem_getVisibleImageRect(li, &rc))
			mRectUnion(rcupdate, &rc);
	}

	UndoItem_endRead_file_zlib(p);

	return ret;
}


//==================================
// 下レイヤへ移す
//==================================


/** item と次のレイヤのイメージをセット */

mlkerr UndoItem_setdat_layerImage_double(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	if(!item) return MLKERR_INVALID_VALUE;
	
	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_writeTileImage(p, item->img);

	if(ret == MLKERR_OK)
		ret = UndoItem_writeTileImage(p, LAYERITEM(item->i.next)->img);

	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** item と次のレイヤのイメージを復元 */

mlkerr UndoItem_restore_layerImage_double(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_readLayerImage(p, item);

	if(ret == MLKERR_OK)
		ret = UndoItem_readLayerImage(p, LayerItem_getNext(item));

	UndoItem_endRead_file_zlib(p);

	return ret;
}


//==================================
// 下レイヤへ結合
//==================================
/*
	UNDO: 上レイヤ + 下レイヤの情報とイメージ (テキスト含む)
	REDO: 結合後のレイヤの情報とイメージ
*/


/** 下レイヤへ結合時の書き込み */

mlkerr UndoItem_setdat_layerForCombine(UndoItem *p,int settype)
{
	LayerItem *item;
	mlkerr ret;

	item = UndoItem_getLayerAtIndex_forParent(p->val[1], p->val[2]);
	if(!item) return MLKERR_INVALID_VALUE;

	//

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	//上のレイヤ or 結合後のレイヤ

	ret = UndoItem_writeLayerInfoAndImage(p, item);
	if(ret) goto ERR;

	//UNDO 時、下のレイヤ

	if(settype == MUNDO_TYPE_UNDO)
	{
		ret = UndoItem_writeLayerInfoAndImage(p, LayerItem_getNext(item));
	}

ERR:
	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** 下レイヤへ結合時の復元 */

mlkerr UndoItem_restore_layerForCombine(UndoItem *p,int runtype,LayerItem *item,mRect *rcupdate)
{
	mRect rc;
	mlkerr ret;

	mRectEmpty(&rc);

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//[UNDO] 上のレイヤを新規復元 + 下レイヤを上書き復元

		ret = UndoItem_readLayer(p, p->val[1], p->val[2], &rc);
		if(ret) goto ERR;
		
		ret = UndoItem_readLayer_overwrite(p, item, &rc);
	}
	else
	{
		//[REDO] 結合後のレイヤを上書き復元 + 上のレイヤを削除
		// item = 上のレイヤ

		TileImage_getHaveImageRect_pixel(item->img, &rc, NULL);

		ret = UndoItem_readLayer_overwrite(p, LayerItem_getNext(item), &rc);

		drawLayer_deleteForUndo(APPDRAW, item);
	}

ERR:
	UndoItem_endRead_file_zlib(p);

	if(!ret) *rcupdate = rc;

	return ret;
}


//==============================
// 複数レイヤ (情報+イメージ)
//==============================
/*
  [int16 x 2:レイヤ位置][レイヤ情報][イメージ] x レイヤ数 ... [-1,-1]
*/


/** 単体レイヤ、または、item がフォルダの場合は複数レイヤ書き込み */

mlkerr UndoItem_setdat_layerForFolder(UndoItem *p,LayerItem *item)
{
	LayerItem *li;
	int16_t lno[2];
	int no1,no2;
	mlkerr ret;

	if(!item) return MLKERR_INVALID_VALUE;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	//レイヤ書き込み

	for(li = item; li; li = LayerItem_getNextRoot(li, item))
	{
		//レイヤ番号

		LayerList_getItemIndex_forParent(APPDRAW->layerlist, li, &no1, &no2);

		lno[0] = no1;
		lno[1] = no2;

		ret = UndoItem_write(p, lno, 4);
		if(ret) goto ERR;

		//レイヤ情報 + イメージ (フォルダの場合はイメージなし)
		
		ret = UndoItem_writeLayerInfoAndImage(p, li);
		if(ret) goto ERR;
	}

	//終了

	lno[0] = lno[1] = -1;

	ret = UndoItem_write(p, lno, 4);

ERR:
	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** すべてのレイヤの情報とイメージを書き込み
 *
 * 画像統合,すべてのレイヤ結合時 */

mlkerr UndoItem_setdat_layerAll(UndoItem *p)
{
	LayerItem *li;
	int16_t lno[2];
	int no1,no2;
	mlkerr ret;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	for(li = LayerList_getTopItem(APPDRAW->layerlist); li; li = LayerItem_getNext(li))
	{
		//レイヤ番号

		LayerList_getItemIndex_forParent(APPDRAW->layerlist, li, &no1, &no2);

		lno[0] = no1;
		lno[1] = no2;

		ret = UndoItem_write(p, lno, 4);
		if(ret) goto ERR;

		//レイヤ情報とイメージ (フォルダの場合はイメージなし)
		
		ret = UndoItem_writeLayerInfoAndImage(p, li);
		if(ret) goto ERR;
	}

	//終了

	lno[0] = lno[1] = -1;

	ret = UndoItem_write(p, lno, 4);

ERR:
	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** 複数レイヤ復元 */

mlkerr UndoItem_restore_layerMulti(UndoItem *p,mRect *rcupdate)
{
	int16_t lno[2];
	mlkerr ret;

	mRectEmpty(rcupdate);

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	while(1)
	{
		//レイヤ番号
		
		ret = UndoItem_read(p, lno, 4);
		if(ret) break;

		if(lno[1] == -1)
		{
			ret = MLKERR_OK;
			break;
		}

		//レイヤ復元

		ret = UndoItem_readLayer(p, lno[0], lno[1], rcupdate);
		if(ret) break;
	}

	UndoItem_endRead_file_zlib(p);

	return ret;
}


//===========================
// レイヤテキスト
//===========================
/*
  val[0] : レイヤ番号
  val[1] : テキストデータインデックス
  val[2] : テキストデータサイズ (位置移動時はなし)
  val[3] : x
  val[4] : y

  --- data ----
  TileImageInfo: 元に戻す時のイメージ情報
  [textdata] : 位置移動時はなし
*/


/* 最初の書き込み時 */

static mlkerr _layertext_single_write_first(UndoItem *p,void *param)
{
	void **pp = (void **)param;
	LayerTextItem *text;
	mlkerr ret;

	text = (LayerTextItem *)pp[0];

	//TileImageInfo

	ret = UndoItem_write(p, pp[1], sizeof(TileImageInfo));
	if(ret) return ret;

	//テキストデータ

	return UndoItem_write(p, text->dat, text->datsize);
}

/* 現在の状態から書き込み */

static mlkerr _layertext_single_write(UndoItem *p,void *param)
{
	void **pp = (void **)param;
	LayerItem *layer;
	LayerTextItem *text;
	TileImageInfo info;
	mlkerr ret;

	layer = (LayerItem *)pp[0];
	text = (LayerTextItem *)pp[1];

	//TileImageInfo

	TileImage_getInfo(layer->img, &info);

	ret = UndoItem_write(p, &info, sizeof(TileImageInfo));
	if(ret) return ret;

	//テキストデータ

	return UndoItem_write(p, text->dat, text->datsize);
}


/** テキストデータセット (最初の書き込み時) */

mlkerr UndoItem_setdat_layertext_single_first(UndoItem *p,TileImageInfo *info)
{
	LayerItem *layer;
	LayerTextItem *text;
	void *param[2];

	if(!UndoItem_getLayerText_atIndex(p->val[0], p->val[1], &layer, &text))
		return MLKERR_INVALID_VALUE;

	p->val[2] = text->datsize;
	p->val[3] = text->x;
	p->val[4] = text->y;

	//

	param[0] = text;
	param[1] = info;

	return UndoItem_writeFixSize(p,
		sizeof(TileImageInfo) + text->datsize, _layertext_single_write_first, (void *)param);
}

/** テキストデータセット (現在の状態から) */

mlkerr UndoItem_setdat_layertext_single(UndoItem *p)
{
	LayerItem *layer;
	LayerTextItem *text;
	void *param[2];

	if(!UndoItem_getLayerText_atIndex(p->val[0], p->val[1], &layer, &text))
		return MLKERR_INVALID_VALUE;

	p->val[2] = text->datsize;
	p->val[3] = text->x;
	p->val[4] = text->y;

	//

	param[0] = layer;
	param[1] = text;

	return UndoItem_writeFixSize(p,
		sizeof(TileImageInfo) + text->datsize, _layertext_single_write, (void *)param);
}

/** テキスト位置移動時のデータセット (現在の状態から) */

mlkerr UndoItem_setdat_layertext_move(UndoItem *p)
{
	LayerItem *layer;
	LayerTextItem *text;
	TileImageInfo info;

	if(!UndoItem_getLayerText_atIndex(p->val[0], p->val[1], &layer, &text))
		return MLKERR_INVALID_VALUE;

	p->val[3] = text->x;
	p->val[4] = text->y;

	TileImage_getInfo(layer->img, &info);

	return UndoItem_writeFull_buf(p, &info, sizeof(TileImageInfo));
}

/** 新規テキストの UNDO<->REDO データセット */

mlkerr UndoItem_setdat_layertext_new_reverse(UndoItem *dst,UndoItem *src)
{
	LayerItem *layer;
	uint8_t *buf;
	TileImageInfo info;
	int size;
	mlkerr ret;

	layer = UndoItem_getLayerAtIndex(src->val[0]);
	if(!layer) return MLKERR_INVALID_VALUE;

	size = sizeof(TileImageInfo) + src->val[2];

	//読み込み

	ret = UndoItem_readTop(src, size, (void **)&buf);
	if(ret) return ret;

	//TileImageInfo 書き換え

	TileImage_getInfo(layer->img, &info);

	memcpy(buf, &info, sizeof(TileImageInfo));

	//書き込み

	ret = UndoItem_writeFull_buf(dst, buf, size);

	//

	mFree(buf);

	return ret;
}

/** タイル配列を復元
 *
 * 新規テキスト UNDO 時, 位置移動時 */

mlkerr UndoItem_restore_layertext_tileimg(UndoItem *p,LayerItem *layer)
{
	uint8_t *buf;
	mlkerr ret;

	//読み込み

	ret = UndoItem_readTop(p, sizeof(TileImageInfo), (void **)&buf);
	if(ret) return ret;

	//タイル配列を元に戻す

	if(!TileImage_resizeTileBuf_forUndo(layer->img, (TileImageInfo *)buf))
		ret = MLKERR_ALLOC;

	mFree(buf);

	return ret;
}

/** テキストデータを復元
 *
 * テキストイメージの描画はこの後に行う。
 *
 * item: 編集時のアイテム。NULL で追加
 * dst: 作成されたテキストアイテム */

mlkerr UndoItem_restore_layertext_single(UndoItem *p,LayerItem *layer,LayerTextItem *item,LayerTextItem **dst)
{
	uint8_t *buf;
	LayerTextItem *newitem;
	mlkerr ret;

	//読み込み

	ret = UndoItem_readTop(p, sizeof(TileImageInfo) + p->val[2], (void **)&buf);
	if(ret) return ret;

	//タイル配列を戻す

	if(!TileImage_resizeTileBuf_forUndo(layer->img, (TileImageInfo *)buf))
	{
		mFree(buf);
		return MLKERR_ALLOC;
	}

	//アイテム追加/置き換え

	if(!item)
		newitem = LayerItem_addText_undo(layer, p->val[3], p->val[4], p->val[2], buf + sizeof(TileImageInfo));
	else
	{
		newitem = LayerItem_replaceText_undo(layer, item,
			p->val[3], p->val[4], p->val[2], buf + sizeof(TileImageInfo));
	}

	ret = (newitem)? MLKERR_OK: MLKERR_ALLOC;

	//
	
	mFree(buf);

	*dst = newitem;

	return ret;
}


//==============================
// レイヤテキストすべて
//==============================


/** 書き込み */

mlkerr UndoItem_setdat_layerTextAll(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_allocOpenWrite_variable(p);
	if(ret) return ret;

	ret = UndoItem_writeLayerText(p, item);

	return UndoItem_closeWrite_variable(p, ret);
}

/** 復元 */

mlkerr UndoItem_restore_layerTextAll(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_openRead(p);
	if(ret) return ret;

	ret = UndoItem_readLayerText(p, item);

	UndoItem_closeRead(p);

	return ret;
}


//==============================
// レイヤテキスト + イメージ
//==============================


/** 書き込み */

mlkerr UndoItem_setdat_layerTextAndImage(UndoItem *p,LayerItem *item)
{
	mlkerr ret;

	ret = UndoItem_beginWrite_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_writeLayerText(p, item);

	if(ret == MLKERR_OK)
		ret = UndoItem_writeTileImage(p, item->img);

	UndoItem_endWrite_file_zlib(p);

	return ret;
}

/** 復元 */

mlkerr UndoItem_restore_layerTextAndImage(UndoItem *p,LayerItem *item,mRect *rcupdate)
{
	mlkerr ret;

	ret = UndoItem_beginRead_file_zlib(p);
	if(ret) return ret;

	ret = UndoItem_readLayerText(p, item);

	if(ret == MLKERR_OK)
	{
		ret = UndoItem_readLayerImage(p, item);

		//更新範囲

		if(ret == MLKERR_OK)
			TileImage_getHaveImageRect_pixel(item->img, rcupdate, NULL);
	}

	UndoItem_endRead_file_zlib(p);

	return ret;
}

