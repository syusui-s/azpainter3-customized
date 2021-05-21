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
 * 標準関数
 *****************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mlk.h"


/**@ デバッグ用メッセージ出力
 * 
 * @d:標準エラー出力に出力する。\
 * 改行は行わない。 */

void mDebug(const char *format,...)
{
	va_list arg;

	va_start(arg, format);
	vfprintf(stderr, format, arg);
	va_end(arg);

	fflush(stderr);
}

/**@ エラー用メッセージ出力
 *
 * @d:標準エラー出力に出力する。\
 * 先頭に "[Error] " が付く。\
 * 改行は行わない。 */

void mError(const char *format,...)
{
	va_list arg;

	fputs("[Error] ", stderr);

	va_start(arg, format);
	vfprintf(stderr, format, arg);
	va_end(arg);

	fflush(stderr);
}

/**@ 警告用メッセージ出力
 *
 * @d:標準エラー出力に出力する。\
 * 先頭に "[Warning] " が付く。\
 * 改行は行わない。 */

void mWarning(const char *format,...)
{
	va_list arg;

	fputs("[Warning] ", stderr);

	va_start(arg, format);
	vfprintf(stderr, format, arg);
	va_end(arg);

	fflush(stderr);
}


//=============================
// メモリ関連 : 非デバッグ用
//=============================


/**@ メモリ解放 (非デバッグ)
 * 
 * @p:ptr NULL の場合は何もしない */

void mFree_r(void *ptr)
{
	if(ptr)
		free(ptr);
}

/**@ メモリ確保 (非デバッグ) */

void *mMalloc_r(mlksize size)
{
	return malloc(size);
}

/**@ メモリ確保してゼロクリア (非デバッグ) */

void *mMalloc0_r(mlksize size)
{
	return calloc(1, size);
}

/**@ メモリサイズ変更 (非デバッグ)
 *
 * @p:ptr NULL の場合、malloc と同じ
 * @r:新しく確保されたバッファのポインタ。\
 * ポインタが変わらない場合もある。\
 * NULL でエラー。その場合、元のメモリは変化しない。 */

void *mRealloc_r(void *ptr,mlksize size)
{
	return realloc(ptr, size);
}

/**@ アラインメントされたメモリ確保 (非デバッグ)
 *
 * @d:ポインタは mFree() で解放できる。
 * @p:size alignment の倍数である必要はない。
 * @p:alignment このバイト単位の位置に配置される。\
 * 2 のべき乗で、かつポインタのサイズ [sizeof(void*)] の倍数であること。\
 * \
 * 負の値で、mMalloc() と同じ。\
 * 0〜ポインタのサイズ以下の場合は、自動で最小値に調整される。 */

void *mMallocAlign_r(mlksize size,int alignment)
{
	void *ptr;

	if(alignment < 0)
		ptr = malloc(size);
	else
	{
		if(alignment < sizeof(void *))
			alignment = sizeof(void *);

		posix_memalign(&ptr, alignment, size);
	}

	return ptr;
}

/**@ 文字列複製 (非デバッグ)
 *
 * @p:str NULL の場合、NULL が返る */

char *mStrdup_r(const char *str)
{
	if(str)
		return strdup(str);
	else
		return NULL;
}

/**@ 文字列を指定長さ分だけ複製 (非デバッグ)
 *
 * @d:終端にはヌル文字が追加される
 * 
 * @p:str NULL の場合、NULL が返る
 * @p:len 負の値で mStrdup と同じ */

char *mStrndup_r(const char *str,int len)
{
	if(!str)
		return NULL;
	else if(len < 0)
		return strdup(str);
	else
		return strndup(str, len);
}


//=============================
// メモリ関連
//=============================


/**@ バッファをゼロクリア
 *
 * @p:buf NULL で何もしない */

void mMemset0(void *buf,mlksize size)
{
	if(buf)
		memset(buf, 0, size);
}

/**@ メモリバッファ複製
 *
 * @d:新しくメモリを確保し、データをコピーする。
 * 
 * @p:src NULL の場合、NULL を返す
 * @r:複製されたバッファのポインタ。NULL で失敗。 */

void *mMemdup(const void *src,mlksize size)
{
	void *buf;

	if(!src) return NULL;

	buf = mMalloc(size);
	if(buf)
		memcpy(buf, src, size);

	return buf;
}


//===========================
// 文字列関連
//===========================


/**@ 文字列の長さ取得
 *
 * @p:str NULL の場合、0 が返る */

int mStrlen(const char *str)
{
	if(str)
		return strlen(str);
	else
		return 0;
}

/**@ 文字列複製 (長さ取得)
 *
 * @d:dst に複製した文字列のポインタを格納し、戻り値で複製した文字の長さを返す。
 *
 * @p:str NULL の場合、*dst には NULL が入り、戻り値は 0 となる。
 * @r:文字列の長さ */

int mStrdup_getlen(const char *str,char **dst)
{
	if(str)
	{
		*dst = mStrdup(str);
		return strlen(str);
	}
	else
	{
		*dst = NULL;
		return 0;
	}
}

/**@ 文字列複製 (解放)
 *
 * @d:*dst のポインタを解放した後、src の文字列を複製して *dst にセットする。
 *
 * @p:dst *dst が NULL なら解放はしない
 * @r:*dst と同じ値が返る */

char *mStrdup_free(char **dst,const char *src)
{
	mFree(*dst);

	return (*dst = mStrdup(src));
}

/**@ 文字列複製 (大文字変換)
 *
 * @d:アルファベットを大文字に変換して複製する */

char *mStrdup_upper(const char *str)
{
	char *buf,*p,c;

	buf = mStrdup(str);

	if(buf)
	{
		for(p = buf; (c = *p); p++)
		{
			if(c >= 'a' && c <= 'z')
				*p = c - 0x20;
		}
	}

	return buf;
}

/**@ 文字列を指定サイズに収めてコピー
 *
 * @d:コピーした文字列は、必ず NULL で終わる。
 *
 * @p:dst コピー先
 * @p:src コピー元
 * @p:size コピー先の最大バイト数 (NULL 文字含む)
 * @r:コピーした文字数 (NULL 文字は含まない) */

int mStrncpy(char *dst,const char *src,int size)
{
	int len = strlen(src);

	if(size <= 0)
		return 0;
	else if(len < size)
		//NULL 文字含めすべてコピー
		memcpy(dst, src, len + 1);
	else
	{
		len = size - 1;

		memcpy(dst, src, len);
		dst[len] = 0;
	}

	return len;
}


//========================
// 色関連
//========================


/**@ RGB のアルファ合成 (アルファ値最大 256)
 *
 * @d:dst の上に src をアルファ値 a で合成した色を取得
 *
 * @p:a アルファ値 (0-256) */

mRgbCol mBlendRGB_a256(mRgbCol src,mRgbCol dst,int a)
{
	int sr,sg,sb,dr,dg,db;

	sr = MLK_RGB_R(src);
	sg = MLK_RGB_G(src);
	sb = MLK_RGB_B(src);
	
	dr = MLK_RGB_R(dst);
	dg = MLK_RGB_G(dst);
	db = MLK_RGB_B(dst);

	sr = ((sr - dr) * a >> 8) + dr;
	sg = ((sg - dg) * a >> 8) + dg;
	sb = ((sb - db) * a >> 8) + db;

	return (sr << 16) | (sg << 8) | sb;
}
