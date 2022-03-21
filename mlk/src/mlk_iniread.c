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
 * mIniRead
 *****************************************/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mlk.h"
#include "mlk_iniread.h"

#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_file.h"
#include "mlk_util.h"
#include "mlk_string.h"
#include "mlk_buf.h"
#include "mlk_unicode.h"


//--------------------

/* 項目リストアイテム */

typedef struct
{
	mListItem i;
	char *key,
		*param;
	uint32_t hash;
}_item;

/* グループリストアイテム */

typedef struct
{
	mListItem i;
	char *name;			//グループ名
	_item *item_top,	//項目の先頭 (NULL で一つもなし)
		*item_end;		//項目の終端
	uint32_t hash;
}_group;

/* mIniRead */

struct _mIniRead
{
	char *buf;			//テキストバッファ
	mList list_group,	//グループのリスト
		list_item;		//項目のリスト

	_group *curgroup;	//現在の読み込みグループ
	_item *curitem;		//現在の読み込みアイテム位置
};

//--------------------

#define _PUT_DEBUG  0
#define _GROUP(p)  ((_group *)(p))
#define _ITEM(p)   ((_item *)(p))

//--------------------



//=========================
// sub
//=========================


/* ハッシュ値計算 */

static uint32_t _calc_hash(const char *pc)
{
	uint32_t h = 0;
	char c;

	while((c = *(pc++)))
	{
		if(c >= 'A' && c <= 'Z') c += 0x20;
		
		h = h * 37 + c;
	}

	return h;
}

/* グループ追加
 *
 * pc:'[' の次の位置 */

static _group *_add_group(mIniRead *p,char *pc)
{
	_group *pi;
	char *p2;

	p2 = strchr(pc, ']');
	if(p2) *p2 = 0;

	//グループ名が空

	if(*pc == 0) return NULL;

	//追加

	pi = (_group *)mListAppendNew(&p->list_group, sizeof(_group));

	if(pi)
	{
		pi->name = pc;
		pi->hash = _calc_hash(pc);
	}

	return pi;
}

/* グループの情報セット (グループを閉じる時) */

static void _set_groupinfo(mIniRead *p,_group *group,mListItem *lastitem)
{
	if(!group) return;

	/* グループ追加時の最後のアイテムが、現在の最後のアイテムと同じ
	 *  = グループに項目が一つもない */

	if(lastitem != p->list_item.bottom)
	{
		//グループの項目の先頭と終端をセット
		
		group->item_top = (lastitem)? _ITEM(lastitem->next): _ITEM(p->list_item.top);
		group->item_end = _ITEM(p->list_item.bottom);
	}
}

/* 項目追加 */

static void _add_item(mIniRead *p,char *pc)
{
	char *p2;
	_item *pi;

	//'=' がない場合は無効

	p2 = strchr(pc, '=');
	if(!p2) return;

	*p2 = 0;

	//キー名が空

	if(*pc == 0) return;

	//追加

	pi = (_item *)mListAppendNew(&p->list_item, sizeof(_item));
	if(!pi) return;

	pi->key = pc;
	pi->param = p2 + 1;
	pi->hash = _calc_hash(pc);

	mUTF8Validate(pi->param, -1);
}

/* バッファからリスト作成 */

static void _create_list(mIniRead *p)
{
	char *pc,*next,c;
	_group *group = NULL;	//現在のグループ
	mListItem *lastitem = NULL;	//グループ追加時点の最後の項目

	for(pc = p->buf; 1; pc = next)
	{
		//行取得
		
		next = mStringGetNextLine(pc, TRUE);
		if(!next) break;

		//空行

		if(*pc == 0) continue;

		//----- 処理

		c = *pc;

		if(c == '[')
		{
			//--- '[' でグループの開始

			//前のグループの情報をセット

			_set_groupinfo(p, group, lastitem);

			//グループ追加

			group = _add_group(p, pc + 1);

			//最後の項目を記録

			lastitem = p->list_item.bottom;
		}
		else if(isalnum(c) || c == '_')
		{
			//'_' or 英数字 => 項目

			_add_item(p, pc);
		}
	}

	//最後のグループの情報セット

	_set_groupinfo(p, group, lastitem);
}

/* カレントグループセット
 *
 * return:グループ名 */

