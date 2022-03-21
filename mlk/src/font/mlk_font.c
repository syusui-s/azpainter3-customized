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
 * mFont (FreeType)
 *****************************************/

#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

#define MLK_FONT_FREETYPE_DEFINE

#include "mlk.h"
#include "mlk_font.h"
#include "mlk_fontinfo.h"
#include "mlk_fontconfig.h"
#include "mlk_charset.h"
#include "mlk_unicode.h"
#include "mlk_str.h"
#include "mlk_font_freetype.h"



//===========================
// mFontSystem
//===========================


/**@ フォントシステム初期化
 *
 * @d:unix の場合は、FreeType の初期化。\
 *  fontconfig の初期化は、別途行っておく必要がある。
 * @r:NULL で失敗。 */

mFontSystem *mFontSystem_init(void)
{
	mFontSystem *p;
	FT_Library lib;

	//FreeType 初期化
	
	if(FT_Init_FreeType(&lib))
	{
		mFontConfig_finish();
		return NULL;
	}

	//確保

	p = (mFontSystem *)mMalloc0(sizeof(mFontSystem));
	if(!p) return NULL;

	p->lib = lib;
	p->lcd_filter = -1;

	return p;
}

/**@ フォントシステム終了処理 */

void mFontSystem_finish(mFontSystem *p)
{
	if(p)
	{
		FT_Done_FreeType(p->lib);

		mFree(p);
	}
}


//===========================
// 色
//===========================


/**@ RGB 色のアルファブレンド (グレイスケール用)
 *
 * @p:bgcol 背景色
 * @p:fgcol 前景色(文字色)
 * @p:a アルファ値 (0-255)
 * @r:合成後の色 */

mRgbCol mFontBlendColor_gray(mRgbCol bgcol,mRgbCol fgcol,int a)
{
	int r,g,b;
	
	r = MLK_RGB_R(bgcol);
	g = MLK_RGB_G(bgcol);
	b = MLK_RGB_B(bgcol);
	
	r = ((MLK_RGB_R(fgcol) - r) * a / 255) + r;
	g = ((MLK_RGB_G(fgcol) - g) * a / 255) + g;
	b = ((MLK_RGB_B(fgcol) - b) * a / 255) + b;

	return MLK_RGB(r, g, b);
}

/**@ アルファブレンド (LCD) */

mRgbCol mFontBlendColor_lcd(mRgbCol bgcol,mRgbCol fgcol,int ra,int ga,int ba)
{
	int r,g,b;
	
	r = MLK_RGB_R(bgcol);
	g = MLK_RGB_G(bgcol);
	b = MLK_RGB_B(bgcol);
	
	r = ((MLK_RGB_R(fgcol) - r) * ra / 255) + r;
	g = ((MLK_RGB_G(fgcol) - g) * ga / 255) + g;
	b = ((MLK_RGB_B(fgcol) - b) * ba / 255) + b;

	return MLK_RGB(r,g,b);
}


//===========================
// ほか
//===========================


/**@ ファミリ名とスタイル名から、フォントファイルパスを取得
 *
 * @p:dst 確保された文字列のポインタが入る
 * @r:-1 でエラー。0〜で、フォント内のインデックス番号。 */

int mFontGetFilename_fromFamily(char **dst,const char *family,const char *style)
{
	mFcPattern pat;
	char *file;
	int index;

	*dst = NULL;

	//マッチするパターンを取得
	
	pat = mFontConfig_matchFamily(family, style);
	if(!pat) return -1;
	
	//フォントファイル名
	
	if(!mFCPattern_getFile(pat, &file, &index))
		index = -1;
	else
		*dst = mStrdup(file);

	mFCPattern_free(pat);

	return index;
}


//===========================
// mFont
//===========================


/* FT_Face 作成 */

