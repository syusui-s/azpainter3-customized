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
 * アンドゥ操作
 *****************************************/

#include <stdio.h> //AppUndo:FILE
#include <string.h>

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_list.h"
#include "mlk_undo.h"

#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "def_tileimage.h"
#include "tileimage.h"

#include "undo.h"
#include "undoitem.h"
#include "pv_undo.h"


//--------------------

AppUndo *g_app_undo = NULL;

//--------------------


//========================
// sub
//========================


/* (mUndo) アイテム作成 */

static mlkerr _newitem_handle(mUndo *p,mListItem **ppdst)
{
	UndoItem *pi;

	//作成
	// :fileno を -1 にしておかないと、
	// :アイテム破棄時に番号 0 のアンドゥファイルが削除される。

	pi = (UndoItem *)mMalloc0(sizeof(UndoItem));
	if(!pi) return MLKERR_ALLOC;

	pi->fileno = -1;

	*ppdst = (mListItem *)pi;

	//変更フラグ
	// :アンドゥ追加、アンドゥ/リドゥ実行時は
	// :イメージが変更されたということなので、フラグ ON

	APPUNDO->fmodify = TRUE;

	return MLKERR_OK;
}

/* (mUndo) アンドゥ <=> リドゥ のデータセット */

static mlkerr _setreverse_handle(mUndo *undo,
	mListItem *dstitem,mListItem *srcitem,int settype)
{
	UndoItem *dst,*src;

	dst = (UndoItem *)dstitem;
	src = (UndoItem *)srcitem;

	//ソースの情報コピー

	dst->type = src->type;

	memcpy(dst->val, src->val, sizeof(int) * UNDOITEM_VAL_NUM);

	//

	switch(src->type)
	{
		//タイルイメージ
		case UNDO_TYPE_TILEIMAGE:
			return UndoItem_setdat_tileimage_reverse(dst, src, settype);

		//レイヤ追加
		case UNDO_TYPE_LAYER_NEW:
			if(settype == MUNDO_TYPE_REDO)
			{
				if(src->val[2])
				{
					//イメージあり
					return UndoItem_setdat_layer(dst,
						UndoItem_getLayerAtIndex_forParent(src->val[0], src->val[1]));
				}
				else
					//イメージなし
					return UndoItem_setdat_newLayer_redo(dst);
			}
			break;

		//レイヤ削除
		case UNDO_TYPE_LAYER_DELETE:
			if(settype == MUNDO_TYPE_UNDO)
			{
				return UndoItem_setdat_layerForFolder(dst, UndoItem_getLayerAtIndex(src->val[0]));
			}
			break;

		//レイヤイメージクリア
		case UNDO_TYPE_LAYER_CLEARIMG:
			if(settype == MUNDO_TYPE_UNDO)
				return UndoItem_setdat_layerImage(dst, UndoItem_getLayerAtIndex(src->val[0]));
			break;

		//レイヤタイプ変更
		case UNDO_TYPE_LAYER_SETTYPE:
			return UndoItem_setdat_layerTextAndImage(dst, UndoItem_getLayerAtIndex(src->val[0]));

		//レイヤタイプ変更 (テキストのみ)
		case UNDO_TYPE_LAYER_SETTYPE_TEXT:
			if(settype == MUNDO_TYPE_UNDO)
				return UndoItem_setdat_layerTextAll(dst, UndoItem_getLayerAtIndex(src->val[0]));
			break;
	
		//下レイヤに移す/結合
		case UNDO_TYPE_LAYER_DROP_AND_COMBINE:
			if(src->val[0])
			{
				//移す
				return UndoItem_setdat_layerImage_double(dst,
					UndoItem_getLayerAtIndex_forParent(src->val[1], src->val[2]));
			}
			else
				return UndoItem_setdat_layerForCombine(dst, settype);
			break;
	
		//すべてのレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_ALL:
			if(settype == MUNDO_TYPE_REDO)
				return UndoItem_setdat_layer(dst, LayerList_getTopItem(APPDRAW->layerlist));
			else
				return UndoItem_setdat_layerAll(dst);
			break;

		//フォルダレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_FOLDER:
			if(settype == MUNDO_TYPE_REDO)
			{
				return UndoItem_setdat_layer(dst,
					UndoItem_getLayerAtIndex_forParent(src->val[0], src->val[1]));
			}
			else
			{
				return UndoItem_setdat_layerForFolder(dst,
					UndoItem_getLayerAtIndex_forParent(src->val[0], src->val[1]));
			}
			break;
		
		//レイヤ全体の左右上下反転/回転
		case UNDO_TYPE_LAYER_EDIT_FULL:
			if(src->val[1] >= 2)
			{
				return UndoItem_setdat_layerImage_folder(dst, UndoItem_getLayerAtIndex(src->val[0]));
			}
			break;

		//レイヤのオフセット移動
		case UNDO_TYPE_LAYER_MOVE_OFFSET:
			dst->val[0] = -dst->val[0];
			dst->val[1] = -dst->val[1];

			if(src->val[2] > 1)
				return UndoItem_copyData(src, dst, src->val[2] * 2);
			break;
	
		//レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST:
			dst->val[0] = src->val[2];
			dst->val[1] = src->val[3];
			dst->val[2] = src->val[0];
			dst->val[3] = src->val[1];
			break;

		//複数レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST_MULTI:
			return UndoItem_copyData(src, dst, src->val[0] * 4 * 2);

		//レイヤテキスト新規
		case UNDO_TYPE_LAYERTEXT_NEW:
			return UndoItem_setdat_layertext_new_reverse(dst, src);

		//レイヤテキスト編集
		case UNDO_TYPE_LAYERTEXT_EDIT:
			return UndoItem_setdat_layertext_single(dst);

		//レイヤテキスト削除
		case UNDO_TYPE_LAYERTEXT_DELETE:
			return UndoItem_copyData(src, dst, sizeof(TileImageInfo) + src->val[2]);

		//レイヤテキスト位置移動
		case UNDO_TYPE_LAYERTEXT_MOVE:
			return UndoItem_setdat_layertext_move(dst);

		//レイヤテキストクリア
		case UNDO_TYPE_LAYERTEXT_CLEAR:
			if(settype == MUNDO_TYPE_UNDO)
				return UndoItem_setdat_layerTextAndImage(dst, UndoItem_getLayerAtIndex(src->val[0]));
			break;

		//イメージビット数の変更
		case UNDO_TYPE_CHANGE_IMAGE_BITS:
			return UndoItem_setdat_layerImage_all(dst, 1);

		//キャンバスサイズ変更 (オフセット移動)
		case UNDO_TYPE_RESIZECANVAS_MOVEOFFSET:
			dst->val[0] = -src->val[0];
			dst->val[1] = -src->val[1];
			dst->val[2] = APPDRAW->imgw;
			dst->val[3] = APPDRAW->imgh;
			break;

		//キャンバスサイズ変更 (切り取り)
		case UNDO_TYPE_RESIZECANVAS_CROP:
			dst->val[0] = APPDRAW->imgw;
			dst->val[1] = APPDRAW->imgh;
			dst->val[2] = -src->val[2];
			dst->val[3] = -src->val[3];

			return UndoItem_setdat_layerImage_all(dst, 0);

		//キャンバス拡大縮小
		case UNDO_TYPE_SCALE_CANVAS:
			dst->val[0] = APPDRAW->imgw;
			dst->val[1] = APPDRAW->imgh;
			dst->val[2] = APPDRAW->imgdpi;
			
			if(settype == MUNDO_TYPE_REDO)
				return UndoItem_setdat_layer(dst, LayerList_getTopItem(APPDRAW->layerlist));
			else
				return UndoItem_setdat_layerAll(dst);
	}

	return MLKERR_OK;
}

