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
 * アンドゥ操作
 ********************************/

typedef struct _LayerItem LayerItem;
typedef struct _LayerTextItem LayerTextItem;
typedef struct _TileImageInfo TileImageInfo;

/* アンドゥ処理後の更新情報 */

typedef struct _UndoUpdateInfo
{
	int type;		//更新のタイプ
	mRect rc;		//更新するイメージ範囲
	LayerItem *layer;	//指定レイヤ (NULL でなし)
}UndoUpdateInfo;

/* 更新のタイプ */

enum
{
	UNDO_UPDATE_NONE,		//更新なし
	UNDO_UPDATE_RECT,		//単体レイヤの一部イメージ (layer != NULL で、一覧上の指定レイヤ更新 [パラメータ部分は更新しない])
	UNDO_UPDATE_RECT_LAYERLIST,		//指定範囲 + レイヤ一覧
	UNDO_UPDATE_LAYERLIST,			//レイヤ一覧のみ更新
	UNDO_UPDATE_FULL_LAYERLIST,		//イメージ全体とレイヤ一覧
	UNDO_UPDATE_IMAGEBITS,			//イメージビット数の変更
	UNDO_UPDATE_CANVAS_RESIZE		//キャンバスサイズ変更 (rc.x1 = w, rc.y1 = h)
};

/*--------*/

int Undo_new(void);
void Undo_free(void);

void Undo_setModifyFlag_off(void);
void Undo_setMaxNum(int num);

mlkbool Undo_isHaveUndo(void);
mlkbool Undo_isHaveRedo(void);
mlkbool Undo_isModify(void);

void Undo_deleteAll(void);
mlkerr Undo_runUndoRedo(mlkbool redo,UndoUpdateInfo *info);

mlkerr Undo_addTilesImage(TileImageInfo *info,mRect *rc);

mlkerr Undo_addLayerNew(LayerItem *item);
mlkerr Undo_addLayerDup(void);
mlkerr Undo_addLayerDelete(void);
mlkerr Undo_addLayerClearImage(void);
mlkerr Undo_addLayerSetType(LayerItem *item,int type,int only_text);
mlkerr Undo_addLayerDropAndCombine(mlkbool drop);
mlkerr Undo_addLayerCombineAll(void);
mlkerr Undo_addLayerCombineFolder(void);
mlkerr Undo_addLayerEditFull(int type);
mlkerr Undo_addLayerMoveOffset(int mx,int my,LayerItem *item);
mlkerr Undo_addLayerMoveList(LayerItem *item,int parent,int pos);
mlkerr Undo_addLayerMoveList_multi(int num,int16_t *dat);

mlkerr Undo_addLayerTextNew(TileImageInfo *info);
mlkerr Undo_addLayerTextEdit(LayerTextItem *text);
mlkerr Undo_addLayerTextMove(LayerTextItem *text);
mlkerr Undo_addLayerTextDelete(LayerTextItem *text);
mlkerr Undo_addLayerTextClear(void);

mlkerr Undo_addChangeImageBits(void);
mlkerr Undo_addResizeCanvas_moveOffset(int mx,int my,int w,int h);
mlkerr Undo_addResizeCanvas_crop(int mx,int my,int w,int h);
mlkerr Undo_addScaleCanvas(void);