static const char *_set_group(mIniRead *p,_group *group)
{
	p->curgroup = group;

	if(group)
	{
		p->curitem = group->item_top;
		return group->name;
	}
	else
	{
		p->curitem = NULL;
		return NULL;
	}
}

/* カレントグループからキーを検索
 *
 * return:キーの値の文字列。キーが存在しない場合 NULL */

static char *_search_key(mIniRead *p,const char *key)
{
	_group *group = p->curgroup;
	_item *pi;
	uint32_t hash;

	if(!group) return NULL;

	hash = _calc_hash(key);

	for(pi = group->item_top; pi; pi = _ITEM(pi->i.next))
	{
		if(pi->hash == hash && strcasecmp(pi->key, key) == 0)
			return pi->param;
	
		if(pi == group->item_end) break;
	}

	return NULL;
}


//=========================
// メイン
//=========================



/**@ 終了 */

void mIniRead_end(mIniRead *p)
{
	if(p)
	{
		mFree(p->buf);
		mListDeleteAll(&p->list_group);
		mListDeleteAll(&p->list_item);

		mFree(p);
	}
}

/**@ ファイルから読み込み
 *
 * @p:dst 作成された mIniRead のポインタが入る。\
 *  ファイルが読み込めなかった場合も、デフォルトの値を取得することが必要なため、常に mIniRead の作成は行われる。\
 *  作成に失敗したときのみ NULL が入る。

 * @r:MLKERR_OK で、ファイルを読み込んだ。\
 *  MLKERR_OPEN でファイルが開けなかった。EMPTY でファイルが空。ほか、エラー。\
 *  {em:※ファイルが存在しない場合でも、通常は値の取得を行うため、戻り値でエラーを判断せず、
 *  mIniRead が NULL なら値の取得処理を行わないようにする。:em} */

mlkerr mIniRead_loadFile(mIniRead **dst,const char *filename)
{
	mIniRead *ini;
	uint8_t *buf;
	mlkerr ret;

	//確保

	ini = (mIniRead *)mMalloc0(sizeof(mIniRead));
	if(!ini)
	{
		*dst = NULL;
		return MLKERR_ALLOC;
	}

	*dst = ini;

	//読み込み (終端に 0 追加。空の場合はエラー)

	ret = mReadFileFull_alloc(filename, MREADFILEFULL_APPEND_0, &buf, NULL);
	if(ret) return ret;

	//読み込めた場合

	ini->buf = (char *)buf;

	_create_list(ini);

	ini->curitem = _ITEM(ini->list_item.top);

	//debug

#if _PUT_DEBUG
	_group *pig;
	_item *pii;

	mDebug("[group]\n");

	for(pig = _GROUP(ini->list_group.top); pig; pig = _GROUP(pig->i.next))
		mDebug("%s top:%p end:%p\n", pig->name, pig->item_top, pig->item_end);

	mDebug("[item]\n");

	for(pii = _ITEM(ini->list_item.top); pii; pii = _ITEM(pii->i.next))
		mDebug("%p key:%s param:%s hash:0x%x\n", pii, pii->key, pii->param, pii->hash);
#endif

	return MLKERR_OK;
}

/**@ ファイルから読み込み
 *
 * @d:ディレクトリとファイル名を指定。
 *
 * @p:path ディレクトリパス (NULL でなし)
 * @p:filename ファイル名 */

mlkerr mIniRead_loadFile_join(mIniRead **dst,const char *path,const char *filename)
{
	mStr str = MSTR_INIT;
	mlkerr ret;

	if(path)
	{
		mStrSetText(&str, path);
		mStrPathJoin(&str, filename);
	}
	else
		mStrSetText(&str, filename);

	ret = mIniRead_loadFile(dst, str.buf);

	mStrFree(&str);

	return ret;
}

/**@ データが空か
 *
 * @d:ファイルが存在しなかった場合・ファイル内容が空の場合・
 * グループが一つも存在しない場合に FALSE が返る。 */

mlkbool mIniRead_isEmpty(mIniRead *p)
{
	return (!p->list_group.top);
}

/**@ データを空にする */

void mIniRead_setEmpty(mIniRead *p)
{
	mFree(p->buf);
	mListDeleteAll(&p->list_group);
	mListDeleteAll(&p->list_item);

	p->buf = NULL;
	p->curgroup = NULL;
	p->curitem = NULL;
}


//=======================
// グループ
//=======================


/**@ カレントグループセット (グループ名)
 *
 * @r:FALSE でグループが見つからなかった */

