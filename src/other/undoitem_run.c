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
 * 実行処理
 *****************************************/

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_list.h"
#include "mlk_undo.h"

#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "tileimage.h"

#include "undo.h"
#include "undoitem.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_layer.h"


//--------------------

#define _CURSOR_WAIT    drawCursor_wait()
#define _CURSOR_RESTORE drawCursor_restore()

//--------------------

/*
 * [!] 実行処理時に失敗しても更新は行われるので、更新情報は常にセットすること。
 *
 * [!] 現在のレイヤのリスト状態によっては、
 *     処理実行後にカレントレイヤが削除されたり非展開フォルダに隠れたりするので、
 *     カレントレイヤが正しくリスト上に表示されるよう注意すること。
 */



/* (共通) レイヤ削除処理
 *
 * 更新範囲のセット + レイヤ削除 */

static mlkerr _run_layerDelete(LayerItem *item,UndoUpdateInfo *info)
{
	if(!item) return MLKERR_INVALID_VALUE;

	//更新範囲
	// :削除前に行う。
	// :空レイヤの場合、更新範囲は空となるが、レイヤ一覧は常に更新しなければらなない。

	info->type = UNDO_UPDATE_RECT_LAYERLIST;

	LayerItem_getVisibleImageRect(item, &info->rc);

	//削除 (カレントレイヤの調整あり)

	drawLayer_deleteForUndo(APPDRAW, item);

	return MLKERR_OK;
}


/** レイヤ新規追加 */

mlkerr UndoItem_runLayerNew(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	if(runtype == MUNDO_TYPE_UNDO)
	{
		//UNDO: レイヤ削除

		return _run_layerDelete(
			UndoItem_getLayerAtIndex_forParent(p->val[0], p->val[1]), info);
	}
	else
	{
		//REDO: レイヤ復元

		if(p->val[2])
		{
			//イメージあり
			
			info->type = UNDO_UPDATE_RECT_LAYERLIST;

			return UndoItem_restore_layer(p, p->val[0], p->val[1], &info->rc);
		}
		else
		{
			//イメージ空

			info->type = UNDO_UPDATE_LAYERLIST;

			return UndoItem_restore_newLayerEmpty(p);
		}	
	}
}

/** レイヤ複製 */

mlkerr UndoItem_runLayerDup(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *src,*item;

	src = UndoItem_getLayerAtIndex(p->val[0]);
	if(!src) return MLKERR_INVALID_VALUE;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//レイヤ削除

		return _run_layerDelete(src, info);
	}
	else
	{
		//レイヤ再複製

		item = LayerList_dupLayer(APPDRAW->layerlist, src);
		if(!item) return MLKERR_ALLOC;

		info->type = UNDO_UPDATE_RECT_LAYERLIST;

		LayerItem_getVisibleImageRect(item, &info->rc);

		return MLKERR_OK;
	}
}

/** レイヤ削除 */

mlkerr UndoItem_runLayerDelete(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	if(runtype == MUNDO_TYPE_REDO)
	{
		//レイヤ削除

		return _run_layerDelete(UndoItem_getLayerAtIndex(p->val[0]), info);
	}
	else
	{
		//レイヤ復元

		info->type = UNDO_UPDATE_RECT_LAYERLIST;

		return UndoItem_restore_layerMulti(p, &info->rc);
	}
}

/** レイヤイメージクリア */

mlkerr UndoItem_runLayerClearImage(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mlkerr ret;

	item = UndoItem_getLayerAtIndex(p->val[0]);
	if(!item) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = item;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//イメージ復元

		ret = UndoItem_restore_layerImage(p, item);

		if(ret == MLKERR_OK)
			LayerItem_getVisibleImageRect(item, &info->rc);

		return ret;
	}
	else
	{
		//クリア

		LayerItem_getVisibleImageRect(item, &info->rc);

		TileImage_clear(item->img);
	}

	return MLKERR_OK;
}