/* (mUndo) アンドゥ/リドゥ実行 */

static mlkerr _run_handle(mUndo *undoptr,mListItem *runitem,int runtype)
{
	AppUndo *undo = (AppUndo *)undoptr;
	UndoItem *item = (UndoItem *)runitem;
	UndoUpdateInfo *update;

	update = &undo->update;

	//デフォルトで更新なし

	update->type = UNDO_UPDATE_NONE;

	mRectEmpty(&update->rc);

	//

	switch(item->type)
	{
		//タイルイメージ
		case UNDO_TYPE_TILEIMAGE:
			update->type = UNDO_UPDATE_RECT;
			update->rc.x1 = item->val[1];
			update->rc.y1 = item->val[2];
			update->rc.x2 = item->val[3];
			update->rc.y2 = item->val[4];
			update->layer = UndoItem_getLayerAtIndex(item->val[0]);
		
			return UndoItem_restore_tileimage(item, runtype);

		//レイヤ追加
		case UNDO_TYPE_LAYER_NEW:
			return UndoItem_runLayerNew(item, update, runtype);

		//レイヤ複製
		case UNDO_TYPE_LAYER_DUP:
			return UndoItem_runLayerDup(item, update, runtype);

		//レイヤ削除
		case UNDO_TYPE_LAYER_DELETE:
			return UndoItem_runLayerDelete(item, update, runtype);

		//レイヤイメージクリア
		case UNDO_TYPE_LAYER_CLEARIMG:
			return UndoItem_runLayerClearImage(item, update, runtype);

		//レイヤタイプ変更
		case UNDO_TYPE_LAYER_SETTYPE:
			return UndoItem_runLayerSetType(item, update, runtype);
	
		//レイヤタイプ変更 (テキストのみ)
		case UNDO_TYPE_LAYER_SETTYPE_TEXT:
			return UndoItem_runLayerSetType_text(item, update, runtype);

		//下レイヤに移す/結合
		case UNDO_TYPE_LAYER_DROP_AND_COMBINE:
			return UndoItem_runLayerDropAndCombine(item, update, runtype);
	
		//すべてのレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_ALL:
			return UndoItem_runLayerCombineAll(item, update, runtype);

		//フォルダレイヤ結合
		case UNDO_TYPE_LAYER_COMBINE_FOLDER:
			return UndoItem_runLayerCombineFolder(item, update, runtype);
	
		//レイヤのオフセット位置移動
		case UNDO_TYPE_LAYER_MOVE_OFFSET:
			return UndoItem_runLayerMoveOffset(item, update);

		//レイヤ全体の左右上下反転/回転
		case UNDO_TYPE_LAYER_EDIT_FULL:
			return UndoItem_runLayerEditFull(item, update);

		//単体レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST:
			return UndoItem_runLayerMoveList(item, update);

		//複数レイヤの順番移動
		case UNDO_TYPE_LAYER_MOVE_LIST_MULTI:
			return UndoItem_runLayerMoveList_multi(item, update, runtype);

		//レイヤテキスト新規
		case UNDO_TYPE_LAYERTEXT_NEW:
			return UndoItem_runLayerTextNew(item, update, runtype);

		//レイヤテキスト編集
		case UNDO_TYPE_LAYERTEXT_EDIT:
			return UndoItem_runLayerTextEdit(item, update);

		//レイヤテキスト位置移動
		case UNDO_TYPE_LAYERTEXT_MOVE:
			return UndoItem_runLayerTextMove(item, update);

		//レイヤテキスト削除
		case UNDO_TYPE_LAYERTEXT_DELETE:
			return UndoItem_runLayerTextDelete(item, update, runtype);

		//レイヤテキストクリア
		case UNDO_TYPE_LAYERTEXT_CLEAR:
			return UndoItem_runLayerTextClear(item, update, runtype);

		//イメージビット数の変更
		case UNDO_TYPE_CHANGE_IMAGE_BITS:
			return UndoItem_runChangeImageBits(item, update);

		//キャンバスサイズ変更 (オフセット移動)
		case UNDO_TYPE_RESIZECANVAS_MOVEOFFSET:
			LayerList_moveOffset_rel_all(APPDRAW->layerlist, item->val[0], item->val[1]);

			update->type = UNDO_UPDATE_CANVAS_RESIZE;
			update->rc.x1 = item->val[2];
			update->rc.y1 = item->val[3];
			break;

		//キャンバスサイズ変更 (切り取り)
		case UNDO_TYPE_RESIZECANVAS_CROP:
			return UndoItem_runCanvasResize(item, update);

		//キャンバス拡大縮小
		case UNDO_TYPE_SCALE_CANVAS:
			return UndoItem_runCanvasScale(item, update, runtype);
	}

	return MLKERR_OK;
}