static FT_Face _create_face(mFontSystem *sys,const char *filename,int index)
{
	FT_Face face;
	FT_Error err;
	char *str;

	//UTF-8 -> ロケール文字列

	str = mUTF8toLocale(filename, -1, NULL);
	if(!str) return NULL;

	//FT_Face 作成

	err = FT_New_Face(sys->lib, str, index, &face);

	mFree(str);

	return (err)? NULL: face;
}

/* FT_Face & mFont 作成
 *
 * filename: UTF-8 */

static mFont *_create_font(mFontSystem *sys,const char *filename,int index)
{
	mFont *p;
	FT_Face face;

	if(!filename || !(*filename)) return NULL;

	//FT_Face 作成

	face = _create_face(sys, filename, index);
	if(!face) return NULL;
	
	//mFont 確保
	
	p = (mFont *)mMalloc0(sizeof(mFont));
	if(!p)
	{
		FT_Done_Face(face);
		return NULL;
	}

	p->sys = sys;
	p->face = face;
	p->dpi = 96;

	return p;
}

/* mFontInfo からフォント作成 */

static mFont *_create_font_info(mFontSystem *sys,const mFontInfo *info)
{
	mFont *p;
	mFcPattern pat;
	char *file;
	int index;

	if((info->mask & MFONTINFO_MASK_FILE) && mStrIsnotEmpty(&info->str_file))
	{
		//----- ファイルパスから

		p = mFontCreate_file(sys, info->str_file.buf, info->index);
		if(!p) return NULL;

		//ex 情報セット

		mFontFT_setGlyphDraw_infoEx(p, info);

		//フォントサイズセット

		mFontFT_setFontSize(p, info, NULL);
	}
	else
	{
		//----- fontconfig から検索
		
		//マッチするパターンを取得
		
		pat = mFontConfig_matchFontInfo(info);
		if(!pat) return NULL;
		
		//フォントファイル名
		
		if(!mFCPattern_getFile(pat, &file, &index))
		{
			mFCPattern_free(pat);
			return NULL;
		}

		//mFont

		p = _create_font(sys, file, index);
		if(!p)
		{
			mFCPattern_free(pat);
			return NULL;
		}

		//グリフ描画情報セット

		mFontFT_setGlyphDraw_fontconfig(p, pat, info);

		//フォントサイズセット

		mFontFT_setFontSize(p, info, pat);

		//

		mFCPattern_free(pat);
	}

	return p;
}


/**@ フォント解放 */

void mFontFree(mFont *p)
{
	if(p)
	{
		if(p->free_font)
			(p->free_font)(p);
	
		FT_Done_Face(p->face);
		
		mFree(p);
	}
}

/**@ フォントファイルから作成
 *
 * @d:フォントサイズは作成後に必ず指定すること。
 *
 * @p:filename フォントファイル名 (UTF-8)
 * @p:index フォントコレクションの場合、読み込むフォントのインデックス位置 (0〜)
 * @r:失敗した場合 NULL。 */

mFont *mFontCreate_file(mFontSystem *sys,const char *filename,int index)
{
	mFont *p;

	p = _create_font(sys, filename, index);
	if(!p) return NULL;

	mFontFT_setGlyphDraw_default(p);

	return p;
}

/**@ フォント作成
 *
 * @d:作成に失敗した場合、デフォルトのフォントが読み込まれる。
 *
 * @p:sys GUI のフォントシステムを使う場合は、mGuiGetFontSystem() で取得する。 */

mFont *mFontCreate(mFontSystem *sys,const mFontInfo *info)
{
	mFont *p;
	mFontInfo info2;

	p = _create_font_info(sys, info);

	//失敗時はデフォルトのフォント

	if(!p)
	{
		mFontInfoInit(&info2);
		mFontInfoCopy(&info2, info);

		info2.mask &= ~(MFONTINFO_MASK_FILE | MFONTINFO_MASK_FAMILY | MFONTINFO_MASK_STYLE);

		p = _create_font_info(sys, &info2);

		mFontInfoFree(&info2);
	}
	
	return p;
}

/**@ デフォルトのフォントを作成
 *
 * @d:サイズは適当な値でセットされている。 */