/** レイヤタイプ変更 */
/*
 [0] レイヤ番号
 [1] 元のカラータイプ
 [2] 変換後のカラータイプ
 [3] 元がテキストレイヤか
*/

mlkerr UndoItem_runLayerSetType(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;

	item = UndoItem_getLayerAtIndex(p->val[0]);
	if(!item) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = item;

	//タイプを戻す

	if(runtype == MUNDO_TYPE_UNDO)
	{
		item->type = p->val[1];

		if(p->val[3]) item->flags |= LAYERITEM_F_TEXT;
	}
	else
	{
		item->type = p->val[2];
		item->flags &= ~LAYERITEM_F_TEXT;
	}

	//テキストとイメージを戻す

	return UndoItem_restore_layerTextAndImage(p, item, &info->rc);
}

/** レイヤタイプ変更 (テキストのみ)
 *
 * テキストレイヤで、同じタイプへの通常レイヤ変更時 */

mlkerr UndoItem_runLayerSetType_text(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;

	item = UndoItem_getLayerAtIndex(p->val[0]);
	if(!item) return MLKERR_INVALID_VALUE;

	//レイヤ一覧のみ更新
	info->type = UNDO_UPDATE_RECT;
	info->layer = item;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//UNDO: テキストレイヤに戻して、テキストを復元
	
		item->flags |= LAYERITEM_F_TEXT;

		return UndoItem_restore_layerTextAll(p, item);
	}
	else
	{
		//REDO: 通常レイヤにして、テキストをクリア
		
		item->flags &= ~LAYERITEM_F_TEXT;

		LayerItem_clearText(item);

		return MLKERR_OK;
	}
}

/** 下レイヤに移す/結合 */

mlkerr UndoItem_runLayerDropAndCombine(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mlkerr ret;

	item = UndoItem_getLayerAtIndex_forParent(p->val[1], p->val[2]);
	if(!item) return MLKERR_INVALID_VALUE;

	//[!] どちらかが非表示状態の場合でも、正しい範囲を更新できるようにするため、
	//    LayerItem_getVisibleImageRect() は使わない。

	info->type = UNDO_UPDATE_RECT_LAYERLIST;

	if(p->val[0] == 0)
	{
		//下レイヤに結合

		return UndoItem_restore_layerForCombine(p, runtype, item, &info->rc);
	}
	else
	{
		//------ 下レイヤに移す (item = 上のレイヤ)

		//[更新範囲]
		// UNDO時 = 復元した上のレイヤのイメージ範囲
		// REDO時 = 結合しようとしている上のレイヤのイメージ範囲

		if(runtype == MUNDO_TYPE_REDO)
			TileImage_getHaveImageRect_pixel(item->img, &info->rc, NULL);

		ret = UndoItem_restore_layerImage_double(p, item);
		if(ret) return ret;

		if(runtype == MUNDO_TYPE_UNDO)
			TileImage_getHaveImageRect_pixel(item->img, &info->rc, NULL);

		return MLKERR_OK;
	}
}

/** すべてのレイヤの結合/画像統合 */

mlkerr UndoItem_runLayerCombineAll(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mlkerr ret;

	info->type = UNDO_UPDATE_FULL_LAYERLIST;

	_CURSOR_WAIT;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//UNDO: 全レイヤ復元 + 結合後レイヤを削除

		item = LayerList_getTopItem(APPDRAW->layerlist);

		ret = UndoItem_restore_layerMulti(p, &info->rc);

		drawLayer_deleteForUndo(APPDRAW, item);
	}
	else
	{
		//REDO: レイヤすべて削除 + 結合後レイヤを復元

		LayerList_clear(APPDRAW->layerlist);

		ret = UndoItem_restore_layer(p, -1, 0, &info->rc); //-1 = root

		APPDRAW->curlayer = LayerList_getTopItem(APPDRAW->layerlist);
	}

	_CURSOR_RESTORE;

	return ret;
}

/** フォルダ内のすべてのレイヤ結合 */

