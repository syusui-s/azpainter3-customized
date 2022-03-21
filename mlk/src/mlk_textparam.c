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
 * mTextParam
 *****************************************/

#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "mlk.h"
#include "mlk_textparam.h"
#include "mlk_str.h"
#include "mlk_list.h"
#include "mlk_string.h"
#include "mlk_buf.h"


//----------------------

struct _mTextParam
{
	mBuf buf;	//文字列を格納するバッファ
	mList list;  //キーと値のリスト
};

/* リストアイテム */

typedef struct
{
	mListItem i;

	char *key,	//buf 内の位置
		*param;	//キーのみの場合、空文字列
	uint32_t hash;     //キーのハッシュ値
}_listitem;

//----------------------


//==============================
// sub
//==============================


/* ハッシュ値計算 */

static uint32_t _get_hash(const char *pc)
{
	uint32_t h = 0;
	char c;

	while((c = (*pc++)))
	{
		if(c >= 'A' && c <= 'Z') c += 0x20;
	
		h = h * 37 + c;
	}

	return h;
}

/* 文字列からリスト作成 */

static mlkbool _create_list(mTextParam *p,const char *text,char splitpair,char splitparam)
{
	mStr str = MSTR_INIT;
	mBuf *dstbuf;
	char *pc,*end,*next;
	_listitem *pi;
	int loop = 1,ret = 0;

	dstbuf = &p->buf;

	for(pc = (char *)text; loop && *pc; pc = next)
	{
		//区切り位置
		// '\' + 区切り文字は対象外

		end = mStringFindChar(pc, splitpair);
			
		while(*end == splitpair && end != pc && end[-1] == '\\')
			end = mStringFindChar(end + 1, splitpair);

		//終端なら次でループ終了

		if(!(*end)) loop = 0;

		//中が空なら次へ

		next = end + 1;

		if(pc == end) continue;

		//エスケープをデコードした文字列

		mStrSetText_len(&str, pc, end - pc);
		mStrDecodeEscape(&str);

		//文字列用バッファに追加

		pc = (char *)mBufAppend_getptr(dstbuf, str.buf, str.len + 1);
		if(!pc) goto ERR;

		//キーと値の区切り文字位置 (NULL でキーのみ)

		end = strchr(pc, splitparam);

		if(end) *(end++) = 0;

		//項目追加

		pi = (_listitem *)mListAppendNew(&p->list, sizeof(_listitem));
		if(!pi) goto ERR;

		pi->key = pc;
		pi->param = (end)? end: pc + str.len;
		pi->hash = _get_hash(pc);
	}

	ret = 1;

ERR:
	mStrFree(&str);

	return ret;
}

/* キー名から検索して、値の位置を取得
 *
 * return: NULL でキーが見つからなかった */

static char *_find(mTextParam *p,const char *key)
{
	_listitem *pi;
	uint32_t hash;

	hash = _get_hash(key);

	for(pi = (_listitem *)p->list.top; pi; pi = (_listitem *)pi->i.next)
	{
		if(hash == pi->hash && strcasecmp(key, pi->key) == 0)
			return pi->param;
	}

	return NULL;
}


//==============================
// main
//==============================


/**@ 解放 */

void mTextParam_free(mTextParam *p)
{
	if(p)
	{
		mBufFree(&p->buf);
		mListDeleteAll(&p->list);
		
		mFree(p);
	}
}

/**@ 文字列から mTextParam 作成
 *
 * @p:text '\\' はエスケープ文字として使われる
 * @p:splitpair ';' など、ペアを区切る文字
 * @p:splitparam '=' など、キーと値を区切る文字 (-1 で '=') */