/* アンドゥデータのセットに失敗した時 */

static void _on_failed(void)
{
	mUndoDeleteAll(&APPUNDO->undo);
}

/* アンドゥデータ追加 */

static mlkerr _add_item(int type,UndoItem **ppdst)
{
	mListItem *pi;
	mlkerr ret;

	ret = _newitem_handle(&APPUNDO->undo, &pi);
	if(ret)
	{
		_on_failed();
		return ret;
	}

	mUndoAdd(&APPUNDO->undo, pi);

	((UndoItem *)pi)->type = type;

	*ppdst = (UndoItem *)pi;

	return ret;
}


//========================
// main
//========================


/* (mList) アイテム破棄ハンドラ */

static void _destroy_item_handle(mList *list,mListItem *item)
{
	UndoItem_free((UndoItem *)item);
}

/* 作業用バッファ解放 */

static void _free_workbuf(AppUndo *p)
{
	mFree(p->writetmpbuf);
	mFree(p->workbuf1);
	mFree(p->workbuf2);
}

/** AppUndo 解放 */

void Undo_free(void)
{
	AppUndo *p = APPUNDO;

	if(p)
	{
		mUndoDeleteAll(&p->undo);

		_free_workbuf(p);
		mFree(p);

		g_app_undo = NULL;
	}
}

