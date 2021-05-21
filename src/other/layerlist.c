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
 * レイヤリスト
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_tree.h"
#include "mlk_list.h"
#include "mlk_rectbox.h"
#include "mlk_charset.h"

#include "def_config.h"
#include "def_draw.h"

#include "layerlist.h"
#include "layeritem.h"
#include "materiallist.h"
#include "tileimage.h"
#include "def_tileimage.h"


/*
 * - レイヤ数が増減した場合、LayerList::num (レイヤ総数) の値を変更すること。
 */

//---------------------

struct _LayerList
{
	mTree tree;
	int num;
};

//---------------------

#define _TOPITEM(p)        ((LayerItem *)p->tree.top)
#define _NEXT_ITEM(p)      (LayerItem *)((p)->i.next)
#define _NEXT_TREEITEM(p)  (LayerItem *)mTreeItemGetNext((mTreeItem *)(p))
#define _PREV_TREEITEM(p)  (LayerItem *)mTreeItemGetPrev((mTreeItem *)(p))

//---------------------



//=========================
// sub
//=========================


/* ツリーアイテム 破棄ハンドラ */

static void _destroy_item(mTree *tree,mTreeItem *item)
{
	LayerItem *p = (LayerItem *)item;

	if(APPDRAW->masklayer)
		APPDRAW->masklayer = NULL;

	if(APPDRAW->ttip_layer)
		APPDRAW->ttip_layer = NULL;

	//レイヤテクスチャ解放

	MaterialList_releaseImage(APPDRAW->list_material, MATERIALLIST_TYPE_TEXTURE, p->img_texture);

	//

	TileImage_free(p->img);

	mListDeleteAll(&p->list_text);

	mFree(p->name);
	mFree(p->texture_path);
}

/* LayerItem 確保 (ツリーへのリンクは行わない) */

static LayerItem *_item_new(LayerList *list)
{
	LayerItem *p;

	if(list->num >= 2000) return NULL;

	p = (LayerItem *)mMalloc0(sizeof(LayerItem));
	if(!p) return NULL;

	p->opacity = 128;
	p->flags = LAYERITEM_F_VISIBLE;

	p->tone_lines = APPCONF->tone_lines_default;
	p->tone_angle = 45;

	return p;
}


//=========================
// main
//=========================


/** レイヤリスト作成 */

LayerList *LayerList_new(void)
{
	LayerList *p;

	p = (LayerList *)mMalloc0(sizeof(LayerList));
	if(!p) return NULL;

	p->tree.item_destroy = _destroy_item;

	return p;
}

/** レイヤリスト解放 */

void LayerList_free(LayerList *p)
{
	if(p)
	{
		mTreeDeleteAll(&p->tree);
		mFree(p);
	}
}

/** クリア */

void LayerList_clear(LayerList *p)
{
	mTreeDeleteAll(&p->tree);
	p->num = 0;
}

/** レイヤ追加
 *
 * イメージは作成しない。
 * フォルダ内に追加されても、展開状態はそのままになるため、手動でセットすること。
 *
 * insert: 挿入先。NULL でルートの最後。
 *  フォルダの場合、フォルダ内の一番上。
 *  通常レイヤの場合、その上。([!] ファイル読み込み時にも使う) */

LayerItem *LayerList_addLayer(LayerList *p,LayerItem *insert)
{
	LayerItem *pi;

	//アイテム作成

	pi = _item_new(p);
	if(!pi) return NULL;

	//リンク

	if(!insert)
	{
		//insert = NULL: ルートの最後に追加

		mTreeLinkBottom(&p->tree, NULL, MTREEITEM(pi));
	}
	else if(LAYERITEM_IS_FOLDER(insert))
	{
		//insert = フォルダ: フォルダの先頭へ

		mTreeLinkInsert_parent(&p->tree, MTREEITEM(insert), insert->i.first, MTREEITEM(pi));
	}
	else
		//insert の上に挿入
		mTreeLinkInsert(&p->tree, MTREEITEM(insert), MTREEITEM(pi));

	//個数

	p->num++;

	return pi;
}

/** レイヤ追加 (親の終端へ)
 *
 * [!] 親がフォルダでない場合は、追加しない。
 *
 * parent: NULL でルート */

