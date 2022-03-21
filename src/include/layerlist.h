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

/************************************
 * レイヤリスト
 ************************************/

#ifndef AZPT_LAYERLIST_H
#define AZPT_LAYERLIST_H

typedef struct _LayerList LayerList;
typedef struct _LayerItem LayerItem;
typedef struct _TileImage TileImage;
typedef struct _mPopupProgress mPopupProgress;


LayerList *LayerList_new(void);
void LayerList_free(LayerList *p);

void LayerList_clear(LayerList *p);

/* add, etc */

LayerItem *LayerList_addLayer(LayerList *p,LayerItem *insert);
LayerItem *LayerList_addLayer_parent(LayerList *p,LayerItem *parent);
LayerItem *LayerList_addLayer_image(LayerList *p,LayerItem *insert,int imgtype,int w,int h);
LayerItem *LayerList_addLayer_index(LayerList *p,int parent,int pos);
LayerItem *LayerList_addLayer_newimage(LayerList *p,int type);
mlkbool LayerList_addLayers_onRoot(LayerList *p,int num);

LayerItem *LayerList_dupLayer(LayerList *p,LayerItem *src);
LayerItem *LayerList_deleteLayer(LayerList *p,LayerItem *item);

/* 取得 */

int LayerList_getNum(LayerList *p);
int LayerList_getNormalLayerNum(LayerList *p);
int LayerList_getBlendLayerNum(LayerList *p);

int LayerList_getItemIndex(LayerList *p,LayerItem *item);
void LayerList_getItemIndex_forParent(LayerList *p,LayerItem *item,int *parent,int *relno);

LayerItem *LayerList_getTopItem(LayerList *p);
LayerItem *LayerList_getBottomLastItem(LayerList *p);
LayerItem *LayerList_getItemAtIndex(LayerList *p,int pos);
LayerItem *LayerList_getItem_bottomVisibleImage(LayerList *p);
void LayerList_getItems_fromIndex(LayerList *p,LayerItem **dst,int parent,int pos);
LayerItem *LayerList_getItem_topPixelLayer(LayerList *p,int x,int y);

int LayerList_getScrollInfo(LayerList *p,int *ret_maxdepth);

TileImage *LayerList_getMaskUnderImage(LayerList *p,LayerItem *current);

/* フラグなど */

mlkbool LayerList_haveCheckedLayer(LayerList *p);
void LayerList_setVisible_all(LayerList *p,mlkbool on);
void LayerList_setVisible_all_checked_rev(LayerList *p,mRect *rcimg);
void LayerList_setVisible_all_img_rev(LayerList *p,mRect *rcimg);
void LayerList_folder_open_all(LayerList *p);
void LayerList_folder_close_all(LayerList *p,LayerItem *curitem);
void LayerList_setFlags_all_off(LayerList *p,uint32_t flags);
void LayerList_replaceToneLines_all(LayerList *p,int src,int dst);

/* リンク */

LayerItem *LayerList_setLink_checked(LayerList *p);
LayerItem *LayerList_setLink_checked_nolock(LayerList *p);
LayerItem *LayerList_setLink_combineMulti(LayerList *p,int target,LayerItem *current);
LayerItem *LayerList_setLink_movetool_single(LayerList *p,LayerItem *item);
LayerItem *LayerList_setLink_movetool_all(LayerList *p);
LayerItem *LayerList_setLink_filltool(LayerList *p,LayerItem *current,int type);

/* アイテム移動 */

mlkbool LayerList_moveitem_updown(LayerList *p,LayerItem *item,mlkbool up);
void LayerList_moveitem(LayerList *p,LayerItem *src,LayerItem *dst,mlkbool in_folder);
mlkbool LayerList_moveitem_forUndo(LayerList *p,int *val,mRect *rcupdate);
int16_t *LayerList_moveitem_checked_to_folder(LayerList *p,LayerItem *dst,int *pnum,mRect *rcupdate);
mlkerr LayerList_moveitem_multi_forUndo(LayerList *p,int num,int16_t *buf,mRect *rcupdate,mlkbool redo);

/* ほか */

void LayerList_setItemName_curlayernum(LayerList *p,LayerItem *item);
void LayerList_setItemName_shiftjis(LayerList *p,LayerItem *item,const char *name);

void LayerList_moveOffset_rel_all(LayerList *p,int movx,int movy);
void LayerList_moveOffset_rel_text(LayerList *p,int movx,int movy);
void LayerList_convertImageBits(LayerList *p,int bits,mPopupProgress *prog);

#endif
