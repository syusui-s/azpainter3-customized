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
 * ユーティリティ関数
 *****************************************/

#include <string.h>
#include <stdarg.h>

#include "mlk.h"
#include "mlk_util.h"


/**@ ホストのバイト順がリトルエンディアンか */

mlkbool mIsByteOrderLE(void)
{
	uint16_t v = 0x1234;

	return (*((uint8_t *)&v) == 0x34);
}

/**@ R-G-B-A 順で並んでいる時の uint32 値取得
 *
 * @d:バッファ上に、c (ARGB) の色が R-G-B-A 順で並んでいて、
 *  それを uint32 値で取得した時の値を取得する。 */

uint32_t mRGBAtoHostOrder(uint32_t c)
{
	uint8_t b[4];

	b[0] = MLK_RGB_R(c);
	b[1] = MLK_RGB_G(c);
	b[2] = MLK_RGB_B(c);
	b[3] = MLK_ARGB_A(c);

	return *((uint32_t *)b);
}

/**@ DPM を DPI に変換
 *
 * @d:DPM (dots per meter) を DPI (dots per inch) に変換する。 */

int mDPMtoDPI(int dpm)
{
	return (int)(dpm * 0.0254 + 0.5);
}

/**@ DPI を DPM に変換
 *
 * @d:DPI (dots per inch) を DPM (dots per meter) に変換する。 */

int mDPItoDPM(int dpi)
{
	return (int)(dpi / 0.0254 + 0.5);
}

/**@ 最初にビットが ON になる位置を取得 (下位から順に)
 *
 * @r:-1 ですべて 0 */

int mGetOnBitPosL(uint32_t val)
{
	int i,pos = 0;

	//8bit 単位でチェック

	for(i = 4; i; i--, pos += 8, val >>= 8)
	{
		if(val & 0xff)
		{
			for(; 1; val >>= 1, pos++)
			{
				if(val & 1) return pos;
			}
		}
	}

	return -1;
}

/**@ 最初にビットが OFF になる位置を取得 (下位から順に)
 *
 * @r:-1 ですべて 1 */

int mGetOffBitPosL(uint32_t val)
{
	return mGetOnBitPosL(~val);
}

/**@ ON になっているビット数を取得 */

int mGetOnBitCount(uint32_t val)
{
	val = (val & 0x55555555) + ((val >> 1) & 0x55555555);
	val = (val & 0x33333333) + ((val >> 2) & 0x33333333);
	val = (val & 0x0f0f0f0f) + ((val >> 4) & 0x0f0f0f0f);
	val = (val & 0x00ff00ff) + ((val >> 8) & 0x00ff00ff);

	return (val & 0x0000ffff) + ((val >> 16) & 0x0000ffff);
}

/**@ フラグの状態を変更する時、状態が変化するかどうか
 *
 * @d:現在の状態から各操作を行った時、状態が変化するかどうかを判定する。
 *
 * @p:type 操作タイプ (0:OFF, 正:ON, 負:反転)
 * @p:curret 現在の状態 (0 で OFF、それ以外は ON)
 * @r:TRUE で状態が変化する */

mlkbool mIsChangeFlagState(int type,int current)
{
	if(type < 0) type = !current;

	return ((!type) != (!current));
}

/**@ フラグの状態を操作して結果を取得
 *
 * @p:type 操作タイプ (0:OFF, 正:ON, 負:反転)
 * @p:current 現在の状態 (0 で OFF、それ以外は ON)
 * @p:ret 結果の値が入る (0 で OFF、1 で ON)
 * @r:TRUE で状態が変化する */

mlkbool mGetChangeFlagState(int type,int current,int *ret)
{
	if(type < 0) type = !current;

	*ret = (type != 0);

	return ((!type) != (!current));
}

/**@ 文字列用ハッシュ値計算 */

uint32_t mCalcStringHash(const char *str)
{
	const uint8_t *ps = (const uint8_t *)str;
	uint32_t h = 0;
	uint8_t c;

	while((c = *(ps++)))
		h = h * 37 + c;

	return h;
}

/**@ n 以下で最大の2の累乗の指数 (ビット数) を取得 */