LayerItem *LayerList_addLayer_parent(LayerList *p,LayerItem *parent)
{
	LayerItem *pi;

	if(parent && !LAYERITEM_IS_FOLDER(parent))
		return NULL;

	//

	pi = _item_new(p);
	if(!pi) return NULL;

	mTreeLinkBottom(&p->tree, MTREEITEM(parent), MTREEITEM(pi));

	p->num++;

	return pi;
}

/** レイヤ追加 (指定タイプのイメージを作成。名前はセットしない)
 *
 * return: イメージの作成に失敗時は NULL */

LayerItem *LayerList_addLayer_image(LayerList *p,LayerItem *insert,int imgtype,int w,int h)
{
	LayerItem *pi;
	TileImage *img;

	img = TileImage_new(imgtype, w, h);
	if(!img) return NULL;

	pi = LayerList_addLayer(p, insert);
	if(!pi)
	{
		TileImage_free(img);
		return NULL;
	}

	LayerItem_replaceImage(pi, img, imgtype);

	return pi;
}

/** 挿入位置をツリー上のインデックス番号で指定して、レイヤ追加 (ファイル読み込み/UNDO時)
 *
 * parent: 親の番号。-1 でルート
 * pos: 挿入位置の番号。-1 で終端 */

LayerItem *LayerList_addLayer_index(LayerList *p,int parent,int pos)
{
	LayerItem *pi,*ins[2];

	//作成

	pi = _item_new(p);
	if(!pi) return NULL;

	//リンク

	LayerList_getItems_fromIndex(p, ins, parent, pos);

	if(ins[1])
		//挿入
		mTreeLinkInsert(&p->tree, MTREEITEM(ins[1]), MTREEITEM(pi));
	else
		//末尾
		mTreeLinkBottom(&p->tree, MTREEITEM(ins[0]), MTREEITEM(pi));

	p->num++;

	return pi;
}

/** レイヤ追加 (新規キャンバス時のデフォルトレイヤ) */

LayerItem *LayerList_addLayer_newimage(LayerList *p,int type)
{
	LayerItem *pi;
	TileImage *img;

	//イメージ

	img = TileImage_new(type, 1, 1);
	if(!img) return NULL;

	//レイヤ

	pi = LayerList_addLayer(p, NULL);
	if(!pi)
	{
		TileImage_free(img);
		return NULL;
	}

	LayerItem_replaceImage(pi, img, type);

	//名前

	LayerItem_setName_layerno(pi, 0);

	return pi;
}

/** 親をルートとして、複数個のレイヤを追加 (APDv3) */

mlkbool LayerList_addLayers_onRoot(LayerList *p,int num)
{
	for(; num > 0; num--)
	{
		if(!LayerList_addLayer(p, NULL))
			return FALSE;
	}

	return TRUE;
}


//=========================
// レイヤ操作
//=========================


/** レイヤを複製 */

LayerItem *LayerList_dupLayer(LayerList *p,LayerItem *src)
{
	LayerItem *pi;
	TileImage *img;

	//イメージを複製

	img = TileImage_newClone(src->img);
	if(!img) return NULL;

	//レイヤ追加

	pi = LayerList_addLayer(p, src);
	if(!pi)
	{
		TileImage_free(img);
		return NULL;
	}

	//情報

	LayerItem_replaceImage(pi, img, src->type);
	LayerItem_copyInfo(pi, src);

	pi->img_texture = MaterialList_getImage(APPDRAW->list_material, MATERIALLIST_TYPE_LAYER_TEXTURE, pi->texture_path, TRUE);

	//テキストを複製

	LayerItem_appendText_dup(pi, src);

	return pi;
}

/** レイヤ削除
 *
 * return: 削除後の選択レイヤ (item がカレントとみなす) */

LayerItem *LayerList_deleteLayer(LayerList *p,LayerItem *item)
{
	LayerItem *next;

	next = LayerItem_getNextPass(item);
	if(!next)
		next = LayerItem_getPrevOpened(item);

	//削除

	mTreeDeleteItem(&p->tree, MTREEITEM(item));

	//アイテム数再計算

	p->num = mTreeItemGetNum(&p->tree);

	return next;
}


//================================
// 取得
//================================