mlkerr UndoItem_runLayerCombineFolder(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mRect rc;
	mlkerr ret;

	item = UndoItem_getLayerAtIndex_forParent(p->val[0], p->val[1]);
	if(!item) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT_LAYERLIST;

	//[UNDO] フォルダの全レイヤ復元, 結合後レイヤ削除
	//[REDO] 結合後レイヤ復元, フォルダ削除

	//復元

	if(runtype == MUNDO_TYPE_UNDO)
		ret = UndoItem_restore_layerMulti(p, &info->rc);
	else
		ret = UndoItem_restore_layer(p, p->val[0], p->val[1], &info->rc);

	//結合後レイヤ、またはフォルダを削除

	if(ret == MLKERR_OK)
	{
		//範囲追加
	
		if(LayerItem_getVisibleImageRect(item, &rc))
			mRectUnion(&info->rc, &rc);

		//削除

		drawLayer_deleteForUndo(APPDRAW, item);
	}

	return ret;
}

/** レイヤ全体の左右上下反転/回転
 *
 * - フォルダの場合は、フォルダ以下が対象。 */

mlkerr UndoItem_runLayerEditFull(UndoItem *p,UndoUpdateInfo *info)
{
	LayerItem *li;

	li = UndoItem_getLayerAtIndex(p->val[0]);
	if(!li) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT_LAYERLIST;

	if(p->val[1] < 2)
	{
		//左右上下反転
		
		LayerItem_editImage_full(li, p->val[1], &info->rc);

		return MLKERR_OK;
	}
	else
	{
		//回転

		return UndoItem_restore_layerImage_multi(p, &info->rc);
	}
}

/** レイヤオフセット位置移動 */
/*
 * <val>
 * [0] x 相対移動値
 * [1] y 相対移動値
 * [2] 対象レイヤ数
 * [3] レイヤ数が1なら、レイヤ番号
 */

mlkerr UndoItem_runLayerMoveOffset(UndoItem *p,UndoUpdateInfo *info)
{
	LayerItem *li,*top;
	mRect rc,rc1,rc2;
	int mx,my,lnum;
	mlkerr ret;

	mx = p->val[0];
	my = p->val[1];
	lnum = p->val[2];

	//レイヤ移動

	if(lnum == 1)
	{
		//単体レイヤ
		
		li = LayerList_getItemAtIndex(APPDRAW->layerlist, p->val[3]);
		if(!li) return MLKERR_INVALID_VALUE;
	
		TileImage_getHaveImageRect_pixel(li->img, &rc1, NULL);

		LayerItem_moveImage(li, mx, my);

		drawCalc_unionRect_relmove(&rc, &rc1, mx, my);

		info->type = UNDO_UPDATE_RECT;
		info->layer = li;
	}
	else
	{
		//複数レイヤ

		ret = UndoItem_restore_link_layerno(p, lnum, &top);
		if(ret) return ret;
	
		mRectEmpty(&rc);

		for(li = top; li; li = li->link)
		{
			TileImage_getHaveImageRect_pixel(li->img, &rc1, NULL);

			LayerItem_moveImage(li, mx, my);

			drawCalc_unionRect_relmove(&rc2, &rc1, mx, my);
			mRectUnion(&rc, &rc2);
		}

		info->type = UNDO_UPDATE_RECT_LAYERLIST;
	}

	info->rc = rc;

	return MLKERR_OK;
}

/** 単体レイヤの順番移動 */

mlkerr UndoItem_runLayerMoveList(UndoItem *p,UndoUpdateInfo *info)
{
	//移動
	
	if(!LayerList_moveitem_forUndo(APPDRAW->layerlist, p->val, &info->rc))
		return MLKERR_INVALID_VALUE;

	//カレントレイヤが見えるように調整

	drawLayer_afterMoveList_forUndo(APPDRAW);

	info->type = UNDO_UPDATE_RECT_LAYERLIST;

	return MLKERR_OK;
}

/** 複数レイヤの順番移動 */

