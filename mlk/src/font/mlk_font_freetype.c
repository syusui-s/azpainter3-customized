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
 * フォントの FreeType 実装処理
 *****************************************/

#include <ft2build.h>
#include FT_CONFIG_CONFIG_H
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_LCD_FILTER_H
#include FT_TRUETYPE_TABLES_H
#include FT_SFNT_NAMES_H
#include FT_MULTIPLE_MASTERS_H
#include FT_CID_H

#define MLK_FONT_FREETYPE_DEFINE

#include "mlk.h"
#include "mlk_font.h"
#include "mlk_fontinfo.h"
#include "mlk_font_freetype.h"
#include "mlk_fontconfig.h"
#include "mlk_str.h"



/**@ 26:6 の固定小数点数を丸めて整数値へ */

int mFontFT_round_fix6(int32_t n)
{
	if(n >= 0)
		return (n + 32) >> 6;
	else
		return (n - 32) >> 6;
}

/**@ FT_Face を取得 */

void *mFontFT_getFace(mFont *p)
{
	return (void *)p->face;
}

/**@ フォントの寸法情報を取得 */

void mFontFT_getMetrics(mFont *p,mFTMetrics *dst)
{
	*dst = p->mt;
}

/**@ LCD フィルターセット */

void mFontFT_setLCDFilter(mFont *p)
{	
#ifdef FT_CONFIG_OPTION_SUBPIXEL_RENDERING

	if(p->gdraw.render_mode == FT_RENDER_MODE_LCD
		&& p->sys->lcd_filter != p->gdraw.lcd_filter)
	{
		FT_Library_SetLcdFilter(p->sys->lib, p->gdraw.lcd_filter);

		p->sys->lcd_filter = p->gdraw.lcd_filter;
	}

#endif
}

/**@ フォントの寸法情報をセット
 *
 * @d:フォントサイズや、mFTGlyphDraw を設定した後に行う。 */

void mFontFT_setMetrics(mFont *p)
{
	FT_Face face = p->face;
	mFTMetrics *dst = &p->mt;
	FT_Size_Metrics *pm;
	FT_Fixed scale;
	int h;

	pm = &face->size->metrics;


	if(FT_IS_SCALABLE(face))
	{
		//-------- スケーラブルフォント

		dst->pixel_per_em = pm->y_ppem;
		dst->ascender = MFT_F6_TO_INT_FLOOR(pm->ascender);
	
		scale = pm->y_scale; //デザイン単位から px(26:6) へ変換するためのスケール値

		//高さ
		// : pm->height では、実際の高さより大きい場合がある。
		// : ascender+descender > pm->height (1px 程度) の場合もある。

		dst->height = MFT_F6_TO_INT_CEIL(pm->ascender - pm->descender);

		h = MFT_F6_TO_INT_CEIL(pm->height);
		if(h < dst->height) dst->height = h;

		//

		dst->line_height = MFT_FUNIT_TO_PX_CEIL(face->height, scale);

		dst->underline_pos = MFT_FUNIT_TO_PX_FLOOR(-(face->underline_position), scale);
		dst->underline_thickness = MFT_FUNIT_TO_PX_CEIL(face->underline_thickness, scale);

		if(dst->underline_thickness == 0)
			dst->underline_thickness = 1;

		//ヒンティングによっては、実際に描画すると、1px ほど大きい場合があるので、
		//実際のグリフを取得して高さを比較する。
		//(グリフがない場合もあるので、フォントの高さより大きい場合のみセット)

		h = mFontFT_getGlyphHeight(p, 'g');
		if(h != -1)
		{
			if(h > dst->height) dst->height = h;
			if(h > dst->line_height) dst->line_height = h;
		}
	}
	else if(face->available_sizes)
	{
		//----- ビットマップフォント
		//[!] スケーラブル用の他の情報はすべて 0 になっている

		h = face->available_sizes->y_ppem >> 6;

		dst->pixel_per_em = h;
		dst->ascender = h;
		dst->height = face->available_sizes->height;
		dst->line_height = dst->height;
		dst->underline_pos = 1;
		dst->underline_thickness = 1;
	}
}

/**@ フォントサイズセット
 *
 * @p:pat NULL で、mFontInfo の情報のみでセット */