/** レイヤの総数を取得 */

int LayerList_getNum(LayerList *p)
{
	return p->num;
}

/** 通常レイヤの数を取得 (フォルダは除く) */

int LayerList_getNormalLayerNum(LayerList *p)
{
	LayerItem *pi;
	int num = 0;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img) num++;
	}

	return num;
}

/** 合成時のレイヤ数を取得 */

int LayerList_getBlendLayerNum(LayerList *p)
{
	LayerItem *pi;
	int num = 0;

	pi = LayerList_getItem_bottomVisibleImage(p);

	for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		num++;

	return num;
}


/** item のツリー先頭からのインデックス位置取得
 *
 * return: item が NULL または見つからなかった場合、-1 */

int LayerList_getItemIndex(LayerList *p,LayerItem *item)
{
	LayerItem *pi;
	int pos = 0;

	if(!item) return -1;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi), pos++)
	{
		if(pi == item) return pos;
	}

	return -1;
}

/** item の親のインデックス位置と、親内での相対位置を取得 */

void LayerList_getItemIndex_forParent(LayerList *p,LayerItem *item,int *parent,int *relno)
{
	mTreeItem *pi;
	int no;

	for(pi = item->i.prev, no = 0; pi; pi = pi->prev, no++);

	*parent = LayerList_getItemIndex(p, LAYERITEM(item->i.parent));
	*relno = no;
}

/** 一番上のアイテム取得 */

LayerItem *LayerList_getTopItem(LayerList *p)
{
	return _TOPITEM(p);
}

/** ツリー上で一番下のアイテム取得 */

LayerItem *LayerList_getBottomLastItem(LayerList *p)
{
	return (LayerItem *)mTreeGetBottomLastItem(&p->tree);
}

/** ツリー上でのインデックス位置からアイテム取得
 *
 * フォルダの子アイテムもカウントに含む。
 *
 * pos: 負の場合は NULL が返る */

LayerItem *LayerList_getItemAtIndex(LayerList *p,int pos)
{
	mTreeItem *pi;
	int cnt;

	if(pos < 0) return NULL;

	for(pi = p->tree.top, cnt = 0; pi && pos != cnt; pi = mTreeItemGetNext(pi), cnt++);

	return (LayerItem *)pi;
}

/** 一番下の表示イメージアイテム取得 (合成時用) */

LayerItem *LayerList_getItem_bottomVisibleImage(LayerList *p)
{
	if(p->tree.bottom)
		return LayerItem_getPrevVisibleImage_incSelf(LAYERITEM(p->tree.bottom));
	else
		return NULL;
}

/** インデックス番号から各アイテム取得
 *
 * dst: [0] 親のアイテム [1] pos の位置のアイテム
 * parent: 親の位置 (負の値でルート)
 * pos: フォルダ内の相対位置 (負の値で NULL) */

void LayerList_getItems_fromIndex(LayerList *p,LayerItem **dst,int parent,int pos)
{
	LayerItem *piparent,*pi;
	int i;

	//親アイテム

	dst[0] = piparent = LayerList_getItemAtIndex(p, parent);

	//位置

	if(pos < 0)
		pi = NULL;
	else
	{
		if(piparent)
			pi = (LayerItem *)piparent->i.first;
		else
			pi = _TOPITEM(p);

		for(i = 0; pi && i != pos; pi = _NEXT_ITEM(pi), i++);
	}

	dst[1] = pi;
}

/** イメージ上で、指定位置に一番最初に点があるレイヤを取得 */

LayerItem *LayerList_getItem_topPixelLayer(LayerList *p,int x,int y)
{
	LayerItem *pi,*last = NULL;

	//後ろから順に調べて最後のアイテム

	pi = LayerList_getItem_bottomVisibleImage(p);

	for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
	{
		if(TileImage_isPixel_opaque(pi->img, x, y))
			last = pi;
	}

	return last;
}

/** レイヤ一覧のスクロール情報取得
 *
 * ret_maxdepth: フォルダの最大深度が入る
 * return: 一覧上に表示されている全アイテムの数 */

