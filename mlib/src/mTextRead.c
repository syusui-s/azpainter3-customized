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
 * mTextRead
 *****************************************/

#include "mDef.h"

#include "mTextRead.h"
#include "mUtilFile.h"
#include "mUtilStr.h"


//-----------------

struct _mTextRead
{
	char *buf,*cur,*next;
	uint32_t size;
};

//-----------------


/**
@defgroup textread mTextRead
@brief テキストファイル読み込み
@ingroup group_etc
 
@{
@file mTextRead.h
*/


/** 終了 */

void mTextReadEnd(mTextRead *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** ファイルから読込 */

mTextRead *mTextRead_readFile(const char *filename)
{
	mTextRead *p;
	mBuf buf;

	//読み込み

	if(!mReadFileFull(filename, MREADFILEFULL_ADD_NULL | MREADFILEFULL_ACCEPT_EMPTY, &buf))
		return NULL;

	//

	p = (mTextRead *)mMalloc(sizeof(mTextRead), TRUE);
	if(!p)
		mFree(buf.buf);
	else
	{
		p->buf = p->cur = p->next = buf.buf;
		p->size = buf.size;
	}

	return p;
}

/** 一行読み込み
 *
 * バッファ内の改行を NULL に変換して、バッファ内のポインタを返す。 @n
 * 先頭から順に一度だけ読み込む場合に使う。 @n
 * 返ったポインタから NULL 文字までの範囲は内容を変更しても構わない。
 * 
 * @return バッファ内のポインタが返る。NULL で終了 */

char *mTextReadGetLine(mTextRead *p)
{
	if(!p->next)
		return NULL;
	else
	{
		p->cur = p->next;
		p->next = mGetStrNextLine(p->cur, TRUE);

		return p->cur;
	}
}

/** 一行読み込み (空行の場合はスキップ) */

char *mTextReadGetLine_skipEmpty(mTextRead *p)
{
	while(p->next)
	{
		p->cur = p->next;
		p->next = mGetStrNextLine(p->cur, TRUE);

		if(*(p->cur)) break;
	}

	return (p->next)? p->cur: NULL;
}

/** @} */