void mFontFT_setFontSize(mFont *p,const mFontInfo *info,mFcPattern pat)
{
	double size;
	int dpi;

	//サイズ (負の値で px 単位)

	if(!pat)
	{
		if(info->mask & MFONTINFO_MASK_SIZE)
			size = info->size;
		else
			size = 10;
	}
	else
	{
		if(!mFCPattern_getDouble(pat, "size", &size))
		{
			if(mFCPattern_getDouble(pat, "pixelsize", &size))
				size = -size;
			else
				size = 10;
		}
	}
		
	//dpi

	if(info->mask & MFONTINFO_MASK_DPI)
		dpi = (info->dpi > 0)? info->dpi: 96;
	else if(!pat)
		dpi = 96;
	else
		dpi = (int)(mFCPattern_getDouble_def(pat, "dpi", 96) + 0.5);

	//フォントサイズセット
	
	if(size < 0)
		FT_Set_Pixel_Sizes(p->face, 0, -size);
	else
		FT_Set_Char_Size(p->face, 0, (int)(size * 64 + 0.5), dpi, dpi);

	//寸法セット

	mFontFT_setMetrics(p);

	//サイズ変更時用に DPI 記録

	p->dpi = dpi;
}


//=======================
// グリフ関連
//=======================


/**@ 文字コードからグリフIDを取得
 *
 * @r:グリフが見つからなかった場合、0 */

uint32_t mFontFT_codeToGID(mFont *p,uint32_t code)
{
	return FT_Get_Char_Index(p->face, code);
}

/**@ GID から CID を取得
 *
 * @r:CID。失敗時は 0 */

uint32_t mFontFT_GID_to_CID(mFont *p,uint32_t gid)
{
	FT_UInt cid;

	if(FT_Get_CID_From_Glyph_Index(p->face, gid, &cid))
		cid = 0;

	return cid;
}

/**@ グリフの高さを取得 (横書き用)
 *
 * @d:グリフの左上原点から、グリフの文字がある部分までの高さを取得する。
 *
 * @r:文字高さ。-1 で失敗 */

int mFontFT_getGlyphHeight(mFont *p,uint32_t code)
{
	FT_GlyphSlot slot;
	mFTPos pos;
	int h;

	pos.x = pos.y = pos.advance = 0;

	if(!mFontFT_loadGlyphH_code(p, code, &pos))
		return -1;

	slot = p->face->glyph;

	//文字がある部分の高さ (LCD_V の場合は1/3)

	h = slot->bitmap.rows;

	if(slot->bitmap.pixel_mode == FT_PIXEL_MODE_LCD_V)
		h /= 3;

	//高さ

	return pos.y + h;
}

/**@ グリフの幅を取得 (横書き用)
 *
 * @d:文字がない空白の部分も含む。
 * @r:失敗した場合、0。 */

int mFontFT_getGlyphWidth_code(mFont *p,uint32_t code)
{
	mFTPos pos;

	pos.x = pos.y = pos.advance = 0;

	if(mFontFT_loadGlyphH_code(p, code, &pos))
		return pos.advance;
	else
		return 0;
}

/**@ 文字コードから、ビットマップグリフをロード
 *
 * @d:主に、グリフの寸法が必要な時に使う。 */

mlkbool mFontFT_loadGlyphH_code(mFont *p,uint32_t code,mFTPos *pos)
{
	code = FT_Get_Char_Index(p->face, code);

	return mFontFT_loadGlyphH_gid(p, code, pos);
}

/**@ GID から、ビットマップグリフをロード (横書き用)
 *
 * @p:pos いずれもフォントデザイン単位。\
 *  x,y : [IN] アウトラインフォント時、座標調整値 (負の値で下方向)。\
 *  [OUT] 描画原点からのビットマップ左上位置 (px)。\
 * advance : [IN] advance の調整値 [OUT] 描画原点から次のグリフへの距離 (px) */