/** AppUndo 作成 */

int Undo_new(void)
{
	AppUndo *p;

	p = (AppUndo *)mMalloc0(sizeof(AppUndo));
	if(!p) return 1;

	p->undo.list.item_destroy = _destroy_item_handle;

	//バッファ確保

	p->writetmpbuf = (uint8_t *)mMalloc(UNDO_WRITETEMP_BUFSIZE);
	p->workbuf1 = (uint8_t *)mMalloc(64 * 64 * 8 + 1024);
	p->workbuf2 = (uint8_t *)mMalloc(64 * 64 * 8 + 1024);
	
	if(!p->writetmpbuf || !p->workbuf1 || !p->workbuf2)
	{
		_free_workbuf(p);
		mFree(p);
		return 1;
	}

	//
	
	g_app_undo = p;

	p->undo.maxnum = 20;
	p->undo.newitem = _newitem_handle;
	p->undo.set_reverse = _setreverse_handle;
	p->undo.run = _run_handle;

	return 0;
}

/** データ更新用フラグを OFF */

void Undo_setModifyFlag_off(void)
{
	APPUNDO->fmodify = FALSE;
}

/** アンドゥ最大回数をセット */

void Undo_setMaxNum(int num)
{
	APPUNDO->undo.maxnum = num;
}

/** アンドゥデータがあるか */

mlkbool Undo_isHaveUndo(void)
{
	return mUndoIsHaveUndo(&APPUNDO->undo);
}

/** リドゥデータがあるか */

mlkbool Undo_isHaveRedo(void)
{
	return mUndoIsHaveRedo(&APPUNDO->undo);
}

/** データが変更されたか */

mlkbool Undo_isModify(void)
{
	//フラグは、新規作成時やファイル保存時はクリアされる。
	//アンドゥの追加や、アンドゥ/リドゥ実行が行われた場合、
	//状態が変更されたということなので、保存が必要であると判断する。

	return APPUNDO->fmodify;
}

/** すべて削除 */

void Undo_deleteAll(void)
{
	mUndoDeleteAll(&APPUNDO->undo);
}

/** アンドゥ/リドゥ実行
 *
 * 実行処理中に失敗した場合は、中途半端な所で止まっている場合があるため、
 * 実行に失敗しても更新は行う。 */

