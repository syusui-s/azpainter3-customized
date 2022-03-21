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

/**********************************************
 * 文字列関連ユーティリティ
 **********************************************/

#include <string.h>
#include <stdlib.h>

#include "mlk.h"
#include "mlk_string.h"

#if !defined(MLK_NO_UNICODE)
#include "mlk_unicode.h"
#endif



//==========================
// 数値 <=> 文字列
//==========================


/**@ 数値を文字列に変換
 *
 * @g:数値と文字列の変換
 *
 * @p:buf 格納先。最大で 12byte 必要。
 * @r:格納した文字長さ (ヌル文字は含まない) */

int mIntToStr(char *buf,int32_t n)
{
	char *pd = buf;
	int len = 0,minus;

	minus = (n < 0);

	//負の値

	if(minus)
	{
		n = -n;
		*(pd++) = '-';
		len = 1;
	}

	//一の位から順にセット

	do
	{
		*(pd++) = '0' + n % 10;
		len++;

		n /= 10;
	} while(n);

	*pd = 0;

	//数値部分を逆順にする

	if(minus)
		mStringReverse(buf + 1, len - 1);
	else
		mStringReverse(buf, len);

	return len;
}

/**@ 数値を文字列に変換 (桁数指定)
 *
 * @d:指定桁数より大きい値の場合、溢れる部分は含まない。
 * @p:dig 桁数。0 の場合は 1 と同じ。\
 * 正の値の場合、0 で埋める。\
 * 負の値の場合、空白で埋める。
 * @r:格納した文字長さ */

int mIntToStr_dig(char *buf,int32_t n,int dig)
{
	int len = 0,i,div,v;
	uint8_t fill_space,nostart = 1;

	//桁数

	if(dig >= 0)
		fill_space = 0;
	else
		fill_space = 1, dig = -dig;

	//符号

	if(n < 0)
	{
		*(buf++) = '-';
		len = 1;
		n = -n;
	}

	//指定桁の数値

	for(i = 1, div = 1; i < dig; i++, div *= 10);

	//セット

	for(i = 0; i < dig; i++, div /= 10, len++)
	{
		v = (n / div) % 10;

		//空白の出力を停止
		if(nostart && v) nostart = 0;

		*(buf++) = (fill_space && nostart)? ' ': '0' + v;
	}

	*buf = 0;

	return len;
}

/**@ 数値を浮動小数点の文字列に変換
 *
 * @d:dig = 1 なら、10 = 1.0。\
 * dig = 2 なら、100 = 1.0 となる。
 * 
 * @p:dig 小数点以下の桁数
 * @r:格納した文字数 */

int mIntToStr_float(char *buf,int32_t n,int dig)
{
	int len = 0,div,i;

	//桁数の値

	for(i = 0, div = 1; i < dig; i++, div *= 10);

	//負の値

	if(n < 0)
	{
		buf[len++] = '-';
		n = -n;
	}

	//整数部分

	len += mIntToStr(buf + len, n / div);

	//小数点部分

	if(dig > 0)
	{
		buf[len++] = '.';
		len += mIntToStr_dig(buf + len, n % div, dig);
	}

	return len;
}

/**@ 文字列を数値に変換 (範囲指定) */

int mStrToInt_range(const char *str,int min,int max)
{
	int n;

	n = atoi(str);

	if(n < min) n = min;
	else if(n < max) n = max;

	return n;
}


//=============================
// 文字列処理
//=============================


/**@ 文字列の配列バッファを解放
 *
 * @g:文字列処理
 *
 * @d:各文字列バッファと配列バッファを解放する。\
 * *buf == NULL となるまで *buf のポインタを解放し、最後に buf を解放する。
 * @p:buf NULL で何もしない */

void mStringFreeArray_tonull(char **buf)
{
	char **pp;

	if(!buf) return;

	//各文字列を解放

	for(pp = buf; *pp; pp++)
		mFree(*pp);

	//buf を解放

	mFree(buf);
}

/**@ 文字列内の指定文字を置き換え
 *
 * @p:str NULL で何もしない */

void mStringReplace(char *str,char from,char to)
{
	if(!str) return;

	for(; *str; str++)
	{
		if(*str == from)
			*str = to;
	}
}