int LayerList_getScrollInfo(LayerList *p,int *ret_maxdepth)
{
	LayerItem *pi;
	int num = 0,maxdepth = 0,depth;

	for(pi = _TOPITEM(p); pi; pi = LayerItem_getNextOpened(pi), num++)
	{
		depth = LayerItem_getTreeDepth(pi);
		if(depth > maxdepth) maxdepth = depth;
	}

	*ret_maxdepth = maxdepth;

	return num;
}

/** 下レイヤをマスク時、マスクとなるイメージを取得 */

TileImage *LayerList_getMaskUnderImage(LayerList *p,LayerItem *current)
{
	LayerItem *pi;

	//"下レイヤをマスクに" が OFF のレイヤが来るまで、下を検索。
	// - 同階層のレイヤのみ。
	// - フォルダが来た場合は、NULL。

	for(pi = _NEXT_ITEM(current);
		pi && pi->img && LAYERITEM_IS_MASK_UNDER(pi); pi = _NEXT_ITEM(pi));

	return (pi)? pi->img: NULL;
}


//===========================
// フラグなど
//===========================


/** チェックフラグが ON のレイヤがあるか */

mlkbool LayerList_haveCheckedLayer(LayerList *p)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_CHECKED(pi) && pi->img)
			return TRUE;
	}

	return FALSE;
}

/** すべてのレイヤの表示フラグ変更 */

void LayerList_setVisible_all(LayerList *p,mlkbool on)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(on)
			pi->flags |= LAYERITEM_F_VISIBLE;
		else
			pi->flags &= ~LAYERITEM_F_VISIBLE;
	}
}

/** チェックされたレイヤの表示を反転
 *
 * rcimg: 表示反転したすべてのレイヤのイメージ範囲 */

void LayerList_setVisible_all_checked_rev(LayerList *p,mRect *rcimg)
{
	LayerItem *pi;
	mRect rc,rc2;

	mRectEmpty(&rc);

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img && LAYERITEM_IS_CHECKED(pi))
		{
			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc, &rc2);

			pi->flags ^= LAYERITEM_F_VISIBLE;
		}
	}

	*rcimg = rc;
}

/** フォルダを除くすべてのレイヤの表示反転 */

void LayerList_setVisible_all_img_rev(LayerList *p,mRect *rcimg)
{
	LayerItem *pi;
	mRect rc,rc2;

	mRectEmpty(&rc);

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img)
		{
			if(TileImage_getHaveImageRect_pixel(pi->img, &rc2, NULL))
				mRectUnion(&rc, &rc2);

			pi->flags ^= LAYERITEM_F_VISIBLE;
		}
	}

	*rcimg = rc;
}

/** すべてのフォルダを開く */

void LayerList_folder_open_all(LayerList *p)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_FOLDER(pi))
			pi->flags |= LAYERITEM_F_FOLDER_OPENED;
	}
}

/** すべてのフォルダを閉じる
 *
 * カレントの親は閉じない。 */

void LayerList_folder_close_all(LayerList *p,LayerItem *curitem)
{
	LayerItem *pi;

	//すべて閉じる

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_FOLDER(pi))
			pi->flags &= ~LAYERITEM_F_FOLDER_OPENED;
	}

	//カレントの親は開く

	LayerItem_setOpened_parent(curitem);
}

/** すべてのレイヤの指定フラグを OFF */

void LayerList_setFlags_all_off(LayerList *p,uint32_t flags)
{
	LayerItem *pi;

	flags = ~flags;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
		pi->flags &= flags;
}

/** すべてのレイヤのトーン線数を置き換え
 *
 * src: 置き換え元の線数 (0 ですべて)
 * dst: 置き換える値 */

void LayerList_replaceToneLines_all(LayerList *p,int src,int dst)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(!src || src == pi->tone_lines)
			pi->tone_lines = dst;
	}
}


//=========================
// リンク
//=========================


/** チェックレイヤのリンクをセット */

LayerItem *LayerList_setLink_checked(LayerList *p)
{
	LayerItem *pi,*top = NULL,*last;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img && LAYERITEM_IS_CHECKED(pi))
			LayerItem_setLink(pi, &top, &last);
	}

	return top;
}

/** チェックレイヤのリンクをセット (ロックレイヤは除く) */

