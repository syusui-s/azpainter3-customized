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
 * MaterialList
 * 素材画像の管理リスト
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"

#include "imagematerial.h"

#include "def_macro.h"
#include "def_config.h"

#include "materiallist.h"


/*
  - 現在使用されているブラシ形状/テクスチャ画像を管理する。
  - 最近使われたものほどリストの先頭にある。
  - ブラシ用の形状・テクスチャ画像は、最近使ったものを最大6つ保存しておく。
  - レイヤ用のテクスチャは、常に使用されているものを読み込んでおく。
*/

//-----------------------

/* リストアイテムデータ */

typedef struct
{
	mListCacheItem i;

	ImageMaterial *img;
	char path[1];		//ファイルパス
}_item;

//-----------------------

#define _TOPITEM(list)  ((_item *)(list)->top)
#define _ITEM(p)        ((_item *)(p))
#define _NEXTITEM(p)    ((_item *)(p)->i.next)

#define _SAVE_NUM     6		//未使用保存数

#define _LIST_TEXTURE 0
#define _LIST_BRUSH   1

#define _PUT_DEBUG  0

//-----------------------


//=========================
// sub
//=========================


/* リスト内に同じパスの画像があるか */

static _item *_search_path(mList *list,const char *path)
{
	_item *pi;

	for(pi = _TOPITEM(list); pi; pi = _NEXTITEM(pi))
	{
		if(strcmp(path, pi->path) == 0)
			return pi;
	}

	return NULL;
}

/* 画像読み込み */

static ImageMaterial *_load_image(int type,const char *path)
{
	ImageMaterial *img;
	mStr str = MSTR_INIT;

	//パス

	if(path[0] == '/')
	{
		//システム

		mGuiGetPath_data(&str,
			(type == MATERIALLIST_TYPE_BRUSH_SHAPE)? APP_DIRNAME_BRUSH: APP_DIRNAME_TEXTURE);

		mStrPathJoin(&str, path + 1);
	}
	else
	{
		//ユーザー

		mStrCopy(&str,
			(type == MATERIALLIST_TYPE_BRUSH_SHAPE)?
				&APPCONF->strUserBrushDir: &APPCONF->strUserTextureDir);

		mStrPathJoin(&str, path);
	}

	//読み込み

	if(type == MATERIALLIST_TYPE_BRUSH_SHAPE)
		img = ImageMaterial_loadBrush(&str);
	else
		img = ImageMaterial_loadTexture(&str);

	mStrFree(&str);

	return img;
}


//=========================
// main
//=========================


/* アイテム破棄ハンドラ */

static void _destroy_item(mList *list,mListItem *p)
{
	ImageMaterial_free(_ITEM(p)->img);
}

/** 初期化 */

void MaterialList_init(mList *arr)
{
	arr[0].item_destroy = _destroy_item;
	arr[1].item_destroy = _destroy_item;
}

/** 画像の取得
 *
 * リスト内に画像がない場合は読み込んでセット。
 * リスト内に存在する場合は、その画像を返す。
 *
 * keep = TRUE にしたブラシ画像の場合は、必ず明示的に MaterialList_releaseImage() を実行。
 * 通常のブラシ画像の場合は解放する必要はない。
 *
 * keep: イメージを常に保持する (レイヤテクスチャの場合は常に TRUE となる) */

ImageMaterial *MaterialList_getImage(mList *arr,int type,const char *path,mlkbool keep)
{
	mList *list;
	_item *pi;
	ImageMaterial *img;
	int len;

	//パスが NULL or 空 => 画像なし

	if(!path || !path[0]) return NULL;

	//ブラシテクスチャで、パスが "?" => オプション指定を使う

	if(type == MATERIALLIST_TYPE_BRUSH_TEXTURE
		&& strcmp(path, "?") == 0)
		return NULL;

	//リスト内に同じものがあるか

	list = arr + ((type == MATERIALLIST_TYPE_BRUSH_SHAPE)? _LIST_BRUSH: _LIST_TEXTURE);

	pi = _search_path(list, path);

	//レイヤテクスチャ時は常に保持

	if(type == MATERIALLIST_TYPE_LAYER_TEXTURE) keep = TRUE;

	//

	if(pi)
	{
		//----- 存在する

		//参照カウンタを+1して先頭へ

		mListCache_refItem(list, (mListCacheItem *)pi);
	}
	else
	{
		//----- 新規追加
		
		//画像読み込み

		img = _load_image(type, path);
		if(!img) return NULL;

		//未使用の古いものを削除

		mListCache_deleteUnused(list, _SAVE_NUM);

		//リストに追加

		len = strlen(path);

		pi = (_item *)mListCache_appendNew(list, sizeof(_item) + len);
		if(!pi)
		{
			ImageMaterial_free(img);
			return NULL;
		}

		pi->img = img;

		memcpy(pi->path, path, len);
	}

	//常に保持でない場合は、参照カウンタを-1

	if(!keep) pi->i.refcnt--;

	//[debug]

#if _PUT_DEBUG
	_item *pp;

	mDebug("--> matlist\n");

	for(pp = _TOPITEM(list); pp; pp = _NEXTITEM(pp))
		mDebug("%s [%d]\n", pp->path, pp->i.refcnt);

	mDebug("<-- matlist\n");
#endif

	return pi->img;
}

/** 指定イメージをリストから解放
 *
 * 参照カウンタを引く (0 でない場合)。
 * 参照カウンタが 0 になったものは、次の新規追加時に古いものから解放される。 */

void MaterialList_releaseImage(mList *arr,int type,ImageMaterial *img)
{
	_item *pi;

	if(!img) return;

	for(pi = _TOPITEM(arr + type); pi; pi = _NEXTITEM(pi))
	{
		if(pi->img == img)
		{
			mListCache_releaseItem(arr + type, (mListCacheItem *)pi);
			break;
		}
	}
}

