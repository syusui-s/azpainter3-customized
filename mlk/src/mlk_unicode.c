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

/*********************************
 * Unicode 関連
 *********************************/

#include <string.h>

#include "mlk.h"
#include "mlk_unicode.h"



//=================================
// UTF-8
//=================================


/**@ UTF-8 1文字のバイト数取得
 *
 * @g:UTF-8
 *
 * @d:先頭のバイトから、UTF-8 1文字のバイト数を判定して返す。
 * @r:バイト数 (1〜4)。\
 * -1 の場合は、不正な文字。 */

int mUTF8GetCharBytes(const char *p)
{
	uint8_t c = *((uint8_t *)p);

	if(c < 0xc0) return 1;
	else if(c < 0xe0) return 2;
	else if(c < 0xf0) return 3;
	else if(c < 0xf8) return 4;
	else return -1;
}

/**@ UTF-8 から Unicode 1文字を取得
 *
 * @p:dst  変換された文字が格納される (U+0000 〜 U+10FFFF)
 * @p:maxlen UTF-8 文字列の最大長さ。負の値で制限なし。
 * @p:ppnext NULL 以外で、次の位置が格納される。\
 * エラーの場合、位置は進まない。
 * 
 * @r:0 で成功。\
 * -1 で、長さが足りない、または不正な文字。\
 * 1 で、正常に読み込めたが、文字として無効なもの。\
 * (文字と次の位置はセットされている) */

int mUTF8GetChar(mlkuchar *dst,const char *src,int maxlen,const char **ppnext)
{
	const uint8_t *ps = (const uint8_t *)src;
	mlkuchar c,min;
	int i,len;
	uint8_t b;
	
	c = *ps;

	if(c < 0x80)
	{
		len = 1;
		min = 0;
	}
	else if(c < 0xc0)
		return -1;
	else if(c < 0xe0)
	{
		len = 2;
		min = 0x80;
		c &= 0x1f;
	}
	else if(c < 0xf0)
	{
		len = 3;
		min = 1<<11;
		c &= 0x0f;
	}
	else if(c < 0xf8)
	{
		len = 4;
		min = 1<<16;
		c &= 0x07;
	}
	else
		return -1;

	//長さが足りない

	if(maxlen >= 0 && len > maxlen) return -1;

	//2byte目以降を処理

	if(len > 1)
	{
		for(i = 1; i < len; i++)
		{
			b = ps[i];

			if((b & 0xc0) != 0x80) return -1;

			c = (c << 6) | (b & 0x3f);
		}

		//最小バイトで表現されていない
		if(c < min) return -1;
	}

	//

	*dst = c;
	
	if(ppnext) *ppnext = src + len;

	//無効文字

	if(c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff) || c == 0xfeff)
		return 1;
	else
		return 0;
}

/**@ UTF-8 文字列を検証
 *
 * @d:不正な文字や無効な文字があればその位置で終了させる (ヌル文字をセット)
 * @p:len 文字列の長さ。負の値でヌル文字まで。
 * @r:検証後の文字列の長さ */

int mUTF8Validate(char *str,int len)
{
	const char *ps,*psend;
	mlkuchar c;
	int ret;

	if(len < 0)
		len = strlen(str);

	ps = str;
	psend = str + len;

	while(ps - str < len && *ps)
	{
		ret = mUTF8GetChar(&c, ps, psend - ps, &ps);

		if(ret != 0)
		{
			*((char *)ps) = 0;
			break;
		}
	}

	return ps - str;
}

/**@ UTF-8 文字列を、検証しながらコピー
 *
 * @d:不正な文字があれば、その位置まで。\
 * 無効な文字があれば、スキップしてコピーする。\
 * ヌル文字は含まない。
 * @p:len src の長さ。負の値でヌル文字まで。
 * @r:コピーした長さ */

int mUTF8CopyValidate(char *dst,const char *src,int len)
{
	const char *ps,*psend;
	mlkuchar c;
	int ret,dstlen = 0;

	if(len < 0)
		len = strlen(src);

	ps = src;
	psend = src + len;

	while(ps - src < len && *ps)
	{
		ret = mUTF8GetChar(&c, ps, psend - ps, &ps);

		if(ret == 0)
		{
			ret = mUnicharToUTF8(c, dst, -1);
			
			dst += ret;
			dstlen += ret;
		}
		else if(ret == 1)
			continue;
		else
			break;
	}

	return dstlen;
}