LayerItem *LayerList_setLink_checked_nolock(LayerList *p)
{
	LayerItem *pi,*top = NULL,*last;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img
			&& LAYERITEM_IS_CHECKED(pi)
			&& !LayerItem_isLock_real(pi))
		{
			LayerItem_setLink(pi, &top, &last);
		}
	}

	return top;
}

/** 複数レイヤ結合時のリンクをセット
 *
 * - 結合順で、下のレイヤから順番に。
 * - テキストレイヤも含む。
 *
 * return: 最初のレイヤ (一番下) */

LayerItem *LayerList_setLink_combineMulti(LayerList *p,int target,LayerItem *current)
{
	LayerItem *pi,*last,*top = NULL;

	switch(target)
	{
		//すべての表示レイヤ
		case 0:
			pi = LayerList_getItem_bottomVisibleImage(p);

			for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
				LayerItem_setLink(pi, &top, &last);
			break;

		//フォルダ内の表示レイヤ
		case 1:
			pi = (LayerItem *)mTreeItemGetBottom(MTREEITEM(current));

			for(; pi; pi = (LayerItem *)mTreeItemGetPrev_root(MTREEITEM(pi), MTREEITEM(current)))
			{
				if(pi->img && LayerItem_isVisible_real(pi))
					LayerItem_setLink(pi, &top, &last);
			}
			break;

		//チェックレイヤ (非表示も含む)
		default:
			pi = (LayerItem *)mTreeGetBottomLastItem(&p->tree);

			for(; pi; pi = (LayerItem *)mTreeItemGetPrev(MTREEITEM(pi)))
			{
				if(LAYERITEM_IS_CHECKED(pi) && pi->img)
					LayerItem_setLink(pi, &top, &last);
			}
			break;
	}

	return top;
}

/** 移動ツール時の対象リンクをセット (単体指定)
 *
 * - ロックレイヤは除く。
 * - フォルダの場合は、フォルダ以下も含む。 */

LayerItem *LayerList_setLink_movetool_single(LayerList *p,LayerItem *item)
{
	LayerItem *pi,*top = NULL,*last;

	for(pi = item; pi; pi = LayerItem_getNextRoot(pi, item))
	{
		if(pi->img && !LayerItem_isLock_real(pi))
			LayerItem_setLink(pi, &top, &last);
	}

	return top;
}

/** 移動ツール時の対象リンクをセット (すべてのイメージ) */

LayerItem *LayerList_setLink_movetool_all(LayerList *p)
{
	LayerItem *pi,*top = NULL,*last;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img && !LayerItem_isLock_real(pi))
			LayerItem_setLink(pi, &top, &last);
	}

	return top;
}

/** 塗りつぶし参照のリンクをセット
 *
 * current: カレントレイヤ
 * type: [0]参照レイヤ [1]カレントレイヤ [2]すべての表示レイヤ
 * return: 先頭のレイヤ (NULL でカレントがフォルダ [*]自動選択時) */

LayerItem *LayerList_setLink_filltool(LayerList *p,LayerItem *current,int type)
{
	LayerItem *pi,*top = NULL,*last;

	switch(type)
	{
		//塗りつぶし参照レイヤ
		case 0:
			for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
			{
				if(pi->img && LAYERITEM_IS_FILLREF(pi))
					LayerItem_setLink(pi, &top, &last);
			}
			break;
		//すべての表示レイヤ
		case 2:
			for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
			{
				if(pi->img && LayerItem_isVisible_real(pi))
					LayerItem_setLink(pi, &top, &last);
			}
			break;
	}

	//参照レイヤがなければ、カレントレイヤ一つのみ

	if(!top)
	{
		//フォルダの場合は NULL
		if(!current->img) return NULL;
	
		top = current;
		top->link = NULL;
	}

	//TileImage のリンクをセット

	for(pi = top; pi; pi = pi->link)
		pi->img->link = (pi->link)? pi->link->img: NULL;

	return top;
}


//=========================
// アイテム移動
//=========================


/** アイテムを上下に移動 (同一階層内) */