mlkbool mIniRead_setGroup(mIniRead *p,const char *name)
{
	_group *pi;
	uint32_t hash;

	//検索

	hash = _calc_hash(name);

	for(pi = _GROUP(p->list_group.top); pi; pi = _GROUP(pi->i.next))
	{
		if(hash == pi->hash && strcasecmp(name, pi->name) == 0)
		{
			_set_group(p, pi);
			return TRUE;
		}
	}

	//見つからなかった

	p->curgroup = NULL;
	p->curitem = NULL;

	return FALSE;
}

/**@ カレントグループセット (数値から) */

mlkbool mIniRead_setGroupNo(mIniRead *p,int no)
{
	char m[16];

	mIntToStr(m, no);

	return mIniRead_setGroup(p, m);
}

/**@ 先頭グループをカレントグループにする
 *
 * @r:先頭グループ名。NULL でグループなし。 */

const char *mIniRead_setGroupTop(mIniRead *p)
{
	return _set_group(p, _GROUP(p->list_group.top));
}

/**@ 次のグループをカレントグループにする
 *
 * @r:現在のグループ名。NULL で次のグループがない。 */

const char *mIniRead_setGroupNext(mIniRead *p)
{
	if(!p->curgroup)
		return NULL;
	else
		return _set_group(p, _GROUP((p->curgroup)->i.next));
}

/**@ 現在のグループの項目数取得 */

int mIniRead_getGroupItemNum(mIniRead *p)
{
	_group *group = p->curgroup;
	_item *pi;
	int num;

	if(!group) return 0;

	for(pi = group->item_top, num = 0; pi; pi = _ITEM(pi->i.next))
	{
		num++;
		if(pi == group->item_end) break;
	}

	return num;
}

/**@ 現在のグループ内に指定キーが存在するか */

mlkbool mIniRead_groupIsHaveKey(mIniRead *p,const char *key)
{
	return (_search_key(p, key) != NULL);
}


//======================
// 値取得
//======================


/**@ グループ内の次の項目を取得
 *
 * @p:key キーの文字列ポインタが格納される
 * @p:param 値の文字列ポインタが格納される
 * @r:FALSE で次の項目がない */

mlkbool mIniRead_getNextItem(mIniRead *p,const char **key,const char **param)
{
	if(!p->curitem)
		return FALSE;
	else
	{
		*key = p->curitem->key;
		*param = p->curitem->param;

		//次へ

		if(p->curgroup && p->curgroup->item_end == p->curitem)
			//グループの最後なら次で終了
			p->curitem = NULL;
		else
			//次へ
			p->curitem = _ITEM(p->curitem->i.next);

		return TRUE;
	}
}

/**@ グループ内の次の項目から、キーの数値と 32bit 数値を取得
 *
 * @p:keyno キーの数値が格納される
 * @p:buf 値の数値が格納される (int32_t or uint32_t)
 * @p:hex 値が16進数文字列か */

mlkbool mIniRead_getNextItem_keyno_int32(mIniRead *p,int *keyno,void *buf,mlkbool hex)
{
	const char *key,*param;

	if(!mIniRead_getNextItem(p, &key, &param))
		return FALSE;
	else
	{
		*keyno = atoi(key);

		if(hex)
			*((uint32_t *)buf) = strtoul(param, NULL, 16);
		else
			*((int32_t *)buf) = atoi(param);

		return TRUE;
	}
}


//===================


/**@ int 値取得
 *
 * @g:値取得
 *
 * @p:def キーがなかった場合のデフォルト値 */

int mIniRead_getInt(mIniRead *p,const char *key,int def)
{
	char *pc = _search_key(p, key);

	if(pc)
		return atoi(pc);
	else
		return def;
}

/**@ 16進数数値取得 */

uint32_t mIniRead_getHex(mIniRead *p,const char *key,uint32_t def)
{
	char *pc = _search_key(p, key);

	if(pc)
		return (uint32_t)strtoul(pc, NULL, 16);
	else
		return def;
}

/**@ 値の文字列を比較
 *
 * @p:comptxt 比較する文字列
 * @p:iscase TRUE で大文字小文字を区別しない
 * @r:同じ文字列なら TRUE、キーがない・等しくない場合は FALSE */