mFont *mFontCreate_default(mFontSystem *sys)
{
	mFont *p;
	mFontInfo info;

	mFontInfoInit(&info);

	p = _create_font_info(sys, &info);

	mFontInfoFree(&info);

	return p;
}

/**@ フォント作成 (ファミリ名から)
 *
 * @d:サイズは別途セットすること。
 * @r:失敗した場合 NULL。 */

mFont *mFontCreate_family(mFontSystem *sys,const char *family,const char *style)
{
	mFont *p;
	char *file;
	int index;

	index = mFontGetFilename_fromFamily(&file, family, style);
	if(index == -1) return NULL;

	p = mFontCreate_file(sys, file, index);

	mFree(file);

	return p;
}

/**@ フォント作成
 *
 * @d:失敗した時は、デフォルトのフォントを読み込まずに、NULL を返す。 */

mFont *mFontCreate_raw(mFontSystem *sys,const mFontInfo *info)
{
	return _create_font_info(sys, info);
}

/**@ 文字列フォーマットからフォント作成 */

mFont *mFontCreate_text(mFontSystem *sys,const char *text)
{
	mFontInfo info;
	mFont *p;

	mFontInfoInit(&info);

	mFontInfoSetFromText(&info, text);

	p = mFontCreate(sys, &info);

	mFontInfoFree(&info);

	return p;
}

/**@ 文字列フォーマットからサイズ (pt) を変更して作成 */

mFont *mFontCreate_text_size(mFontSystem *sys,const char *text,double size)
{
	mFontInfo info;
	mFont *p;

	mFontInfoInit(&info);

	mFontInfoSetFromText(&info, text);

	info.mask |= MFONTINFO_MASK_SIZE;
	info.size = size;

	p = mFontCreate(sys, &info);

	mFontInfoFree(&info);

	return p;
}

/**@ フォントファイル内に複数のフォントが含まれる場合、名前とインデックスを列挙する
 *
 * @d:フォントコレクションで複数のフォントが含まれる場合や、
 *  バリアブルフォントで名前付きのスタイルがある場合、複数の情報が取得される。\
 *  \
 *  index は、FreeType においてファイルを開く時のインデックス値。\
 *  バリアブルフォントの場合は、bit16-30 にスタイルのインデックス (1〜) がセットされる。
 *
 * @r:-1 でエラー。それ以外で、含まれている数。 */

int mFontGetListNames(mFontSystem *sys,const char *filename,
	void (*func)(int index,const char *name,void *param),void *param)
{
	FT_Face face;
	int i,num,fvariable;
	mStr str = MSTR_INIT;

	//フォントの情報取得

	face = _create_face(sys, filename, -1);
	if(!face) return -1;

	if(face->style_flags >> 16)
	{
		//バリアブルフォントのスタイル数

		fvariable = TRUE;
		num = face->style_flags >> 16;
	}
	else
	{
		fvariable = FALSE;
		num = face->num_faces;
	}

	FT_Done_Face(face);

	//列挙

	if(fvariable)
	{
		//バリアブルフォント

		face = _create_face(sys, filename, 0);
		if(!face) return -1;

		mFontFT_enumVariableStyle(sys->lib, face, func, param);

		FT_Done_Face(face);
	}
	else
	{
		//通常
	
		for(i = 0; i < num; i++)
		{
			face = _create_face(sys, filename, i);
			if(!face) return -1;

			mFontFT_getFontFullName(face, &str);

			(func)(i, str.buf, param);

			FT_Done_Face(face);
		}

		mStrFree(&str);
	}

	return num;
}

/**@ フォントの高さ取得 */

int mFontGetHeight(mFont *p)
{
	return (p)? p->mt.height: 12;
}

/**@ フォントの行間取得 */

int mFontGetLineHeight(mFont *p)
{
	return (p)? p->mt.line_height: 12;
}

/**@ 縦書き時の高さ取得 */