mlkerr UndoItem_runLayerMoveList_multi(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	int16_t *buf;
	mlkerr ret;

	//読み込み

	ret = UndoItem_readTop(p, p->val[0] * 4 * 2, (void **)&buf);
	if(ret) return ret;

	//移動

	info->type = UNDO_UPDATE_RECT_LAYERLIST;

	ret = LayerList_moveitem_multi_forUndo(APPDRAW->layerlist, p->val[0], buf,
		&info->rc, (runtype == MUNDO_TYPE_REDO));

	mFree(buf);

	//

	drawLayer_afterMoveList_forUndo(APPDRAW);

	return ret;
}


//=============================
// 全体
//=============================


/** イメージビット数の変更 */

mlkerr UndoItem_runChangeImageBits(UndoItem *p,UndoUpdateInfo *info)
{
	int ret;

	info->type = UNDO_UPDATE_IMAGEBITS;

	//ビット数の変更

	ret = drawImage_changeImageBits_proc(APPDRAW, NULL);
	if(ret) return ret;

	//イメージ復元

	_CURSOR_WAIT;

	ret = UndoItem_restore_layerImage_multi(p, &info->rc);

	_CURSOR_RESTORE;

	return ret;
}

/** キャンバスイメージ変更(切り取り) */

mlkerr UndoItem_runCanvasResize(UndoItem *p,UndoUpdateInfo *info)
{
	mRect rc;
	mlkerr ret;

	info->type = UNDO_UPDATE_CANVAS_RESIZE;
	info->rc.x1 = p->val[0];
	info->rc.y1 = p->val[1];

	//-----

	_CURSOR_WAIT;

	//通常レイヤのイメージ復元

	ret = UndoItem_restore_layerImage_multi(p, &rc);

	//テキストレイヤは位置移動

	LayerList_moveOffset_rel_text(APPDRAW->layerlist, p->val[2], p->val[3]);

	_CURSOR_RESTORE;

	return ret;
}

/** キャンバス拡大縮小 */

mlkerr UndoItem_runCanvasScale(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *item;
	mRect rc;
	mlkerr ret;

	info->type = UNDO_UPDATE_CANVAS_RESIZE;
	info->rc.x1 = p->val[0];
	info->rc.y1 = p->val[1];

	drawImage_changeDPI(APPDRAW, p->val[2]);

	//

	_CURSOR_WAIT;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//UNDO: 全レイヤ復元 + 結合後レイヤを削除

		item = LayerList_getTopItem(APPDRAW->layerlist);

		ret = UndoItem_restore_layerMulti(p, &rc);

		drawLayer_deleteForUndo(APPDRAW, item);
	}
	else
	{
		//REDO: レイヤすべて削除 + 結合後レイヤを復元

		LayerList_clear(APPDRAW->layerlist);

		ret = UndoItem_restore_layer(p, -1, 0, &rc); //-1 = root

		APPDRAW->curlayer = LayerList_getTopItem(APPDRAW->layerlist);
	}

	_CURSOR_RESTORE;

	return ret;
}


//============================
// レイヤテキスト
//============================


/** 新規 */

mlkerr UndoItem_runLayerTextNew(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *layer;
	LayerTextItem *item;
	mlkerr ret;

	layer = UndoItem_getLayerAtIndex(p->val[0]);
	if(!layer) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = layer;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//---- UNDO: テキスト削除

		item = LayerItem_getTextItem_atIndex(layer, p->val[1]);
		if(!item) return MLKERR_INVALID_VALUE;

		//現在のテキスト範囲を消去

		drawText_clearItemRect(APPDRAW, layer, item, &info->rc);

		//タイル配列を元に戻す

		ret = UndoItem_restore_layertext_tileimg(p, layer);
		if(ret) return ret;

		//テキスト削除

		LayerItem_deleteText(layer, item);
	}
	else
	{
		//--- REDO: 新規テキスト

		//テキストを復元 & タイル配列を戻す

		ret = UndoItem_restore_layertext_single(p, layer, NULL, &item);
		if(ret) return ret;

		//描画
		// :rcdraw はここでセット。

		drawText_drawLayerText(APPDRAW, item, layer->img, &item->rcdraw);

		info->rc = item->rcdraw;
	}

	return MLKERR_OK;
}