/**@ 文字の並び順を逆にする
 *
 * @p:len 文字長さ */

void mStringReverse(char *str,int len)
{
	int cnt;
	char *p1,*p2;
	char c;

	//先頭と終端の文字を入れ替える

	p1 = str;
	p2 = str + len - 1;

	for(cnt = len >> 1; cnt > 0; cnt--, p1++, p2--)
	{
		c = *p1; *p1 = *p2; *p2 = c;
	}
}

/**@ 文字列内から指定文字の位置を返す
 *
 * @d:文字列の先頭から指定文字を探し、最初に見つかった位置を返す。
 * @r:文字が見つかった位置。\
 * 見つからなかった場合は、終端位置 (ヌル文字の位置)。 */

char *mStringFindChar(const char *str,char ch)
{
	for(; *str != ch && *str; str++);

	return (char *)str;
}

/**@ 文字列比較 (NULL で終わる文字列と、長さ指定の文字列)
 *
 * @d:NULL で終わる文字列 str1 と、str2 の指定長さ分の文字列を比較する。\
 *  str2[len2] = 0 と仮定する。 */

int mStringCompare_null_len(const char *str1,const char *str2,int len2)
{
	int ret;

	if(!str1 && !str2) return 0;

	ret = strncmp(str1, str2, len2);
	if(ret == 0)
		return (str1[len2] == 0)? 0: 1;
	else
		return ret;
}

/**@ 文字列比較 (文字長さ指定)
 *
 * @d:ポインタが NULL、または長さが 0 の場合でも正しく比較される。
 *
 * @r:0 で等しい。-1 で str1 < str2。1 で str1 > str2。 */

int mStringCompare_len(const char *str1,const char *str2,int len1,int len2)
{
	const uint8_t *pc1,*pc2;
	int c1,c2;

	if(!str1 && !str2) return 0;

	pc1 = (const uint8_t *)str1;
	pc2 = (const uint8_t *)str2;

	//異なる文字を検索

	c1 = (pc1)? *pc1: 0;
	c2 = (pc2)? *pc2: 0;

	while(c1 == c2 && len1 > 0 && len2 > 0)
	{
		c1 = (pc1)? *(pc1++): 0;
		c2 = (pc2)? *(pc2++): 0;

		len1--;
		len2--;
	}

	//終端位置なら 0 とする

	if(!len1) c1 = 0;
	if(!len2) c2 = 0;

	//比較

	if(c1 == c2)
		return 0;
	else
		return (c1 < c2)? -1: 1;
}

/**@ 文字列比較 (指定文字まで)
 *
 * @d:比較時、ch の文字が見つかった場合は、文字列の終端とみなす。\
 * また、ポインタが NULL の場合は、空文字列とみなす。
 * @r:0 で等しい。-1 で str1 < str2。1 で str1 > str2。 */

int mStringCompare_tochar(const char *str1,const char *str2,char ch)
{
	const uint8_t *pc1,*pc2;
	uint8_t c1,c2;

	pc1 = (const uint8_t *)str1;
	pc2 = (const uint8_t *)str2;

	c1 = (pc1)? *pc1: 0;
	c2 = (pc2)? *pc2: 0;

	while(c1 && c1 == c2 && c1 != ch && c2 != ch)
	{
		c1 = *(pc1++);
		c2 = *(pc2++);
	}

	if(c1 == ch) c1 = 0;
	if(c2 == ch) c2 = 0;

	if(c1 == c2)
		return 0;
	else
		return (c1 < c2)? -1: 1;
}

/**@ 文字列比較 (数字部分は数値として比較)
 *
 * @d:文字列内で数字が連続している部分は、数値として値を比較する。\
 * (マイナス記号は数字とみなさない)\
 * ポインタが NULL の場合は、空文字列とみなす。
 * @r:0 で等しい。-1 で str1 < str2。1 で str1 > str2。 */