int mFontGetVertHeight(mFont *p)
{
	return (p)? p->mt.pixel_per_em: 12;
}

/**@ ベースラインから上方向の距離を取得 */

int mFontGetAscender(mFont *p)
{
	return (p)? p->mt.ascender: 0;
}

/**@ フォント内に縦書きレイアウトが存在するか */

mlkbool mFontIsHaveVert(mFont *p)
{
	return (p && FT_HAS_VERTICAL(p->face) != 0);
}

/**@ フォントサイズセット
 *
 * @p:size 正の場合、pt。負の場合、px。\
 *  dpi は、フォント作成時の値を使う。 */

void mFontSetSize(mFont *p,double size)
{
	if(size < 0)
		mFontSetSize_pixel(p, (int)(-size + 0.5));
	else
		mFontSetSize_pt(p, size, -1);
}

/**@ フォントサイズセット (pt 単位)
 *
 * @p:dpi 0 以下の場合、フォント作成時に使われた DPI */

void mFontSetSize_pt(mFont *p,double size,int dpi)
{
	if(!p) return;
	
	if(dpi <= 0)
		dpi = p->dpi;

	FT_Set_Char_Size(p->face, 0, (int)(size * 64 + 0.5), dpi, dpi);

	mFontFT_setMetrics(p);
}

/**@ フォントサイズセット (px 単位) */

void mFontSetSize_pixel(mFont *p,int size)
{
	if(!p) return;

	FT_Set_Pixel_Sizes(p->face, 0, size);

	mFontFT_setMetrics(p);
}

/**@ テキストフォーマットで指定されているサイズをセット
 *
 * @d:サイズが指定されていない場合は、何もしない。 */

void mFontSetSize_fromText(mFont *p,const char *text)
{
	mFontInfo info;

	mFontInfoInit(&info);
	mFontInfoSetFromText(&info, text);

	if(info.mask & MFONTINFO_MASK_SIZE)
	{
		if(info.size < 0)
			mFontSetSize_pixel(p, -info.size);
		else
		{
			mFontSetSize_pt(p, info.size,
				(info.mask & MFONTINFO_MASK_DPI)? info.dpi: 96);
		}
	}

	mFontInfoFree(&info);
}

/**@ mFontInfo の拡張情報を適用 */

void mFontSetInfoEx(mFont *p,const mFontInfo *info)
{
	if(!p) return;

	mFontFT_setGlyphDraw_infoEx(p, info);
	mFontFT_setMetrics(p);
}


//============================
// 幅取得
//============================


/**@ テキストの px 幅取得
 *
 * @p:text UTF-8 文字列
 * @p:len 文字列の長さ。負の値でヌル文字まで。 */

int mFontGetTextWidth(mFont *p,const char *text,int len)
{
	const char *pc;
	uint32_t code;
	int ret,w = 0;

	if(!p || !text) return 0;

	if(len < 0) len = mStrlen(text);
	if(len == 0) return 0;

	mFontFT_setLCDFilter(p);

	for(pc = text; *pc && pc - text < len; )
	{
		ret = mUTF8GetChar(&code, pc, len - (pc - text), &pc);
		if(ret < 0) break;
		else if(ret > 0) continue;

		w += mFontFT_getGlyphWidth_code(p, code);
	}
	
	return w;
}

/**@ テキストの幅取得 (ホットキー処理付き)
 *
 * @d:'&' と次の1文字は、ホットキーとして扱う。 */

int mFontGetTextWidth_hotkey(mFont *p,const char *text,int len)
{
	const char *pc;
	uint32_t code;
	int ret,w = 0,prev_and = 0;

	if(!p || !text) return 0;
	
	if(len < 0) len = mStrlen(text);
	if(len == 0) return 0;
	
	mFontFT_setLCDFilter(p);

	for(pc = text; *pc && pc - text < len; )
	{
		if(*pc == '&')
		{
			pc++;
		
			if(prev_and)
			{
				//前が & で今回も & なら、& が1文字
				code = '&';
				prev_and = FALSE;
			}
			else
			{
				//& はスキップする
				prev_and = TRUE;
				continue;
			}
		}
		else
		{
			//通常文字

			ret = mUTF8GetChar(&code, pc, len - (pc - text), &pc);
			if(ret < 0) break;
			else if(ret > 0) continue;

			prev_and = FALSE;
		}

		w += mFontFT_getGlyphWidth_code(p, code);
	}
	
	return w;
}

