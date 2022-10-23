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
 * FILE * のユーティリティ
 *****************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "mlk.h"
#include "mlk_stdio.h"
#include "mlk_charset.h"
#include "mlk_util.h"


/**@ ファイルを開く
 *
 * @p:filename ファイル名 (UTF-8) */

FILE *mFILEopen(const char *filename,const char *mode)
{
	char *str;
	FILE *fp;

	str = mUTF8toLocale(filename, -1, NULL);
	if(!str) return NULL;

	fp = fopen(str, mode);

	mFree(str);

	return fp;
}

/**@ ファイルを閉じる
 *
 * @p:fp NULL の場合は何もしない */

void mFILEclose(FILE *fp)
{
	if(fp) fclose(fp);
}

/**@ ファイルを閉じて、NULL をセット
 *
 * @p:fp (*fp) が NULL の場合は何もしない */

void mFILEclose_null(FILE **fp)
{
	if(*fp)
	{
		fclose(*fp);
		*fp = NULL;
	}
}

/**@ ファイルサイズ取得 */

mlkfoff mFILEgetSize(FILE *fp)
{
	struct stat st;

	if(fstat(fileno(fp), &st) == -1)
		return 0;
	else if((st.st_mode & S_IFMT) == S_IFREG)
		return st.st_size;
	else
		return 0;
}


//===========================
// 読み込み
//===========================


/**@ 指定バイト数読み込み
 *
 * @g:読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadOK(FILE *fp,void *buf,int32_t size)
{
	return (fread(buf, 1, size, fp) != size);
}

/**@ 1バイト読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadByte(FILE *fp,void *buf)
{
	return (fread(buf, 1, 1, fp) != 1);
}

/**@ 16bit (LE) 読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadLE16(FILE *fp,void *buf)
{
	uint8_t v[2];

	if(fread(v, 1, 2, fp) < 2)
		return 1;
	else
	{
		*((uint16_t *)buf) = (v[1] << 8) | v[0];
		return 0;
	}
}

/**@ 32bit (LE) 読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadLE32(FILE *fp,void *buf)
{
	uint8_t v[4];

	if(fread(v, 1, 4, fp) < 4)
		return 1;
	else
	{
		*((uint32_t *)buf) = ((uint32_t)v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
		return 0;
	}
}

/**@ 16bit (BE) 読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadBE16(FILE *fp,void *buf)
{
	uint8_t v[2];

	if(fread(v, 1, 2, fp) < 2)
		return 1;
	else
	{
		*((uint16_t *)buf) = (v[0] << 8) | v[1];
		return 0;
	}
}

/**@ 32bit (BE) 読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadBE32(FILE *fp,void *buf)
{
	uint8_t v[4];

	if(fread(v, 1, 4, fp) < 4)
		return 1;
	else
	{
		*((uint32_t *)buf) = ((uint32_t)v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
		return 0;
	}
}

/**@ 16bit (LE) 読み込み
 *
 * @r:データが足りない場合 0 */

uint16_t mFILEgetLE16(FILE *fp)
{
	uint16_t v = 0;

	mFILEreadLE16(fp, &v);
	return v;
}

/**@ 32bit (LE) 読み込み
 *
 * @r:データが足りない場合 0 */

uint32_t mFILEgetLE32(FILE *fp)
{
	uint32_t v = 0;

	mFILEreadLE32(fp, &v);
	return v;
}

/**@ 文字列を読み込んで比較
 *
 * @d:cmp の文字数分を読み込んで、比較する。\
 * 大文字小文字は区別する。
 *
 * @p:cmp 比較文字列
 * @r:0 で成功し、等しい。それ以外で失敗 */

int mFILEreadStr_compare(FILE *fp,const char *cmp)
{
	int len,ret = 1;
	char *buf;

	len = strlen(cmp);
	if(len == 0) return 1;

	buf = (char *)mMalloc(len);
	if(!buf) return 1;

	if(fread(buf, 1, len, fp) == len)
		ret = (strncmp(buf, cmp, len) != 0);

	mFree(buf);

	return ret;
}

/**@ 文字列読み込み (前に文字数データ [可変サイズ] がある)
 *
 * @d:「文字数 (可変サイズ)」＋「文字列データ」の文字列を読み込む。\
 * \
 * 文字数データは 1byte 単位。\
 * 下位の 7 bitが順に格納されている。\
 * 最上位ビットが ON の場合、データが続く。
 * 
 * @p:ppdst 確保された文字列ポインタが入る。\
 * *ppdst == NULL で空文字列。
 * @r:文字列の長さ。\
 * -1 でエラー。0 で空文字列。 */