/**@ UTF-8 文字列を UTF-16 文字列に変換
 *
 * @d:バッファに余裕がある場合は、最後にヌル文字を追加する。
 * 
 * @p:srclen UTF-8 のバイト数。負の値でヌル文字まで。
 * @p:dst    NULL で、変換後の文字数のみ計算
 * @p:dstlen 変換先のバッファの文字数 (16bit 単位)。dst が NULL の場合は無視。
 * @r:変換した文字数 (ヌル文字は含まない)。\
 * -1 でエラー。 */

int mUTF8toUTF16(const char *src,int srclen,mlkuchar16 *dst,int dstlen)
{
	const char *ps,*psend;
	mlkuchar uc;
	mlkuchar16 u16[2];
	int reslen = 0,ret;

	if(srclen < 0)
		srclen = strlen(src);
	
	ps = src;
	psend = src + srclen;

	while(ps - src < srclen && *ps)
	{
		ret = mUTF8GetChar(&uc, ps, psend - ps, &ps);

		if(ret == 0)
		{
			//Unicode -> UTF-16

			ret = mUnicharToUTF16(uc, u16, 2);
			if(ret == -1) continue;
		
			if(dst)
			{
				//出力先が足りない
				if(reslen + ret > dstlen) break;

				dst[0] = u16[0];
				if(ret == 2) dst[1] = u16[1];

				dst += ret;
			}
			
			reslen += ret;
		}
		else if(ret == 1)
			continue;
		else
			return -1;
	}

	//ヌル文字追加
	
	if(dst && reslen < dstlen) *dst = 0;
	
	return reslen;
}

/**@ UTF-8 文字列を UTF-32 文字列に変換
 * 
 * @d:バッファに余裕がある場合は、最後にヌル文字を追加する。
 * 
 * @p:srclen UTF-8 のバイト数。負の値でヌル文字まで。
 * @p:dst    NULL で、変換後の文字数のみ計算
 * @p:dstlen 変換先のバッファの文字数。dst が NULL の場合は無視。
 * @r:変換した文字数 (ヌル文字は含まない)。\
 * -1 でエラー。 */

int mUTF8toUTF32(const char *src,int srclen,mlkuchar *dst,int dstlen)
{
	const char *ps,*psend;
	mlkuchar uc;
	int reslen = 0,ret;

	if(srclen < 0)
		srclen = strlen(src);
	
	ps = src;
	psend = src + srclen;

	while(ps - src < srclen && *ps)
	{
		ret = mUTF8GetChar(&uc, ps, psend - ps, &ps);

		if(ret == 0)
		{
			if(dst)
			{
				//出力先が一杯
				if(reslen >= dstlen) break;

				*(dst++) = uc;
			}
			
			reslen++;
		}
		else if(ret == 1)
			continue;
		else
			return -1;
	}

	//ヌル文字追加
	
	if(dst && reslen < dstlen) *dst = 0;
	
	return reslen;
}

/**@ UTF-8 文字列から UTF-32 文字列に変換 (確保)
 *
 * @d:必要な長さ分のバッファを確保して変換する。\
 * ヌル文字も含まれる。
 * @p:dstlen NULL 以外で、結果の文字数が格納される
 * @r:NULL でエラー */

mlkuchar *mUTF8toUTF32_alloc(const char *src,int srclen,int *dstlen)
{
	int len;
	mlkuchar *buf;
	
	len = mUTF8toUTF32(src, srclen, NULL, 0);
	if(len < 0) return NULL;
	
	buf = (mlkuchar *)mMalloc((len + 1) << 2);
	if(!buf) return NULL;
	
	mUTF8toUTF32(src, srclen, buf, len + 1);
	
	if(dstlen) *dstlen = len;
	
	return buf;
}


//=============================
// UTF-32
//=============================


/**@ Unicode 1文字を UTF-8 に変換
 *
 * @g:UTF-32
 * 
 * @p:dst NULL で必要なバイト数のみ計算
 * @p:maxlen 出力先の最大バイト数。\
 * 負の値で制限なし。\
 * dst が NULL の場合は無視。
 * @r:変換後のバイト数。\
 * -1 でバッファが足りない、または不正な文字。 */

int mUnicharToUTF8(mlkuchar c,char *dst,int maxlen)
{
	int len,top,i;

	if(c < 0x80)
	{
		len = 1;
		top = 0;
	}
	else if(c < 0x800)
	{
		len = 2;
		top = 0xc0;
	}
	else if(c < 0x10000)
	{
		len = 3;
		top = 0xe0;
	}
	else if(c <= 0x10ffff)
	{
		len = 4;
		top = 0xf0;
	}
	else
		return -1;

	//

	if(dst)
	{
		if(maxlen >= 0 && len > maxlen) return -1;
	
		for(i = len - 1; i > 0; i--)
		{
			dst[i] = (c & 0x3f) | 0x80;
			c >>= 6;
		}

		dst[0] = c | top;
	}

	return len;
}

