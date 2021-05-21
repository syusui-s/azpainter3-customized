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
 * fontconfig
 *****************************************/

#include <fontconfig/fontconfig.h>

#include "mlk.h"
#include "mlk_fontconfig.h"
#include "mlk_fontinfo.h"
#include "mlk_str.h"


#define _PUT_DEBUG 0


//================================
// sub
//================================


/* mFontInfo から FcPattern 作成 */

static FcPattern *_create_pattern_info(const mFontInfo *p)
{
	FcPattern *pat;
	uint32_t mask;
	int n,fstyle = FALSE;
	
	pat = FcPatternCreate();
	if(!pat) return NULL;

	mask = p->mask;

	//ファミリ名

	if((mask & MFONTINFO_MASK_FAMILY) && mStrIsnotEmpty(&p->str_family))
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)p->str_family.buf);
	else
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)"sans-serif");

	//スタイル

	if((mask & MFONTINFO_MASK_STYLE) && mStrIsnotEmpty(&p->str_style))
	{
		FcPatternAddString(pat, FC_STYLE, (FcChar8 *)p->str_style.buf);
		fstyle = TRUE;
	}
	
	//サイズ

	if(mask & MFONTINFO_MASK_SIZE)
	{
		if(p->size < 0)
			//px 単位
			FcPatternAddDouble(pat, FC_PIXEL_SIZE, -(p->size));
		else
			//pt 単位
			FcPatternAddDouble(pat, FC_SIZE, p->size);
	}

	//太さ (指定なしなら通常)

	if(mask & MFONTINFO_MASK_WEIGHT)
		n = p->weight;
	else
		n = (fstyle)? -1: MFONTINFO_WEIGHT_REGULAR;

	if(n != -1)
		FcPatternAddInteger(pat, FC_WEIGHT, n);
	
	//斜体 (指定なしなら通常)

	if(mask & MFONTINFO_MASK_SLANT)
		n = p->slant;
	else
		n = (fstyle)? -1: MFONTINFO_SLANT_ROMAN;

	if(n != -1)
		FcPatternAddInteger(pat, FC_SLANT, n);

	//debug
	
#if _PUT_DEBUG
	mDebug("--- raw ---\n");
	FcPatternPrint(pat);
#endif

	//pat に対して、デフォルトの構成を適用

	if(!FcConfigSubstitute(NULL, pat, FcMatchPattern))
	{
		FcPatternDestroy(pat);
		return NULL;
	}

	return pat;
}

/* ファミリ名とスタイルから FcPattern 作成
 *
 * family: NULL または空で "sans-serif" */

static FcPattern *_create_pattern_family(const char *family,const char *style)
{
	FcPattern *pat;
	
	pat = FcPatternCreate();
	if(!pat) return NULL;

	//ファミリ名

	if(!family || !(*family))
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)"sans-serif");
	else
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)family);

	//スタイル

	if(style && *style)
		FcPatternAddString(pat, FC_STYLE, (FcChar8 *)style);

	//pat に対して、デフォルトの構成を適用

	if(!FcConfigSubstitute(NULL, pat, FcMatchPattern))
	{
		FcPatternDestroy(pat);
		return NULL;
	}

	return pat;
}

/* FcPattern から、マッチするパターンを取得
 *
 * pat は削除される。 */

static FcPattern *_match_pattern(FcPattern *pat)
{
	FcPattern *pat_match;
	FcResult ret;
	
#if _PUT_DEBUG
	mDebug("--- substitute ---\n");
	FcPatternPrint(pat);
#endif
	
	pat_match = FcFontMatch(0, pat, &ret);
	
	FcPatternDestroy(pat);

#if _PUT_DEBUG
	if(pat_match)
	{
		mDebug("--- match ---\n");
		FcPatternPrint(pat_match);
	}
#endif

	return pat_match;
}


//================================
//
//================================


/**@ fontconfig 初期化 */

