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
 * mCursor (X11/Wayland 共通)
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_cursor.h"
#include "mlk_list.h"
#include "mlk_imagebuf.h"
#include "mlk_loadimage.h"

#include "mlk_pv_gui.h"


//---------------------

typedef struct
{
	mListCacheItem i;
	mCursor cursor;
	int curtype;
}cache_item;

#define _ITEM(p)  ((cache_item *)(p))

#define _CACHE_ITEM_MAX  15	//未使用の最大数
#define _PUT_DEBUG  0

//---------------------

static const char *g_type_name[] = {
	"left_ptr\0",
	"h_double_arrow\0", "v_double_arrow\0", "bd_double_arrow\0", "fd_double_arrow\0",
	"left_side\0", "right_side\0", "top_side\0", "bottom_side\0",
	"top_left_corner\0", "top_right_corner\0", "bottom_left_corner\0", "bottom_right_corner\0",
	"col-resize\0h_double_arrow\0", "row-resize\0v_double_arrow\0"
};

static const uint8_t g_cursor_empty[] = {
	1,1,0,0, 0,0
};

//---------------------


//=============================
// sub
//=============================


/** アイテム破棄ハンドラ */

static void _cache_item_destroy(mList *list,mListItem *item)
{
	mCursorFree(_ITEM(item)->cursor);
}

/** キャッシュリスト初期化 (GUI 初期化時に呼ばれる) */

void __mCursorCacheInit(mList *list)
{
	list->item_destroy = _cache_item_destroy;
}

/** デバッグ表示 */

#if _PUT_DEBUG

static void _put_debug(const char *type)
{
	cache_item *pi;

	mDebug(">--- cursor cache [%s]\n", type);

	for(pi = _ITEM(MLKAPP->list_cursor_cache.top); pi; pi = _ITEM(pi->i.next))
		mDebug("  type(%d) cursor(%p) ref(%u)\n", pi->curtype, pi->cursor, pi->i.refcnt);

	mDebug("<--- cursor cache [%s]\n", type);
}

#endif


//============================
// mCursor
//============================


/**@ カーソル解放 */

void mCursorFree(mCursor cur)
{
	(MLKAPP->bkend.cursor_free)(cur);
}

/**@ カーソル読み込み
 *
 * @p:name カーソル名
 * @r:失敗した場合 0 */

mCursor mCursorLoad(const char *name)
{
	return (MLKAPP->bkend.cursor_load)(name);
}

/**@ 複数名からカーソル読み込み
 *
 * @p:names ヌル文字で区切って、複数指定する。終端はヌル文字を2つ連続させる。\
 *  先頭から順に読み込み、成功したものが返る。
 * @r:いずれも失敗した場合は、0 */

mCursor mCursorLoad_names(const char *names)
{
	const char *pc;
	mCursor cur;

	for(pc = names; *pc; pc += strlen(pc) + 1)
	{
		cur = mCursorLoad(pc);
		if(cur) return cur;
	}

	return 0;
}

/**@ 指定タイプのカーソル読み込み
 *
 * @r:失敗時、0 */

mCursor mCursorLoad_type(int type)
{
	if(type == MCURSOR_TYPE_NONE)
		return 0;
	else if(type < 0 || type >= MCURSOR_TYPE_MAX)
		type = 0;

	return mCursorLoad_names(g_type_name[type - 1]);
}

/**@ 1bit データからカーソルを作成
 *
 * @d:[uint8] 幅\
 * [uint8] 高さ\
 * [uint8] ホットスポット位置 X\
 * [uint8] ホットスポット位置 Y\
 * [N] イメージデータ 1bit (0=白、1=黒)\
 * - (width + 7) / 8 * height 分のデータ。\
 * - X は下位ビットから順に並ぶ。Y は上から下。\
 * [N] マスクデータ 1bit (0=透明、1=不透明)\
 * - イメージデータと同じ。 */

mCursor mCursorCreate1bit(const uint8_t *buf)
{
	return (MLKAPP->bkend.cursor_create1bit)(buf);
}

/**@ 空 (透明) のカーソルを作成 */

mCursor mCursorCreateEmpty(void)
{
	return (MLKAPP->bkend.cursor_create1bit)(g_cursor_empty);
}

/**@ RGBA イメージからカーソルを作成
 *
 * @p:buf R-G-B-A 順で、左から右、上から下に並んでいる。余分な境界バイトはなし。 */

mCursor mCursorCreateRGBA(int width,int height,int hotspot_x,int hotspot_y,const uint8_t *buf)
{
	if(hotspot_x < 0)
		hotspot_x = 0;
	else if(hotspot_x >= width)
		hotspot_x = width - 1;

	if(hotspot_y < 0)
		hotspot_y = 0;
	else if(hotspot_y >= height)
		hotspot_y = height - 1;

	return (MLKAPP->bkend.cursor_createRGBA)(width, height, hotspot_x, hotspot_y, buf);
}

