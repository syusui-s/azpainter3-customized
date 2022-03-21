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

/********************************
 * アンドゥアイテム
 ********************************/

typedef struct _LayerItem LayerItem;
typedef struct _TileImage TileImage;
typedef struct _UndoUpdateInfo UndoUpdateInfo;

#define UNDOITEM_VAL_NUM  5

/** UNDO アイテム */

typedef struct
{
	mListItem i;

	uint8_t *buf;	//データのバッファ
	uint32_t size;	//バッファのデータサイズ
	int type,		//データの種類
		fileno,		//ファイル保存時、ファイル名の番号 (-1 でバッファ)
		val[UNDOITEM_VAL_NUM];	//データ値
}UndoItem;

/** アンドゥ種類 */

enum
{
	UNDO_TYPE_TILEIMAGE,				//タイル単位のイメージ
	UNDO_TYPE_LAYER_NEW,				//レイヤ追加
	UNDO_TYPE_LAYER_DUP,				//レイヤ複製
	UNDO_TYPE_LAYER_DELETE,				//レイヤ削除
	UNDO_TYPE_LAYER_CLEARIMG,			//レイヤクリア
	UNDO_TYPE_LAYER_SETTYPE,			//レイヤタイプ変更
	UNDO_TYPE_LAYER_SETTYPE_TEXT,		//レイヤタイプ変更 (テキスト -> 通常レイヤ)
	UNDO_TYPE_LAYER_DROP_AND_COMBINE,	//下レイヤに移す、下レイヤに結合
	UNDO_TYPE_LAYER_COMBINE_ALL,		//すべてのレイヤ結合/画像統合
	UNDO_TYPE_LAYER_COMBINE_FOLDER,		//フォルダ内のレイヤ結合
	UNDO_TYPE_LAYER_EDIT_FULL,			//レイヤ全体の左右上下反転/回転
	UNDO_TYPE_LAYER_MOVE_OFFSET,		//レイヤのオフセット位置移動
	UNDO_TYPE_LAYER_MOVE_LIST,			//レイヤの順番移動
	UNDO_TYPE_LAYER_MOVE_LIST_MULTI,	//複数レイヤの順番移動

	//レイヤテキスト
	UNDO_TYPE_LAYERTEXT_NEW,	//新規追加
	UNDO_TYPE_LAYERTEXT_EDIT,	//編集
	UNDO_TYPE_LAYERTEXT_MOVE,	//位置移動
	UNDO_TYPE_LAYERTEXT_DELETE,	//削除
	UNDO_TYPE_LAYERTEXT_CLEAR,	//クリア
	
	UNDO_TYPE_CHANGE_IMAGE_BITS,		//イメージビット数変更
	UNDO_TYPE_RESIZECANVAS_MOVEOFFSET,	//キャンバスサイズ変更 (オフセット移動のみ)
	UNDO_TYPE_RESIZECANVAS_CROP,		//キャンバスサイズ変更 (範囲外切り取り)
	UNDO_TYPE_SCALE_CANVAS				//キャンバス拡大縮小
};

/** データ確保タイプ (UndoItem_alloc() 時のサイズ) */

enum
{
	UNDO_ALLOC_FILE = -1,			//常にファイル
	UNDO_ALLOC_MEM_VARIABLE = -2	//メモリ優先、可変サイズ
};


/*---------------*/

/** undoitem_run.c */