mlkbool mFontFT_loadGlyphH_gid(mFont *p,uint32_t gid,mFTPos *pos)
{
	FT_Face face = p->face;
	uint32_t f;

	//グリフスロットにロード

	if(FT_Load_Glyph(face, gid, p->gdraw.fload_glyph))
		return FALSE;

	//

	f = p->gdraw.flags;

	if(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	{
		//------ アウトライン

		//座標移動
		//(原点をグリフ左上へ)

		FT_Outline_Translate(&face->glyph->outline,
			FT_MulFix(pos->x, face->size->metrics.x_scale),
			FT_MulFix(pos->y - face->ascender, face->size->metrics.y_scale));

		//太字化

		if(f & MFTGLYPHDRAW_F_EMBOLDEN)
			FT_Outline_Embolden(&face->glyph->outline, 1<<5);

		//行列

		if(f & MFTGLYPHDRAW_F_SLANT_MATRIX)
			FT_Outline_Transform(&face->glyph->outline, &p->gdraw.slant_matrix);

		//ビットマップに変換

		if(FT_Render_Glyph(face->glyph, p->gdraw.render_mode))
			return FALSE;

		//

		pos->y = -(face->glyph->bitmap_top);
	}
	else if(face->glyph->format == FT_GLYPH_FORMAT_BITMAP)
	{
		//------ ビットマップ
		// ベースライン (ascender) は、高さと等しい。

		//太字化

		if(f & MFTGLYPHDRAW_F_EMBOLDEN)
		{
			FT_GlyphSlot_Own_Bitmap(face->glyph);
			FT_Bitmap_Embolden(p->sys->lib, &face->glyph->bitmap, 1<<6, 0);
		}

		//原点移動

		pos->y = p->mt.ascender - face->glyph->bitmap_top;
	}

	//

	pos->x = face->glyph->bitmap_left;

	pos->advance = mFontFT_round_fix6(face->glyph->advance.x
		+ FT_MulFix(pos->advance, face->size->metrics.x_scale));
	
	return TRUE;
}

/**@ 水平グリフ描画 (GID)
 *
 * @r:次の x 位置 */

int mFontFT_drawGlyphH_gid(mFont *p,int x,int y,uint32_t gid,mFTPos *pos,mFontDrawInfo *info,void *param)
{
	if(mFontFT_loadGlyphH_gid(p, gid, pos))
	{
		mFontFT_drawGlyph(p, x + pos->x, y + pos->y, info, param);
	
		x += pos->advance;
	}

	return x;
}

/**@ 水平グリフ描画 (文字コード)
 *
 * @r:次の x 位置 */

int mFontFT_drawGlyphH_code(mFont *p,int x,int y,uint32_t code,mFontDrawInfo *info,void *param)
{
	mFTPos pos;

	code = FT_Get_Char_Index(p->face, code);

	pos.x = pos.y = pos.advance = 0;

	return mFontFT_drawGlyphH_gid(p, x, y, code, &pos, info, param);
}

/**@ スロットにロードされているグリフを描画 (水平/垂直共通) */

void mFontFT_drawGlyph(mFont *p,int x,int y,mFontDrawInfo *drawinfo,void *param)
{
	FT_Bitmap *bm;
	uint8_t *pbuf,*pb,f,r,g,b,is_bgr;
	int ix,iy,xend,yend,w,h,pitch,pitch2,pitch3;
	mFuncFontSetPixelMono setpix_mono;
	mFuncFontSetPixelGray setpix_gray;
	mFuncFontSetPixelLCD setpix_lcd;

	bm = &p->face->glyph->bitmap;
	
	w = bm->width;
	h = bm->rows;
	pbuf  = bm->buffer;
	pitch = bm->pitch;
	
	if(pitch < 0) pbuf += -pitch * (h - 1);
	
	is_bgr = ((p->gdraw.flags & MFTGLYPHDRAW_F_SUBPIXEL_BGR) != 0);
	
	//
	
	if(bm->pixel_mode == FT_PIXEL_MODE_MONO)
	{
		//1bit モノクロ

		setpix_mono = drawinfo->setpix_mono;
		xend = x + w;
		yend = y + h;

		for(iy = y; iy < yend; iy++, pbuf += pitch)
		{
			for(ix = x, f = 0x80, pb = pbuf; ix < xend; ix++)
			{
				if(*pb & f)
					(setpix_mono)(ix, iy, param);
				
				f >>= 1;
				if(!f) { f = 0x80; pb++; }
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		//8bit グレイスケール
		
		setpix_gray = drawinfo->setpix_gray;
		xend = x + w;
		yend = y + h;
		pitch -= w;

		for(iy = y; iy < yend; iy++, pbuf += pitch)
		{
			for(ix = x; ix < xend; ix++, pbuf++)
			{
				if(*pbuf)
					(setpix_gray)(ix, iy, *pbuf, param);
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_LCD)
	{
		//LCD
		
		setpix_lcd = drawinfo->setpix_lcd;
		pitch -= w;
		w /= 3;
		xend = x + w;
		yend = y + h;
		
		for(iy = y; iy < yend; iy++, pbuf += pitch)
		{
			for(ix = x; ix < xend; ix++, pbuf += 3)
			{
				if(is_bgr)
					r = pbuf[2], g = pbuf[1], b = pbuf[0];
				else
					r = pbuf[0], g = pbuf[1], b = pbuf[2];

				if(r | g | b)
					(setpix_lcd)(ix, iy, r, g, b, param);
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_LCD_V)
	{
		//LCD Vertical

		setpix_lcd = drawinfo->setpix_lcd;
		pitch2 = pitch * 3 - w;
		pitch3 = pitch << 1;
		h /= 3;
		xend = x + w;
		yend = y + h;
		
		for(iy = y; iy < yend; iy++, pbuf += pitch2)
		{
			for(ix = x; ix < xend; ix++, pbuf++)
			{
				if(is_bgr)
					r = pbuf[pitch3], g = pbuf[pitch], b = pbuf[0];
				else
					r = pbuf[0], g = pbuf[pitch], b = pbuf[pitch3];

				if(r | g | b)
					(setpix_lcd)(ix, iy, r, g, b, param);
			}
		}
	}
}


//===============================
// mFTGlyphDraw に値をセット
//===============================


/* fload_glyph のフラグを ON/OFF */

static void _loadglyph_onoff(mFTGlyphDraw *p,uint32_t f,uint32_t onoff)
{
	if(onoff)
		p->fload_glyph |= f;
	else
		p->fload_glyph &= ~f;
}

/* flags のフラグを ON/OFF */

static void _flags_onoff(mFTGlyphDraw *p,uint32_t f,uint32_t onoff)
{
	if(onoff)
		p->flags |= f;
	else
		p->flags &= ~f;
}


/**@ mFTGlyphDraw にデフォルト値をセット (直接ファイル読み込み時) */

void mFontFT_setGlyphDraw_default(mFont *font)
{
	mFTGlyphDraw *p = &font->gdraw;

	mMemset0(p, sizeof(mFTGlyphDraw));

	p->render_mode = FT_RENDER_MODE_NORMAL;
	p->lcd_filter = FT_LCD_FILTER_DEFAULT;

	p->def_hinting = MFTGLYPHDRAW_HINTING_NONE;
	p->def_rendering = MFTGLYPHDRAW_RENDER_LCD_RGB;
	p->def_lcdfilter = 1;
	p->def_flags = MFTGLYPHDRAW_SWITCH_AUTOHINT; //MFTGLYPHDRAW_SWITCH_* のフラグ

	mFontFT_setGlyphDraw_hinting(font, p->def_hinting);
	mFontFT_setGlyphDraw_rendermode(font, p->def_rendering);
}

/**@ mFTGlyphDraw にデータをセット (fontconfig からのマッチ時) */

void mFontFT_setGlyphDraw_fontconfig(mFont *font,mFcPattern pat,const mFontInfo *info)
{
	mFTGlyphDraw *p = &font->gdraw;
	int n;
	double matrix[4];
	
	mMemset0(p, sizeof(mFTGlyphDraw));

	//ヒンティング

	if(mFCPattern_getBool_def(pat, "hinting", TRUE))
		n = mFCPattern_getInt_def(pat, "hintstyle", 1);
	else
		n = MFTGLYPHDRAW_HINTING_NONE;

	mFontFT_setGlyphDraw_hinting(font, n);

	p->def_hinting = n;

	//レンダリングモード

	if(!mFCPattern_getBool_def(pat, "antialias", TRUE))
		//antialias = false
		n = MFTGLYPHDRAW_RENDER_MONO;
	else
	{
		//rgba = 0:unknown, 1:RGB, 2:BGR, 3:VRGB, 4:VBGR, 5:NONE
		
		n = mFCPattern_getInt_def(pat, "rgba", 0);
		if(n < 1 || n > 4) n = 0;
		
		n += MFTGLYPHDRAW_RENDER_GRAY;
	}

	mFontFT_setGlyphDraw_rendermode(font, n);

	p->def_rendering = n;

	//LCD Filter
	// 0:NONE, 1:DEFAULT, 2:LIGHT, 3:LEGACY(非推奨)
	
	n = mFCPattern_getInt_def(pat, "lcdfilter", 1);
	
	if(n == 0)
		n = FT_LCD_FILTER_NONE;
	else if(n == 2)
		n = FT_LCD_FILTER_LIGHT;
	else if(n == 3)
		n = FT_LCD_FILTER_LEGACY;
	else
		n = FT_LCD_FILTER_DEFAULT;

	p->lcd_filter = n;
	p->def_lcdfilter = n;
	
	//オートヒント

	n = mFCPattern_getBool_def(pat, "autohint", TRUE);

	_loadglyph_onoff(p, FT_LOAD_NO_AUTOHINT, (n == 0));

	if(n)
		p->def_flags |= MFTGLYPHDRAW_SWITCH_AUTOHINT;

	//埋め込みビットマップ

	n = mFCPattern_getBool_def(pat, "embeddedbitmap", TRUE);

	_loadglyph_onoff(p, FT_LOAD_NO_BITMAP, (n == 0)); 

	if(n)
		p->def_flags |= MFTGLYPHDRAW_SWITCH_EMBED_BITMAP;

	//太字処理
	
	if(mFCPattern_getBool_def(pat, "embolden", FALSE))
		p->flags |= MFTGLYPHDRAW_F_EMBOLDEN;
	
	//斜体処理 (matrix)
	
	if(mFCPattern_getMatrix(pat, "matrix", matrix))
	{
		p->flags |= MFTGLYPHDRAW_F_SLANT_MATRIX;

		p->slant_matrix.xx = MFT_DOUBLE_TO_F16(matrix[0]);
		p->slant_matrix.xy = MFT_DOUBLE_TO_F16(matrix[1]);
		p->slant_matrix.yx = MFT_DOUBLE_TO_F16(matrix[2]);
		p->slant_matrix.yy = MFT_DOUBLE_TO_F16(matrix[3]);
	}

	//----- mFontInfoEx からセット

	if(info->mask & MFONTINFO_MASK_EX)
		mFontFT_setGlyphDraw_infoEx(font, info);
}

/**@ mFontInfo の ex データから mFTGlyphDraw に情報セット
 *
 * @d:マスクで指定されていない値は、デフォルトを使う。\
 *  フォントファイル直接読み込み時は、固定のデフォルト値。\
 *  fontconfig から読み込んだ場合は、fontconfig における設定値。 */

void mFontFT_setGlyphDraw_infoEx(mFont *p,const mFontInfo *info)
{
	mFTGlyphDraw *draw = &p->gdraw;
	int n;

	if(!(info->mask & MFONTINFO_MASK_EX)) return;

	//ヒンティング
	
	mFontFT_setGlyphDraw_hinting(p,
		(info->ex.mask & MFONTINFO_EX_MASK_HINTING)? info->ex.hinting: draw->def_hinting);

	//レンダリング

	mFontFT_setGlyphDraw_rendermode(p,
		(info->ex.mask & MFONTINFO_EX_MASK_RENDERING)? info->ex.rendering: draw->def_rendering);

	//LCD フィルター

	draw->lcd_filter = (info->ex.mask & MFONTINFO_EX_MASK_LCD_FILTER)? info->ex.lcd_filter: draw->def_lcdfilter;

	//オートヒント

	if(info->ex.mask & MFONTINFO_EX_MASK_AUTO_HINT)
		n = !(info->ex.flags & MFONTINFO_EX_FLAGS_AUTO_HINT);
	else
		n = !(draw->def_flags & MFTGLYPHDRAW_SWITCH_AUTOHINT);

	_loadglyph_onoff(draw, FT_LOAD_NO_AUTOHINT, n);

	//埋め込みビットマップ

	if(info->ex.mask & MFONTINFO_EX_MASK_EMBEDDED_BITMAP)
		n = !(info->ex.flags & MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP);
	else
		n = !(draw->def_flags & MFTGLYPHDRAW_SWITCH_EMBED_BITMAP);
	
	_loadglyph_onoff(draw, FT_LOAD_NO_BITMAP, n);
}

/**@ mFTGlyphDraw のヒンティングのタイプを変更
 *
 * @d:FULL の場合、レンダリングモードによって変化あり。 */

void mFontFT_setGlyphDraw_hinting(mFont *font,int type)
{
	mFTGlyphDraw *p = &font->gdraw;
	uint32_t f;

	f = p->fload_glyph &
		(~(FT_LOAD_NO_HINTING | FT_LOAD_TARGET_LIGHT | FT_LOAD_TARGET_LCD | FT_LOAD_TARGET_LCD_V));

	switch(type)
	{
		//なし
		case MFTGLYPHDRAW_HINTING_NONE:
			f |= FT_LOAD_NO_HINTING;
			break;
		//SLIGHT
		case MFTGLYPHDRAW_HINTING_SLIGHT:
			f |= FT_LOAD_TARGET_LIGHT;
			break;
		//FULL
		case MFTGLYPHDRAW_HINTING_FULL:
			if(p->render_mode == FT_RENDER_MODE_LCD_V)
				f |= FT_LOAD_TARGET_LCD_V;
			else
				f |= FT_LOAD_TARGET_LCD;
			break;
		//MEDIUM
		default:
			f |= FT_LOAD_TARGET_NORMAL;
			break;
	}

	p->fload_glyph = f;
}

/**@ mFTGlyphDraw のレンダリングモードを変更
 *
 * @d:埋め込みビットマップの有効/無効は、別途設定する必要あり。 */

void mFontFT_setGlyphDraw_rendermode(mFont *font,int type)
{
	mFTGlyphDraw *p = &font->gdraw;
	FT_Render_Mode mode;

	p->flags &= ~MFTGLYPHDRAW_F_SUBPIXEL_BGR;

	switch(type)
	{
		case MFTGLYPHDRAW_RENDER_MONO:
			mode = FT_RENDER_MODE_MONO;
			break;
		case MFTGLYPHDRAW_RENDER_LCD_RGB:
			mode = FT_RENDER_MODE_LCD;
			break;
		case MFTGLYPHDRAW_RENDER_LCD_BGR:
			mode = FT_RENDER_MODE_LCD;
			p->flags |= MFTGLYPHDRAW_F_SUBPIXEL_BGR;
			break;
		case MFTGLYPHDRAW_RENDER_LCD_VRGB:
			mode = FT_RENDER_MODE_LCD_V;
			break;
		case MFTGLYPHDRAW_RENDER_LCD_VBGR:
			mode = FT_RENDER_MODE_LCD_V;
			p->flags |= MFTGLYPHDRAW_F_SUBPIXEL_BGR;
			break;
		default:
			mode = FT_RENDER_MODE_NORMAL;
			break;
	}

	p->render_mode = mode;
}

/**@ フラグで mFTGlyphDraw の各情報をセット
 *
 * @p:mask 変更する値のフラグを ON にしたマスク
 * @p:flags 設定するフラグ */

void mFontFT_setGlyphDraw_switch(mFont *font,uint32_t mask,uint32_t flags)
{
	mFTGlyphDraw *p = &font->gdraw;

	//オートヒンティング

	if(mask & MFTGLYPHDRAW_SWITCH_AUTOHINT)
		_loadglyph_onoff(p, FT_LOAD_NO_AUTOHINT, !(flags & MFTGLYPHDRAW_SWITCH_AUTOHINT));

	//埋め込みビットマップ

	if(mask & MFTGLYPHDRAW_SWITCH_EMBED_BITMAP)
		_loadglyph_onoff(p, FT_LOAD_NO_BITMAP, !(flags & MFTGLYPHDRAW_SWITCH_EMBED_BITMAP));

	//太字化

	if(mask & MFTGLYPHDRAW_SWITCH_EMBOLDEN)
		_flags_onoff(p, MFTGLYPHDRAW_F_EMBOLDEN, flags & MFTGLYPHDRAW_SWITCH_EMBOLDEN);

	//斜体化

	if(mask & MFTGLYPHDRAW_SWITCH_SLANT_MATRIX)
	{
		_flags_onoff(p, MFTGLYPHDRAW_F_SLANT_MATRIX, flags & MFTGLYPHDRAW_SWITCH_SLANT_MATRIX);

		//行列値
		
		if(flags & MFTGLYPHDRAW_SWITCH_SLANT_MATRIX)
		{
			p->slant_matrix.xx = p->slant_matrix.yy = 1<<16;
			p->slant_matrix.xy = MFT_DOUBLE_TO_F16(0.2);
			p->slant_matrix.yx = 0;
		}
	}
}


//=====================


/**@ 指定テーブルのデータを読み込み
 *
 * @p:tag テーブル識別子
 * @p:ppbuf 確保されたバッファのポインタが入る
 * @p:psize データサイズが入る */

mlkerr mFontFT_loadTable(mFont *p,uint32_t tag,void **ppbuf,uint32_t *psize)
{
	FT_ULong len = 0;
	void *buf;

	//TrueType/OpenType でない

	if(!FT_IS_SFNT(p->face))
		return MLKERR_UNSUPPORTED;

	//長さを取得

	if(FT_Load_Sfnt_Table(p->face, tag, 0, NULL, &len))
		return MLKERR_UNFOUND;

	//確保

	buf = mMalloc(len);
	if(!buf) return MLKERR_ALLOC;

	//読み込み

	if(FT_Load_Sfnt_Table(p->face, tag, 0, (FT_Byte *)buf, &len))
		return MLKERR_UNKNOWN;

	*ppbuf = buf;
	*psize = len;

	return MLKERR_OK;
}

/**@ (SFNT 時) name テーブル内から name id を検索して、文字列取得
 *
 * @d:対応しているのは Unicode 文字列のみ。\
 *  platform = Windows 時は lang id = 0x409 の英語を優先して取得するが、存在しない場合は、
 *  リスト上で一番最後の言語の文字列となる。
 *
 * @r:TRUE で取得に成功 */

mlkbool mFontFT_getNameTbl_id(void *face,int id,mStr *str)
{
	FT_Face p = (FT_Face)face;
	FT_SfntName name;
	int i,num,fset;

	//文字列の数

	num = FT_Get_Sfnt_Name_Count(p);

	//検索
	// :同じ name id で複数のフォーマットが存在する場合あり。

	fset = 0;

	for(i = 0; i < num; i++)
	{
		if(FT_Get_Sfnt_Name(p, i, &name)) continue;

		if(name.name_id == id
			&& (name.platform_id == 0 || name.platform_id == 3))
		{
			//UTF-16BE からセット

			mStrSetText_utf16be(str, name.string, name.string_len / 2);

			fset = TRUE;

			//確定

			if(name.platform_id == 0
				|| (name.platform_id == 3 && name.language_id == 0x409))
			{
				break;
			}
		}
	}

	return fset;
}

/**@ フォントのフル名を取得 */

mlkbool mFontFT_getFontFullName(void *face,mStr *str)
{
	FT_Face p = (FT_Face)face;

	if(!mFontFT_getNameTbl_id(face, 4, str))
		mStrSetFormat(str, "%s %s", p->family_name, p->style_name);

	return TRUE;
}

/**@ バリアブルフォントの名前付きスタイルを列挙 */

void mFontFT_enumVariableStyle(void *ftlib,void *face,void (*func)(int index,const char *name,void *param),void *param)
{
#if !defined(MLK_NO_FREETYPE_VARIABLE)
	FT_Face p = (FT_Face)face;
	FT_MM_Var *var;
	FT_Var_Named_Style *ps;
	FT_UInt i;
	mStr str = MSTR_INIT;

	if(FT_Get_MM_Var(p, &var)) return;

	ps = var->namedstyle;

	for(i = 0; i < var->num_namedstyles; i++, ps++)
	{
		if(!mFontFT_getNameTbl_id(face, ps->strid, &str))
			continue;

		(func)((i + 1) << 16, str.buf, param);
	}

	FT_Done_MM_Var((FT_Library)ftlib, var);

	mStrFree(&str);
#endif
}

