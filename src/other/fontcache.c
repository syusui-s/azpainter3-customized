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

/***************************
 * フォントキャッシュ
 ***************************/

#include <string.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CID_H

#define MLK_FONT_FREETYPE_DEFINE

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_font.h"
#include "mlk_font_freetype.h"
#include "mlk_opentype.h"
#include "mlk_util.h"

#include "fontcache.h"


//-------------

typedef struct
{
	mListCacheItem i;

	mFont *font;
	uint32_t hash;	//ファイル名ハッシュ
	int index;
	char filename[1]; //0 でデフォルトフォント
}_cacheitem;

static mList g_list;

#define _LIST_MAXNUM  6  //リスト全体の保持数

//-------------


//==============================
// フォント初期化
//==============================


/* VORG 読み込み */

static void _read_VORG(mFont *p)
{
	void *buf;
	uint32_t size;

	if(mFontFT_loadTable(p, MLK_MAKE32_4('V','O','R','G'), &buf, &size) == MLKERR_OK)
	{
		mOpenType_readVORG(buf, size, &p->sub->vorg);

		mFree(buf);
	}
}

/* GSUB: Feature 取得 */

static mlkerr _get_feature(mOTLayout *p,uint32_t feature_tag,mOT_TABLE *dst)
{
	uint32_t script_tag,
	 tags[] = {
		MLK_MAKE32_4('h','a','n','i'), MLK_MAKE32_4('k','a','n','a'), MLK_MAKE32_4('h','a','n','g'), 0
	};

	mOTLayout_searchScriptList(p, tags, &script_tag);

	return mOTLayout_getFeature2(p, script_tag, 0, feature_tag, dst);
}

/* vert/vrt2 読み込み */

static void _read_GSUB_vert(mFont *p,mOTLayout *ot)
{
	mOT_TABLE tbl;
	uint8_t *vert = NULL, *vrt2 = NULL;

	//vert

	if(_get_feature(ot, MOPENTYPE_FEATURE_VERT, &tbl) == MLKERR_OK)
		mOTLayout_createGSUB_single(ot, &tbl, &vert);

	//vrt2

	if(_get_feature(ot, MOPENTYPE_FEATURE_VRT2, &tbl) == MLKERR_OK)
		mOTLayout_createGSUB_single(ot, &tbl, &vrt2);

	//結合

	p->sub->gsub_vert = mOpenType_combineData_GSUB_single(vert, vrt2);
}

/* GSUB 読み込み */

static void _read_GSUB(mFont *p)
{
	mOTLayout *ot;
	void *buf;
	uint32_t size;
	mOT_TABLE tbl;

	if(mFontFT_loadTable(p, MLK_MAKE32_4('G','S','U','B'), &buf, &size))
		return;

	if(mOTLayout_new(&ot, buf, size, FALSE) == MLKERR_OK)
	{
		//vert
		
		_read_GSUB_vert(p, ot);

		//ルビ文字
	
		if(_get_feature(ot, MOPENTYPE_FEATURE_RUBY, &tbl) == MLKERR_OK)
			mOTLayout_createGSUB_single(ot, &tbl, &p->sub->gsub_ruby);
			
		//hwid

		if(_get_feature(ot, MLK_MAKE32_4('h','w','i','d'), &tbl) == MLKERR_OK)
			mOTLayout_createGSUB_single(ot, &tbl, &p->sub->gsub_hwid);

		//twid

		if(_get_feature(ot, MLK_MAKE32_4('t','w','i','d'), &tbl) == MLKERR_OK)
			mOTLayout_createGSUB_single(ot, &tbl, &p->sub->gsub_twid);

		//
	
		mOTLayout_free(ot);
	}

	mFree(buf);
}

/* フォント解放時ハンドラ */

static void _free_handle(mFont *font)
{
	mFTSubData *p = font->sub;

	//サブデータ

	if(p)
	{
		mFree(p->vorg.buf);

		mFree(p->gsub_vert);
		mFree(p->gsub_ruby);
		mFree(p->gsub_hwid);
		mFree(p->gsub_twid);
		
		mFree(p);
	}
}

/* フォント作成後の初期化 */

static void _init_font(mFont *p)
{
	FT_Bool is_cid;

	p->free_font = _free_handle;

	//--- サブデータ

	p->sub = (mFTSubData *)mMalloc0(sizeof(mFTSubData));

	//CID フォントかどうか
	// :FT_IS_CID_KEYED() では取得できない。
	
	FT_Get_CID_Is_Internally_CID_Keyed(p->face, &is_cid);

	p->sub->is_cid = is_cid;

	//

	_read_VORG(p);
	_read_GSUB(p);
}


