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
 * mStr (UTF-8 文字列)
 *****************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "mlk.h"
#include "mlk_str.h"
#include "mlk_string.h"
#include "mlk_charset.h"
#include "mlk_unicode.h"
#include "mlk_util.h"


/*-----------

buf : 文字列バッファ (NULL で未確保)
len : 文字列長さ (NULL 含まない)
allocsize : バッファ確保サイズ (NULL 含む)

[!] buf == NULL の状態でも正しく動作するようにすること。
[!] 常に正規化された UTF-8 文字列であること。

-------------*/


//==============================
// sub
//==============================


/* 文字長さから確保サイズ取得
 * 
 * return: -1 = サイズが大きすぎる */

static int _get_allocsize(int len)
{
	int i;
	
	for(i = 32; i <= len && i < (1<<20); i <<= 1);
	
	return (i >= (1<<20))? -1: i;
}

/* フォーマット文字列追加 */

static void _append_format(mStr *str,const char *pc,va_list ap)
{
	const char *pctxt;
	int dig,fdig,nval;
	uint8_t fill0;
	char m[32],m2[16];

	for(; *pc; pc++)
	{
		//% 以外
	
		if(*pc != '%')
		{
			mStrAppendChar(str, *pc);
			continue;
		}

		//------ %

		pc++;
		dig = fdig = 0;
		fill0 = FALSE;

		//次の文字

		if(*pc == 0)
			break;
		else if(*pc == '%')
		{
			//%%
			mStrAppendChar(str, '%');
			continue;
		}

		//'0' (桁を0で埋める)

		if(*pc == '0')
		{
			fill0 = TRUE;
			pc++;
		}

		//桁数
		
		if(*pc == '*')
		{
			//引数で桁数指定

			dig = va_arg(ap, int);
			pc++;
		}
		else
		{
			//数値:桁数指定

			for(; *pc >= '0' && *pc <= '9'; pc++)
			{
				dig *= 10;
				dig += *pc - '0';
			}
		}

		//小数点以下桁数

		if(*pc == '.')
		{
			for(pc++; *pc >= '0' && *pc <= '9'; pc++)
			{
				fdig *= 10;
				fdig += *pc - '0';
			}
		}

		//型

		if(!(*pc)) break;

		switch(*pc)
		{
			//int
			case 'd':
				nval = va_arg(ap, int);
				
				if(dig == 0)
					//桁数指定なし
					mStrAppendInt(str, nval);
				else
				{
					//桁数指定あり
					
					if(dig > 30) dig = 30;
					if(!fill0) dig = -dig;
					
					mIntToStr_dig(m, nval, dig);
					mStrAppendText(str, m);
				}
				break;

			//int 浮動小数点
			case 'F':
				nval = va_arg(ap, int);

				mIntToStr_float(m, nval, fdig);
				mStrAppendText(str, m);
				break;

			//16進数
			case 'X':
			case 'x':
				if(dig == 0)
				{
					//桁数指定なし

					m2[0] = '%';
					m2[1] = *pc;
					m2[2] = 0;
				}
				else
				{
					//桁数指定あり

					if(dig > 16) dig = 16;

					if(fill0)
						snprintf(m2, 16, "%%0%d%c", dig, *pc);
					else
						snprintf(m2, 16, "%%%d%c", dig, *pc);
				}

				nval = va_arg(ap, int);
				snprintf(m, 32, m2, nval);
				mStrAppendText(str, m);
				break;

			//char
			case 'c':
				nval = va_arg(ap, int);

				if(nval >= -128 && nval <= 127)
					mStrAppendChar(str, nval);
				break;

			//char *
			case 's':
				pctxt = va_arg(ap, const char *);
				
				if(dig == 0)
					mStrAppendText(str, pctxt);
				else
					mStrAppendText_len(str, pctxt, dig);
				break;

			//mStr *
			case 't':
				mStrAppendStr(str, va_arg(ap, mStr *));
				break;
		}
	}
}

/* パス区切りの最後の位置を取得
 *
 * return: -1 で見つからなかった */

static int _get_path_seppos(const char *path)
{
	char *pc;

	if(!path)
		return -1;
	else
	{
		pc = strrchr(path, MLK_DIRSEP);

		return (pc)? pc - path: -1;
	}
}

/* ファイル名部分の先頭位置取得 */

static char *_get_path_fname(const char *path)
{
	char *pc;

	if(!path)
		return NULL;
	else
	{
		pc = strrchr(path, MLK_DIRSEP);

		if(pc)
			return pc + 1;
		else
			return (char *)path;
	}
}


//==============================
// main
//==============================


/**@ 解放 */

void mStrFree(mStr *str)
{
	if(str->buf)
	{
		mFree(str->buf);
		
		str->buf = NULL;
		str->len = str->allocsize = 0;
	}
}

/**@ 初期化 */

void mStrInit(mStr *str)
{
	memset(str, 0, sizeof(mStr));
}

/**@ 初期バッファ確保
 *
 * @d:str の現在値に関係なく、強制的に値が変更される。
 * @p:size 初期確保サイズ */

mlkbool mStrAlloc(mStr *str,int size)
{
	str->buf = (char *)mMalloc(size);
	if(!str->buf) return FALSE;

	str->buf[0] = 0;
	str->len = 0;
	str->allocsize = size;
		
	return TRUE;
}

/**@ バッファを拡張リサイズ
 *
 * @d:バッファに余裕があれば何もしない。
 * @p:len 文字の長さ
 * @r:FALSE で、サイズが大きすぎる、または確保に失敗。 */

mlkbool mStrResize(mStr *str,int len)
{
	int size;
	char *newbuf;
	
	size = _get_allocsize(len);
	if(size == -1) return FALSE;
	
	if(!str->buf)
	{
		//新規確保

		return mStrAlloc(str, size);
	}
	else if(size > str->allocsize)
	{
		//サイズ変更
	
		newbuf = (char *)mRealloc(str->buf, size);
		if(!newbuf) return FALSE;
		
		str->buf = newbuf;
		str->allocsize = size;
	}

	return TRUE;
}


//====================


/**@ 空文字列かどうか */

mlkbool mStrIsEmpty(const mStr *str)
{
	return (!str->buf || !str->buf[0]);
}

/**@ 空文字列でないかどうか
 *
 * @r:TRUE で空文字列でない */

mlkbool mStrIsnotEmpty(const mStr *str)
{
	return (str->buf && str->buf[0]);
}

/**@ 終端の1文字を取得 */

char mStrGetLastChar(const mStr *str)
{
	if(str->buf && str->len > 0)
		return str->buf[str->len - 1];
	else
		return 0;
}