mlkbool mIniRead_compareText(mIniRead *p,const char *key,const char *comptxt,mlkbool iscase)
{
	char *pc = _search_key(p, key);

	if(!pc)
		return FALSE;
	else if(iscase)
		return (strcasecmp(pc, comptxt) == 0);
	else
		return (strcmp(pc, comptxt) == 0);
}

/**@ 文字列を取得
 *
 * @d:バッファ内のポインタが返る。解放などは行わないこと。 */

const char *mIniRead_getText(mIniRead *p,const char *key,const char *def)
{
	char *pc = _search_key(p, key);

	return (pc)? pc: def;
}

/**@ 文字列を取得 (複製)
 *
 * @r:複製された文字列。NULL または空文字列の場合は、NULL。 */

char *mIniRead_getText_dup(mIniRead *p,const char *key,const char *def)
{
	const char *txt;

	txt = mIniRead_getText(p, key, def);

	if(!txt || !(*txt))
		return NULL;
	else
		return mStrdup(txt);
}

/**@ バッファに文字列取得
 *
 * @p:size バッファのサイズ
 * @p:def デフォルトの文字列。NULL で空文字列。
 * @r:セットされた文字列の長さ */

int mIniRead_getTextBuf(mIniRead *p,const char *key,char *buf,uint32_t size,const char *def)
{
	char *pc = _search_key(p, key);
	int len;

	if(!pc) pc = (char *)def;

	if(!pc)
	{
		//空文字列
		buf[0] = 0;
		return 0;
	}
	else
	{
		len = strlen(pc);
		if(len >= size) len = size - 1;

		memcpy(buf, pc, len);
		buf[len] = 0;

		return mUTF8Validate(buf, len);
	}
}

/**@ 文字列を mStr で取得
 *
 * @p:def NULL で空文字列
 * @r:キーが存在した場合 TRUE */

mlkbool mIniRead_getTextStr(mIniRead *p,const char *key,mStr *str,const char *def)
{
	char *pc = _search_key(p, key);

	mStrSetText(str, (pc)? pc: def);

	return (pc != NULL);
}

/**@ mStr 文字列取得 (キー名は番号)
 *
 * @r:キーが存在した場合 TRUE */

mlkbool mIniRead_getTextStr_keyno(mIniRead *p,int keyno,mStr *str,const char *def)
{
	char m[16];

	mIntToStr(m, keyno);

	return mIniRead_getTextStr(p, m, str, def);
}

/**@ mStr 配列に文字列を取得
 *
 * @d:キーが数値で連番の複数項目を、mStr の配列に文字列として取得。\
 * キーがない場合のデフォルトは空文字列となる。
 *
 * @p:keytop キーの数値の先頭
 * @p:maxnum 配列の数 */

void mIniRead_getTextStrArray(mIniRead *p,int keytop,mStr *array,int arraynum)
{
	int i;

	for(i = 0; i < arraynum; i++)
		mIniRead_getTextStr_keyno(p, keytop + i, array + i, NULL);
}

/**@ カンマで区切られた複数数値を配列で取得
 *
 * @p:buf 数値配列のバッファ
 * @p:maxnum 配列の最大個数
 * @p:bytes 数値1つのバイト数 (1,2,4)
 * @p:hex 16進数文字列か
 * @r:読み込まれた数値の数 */

int mIniRead_getNumbers(mIniRead *p,const char *key,void *buf,int maxnum,int bytes,mlkbool hex)
{
	uint8_t *pd = (uint8_t *)buf;
	char *srcbuf,*ps;
	int n,num = 0;

	//値の文字列から、0 で区切った文字列を作成

	srcbuf = mStringDup_replace0(mIniRead_getText(p, key, ""), ',');
	if(!srcbuf) return 0;

	//

	for(ps = srcbuf; *ps && num < maxnum; ps += strlen(ps) + 1)
	{
		if(hex)
			n = strtoul(ps, NULL, 16);
		else
			n = atoi(ps);

		//セット

		switch(bytes)
		{
			case 2:
				*((uint16_t *)pd) = n;
				pd += 2;
				break;
			case 4:
				*((int32_t *)pd) = n;
				pd += 4;
				break;
			default:
				*(pd++) = n;
				break;
		}

		num++;
	}

	mFree(srcbuf);

	return num;
}

/**@ カンマで区切られた値を配列としてメモリ確保して取得
 *
 * @p:bytes 数値1つのバイト数 (1,2,4)
 * @p:hex  16進数文字列か
 * @p:append_bytes 配列の終端に追加するバイト数
 * @p:psize NULL 以外の場合、確保したバイト数が格納される
 * @r:確保したバッファ (空の場合は NULL) */

