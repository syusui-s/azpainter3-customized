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
 * ロケールの文字セット関連
 *****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <iconv.h>
#include <errno.h>

#include "mlk.h"
#include "mlk_charset.h"


//----------------

typedef struct
{
	const char *name;
	int is_utf8;
}mCharset;

static mCharset g_charset = {0, 1};

//----------------


/**@ ロケールを初期化
 *
 * @d:setlocale(LC_ALL, "") を実行し、文字セット情報を取得する。\
 * ASCII 文字以外の文字列を扱う場合、アプリ開始時に実行すること。\
 * \
 * 文字セット情報は静的変数に保存される。\
 * 一度設定したら変更することはほぼないと思われるので、
 * 読み込み専用データとして扱うため、スレッドの対策はしていない。 */

void mInitLocale(void)
{
	char *m;

	setlocale(LC_ALL, "");

	//文字セット

	m = nl_langinfo(CODESET);

	g_charset.is_utf8 = (!m || m[0] == 0 || strcmp(m, "UTF-8") == 0);

	if(g_charset.is_utf8)
		g_charset.name = "UTF-8";
	else
		g_charset.name = m;
}

/**@ ロケールの文字セット名を取得
 *
 * @d:デフォルトで "UTF-8" となる */

const char *mGetLocaleCharset(void)
{
	return g_charset.name;
}

/**@ ロケールの文字セットが UTF-8 かどうか
 *
 * @d:mInitLocale() が実行されていない場合、UTF-8 とみなす。 */

mlkbool mLocaleCharsetIsUTF8(void)
{
	return g_charset.is_utf8;
}


//==============================
// iconv
//==============================


/**@ iconv 開く
 *
 * @g:iconv
 *
 * @p:p 値が格納される
 * @p:from 変換元の文字コード
 * @p:to 変換先の文字コード */

mlkbool mIconvOpen(mIconv *p,const char *from,const char *to)
{
	*p = iconv_open(to, from);

	return (*p != (iconv_t)-1);
}

/**@ iconv 閉じる */

void mIconvClose(mIconv p)
{
	if(p != (iconv_t)-1)
		iconv_close((iconv_t)p);
}

/**@ 文字コード変換
 *
 * @d:ソースバッファから一度に変換する。
 *
 * @p:src 変換元文字列
 * @p:srclen 変換元文字列のバイト数。\
 *  負の値で、char * として扱った時の、ヌル文字まで。
 * @p:dstlen NULL 以外で、変換後のバイト数が格納される (ヌル文字は含まない)
 * @p:nullsize 終端にセットするヌル文字 (0) のバイト数。\
 *  0 以下でセットしない。
 * @r:変換後の、確保された文字列。NULL で失敗。 */

char *mIconvConvert(mIconv p,const void *src,int srclen,int *dstlen,int nullsize)
{
	char *dstbuf,*pd,*ps;
	size_t ssize,dsize,ret,bufsize,pos;
	int i,freset = FALSE,ferr = FALSE;

	if(p == (mIconv)-1) return NULL;

	if(srclen < 0)
		srclen = strlen((const char *)src);
	
	if(nullsize < 0) nullsize = 0;

	bufsize = srclen + nullsize;
	if(bufsize < 256) bufsize = 256;

	dstbuf = (char *)mMalloc(bufsize);
	if(!dstbuf) return NULL;

	ps = (char *)src;
	pd = dstbuf;
	ssize = srclen;
	dsize = bufsize - nullsize;

	//

	while(1)
	{
		ret = iconv((iconv_t)p, (freset)? NULL: &ps, &ssize, &pd, &dsize);

		if(ret == (size_t)-1)
		{
			//エラー
			
			if(errno == E2BIG)
			{
				//出力バッファが足りない

				pos = pd - dstbuf;
				bufsize *= 2;

				pd = (char *)mRealloc(dstbuf, bufsize);
				if(!pd)
				{
					ferr = TRUE;
					break;
				}

				dstbuf = pd;
				pd += pos;
				dsize = bufsize - nullsize - pos;
			}
			else
			{
				//入力に不完全なもの・無効な文字があった場合など
				ferr = TRUE;
				break;
			}
		}
		else
		{
			//完了した
			
			if(freset)
				break;
			else
			{
				//最後に状態をリセット
				freset = TRUE;
				ssize = 0;
			}
		}
	}

	//エラー時

	if(ferr)
	{
		mFree(dstbuf);
		return NULL;
	}

	//---- 成功

	//結果バイト数

	pos = pd - dstbuf;

	if(dstlen) *dstlen = pos;

	//0 を追加

	for(i = nullsize; i; i--)
		*(pd++) = 0;

	//領域を縮小

	return mRealloc(dstbuf, pos + nullsize);
}