int mFILEreadStr_variable(FILE *fp,char **ppdst)
{
	int len,shift;
	uint8_t b;
	char *buf;

	*ppdst = NULL;

	//長さ

	for(len = shift = 0; 1; shift += 7)
	{
		if(mFILEreadByte(fp, &b)) return -1;

		len |= (b & 0x7f) << shift;

		if(b < 128) break;
	}

	//文字列

	if(len)
	{
		buf = (char *)mMalloc(len + 1);
		if(!buf) return -1;

		if(fread(buf, 1, len, fp) != len)
		{
			mFree(buf);
			return -1;
		}

		buf[len] = 0;

		*ppdst = buf;
	}

	return len;
}

/**@ 文字列読み込み (前に 16bit BE の文字長さデータ)
 *
 * @d:「16bit BE 文字数」＋「文字列」の文字列データを読み込む。
 *
 * @p:ppdst 確保された文字列ポインタが入る。\
 * *ppdst == NULL で空文字列。
 * @r:文字列の長さ。\
 * -1 でエラー、0 で空文字列。 */

int mFILEreadStr_lenBE16(FILE *fp,char **ppdst)
{
	uint16_t len;
	char *pc;

	*ppdst = NULL;

	//長さ

	if(mFILEreadBE16(fp, &len)) return -1;

	//文字列

	if(len)
	{
		pc = (char *)mMalloc(len + 1);
		if(!pc) return -1;

		if(fread(pc, 1, len, fp) != len)
		{
			mFree(pc);
			return -1;
		}

		pc[len] = 0;

		*ppdst = pc;
	}

	return len;
}

/**@ 16bit 配列読み込み (LE)
 *
 * @d:データが足りない場合は、途中まで読み込む。
 * @r:実際に読み込んだ数 */

int mFILEreadArrayLE16(FILE *fp,void *buf,int num)
{
	int size;

	size = fread(buf, 1, num << 1, fp);

	num = size >> 1;

#if defined(MLK_BIG_ENDIAN)
	mSwapByte_16bit(buf, num);
#endif

	return num;
}

/**@ 16bit 配列読み込み (BE)
 *
 * @d:データが足りない場合は、途中まで読み込む。
 * @r:実際に読み込んだ数 */

int mFILEreadArrayBE16(FILE *fp,void *buf,int num)
{
	int size;

	size = fread(buf, 1, num << 1, fp);

	num = size >> 1;

#if !defined(MLK_BIG_ENDIAN)
	mSwapByte_16bit(buf, num);
#endif

	return num;
}

/**@ 32bit 配列読み込み (BE)
 *
 * @r:実際に読み込んだ数 */

int mFILEreadArrayBE32(FILE *fp,void *buf,int num)
{
	int size;

	size = fread(buf, 1, num << 2, fp);

	num = size >> 2;

#if !defined(MLK_BIG_ENDIAN)
	mSwapByte_32bit(buf, num);
#endif

	return num;
}

/**@ 指定フォーマットで複数データ読み込み (LE)
 *
 * @d:終端まで来たら、その位置で終わる。
 *
 * @p:format
 * @tbl>
 * |||b||1byte||
 * |||h||2byte||
 * |||i||4byte||
 * @tbl<
 * \
 * 上記以外はスキップ。
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadFormatLE(FILE *fp,const char *format,...)
{
	va_list ap;
	char c;
	void *buf;
	int ret = 0;

	va_start(ap, format);

	for(; (c = *format); format++)
	{
		buf = va_arg(ap, void *);

		switch(c)
		{
			case 'i':
				ret = mFILEreadLE32(fp, buf);
				break;
			case 'h':
				ret = mFILEreadLE16(fp, buf);
				break;
			case 'b':
				ret = mFILEreadByte(fp, buf);
				break;
			default:
				continue;
		}
		
		if(ret) break;
	}

	va_end(ap);

	return ret;
}

/**@ 指定フォーマットで複数データ読み込み (BE)
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEreadFormatBE(FILE *fp,const char *format,...)
{
	va_list ap;
	char c;
	void *buf;
	int ret = 0;

	va_start(ap, format);

	for(; (c = *format); format++)
	{
		buf = va_arg(ap, void *);
		
		switch(c)
		{
			case 'i':
				ret = mFILEreadBE32(fp, buf);
				break;
			case 'h':
				ret = mFILEreadBE16(fp, buf);
				break;
			case 'b':
				ret = mFILEreadByte(fp, buf);
				break;
			default:
				continue;
		}

		if(ret) break;
	}

	va_end(ap);

	return ret;
}


//===========================
// 書き込み
//===========================


/**@ 指定サイズ書き込み
 *
 * @g:書き込み
 * 
 * @r:0 で成功、それ以外で失敗 */