mlkbool mFontConfig_init(void)
{
	return FcInit();
}

/**@ fontconfig 終了 */

void mFontConfig_finish(void)
{
	FcFini();
}

/**@ 指定したフォント構成にマッチするパターンを取得
 *
 * @r:NULL で失敗 */

mFcPattern mFontConfig_matchFontInfo(const mFontInfo *info)
{
	FcPattern *pat;
	
	pat = _create_pattern_info(info);
	if(!pat) return NULL;

	return _match_pattern(pat);
}

/**@ ファミリ名からマッチするパターンを取得 */

mFcPattern mFontConfig_matchFamily(const char *family,const char *style)
{
	FcPattern *pat;
	
	pat = _create_pattern_family(family, style);
	if(!pat) return NULL;

	return _match_pattern(pat);
}

/**@ パターンを解放
 *
 * @p:pat NULL なら何もしない */

void mFCPattern_free(mFcPattern pat)
{
	if(pat)
		FcPatternDestroy(pat);
}

/**@ パターンからファイル情報取得
 * 
 * @p:file  ファイル名のポインタが格納される (UTF-8 文字列)。\
 * パターンの領域内のポインタが返るので、解放しないこと。\
 * パターンを解放するまでは有効。失敗時は NULL が入る。
 * @p:index ファイル内の、フォントのインデックス番号が格納される。
 * @r:TRUE で成功 */

mlkbool mFCPattern_getFile(mFcPattern pat,char **file,int *index)
{
	*file = NULL;
	*index = 0;

	if(FcPatternGetString(pat, FC_FILE, 0, (FcChar8 **)file) != FcResultMatch)
		return FALSE;
	
	FcPatternGetInteger(pat, FC_INDEX, 0, index);
	
	return TRUE;
}

/**@ パターンから文字列取得
 *
 * @p:object 値の名前
 * @r:文字列のポインタ (UTF-8)。\
 * パターン内の領域のポインタが返るため、解放しないこと。\
 * NULL で見つからなかった。 */

char *mFCPattern_getText(mFcPattern pat,const char *object)
{
	FcChar8 *text;

	if(FcPatternGetString(pat, object, 0, &text) == FcResultMatch)
		return (char *)text;
	else
		return NULL;
}

/**@ パターンから bool 値取得
 *
 * @p:def 値が見つからなかった場合のデフォルト値
 * @r:bool 値が返る */

mlkbool mFCPattern_getBool_def(mFcPattern pat,const char *object,mlkbool def)
{
	FcBool ret;

	if(FcPatternGetBool(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/**@ パターンから int 値取得 */

int mFCPattern_getInt_def(mFcPattern pat,const char *object,int def)
{
	int ret;

	if(FcPatternGetInteger(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/**@ パターンから double 値取得 */

double mFCPattern_getDouble_def(mFcPattern pat,const char *object,double def)
{
	double ret;

	if(FcPatternGetDouble(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/**@ パターンから double 値取得
 *
 * @p:dst 取得した値が格納される
 * @r:TRUE で見つかった */

mlkbool mFCPattern_getDouble(mFcPattern pat,const char *object,double *dst)
{
	return (FcPatternGetDouble(pat, object, 0, dst) == FcResultMatch);
}

/**@ パターンから matrix 取得
 *
 * @p:matrix double x 4 の値が格納される (0:xx, 1:xy, 2:yx, 3:yy)
 * @r:TRUE で見つかった */

mlkbool mFCPattern_getMatrix(mFcPattern pat,const char *object,double *matrix)
{
	FcMatrix *mt;
	
	if(FcPatternGetMatrix(pat, object, 0, &mt) != FcResultMatch)
		return FALSE;
	else
	{
		matrix[0] = mt->xx;
		matrix[1] = mt->xy;
		matrix[2] = mt->yx;
		matrix[3] = mt->yy;
		return TRUE;
	}
}