int mCalcMaxLog2(uint32_t n)
{
	uint32_t v = 1;
	int cnt = 0;

	if(n == 0) return 0;

	for(; n >= (v << 1); v <<= 1, cnt++);

	return cnt;
}


//======================
// バッファ値
//======================


/**@ バッファから uint16 取得 (BE) */

uint16_t mGetBufBE16(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return (ps[0] << 8) | ps[1];
}

/**@ バッファから uint32 取得 (BE) */

uint32_t mGetBufBE32(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return ((uint32_t)ps[0] << 24) | (ps[1] << 16) | (ps[2] << 8) | ps[3];
}

/**@ バッファから uint16 取得 (LE) */

uint16_t mGetBufLE16(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return (ps[1] << 8) | ps[0];
}

/**@ バッファから uint32 取得 (LE) */

uint32_t mGetBufLE32(const void *buf)
{
	const uint8_t *ps = (const uint8_t *)buf;

	return ((uint32_t)ps[3] << 24) | (ps[2] << 16) | (ps[1] << 8) | ps[0];
}

/**@ バッファに uint16 をセット (BE) */

void mSetBufBE16(uint8_t *buf,uint16_t val)
{
	buf[0] = (uint8_t)(val >> 8);
	buf[1] = (uint8_t)val;
}

/**@ バッファに uint32 をセット (BE) */

void mSetBufBE32(uint8_t *buf,uint32_t val)
{
	buf[0] = (uint8_t)(val >> 24);
	buf[1] = (uint8_t)(val >> 16);
	buf[2] = (uint8_t)(val >> 8);
	buf[3] = (uint8_t)val;
}

/**@ バッファに uint16 をセット (LE) */

void mSetBufLE16(uint8_t *buf,uint16_t val)
{
	buf[0] = (uint8_t)val;
	buf[1] = (uint8_t)(val >> 8);
}

/**@ バッファに uint32 をセット (LE) */

void mSetBufLE32(uint8_t *buf,uint32_t val)
{
	buf[0] = (uint8_t)val;
	buf[1] = (uint8_t)(val >> 8);
	buf[2] = (uint8_t)(val >> 16);
	buf[3] = (uint8_t)(val >> 24);
}


/**@ バッファにデータをセット
 *
 * @p:format
 * @tbl>
 * |||>||ビッグエンディアン指定||
 * |||<||リトルエンディアン指定 (デフォルト)||
 * |||数字||フォーマットの前に置くと、指定数の配列。\
 * ポインタを指定する。\
 * 数字がない、または 0 の場合は、直接値を指定。||
 * |||b||8bit||
 * |||h||16bit||
 * |||i||32bit||
 * |||s||文字列。\
 * 前に数字が指定されていた場合は、文字数。||
 * |||z||ゼロ値。前に数字でバイト数を指定。||
 * @tbl<
 * \
 * 上記以外の文字は、スキップする。
 *
 * @r:セットしたバイト数 */