int mFILEwriteOK(FILE *fp,const void *buf,int32_t size)
{
	return (fwrite(buf, 1, size, fp) != size);
}

/**@ 1バイト書き込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEwriteByte(FILE *fp,uint8_t val)
{
	return (fwrite(&val, 1, 1, fp) != 1);
}

/**@ 16bit (LE) 書き込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEwriteLE16(FILE *fp,uint16_t val)
{
	uint8_t b[2];

	b[0] = (uint8_t)val;
	b[1] = (uint8_t)(val >> 8);

	return (fwrite(b, 1, 2, fp) != 2);
}

/**@ 32bit (LE) 書き込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEwriteLE32(FILE *fp,uint32_t val)
{
	uint8_t b[4];

	b[0] = (uint8_t)val;
	b[1] = (uint8_t)(val >> 8);
	b[2] = (uint8_t)(val >> 16);
	b[3] = (uint8_t)(val >> 24);

	return (fwrite(b, 1, 4, fp) != 4);
}

/**@ 16bit (BE) 書き込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEwriteBE16(FILE *fp,uint16_t val)
{
	uint8_t b[2];

	b[0] = (uint8_t)(val >> 8);
	b[1] = (uint8_t)val;

	return (fwrite(b, 1, 2, fp) != 2);
}

/**@ 32bit (BE) 書き込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEwriteBE32(FILE *fp,uint32_t val)
{
	uint8_t b[4];

	b[0] = (uint8_t)(val >> 24);
	b[1] = (uint8_t)(val >> 16);
	b[2] = (uint8_t)(val >> 8);
	b[3] = (uint8_t)val;

	return (fwrite(b, 1, 4, fp) != 4);
}

/**@ ゼロを指定サイズ分書き込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mFILEwrite0(FILE *fp,int size)
{
	uint8_t buf[32];

	//32byte 単位で書き込み

	memset(buf, 0, 32);

	while(size >= 32)
	{
		if(fwrite(buf, 1, 32, fp) != 32)
			return 1;

		size -= 32;
	}

	//残り

	if(size)
	{
		if(fwrite(buf, 1, size, fp) != size)
			return 1;
	}

	return 0;
}

/**@ 文字列書き込み (可変長さ+文字列)
 *
 * @p:text NULL で空文字列として扱う
 * @p:len 文字長さ。負の値でヌル文字まで。
 * @r:書き込んだバイト数 */

int mFILEwriteStr_variable(FILE *fp,const char *text,int len)
{
	int n,cnt;

	if(!text)
		len = 0;
	else if(len < 0)
		len = strlen(text);

	if(len == 0)
	{
		//空文字列

		mFILEwriteByte(fp, 0);
		return 1;
	}
	else
	{
		for(n = len, cnt = 0; n; n >>= 7, cnt++)
			mFILEwriteByte(fp, (n < 128)? n: (n & 0x7f) | 0x80);

		fwrite(text, 1, len, fp);

		return cnt + len;
	}
}

/**@ 文字列書き込み (16bit BE 長さ + 文字列)
 *
 * @p:text NULL で空文字列として扱う
 * @p:len 文字長さ。負の値でヌル文字まで。\
 * 値が 16bit の範囲を超える場合は、-1 が返る。
 * @r:書き込んだバイト数 */

int mFILEwriteStr_lenBE16(FILE *fp,const char *text,int len)
{
	//長さ

	if(!text)
		len = 0;
	else if(len < 0)
		len = strlen(text);

	if(len > 0xffff) return -1;

	mFILEwriteBE16(fp, len);

	//文字列

	if(len) fwrite(text, 1, len, fp);

	return 2 + len;
}

/**@ 16bit 配列書き込み (BE) */

void mFILEwriteArrayBE16(FILE *fp,void *buf,int num)
{
	uint16_t *ps = (uint16_t *)buf;

	for(; num > 0; num--)
		mFILEwriteBE16(fp, *(ps++));
}

/**@ 32bit 配列書き込み (BE) */

void mFILEwriteArrayBE32(FILE *fp,void *buf,int num)
{
	uint32_t *ps = (uint32_t *)buf;

	for(; num > 0; num--)
		mFILEwriteBE32(fp, *(ps++));
}

