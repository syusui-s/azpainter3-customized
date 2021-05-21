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
 * mFontInfo
 *****************************************/

#include "mlk.h"
#include "mlk_fontinfo.h"
#include "mlk_str.h"
#include "mlk_textparam.h"


//ファイルパス内に ';' が含まれる可能性があるため、\ でエスケープ


/**@ 初期化 */

void mFontInfoInit(mFontInfo *p)
{
	mMemset0(p, sizeof(mFontInfo));
}

/**@ mFontInfo のメンバを解放 */

void mFontInfoFree(mFontInfo *p)
{
	mStrFree(&p->str_family);
	mStrFree(&p->str_style);
	mStrFree(&p->str_file);
}

/**@ mFontInfo のデータをコピー
 *
 * @p:dst 現在の値を解放した上で、セットされる。\
 * 新規状態の場合は、ゼロクリアしておくこと。 */

void mFontInfoCopy(mFontInfo *dst,const mFontInfo *src)
{
	mFontInfoFree(dst);

	*dst = *src;

	//mStr を初期化してコピー
	mStrCopy_init(&dst->str_family, &src->str_family);
	mStrCopy_init(&dst->str_style, &src->str_style);
	mStrCopy_init(&dst->str_file, &src->str_file);
}

/**@ 文字列フォーマットから mFontInfo をセット
 *
 * @d:※dst はあらかじめ初期化しておくこと。\
 * また、使用後は mFontInfoFree() で解放すること。
 *
 * @p:format "type1=value1;type2=value2;..."。\
 *  NULL の場合は mask=0 となる。\
 * \
 * @tbl>
 * |||file=[text]||フォントファイルパス。\
 * パス名に ';' '\\' が含まれる場合は、'\\' でエスケープ。||
 * |||index=[int]||file 指定時、フォントファイル内のインデックス番号 (default=0)||
 * |||family=[text]||ファミリ名||
 * |||style=[text]||スタイル名||
 * |||size=[double]||フォントサイズ。\
 * 正の値で pt 単位、負の値で px 単位。||
 * |||weight=[int]||太さの数値||
 * |||slant=[int]||傾きの数値||
 * |||dpi=[int]||解像度 (DPI)。\
 * フォントサイズが pt 単位の場合に使われる。||
 * |||hinting=[int]||ヒンティング||
 * |||rendering=[int]||レンダリング||
 * |||lcdfilter=[int]||LCD フィルター||
 * |||autohint=[0 or 1]||オートヒンティングあり/なし||
 * |||embeddedbitmap=[0 or 1]||埋め込みビットマップ有効/無効||
 * @tbl<
 */

