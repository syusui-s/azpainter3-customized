/*$
 Copyright (C) 2013-2020 Azel.

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

#include "mDef.h"

#include "mFontConfig.h"
#include "mFont.h"
#include "mStr.h"


/**
@defgroup fontconfig mFontConfig
@brief fontconfig ユーティリティ

@details
文字列は UTF-8 で指定する。

@ingroup group_lib
 
@{
@file mFontConfig.h

@struct mFontInfo

*/


//================================
// sub
//================================


/** mFontInfo から FcPattern 作成 */

static FcPattern *_create_fc_pattern(mFontInfo *p)
{
	FcPattern *pat;
	int weight,slant;
	
	pat = FcPatternCreate();

	//フォントファミリー

	if((p->mask & MFONTINFO_MASK_FAMILY) && !mStrIsEmpty(&p->strFamily))
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)p->strFamily.buf);
	else
		FcPatternAddString(pat, FC_FAMILY, (FcChar8 *)"sans-serif");

	//スタイル

	if((p->mask & MFONTINFO_MASK_STYLE) && !mStrIsEmpty(&p->strStyle))
		FcPatternAddString(pat, FC_STYLE, (FcChar8 *)p->strStyle.buf);
	
	//サイズ

	if(p->mask & MFONTINFO_MASK_SIZE)
	{
		if(p->size < 0)
			FcPatternAddDouble(pat, FC_PIXEL_SIZE, -(p->size));
		else
			FcPatternAddDouble(pat, FC_SIZE, p->size);
	}

	//太さ (指定なしなら通常)

	weight = FC_WEIGHT_NORMAL;
	
	if(p->mask & MFONTINFO_MASK_WEIGHT)
	{
		switch(p->weight)
		{
			case MFONTINFO_WEIGHT_BOLD:
				weight = FC_WEIGHT_BOLD;
				break;
		}
	}

	FcPatternAddInteger(pat, FC_WEIGHT, weight);
	
	//斜体 (指定なしなら通常)

	slant = FC_SLANT_ROMAN;
	
	if(p->mask & MFONTINFO_MASK_SLANT)
	{
		switch(p->slant)
		{
			case MFONTINFO_SLANT_ITALIC:
				slant = FC_SLANT_ITALIC;
				break;
			case MFONTINFO_SLANT_OBLIQUE:
				slant = FC_SLANT_OBLIQUE;
				break;
		}
	}
		
	FcPatternAddInteger(pat, FC_SLANT, slant);
	
	//デフォルトの構成を適用

	if(!FcConfigSubstitute(NULL, pat, FcMatchPattern))
	{
		FcPatternDestroy(pat);
		return NULL;
	}
	
	return pat;
}


//================================
//
//================================


/** fontconfig 初期化 */

mBool mFontConfigInit(void)
{
	return FcInit();
}

/** fontconfig 解放 */

void mFontConfigEnd(void)
{
	FcFini();
}

/** パターンを解放 */

void mFontConfigPatternFree(mFcPattern *pat)
{
	if(pat)
		FcPatternDestroy(pat);
}

/** 指定した構成にマッチするパターン取得 */

mFcPattern *mFontConfigMatch(mFontInfo *conf)
{
	FcPattern *pat,*pat_match;
	FcResult ret;
	
	pat = _create_fc_pattern(conf);
	if(!pat) return NULL;
	
	pat_match = FcFontMatch(0, pat, &ret);
	
	FcPatternDestroy(pat);
		
	return pat_match;
}

/** パターンからファイル情報取得
 * 
 * @param file pat の領域内のポインタが返る。pat を解放するまでは有効 */

int mFontConfigGetFileInfo(mFcPattern *pat,char **file,int *index)
{
	*file  = NULL;
	*index = 0;

	if(FcPatternGetString(pat, FC_FILE, 0, (FcChar8 **)file) != FcResultMatch)
		return -1;
	
	FcPatternGetInteger(pat, FC_INDEX, 0, index);
	
	return 0;
}

/** パターンから文字列取得
 *
 * @return ポインタは解放しないこと。NULL で失敗。 */

char *mFontConfigGetText(mFcPattern *pat,const char *object)
{
	FcChar8 *text;

	if(FcPatternGetString(pat, object, 0, &text) == FcResultMatch)
		return (char *)text;
	else
		return NULL;
}

/** パターンから bool 値取得 */

mBool mFontConfigGetBool(mFcPattern *pat,const char *object,mBool def)
{
	FcBool ret;

	if(FcPatternGetBool(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/** パターンから int 値取得 */

int mFontConfigGetInt(mFcPattern *pat,const char *object,int def)
{
	int ret;

	if(FcPatternGetInteger(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/** パターンから double 値取得 */

double mFontConfigGetDouble(mFcPattern *pat,const char *object,double def)
{
	double ret;

	if(FcPatternGetDouble(pat, object, 0, &ret) == FcResultMatch)
		return ret;
	else
		return def;
}

/** パターンから double 値取得 */

mBool mFontConfigGetDouble2(mFcPattern *pat,const char *object,double *dst)
{
	return (FcPatternGetDouble(pat, object, 0, dst) == FcResultMatch);
}

/** パターンから matrix 取得 */

mBool mFontConfigGetMatrix(mFcPattern *pat,const char *object,double *matrix)
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

/** @} */