/**@ 文字コードを変換して関数で出力
 *
 * @d:ソースバッファから変換し、関数を使って出力する。
 *
 * @p:src 変換元文字列
 * @p:srclen 変換元文字列のバイト数。\
 *  負の値で、char * として扱った時の、ヌル文字まで。
 * @p:func 出力用関数。戻り値が 0 以外で強制終了する。
 * @p:param 関数に渡されるパラメータ */

void mIconvConvert_outfunc(mIconv p,const void *src,int srclen,
	int (*func)(void *buf,int size,void *param),void *param)
{
	char *ps,*pd,*dstbuf;
	size_t ssize,dsize,ret;
	int freset = FALSE;

	if(p == (mIconv)-1) return;

	if(srclen < 0)
		srclen = strlen((const char *)src);
	
	dstbuf = (char *)mMalloc(1024);
	if(!dstbuf) return;

	//

	ps = (char *)src;
	pd = dstbuf;
	ssize = srclen;
	dsize = 1024;

	while(1)
	{
		ret = iconv((iconv_t)p, (freset)? NULL: &ps, &ssize, &pd, &dsize);

		if(ret == (size_t)-1)
		{
			//エラー
			
			if(errno == E2BIG)
			{
				//出力バッファが足りない

				if((func)(dstbuf, pd - dstbuf, param))
					break;

				pd = dstbuf;
				dsize = 1024;
			}
			else
			{
				//入力に不完全なもの・無効な文字があった場合など
				break;
			}
		}
		else
		{
			//完了した
			
			if(freset)
				break;
			else
			{
				//最後に状態をリセット
				freset = TRUE;
				ssize = 0;
			}
		}
	}

	//残り

	if(pd != dstbuf)
		(func)(dstbuf, pd - dstbuf, param);

	mFree(dstbuf);
}

/**@ コールバック関数を使って文字コード変換
 *
 * @d:入出力用のバッファを確保し、in 関数で入力文字列をセット、out 関数で出力を処理する。\
 * \
 * コールバック関数は、負の値を返すとエラー終了する。\
 * in 関数では、セットした文字列の長さを返す。0 でデータの終了となる。\
 * out 関数では、すべてのデータを一度で処理すること。0 以上の値を返すと成功とする。\
 *
 * @p:inbufsize 入力バッファサイズ
 * @p:outbufsize 出力バッファサイズ
 * @p:param コールバック関数に渡されるユーザー定義値
 * @r:エラーがあった場合、FALSE */

mlkbool mIconvConvert_callback(mIconv p,int inbufsize,int outbufsize,mIconvCallback *cb,void *param)
{
	char *inbuf,*outbuf,*pin,*pout;
	size_t insize,outsize,reterr;
	int ret, flags = 0;
	mlkbool retval = FALSE;

	if(p == (mIconv)-1) return FALSE;

	//バッファ確保

	inbuf = (char *)mMalloc(inbufsize);
	outbuf = (char *)mMalloc(outbufsize);

	if(!inbuf || !outbuf) goto END;

	//

	pout = outbuf;
	insize = 0;
	outsize = outbufsize;

	/* flags: 1=リセット, 2=入力に不完全な文字列があった */

	while(1)
	{
		//入力文字列取得
		
		if(insize == 0 || (flags & 2))
		{
			ret = (cb->in)(inbuf + insize, inbufsize - insize, param);
			if(ret < 0) break;

			if(ret == 0)
				flags |= 1;

			insize += ret;
			pin = inbuf;
		}

		//変換

		reterr = iconv((iconv_t)p, (flags & 1)? NULL: &pin, &insize, &pout, &outsize);

		//前回 EINVAL で、再変換しても EINVAL の場合は、エラーとする

		if(flags & 2)
		{
			if(reterr == (size_t)-1 && errno == EINVAL)
				break;

			flags &= ~2;
		}

		//

		if(reterr == (size_t)-1)
		{
			//エラー

			if(errno == EINVAL)
			{
				//入力に不完全な文字列があった
				//(入力データが足りていない場合があるため、追加取得する)

				flags |= 2;

				memmove(inbuf, pin, insize);
			}
			else if(errno == E2BIG)
			{
				//出力バッファが足りない

				ret = (cb->out)(outbuf, pout - outbuf, param);
				if(ret < 0) break;

				pout = outbuf;
				outsize = outbufsize;
			}
			else
				break;
		}
		else
		{
			//入力をすべて処理した

			if(flags & 1)
			{
				retval = TRUE;
				break;
			}

			insize = 0;
		}
	}

	//残った出力を処理

	if(retval && pout != outbuf)
	{
		ret = (cb->out)(outbuf, pout - outbuf, param);
		if(ret < 0) retval = FALSE;
	}

	//
	
END:
	mFree(inbuf);
	mFree(outbuf);

	return retval;
}