int mSetBuf_format(void *buf,const char *format,...)
{
	va_list ap;
	uint8_t *pd = (uint8_t *)buf;
	int n,num,is_be = 0;
	char c;
	const uint8_t *ps8;
	const uint16_t *ps16;
	const uint32_t *ps32;

	va_start(ap, format);

	while(1)
	{
		c = *(format++);
		if(!c) break;
	
		//数字 (配列数)

		num = 0;

		if(c >= '0' && c <= '9')
		{
			do
			{
				num = num * 10 + (c - '0');

				c = *(format++);
				if(!c) goto END;
			} while(c >= '0' && c <= '9');
		}

		//

		switch(c)
		{
			//i (32bit)
			case 'i':
				if(num == 0)
				{
					n = va_arg(ap, int32_t);
					
					if(is_be)
						mSetBufBE32(pd, n);
					else
						mSetBufLE32(pd, n);

					pd += 4;
				}
				else
				{
					ps32 = va_arg(ap, const uint32_t *);

					if(is_be)
					{
						for(; num; num--, pd += 4)
							mSetBufBE32(pd, *(ps32++));
					}
					else
					{
						for(; num; num--, pd += 4)
							mSetBufLE32(pd, *(ps32++));
					}
				}
				break;

			//h (16bit)
			case 'h':
				if(num == 0)
				{
					n = (uint16_t)va_arg(ap, int);

					if(is_be)
						mSetBufBE16(pd, n);
					else
						mSetBufLE16(pd, n);
					
					pd += 2;
				}
				else
				{
					ps16 = va_arg(ap, const uint16_t *);

					if(is_be)
					{
						for(; num; num--, pd += 2)
							mSetBufBE16(pd, *(ps16++));
					}
					else
					{
						for(; num; num--, pd += 2)
							mSetBufLE16(pd, *(ps16++));
					}
				}
				break;
		
			//b (8bit)
			case 'b':
				if(num == 0)
					*(pd++) = (uint8_t)va_arg(ap, int);
				else
				{
					ps8 = va_arg(ap, const uint8_t *);

					memcpy(pd, ps8, num);
					pd += num;
				}
				break;

			//z (ゼロ)
			case 'z':
				memset(pd, 0, num);
				pd += num;
				break;
			
			//文字列
			case 's':
				ps8 = va_arg(ap, const uint8_t *);

				if(num == 0) num = -1;

				while(*ps8)
				{
					*(pd++) = *(ps8++);

					if(num != -1)
					{
						num--;
						if(num == 0) break;
					}
				}
				break;
		
			//BE
			case '>':
				is_be = 1;
				break;
			//LE
			case '<':
				is_be = 0;
				break;
		}
	}

END:
	va_end(ap);

	return pd - (uint8_t *)buf;
}

/**@ バッファから各データ取得
 *
 * @p:format
 * @tbl>
 * |||>||ビッグエンディアン指定||
 * |||<||リトルエンディアン指定 (デフォルト)||
 * |||数字||フォーマットの前に置くと、指定数の配列||
 * |||b||8bit||
 * |||h||16bit||
 * |||i||32bit||
 * |||s||文字列 (最後に NULL 文字が追加される)||
 * |||S||前に数字を指定して、指定バイト分をスキップ||
 * @tbl<
 * \
 * 上記以外の文字はスキップされる。
 *
 * @r:読み込んだバイト数 */

int mGetBuf_format(const void *buf,const char *format,...)
{
	va_list ap;
	char c;
	int is_be = 0,num;
	const uint8_t *src;
	uint8_t *pd8;
	uint16_t *pd16;
	uint32_t *pd32;

	va_start(ap, format);

	src = (const uint8_t *)buf;

	while(1)
	{
		c = *(format++);
		if(!c) break;

		//数字 (配列数)

		num = 0;

		if(c >= '0' && c <= '9')
		{
			do
			{
				num = num * 10 + (c - '0');

				c = *(format++);
				if(!c) goto END;
			} while(c >= '0' && c <= '9');
		}

		if(num <= 0) num = 1;

		//
		
		switch(c)
		{
			//i (32bit)
			case 'i':
				pd32 = va_arg(ap, uint32_t *);

				if(is_be)
				{
					for(; num; num--, src += 4)
						*(pd32++) = mGetBufBE32(src);
				}
				else
				{
					for(; num; num--, src += 4)
						*(pd32++) = mGetBufLE32(src);
				}
				break;

			//h (16bit)
			case 'h':
				pd16 = va_arg(ap, uint16_t *);

				if(is_be)
				{
					for(; num; num--, src += 2)
						*(pd16++) = mGetBufBE16(src);
				}
				else
				{
					for(; num; num--, src += 2)
						*(pd16++) = mGetBufLE16(src);
				}
				break;
		
			//b,s (8bit)
			case 'b':
			case 's':
				pd8 = va_arg(ap, uint8_t *);
				memcpy(pd8, src, num);
				src += num;

				if(c == 's')
					pd8[num] = 0;
				break;

			//skip
			case 'S':
				src += num;
				break;

			//BE
			case '>':
				is_be = 1;
				break;
			//LE
			case '<':
				is_be = 0;
				break;
		}
	}

END:
	va_end(ap);

	return src - (uint8_t *)buf;
}

/**@ バッファをコピー (16bit、BE とホストエンディアンの変換)
 *
 * @d:BE -> HOST または HOST -> BE 時。
 *
 * @p:src ビッグエンディアンのソースデータ
 * @p:cnt 16bit 単位の個数 */