/**@ Unicode 1文字を UTF-16 に変換
 *
 * @p:dst NULL で必要なバイト数のみ計算
 * @p:maxlen 出力先の最大文字数 (16bit 単位)。\
 * 負の値で制限なし。\
 * dst が NULL の場合は無視。
 * @r:変換後の文字数 (16bit 単位)。\
 * -1 でバッファが足りない、または不正な文字。 */

int mUnicharToUTF16(mlkuchar c,mlkuchar16 *dst,int maxlen)
{
	if(c < 0x10000)
	{
		//1文字

		if(maxlen < 1) return -1;

		*dst = c;
		return 1;
	}
	else if(c <= 0x10ffff)
	{
		//2文字

		if(maxlen < 2) return -1;

		c -= 0x10000;

		dst[0] = (c >> 10) + 0xd800;
		dst[1] = (c & 0x3ff) + 0xdc00;

		return 2;
	}
	else
		return -1;
}


/**@ UTF-32 文字列の文字数取得 */

int mUTF32GetLen(const mlkuchar *p)
{
	int i;

	if(!p) return 0;
	
	for(i = 0; *p; p++, i++);
	
	return i;
}

/**@ UTF-32 文字列を複製
 *
 * @r:確保されたバッファ。NULL でエラー。 */

mlkuchar *mUTF32Dup(const mlkuchar *src)
{
	if(!src)
		return NULL;
	else
	{
		mlkuchar *buf;
		int size;

		size = (mUTF32GetLen(src) + 1) << 2;

		buf = (mlkuchar *)mMalloc(size);
		if(buf)
			memcpy(buf, src, size);

		return buf;
	}
}

/**@ UTF-32 文字列を UTF-8 文字列に変換
 *
 * @d:バッファに余裕がある場合、ヌル文字が追加される。\
 * バッファが足りない場合は、変換できる分までが処理される。
 *
 * @p:srclen Unicode 文字数。負の値でヌル文字まで。
 * @p:dst NULL で必要なバイト数のみ計算
 * @p:dstlen 変換先バッファのバイト数
 * @r:変換後のバイト数。-1 でエラー。 */

int mUTF32toUTF8(const mlkuchar *src,int srclen,char *dst,int dstlen)
{
	int len = 0,ret;

	if(srclen < 0)
		srclen = mUTF32GetLen(src);
	
	for(; srclen > 0 && *src; src++, srclen--)
	{
		ret = mUnicharToUTF8(*src, dst, dstlen);

		if(ret == -1) return -1;

		if(dst)
		{
			dst += ret;
			dstlen -= ret;
		}
		
		len += ret;
	}

	//ヌル文字追加
	if(dst && dstlen > 0) *dst = 0;
	
	return len;
}

/**@ Unicode 文字列から UTF-8 文字列に変換 (確保)
 *
 * @d:バッファを確保して変換する。\
 * 常にヌル文字が追加される。
 * @p:dstlen NULL 以外で、変換後の長さが格納される (ヌル文字は含まない)
 * @r:確保されたバッファ。NULL でエラー。 */

char *mUTF32toUTF8_alloc(const mlkuchar *src,int srclen,int *dstlen)
{
	char *buf;
	int len;

	len = mUTF32toUTF8(src, srclen, NULL, 0);
	if(len < 0) return NULL;
	
	buf = (char *)mMalloc(len + 1);
	if(!buf) return NULL;

	mUTF32toUTF8(src, srclen, buf, len + 1);
	
	if(dstlen) *dstlen = len;
	
	return buf;
}

/**@ UTF-32 文字列を数値に変換
 * 
 * @p:dig 小数点以下の桁数。\
 * 0 以下で整数。\
 * dig = 1 なら 1.0 = 10。 */

int mUTF32toInt_float(const mlkuchar *str,int dig)
{
	const mlkuchar *p = str;
	int fcnt = -1,n = 0;

	//符号
	
	if(*p == '-' || *p == '+') p++;

	//数字
	
	for(; *p; p++)
	{
		if(*p == '.')
		{
			//小数点
			
			if(dig <= 0 || fcnt != -1) break;
		
			fcnt = 0;
		}
		else if(*p >= '0' && *p <= '9')
		{
			n *= 10;
			n += *p - '0';
			
			if(fcnt != -1)
			{
				//小数点より後
				fcnt++;
				if(fcnt >= dig) break;
			}
		}
		else
			break;
	}
	
	//小数点以下の桁数が足りない場合
	
	if(dig > 0 && fcnt < dig)
	{
		if(fcnt == -1) fcnt = 0;
		
		for(; fcnt < dig; fcnt++, n *= 10);
	}

	//符号反転
	
	if(*str == '-') n = -n;
	
	return n;
}