mlkbool LayerList_moveitem_updown(LayerList *p,LayerItem *item,mlkbool up)
{
	mTreeItem *next;

	if(up)
	{
		//上へ
		
		if(!item->i.prev) return FALSE;
		
		mTreeMoveItem(&p->tree, MTREEITEM(item), item->i.prev);
	}
	else
	{
		//下へ
	
		next = item->i.next;
		if(!next) return FALSE;
		
		next = next->next;

		if(next)
			mTreeMoveItem(&p->tree, MTREEITEM(item), next);
		else
			mTreeMoveItemToBottom(&p->tree, MTREEITEM(item), item->i.parent);
	}

	return TRUE;
}

/** アイテムを移動
 *
 * src: 移動元
 * dst: 移動先 (NULL で、ルートの末尾)
 * in_folder: dst のフォルダに移動するか。
 *  FALSE で、dst の上に移動。
 *  TRUE で、dst のフォルダへ移動 (子があれば、フォルダの先頭へ。なければ子にする)。
 *   dst フォルダは常に展開する。 */

void LayerList_moveitem(LayerList *p,LayerItem *src,LayerItem *dst,mlkbool in_folder)
{
	if(!dst)
	{
		//ルートの末尾へ

		mTreeMoveItemToBottom(&p->tree, MTREEITEM(src), NULL);
	}
	else if(!in_folder)
	{
		//dst の上へ移動

		mTreeMoveItem(&p->tree, MTREEITEM(src), MTREEITEM(dst));
	}
	else
	{
		//dst フォルダの子へ

		if(dst->i.first)
			//フォルダの先頭
			mTreeMoveItem(&p->tree, MTREEITEM(src), dst->i.first);
		else
			//子にする
			mTreeMoveItemToBottom(&p->tree, MTREEITEM(src), MTREEITEM(dst));

		//展開させる

		dst->flags |= LAYERITEM_F_FOLDER_OPENED;
	}
}

/** アンドゥ処理時の位置移動
 *
 * val: 移動元と移動先のレイヤ位置 */

mlkbool LayerList_moveitem_forUndo(LayerList *p,int *val,mRect *rcupdate)
{
	LayerItem *src[2],*dst[2];

	//移動元レイヤ

	LayerList_getItems_fromIndex(p, src, val[0], val[1]);
	if(!src[1]) return FALSE;

	//移動先レイヤ
	//(移動元のツリーリンクを外した状態で取得)

	mTreeLinkRemove(&p->tree, MTREEITEM(src[1]));

	LayerList_getItems_fromIndex(p, dst, val[2], val[3]);

	//移動 (リンクをつなげる)

	if(dst[1])
		mTreeLinkInsert(&p->tree, MTREEITEM(dst[1]), MTREEITEM(src[1]));
	else
		mTreeLinkBottom(&p->tree, MTREEITEM(dst[0]), MTREEITEM(src[1]));

	//更新範囲

	LayerItem_getVisibleImageRect(src[1], rcupdate);

	return TRUE;
}

/** チェックレイヤを指定フォルダに移動
 *
 * - 移動後、チェックは OFF となる。
 * - フォルダは元々チェックができないので、対象外。
 *
 * pnum: 移動レイヤ数が入る
 * rcupdate: 更新範囲が入る
 * return: アンドゥ用データ (NULL で移動レイヤなし)。
 *  移動レイヤ数 x int16 x 4 (移動前の位置x2, 移動後の位置x2)  */