void *mIniRead_getNumbers_alloc(mIniRead *p,const char *key,int bytes,mlkbool hex,int append_bytes,uint32_t *psize)
{
	mBuf mem;
	char *srcbuf,*ps;
	uint8_t *dstbuf;
	int32_t n;
	uint16_t n16;
	mlkbool ret;

	if(psize) *psize = 0;

	//カンマを 0 に置き換えた文字列

	srcbuf = mStringDup_replace0(mIniRead_getText(p, key, ""), ',');
	if(!srcbuf) return NULL;

	//作業用バッファ

	mBufInit(&mem);

	if(!mBufAlloc(&mem, bytes * 64, bytes * 64)) goto ERR;

	//データ取得

	for(ps = srcbuf; *ps; ps += strlen(ps) + 1)
	{
		if(hex)
			n = strtoul(ps, NULL, 16);
		else
			n = atoi(ps);

		switch(bytes)
		{
			case 2:
				n16 = n;
				ret = mBufAppend(&mem, &n16, 2);
				break;
			case 4:
				ret = mBufAppend(&mem, &n, 4);
				break;
			default:
				ret = mBufAppendByte(&mem, n);
				break;
		}

		if(!ret) goto ERR;
	}

	mFree(srcbuf);
	srcbuf = NULL;

	//追加サイズ

	if(append_bytes > 0
		&& !mBufAppend0(&mem, append_bytes))
		goto ERR;

	//結果

	dstbuf = mBufCopyToBuf(&mem);

	if(psize && dstbuf)
		*psize = mem.cursize;

	mBufFree(&mem);

	return dstbuf;

ERR:
	mFree(srcbuf);
	mBufFree(&mem);
	return NULL;
}

/**@ カンマで区切られた mPoint 取得 */

mlkbool mIniRead_getPoint(mIniRead *p,const char *key,mPoint *pt,int defx,int defy)
{
	int n[2];

	if(mIniRead_getNumbers(p, key, n, 2, 4, FALSE) < 2)
	{
		pt->x = defx;
		pt->y = defy;
		return FALSE;
	}
	else
	{
		pt->x = n[0];
		pt->y = n[1];
		return TRUE;
	}
}

/**@ カンマで区切られた mSize 取得 */

mlkbool mIniRead_getSize(mIniRead *p,const char *key,mSize *size,int defw,int defh)
{
	int n[2];

	if(mIniRead_getNumbers(p, key, n, 2, 4, FALSE) < 2)
	{
		size->w = defw;
		size->h = defh;
		return FALSE;
	}
	else
	{
		size->w = n[0];
		size->h = n[1];
		return TRUE;
	}
}

/**@ カンマで区切られた mBox 取得 */

mlkbool mIniRead_getBox(mIniRead *p,const char *key,mBox *box,int defx,int defy,int defw,int defh)
{
	int n[4];

	if(mIniRead_getNumbers(p, key, n, 4, 4, FALSE) < 4)
	{
		box->x = defx;
		box->y = defy;
		box->w = defw;
		box->h = defh;
		return FALSE;
	}
	else
	{
		box->x = n[0];
		box->y = n[1];
		box->w = n[2];
		box->h = n[3];
		return TRUE;
	}
}

/**@ Base64 をデコードして取得
 *
 * @p:psize NULL 以外で、データの元サイズが入る
 * @r:確保されたバッファが返る。\
 * NULL でキーがないか、エラー。 */

void *mIniRead_getBase64(mIniRead *p,const char *key,uint32_t *psize)
{
	char *pc = _search_key(p, key);
	int srcsize,ret;
	uint8_t *buf;

	if(!pc) return NULL;

	//元データサイズ

	srcsize = atoi(pc);
	if(srcsize <= 0) return NULL;

	//Base64 コード位置

	for(; *pc && *pc != ':'; pc++);

	if(*pc == 0) return NULL;

	pc++;

	//確保

	buf = (uint8_t *)mMalloc(srcsize);
	if(!buf) return NULL;

	//デコード

	ret = mDecodeBase64(buf, srcsize, pc, -1);

	if(ret < 0)
	{
		mFree(buf);
		return NULL;
	}

	if(psize) *psize = ret;

	return buf;
}