//======================
// list sub
//======================


/* ファイル名から検索
 *
 * filename: NULL でデフォルトフォント */

static _cacheitem *_find_font_name(const char *filename,int index)
{
	_cacheitem *pi;
	uint32_t hash;

	if(!filename)
	{
		//デフォルトフォント

		MLK_LIST_FOR(g_list, pi, _cacheitem)
		{
			if(pi->filename[0] == 0)
				return pi;
		}
	}
	else
	{
		hash = mCalcStringHash(filename);

		MLK_LIST_FOR(g_list, pi, _cacheitem)
		{
			if(pi->hash == hash
				&& pi->index == index
				&& strcmp(pi->filename, filename) == 0)
				return pi;
		}
	}

	return NULL;
}

/* mFont * から検索 */

static _cacheitem *_find_font_ptr(mFont *font)
{
	_cacheitem *pi;

	MLK_LIST_FOR(g_list, pi, _cacheitem)
	{
		if(pi->font == font)
			return pi;
	}

	return NULL;
}


//=========================
// main
//=========================


/* アイテム破棄ハンドラ */

static void _item_destroy(mList *list,mListItem *item)
{
	mFontFree(((_cacheitem *)item)->font);
}

/** 初期化 */

void FontCache_init(void)
{
	mListInit(&g_list);

	g_list.item_destroy = _item_destroy;
}

/** 解放 */

void FontCache_free(void)
{
	mListDeleteAll(&g_list);
}

/** フォント読み込み
 *
 * filename: NULL でデフォルトフォント */

mFont *FontCache_loadFont(const char *filename,int index)
{
	_cacheitem *pi;
	mFont *font;
	int len,is_default;

	//キャッシュから検索

	pi = _find_font_name(filename, index);

	//見つかった場合、参照カウンタ+1

	if(pi)
	{
		mListCache_refItem(&g_list, MLISTCACHEITEM(pi));
		return pi->font;
	}

	//------- 新規追加

	//フォント作成
	// :失敗時はデフォルトフォント

	font = mFontCreate_file(mGuiGetFontSystem(), filename, index);

	if(font)
		is_default = FALSE;
	else
	{
		//---- 失敗時

		//デフォルトフォントがあるか

		pi = _find_font_name(NULL, 0);

		if(pi)
		{
			mListCache_refItem(&g_list, MLISTCACHEITEM(pi));
			return pi->font;
		}

		//作成
		
		font = mFontCreate_default(mGuiGetFontSystem());
		if(!font) return NULL;

		is_default = TRUE;
	}

	//フォント初期化

	_init_font(font);

	//アイテム追加

	len = (is_default)? 0: strlen(filename);

	pi = (_cacheitem *)mListCache_appendNew(&g_list, sizeof(_cacheitem) + len);
	if(!pi)
	{
		mFontFree(font);
		return NULL;
	}

	//データセット

	pi->font = font;

	if(!is_default)
	{
		pi->hash = mCalcStringHash(filename);
		pi->index = index;

		memcpy(pi->filename, filename, len);
	}

	//リスト全体の数により古いものを削除

	mListCache_deleteUnused_allnum(&g_list, _LIST_MAXNUM);

	//
	
#if 0
	mDebug("------->\n");
	MLK_LIST_FOR(g_list, pi, _cacheitem)
	{
		mDebug("[cnt:%d] '%s' %d\n", pi->i.refcnt, pi->filename, pi->index);
	}
	mDebug("<-------\n");
#endif

	return font;
}

/** ファミリ名とスタイルからフォント読み込み */

mFont *FontCache_loadFont_family(const char *family,const char *style)
{
	mFont *font;
	char *fname;
	int index;

	index = mFontGetFilename_fromFamily(&fname, family, style);

	if(index == -1)
		//失敗時はデフォルトフォント
		return FontCache_loadFont(NULL, 0);
	else
	{
		font = FontCache_loadFont(fname, index);
		
		mFree(fname);

		return font;
	}
}

/** フォントを解放 */

void FontCache_releaseFont(mFont *font)
{
	_cacheitem *pi;

	pi = _find_font_ptr(font);
	if(!pi) return;

	mListCache_releaseItem(&g_list, MLISTCACHEITEM(pi));
}