/**@ UTF-8 の文字数からバイト数を取得
 *
 * @d:先頭から len 文字数までのバイト数を返す。
 * @p:len 1文字を1単位とした文字数 */

int mStrGetBytes_tolen(const mStr *str,int len)
{
	int pos;
	
	for(pos = 0; len > 0 && pos < str->len; len--)
		pos += mUTF8GetCharBytes(str->buf + pos);
	
	if(pos > str->len) pos = str->len;
	
	return pos;
}


//===================


/**@ 文字列を空にする */

void mStrEmpty(mStr *str)
{
	if(str->buf)
	{
		str->buf[0] = 0;
		str->len = 0;
	}
}

/**@ 文字列の長さをセット
 *
 * @d:len の位置にヌル文字をセットして、文字長さを変更する。\
 * @p:len 確保サイズ未満の位置 */

void mStrSetLen(mStr *str,int len)
{
	if(len < str->allocsize)
	{
		str->buf[len] = 0;
		str->len = len;
	}
}

/**@ 文字長さを、指定バイト以下になるように調整
 *
 * @d:UTF-8 の1文字が崩れないように、指定バイト以下に収める。
 * @r:TRUE で文字列が切り詰められた */

mlkbool mStrSetLen_bytes(mStr *str,int size)
{
	int pos,n;

	if(str->buf && str->len)
	{
		for(pos = 0; pos < str->len; pos += n)
		{
			n = mUTF8GetCharBytes(str->buf + pos);

			if(pos + n > size)
			{
				str->buf[pos] = 0;
				str->len = pos;
				return TRUE;
			}
		}
	}
	
	return FALSE;
}

/**@ 文字列の長さをセット (ヌル文字の位置)
 *
 * @d:ヌル文字の位置を取得して、それを文字列の長さとする。 */

void mStrSetLen_null(mStr *str)
{
	if(str->buf)
		str->len = strlen(str->buf);
}

/**@ 指定文字数までに制限した文字列にする
 *
 * @d:指定文字数より大きい場合、終端に "..." がつく。\
 * 改行は除外される。
 *
 * @p:len UTF-8 文字数
 * @r:TRUE で指定長さ以上の文字列 */

mlkbool mStrLimitText(mStr *str,int len)
{
	mStr tmp;
	const char *pc;
	mlkuchar c;

	if(!str->buf || !str->len) return FALSE;

	mStrCopy_init(&tmp, str);

	mStrEmpty(str);

	for(pc = tmp.buf; *pc; )
	{
		mUTF8GetChar(&c, pc, -1, &pc);

		if(c == '\n' || c == '\r')
			continue;

		if(len <= 0)
		{
			mStrAppendText(str, "...");
			mStrFree(&tmp);
			return TRUE;
		}

		mStrAppendUnichar(str, c);
		len--;
	}

	mStrFree(&tmp);

	return FALSE;
}

/**@ UTF-8 文字列を検証
 *
 * @d:不正な文字があればそこで終了し、無効な文字は取り除く。\
 * 他の関数では検証しつつセットされているが、直接セットした文字列を検証したい場合に使う。 */

void mStrValidate(mStr *str)
{
	mStr tmp;
	int len;

	mStrCopy_init(&tmp, str);

	len = mUTF8CopyValidate(str->buf, tmp.buf, tmp.len);

	str->buf[len] = 0;
	str->len = len;

	mStrFree(&tmp);
}

/**@ mStr の内容をコピー
 *
 * @d:src が空の場合、dst は未確保状態のままになる場合がある。 */

void mStrCopy(mStr *dst,const mStr *src)
{
	if(mStrIsEmpty(src))
		mStrEmpty(dst);
	else if(mStrResize(dst, src->len))
	{
		memcpy(dst->buf, src->buf, src->len + 1);

		dst->len = src->len;
	}
}

/**@ mStr の内容をコピー (強制初期化)
 *
 * @d:dst を強制的に初期化して、コピーする。\
 * dst が初期化されていない状態で、直接コピーしたい時に。 */

void mStrCopy_init(mStr *dst,const mStr *src)
{
	mStrInit(dst);
	mStrCopy(dst, src);
}

/**@ mStr の内容をコピー (常に確保)
 *
 * @d:src が空の場合でも、dst は必ずバッファが確保される。 */

void mStrCopy_alloc(mStr *dst,const mStr *src)
{
	if(!mStrIsEmpty(src))
		mStrCopy(dst, src);
	else
	{
		//src が空の場合

		if(dst->buf)
			mStrEmpty(dst);
		else
			mStrAlloc(dst, 32);
	}
}


//================================
// セット
//================================


/**@ 1文字のみの文字列にする */

void mStrSetChar(mStr *str,char c)
{
	if(mStrResize(str, 1))
	{
		str->buf[0] = c;
		str->buf[1] = 0;
		str->len = 1;
	}
}

/**@ 文字列をセット
 * 
 * @p:text UTF-8。NULL で空にする。 */

void mStrSetText(mStr *str,const char *text)
{
	int len;

	if(!text)
		mStrEmpty(str);
	else
	{
		len = strlen(text);
		
		if(mStrResize(str, len))
		{
			len = mUTF8CopyValidate(str->buf, text, len);

			str->buf[len] = 0;
			str->len = len;
		}
	}
}

/**@ 文字列をセット (長さ指定)
 *
 * @d:len より前の位置にヌル文字がある場合は、その位置までとなる。\
 *  複数バイトの1文字の途中の位置となる場合は、認識できる最後の文字までとなる。
 * @p:text NULL で空にする
 * @p:len 0 以下で空にする */

void mStrSetText_len(mStr *str,const char *text,int len)
{
	if(len < 1 || !text)
		mStrEmpty(str);
	else if(mStrResize(str, len))
	{
		len = mUTF8CopyValidate(str->buf, text, len);

		str->buf[len] = 0;
		str->len = len;
	}
}

/**@ ロケールの文字列からセット
 *
 * @p:len 負の値でヌル文字まで */

void mStrSetText_locale(mStr *str,const char *text,int len)
{
	char *buf;
	int dlen;
	
	if(!text)
		mStrEmpty(str);
	else
	{
		buf = mLocaletoUTF8(text, len, &dlen);
		if(!buf) return;
		
		mStrSetText_len(str, buf, dlen);
		
		mFree(buf);
	}
}

/**@ UTF-32 文字列からセット
 *
 * @d:エンディアンはホストに依存。 */

void mStrSetText_utf32(mStr *str,const mlkuchar *text,int len)
{
	int dlen;

	mStrEmpty(str);
	
	if(text)
	{
		dlen = mUTF32toUTF8(text, len, NULL, 0);

		if(dlen != -1 && mStrResize(str, dlen))
		{
			mUTF32toUTF8(text, len, str->buf, str->allocsize);
			
			str->len = dlen;
		}
	}
}