void mCopyBuf_16bit_BEtoHOST(void *dst,const void *src,uint32_t cnt)
{
	memcpy(dst, src, cnt * 2);

#if !defined(MLK_BIG_ENDIAN)
	mSwapByte_16bit(dst, cnt);
#endif
}

/**@ バッファをコピー (32bit、BE とホストエンディアンの変換)
 * 
 * @d:BE -> HOST または HOST -> BE 時。
 *
 * @p:src ビッグエンディアンのソースデータ
 * @p:cnt 32bit 単位の個数 */

void mCopyBuf_32bit_BEtoHOST(void *dst,const void *src,uint32_t cnt)
{
#if defined(MLK_BIG_ENDIAN)
	memcpy(dst, src, cnt * 4);
#else
	uint8_t *ps,*pd;

	ps = (uint8_t *)src;
	pd = (uint8_t *)dst;
	
	for(; cnt; cnt--, ps += 4, pd += 4)
	{
		pd[0] = ps[3];
		pd[1] = ps[2];
		pd[2] = ps[1];
		pd[3] = ps[0];
	}
#endif
}

/**@ ビット値を反転 */

void mReverseBit(uint8_t *buf,uint32_t bytes)
{
	for(; bytes; bytes--, buf++)
		*buf = ~(*buf);
}

/**@ バッファの値を反転 (8bit) */

void mReverseVal_8bit(uint8_t *buf,uint32_t cnt)
{
	for(; cnt; cnt--, buf++)
		*buf = 255 - *buf;
}

/**@ バッファの値を反転 (16bit) */

void mReverseVal_16bit(void *buf,uint32_t cnt)
{
	uint16_t *p = (uint16_t *)buf;

	for(; cnt; cnt--, p++)
		*p = 0xffff - *p;
}

/**@ 16bit 値のバイト順を入れ替え
 *
 * @p:cnt 16bit 値の個数 */

void mSwapByte_16bit(void *buf,uint32_t cnt)
{
	uint8_t *pd = (uint8_t *)buf,a,b;

	for(; cnt; cnt--, pd += 2)
	{
		a = pd[0], b = pd[1];
		pd[0] = b, pd[1] = a;
	}
}

/**@ 32bit 値のバイト順を入れ替え (反転)
 *
 * @p:cnt 32bit 値の個数 */

void mSwapByte_32bit(void *buf,uint32_t cnt)
{
	uint8_t *pd = (uint8_t *)buf,a,b,c,d;

	for(; cnt; cnt--, pd += 4)
	{
		a = pd[0], b = pd[1], c = pd[2], d = pd[3];
		
		pd[0] = d, pd[1] = c, pd[2] = b, pd[3] = a;
	}
}


//================================
// 値の履歴
//================================


/**@ 履歴用の配列バッファに値を追加 (16bit)
 *
 * @d:新しい値は常に先頭に置かれる。\
 *  バッファ内に val と同じ値が存在する場合、それを先頭に移動する。\
 *  \
 *  存在しない場合は、全体を一つ下にずらして、先頭に追加する。\
 *  この時、[num - 1] の位置にある値は削除される。
 *
 * @p:num バッファの配列数
 * @p:addval 追加する値
 * @p:endval 終端として扱われる値 */

void mAddRecentVal_16bit(void *buf,int num,int addval,int endval)
{
	uint16_t *pd;
	int i,del;

	pd = (uint16_t *)buf;

	//削除する位置
	
	del = num - 1;

	//同じ値が存在するか

	for(i = 0; i < num; i++)
	{
		if(pd[i] == endval) break;
	
		if(pd[i] == addval)
		{
			del = i;
			break;
		}
	}

	//ずらす

	for(i = del; i > 0; i--)
		pd[i] = pd[i - 1];

	//先頭にセット

	pd[0] = addval;
}

/**@ 履歴用の配列バッファに値を追加 (32bit)
 *
 * @d:新しい値は常に先頭に置かれる。\
 *  バッファ内に val と同じ値が存在する場合、それを先頭に移動する。\
 *  \
 *  存在しない場合は、全体を一つ下にずらして、先頭に追加する。\
 *  この時、[num - 1] の位置にある値は削除される。
 *
 * @p:num バッファの配列数
 * @p:addval 追加する値
 * @p:endval 終端として扱われる値 */