/**@ テキストの幅取得 (UTF-32 文字列)
 *
 * @p:len 文字数 (文字単位) */

int mFontGetTextWidth_utf32(mFont *p,const mlkuchar *text,int len)
{
	int i,w = 0;
	
	if(!p || !text) return 0;
	
	mFontFT_setLCDFilter(p);

	for(i = 0; *text; text++, i++)
	{
		if(len >= 0 && i >= len) break;
	
		w += mFontFT_getGlyphWidth_code(p, *text);
	}
	
	return w;
}

/**@ UTF-32 １文字の幅取得 */

int mFontGetCharWidth_utf32(mFont *p,mlkuchar code)
{
	mFontFT_setLCDFilter(p);
	
	return mFontFT_getGlyphWidth_code(p, code);
}


//===============================
// 描画
//===============================


/**@ 水平にテキスト描画
 *
 * @p:x,y  左上位置
 * @p:len  負の値でヌル文字まで
 * @p:param 描画用パラメータ */

void mFontDrawText(mFont *p,int x,int y,
	const char *text,int len,mFontDrawInfo *info,void *param)
{
	const char *pc;
	int ret;
	uint32_t code;

	if(!p || !text) return;

	if(len < 0) len = mStrlen(text);
	if(len == 0) return;

	mFontFT_setLCDFilter(p);

	for(pc = text; *pc && pc - text < len; )
	{
		ret = mUTF8GetChar(&code, pc, len - (pc - text), &pc);
		if(ret < 0) break;
		else if(ret > 0) continue;

		x = mFontFT_drawGlyphH_code(p, x, y, code, info, param);
	}
}

/**@ テキスト描画 (ホットキー処理付き)
 *
 * @d:&+1文字で、文字を下線付きにする。"&&" で "&"。 */

void mFontDrawText_hotkey(mFont *p,int x,int y,
	const char *text,int len,mFontDrawInfo *info,void *param)
{
	const char *pc;
	uint32_t code;
	int ret,prev_and = 0,nextx;
	
	if(!p || !text) return;

	if(len < 0) len = mStrlen(text);
	if(len == 0) return;

	mFontFT_setLCDFilter(p);

	for(pc = text; *pc && pc - text < len; )
	{
		if(*pc == '&')
		{
			pc++;
		
			if(prev_and)
			{
				//&&
				code = '&';
				prev_and = FALSE;
			}
			else
			{
				prev_and = TRUE;
				continue;
			}
		}
		else
		{
			ret = mUTF8GetChar(&code, pc, len - (pc - text), &pc);
			if(ret < 0) break;
			else if(ret > 0) continue;
		}

		//グリフ
		
		nextx = mFontFT_drawGlyphH_code(p, x, y, code, info, param);

		//前が & なら下線

		if(prev_and)
		{
			if(nextx - x)
			{
				(info->draw_underline)(
					x, y + p->mt.ascender + p->mt.underline_pos,
					nextx - x, p->mt.underline_thickness, param);
			}

			prev_and = FALSE;
		}

		x = nextx;
	}
}

/**@ テキスト描画 (UTF-32 文字列) */

void mFontDrawText_utf32(mFont *p,int x,int y,
	const mlkuchar *text,int len,mFontDrawInfo *info,void *param)
{
	int i;
	
	if(!p || !text) return;
	
	mFontFT_setLCDFilter(p);

	for(i = 0; *text; text++, i++)
	{
		if(len >= 0 && i >= len) break;

		x = mFontFT_drawGlyphH_code(p, x, y, *text, info, param);
	}
}