mlkerr UndoItem_runLayerNew(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerDup(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerDelete(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerClearImage(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerSetType(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerSetType_text(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerDropAndCombine(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerCombineAll(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerCombineFolder(UndoItem *p,UndoUpdateInfo *info,int runtype);

mlkerr UndoItem_runLayerEditFull(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runLayerMoveOffset(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runLayerMoveList(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runLayerMoveList_multi(UndoItem *p,UndoUpdateInfo *info,int runtype);

mlkerr UndoItem_runChangeImageBits(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runCanvasResize(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runCanvasScale(UndoItem *p,UndoUpdateInfo *info,int runtype);

mlkerr UndoItem_runLayerTextNew(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerTextEdit(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runLayerTextMove(UndoItem *p,UndoUpdateInfo *info);
mlkerr UndoItem_runLayerTextDelete(UndoItem *p,UndoUpdateInfo *info,int runtype);
mlkerr UndoItem_runLayerTextClear(UndoItem *p,UndoUpdateInfo *info,int runtype);

/** undoitem_dat.c */

mlkerr UndoItem_setdat_movelayerlist_multi(UndoItem *p,int num,int16_t *buf);

mlkerr UndoItem_setdat_link_layerno(UndoItem *p,LayerItem *item,int num);
mlkerr UndoItem_restore_link_layerno(UndoItem *p,int num,LayerItem **pptop);

mlkerr UndoItem_setdat_layer(UndoItem *p,LayerItem *item);
mlkerr UndoItem_restore_layer(UndoItem *p,int parent,int pos,mRect *rc);
mlkerr UndoItem_restore_layer_overwrite(UndoItem *p,LayerItem *item,mRect *rc);

mlkerr UndoItem_setdat_newLayer_redo(UndoItem *p);
mlkerr UndoItem_restore_newLayerEmpty(UndoItem *p);

mlkerr UndoItem_setdat_layerImage(UndoItem *p,LayerItem *item);
mlkerr UndoItem_restore_layerImage(UndoItem *p,LayerItem *item);

mlkerr UndoItem_setdat_layerImage_folder(UndoItem *p,LayerItem *item);
mlkerr UndoItem_setdat_layerImage_all(UndoItem *p,int type);
mlkerr UndoItem_restore_layerImage_multi(UndoItem *p,mRect *rcupdate);

mlkerr UndoItem_setdat_layerImage_double(UndoItem *p,LayerItem *item);
mlkerr UndoItem_restore_layerImage_double(UndoItem *p,LayerItem *item);

mlkerr UndoItem_setdat_layerForCombine(UndoItem *p,int settype);
mlkerr UndoItem_restore_layerForCombine(UndoItem *p,int runtype,LayerItem *item,mRect *rcupdate);

mlkerr UndoItem_setdat_layerForFolder(UndoItem *p,LayerItem *item);
mlkerr UndoItem_setdat_layerAll(UndoItem *p);
mlkerr UndoItem_restore_layerMulti(UndoItem *p,mRect *rcupdate);

mlkerr UndoItem_setdat_layertext_single_first(UndoItem *p,TileImageInfo *info);
mlkerr UndoItem_setdat_layertext_single(UndoItem *p);
mlkerr UndoItem_setdat_layertext_new_reverse(UndoItem *dst,UndoItem *src);
mlkerr UndoItem_setdat_layertext_move(UndoItem *p);
mlkerr UndoItem_restore_layertext_tileimg(UndoItem *p,LayerItem *layer);
mlkerr UndoItem_restore_layertext_single(UndoItem *p,LayerItem *layer,LayerTextItem *item,LayerTextItem **dst);

mlkerr UndoItem_setdat_layerTextAll(UndoItem *p,LayerItem *item);
mlkerr UndoItem_restore_layerTextAll(UndoItem *p,LayerItem *item);

mlkerr UndoItem_setdat_layerTextAndImage(UndoItem *p,LayerItem *item);
mlkerr UndoItem_restore_layerTextAndImage(UndoItem *p,LayerItem *item,mRect *rcupdate);

/** undoitem_sub.c */

LayerItem *UndoItem_getLayerAtIndex(int no);
LayerItem *UndoItem_getLayerAtIndex_forParent(int parent,int pos);
mlkbool UndoItem_getLayerText_atIndex(int layerno,int index,LayerItem **pplayer,LayerTextItem **pptext);

void UndoItem_setval_curlayer(UndoItem *p,int valno);
void UndoItem_setval_layerno(UndoItem *p,int valno,LayerItem *item);
void UndoItem_setval_layerno_forParent(UndoItem *p,int valno,LayerItem *item);

int UndoItem_getLayerInfoSize(LayerItem *li);

mlkerr UndoItem_beginWrite_file_zlib(UndoItem *p);
void UndoItem_endWrite_file_zlib(UndoItem *p);

mlkerr UndoItem_beginRead_file_zlib(UndoItem *p);
void UndoItem_endRead_file_zlib(UndoItem *p);

mlkerr UndoItem_writeLayerText(UndoItem *p,LayerItem *li);
mlkerr UndoItem_readLayerText(UndoItem *p,LayerItem *li);

mlkerr UndoItem_writeLayerInfoAndImage(UndoItem *p,LayerItem *item);
mlkerr UndoItem_writeLayerInfo(UndoItem *p,LayerItem *li);
mlkerr UndoItem_writeTileImage(UndoItem *p,TileImage *img);

mlkerr UndoItem_readLayerInfo_new(UndoItem *p,int parent_pos,int rel_pos,LayerItem **ppdst,int *is_folder);
mlkerr UndoItem_readLayer(UndoItem *p,int parent_pos,int rel_pos,mRect *rcupdate);
mlkerr UndoItem_readLayer_overwrite(UndoItem *p,LayerItem *item,mRect *rcupdate);
mlkerr UndoItem_readLayerImage(UndoItem *p,LayerItem *item);

/** undoitem_tileimg.c */

mlkerr UndoItem_setdat_tileimage(UndoItem *p,TileImageInfo *info);
mlkerr UndoItem_setdat_tileimage_reverse(UndoItem *dst,UndoItem *src,int settype);
mlkerr UndoItem_restore_tileimage(UndoItem *p,int runtype);

/** undoitem_base.c */

mlkerr UndoItem_alloc(UndoItem *p,int size);
void UndoItem_free(UndoItem *p);

mlkerr UndoItem_writeFixSize(UndoItem *p,int size,mlkerr (*func)(UndoItem *,void *),void *param);
mlkerr UndoItem_writeFull_buf(UndoItem *p,void *buf,int size);
mlkerr UndoItem_readTop(UndoItem *p,int size,void **ppdst);
mlkerr UndoItem_copyData(UndoItem *src,UndoItem *dst,int size);

mlkerr UndoItem_allocOpenWrite_variable(UndoItem *p);
mlkerr UndoItem_closeWrite_variable(UndoItem *p,mlkerr err);

mlkerr UndoItem_openWrite(UndoItem *p);
mlkerr UndoItem_write(UndoItem *p,const void *buf,int size);
mlkerr UndoItem_closeWrite(UndoItem *p);

mlkerr UndoItem_writeEncSize_temp(void);
void UndoItem_writeEncSize_real(uint32_t size);

mlkerr UndoItem_openRead(UndoItem *p);
mlkerr UndoItem_read(UndoItem *p,void *buf,int size);
void UndoItem_readSeek(UndoItem *p,int seek);
void UndoItem_closeRead(UndoItem *p);