int mStringCompare_number(const char *str1,const char *str2)
{
	const uint8_t *p1,*p2;
	char *pc1,*pc2;
	uint8_t c1,c2;
	int f;
	unsigned long int v1,v2;

	p1 = (const uint8_t *)str1;
	p2 = (const uint8_t *)str2;

	c1 = (p1)? *p1: 0;
	c2 = (p2)? *p2: 0;

	while(1)
	{
		//数字が見つかるまで通常比較

		f = 0;

		while(c1 && c2)
		{
			if(c1 >= '0' && c1 <= '9'
				&& c2 >= '0' && c2 <= '9')
			{
				f = 1;
				break;
			}

			if(c1 != c2) break;
			
			c1 = *(++p1);
			c2 = *(++p2);
		}

		//数字が見つからずに終わった場合の最終比較

		if(f == 0)
		{
			if(c1 == c2)
				return 0;
			else
				return (c1 < c2)? -1: 1;
		}

		//数字が見つかった場合、数値を取り出して比較

		v1 = strtoul((const char *)p1, &pc1, 10);
		v2 = strtoul((const char *)p2, &pc2, 10);

		if(v1 == v2)
		{
			//数値が同じ場合、後の文字列を続けて比較
			
			p1 = (const uint8_t *)pc1;
			p2 = (const uint8_t *)pc2;

			c1 = *p1;
			c2 = *p2;
		}
		else
			return (v1 < v2)? -1: 1;
	}
}

/**@ 指定文字を 0 に置き換えた文字列を複製
 *
 * @d:ch を 0 に置き換えて文字列を複製する。
 * @p:str NULL の場合、空文字列とみなす
 * @r:確保された文字列。NULL でエラー。\
 * 終端は 0 が２つ連続している。*/

char *mStringDup_replace0(const char *str,char ch)
{
	char *buf;
	int len;

	len = (str)? strlen(str): 0;

	buf = (char *)mMalloc(len + 2);
	if(!buf) return NULL;

	memcpy(buf, str, len);

	buf[len] = 0;
	buf[len + 1] = 0;

	mStringReplace(buf, ch, 0);

	return buf;
}


//=====================


/**@ 指定文字で区切った次の文字列を取得
 *
 * @d:"abc;def;" などのように、特定文字で区切った一つの文字列内から、
 * 次の文字列を取得する。\
 * \
 * {em:区切り文字が複数連続している部分は、一つの区切り文字として扱われる。:em}\
 * ch を 0 にして使うことはできない。\
 * \
 * 実行後は、*ppend の値を *pptop に代入して、繰り返し実行していく。
 *
 * @p:pptop 先頭位置。次の先頭位置が格納される。
 * @p:ppend 次の文字列の開始位置が格納される。\
 * *pptop 〜 *ppend - 1 の範囲が、区切った文字列となる。
 * @r:終端まで来た場合、FALSE */

mlkbool mStringGetNextSplit(const char **pptop,const char **ppend,char ch)
{
	const char *top;
	char *end;

	if(!(*pptop)) return FALSE;

	//文字列の先頭位置 (ch の文字をスキップ)

	for(top = *pptop; *top == ch && *top; top++);

	if(!(*top)) return FALSE;

	//終端 (次の ch 位置)

	end = mStringFindChar(top, ch);

	*pptop = top;
	*ppend = end;

	return TRUE;
}

/**@ 指定文字で区切られた文字列から、指定位置のテキストを取得
 *
 * @d:区切り文字が複数連続している部分は、1つの区切り文字として扱う。\
 * ch を 0 にして使うことはできない。
 *
 * @p:index 取得するテキストのインデックス位置
 * @p:pptop テキストの先頭位置が入る。範囲外の場合 NULL。
 * @r:テキストの長さ。-1 でインデックスが範囲外。 */

int mStringGetSplitText_atIndex(const char *str,char ch,int index,char **pptop)
{
	const char *top,*end;
	int cur = 0;

	top = str;

	while(mStringGetNextSplit(&top, &end, ch))
	{
		if(cur == index)
		{
			*pptop = (char *)top;
			return end - top;
		}
	
		top = end;
		cur++;
	}

	*pptop = NULL;

	return -1;
}

/**@ ヌル文字で区切られた文字列から、指定位置のテキストを取得
 *
 * @p:str 終端には、2つのヌル文字が連続していること。
 * @r:範囲内にテキストがない場合、NULL。\
 *  index = 0 で、先頭がヌル文字の場合も NULL。 */