void mAddRecentVal_32bit(void *buf,int num,uint32_t addval,uint32_t endval)
{
	uint32_t *pd;
	int i,del;

	pd = (uint32_t *)buf;

	//削除する位置
	
	del = num - 1;

	//同じ値が存在するか

	for(i = 0; i < num; i++)
	{
		if(pd[i] == endval) break;
	
		if(pd[i] == addval)
		{
			del = i;
			break;
		}
	}

	//ずらす

	for(i = del; i > 0; i--)
		pd[i] = pd[i - 1];

	//先頭にセット

	pd[0] = addval;
}


//================================
// Base64
//================================


/**@ Base64 エンコード後のサイズを取得
 *
 * @g:Base64 */

int mGetBase64EncodeSize(int size)
{
	return ((size + 2) / 3) * 4;
}

/**@ Base64 エンコード
 *
 * @d:dstsize に余裕があった場合、最後に 0 を追加する。
 *
 * @p:size 元データのサイズ
 * @r:エンコード後のサイズ (最後の 0 は含まない)。\
 * -1 でバッファが足りない。 */

int mEncodeBase64(void *dst,int dstsize,const void *src,int size)
{
	char *pd;
	const uint8_t *ps;
	int i,shift,val,n,encsize = 0;
	char m[4];

	pd = (char *)dst;
	ps = (const uint8_t *)src;

	for(; size > 0; size -= 3, ps += 3, pd += 4)
	{
		//src 3byte 取得

		if(size >= 3)
			val = (ps[0] << 16) | (ps[1] << 8) | ps[2];
		else if(size == 2)
			val = (ps[0] << 16) | (ps[1] << 8);
		else
			val = ps[0] << 16;

		//エンコード (3byte => 4文字)

		for(i = 0, shift = 18; i < 4; i++, shift -= 6)
		{
			n = (val >> shift) & 63;

			if(n < 26) n += 'A';
			else if(n < 52) n += 'a' - 26;
			else if(n < 62) n += '0' - 52;
			else if(n == 62) n = '+';
			else n = '/';

			m[i] = n;
		}

		//終端時、余りを '=' で埋める

		if(size == 1)
			m[2] = m[3] = '=';
		else if(size == 2)
			m[3] = '=';

		//セット

		if(encsize + 4 > dstsize) return -1;

		pd[0] = m[0];
		pd[1] = m[1];
		pd[2] = m[2];
		pd[3] = m[3];

		encsize += 4;
	}

	//余裕があれば 0 を追加

	if(encsize < dstsize)
		*pd = 0;

	return encsize;
}

/**@ Base64 デコード
 *
 * @p:len src の文字数。負の値で NULL 文字まで。
 * @r:デコード後のサイズ。-1 でエラー。 */

int mDecodeBase64(void *dst,int dstsize,const char *src,int len)
{
	uint8_t *pd;
	int decsize = 0,i,val,shift,n,size;

	if(len < 0) len = strlen(src);

	pd = (uint8_t *)dst;

	while(len)
	{
		//デコード (4文字 => 24bit)

		for(i = 0, val = 0, shift = 18; i < 4; i++, shift -= 6)
		{
			if(len == 0) return -1;
			
			n = *(src++);
			len--;

			if(n >= 'A' && n <= 'Z') n -= 'A';
			else if(n >= 'a' && n <= 'z') n = n - 'a' + 26;
			else if(n >= '0' && n <= '9') n = n - '0' + 52;
			else if(n == '+') n = 62;
			else if(n == '/') n = 63;
			else if(n == '=') n = 0;
			else return -1;

			val |= n << shift;
		}

		//データサイズ
		//最後が '??==' なら 1byte。'???=' なら 2byte

		size = 3;

		if(len == 0 && src[-1] == '=')
			size = (src[-2] == '=')? 1: 2;

		//出力先が足りない

		if(decsize + size > dstsize) return -1;

		//24bit => 1-3 byte

		*(pd++) = (uint8_t)(val >> 16);

		if(size >= 2)
			*(pd++) = (uint8_t)(val >> 8);

		if(size == 3)
			*(pd++) = (uint8_t)val;

		decsize += size;
	}

	return decsize;
}
  