mlkerr Undo_runUndoRedo(mlkbool redo,UndoUpdateInfo *info)
{
	mlkerr ret;

	if(redo)
		ret = mUndoRun_redo(&APPUNDO->undo);
	else
		ret = mUndoRun_undo(&APPUNDO->undo);

	*info = APPUNDO->update;

	return ret;
}


//========================
// アンドゥ追加
//========================


/** タイルイメージ
 *
 * rc: 更新されたイメージの範囲 */

mlkerr Undo_addTilesImage(TileImageInfo *info,mRect *rc)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_TILEIMAGE, &pi);
	if(ret) return ret;

	//[0] レイヤ番号 [1..4] 更新イメージ範囲

	UndoItem_setval_curlayer(pi, 0);

	pi->val[1] = rc->x1;
	pi->val[2] = rc->y1;
	pi->val[3] = rc->x2;
	pi->val[4] = rc->y2;

	ret = UndoItem_setdat_tileimage(pi, info);
	if(ret)
		_on_failed();

	return ret;
}

/** 新規レイヤ追加 */

mlkerr Undo_addLayerNew(LayerItem *item)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_NEW, &pi);
	if(ret) return ret;

	//[0][1] レイヤ番号 [2] 追加時にイメージがあるか

	UndoItem_setval_layerno_forParent(pi, 0, item);

	pi->val[2] = (LAYERITEM_IS_IMAGE(item) && TileImage_getHaveTileNum(item->img));

	return MLKERR_OK;
}

/** レイヤ複製 */

mlkerr Undo_addLayerDup(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_DUP, &pi);
	if(ret) return ret;

	UndoItem_setval_curlayer(pi, 0);

	return MLKERR_OK;
}

/** レイヤ削除 */

mlkerr Undo_addLayerDelete(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_DELETE, &pi);
	if(ret) return ret;

	UndoItem_setval_curlayer(pi, 0);

	ret = UndoItem_setdat_layerForFolder(pi, APPDRAW->curlayer);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤイメージクリア */

mlkerr Undo_addLayerClearImage(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_CLEARIMG, &pi);
	if(ret) return ret;

	UndoItem_setval_curlayer(pi, 0);

	ret = UndoItem_setdat_layerImage(pi, APPDRAW->curlayer);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤタイプ変更 (変換前)
 *
 * type: 変換後のタイプ
 * only_text: 元がテキストレイヤで、同じタイプへの変更時 */

mlkerr Undo_addLayerSetType(LayerItem *item,int type,int only_text)
{
	UndoItem *pi;
	mlkerr ret;

	if(only_text)
	{
		//---- テキストレイヤ -> 通常レイヤ (同じタイプ)

		ret = _add_item(UNDO_TYPE_LAYER_SETTYPE_TEXT, &pi);
		if(ret) return ret;

		UndoItem_setval_layerno(pi, 0, item);

		ret = UndoItem_setdat_layerTextAll(pi, item);
	}
	else
	{
		//----- タイプの変更あり

		ret = _add_item(UNDO_TYPE_LAYER_SETTYPE, &pi);
		if(ret) return ret;

		//[0] レイヤ番号
		//[1] 元のカラータイプ
		//[2] 変換後のカラータイプ
		//[3] 元レイヤがテキストレイヤか

		UndoItem_setval_layerno(pi, 0, item);

		pi->val[1] = item->type;
		pi->val[2] = type;
		pi->val[3] = LAYERITEM_IS_TEXT(item);

		//

		ret = UndoItem_setdat_layerTextAndImage(pi, item);
	}

	if(ret)
		_on_failed();

	return ret;
}

/** 下レイヤに移す/結合 */

mlkerr Undo_addLayerDropAndCombine(mlkbool drop)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_DROP_AND_COMBINE, &pi);
	if(ret) return ret;

	//[0] 下レイヤに移すか [1][2] レイヤ番号

	pi->val[0] = drop;
	
	UndoItem_setval_layerno_forParent(pi, 1, APPDRAW->curlayer);

	//

	if(drop)
		ret = UndoItem_setdat_layerImage_double(pi, APPDRAW->curlayer);
	else
		ret = UndoItem_setdat_layerForCombine(pi, MUNDO_TYPE_UNDO);

	if(ret) _on_failed();

	return ret;
}