char *mStringGetNullSplitText(const char *str,int index)
{
	int cur;

	if(!str) return NULL;

	for(cur = 0; *str; str += strlen(str) + 1, cur++)
	{
		if(cur == index) return (char *)str;
	}

	return NULL;
}

/**@ 文字列の行数取得
 *
 * @d:改行 (\\n) が見つかった回数が行数となる。\
 * 終端が改行の場合、行数には含めない。
 * @r:行数 */

int mStringGetLineCnt(const char *str)
{
	const char *pc,*pc2;
	int cnt = 1;

	if(!str) return 0;
	
	for(pc = str; *pc; )
	{
		pc2 = strchr(pc, '\n');
		if(!pc2) break;
		
		pc = pc2 + 1;

		//次が終端なら含めない
		if(*pc) cnt++;
	}
	
	return cnt;
}

/**@ 次の行の先頭位置を取得
 *
 * @d:文字列内から、改行の次の位置を取得する。\
 * 改行は \\n と \\r が対象。\
 * 改行で終わっている場合、戻り値は終端位置となる。
 * 
 * @p:replace0 改行文字を 0 に置き換えるか。\
 * \\r+\\n の場合は、２つとも NULL になる。
 * @r:次の行の先頭位置。NULL で終了。 */

char *mStringGetNextLine(char *str,mlkbool replace0)
{
	char *pc;

	if(!(*str)) return NULL;

	//改行を検索

	for(pc = str; *pc != '\r' && *pc != '\n' && *pc; pc++);

	//置き換えと、次の先頭位置

	if(*pc == '\r' && pc[1] == '\n')
	{
		if(replace0) *pc = 0, pc[1] = 0;
		pc += 2;
	}
	else if(*pc)
	{
		if(replace0) *pc = 0;
		pc++;
	}

	return pc;
}

/**@ ヌル文字で区切った複数文字列の個数を取得
 *
 * @d:文字列の最後にはヌル文字が2つ続いていること。 */

int mStringGetNullSplitNum(const char *str)
{
	int num = 0;

	for(; *str; str += strlen(str) + 1, num++);

	return num;
}

/**@ 指定文字で区切られた文字列から int 値の配列を取得
 *
 * @p:ch 区切り文字
 * @p:maxnum 配列の最大数
 * @r:セットされた個数 */

int mStringGetSplitInt(const char *str,char ch,int *dst,int maxnum)
{
	const char *end;
	int cnt = 0;

	while(mStringGetNextSplit(&str, &end, ch) && cnt < maxnum)
	{
		*(dst++) = atoi(str);
		cnt++;
	
		str = end;
	}

	return cnt;
}

/**@ 指定文字で区切られた文字列の中から、文字列を見つけてインデックスを取得
 *
 * @d:str 内の、ch で区切られた文字列の中から、target と等しい文字列を探し、
 * そのインデックス位置を返す。
 * 
 * @p:target 検索する文字列
 * @p:len target 文字列の長さ。負の値でヌル文字まで。
 * @p:str 検索元の、ch で区切った文字列。\
 * 区切り文字が連続している場合は、空文字列の区切りとみなす (検索対象となる)。
 * @p:ch 区切り文字
 * @p:iscase TRUE で大文字小文字を区別しない
 * @r:見つかった場合、str 内の単語単位のインデックス位置。\
 * -1 で見つからなかった。 */

int mStringGetSplitTextIndex(const char *target,int len,const char *str,char ch,mlkbool iscase)
{
	const char *pc,*pend;
	int ret,no;

	if(len < 0)
		len = strlen(target);

	for(pc = str, no = 0; *pc; no++)
	{
		//終端

		pend = mStringFindChar(pc, ch);

		//文字数が同じ場合、比較

		if(pend - pc == len)
		{
			if(iscase)
				ret = strncasecmp(target, pc, len);
			else
				ret = strncmp(target, pc, len);

			if(ret == 0) return no;
		}

		//次へ

		pc = (*pend)? pend + 1: pend;
	}

	return -1;
}


//=====================