/**@ UTF-32 文字列比較
 *
 * @d:ポインタが NULL の場合、空文字列とみなす。
 * @r:0 で等しい。-1 で str1 < str2。1 で str1 > str2。 */

int mUTF32Compare(const mlkuchar *str1,const uint32_t *str2)
{
	//ポインタが NULL の時

	if(!str1 && !str2)
		return 0;
	else if(!str1)
		return -1;
	else if(!str2)
		return 1;

	//比較

	for(; *str1 == *str2 && *str1 && *str2; str1++, str2++);

	if(*str1 == 0 && *str2 == 0)
		return 0;
	else
		return (*str1 < *str2)? -1: 1;
}


//=============================
// UTF-16
//=============================


/**@ UTF-16 文字列の長さ取得
 *
 * @g:UTF-16 */

int mUTF16GetLen(const mlkuchar16 *p)
{
	int len = 0;
	
	for(; *p; p++, len++);

	return len;
}

/**@ UTF-16 文字列から Unicode 1文字取得
 *
 * @p:dst 変換後の Unicode 文字が格納される
 * @p:ppnext NULL 以外で、次の文字の位置が格納される
 * @r:0 で成功。\
 * -1 でエラー。\
 * 1 で無効な文字 (次の位置は正しくセットされる) */

int mUTF16GetChar(mlkuchar *dst,const mlkuchar16 *src,const mlkuchar16 **ppnext)
{
	mlkuchar c,c2;

	c = *(src++);

	//サロゲート (U+D800 .. U+DFFF)

	if((c & 0xf800) == 0xd800)
	{
		//ローサロゲートは先頭に来ない
		if(c & 0x0400) return -1;

		c2 = *(src++);

		//2番目がローサロゲートでない
		if((c2 & 0xfc00) != 0xdc00) return -1;

		c = 0x10000 + ((c - 0xd800) << 10) + (c2 - 0xdc00);
	}

	*dst = c;

	if(ppnext) *ppnext = src;

	if(c == 0xfeff)
		return 1;
	else
		return 0;
}

/**@ UTF-16 文字列を UTF-8 文字列に変換
 *
 * @d:UTF-16 の文字数は 16bit を1単位とする。\
 * 変換先に余裕があればヌル文字を追加する。
 * 
 * @p:srclen UTF-16 文字列の文字数。-1 でヌル文字まで。\
 *  途中でヌル文字が来た時は、そこで終了する。
 * @p:dst  変換先。NULL で必要な文字数のみ計算。
 * @p:dstlen 変換先のバイト数。dst が NULL の場合は無視。
 * @r:変換した文字数 (ヌル文字は含まない)。\
 *  -1 でエラー。 */

int mUTF16toUTF8(const mlkuchar16 *src,int srclen,char *dst,int dstlen)
{
	const mlkuchar16 *next;
	mlkuchar uc;
	int len = 0,ret;

	if(srclen < 0)
		srclen = mUTF16GetLen(src);
	
	while(srclen > 0 && *src)
	{
		//UTF-16 => Unicode

		ret = mUTF16GetChar(&uc, src, &next);
		if(ret == -1) return -1;

		srclen -= next - src;
		src = next;

		if(ret == 1) continue;

		//Unicode => UTF-8

		ret = mUnicharToUTF8(uc, dst, dstlen);
		if(ret == -1) return -1;

		if(dst)
		{
			dst += ret;
			dstlen -= ret;
		}

		len += ret;
	}

	//ヌル文字追加
	if(dst && dstlen > 0) *dst = 0;
	
	return len;
}

/**@ UTF-16 文字列を UTF-8 文字列に変換 (確保)
 *
 * @d:終端には常にヌル文字がセットされる。
 * @p:dstlen NULL 以外で、変換後のバイト数が格納される */

char *mUTF16toUTF8_alloc(const mlkuchar16 *src,int srclen,int *dstlen)
{
	int len;
	char *buf;
	
	len = mUTF16toUTF8(src, srclen, NULL, 0);
	if(len == -1) return NULL;
	
	buf = (char *)mMalloc(len + 1);
	if(!buf) return NULL;
	
	mUTF16toUTF8(src, srclen, buf, len + 1);
	
	if(dstlen) *dstlen = len;
	
	return buf;
}