/**@ UTF-16BE から文字列セット
 *
 * @p:len 2byte 単位での文字数 */

void mStrSetText_utf16be(mStr *str,const void *text,int len)
{
	uint16_t *buf;
	int dlen;

	mStrEmpty(str);

	if(text && len > 0)
	{
		buf = (uint16_t *)mMalloc(len * 2);
		if(!buf) return;

		mCopyBuf_16bit_BEtoHOST(buf, text, len);

		dlen = mUTF16toUTF8(buf, len, NULL, 0);

		if(dlen != -1 && mStrResize(str, dlen))
		{
			mUTF16toUTF8(buf, len, str->buf, str->allocsize);
			str->len = dlen;
		}

		mFree(buf);
	}
}

/**@ int 値を文字列に変換してセット */

void mStrSetInt(mStr *str,int val)
{
	char m[16];
	
	mIntToStr(m, val);
	mStrSetText(str, m);
}

/**@ int 値を指定桁数で文字列に変換してセット */

void mStrSetIntDig(mStr *str,int val,int dig)
{
	char m[32];
	
	mIntToStr_dig(m, val, dig);
	mStrSetText(str, m);
}

/**@ double 値を文字列に変換してセット
 *
 * @d:小数点以下が 0 になる場合、整数部分のみをセットする。
 * @p:dig 小数点以下の桁数。 */

void mStrSetDouble(mStr *str,double d,int dig)
{
	char m[64];

	if(d - (int)d == 0)
		//小数点以下が0
		snprintf(m, 64, "%d", (int)d);
	else
		snprintf(m, 64, "%.*f", dig, d);

	mStrSetText(str, m);
}

/**@ フォーマット文字列からセット */

void mStrSetFormat(mStr *str,const char *format,...)
{
	va_list ap;

	mStrEmpty(str);

	va_start(ap, format);

	_append_format(str, format, ap);

	va_end(ap);
}


//============================
// 追加
//============================


/**@ 1文字追加
 *
 * @p:c  0 でも可 */

void mStrAppendChar(mStr *str,char c)
{
	if(mStrResize(str, str->len + 1))
	{
		str->buf[str->len] = c;
		str->buf[str->len + 1] = 0;
		str->len++;
	}
}

/**@ Unicode 1文字を追加 */

void mStrAppendUnichar(mStr *str,mlkuchar c)
{
	int len;
	
	len = mUnicharToUTF8(c, NULL, 0);
	
	if(len > 0 && mStrResize(str, str->len + len))
	{
		mUnicharToUTF8(c, str->buf + str->len, len);
		
		str->len += len;
		str->buf[str->len] = 0;
	}
}

/**@ 文字列を追加 */

void mStrAppendText(mStr *str,const char *text)
{
	int len;

	if(text)
	{
		len = strlen(text);
		
		if(mStrResize(str, str->len + len))
		{
			len = mUTF8CopyValidate(str->buf + str->len, text, len);

			str->len += len;
			str->buf[str->len] = 0;
		}
	}
}

/**@ 文字列を追加 (長さ指定) */

void mStrAppendText_len(mStr *str,const char *text,int len)
{
	if(text && len > 0 && mStrResize(str, str->len + len))
	{
		len = mUTF8CopyValidate(str->buf + str->len, text, len);

		str->len += len;
		str->buf[str->len] = 0;
	}
}

/**@ ロケールの文字列を追加
 *
 * @p:len 負の値でヌル文字まで */

void mStrAppendText_locale(mStr *str,const char *text,int len)
{
	char *buf;
	int dlen;

	if(!text) return;

	buf = mLocaletoUTF8(text, len, &dlen);
	if(buf)
	{
		mStrAppendText_len(str, buf, dlen);
		mFree(buf);
	}
}

/**@ 指定文字を '\\' でエスケープする
 *
 * @d:text 内に lists で列挙した文字がある場合、'\\' + 文字にする。\
 *  '\\' は、'\\\\' となる。
 *
 * @p:text 元の文字列
 * @p:lists エスケープ対象の文字のリスト (各文字が対象となる) */

void mStrAppendText_escapeChar(mStr *str,const char *text,const char *lists)
{
	const char *pc;
	int flag;
	char c;

	if(!text) return;

	while((c = *(text++)))
	{
		//エスケープ対象文字か

		if(c == '\\')
			flag = 1;
		else
		{
			for(pc = lists; *pc && c != *pc; pc++);

			flag = (*pc != 0);
		}
		
		//
	
		if(flag) mStrAppendChar(str, '\\');

		mStrAppendChar(str, c);
	}

	mStrValidate(str);
}

/**@ コマンドライン用にエスケープした文字列を追加
 *
 * @d:エスケープ対象は、『!?()[]<>{};&|^*$`'"\』 */

void mStrAppendText_escapeForCmdline(mStr *str,const char *text)
{
	//32-127 の各文字のフラグ (1bit)
	// !?()[]<>{};&|^*$`'"\ の文字がエスケープ対象
	uint8_t flags[12] = {0xd7,0x07,0x00,0xd8,0x00,0x00,0x00,0x78,0x01,0x00,0x00,0x38};
	char c;
	int n;
	
	if(!text) return;

	while((c = *(text++)))
	{
		//フラグが ON ならエスケープ対象
		
		if(c >= 32 && c <= 127)
		{
			n = c - 32;

			if(flags[n >> 3] & (1 << (n & 7)))
				mStrAppendChar(str, '\\');
		}

		mStrAppendChar(str, c);
	}
}

/**@ int 値を文字列に変換して追加 */

void mStrAppendInt(mStr *str,int val)
{
	char m[16];

	mIntToStr(m, val);
	mStrAppendText(str, m);
}

/**@ double 値を文字列に変換して追加
 *
 * @p:dig 小数点以下の桁数 */

void mStrAppendDouble(mStr *str,double d,int dig)
{
	char m[64];

	if(d - (int)d == 0)
		snprintf(m, 64, "%d", (int)d);
	else
		snprintf(m, 64, "%.*f", dig, d);

	mStrAppendText(str, m);
}

/**@ mStr の文字列を追加 */

void mStrAppendStr(mStr *dst,mStr *src)
{
	if(src->buf && mStrResize(dst, dst->len + src->len))
	{
		memcpy(dst->buf + dst->len, src->buf, src->len + 1);
		dst->len += src->len;
	}
}

/**@ フォーマット文字列を追加 */