/** すべてのレイヤの結合/画像統合 */

mlkerr Undo_addLayerCombineAll(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_COMBINE_ALL, &pi);
	if(ret) return ret;

	ret = UndoItem_setdat_layerAll(pi);
	if(ret)
		_on_failed();

	return ret;
}

/** フォルダレイヤの結合 */

mlkerr Undo_addLayerCombineFolder(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_COMBINE_FOLDER, &pi);
	if(ret) return ret;

	//[0][1] レイヤ番号

	UndoItem_setval_layerno_forParent(pi, 0, APPDRAW->curlayer);

	ret = UndoItem_setdat_layerForFolder(pi, APPDRAW->curlayer);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤ全体の左右上下反転/回転 */

mlkerr Undo_addLayerEditFull(int type)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_EDIT_FULL, &pi);
	if(ret) return ret;

	//[0] レイヤ番号 [1] タイプ

	UndoItem_setval_curlayer(pi, 0);

	pi->val[1] = type;

	//回転時はイメージ保存

	if(type >= 2)
	{
		ret = UndoItem_setdat_layerImage_folder(pi, APPDRAW->curlayer);
		if(ret)
		{
			_on_failed();
			return ret;
		}
	}

	return MLKERR_OK;
}

/** 複数レイヤのオフセット位置移動
 *
 * item: リンクの先頭 */

mlkerr Undo_addLayerMoveOffset(int mx,int my,LayerItem *item)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_MOVE_OFFSET, &pi);
	if(ret) return ret;

	//[0] x 相対移動数
	//[1] y 相対移動数
	//[2] 対象レイヤ数
	//[3] レイヤ数が1なら、レイヤ番号

	pi->val[0] = -mx;
	pi->val[1] = -my;
	pi->val[2] = LayerItem_getLinkNum(item);

	if(pi->val[2] == 1)
	{
		pi->val[3] = LayerList_getItemIndex(APPDRAW->layerlist, item);
		
		ret = MLKERR_OK;
	}
	else
	{
		//複数レイヤ
		
		ret = UndoItem_setdat_link_layerno(pi, item, pi->val[2]);
		if(ret)
			_on_failed();
	}

	return ret;
}

/** 単体レイヤの順番移動 (移動後に呼び出す)
 *
 * item: 移動したレイヤ
 * parent,pos: 移動前のレイヤ位置 */

mlkerr Undo_addLayerMoveList(LayerItem *item,int parent,int pos)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_MOVE_LIST, &pi);
	if(ret) return ret;

	//[0] 移動後の親位置
	//[1] 移動後の相対位置
	//[2] 移動前の親位置
	//[3] 移動前の相対位置

	UndoItem_setval_layerno_forParent(pi, 0, item);

	pi->val[2] = parent;
	pi->val[3] = pos;

	return MLKERR_OK;
}

/** 複数レイヤの順番移動
 *
 * dat: int16 x 4 x num (移動前と移動後の位置) */

mlkerr Undo_addLayerMoveList_multi(int num,int16_t *dat)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYER_MOVE_LIST_MULTI, &pi);
	if(ret) return ret;

	ret = UndoItem_setdat_movelayerlist_multi(pi, num, dat);
	if(ret)
		_on_failed();

	return ret;
}

//-------------

/** レイヤテキストの新規追加 (追加後) */

mlkerr Undo_addLayerTextNew(TileImageInfo *info)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYERTEXT_NEW, &pi);
	if(ret) return ret;

	//[0] レイヤ番号
	//[1] テキストインデックス
	//[2] データサイズ
	//[3] x
	//[4] y

	UndoItem_setval_curlayer(pi, 0);

	pi->val[1] = APPDRAW->curlayer->list_text.num - 1;

	//

	ret = UndoItem_setdat_layertext_single_first(pi, info);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤテキストの編集 (編集前) */

