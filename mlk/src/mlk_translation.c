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
 * mTranslation
 *****************************************/

#include <stdlib.h>	//getenv
#include <stdio.h>

#include "mlk.h"
#include "mlk_translation.h"

#include "mlk_str.h"
#include "mlk_util.h"
#include "mlk_stdio.h"
#include "mlk_file.h"


//-------------------

struct _mTranslation
{
	uint8_t *buf,		//データバッファ (グループリストの位置)
		*curgroup_top;	//カレントグループのアイテム先頭位置

	int group_num,			//グループの数
		curgroup_id,		//カレントグループのID (-1 でなし)
		curgroup_itemnum,	//カレントグループのアイテム数
		is_fromfile;		//TRUE で、ファイルから読み込んだ

	uint32_t offset_itemlist,	//項目リストのデータ位置
		offset_text;			//文字列のデータ位置
};

//-------------------


//======================
// 検索関連
//======================


/** ID 検索
 *
 * itemnum: データ数
 * bytes: 1個のデータのバイト数 */

static uint8_t *_search_id(uint8_t *buf,uint16_t id,int itemnum,int bytes)
{
	uint8_t *ps;
	int low,high,mid;
	uint16_t comp;

	low = 0;
	high = itemnum - 1;

	while(low <= high)
	{
		mid = (low + high) / 2;

		ps = buf + mid * bytes;
		comp = (ps[0] << 8) | ps[1];

		if(comp == id)
			return ps;
		else if(comp < id)
			low = mid + 1;
		else
			high = mid - 1;
	}

	return NULL;
}

/** グループ検索
 *
 * itemnum: グループの文字列数が入る
 * return: 文字列リストデータの先頭位置。NULL で見つからなかった。 */

static uint8_t *_search_group(mTranslation *p,uint16_t id,int *itemnum)
{
	uint8_t *ps;

	if(id == 0xffff)
	{
		//65535 はシステム用。
		//存在すれば、常に終端位置
		
		ps = p->buf + ((p->group_num - 1) << 3);

		if(*ps != 255 || ps[1] != 255) ps = NULL;
	}
	else
		ps = _search_id(p->buf, id, p->group_num, 8);

	//

	if(!ps)
		return NULL;
	else
	{
		//文字列数
		*itemnum = mGetBufBE16(ps + 2);

		return p->buf + p->offset_itemlist + mGetBufBE32(ps + 4);
	}
}

/** 文字列検索
 *
 * ps: 文字列リストデータの先頭位置
 * itemnum: グループの文字列数 */

static const char *_search_item(mTranslation *p,uint8_t *ps,int itemnum,uint16_t id)
{
	if(!ps) return NULL;

	ps = _search_id(ps, id, itemnum, 6);
	if(!ps)
		return NULL;
	else
		return (const char *)(p->buf + p->offset_text + mGetBufBE32(ps + 2));
}


//======================
// 読み込み関連
//======================


/** mtr ファイルから読み込み */

static mlkbool _load_file(mTranslation *p,FILE *fp)
{
	uint8_t ver;
	uint32_t textsize,itemnum,size;

	//ヘッダ

	if(mFILEreadStr_compare(fp, "MLKTR")) return FALSE;

	//バージョン

	if(mFILEreadByte(fp, &ver) || ver != 0)
		return FALSE;

	//グループ数、アイテム数、文字列サイズ

	if(mFILEreadBE32(fp, &p->group_num)
		|| p->group_num == 0
		|| mFILEreadBE32(fp, &itemnum)
		|| mFILEreadBE32(fp, &textsize))
		return FALSE;

	//メモリ確保

	size = p->group_num * 8 + itemnum * 6 + textsize;

	p->buf = (uint8_t *)mMalloc(size);
	if(!p->buf) return FALSE;

	//データ読み込み

	if(fread(p->buf, 1, size, fp) != size)
		return FALSE;

	//

	p->offset_itemlist = p->group_num * 8;
	p->offset_text = p->offset_itemlist + itemnum * 6;
	p->is_fromfile = TRUE;

	p->curgroup_id = -1;
	
	return TRUE;
}

/** ディレクトリから mtr ファイル検索
 *
 * 先に5文字の言語名で検索。見つからなければ先頭2文字の言語名で検索
 *
 * return: TRUE でファイルが見つかった */