/** 編集 */

mlkerr UndoItem_runLayerTextEdit(UndoItem *p,UndoUpdateInfo *info)
{
	LayerItem *layer;
	LayerTextItem *text;
	mlkerr ret;

	if(!UndoItem_getLayerText_atIndex(p->val[0], p->val[1], &layer, &text))
		return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = layer;

	//現在のテキスト範囲を消去

	drawText_clearItemRect(APPDRAW, layer, text, &info->rc);

	//テキストを復元 & タイル配列を戻す

	ret = UndoItem_restore_layertext_single(p, layer, text, &text);
	if(ret) return ret;

	//描画
	// :rcdraw はここでセット。

	drawText_drawLayerText(APPDRAW, text, layer->img, &text->rcdraw);

	mRectUnion(&info->rc, &text->rcdraw);

	return MLKERR_OK;
}

/** 位置移動 */

mlkerr UndoItem_runLayerTextMove(UndoItem *p,UndoUpdateInfo *info)
{
	LayerItem *layer;
	LayerTextItem *text;
	mlkerr ret;

	if(!UndoItem_getLayerText_atIndex(p->val[0], p->val[1], &layer, &text))
		return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = layer;

	//現在のテキスト範囲を消去

	drawText_clearItemRect(APPDRAW, layer, text, &info->rc);

	//タイル配列を戻す

	ret = UndoItem_restore_layertext_tileimg(p, layer);
	if(ret) return ret;

	text->x = p->val[3];
	text->y = p->val[4];

	//描画
	// :rcdraw はここでセット。

	drawText_drawLayerText(APPDRAW, text, layer->img, &text->rcdraw);

	mRectUnion(&info->rc, &text->rcdraw);

	return MLKERR_OK;
}

/** レイヤテキスト削除 */

mlkerr UndoItem_runLayerTextDelete(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *layer;
	LayerTextItem *item;
	mListItem *ins;
	mlkerr ret;

	layer = UndoItem_getLayerAtIndex(p->val[0]);
	if(!layer) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = layer;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//--- UNDO: テキスト復元

		//追加後の挿入位置

		ins = mListGetItemAtIndex(&layer->list_text, p->val[1]);

		//テキストを復元 (追加)

		ret = UndoItem_restore_layertext_single(p, layer, NULL, &item);
		if(ret) return ret;

		//位置を移動

		mListMove(&layer->list_text, MLISTITEM(item), ins);

		//描画

		drawText_drawLayerText(APPDRAW, item, layer->img, &item->rcdraw);

		info->rc = item->rcdraw;
	}
	else
	{
		//---- REDO: テキスト削除

		item = LayerItem_getTextItem_atIndex(layer, p->val[1]);
		if(!item) return MLKERR_INVALID_VALUE;

		//現在のテキスト範囲を消去

		drawText_clearItemRect(APPDRAW, layer, item, &info->rc);

		//テキスト削除

		LayerItem_deleteText(layer, item);
	}

	return MLKERR_OK;
}

/** レイヤテキストクリア */

mlkerr UndoItem_runLayerTextClear(UndoItem *p,UndoUpdateInfo *info,int runtype)
{
	LayerItem *layer;

	layer = UndoItem_getLayerAtIndex(p->val[0]);
	if(!layer) return MLKERR_INVALID_VALUE;

	info->type = UNDO_UPDATE_RECT;
	info->layer = layer;

	if(runtype == MUNDO_TYPE_UNDO)
	{
		//UNDO: テキストとイメージを元に戻す

		return UndoItem_restore_layerTextAndImage(p, layer, &info->rc);
	}
	else
	{
		//REDO: 再クリア

		LayerItem_getVisibleImageRect(layer, &info->rc);

		TileImage_clear(layer->img);
		LayerItem_clearText(layer);
	}

	return MLKERR_OK;
}