mlkerr Undo_addLayerTextEdit(LayerTextItem *text)
{
	UndoItem *pi;
	TileImageInfo info;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYERTEXT_EDIT, &pi);
	if(ret) return ret;

	//[0] レイヤ番号
	//[1] テキストインデックス
	//[2] データサイズ
	//[3] x
	//[4] y

	UndoItem_setval_curlayer(pi, 0);

	pi->val[1] = mListItemGetIndex(MLISTITEM(text));

	//

	TileImage_getInfo(APPDRAW->curlayer->img, &info);

	ret = UndoItem_setdat_layertext_single_first(pi, &info);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤテキストのテキスト位置移動 (移動前) */

mlkerr Undo_addLayerTextMove(LayerTextItem *text)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYERTEXT_MOVE, &pi);
	if(ret) return ret;

	//[0] レイヤ番号
	//[1] テキストインデックス
	//[3] x
	//[4] y

	UndoItem_setval_curlayer(pi, 0);

	pi->val[1] = mListItemGetIndex(MLISTITEM(text));

	//

	ret = UndoItem_setdat_layertext_move(pi);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤテキストのテキスト削除 */

mlkerr Undo_addLayerTextDelete(LayerTextItem *text)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYERTEXT_DELETE, &pi);
	if(ret) return ret;

	//[0] レイヤ番号
	//[1] テキストインデックス
	//[2] データサイズ
	//[3] x
	//[4] y

	UndoItem_setval_curlayer(pi, 0);

	pi->val[1] = mListItemGetIndex(MLISTITEM(text));

	//(TileImageInfo は必要ないが、あっても問題はない)

	ret = UndoItem_setdat_layertext_single(pi);
	if(ret)
		_on_failed();

	return ret;
}

/** レイヤテキスト、すべてクリア */

mlkerr Undo_addLayerTextClear(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_LAYERTEXT_CLEAR, &pi);
	if(ret) return ret;

	UndoItem_setval_curlayer(pi, 0);

	ret = UndoItem_setdat_layerTextAndImage(pi, APPDRAW->curlayer);
	if(ret)
		_on_failed();

	return ret;
}

//-----------

/** イメージビット数の変更 */

mlkerr Undo_addChangeImageBits(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_CHANGE_IMAGE_BITS, &pi);
	if(ret) return ret;

	ret = UndoItem_setdat_layerImage_all(pi, 1);
	if(ret)
		_on_failed();

	return ret;
}

/** キャンバスサイズ変更
 *
 * 範囲外を切り取らず、レイヤ全体をオフセット移動した時。 */

mlkerr Undo_addResizeCanvas_moveOffset(int mx,int my,int w,int h)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_RESIZECANVAS_MOVEOFFSET, &pi);
	if(ret) return ret;

	pi->val[0] = -mx;
	pi->val[1] = -my;
	pi->val[2] = w;
	pi->val[3] = h;

	return MLKERR_OK;
}

/** キャンバスサイズ変更 (範囲外切り取り時) */

mlkerr Undo_addResizeCanvas_crop(int mx,int my,int w,int h)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_RESIZECANVAS_CROP, &pi);
	if(ret) return ret;

	pi->val[0] = w;
	pi->val[1] = h;
	pi->val[2] = -mx;
	pi->val[3] = -my;

	//テキストレイヤ以外の全イメージ

	ret = UndoItem_setdat_layerImage_all(pi, 0);
	if(ret)
		_on_failed();

	return ret;
}

/** キャンバス拡大縮小 */

mlkerr Undo_addScaleCanvas(void)
{
	UndoItem *pi;
	mlkerr ret;

	ret = _add_item(UNDO_TYPE_SCALE_CANVAS, &pi);
	if(ret) return ret;

	pi->val[0] = APPDRAW->imgw;
	pi->val[1] = APPDRAW->imgh;
	pi->val[2] = APPDRAW->imgdpi;

	ret = UndoItem_setdat_layerAll(pi);
	if(ret)
		_on_failed();

	return ret;
}