mTextParam *mTextParam_new(const char *text,int splitpair,int splitparam)
{
	mTextParam *p;

	if(splitparam < 0) splitparam = '=';

	p = (mTextParam *)mMalloc0(sizeof(mTextParam));
	if(!p) return NULL;

	//mBuf

	if(!mBufAlloc(&p->buf, 256, 256))
	{
		mFree(p);
		return NULL;
	}

	//リストを作成

	_create_list(p, text, splitpair, splitparam);

	return p;
}


/**@ int 値を取得
 *
 * @d:値部分の文字列は、16進数 (0x...)、8進数 (0...) にも対応
 * @r:FALSE でキーが見つからなかった */

mlkbool mTextParam_getInt(mTextParam *p,const char *key,int *dst)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*dst = strtol(pc, NULL, 0);
		return TRUE;
	}
	else
		return FALSE;
}

/**@ int 値を取得 (範囲指定) */

mlkbool mTextParam_getIntRange(mTextParam *p,const char *key,int *dst,int min,int max)
{
	int n;

	if(!mTextParam_getInt(p, key, &n))
		return FALSE;
	else
	{
		if(n < min) n = min;
		else if(n > max) n = max;

		*dst = n;

		return TRUE;
	}
}

/**@ double 値を取得 */

mlkbool mTextParam_getDouble(mTextParam *p,const char *key,double *dst)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*dst = strtod(pc, NULL);
		return TRUE;
	}
	else
		return FALSE;
}

/**@ double 値を指定桁数の int にして取得 */

mlkbool mTextParam_getDoubleInt(mTextParam *p,const char *key,int *dst,int dig)
{
	double d;
	int n;

	if(!mTextParam_getDouble(p, key, &d))
		return FALSE;
	else
	{
		for(n = 1; dig > 0; dig--, n *= 10);
	
		*dst = (int)round(d * n);

		return TRUE;
	}
}

/**@ double 値を指定桁数の int にして取得 (範囲指定) */

mlkbool mTextParam_getDoubleIntRange(mTextParam *p,const char *key,int *dst,int dig,int min,int max)
{
	int n;

	if(!mTextParam_getDoubleInt(p, key, &n, dig))
		return FALSE;
	else
	{
		if(n < min) n = min;
		else if(n > max) n = max;

		*dst = n;

		return TRUE;
	}
}

/**@ 文字列を取得 (バッファ内の位置)
 *
 * @d:取得したポインタは解放したりしないこと。 */

mlkbool mTextParam_getTextRaw(mTextParam *p,const char *key,char **dst)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*dst = pc;
		return TRUE;
	}
	else
		return FALSE;
}

/**@ 文字列を複製して取得 */

mlkbool mTextParam_getTextDup(mTextParam *p,const char *key,char **buf)
{
	char *pc = _find(p, key);

	if(pc)
	{
		*buf = mStrdup(pc);
		return TRUE;
	}
	else
		return FALSE;
}

/**@ 文字列を取得 (mStr) */

mlkbool mTextParam_getTextStr(mTextParam *p,const char *key,mStr *str)
{
	char *pc = _find(p, key);

	if(pc)
	{
		mStrSetText(str, pc);
		return TRUE;
	}
	else
		return FALSE;
}

/**@ 値の文字列が指定単語リストのいずれかに一致する場合、そのインデックス番号を取得
 *
 * @p:words NULL 文字で区切った単語リスト。最後は2つのヌル文字で終わること。
 * @p:iscase TRUE で大文字小文字を区別しない
 * @r:一致するものがあった場合、インデックス番号。\
 * キーがない、または一致しなかった場合、-1。 */

int mTextParam_findWord(mTextParam *p,const char *key,const char *wordlist,mlkbool iscase)
{
	char *param = _find(p, key);
	const char *pc;
	int ret,no;

	if(param)
	{
		for(pc = wordlist, no = 0; *pc; pc += strlen(pc) + 1, no++)
		{
			if(iscase)
				ret = strcasecmp(pc, param);
			else
				ret = strcmp(pc, param);

			if(ret == 0) return no;
		}
	}

	return -1;
}