void mFontInfoSetFromText(mFontInfo *dst,const char *format)
{
	mTextParam *tp;
	int n;

	dst->mask = 0;

	if(!format) return;

	//mTextParam 作成

	tp = mTextParam_new(format, ';', '=');
	if(!tp) return;

	//ファイル

	if(mTextParam_getTextStr(tp, "file", &dst->str_file))
	{
		dst->mask |= MFONTINFO_MASK_FILE;
		dst->index = 0;

		//index

		if(mTextParam_getInt(tp, "index", &n))
			dst->index = n;
	}

	//ファミリ

	if(mTextParam_getTextStr(tp, "family", &dst->str_family))
		dst->mask |= MFONTINFO_MASK_FAMILY;

	//スタイル

	if(mTextParam_getTextStr(tp, "style", &dst->str_style))
		dst->mask |= MFONTINFO_MASK_STYLE;

	//サイズ

	if(mTextParam_getDouble(tp, "size", &dst->size))
		dst->mask |= MFONTINFO_MASK_SIZE;

	//太さ

	if(mTextParam_getInt(tp, "weight", &n))
	{
		dst->mask |= MFONTINFO_MASK_WEIGHT;
		dst->weight = n;
	}

	//傾き

	if(mTextParam_getInt(tp, "slant", &n))
	{
		dst->mask |= MFONTINFO_MASK_SLANT;
		dst->slant = n;
	}

	//dpi

	if(mTextParam_getInt(tp, "dpi", &n))
	{
		dst->mask |= MFONTINFO_MASK_DPI;
		dst->dpi = n;
	}

	//-------- ex

	//ヒンティング

	if(mTextParam_getInt(tp, "hinting", &n))
	{
		dst->mask |= MFONTINFO_MASK_EX;
		dst->ex.mask |= MFONTINFO_EX_MASK_HINTING;
		dst->ex.hinting = n;
	}

	//レンダリング

	if(mTextParam_getInt(tp, "rendering", &n))
	{
		dst->mask |= MFONTINFO_MASK_EX;
		dst->ex.mask |= MFONTINFO_EX_MASK_RENDERING;
		dst->ex.rendering = n;
	}

	//LCD フィルター

	if(mTextParam_getInt(tp, "lcdfilter", &n))
	{
		dst->mask |= MFONTINFO_MASK_EX;
		dst->ex.mask |= MFONTINFO_EX_MASK_LCD_FILTER;
		dst->ex.lcd_filter = n;
	}

	//フラグ (オートヒント)

	if(mTextParam_getInt(tp, "autohint", &n))
	{
		dst->mask |= MFONTINFO_MASK_EX;
		dst->ex.mask |= MFONTINFO_EX_MASK_AUTO_HINT;
		if(n) dst->ex.flags |= MFONTINFO_EX_FLAGS_AUTO_HINT;
	}

	//フラグ (埋め込みビットマップ)

	if(mTextParam_getInt(tp, "embeddedbitmap", &n))
	{
		dst->mask |= MFONTINFO_MASK_EX;
		dst->ex.mask |= MFONTINFO_EX_MASK_EMBEDDED_BITMAP;
		if(n) dst->ex.flags |= MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP;
	}

	mTextParam_free(tp);
}

/**@ mFontInfo から文字列フォーマットを取得 */

void mFontInfoToText(mStr *str,const mFontInfo *info)
{
	uint32_t mask;

	mStrEmpty(str);

	mask = info->mask;

	//ファイル名

	if((mask & MFONTINFO_MASK_FILE) && mStrIsnotEmpty(&info->str_file))
	{
		mStrAppendText(str, "file=");
		mStrAppendText_escapeChar(str, info->str_file.buf, ";");
		mStrAppendFormat(str, ";index=%d;", info->index);
	}

	//ファミリ

	if((mask & MFONTINFO_MASK_FAMILY) && mStrIsnotEmpty(&info->str_family))
		mStrAppendFormat(str, "family=%t;", &info->str_family);

	//スタイル

	if((mask & MFONTINFO_MASK_STYLE) && mStrIsnotEmpty(&info->str_style))
		mStrAppendFormat(str, "style=%t;", &info->str_style);

	//サイズ

	if(mask & MFONTINFO_MASK_SIZE)
	{
		mStrAppendText(str, "size=");
		mStrAppendDouble(str, info->size, 1);
		mStrAppendChar(str, ';');
	}

	//太さ

	if(mask & MFONTINFO_MASK_WEIGHT)
		mStrAppendFormat(str, "weight=%d;", info->weight);

	//傾き

	if(mask & MFONTINFO_MASK_SLANT)
		mStrAppendFormat(str, "slant=%d;", info->slant);

	//dpi

	if(mask & MFONTINFO_MASK_DPI)
		mStrAppendFormat(str, "dpi=%d;", info->dpi);

	//------ ex

	if(!(mask & MFONTINFO_MASK_EX)) return;

	mask = info->ex.mask;

	//ヒンティング

	if(mask & MFONTINFO_EX_MASK_HINTING)
		mStrAppendFormat(str, "hinting=%d;", info->ex.hinting);

	//レンダリング

	if(mask & MFONTINFO_EX_MASK_RENDERING)
		mStrAppendFormat(str, "rendering=%d;", info->ex.rendering);

	//LCD フィルター

	if(mask & MFONTINFO_EX_MASK_LCD_FILTER)
		mStrAppendFormat(str, "lcdfilter=%d;", info->ex.lcd_filter);

	//フラグ (オートヒント)

	if(mask & MFONTINFO_EX_MASK_AUTO_HINT)
		mStrAppendFormat(str, "autohint=%d;", ((info->ex.flags & MFONTINFO_EX_FLAGS_AUTO_HINT) != 0));

	//フラグ (埋め込みビットマップ)

	if(mask & MFONTINFO_EX_MASK_EMBEDDED_BITMAP)
		mStrAppendFormat(str, "embeddedbitmap=%d;", ((info->ex.flags & MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP) != 0));
}

