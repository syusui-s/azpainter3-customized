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
 * mBuf
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_buf.h"
#include "mlk_unicode.h"



/**@ 指定サイズ分を追加してリサイズ */

static mlkbool _resize_add(mBuf *p,int size)
{
	uint8_t *newbuf;
	mlksize addsize,newsize;

	if(size == 0) return TRUE;
	
	if(!p->buf) return FALSE;

	addsize = p->cursize + size;

	if(addsize > p->allocsize)
	{
		//リサイズ
		
		newsize = p->allocsize + p->expand_size;
		if(newsize < addsize) newsize = addsize;

		newbuf = (uint8_t *)mRealloc(p->buf, newsize);
		if(!newbuf) return FALSE;

		p->buf = newbuf;
		p->allocsize = newsize;
	}

	return TRUE;
}


//========================


/**@ 初期化 */

void mBufInit(mBuf *p)
{
	p->buf = NULL;
	p->allocsize = p->cursize = 0;
}

/**@ 解放 */

void mBufFree(mBuf *p)
{
	if(p)
	{
		mFree(p->buf);

		p->buf = NULL;
		p->allocsize = p->cursize = 0;
	}
}

/**@ 初期確保
 *
 * @d:すでに確保されている場合は、先に解放される。
 *
 * @p:allocsize 初期確保サイズ
 * @p:expand_size  自動拡張時に追加する単位 */

mlkbool mBufAlloc(mBuf *p,mlksize allocsize,mlksize expand_size)
{
	mBufFree(p);

	p->buf = (uint8_t *)mMalloc(allocsize);
	if(!p->buf) return FALSE;

	p->allocsize = allocsize;
	p->cursize = 0;
	p->expand_size = expand_size;

	return TRUE;
}

/**@ データ追加の現在位置をリセット */

void mBufReset(mBuf *p)
{
	p->cursize = 0;
}

/**@ 現在の位置をセット */

void mBufSetCurrent(mBuf *p,mlksize pos)
{
	p->cursize = pos;
}

/**@ データ追加位置を指定サイズ前に戻す */

void mBufBack(mBuf *p,int size)
{
	if(size < p->cursize)
		p->cursize -= size;
	else
		p->cursize = 0;
}

/**@ データ追加の残りサイズ取得 */

mlksize mBufGetRemain(mBuf *p)
{
	return p->allocsize - p->cursize;
}

/**@ 現在のデータ追加位置のポインタ取得 */

uint8_t *mBufGetCurrent(mBuf *p)
{
	return p->buf + p->cursize;
}

/**@ 現在の位置に合わせてバッファをサイズ変更する
 *
 * @d:余ったバッファを切り捨てる。\
 * 現在位置が 0 の場合は何もしない。 */

void mBufCutCurrent(mBuf *p)
{
	uint8_t *newbuf;

	if(p->cursize && p->cursize != p->allocsize)
	{
		newbuf = (uint8_t *)mRealloc(p->buf, p->cursize);
		if(!newbuf) return;

		p->buf = newbuf;
		p->allocsize = p->cursize;
	}
}

/**@ データを追加
 *
 * @d:buf が NULL、または size が 0 以下の場合は何もしない
 * @r:リサイズに失敗した場合 FALSE */

mlkbool mBufAppend(mBuf *p,const void *buf,int size)
{
	if(!buf || size <= 0)
		return TRUE;

	//サイズ拡張

	if(!_resize_add(p, size))
		return FALSE;

	//データ追加

	memcpy(p->buf + p->cursize, buf, size);

	p->cursize += size;

	return TRUE;
}

/**@ データを追加 (先頭位置のポインタを返す)
 *
 * @r:追加したデータの先頭位置のポインタ。NULL で失敗。 */

void *mBufAppend_getptr(mBuf *p,const void *buf,int size)
{
	mlksize pos;

	pos = p->cursize;

	if(!mBufAppend(p, buf, size))
		return NULL;
	else
		return (void *)(p->buf + pos);
}

/**@ 1byte データ追加 */

mlkbool mBufAppendByte(mBuf *p,uint8_t dat)
{
	return mBufAppend(p, &dat, 1);
}

/**@ 指定サイズ分のゼロ値を追加 */

mlkbool mBufAppend0(mBuf *p,int size)
{
	if(!_resize_add(p, size))
		return FALSE;

	memset(p->buf + p->cursize, 0, size);
	p->cursize += size;

	return TRUE;
}

/**@ UTF-8 文字列を追加
 *
 * @d:正規化した UTF-8 文字列を追加。\
 * ヌル文字は常にセットされる。\
 * ※空文字列の場合もセットされる。
 * 
 * @p:len  ヌル文字を含まないサイズ。負の値で自動。
 * @r:セットされた文字列先頭のデータ位置。\
 *  -1 で、buf == NULL または、エラー。\
 *  {em:※バッファがリサイズされると、ポインタ位置が変わる場合があるため、ポインタで直接扱わないようにすること。:em} */

int32_t mBufAppendUTF8(mBuf *p,const char *buf,int len)
{
	char *top;
	int pos;

	if(!buf) return -1;

	if(len < 0)
		len = mStrlen(buf);

	if(!_resize_add(p, len + 1)) return -1;

	//セット

	pos = p->cursize;

	top = (char *)(p->buf + p->cursize);

	len = mUTF8CopyValidate(top, buf, len);

	top[len] = 0;

	p->cursize += len + 1;

	return pos;
}

/**@ 新しく確保したバッファにデータをコピーする
 *
 * @d:mBuf の現在位置までのサイズでバッファを確保し、
 * データをコピーして返す。
 * @r:現在のサイズが 0 なら NULL */

void *mBufCopyToBuf(mBuf *p)
{
	if(p->cursize == 0) return NULL;

	return mMemdup(p->buf, p->cursize);
}