static mlkbool _get_filename(mStr *strdst,
	const char *path,const char *lang,const char *prefix)
{
	mStr str = MSTR_INIT,strlang = MSTR_INIT;
	char *pc;
	mlkbool ret;

	//言語名

	if(lang)
		mStrSetText(&strlang, lang);
	else
	{
		pc = getenv("LANG");
		if(!pc)
			mStrSetText(&strlang, "en");
		else
		{
			//'.' 以降を除去

			mStrSetText(&strlang, pc);
			mStrFindChar_toend(&strlang, '.');
		}
	}

	//1文字以下の場合はエラー

	if(strlang.len < 2)
	{
		mStrFree(&strlang);
		return FALSE;
	}

	//検索

	mStrPathCombine_prefix(&str, path, prefix, strlang.buf, "mtr");

	ret = mIsExistFile(str.buf);
	if(!ret)
	{
		//見つからなければ、先頭2文字で検索 (3文字以上の場合)

		if(strlang.len > 2)
		{
			mStrSetLen(&strlang, 2);

			mStrPathCombine_prefix(&str, path, prefix, strlang.buf, "mtr");

			ret = mIsExistFile(str.buf);
		}
	}

	//

	if(ret)
		mStrCopy(strdst, &str);

	mStrFree(&str);
	mStrFree(&strlang);

	return ret;
}


//======================
// メイン
//======================


/**@ 解放 */

void mTranslationFree(mTranslation *p)
{
	if(p)
	{
		if(p->is_fromfile)
			mFree(p->buf);

		mFree(p);
	}
}

/**@ デフォルトの翻訳データを作成
 *
 * @p:buf 埋め込みデータのバッファ */

mTranslation *mTranslationLoadDefault(const void *buf)
{
	mTranslation *p;
	uint8_t *ps = (uint8_t *)buf;
	uint32_t itemnum;

	p = (mTranslation *)mMalloc0(sizeof(mTranslation));
	if(!p) return NULL;

	p->group_num = mGetBufBE32(ps);
	itemnum = mGetBufBE32(ps + 4);

	p->buf = ps + 12;
	p->offset_itemlist = p->group_num * 8;
	p->offset_text = p->offset_itemlist + itemnum * 6;

	p->curgroup_id = -1;

	return p;
}

/**@ mtr ファイルから翻訳データ読み込み
 *
 * @p:local TRUE で filename はロケール文字列。FALSE で UTF-8。 */

mTranslation *mTranslationLoadFile(const char *filename,mlkbool locale)
{
	mTranslation *p = NULL;
	FILE *fp;

	if(locale)
		fp = fopen(filename, "rb");
	else
		fp = mFILEopen(filename, "rb");

	if(!fp) return NULL;

	p = (mTranslation *)mMalloc0(sizeof(mTranslation));
	if(!p) goto ERR;

	if(!_load_file(p, fp)) goto ERR;

	fclose(fp);

	return p;

ERR:
	fclose(fp);
	mTranslationFree(p);
	return NULL;
}

/**@ mtr ファイルから翻訳データ読み込み (言語指定)
 *
 * @d:指定ディレクトリから、言語を元に検索。
 *
 * @p:path mtr ファイル検索パス
 * @p:lang "en", "en_US" など。NULL で LANG 環境変数から取得。
 * @p:prefix ファイル名の接頭語 (言語名の前に付く)。NULL でなし。
 * @r:NULL で読み込まなかった */

mTranslation *mTranslationLoadFile_lang(const char *path,const char *lang,const char *prefix)
{
	mStr str = MSTR_INIT;
	mTranslation *p;

	//ファイル名検索

	if(!_get_filename(&str, path, lang, prefix))
		return NULL;

	//読み込み

	p = mTranslationLoadFile(str.buf, FALSE);

	mStrFree(&str);

	return p;
}

/**@ カレントグループをセット
 *
 * @d:現在セットされているグループと同じ ID なら、何もせず TRUE が返る。
 *
 * @r:グループが見つかったか */

mlkbool mTranslationSetGroup(mTranslation *p,uint16_t id)
{
	if(!p)
		return FALSE;
	else if(id == p->curgroup_id && p->curgroup_top)
		//すでにセットされている
		return TRUE;
	else
	{
		p->curgroup_id = id;
		p->curgroup_top = _search_group(p, id, &p->curgroup_itemnum);

		return (p->curgroup_top != NULL);
	}
}

/**@ カレントグループから文字列取得
 *
 * @r:文字列ポインタ (NULL 文字含む)。NULL で文字列なし。 */

const char *mTranslationGetText(mTranslation *p,uint16_t id)
{
	if(!p)
		return NULL;
	else
		return _search_item(p, p->curgroup_top, p->curgroup_itemnum, id);
}

/**@ 指定グループから文字列取得
 *
 * @d:※カレントグループは変更されない。 */

const char *mTranslationGetText2(mTranslation *p,uint16_t groupid,uint16_t itemid)
{
	if(!p)
		return NULL;
	else if(p->curgroup_id == groupid)
	{
		//カレントグループと同じ場合

		return _search_item(p, p->curgroup_top, p->curgroup_itemnum, itemid);
	}
	else
	{
		//グループ検索
		
		uint8_t *top;
		int num;

		top = _search_group(p, groupid, &num);

		return _search_item(p, top, num, itemid);
	}
}

