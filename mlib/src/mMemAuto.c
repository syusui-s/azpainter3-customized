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
 * mMemAuto
 *****************************************/

#include <string.h>

#include "mDef.h"
#include "mMemAuto.h"


/*************//**

@defgroup memauto mMemAuto
@brief 自動拡張メモリバッファ
@ingroup group_data
 
@{

@file mMemAuto.h
@struct mMemAuto

******************/


//========================


/** サイズ追加 */

static mBool _resize_append(mMemAuto *p,uintptr_t size)
{
	uint8_t *newbuf;
	uintptr_t addsize,newsize;

	if(size == 0) return TRUE;
	if(!p->buf) return FALSE;

	//サイズ拡張

	addsize = p->curpos + size;

	if(addsize > p->allocsize)
	{
		newsize = p->allocsize + p->appendsize;

		if(newsize < addsize) newsize = addsize;

		newbuf = (uint8_t *)mRealloc(p->buf, newsize);
		if(!newbuf) return FALSE;

		p->buf = newbuf;
		p->allocsize = newsize;
	}

	return TRUE;
}


//========================


/** 構造体初期化 */

void mMemAutoInit(mMemAuto *p)
{
	p->buf = NULL;
	p->allocsize = p->curpos = 0;
}

/** 解放 */

void mMemAutoFree(mMemAuto *p)
{
	if(p && p->buf)
	{
		mFree(p->buf);

		p->buf = NULL;
		p->allocsize = p->curpos = 0;
	}
}

/** 初期確保
 *
 * @param initsize 初期確保サイズ
 * @param appendsize 自動拡張時に追加するサイズ */

mBool mMemAutoAlloc(mMemAuto *p,uintptr_t initsize,uintptr_t appendsize)
{
	mMemAutoFree(p);

	p->buf = (uint8_t *)mMalloc(initsize, FALSE);
	if(!p->buf) return FALSE;

	p->allocsize = initsize;
	p->curpos = 0;
	p->appendsize = appendsize;

	return TRUE;
}

/** データ追加の現在位置をリセット */

void mMemAutoReset(mMemAuto *p)
{
	p->curpos = 0;
}

/** データ追加位置を指定サイズ前に戻す */

void mMemAutoBack(mMemAuto *p,int size)
{
	if(size < p->curpos)
		p->curpos -= size;
	else
		p->curpos = 0;
}

/** データ追加の残りサイズ取得 */

uintptr_t mMemAutoGetRemain(mMemAuto *p)
{
	return p->allocsize - p->curpos;
}

/** 終端位置のポインタ取得 */

uint8_t *mMemAutoGetBottom(mMemAuto *p)
{
	return p->buf + p->curpos;
}

/** 現在の位置サイズでサイズ変更する */

void mMemAutoCutCurrent(mMemAuto *p)
{
	uint8_t *newbuf;

	if(p->curpos && p->curpos != p->allocsize)
	{
		newbuf = (uint8_t *)mRealloc(p->buf, p->curpos);
		if(!newbuf) return;

		p->buf = newbuf;
		p->allocsize = p->curpos;
	}
}

/** データを追加 */

mBool mMemAutoAppend(mMemAuto *p,void *dat,uintptr_t size)
{
	if(!dat) return TRUE;

	//サイズ拡張

	if(!_resize_append(p, size))
		return FALSE;

	//データ追加

	memcpy(p->buf + p->curpos, dat, size);

	p->curpos += size;

	return TRUE;
}

/** 1byte データ追加 */

mBool mMemAutoAppendByte(mMemAuto *p,uint8_t dat)
{
	return mMemAutoAppend(p, &dat, 1);
}

/** 指定サイズ分のゼロ値を追加 */

mBool mMemAutoAppendZero(mMemAuto *p,int size)
{
	if(!_resize_append(p, size))
		return FALSE;
	else
	{
		memset(p->buf + p->curpos, 0, size);
		p->curpos += size;

		return TRUE;
	}
}

/** @} */