int16_t *LayerList_moveitem_checked_to_folder(LayerList *p,LayerItem *dst,int *pnum,mRect *rcupdate)
{
	LayerItem *pi,*top = NULL,*last;
	mRect rc;
	int16_t *buf,*pd;
	int num = 0,no1,no2;

	//リンクセット & レイヤ数取得 (最後のレイヤから逆順に)

	for(pi = (LayerItem *)mTreeGetBottomLastItem(&p->tree); pi; pi = _PREV_TREEITEM(pi))
	{
		if(pi->img && LAYERITEM_IS_CHECKED(pi))
		{
			pi->flags ^= LAYERITEM_F_CHECKED;

			//すでに指定フォルダの子なら、そのまま

			if((LayerItem *)pi->i.parent != dst)
			{
				LayerItem_setLink(pi, &top, &last);
				num++;
			}
		}
	}

	if(num == 0) return NULL;

	//バッファ確保

	buf = pd = (int16_t *)mMalloc(num * 4 * 2);
	if(!buf) return NULL;

	//移動
	// :フォルダの子の先頭位置に、上から順に並べていくので、
	// :一番下のレイヤから順にフォルダの先頭へ挿入していく。

	mRectEmpty(rcupdate);

	for(pi = top; pi; pi = pi->link)
	{
		//移動前の位置
		
		LayerList_getItemIndex_forParent(p, pi, &no1, &no2);

		pd[0] = no1;
		pd[1] = no2;
		pd += 2;

		//移動

		if(dst->i.first)
			mTreeMoveItem(&p->tree, MTREEITEM(pi), dst->i.first);
		else
			mTreeMoveItemToBottom(&p->tree, MTREEITEM(pi), MTREEITEM(dst));

		//移動後の位置

		LayerList_getItemIndex_forParent(p, pi, &no1, &no2);

		pd[0] = no1;
		pd[1] = no2;
		pd += 2;

		//

		LayerItem_getVisibleImageRect(pi, &rc);
		mRectUnion(rcupdate, &rc);
	}

	//カレントレイヤがフォルダ下に移動する場合があるので、展開

	dst->flags |= LAYERITEM_F_FOLDER_OPENED;

	*pnum = num;

	return buf;
}

/** 複数レイヤの順番移動 (アンドゥ処理時) */

mlkerr LayerList_moveitem_multi_forUndo(LayerList *p,int num,int16_t *buf,mRect *rcupdate,mlkbool redo)
{
	mRect rc;
	int no[4];

	//アンドゥ時は最後から逆順に

	if(!redo) buf += (num - 1) * 4;

	for(; num > 0; num--)
	{
		//移動元と移動先の位置
		
		if(redo)
			no[0] = buf[0], no[1] = buf[1], no[2] = buf[2], no[3] = buf[3];
		else
			no[0] = buf[2], no[1] = buf[3], no[2] = buf[0], no[3] = buf[1];

		//移動
	
		if(!LayerList_moveitem_forUndo(p, no, &rc))
			return MLKERR_INVALID_VALUE;

		//

		mRectUnion(rcupdate, &rc);

		if(redo)
			buf += 4;
		else
			buf -= 4;
	}

	return MLKERR_OK;
}


//===============================
// ほか
//===============================


/** レイヤ名をレイヤ数からセット
 *
 * レイヤ追加後の現在数からセットする。 */

void LayerList_setItemName_curlayernum(LayerList *p,LayerItem *item)
{
	LayerItem_setName_layerno(item, p->num - 1);
}

/** Shift-JIS 文字列として、レイヤ名セット */

void LayerList_setItemName_shiftjis(LayerList *p,LayerItem *item,const char *name)
{
	char *buf;
	
	buf = mConvertCharset(name, -1, "CP932", "UTF-8", NULL, 1);

	if(!buf)
		LayerList_setItemName_curlayernum(p, item);
	else
	{
		mFree(item->name);
		item->name = buf;
	}
}

/** すべてのイメージのオフセット位置を相対移動
 *
 * (キャンバスサイズ変更時) */

void LayerList_moveOffset_rel_all(LayerList *p,int movx,int movy)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_IMAGE(pi))
			LayerItem_moveImage(pi, movx, movy);
	}
}

/** すべてのテキストレイヤの位置を相対移動
 *
 * (キャンバスサイズ変更:切り取り時) */

void LayerList_moveOffset_rel_text(LayerList *p,int movx,int movy)
{
	LayerItem *pi;

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(LAYERITEM_IS_TEXT(pi))
			LayerItem_moveImage(pi, movx, movy);
	}
}

/** すべてのレイヤのイメージビット数を変更 */

void LayerList_convertImageBits(LayerList *p,int bits,mPopupProgress *prog)
{
	LayerItem *pi;
	void *tblbuf;

	//変換用テーブル

	if(bits == 8)
		tblbuf = (void *)TileImage_create16fixto8_table();
	else
		tblbuf = (void *)TileImage_create8to16fix_table();

	//

	for(pi = _TOPITEM(p); pi; pi = _NEXT_TREEITEM(pi))
	{
		if(pi->img)
			TileImage_convertBits(pi->img, bits, tblbuf, prog);
	}

	mFree(tblbuf);
}