/**@ 文字列が ASCII 文字 (0x00-0x7E) のみで構成されているか */

mlkbool mStringIsASCII(const char *str)
{
	for(; *str > 0 && *str < 0x7f; str++);

	return (*str == 0);
}

/**@ 文字列が半角数字のみで構成されているか */

mlkbool mStringIsNum(const char *str)
{
	for(; *str >= '0' && *str <= '9'; str++);

	return (*str == 0);
}

/**@ ファイル名として有効な文字のみで構成されているか
 *
 * @d:パスは含まないこと。 */

mlkbool mStringIsValidFilename(const char *str)
{
	//[Linux] '/' 以外使用可能

	for(; *str; str++)
	{
		if(*str == '/') return FALSE;
	}

	return TRUE;
}

/**@ 文字列を大文字に変換 */

void mStringUpper(char *str)
{
	if(!str) return;

	for(; *str; str++)
	{
		if(*str >= 'a' && *str <= 'z')
			*str -= 0x20;
	}
}

/**@ 文字列を小文字に変換 */

void mStringLower(char *str)
{
	if(!str) return;

	for(; *str; str++)
	{
		if(*str >= 'A' && *str <= 'Z')
			*str += 0x20;
	}
}

/**@ 可変長の文字長さと文字列データを合わせたバイト数を取得
 *
 * @d:文字長さは可変長で、128 以上の場合は、最上位ビットを ON にして、
 * 下位 7bit に値を入れる。 */

int mStringGetVariableSize(const char *str)
{
	int len,bytes;

	if(!str)
		return 1;
	else
	{
		len = strlen(str);
		bytes = 1 + len;

		for(; len > 127; len >>= 7, bytes++);

		return bytes;
	}
}


//========================
// マッチ
//========================


#if !defined(MLK_NO_UNICODE)

/**@ 文字列マッチ (ワイルドカード有効)
 *
 * @p:str マッチ対象文字列
 * @p:pattern マッチのパターン文字列。\
 * '*' で、0文字以上のすべての文字にマッチ。\
 * '?' で、1文字のすべての文字にマッチ。
 * @p:iscase TRUE で大文字小文字を区別しない
 * @r:マッチしたか */

mlkbool mStringMatch(const char *str,const char *pattern,mlkbool iscase)
{
	mlkuchar usrc,upat;
	int ret;

	if(!str || !pattern) return FALSE;

	while(*str && *pattern)
	{
		//Unicode 取得
		
		if(mUTF8GetChar(&usrc, str, -1, &str) < 0
			|| mUTF8GetChar(&upat, pattern, -1, &pattern) < 0)
			break;
	
		if(upat == '*')
		{
			//最後の '*' ならすべてマッチ

			if(*pattern == 0) return TRUE;

			//

			while(*str)
			{
				if(mStringMatch(str, pattern, iscase))
					return TRUE;

				ret = mUTF8GetCharBytes(str);
				if(ret == -1) return FALSE;

				str += ret;
			}

			return FALSE;
		}
		//'?' は無条件で1文字進める
		else if(upat != '?')
		{
			//通常文字比較

			if(iscase)
			{
				if(usrc >= 'A' && usrc <= 'Z') usrc += 0x20;
				if(upat >= 'A' && upat <= 'Z') upat += 0x20;
			}

			if(usrc != upat) return FALSE;
		}
	}

	//双方最後まで来たらマッチした
	return (*str == 0 && *pattern == 0);
}

/**@ 複数のパターンから、ワイルドカードマッチ
 *
 * @p:str 比較対象文字列
 * @p:patterns パターン文字列。\
 *  {em:ヌル文字で区切り、最後はヌル文字を２つ続ける。:em}\
 * @p:iscase TRUE で大文字小文字を区別しない
 * @r:-1 でマッチしなかった。\
 *  0 以上で、マッチしたパターンのインデックス番号。 */

int mStringMatch_sum(const char *str,const char *patterns,mlkbool iscase)
{
	const char *pat;
	int no = 0;

	for(pat = patterns; *pat; pat += strlen(pat) + 1, no++)
	{
		if(mStringMatch(str, pat, iscase))
			break;
	}

	return (*pat)? no: -1;
}

#endif