//==============================
// 変換
//==============================


/* ロケール文字列をワイド文字列に変換
 *
 * dst: NULL で変換後の文字数のみ取得
 * return: 変換された文字数。-1 でエラー */

static int _locale_to_wide(wchar_t *dst,const char *src,int srclen)
{
	wchar_t wc;
	int len,dlen = 0;

	mbtowc(NULL, NULL, 0);

	while(srclen > 0)
	{
		len = mbtowc(&wc, src, srclen);
		if(len == -1) return -1;

		if(dst) *(dst++) = wc;
		dlen++;

		src += len;
		srclen -= len;
	}

	return dlen;
}

/* ワイド文字列をロケール文字列に変換 */

static int _wide_to_locale(char *dst,const wchar_t *src,int srclen)
{
	char m[MB_CUR_MAX];
	int len,dlen = 0;

	wctomb(NULL, 0);

	while(srclen > 0)
	{
		len = wctomb(m, *(src++));
		if(len == -1) return -1;

		if(dst)
		{
			memcpy(dst, m, len);
			dst += len;
		}
		
		dlen += len;

		srclen--;
	}

	return dlen;
}


/**@ 文字列の文字コードを変換
 *
 * @g:変換
 * 
 * @p:src 変換元文字列
 * @p:srclen 変換元文字列のバイト数 (負の値で、char * におけるヌル文字まで)
 * @p:from 変換元の文字コード
 * @p:to 変換先の文字コード
 * @p:dstlen NULL 以外で、変換後の長さ
 * @p:nullsize 終端に追加する 0 のバイト数
 * @r:変換後の、確保された文字列。NULL で失敗。 */

void *mConvertCharset(const void *src,int srclen,
	const char *from,const char *to,int *dstlen,int nullsize)
{
	mIconv ic;
	char *dst;

	if(!mIconvOpen(&ic, from, to))
		return NULL;
	else
	{
		dst = mIconvConvert(ic, src, srclen, dstlen, nullsize);

		mIconvClose(ic);

		return dst;
	}
}

/**@ UTF-8 文字列をロケール文字列に変換
 *
 * @d:ロケールが UTF-8 の場合、そのままコピーされる。
 * @p:len 文字列の長さ。負の値でヌル文字まで。
 * @p:dstlen NULL 以外で、変換された文字列の長さが格納される
 * @r:確保された文字列。NULL でエラー。 */

char *mUTF8toLocale(const char *str,int len,int *dstlen)
{
	if(g_charset.is_utf8)
	{
		if(len < 0) len = strlen(str);

		if(dstlen) *dstlen = len;
		
		return mStrndup(str, len);
	}
	else
		return (char *)mConvertCharset(str, len, "UTF-8", g_charset.name, dstlen, 1);
}

/**@ ロケール文字列を UTF-8 文字列に変換
 *
 * @d:ロケールが UTF-8 の場合、そのままコピーされる。
 * @p:len 文字列の長さ。負の値でヌル文字まで。
 * @p:dstlen NULL 以外で、変換された文字列の長さが格納される
 * @r:確保された文字列。NULL でエラー。 */

char *mLocaletoUTF8(const char *str,int len,int *dstlen)
{
	if(g_charset.is_utf8)
	{
		if(len < 0) len = strlen(str);

		if(dstlen) *dstlen = len;

		return mStrndup(str, len);
	}
	else
		return (char *)mConvertCharset(str, len, g_charset.name, "UTF-8", dstlen, 1);
}

/**@ ロケール文字列を UTF-32 文字列に変換
 *
 * @d:エンディアンは、ホストに依存。
 *
 * @p:len src の長さ。負の値で自動。
 * @p:dstlen NULL 以外で、変換後の文字数が入る */

mlkuchar *mLocaletoUTF32(const char *src,int len,int *dstlen)
{
	mlkuchar *buf;
	int size;

	buf = (mlkuchar *)mConvertCharset(src, len, g_charset.name,
	#ifdef MLK_BIG_ENDIAN
		"UTF-32BE",
	#else
		"UTF-32LE",
	#endif
		&size, 4);

	if(buf && dstlen)
		*dstlen = size >> 2;

	return buf;
}