/**@ フォント情報から、ファミリ名またはファイル名とサイズを、文字列にして取得 */

void mFontInfoGetText_family_size(mStr *str,const mFontInfo *info)
{
	int n;

	//ファミリ名/ファイル名

	if(info->mask & MFONTINFO_MASK_FILE)
		mStrPathGetBasename(str, info->str_file.buf);
	else if(!(info->mask & MFONTINFO_MASK_FAMILY) || mStrIsEmpty(&info->str_family))
		mStrSetText(str, "(default)");
	else
		mStrCopy(str, &info->str_family);

	//サイズ

	if(info->mask & MFONTINFO_MASK_SIZE)
	{
		mStrAppendText(str, ", ");

		if(info->size < 0)
			//px
			mStrAppendFormat(str, "%dpx", (int)(-(info->size)));
		else
		{
			//pt
			
			n = (int)(info->size * 10 + 0.5);

			if(n % 10 == 0)
				mStrAppendFormat(str, "%dpt", n / 10);
			else
				mStrAppendFormat(str, "%.1Fpt", n);
		}
	}
}

/**@ フォントのファミリ名とスタイル、またはファイル名をコピーする */

void mFontInfoCopyName(mFontInfo *dst,const mFontInfo *src)
{
	//dst の指定値を空に

	dst->mask &= ~(MFONTINFO_MASK_FAMILY | MFONTINFO_MASK_STYLE | MFONTINFO_MASK_FILE
		| MFONTINFO_MASK_WEIGHT | MFONTINFO_MASK_SLANT);

	dst->index = 0;

	mStrFree(&dst->str_family);
	mStrFree(&dst->str_style);
	mStrFree(&dst->str_file);

	//コピー

	if(src->mask & MFONTINFO_MASK_FILE)
	{
		dst->mask |= MFONTINFO_MASK_FILE;
		dst->index = src->index;
		
		mStrCopy(&dst->str_file, &src->str_file);
	}
	else if(src->mask & MFONTINFO_MASK_FAMILY)
	{
		dst->mask |= MFONTINFO_MASK_FAMILY;

		mStrCopy(&dst->str_family, &src->str_family);

		//スタイル

		if(src->mask & MFONTINFO_MASK_STYLE)
		{
			dst->mask |= MFONTINFO_MASK_STYLE;

			mStrCopy(&dst->str_style, &src->str_style);
		}
	}
}

/**@ 埋め込みビットマップの指定がない場合、デフォルトとして値をセット */

void mFontInfoSetDefault_embeddedBitmap(mFontInfo *p,int is_true)
{
	if(!(p->mask & MFONTINFO_MASK_EX)
		|| !(p->ex.mask & MFONTINFO_EX_MASK_EMBEDDED_BITMAP))
	{
		p->mask |= MFONTINFO_MASK_EX;
		p->ex.mask |= MFONTINFO_EX_MASK_EMBEDDED_BITMAP;

		if(is_true)
			p->ex.flags |= MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP;
		else
			p->ex.flags &= ~MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP;
	}
}