void mStrAppendFormat(mStr *str,const char *format,...)
{
	va_list ap;
	
	va_start(ap, format);

	_append_format(str, format, ap);

	va_end(ap);
}

/**@ 先頭に文字列を挿入 */

void mStrPrependText(mStr *str,const char *text)
{
	mStr tmp;

	mStrCopy_init(&tmp, str);

	mStrSetText(str, text);
	mStrAppendStr(str, &tmp);

	mStrFree(&tmp);
}


//==============================
// パーセントエンコーディング
//==============================


/**@ パーセントエンコーディングにエンコードして追加
 *
 * @d:パーセントエンコーディングは、URL などで使われ、
 * 半角英数字と一部の記号以外は '%XX' (16進数) で表す。\
 * エンコードされない (そのままとなる) 記号は、【 - . _ ~】
 * @p:space_to_plus TRUE で、半角空白を '+' にする。FALSE で %20。 */

void mStrAppendEncode_percentEncoding(mStr *str,const char *text,mlkbool space_to_plus)
{
	char c,flag,m[6];
	
	if(!text) return;

	for(; (c = *text); text++)
	{
		//flag: 0=そのまま 1=エンコーディング

		if((c >= 0 && c < 0x20) || c == 0x7f)
			continue;
		else if(space_to_plus && c == ' ')
		{
			flag = 0;
			c = '+';
		}
		else if(isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~')
			flag = 0;
		else
			flag = 1;

		//

		if(flag == 0)
			mStrAppendChar(str, c);
		else
		{
			snprintf(m, 6, "%%%02X", (uint8_t)c);
			mStrAppendText(str, m);
		}
	}
}

/**@ パーセントエンコーディングをデコードした文字列をセット
 *
 * @p:plus_to_space '+' はスペースに変換する。 */

void mStrSetDecode_percentEncoding(mStr *str,const char *text,mlkbool plus_to_space)
{
	char c,m[3] = {0,0,0};

	mStrEmpty(str);

	if(!text) return;

	for(; (c = *text); text++)
	{
		if(plus_to_space && c == '+')
			mStrAppendChar(str, ' ');
		else if(c != '%')
			mStrAppendChar(str, c);
		else if(text[1] && text[2])
		{
			//%XX
			
			m[0] = text[1];
			m[1] = text[2];

			c = (char)strtoul(m, NULL, 16);
			if(c == 0) break;
			
			mStrAppendChar(str, c);

			text += 2;
		}
	}

	mStrValidate(str);
}


//============================
// URL
//============================


/**@ text/uri-list の文字列をデコードしてセット
 *
 * @d:複数の URI は改行 (CR/LF) で区切られている。\
 * 行頭が '#' の場合はコメントとして扱い、除去する。\
 * 変換後、各 URI は '\\t' で区切り、'%XX' は文字に変換する。
 *
 * @p:text UTF-8 文字列
 * @p:localfile TRUE で、"file://" で始まるローカルファイルのみ取得し、先頭の "file://" を除去する。
 * @r:セットされた URI の数 */

int mStrSetDecode_urilist(mStr *str,const char *text,mlkbool localfile)
{
	char *buf,*top,*next,*ps,*pd;
	char m[3] = {0,0,0};
	int num = 0;

	mStrEmpty(str);

	//バッファを直接修正していくため、元文字列をコピー

	buf = mStrdup(text);
	if(!buf) return 0;

	//

	for(top = buf; top; top = next)
	{
		//行取得
		
		next = mStringGetNextLine(top, TRUE);
		if(!next) break;

		//コメント

		if(*top == '#') continue;
	
		//"file://" のみ

		if(localfile)
		{
			if(*top == 'f' && strncmp(top, "file://", 7) == 0)
				top += 7;
			else
				continue;
		}

		//終端まで文字変換 (バッファに直接セット)

		for(ps = pd = top; *ps; pd++)
		{
			if(*ps == '%' && ps[1] && ps[2])
			{
				m[0] = ps[1];
				m[1] = ps[2];

				*pd = (char)strtoul(m, NULL, 16);
				ps += 3;
			}
			else
			{
				if(ps != pd) *pd = *ps;
				ps++;
			}
		}

		*pd = 0;

		//URI 追加

		if(*top)
		{
			mStrAppendText_len(str, top, pd - top);
			mStrAppendChar(str, '\t');

			num++;
		}
	}

	mFree(buf);

	return num;
}


//==================================
// 取得
//==================================


/**@ 指定位置から指定文字数分を取り出す
 *
 * @p:dst 格納先
 * @p:src 取り出す元文字列
 * @p:pos src から取り出す開始位置
 * @p:len 取り出す文字数。負の値で、終端まで。 */

void mStrGetMid(mStr *dst,mStr *src,int pos,int len)
{
	int maxlen;

	if(pos >= src->len)
		//開始位置が終端以上
		mStrEmpty(dst);
	else
	{
		maxlen = src->len - pos;

		if(len < 0 || len > maxlen)
			len = maxlen;

		mStrSetText_len(dst, src->buf + pos, len);
	}
}

/**@ 指定文字で区切られた中の指定位置の文字列を取得
 *
 * @d:区切り文字が複数連続している場合は空文字列とする。
 * @p:split 区切り文字
 * @p:index 取得する文字列のインデックス番号 (0〜) */

void mStrGetSplitText(mStr *dst,const char *text,char split,int index)
{
	const char *pc,*pend;
	int cnt;

	mStrEmpty(dst);

	if(!text) return;

	for(pc = text, cnt = 0; *pc; cnt++)
	{
		pend = mStringFindChar(pc, split);

		if(cnt == index)
		{
			mStrSetText_len(dst, pc, pend - pc);
			break;
		}

		//次へ

		pc = (*pend)? pend + 1: pend;
	}
}


//==================================
// 変換
//==================================


/**@ 半角英字を小文字に変換 */

void mStrLower(mStr *str)
{
	char *p;

	if(!str->buf) return;
	
	for(p = str->buf; *p; p++)
	{
		if(*p >= 'A' && *p <= 'Z')
			*p += 0x20;
	}
}

/**@ 半角英字を大文字に変換 */

void mStrUpper(mStr *str)
{
	char *p;
	
	if(!str->buf) return;

	for(p = str->buf; *p; p++)
	{
		if(*p >= 'a' && *p <= 'z')
			*p -= 0x20;
	}
}

/**@ 文字列を int 数値に変換
 *
 * @r:空の場合、0 が返る。 */

int mStrToInt(mStr *str)
{
	if(str->buf)
		return atoi(str->buf);
	else
		return 0;
}

/**@ 文字列を double 値に変換 */

double mStrToDouble(mStr *str)
{
	if(str->buf)
		return strtod(str->buf, NULL);
	else
		return 0;
}

/**@ 指定文字で区切られた複数数値を配列で取得
 *
 * @p:dst 格納先
 * @p:split 区切り文字。0 で、数字以外の文字すべてを区切りとする。
 * @p:maxnum 取得する最大数
 * @r:取得した個数 */

int mStrToIntArray(mStr *str,int *dst,int maxnum,char split)
{
	char *p = str->buf;
	int num = 0;

	if(!p) return 0;

	while(*p && num < maxnum)
	{
		//数値取得

		*(dst++) = atoi(p);
		num++;

		//次の数値へ

		if(split)
		{
			for(; *p && *p != split; p++);
		}
		else
		{
			//数字以外
			for(; *p && *p >= '0' && *p <= '9'; p++);
		}

		if(*p) p++;
	}

	return num;
}

/**@ 指定文字で区切られた複数数値を配列で取得
 *
 * @d:値を min,max の範囲内に収めて格納する。 */

int mStrToIntArray_range(mStr *str,int *dst,int maxnum,char split,int min,int max)
{
	int i,num,n;

	num = mStrToIntArray(str, dst, maxnum, split);

	for(i = 0; i < num; i++)
	{
		n = dst[i];
		if(n < min) n = min;
		else if(n > max) n = max;

		dst[i] = n;
	}

	return num;
}

/**@ 文字を置き換え
 *
 * @p:to 0 でも可 (文字長さはそのままとなる)。\
 *  ※終端にヌルが２つ必要な場合は、mStrReplaceChar_null() を使うこと。 */

void mStrReplaceChar(mStr *str,char from,char to)
{
	char *p;
	
	if(!str->buf) return;

	for(p = str->buf; *p; p++)
	{
		if(*p == from)
			*p = to;
	}
}

/**@ 指定文字をヌル文字に置換え
 *
 * @d:終端は常にヌル文字が２つ続くようにする。\
 *  文字長さには、ヌル文字も含まれる。*/

void mStrReplaceChar_null(mStr *str,char ch)
{
	if(mStrGetLastChar(str) != ch)
		mStrAppendChar(str, ch);
	
	mStrReplaceChar(str, ch, 0);
}

/**@ 指定文字で区切られた複数文字列の、指定位置の文字列を置換
 *
 * @p:split 区切り文字
 * @p:index 置き換える文字列のインデックス位置 (0〜)
 * @p:replace 置き換える文字列 */

void mStrReplaceSplitText(mStr *str,char split,int index,const char *replace)
{
	mStr copy = MSTR_INIT;
	int cnt;
	char *pc,*pend;

	mStrCopy_alloc(&copy, str);
	mStrEmpty(str);

	for(pc = copy.buf, cnt = 0; 1; cnt++)
	{
		//終端位置

		if(*pc)
			pend = mStringFindChar(pc, split);
		else
			pend = pc;

		//追加

		if(cnt == index)
			mStrAppendText(str, replace);
		else
			mStrAppendText_len(str, pc, pend - pc);

		//終了
		/* インデックス位置が文字列内の個数より多い場合は、
		 * 間を空文字列として追加 */

		if(!(*pend) && cnt >= index) break;

		//区切り文字追加

		mStrAppendChar(str, split);

		//次へ

		pc = (*pend)? pend + 1: pend;
	}

	mStrFree(&copy);
}

/**@ [指定文字+1文字] の部分を、任意の文字列に置き換える
 *
 * @d:"%f" など、任意の指定文字と後に続く1文字の部分を、任意の文字列に置き換える。
 *
 * @p:paramch '%' など、置き換えの対象となる先頭文字。\
 * この文字が2つ続くと、その文字1つに置き換える。
 * @p:reparray 置き換え情報を定義した文字列の配列。\
 * 各文字列の先頭文字は、paramch に続く2番目の文字。2文字目以降は、置き換える文字列をセットする。\
 * \
 * 例： "freplace" => %f を "replace" に置き換え
 * @p:arraynum reparray の配列数 */

void mStrReplaceParams(mStr *str,char paramch,mStr *reparray,int arraynum)
{
	mStr copy;
	char *ps,c;
	int i;

	//置き換え対象が空
	if(mStrIsEmpty(str)) return;

	mStrCopy_init(&copy, str);
	mStrEmpty(str);

	for(ps = copy.buf; *ps; )
	{
		c = *(ps++);
	
		if(c != paramch)
			//通常文字
			mStrAppendChar(str, c);
		else
		{
			c = *(ps++);

			if(!c)
				//パラメータ文字で終わっている
				break;
			else if(c == paramch)
				//パラメータ文字 x 2
				mStrAppendChar(str, c);
			else
			{
				//置換対象検索
				
				for(i = 0; i < arraynum; i++)
				{
					if(reparray[i].buf[0] == c) break;
				}

				if(i == arraynum)
					//見つからなかった場合
					ps--;
				else
					mStrAppendText(str, reparray[i].buf + 1);
			}
		}
	}

	mStrFree(&copy);
}

/**@ '\\' + 文字のエスケープをデコード
 *
 * @d:'\\' + 文字を、文字のみに変換。 */

void mStrDecodeEscape(mStr *str)
{
	char *pd,*ps;

	if(!str->buf) return;

	ps = pd = str->buf;

	while(*ps)
	{
		if(*ps == '\\')
		{
			ps++;
			if(!(*ps)) break;
		}

		*(pd++) = *(ps++);
	}

	*pd = 0;

	str->len = pd - str->buf;
}


//==================================
// 検索
//==================================


/**@ 先頭から文字を検索
 *
 * @r:文字の位置 (バイト数)。-1 で見つからなかった。 */

int mStrFindChar(mStr *str,char ch)
{
	char *p;

	if(!str->buf) return -1;

	p = strchr(str->buf, ch);

	return (p)? p - str->buf: -1;
}

/**@ 終端から文字を検索
 *
 * @r:文字の位置。-1 で見つからなかった。 */

int mStrFindChar_rev(mStr *str,char ch)
{
	char *pc;
	int pos;

	if(!str->buf) return -1;

	pos = str->len - 1;

	for(pc = str->buf + pos; pos >= 0 && *pc != ch; pos--, pc--); 

	return pos;
}

/**@ 先頭から指定文字を検索し、その位置で文字列を終わらせる */

void mStrFindChar_toend(mStr *str,char ch)
{
	int pos = mStrFindChar(str, ch);

	if(pos != -1)
		mStrSetLen(str, pos);
}


//==================================
// 比較
//==================================


/**@ 文字列比較
 *
 * @d:str と text の文字列を比較して、等しいかどうかを返す。\
 * NULL の場合は、空文字列とみなす。 */

mlkbool mStrCompareEq(mStr *str,const char *text)
{
	if(str->buf && text)
		return (strcmp(str->buf, text) == 0);
	else
		//いずれかが NULL の場合
		return ((!str->buf || !*(str->buf)) && (!text || !*text));
}

/**@ 文字列比較 (大/小文字区別しない) */

mlkbool mStrCompareEq_case(mStr *str,const char *text)
{
	if(str->buf && text)
		return (strcasecmp(str->buf, text) == 0);
	else
		return ((!str->buf || !*(str->buf)) && (!text || !*text));
}

/**@ 文字列の先頭から指定長さを比較 */

mlkbool mStrCompareEq_len(mStr *str,const char *text,int len)
{
	if(!str->buf || !text)
		return FALSE;
	else
		return (strncmp(str->buf, text, len) == 0);
}


//==================================
// パス
//==================================


/**@ パスがトップの状態かどうか
 *
 * @g:パス
 * 
 * @d:空文字列、もしくは "/" でトップとみなす。 */

mlkbool mStrPathIsTop(mStr *str)
{
	return (mStrIsEmpty(str)
		|| (str->buf[0] == MLK_DIRSEP && str->buf[1] == 0));
}

/**@ 終端にパス区切り文字を追加
 *
 * @d:空の場合や、終端にすでに区切りがある場合は何もしない。 */

void mStrPathAppendDirSep(mStr *str)
{
	if(str->len > 0 && str->buf[str->len - 1] != MLK_DIRSEP)
		mStrAppendChar(str, MLK_DIRSEP);
}

/**@ パスを結合する
 *
 * @d:現在のパスに区切り文字を追加後、指定パスを追加する。\
 * str が空の場合や、区切り文字で終わっている場合は、パス区切り文字は追加しない。*/

void mStrPathJoin(mStr *str,const char *path)
{
	mStrPathAppendDirSep(str);
	mStrAppendText(str, path);
}

/**@ パスを正規化
 *
 * @d:途中にある "." ".." を除去し、正確な絶対パスにする。 */

void mStrPathNormalize(mStr *str)
{
	char *buf,*dst;

	if(mStrIsEmpty(str)) return;

	buf = mUTF8toLocale(str->buf, str->len, NULL);
	if(!buf) return;

	dst = realpath(buf, NULL);

	mFree(buf);
	
	if(dst)
	{
		mStrSetText_locale(str, dst, -1);
		free(dst);
	}
}


//----------------


/**@ ホームディレクトリパスをセット */

void mStrPathSetHome(mStr *str)
{
	mStrSetText_locale(str, getenv("HOME"), -1);
}

/**@ ホームディレクトリに指定パスを結合してセット */

void mStrPathSetHome_join(mStr *str,const char *path)
{
	mStrPathSetHome(str);
	mStrPathJoin(str, path);
}

/**@ 先頭が "~" の場合、ホームディレクトリに置き換え */

void mStrPathReplaceHome(mStr *str)
{
	if(str->len > 0 && str->buf[0] == '~')
	{
		mStr tmp = MSTR_INIT;

		mStrSetText_len(&tmp, str->buf + 1, str->len - 1);
		mStrPathSetHome(str);
		mStrAppendStr(str, &tmp);

		mStrFree(&tmp);
	}
}

/**@ デフォルトの作業用ディレクトリをセット */

void mStrPathSetTempDir(mStr *str)
{
	mStrSetText(str, "/tmp");
}


//----------------


/**@ 終端のパス区切り文字を除去する
 *
 * @d:文字列が区切り文字1つのみの場合は、何もしない。 */

void mStrPathRemoveBottomDirSep(mStr *str)
{
	if(str->len > 1 && str->buf[str->len - 1] == MLK_DIRSEP)
		mStrSetLen(str, str->len - 1);
}

/**@ パスから下位部分を取り除いて、ディレクトリ名のみにする
 *
 * @d:ディレクトリ部分がない場合は、空になる。\
 * "/name" や "/" の場合は、"/" となる。 */

void mStrPathRemoveBasename(mStr *str)
{
	int pos;

	mStrPathRemoveBottomDirSep(str);

	pos = mStrFindChar_rev(str, MLK_DIRSEP);

	if(pos == -1)
		mStrEmpty(str);
	else if(pos == 0)
		//"/..." の場合、"/" にする
		mStrSetLen(str, 1);
	else
		mStrSetLen(str, pos);
}

/**@ 拡張子部分を取り除く
 *
 * @d:ファイル名部分の一番最後の "." 以降を取り除く。\
 * ファイル名の先頭が "." の場合は、拡張子として扱わない。 */

void mStrPathRemoveExt(mStr *str)
{
	int pos;
	char *pc;

	if(!str->buf) return;

	pos = mStrFindChar_rev(str, MLK_DIRSEP);
	if(pos == -1)
		pos = 0;
	else
		pos++;

	//ファイル名部分の最後の '.' を検索

	pc = strrchr(str->buf + pos, '.');

	if(pc && pc != str->buf + pos)
		mStrSetLen(str, pc - str->buf);
}

/**@ ファイル名として無効な文字を置き換える
 *
 * @d:'/' はファイル名として使えない。\
 * ※ str にディレクトリ部分は含めないようにすること。
 * @p:rep 置き換える文字 */

void mStrPathReplaceDisableChar(mStr *str,char rep)
{
	mStrReplaceChar(str, MLK_DIRSEP, rep);
}

/**@ パスからディレクトリ部分のみを取得
 *
 * @d:ディレクトリ部分がない場合は空。 */

void mStrPathGetDir(mStr *dst,const char *path)
{
	int pos;

	pos = _get_path_seppos(path);

	if(pos == -1)
		mStrEmpty(dst);
	else
		mStrSetText_len(dst, path, pos);
}

/**@ ディレクトリ部分を除いたファイル名部分を取得 */

void mStrPathGetBasename(mStr *dst,const char *path)
{
	int pos;

	pos = _get_path_seppos(path);

	if(pos == -1)
		mStrSetText(dst, path);
	else
		mStrSetText(dst, path + pos + 1);
}

/**@ 拡張子を除いたファイル名のみ取得してセット
 *
 * @d:先頭が "." の場合は、拡張子として扱わない。 */

void mStrPathGetBasename_noext(mStr *dst,const char *path)
{
	int pos;

	mStrPathGetBasename(dst, path);

	//拡張子除去 (先頭が '.' の場合は除く)

	pos = mStrFindChar_rev(dst, '.');

	if(pos > 0)
		mStrSetLen(dst, pos);
}

/**@ 拡張子を取得
 *
 * @d:先頭が "." の場合は、拡張子として扱わないため、空となる。 */

void mStrPathGetExt(mStr *dst,const char *path)
{
	char *pcfname,*pc;

	pcfname = _get_path_fname(path);

	//最後の '.' を検索

	pc = strrchr(pcfname, '.');

	if(!pc || pc == pcfname)
		mStrEmpty(dst);
	else
		mStrSetText(dst, pc + 1);
}

/**@ パスをディレクトリとファイル名部分に分割して取得
 *
 * @p:dstdir ディレクトリ部分が格納される (最後のパス区切り文字も含む)
 * @p:dstname ファイル名部分が格納される */

void mStrPathSplit(mStr *dstdir,mStr *dstname,const char *path)
{
	int pos;

	pos = _get_path_seppos(path);

	if(pos == -1)
	{
		//ファイル名のみ
		mStrEmpty(dstdir);
		mStrSetText(dstname, path);
	}
	else
	{
		mStrSetText_len(dstdir, path, pos + 1);
		mStrSetText(dstname, path + pos + 1);
	}
}

/**@ パスを拡張子の前と後に分割して取得
 *
 * @p:dstpath 拡張子より前の部分が格納される
 * @p:dstext 拡張子部分が格納される。NULL 指定可能。\
 * 文字列内には '.' も含まれる。\
 * '.' がファイル名先頭に一つのみの場合は、拡張子はないものとして扱われる。 */

void mStrPathSplitExt(mStr *dstpath,mStr *dstext,const char *path)
{
	char *pcfname,*pc;

	pcfname = _get_path_fname(path);

	//'.' 検索

	pc = strrchr(pcfname, '.');

	if(!pc || pc == pcfname)
	{
		//'.' がない or 先頭が'.' => 拡張子なし

		mStrSetText(dstpath, path);
		if(dstext) mStrEmpty(dstext);
	}
	else
	{
		mStrSetText_len(dstpath, path, pc - path);
		if(dstext) mStrSetText(dstext, pc);
	}
}

/**@ パスに拡張子を追加する
 *
 * @d:'.' + ext が終端に追加される。
 *
 * @p:ext NULL または空文字列の場合は、何もしない */

void mStrPathAppendExt(mStr *str,const char *ext)
{
	if(ext && *ext)
	{
		mStrAppendChar(str, '.');
		mStrAppendText(str, ext);
	}
}

/**@ 拡張子を変更またはセットする
 *
 * @d:すでに同じ拡張子がある場合はそのまま。\
 * (拡張子は大文字小文字は区別されない)\
 * 拡張子がない、または拡張子が異なる場合は、拡張子を追加・変更する。
 * @r:拡張子が追加・変更された場合は TRUE */

mlkbool mStrPathSetExt(mStr *str,const char *ext)
{
	if(mStrPathCompareExtEq(str, ext))
		return FALSE;
	else
	{
		mStrPathRemoveExt(str);
		mStrPathAppendExt(str, ext);
		
		return TRUE;
	}
}

/**@ ディレクトリ・ファイル名・拡張子を結合したパスをセット
 *
 * @p:ext NULL で拡張子なし。'.' は含まない。 */

void mStrPathCombine(mStr *dst,const char *dir,const char *fname,const char *ext)
{
	mStrSetText(dst, dir);
	mStrPathJoin(dst, fname);
	mStrPathAppendExt(dst, ext);
}

/**@ ディレクトリ・ファイル名の前の接頭語・ファイル名・拡張子を結合したパスをセット
 *
 * @p:prefix NULL でなし
 * @p:ext NULL で拡張子なし。'.' は含まない。 */

void mStrPathCombine_prefix(mStr *dst,const char *dir,const char *prefix,const char *fname,const char *ext)
{
	mStrSetText(dst, dir);

	if(prefix)
	{
		mStrPathJoin(dst, prefix);
		mStrAppendText(dst, fname);
	}
	else
		mStrPathJoin(dst, fname);

	mStrPathAppendExt(dst, ext);
}

/**@ 入力ファイル名を元に、出力ファイル名を取得
 *
 * @p:outdir 出力先のディレクトリ。NULL または空で、入力先と同じ場所。
 * @p:outext 出力ファイル名の拡張子。'.' は含まない。NULL で入力と同じ。*/

void mStrPathGetOutputFile(mStr *dst,const char *infile,const char *outdir,const char *outext)
{
	mStr fname = MSTR_INIT;

	//出力先ディレクトリ

	if(outdir && *outdir)
		mStrSetText(dst, outdir);
	else
		mStrPathGetDir(dst, infile);

	//ファイル名

	if(!outext)
		mStrPathGetBasename(&fname, infile);
	else
	{
		mStrPathGetBasename_noext(&fname, infile);
		mStrPathAppendExt(&fname, outext);
	}

	//結合

	mStrPathJoin(dst, fname.buf);

	mStrFree(&fname);
}

/**@ 入力ファイル名を元に、出力ファイル名を取得 (接尾後付き)
 *
 * @d:[DIR]/[INPUTNAME][SUFFIX].[EXT]
 * 
 * @p:outdir 出力先のディレクトリ。NULL または空で、入力先と同じ場所。
 * @p:outext 出力ファイル名の拡張子。NULL で入力と同じ。
 * @p:suffix ファイル名の後に付ける接尾語文字列 */

void mStrPathGetOutputFile_suffix(mStr *dst,const char *infile,
	const char *outdir,const char *outext,const char *suffix)
{
	mStr fname = MSTR_INIT,ext = MSTR_INIT;

	//出力先ディレクトリ

	if(outdir && *outdir)
		mStrSetText(dst, outdir);
	else
		mStrPathGetDir(dst, infile);

	//ファイル名

	mStrPathGetBasename_noext(&fname, infile);
	mStrAppendText(&fname, suffix);

	//拡張子

	mStrAppendChar(&fname, '.');

	if(outext)
		mStrAppendText(&fname, outext);
	else
	{
		mStrPathGetExt(&ext, infile);
		mStrAppendStr(&fname, &ext);
		mStrFree(&ext);
	}

	//結合

	mStrPathJoin(dst, fname.buf);

	mStrFree(&fname);
}

/**@ パスが同じか比較
 *
 * @d:unix の場合、大文字小文字は区別されるので、通常の文字列比較と同じ。
 *
 * @r:str が未確保、または path == NULL の場合は FALSE が返る。 */

mlkbool mStrPathCompareEq(mStr *str,const char *path)
{
	if(str->buf && path)
		return (strcmp(str->buf, path) == 0);
	else
		return FALSE;
}

/**@ パスの拡張子を比較
 *
 * @d:str の終端が ".ext" かどうかを比較する。\
 * 大文字小文字は区別しない。
 * @p:ext 拡張子文字列。"." は含まない。 */

mlkbool mStrPathCompareExtEq(mStr *str,const char *ext)
{
	mStr cmp = MSTR_INIT;
	mlkbool ret;

	if(!str->buf || !ext) return FALSE;

	//比較文字列作成

	mStrSetChar(&cmp, '.');
	mStrAppendText(&cmp, ext);

	//終端比較

	if(str->len < cmp.len)
		ret = FALSE;
	else
		ret = (strcasecmp(str->buf + str->len - cmp.len, cmp.buf) == 0);

	mStrFree(&cmp);

	return ret;
}

/**@ パスの拡張子を複数比較
 *
 * @d:str のパスの拡張子が、exts で定義された拡張子のいずれかと一致するか。
 * @p:exts 拡張子のリスト。':' で区切る。'.' は含めない。
 * @r:いずれかに一致すれば TRUE */

mlkbool mStrPathCompareExts(mStr *str,const char *exts)
{
	mStr tmp = MSTR_INIT;
	const char *ptop,*pend;
	mlkbool ret = FALSE;

	if(!str->buf || !exts) return FALSE;

	ptop = exts;

	while(mStringGetNextSplit(&ptop, &pend, ':'))
	{
		//比較文字列

		mStrSetChar(&tmp, '.');
		mStrAppendText_len(&tmp, ptop, pend - ptop);

		//終端比較

		if(str->len >= tmp.len
			&& strcasecmp(str->buf + str->len - tmp.len, tmp.buf) == 0)
		{
			ret = TRUE;
			break;
		}

		ptop = pend;
	}

	mStrFree(&tmp);

	return ret;
}

/**@ ディレクトリ部分を比較
 *
 * @p:dir 終端に余分なパス区切りは含めないこと */

mlkbool mStrPathCompareDir(mStr *str,const char *dir)
{
	mStr tmp = MSTR_INIT;
	mlkbool ret;

	mStrPathGetDir(&tmp, str->buf);

	ret = (strcmp(tmp.buf, dir) == 0);

	mStrFree(&tmp);

	return ret;
}

/**@ ファイルダイアログの複数選択文字列から、各ファイルのフルパスを取り出す
 *
 * @d:戻り値が FALSE になるまで続けること。
 * @p:tmp1,tmp2 作業用 (開始時は NULL を入れておく)
 * @r:FALSE ですべて取り出した */

mlkbool mStrPathExtractMultiFiles(mStr *dst,const char *text,const char **tmp1,const char **tmp2)
{
	const char *ptop,*pend,*dirend;

	//ディレクトリ部分の終端

	if(*tmp1)
		dirend = *tmp1;
	else
	{
		dirend = strchr(text, '\t');
		if(!dirend) return FALSE;

		*tmp1 = dirend;
	}

	//ファイル名先頭

	if(*tmp2)
		ptop = *tmp2;
	else
		ptop = dirend + 1;

	if(*ptop == 0) return FALSE;

	//終端

	pend = strchr(ptop, '\t');
	if(!pend) return FALSE;

	//パスセット

	mStrSetText_len(dst, text, dirend - text);
	mStrPathAppendDirSep(dst);
	mStrAppendText_len(dst, ptop, pend - ptop);

	//次の開始位置

	*tmp2 = pend + 1;

	return TRUE;
}


//===============================
// 配列
//===============================


/**@ mStr の配列をすべて解放
 *
 * @g:配列
 *
 * @p:num 配列数 */

void mStrArrayFree(mStr *p,int num)
{
	int i;

	for(i = 0; i < num; i++)
		mStrFree(p + i);
}

/**@ mStr 配列を初期化 */

void mStrArrayInit(mStr *p,int num)
{
	memset(p, 0, sizeof(mStr) * num);
}

/**@ mStr 配列の各文字列をすべてコピー
 *
 * @d:mStrCopy() を使う。 */

void mStrArrayCopy(mStr *dst,mStr *src,int num)
{
	int i;

	for(i = 0; i < num; i++)
		mStrCopy(dst + i, src + i);
}

/**@ mStr 配列の内容を先頭方向に1つシフトする
 * 
 * @d:start のデータは削除され、以降は p[i] = p[i + 1] というように、
 * mStr のデータがそのままコピーされる。\
 * end は空となる。
 * @p:start 配列の先頭位置
 * @p:end   配列の終端位置 */

void mStrArrayShiftUp(mStr *p,int start,int end)
{
	int i;

	mStrFree(p + start);

	for(i = start; i < end; i++)
		p[i] = p[i + 1];

	/* 上記で end の値そのものをコピーしているので、
	 * end - 1 と end のデータは同じになっている。
	 * end は解放してはいけないので、値を初期化して空にする。 */
	
	mStrInit(p + end);
}

/**@ 履歴として扱っている配列に、新しい文字列をセット
 *
 * @d:各文字列が同じ項目かどうかの判定は独自で行い、その後配列内にセットしたい時に使う。\
 * text が array 内の文字列のポインタでも問題ない。
 *
 * @p:arraynum 配列の数
 * @p:index 負の値で、text を新規追加する。終端の値を削除して、先頭に text を挿入する。\
 * 正の値で、その位置のデータを先頭へ移動して、先頭に text をセット。
 * @p:text セットする文字列 */

void mStrArraySetRecent(mStr *p,int arraynum,int index,const char *text)
{
	mStr str = MSTR_INIT;
	int i;

	//新規時は終端

	if(index < 0)
		index = arraynum - 1;
	
	//新規文字列

	mStrSetText(&str, text);

	//削除

	mStrFree(p + index);

	//先頭を空ける

	for(i = index; i > 0; i--)
		p[i] = p[i - 1];

	//先頭にセット (str をそのままセット)

	p[0] = str;
}

/**@ 履歴として扱っている配列に、新しい文字列を追加
 *
 * @d:文字列は大文字小文字の区別ありで比較され、
 * 配列内に同じ文字列があれば、それを先頭へと移動する。\
 * 配列内に同じ文字列がなければ、新規追加とする。\
 * text が 配列内の文字列のポインタでも問題ない。 */

void mStrArrayAddRecent(mStr *p,int arraynum,const char *text)
{
	int i,find;
	mlkbool ret;

	//配列内に同じ文字列があるか

	for(i = 0, find = -1; i < arraynum; i++)
	{
		ret = mStrCompareEq(p + i, text);
	
		if(ret)
		{
			find = i;
			break;
		}
	}

	//セット

	mStrArraySetRecent(p, arraynum, find, text);
}