/**@ ワイド文字列を UTF-8 文字列に変換
 *
 * @p:len ソースの文字数。常に指定する
 * @p:dstlen NULL 以外で、変換後のバイト数が格納される */

char *mWidetoUTF8(const void *src,int len,int *dstlen)
{
	char *buf,*ubuf;
	int llen;

	//ロケール文字列のバイト数

	llen = _wide_to_locale(NULL, (const wchar_t *)src, len);
	if(llen == -1) return NULL;

	//wchar_t -> locale

	buf = (char *)mMalloc(llen + 1);
	if(!buf) return NULL;

	_wide_to_locale(buf, (const wchar_t *)src, len);

	//locale -> UTF-8

	if(g_charset.is_utf8)
	{
		buf[llen] = 0;
		*dstlen = llen;
		
		return buf;
	}
	else
	{
		ubuf = mLocaletoUTF8(buf, llen, dstlen);

		mFree(buf);

		return ubuf;
	}
}

/**@ ワイド文字列を UTF-32 に変換
 *
 * @p:len src の文字数。常に指定する。 */

mlkuchar *mWidetoUTF32(const void *src,int len,int *dstlen)
{
	void *ubuf;
	char *buf;
	size_t llen;

	//ロケール文字列のバイト数

	llen = _wide_to_locale(NULL, (const wchar_t *)src, len);
	if(llen == -1) return NULL;

	//wchar_t -> locale

	buf = (char *)mMalloc(llen + 1);
	if(!buf) return NULL;

	_wide_to_locale(buf, (const wchar_t *)src, len);
	buf[llen] = 0;

	//locale -> UTF-32

	ubuf = mLocaletoUTF32(buf, llen, dstlen);

	mFree(buf);

	return (mlkuchar *)ubuf;
}

/**@ ロケール文字列をワイド文字列に変換
 *
 * @p:len src の長さ。負の値で自動。
 * @p:dstlen NULL 以外で、結果の文字数が入る。
 * @r:確保されたバッファ。wchar_t * に変換する。 */

void *mLocaletoWide(const char *src,int len,int *dstlen)
{
	wchar_t *buf;
	size_t wlen;

	if(len < 0) len = strlen(src);

	//文字数

	wlen = _locale_to_wide(NULL, src, len);
	if(wlen == -1) return NULL;

	//確保

	buf = (wchar_t *)mMalloc(sizeof(wchar_t) * (wlen + 1));
	if(!buf) return NULL;

	//変換

	_locale_to_wide(buf, src, len);
	buf[wlen] = 0;

	if(dstlen) *dstlen = wlen;

	return buf;
}


//==============================
// 出力
//==============================


/* FILE * に出力 */

static int _put_utf8(void *buf,int size,void *param)
{
	fwrite(buf, 1, size, (FILE *)param);

	return 0;
}

/**@ UTF-8 文字列をロケール文字列として出力
 *
 * @g:UTF-8 文字列出力
 *
 * @p:fp FILE*
 * @p:str NULL で何もしない
 * @p:len 負の値でヌル文字まで */

void mPutUTF8(void *fp,const char *str,int len)
{
	mIconv ic;

	if(!str) return;

	if(len < 0)
		len = strlen(str);

	if(g_charset.is_utf8)
		fwrite(str, 1, len, fp);
	else
	{
		if(mIconvOpen(&ic, "UTF-8", g_charset.name))
		{
			mIconvConvert_outfunc(ic, str, len, _put_utf8, fp);

			mIconvClose(ic);
		}
	}
}

/**@ UTF-8 文字列を標準出力に出力 */

void mPutUTF8_stdout(const char *str)
{
	mPutUTF8(stdout, str, -1);
}

/**@ UTF-8 文字列を標準エラー出力に出力 */

void mPutUTF8_stderr(const char *str)
{
	mPutUTF8(stderr, str, -1);
}

/**@ UTF-8 文字列を標準出力に出力
 *
 * @p:format 最初の '%s' の部分に str が挿入される。\
 *  複数の %s や、それ以外の %* には対応していない。 */

void mPutUTF8_format_stdout(const char *format,const char *str)
{
	char *pc;

	pc = strstr(format, "%s");
	if(!pc)
		fputs(format, stdout);
	else
	{
		fwrite(format, 1, pc - format, stdout);
		mPutUTF8_stdout(str);
		fputs(pc + 2, stdout);
	}
}
