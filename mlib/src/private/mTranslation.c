/*$
 Copyright (C) 2013-2020 Azel.

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


#include <stdlib.h>

#include "mDef.h"
#include "mTranslation.h"

#include "mFile.h"
#include "mStr.h"
#include "mUtil.h"
#include "mUtilFile.h"


//======================
// 検索関連
//======================


/** グループ検索
 *
 * @return 項目データの先頭位置 */

static uint8_t *_findGroup(mTranslation *p,uint16_t groupid,int *itemnum)
{
	uint8_t *ps = p->buf;
	int i;
	uint16_t id;

	if(!ps) return NULL;

	for(i = p->groupNum; i > 0; i--, ps += 8)
	{
		id = mGetBuf16BE(ps);

		if(id == groupid)
		{
			*itemnum = mGetBuf16BE(ps + 2);
			
			return p->buf + p->offsetItem + mGetBuf32BE(ps + 4);
		}
	}

	return NULL;
}

/** 項目検索 */

static const char *_findItem(mTranslation *p,uint8_t *itemtop,int itemnum,uint16_t itemid)
{
	uint8_t *ps = itemtop;
	int i;
	uint16_t id;

	if(!ps) return NULL;

	for(i = itemnum; i > 0; i--, ps += 6)
	{
		id = mGetBuf16BE(ps);

		if(id == itemid)
			return (const char *)(p->buf + p->offsetText + mGetBuf32BE(ps + 2));
	}

	return NULL;
}


//======================
// 読み込み関連
//======================


/** mtr ファイル検索＆ファイル名取得
 *
 * 先にフル言語名で検索後、見つからなければ先頭2文字の言語名で検索 */

static mBool _getFileName(mStr *strDst,const char *lang,const char *dirpath)
{
	mStr str = MSTR_INIT,strlang = MSTR_INIT;
	char *pc;
	mBool ret = TRUE;

	//言語名 (最低2文字)

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
			mStrFindCharToEnd(&strlang, '.');
		}
	}

	if(strlang.len < 2)
	{
		mStrFree(&strlang);
		return FALSE;
	}

	//フル言語名で検索

	mStrPathCombine(&str, dirpath, strlang.buf, "mtr");

	if(!mIsFileExist(str.buf, FALSE))
	{
		//見つからなければ、先頭2文字で検索

		if(strlang.len == 2)
			ret = FALSE;
		else
		{
			mStrSetLen(&strlang, 2);

			mStrPathCombine(&str, dirpath, strlang.buf, "mtr");

			ret = mIsFileExist(str.buf, FALSE);
		}
	}

	//

	if(ret)
		mStrCopy(strDst, &str);

	mStrFree(&str);
	mStrFree(&strlang);

	return ret;
}

/** mtr ファイルから読み込み */

static mBool _loadFile(mTranslation *p,mFile file)
{
	uint8_t ver = 0xff;
	uint32_t textsize,size,itemnum;

	//ヘッダ

	if(!mFileReadCompareText(file, "MLIBTR")) return FALSE;

	//バージョン

	mFileRead(file, &ver, 1);
	if(ver != 0) return FALSE;

	//個数

	p->groupNum = mFileGet32BE(file);
	itemnum = mFileGet32BE(file);

	//テキストサイズ

	textsize = mFileGet32BE(file);

	//メモリ確保

	size = p->groupNum * 8 + itemnum * 6 + textsize;

	p->buf = (uint8_t *)mMalloc(size, FALSE);
	if(!p->buf) return FALSE;

	//データ読み込み

	if(!mFileReadSize(file, p->buf, size))
	{
		mFree(p->buf);
		return FALSE;
	}

	//

	p->offsetItem = p->groupNum * 8;
	p->offsetText = p->offsetItem + itemnum * 6;
	p->bLoadFile = TRUE;
	
	return TRUE;
}


//======================
// メイン
//======================


/** 解放 */

void mTranslationFree(mTranslation *p)
{
	if(p->buf && p->bLoadFile)
		mFree(p->buf);
}

/** 埋め込みデータセット */

void mTranslationSetEmbed(mTranslation *p,const void *buf)
{
	uint8_t *ps = (uint8_t *)buf;
	uint32_t itemnum;

	p->groupNum = mGetBuf32BE(ps);
	itemnum = mGetBuf32BE(ps + 4);

	p->buf = ps + 12;
	p->offsetItem = p->groupNum * 8;
	p->offsetText = p->offsetItem + itemnum * 6;
	p->bLoadFile = FALSE;
}

/** mtr ファイルを読み込み (指定ファイル)
 *
 * @param localestr filename はロケールの文字列 */

mBool mTranslationLoadFile(mTranslation *p,const char *filename,mBool localestr)
{
	mFile file;
	mBool ret;

	if(!mFileOpenRead(&file, filename, localestr? MFILEOPEN_FILENAME_LOCALE: 0))
		return FALSE;

	ret = _loadFile(p, file);

	mFileClose(file);

	return ret;
}

/** mtr ファイルを読み込み (ディレクトリ検索)
 *
 * @param lang "en_US" など。NULL でシステムの言語。
 * @param dirpath ファイル検索パス */

mBool mTranslationLoadFile_dir(mTranslation *p,const char *lang,const char *dirpath)
{
	mStr fname = MSTR_INIT;
	mBool ret;

	//ファイル名取得

	if(!_getFileName(&fname, lang, dirpath))
		return FALSE;

	//読み込み

	ret = mTranslationLoadFile(p, fname.buf, FALSE);

	mStrFree(&fname);

	return ret;
}

/** グループセット */

mBool mTranslationSetGroup(mTranslation *p,uint16_t id)
{
	p->curgroupItem = _findGroup(p, id, &p->curgroupItemNum);

	return (p->curgroupItem != NULL);
}

/** カレントグループから文字列取得 */

const char *mTranslationGetText(mTranslation *p,uint16_t id)
{
	return _findItem(p, p->curgroupItem, p->curgroupItemNum, id);
}

/** 指定グループから文字列取得
 *
 * カレントグループに変更なし */

const char *mTranslationGetText2(mTranslation *p,uint16_t groupid,uint16_t itemid)
{
	uint8_t *top;
	int num;

	top = _findGroup(p, groupid, &num);

	return _findItem(p, top, num, itemid);
}