/**@ PNG 画像ファイルからカーソルを読み込み
 *
 * @d:アルファ付き、または透過付きであること。 */

mCursor mCursorLoadFile_png(const char *filename,int hotspot_x,int hotspot_y)
{
	mImageBuf *img;
	mCursor cur;
	mLoadImageOpen open;
	mLoadImageType tp;

	mLoadImage_checkPNG(&tp, NULL, 0);

	open.type = MLOADIMAGE_OPEN_FILENAME;
	open.filename = filename;

	img = mImageBuf_loadImage(&open, &tp, 32, 0);
	if(!img) return 0;

	cur = mCursorCreateRGBA(img->width, img->height, hotspot_x, hotspot_y, img->buf);

	mImageBuf_free(img);

	return cur;
}

/**@ リサイズのフラグからカーソルタイプ取得
 *
 * @r:該当しない場合はデフォルトカーソル */

int mCursorGetType_resize(int flags)
{
	int type;

	if(flags == MCURSOR_RESIZE_TOP_LEFT)
		//左上
		type = MCURSOR_TYPE_RESIZE_NW;
	else if(flags == MCURSOR_RESIZE_TOP_RIGHT)
		//右上
		type = MCURSOR_TYPE_RESIZE_NE;
	else if(flags == MCURSOR_RESIZE_BOTTOM_LEFT)
		//左下
		type = MCURSOR_TYPE_RESIZE_SW;
	else if(flags == MCURSOR_RESIZE_BOTTOM_RIGHT)
		//右下
		type = MCURSOR_TYPE_RESIZE_SE;
	else if(flags == MCURSOR_RESIZE_TOP)
		//上
		type = MCURSOR_TYPE_RESIZE_N;
	else if(flags == MCURSOR_RESIZE_BOTTOM)
		//下
		type = MCURSOR_TYPE_RESIZE_S;
	else if(flags == MCURSOR_RESIZE_LEFT)
		//左
		type = MCURSOR_TYPE_RESIZE_W;
	else if(flags == MCURSOR_RESIZE_RIGHT)
		//右
		type = MCURSOR_TYPE_RESIZE_E;
	else
		type = MCURSOR_TYPE_DEFAULT;

	return type;
}


//============================
// mCursorCache
//============================


/**@ カーソルキャッシュから指定タイプのカーソルを取得
 *
 * @g:mCursorCache
 *
 * @d:キャッシュに存在しない場合は追加する。 */

mCursor mCursorCache_getCursor_type(int type)
{
	mList *list;
	cache_item *pi;
	mCursor cur;

	if(type == MCURSOR_TYPE_NONE)
		return 0;

	list = &MLKAPP->list_cursor_cache;

	//キャッシュ内から検索し、あれば参照カウンタ+1
	//(上から順に新しい)

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->curtype == type)
		{
			mListCache_refItem(list, MLISTCACHEITEM(pi));

		#if _PUT_DEBUG
			_put_debug("addref");
		#endif
			
			return pi->cursor;
		}
	}

	//----- 見つからなかったため、新規追加

	//カーソル作成

	cur = mCursorLoad_type(type);
	if(!cur) return 0;

	//キャッシュに追加

	pi = (cache_item *)mListCache_appendNew(list, sizeof(cache_item));
	if(!pi)
	{
		mCursorFree(cur);
		return 0;
	}

	pi->curtype = type;
	pi->cursor = cur;

	mListCache_deleteUnused(list, _CACHE_ITEM_MAX);

#if _PUT_DEBUG
	_put_debug("add");
#endif

	return cur;
}

/**@ 指定カーソルをキャッシュに追加
 *
 * @d:すでに存在する場合は、参照カウンタを+1する。 */

void mCursorCache_addCursor(mCursor cur)
{
	mList *list;
	cache_item *pi;

	list = &MLKAPP->list_cursor_cache;

	//存在している場合、参照カウンタ+1

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->cursor == cur)
		{
			mListCache_refItem(list, MLISTCACHEITEM(pi));

		#if _PUT_DEBUG
			_put_debug("addref");
		#endif

			return;
		}
	}

	//存在しない場合、新規追加

	pi = (cache_item *)mListCache_appendNew(list, sizeof(cache_item));
	if(!pi) return;

	pi->curtype = MCURSOR_TYPE_NONE;
	pi->cursor = cur;

	mListCache_deleteUnused(list, _CACHE_ITEM_MAX);

#if _PUT_DEBUG
	_put_debug("add");
#endif
}

/**@ 指定カーソルをキャッシュから解放 */

void mCursorCache_release(mCursor cur)
{
	mList *list;
	cache_item *pi;

	if(!cur) return;

	list = &MLKAPP->list_cursor_cache;

	for(pi = _ITEM(list->top); pi; pi = _ITEM(pi->i.next))
	{
		if(pi->cursor == cur)
		{
			mListCache_releaseItem(list, MLISTCACHEITEM(pi));

		#if _PUT_DEBUG
			_put_debug("release");
		#endif
			break;
		}
	}
}

