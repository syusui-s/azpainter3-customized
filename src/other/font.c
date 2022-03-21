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
 * フォント
 *****************************************/

#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H
#include FT_OUTLINE_H

#define MLK_FONT_FREETYPE_DEFINE

#include "mlk_gui.h"
#include "mlk_font.h"
#include "mlk_fontinfo.h"
#include "mlk_font_freetype.h"
#include "mlk_opentype.h"
#include "mlk_pixbuf.h"
#include "mlk_str.h"

#include "def_drawtext.h"
#include "regfont.h"

#include "font.h"
#include "pv_font.h"
#include "fontcache.h"


//----------------------

//DrawFont

struct _DrawFont
{
	mFont *font[3];  //[0]base [1]rep1 [2]rep2

	uint8_t *charbuf;		//置換フォントの文字データ(rep1+rep2)
	uint16_t charflags[2],	//置換フォントの文字フラグ
		charsize[2];		//置換フォントの文字データサイズ
};

//描画用データ

typedef struct
{
	DrawFontString str;
	mFontDrawInfo *fdinfo;
	uint32_t *pchar;
	int len,
		charsp,
		linesp,
		topx,topy,		//描画開始位置
		relx,rely,		//現在の相対位置
		frotate;		//回転するか
	double dcos,dsin;
	FT_Matrix mat;		//回転用行列
}_drawdata;

enum
{
	_DRAW_F_VERT = 1<<0,		//縦書き
	_DRAW_F_HORZ_IN_VERT = 1<<1	//縦中横
};

//----------------------



//==============================
//
//==============================


/* 登録フォントの作成
 *
 * return: FALSE で、デフォルトフォントを単一で作成 */

static mlkbool _create_regfont(DrawFont *p,DrawTextData *dt)
{
	mListItem *pi;
	RegFontDat *dat;
	int i;

	//ID から検索 (なければデフォルトフォント)

	pi = RegFont_findID(dt->font_param);
	if(!pi) return FALSE;

	//RegFontDat

	dat = RegFontDat_make(pi);
	if(!dat) return FALSE;

	//ベースフォント

	p->font[0] = FontCache_loadFont(dat->str_basefont.buf, dat->index_base);
	if(!p->font[0])
	{
		RegFontDat_free(dat);
		return FALSE;
	}

	//置き換えフォント
	// :失敗時は NULL

	for(i = 0; i < 2; i++)
	{
		p->font[1 + i] = FontCache_loadFont(dat->str_repfont[i].buf, dat->index_rep[i]);

		p->charflags[i] = dat->charflags[i];
		p->charsize[i] = dat->charsize[i];
	}

	//文字データ

	p->charbuf = (uint8_t *)mMemdup(dat->charbuf, p->charsize[0] + p->charsize[1]);

	RegFontDat_free(dat);
	
	return TRUE;
}


//==============================
// main
//==============================


/** 初期化 */

void DrawFontInit(void)
{
	FontCache_init();
}

/** 終了時の解放 */

void DrawFontFinish(void)
{
	FontCache_free();
}


/** フォント解放 */

void DrawFont_free(DrawFont *p)
{
	if(p)
	{
		FontCache_releaseFont(p->font[2]);
		FontCache_releaseFont(p->font[1]);
		FontCache_releaseFont(p->font[0]);

		mFree(p->charbuf);
		
		mFree(p);
	}
}

/** フォント作成 */

DrawFont *DrawFont_create(DrawTextData *dt,int imgdpi)
{
	DrawFont *p;
	mFont *font;

	//DrawFont

	p = (DrawFont *)mMalloc0(sizeof(DrawFont));
	if(!p) return NULL;

	//フォント

	switch(dt->fontsel)
	{
		//一覧
		case DRAWTEXT_FONTSEL_LIST:
			font = FontCache_loadFont_family(dt->str_font.buf, dt->str_style.buf);
			if(!font) return NULL;

			p->font[0] = font;
			break;
		//登録フォント
		case DRAWTEXT_FONTSEL_REGIST:
			if(!_create_regfont(p, dt))
			{
				font = FontCache_loadFont(NULL, 0);
				if(!font) return NULL;

				p->font[0] = font;
			}
			break;
		//フォントファイル
		case DRAWTEXT_FONTSEL_FILE:
			font = FontCache_loadFont(dt->str_font.buf, dt->font_param);
			if(!font) return NULL;

			p->font[0] = font;
			break;
	}

	//初期化

	DrawFont_setSize(p, dt, imgdpi);

	DrawFont_setInfo(p, dt);

	return p;
}

/** フォントサイズのセット */

void DrawFont_setSize(DrawFont *p,DrawTextData *dt,int imgdpi)
{
	int i,dpi;
	double dpt;

	if(!p) return;

	dpt = dt->fontsize * 0.1;

	if(dt->flags & DRAWTEXT_F_ENABLE_DPI)
		dpi = dt->dpi;
	else
		dpi = imgdpi;

	for(i = 0; i < 3; i++)
	{
		if(dt->unit_fontsize == DRAWTEXT_UNIT_FONTSIZE_PT)
			//pt
			mFontSetSize_pt(p->font[i], dpt, dpi);
		else
			//px
			mFontSetSize_pixel(p->font[i], dt->fontsize);
	}
}

/** ルビサイズのセット */

void DrawFont_setRubySize(DrawFont *p,DrawTextData *dt,int imgdpi)
{
	int i,unit,dpi,size;
	double dpt;

	if(!p) return;

	unit = dt->unit_rubysize;

	dpi = (dt->flags & DRAWTEXT_F_ENABLE_DPI)? dt->dpi: imgdpi;

	size = dt->rubysize;
	dpt = size * 0.1;

	//フォントサイズに対する%

	if(unit == DRAWTEXT_UNIT_RUBYSIZE_PERS)
	{
		if(dt->unit_fontsize == DRAWTEXT_UNIT_FONTSIZE_PT)
		{
			dpt = dt->fontsize * 0.1 * (size / 100.0);
			unit = DRAWTEXT_UNIT_RUBYSIZE_PT;
		}
		else
		{
			size = dt->fontsize * size / 100;
			unit = DRAWTEXT_UNIT_RUBYSIZE_PX;
		}
	}

	//

	for(i = 0; i < 3; i++)
	{
		if(unit == DRAWTEXT_UNIT_RUBYSIZE_PT)
			//pt
			mFontSetSize_pt(p->font[i], dpt, dpi);
		else
			//px
			mFontSetSize_pixel(p->font[i], size);
	}
}

/** 各情報をセット */

void DrawFont_setInfo(DrawFont *p,DrawTextData *dt)
{
	mFontInfo info;
	int i;
	uint32_t flags;

	if(!p) return;

	//mFontInfoEx

	mFontInfoInit(&info);

	info.mask = MFONTINFO_MASK_EX;
	info.ex.mask = MFONTINFO_EX_MASK_HINTING | MFONTINFO_EX_MASK_RENDERING
		| MFONTINFO_EX_MASK_AUTO_HINT | MFONTINFO_EX_MASK_EMBEDDED_BITMAP;

	info.ex.hinting = dt->hinting;
	info.ex.rendering = (dt->flags & DRAWTEXT_F_MONO)?
		MFONTINFO_EX_RENDERING_MONO: MFONTINFO_EX_RENDERING_GRAY;

	if(!(dt->flags & DRAWTEXT_F_DISABLE_AUTOHINT))
		info.ex.flags |= MFONTINFO_EX_FLAGS_AUTO_HINT;

	if(dt->flags & DRAWTEXT_F_ENABLE_BITMAP)
		info.ex.flags |= MFONTINFO_EX_FLAGS_EMBEDDED_BITMAP;

	//太字化/斜体化

	flags = 0;

	if(dt->flags & DRAWTEXT_F_BOLD)
		flags |= MFTGLYPHDRAW_SWITCH_EMBOLDEN;

	if(dt->flags & DRAWTEXT_F_ITALIC)
		flags |= MFTGLYPHDRAW_SWITCH_SLANT_MATRIX;

	//

	for(i = 0; i < 3; i++)
	{
		if(p->font[i])
		{
			mFontSetInfoEx(p->font[i], &info);

			mFontFT_setGlyphDraw_switch(p->font[i],
				MFTGLYPHDRAW_SWITCH_EMBOLDEN | MFTGLYPHDRAW_SWITCH_SLANT_MATRIX, flags);
		}
	}
}


//===========================
// 描画サブ
//===========================


/* [mFont] GID からグリフをロード (横書き)
 *
 * mat: 回転用。NULL でなし */

static mlkbool _load_glyph_horz(mFont *p,uint32_t gid,mFTPos *pos,FT_Matrix *mat,uint8_t flags)
{
	FT_Face face = p->face;
	FT_Outline *outline;
	uint32_t f;

	//グリフスロットにロード

	if(FT_Load_Glyph(face, gid, p->gdraw.fload_glyph))
		return FALSE;

	//

	f = p->gdraw.flags;

	if(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	{
		//------ アウトライン

		outline = &face->glyph->outline;

		//太字化

		if(f & MFTGLYPHDRAW_F_EMBOLDEN)
			FT_Outline_Embolden(outline, 1<<5);

		//斜体化

		if(f & MFTGLYPHDRAW_F_SLANT_MATRIX)
			FT_Outline_Transform(outline, &p->gdraw.slant_matrix);

		//座標移動
		//(原点をグリフ左上へ)

		FT_Outline_Translate(outline, 0,
			FT_MulFix(-face->ascender, face->size->metrics.y_scale));

		//回転

		if(mat)
			FT_Outline_Transform(outline, mat);

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
	pos->advance = mFontFT_round_fix6(face->glyph->advance.x);
	
	return TRUE;
}

/* [mFont] GID からグリフをロード (縦書き)
 *
 * mat: 回転用。NULL でなし */

static mlkbool _load_glyph_vert(mFont *p,uint32_t gid,mFTPos *pos,FT_Matrix *mat,uint8_t flags)
{
	FT_Face face = p->face;
	FT_Outline *outline;
	uint32_t f;
	int origin;

	//グリフスロットにロード

	if(FT_Load_Glyph(face, gid, p->gdraw.fload_glyph | FT_LOAD_VERTICAL_LAYOUT))
		return FALSE;

	//

	f = p->gdraw.flags;

	if(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	{
		//------ アウトライン

		outline = &face->glyph->outline;

		//太字化

		if(f & MFTGLYPHDRAW_F_EMBOLDEN)
			FT_Outline_Embolden(outline, 1<<5);

		//斜体化

		if(f & MFTGLYPHDRAW_F_SLANT_MATRIX)
			FT_Outline_Transform(outline, &p->gdraw.slant_matrix);

		//Y 原点
		// :VORG テーブルがあればそこから取得。なければ ascender。

		if(!mOpenType_getVORG_originY(&p->sub->vorg, gid, &origin))
			origin = face->ascender;

		//座標移動
		//(原点をグリフ左上へ)

		FT_Outline_Translate(outline, 0,
			FT_MulFix(-origin, face->size->metrics.y_scale));

		//回転

		if(mat)
			FT_Outline_Transform(outline, mat);

		//ビットマップに変換

		if(FT_Render_Glyph(face->glyph, p->gdraw.render_mode))
			return FALSE;

		//

		pos->y = -(face->glyph->bitmap_top);
	}
	else if(face->glyph->format == FT_GLYPH_FORMAT_BITMAP)
	{
		//------ ビットマップ
		// グリフ中央・上が原点になっている

		//太字化

		if(f & MFTGLYPHDRAW_F_EMBOLDEN)
		{
			FT_GlyphSlot_Own_Bitmap(face->glyph);
			FT_Bitmap_Embolden(p->sys->lib, &face->glyph->bitmap, 1<<6, 0);
		}

		//原点移動

		pos->y = face->glyph->bitmap_top;
	}

	//

	pos->x = face->glyph->bitmap_left;

	if(flags & _DRAW_F_HORZ_IN_VERT)
		//縦中横
		pos->advance = mFontFT_round_fix6(p->face->glyph->metrics.horiAdvance);
	else
		pos->advance = mFontFT_round_fix6(face->glyph->advance.y);
	
	return TRUE;
}

/* GID からグリフの描画
 *
 * return: 送り幅 */

static int _draw_glyph(mFont *font,int relx,int rely,uint32_t gid,_drawdata *dat,uint8_t flags)
{
	mFTPos pos;
	int x,y;
	mlkbool ret;

	//グリフロード

	if(flags & (_DRAW_F_VERT | _DRAW_F_HORZ_IN_VERT))
		ret = _load_glyph_vert(font, gid, &pos, (dat->frotate)? &dat->mat: NULL, flags);
	else
		ret = _load_glyph_horz(font, gid, &pos, (dat->frotate)? &dat->mat: NULL, flags);

	if(!ret) return 0;

	//描画

	x = dat->topx + pos.x;
	y = dat->topy + pos.y;
	
	if(dat->frotate)
	{
		x = (int)(x + (relx * dat->dcos - rely * dat->dsin));
		y = (int)(y + (relx * dat->dsin + rely * dat->dcos));
	}
	else
	{
		x += relx;
		y += rely;
	}

	mFontFT_drawGlyph(font, x, y, dat->fdinfo, 0);

	return pos.advance;
}


//===========================
// 文字
//===========================


/* 次の文字を取得
 *
 * return: FALSE で終了 */

static mlkbool _get_next_char(_drawdata *dat,uint32_t *dst)
{
	if(dat->len == 0) return FALSE;

	*dst = *(dat->pchar);

	dat->pchar++;
	dat->len--;

	return TRUE;
}

/* 指定文字の次までスキップ */

static void _skip_to_char_next(_drawdata *dat,uint32_t c)
{
	uint32_t *pc,ctmp;
	int len;

	pc = dat->pchar;
	len = dat->len;

	while(len)
	{
		ctmp = *(pc++);
		len--;

		if(ctmp == c) break;
	}

	dat->pchar = pc;
	dat->len = len;
}

/* Unicode から置き換えフォント検索 */

static mFont *_repfont_unicode(DrawFont *p,uint32_t code)
{
	mFont *font;
	int i,pos;
	uint16_t flags;

	for(i = 0; i < 2; i++)
	{
		font = p->font[1 + i];
		if(!font) continue;

		flags = p->charflags[i];

		//文字種

		if((flags & REGFONT_CHARF_MASK)
			&& RegFontCode_checkFlags(code, flags))
			return font;

		//文字コード

		if(!(flags & REGFONT_CHARF_CODE_CID) && p->charsize[i])
		{
			pos = (i == 0)? 0: p->charsize[0];
		
			if(RegFontCode_checkBuf((uint32_t *)(p->charbuf + pos), p->charsize[i] / 4, code))
				return font;
		}
	}

	return p->font[0];
}

/* CID から置き換えフォント検索 */

static mFont *_repfont_cid(DrawFont *p,uint32_t code)
{
	mFont *font;
	int i,pos;

	for(i = 0; i < 2; i++)
	{
		font = p->font[1 + i];
		if(!font) continue;

		//CID フォントのみ

		if(!font->sub->is_cid) continue;

		//文字コード

		if((p->charflags[i] & REGFONT_CHARF_CODE_CID) && p->charsize[i])
		{
			pos = (i == 0)? 0: p->charsize[0];
		
			if(RegFontCode_checkBuf((uint32_t *)(p->charbuf + pos), p->charsize[i] / 4, code))
				return font;
		}
	}

	return p->font[0];
}

/* 文字からフォントと GID を取得 */

static mFont *_get_font_gid(DrawFont *p,uint32_t c,uint32_t *dst)
{
	mFont *font;
	uint32_t gid;
	
	gid = CHAR_GET_CODE(c);

	//font & gid

	if(c & CHAR_F_CID)
		//CIDフォント場合は、CID key が GID となる
		font = p->font[0];
	else
	{
		//Unicode -> GID

		font = _repfont_unicode(p, gid);
		
		gid = FT_Get_Char_Index(font->face, gid);

		//置換フォントで GID = 0 の場合、ベースフォントのグリフを使う

		if(font != p->font[0] && !gid)
		{
			font = p->font[0];
			gid = FT_Get_Char_Index(font->face, CHAR_GET_CODE(c));
		}
	}

	//CID で置換フォント検索
	// :Unicode ですでに置き換えられている場合は除外。
	// :ベースフォントと置換フォントが CID の場合のみ。

	if(font == p->font[0] && font->sub->is_cid)
		font = _repfont_cid(p, gid);

	*dst = gid;

	return font;
}

/* 縦中横時の GID 置き換え */

static uint32_t _replace_tateyoko(mFont *font,int type,uint32_t gid)
{
	if(type == 1)
		//半角幅
		gid = mOpenType_getGSUB_replaceSingle(font->sub->gsub_hwid, gid);
	else if(type == 2)
		//1/3幅
		gid = mOpenType_getGSUB_replaceSingle(font->sub->gsub_twid, gid);

	return gid;
}

/* 横書きのルビ文字置き換え */

static uint32_t _replace_horz_ruby(mFont *font,uint32_t gid,DrawTextData *dt)
{
	//ruby

	if(!(dt->flags & DRAWTEXT_F_NO_RUBY_GLYPH))
		gid = mOpenType_getGSUB_replaceSingle(font->sub->gsub_ruby, gid);

	return gid;
}

/* 縦書きのルビ文字置き換え */

static uint32_t _replace_vert_ruby(mFont *font,uint32_t gid,DrawTextData *dt)
{
	//vert
	
	gid = mOpenType_getGSUB_replaceSingle(font->sub->gsub_vert, gid);

	//ruby

	if(!(dt->flags & DRAWTEXT_F_NO_RUBY_GLYPH))
		gid = mOpenType_getGSUB_replaceSingle(font->sub->gsub_ruby, gid);

	return gid;
}


//===========================
// 描画
//===========================


/* 描画用データセット
 *
 * return: FALSE で描画なし */

static mlkbool _set_drawdata(DrawFont *p,_drawdata *dst,DrawTextData *dt,int x,int y,mFontDrawInfo *info)
{
	mFont *font = p->font[0];
	double rd,dcos,dsin;

	if(!p || mStrIsEmpty(&dt->str_text)) return FALSE;

	mMemset0(dst, sizeof(_drawdata));

	dst->topx = x;
	dst->topy = y;
	dst->fdinfo = info;

	//文字データ
	
	__DrawFont_getString(&dst->str, dt->str_text.buf, dt->flags & DRAWTEXT_F_ENABLE_FORMAT);

	dst->pchar = (uint32_t *)dst->str.buf.buf;
	dst->len = dst->str.buf.cursize / 4;

	//字間

	if(dt->unit_char_space == 0)
		dst->charsp = font->mt.pixel_per_em * dt->char_space / 100;
	else
		dst->charsp = dt->char_space;

	//行間

	if(dt->unit_line_space == 0)
		dst->linesp = font->mt.pixel_per_em * dt->line_space / 100;
	else
		dst->linesp = dt->line_space;

	//回転

	if(dt->angle)
	{
		rd = dt->angle * MLK_MATH_PI / 180;
		dcos = cos(rd);
		dsin = sin(rd);

		dst->mat.xx = (int)(dcos * (1<<16));
		dst->mat.xy = (int)(-dsin * (1<<16));
		dst->mat.yx = -dst->mat.xy;
		dst->mat.yy = dst->mat.xx;

		dst->dcos = dcos;
		dst->dsin = -dsin;

		dst->frotate = TRUE;
	}

	return TRUE;
}

/* 濁点/半濁点描画 */

static void _draw_dakuten(DrawFont *p,int w,uint32_t c,_drawdata *dat,uint8_t flags)
{
	mFont *font;
	uint32_t code;

	code = (c & CHAR_F_DAKUTEN)? 0x309B: 0x309C;

	font = _get_font_gid(p, code, &c);

	_draw_glyph(font, dat->relx + w, dat->rely, c, dat, flags);
}


//========== 横書き


/* ルビ描画 (横書き) */

static void _draw_horz_ruby(DrawFont *p,int imgdpi,DrawTextData *dt,_drawdata *dat)
{
	RubyItem *pi;
	mFont *font;
	uint32_t *pc,*pctop,c;
	int len,toplen,w,rubylen,n,xpos,i,rpos;
	mFTPos pos;

	DrawFont_setRubySize(p, dt, imgdpi);

	//ルビ位置
	// :% はベースフォントが対象

	if(dt->unit_ruby_pos == 0)
		rpos = p->font[0]->mt.pixel_per_em * dt->ruby_pos / 100;
	else
		rpos = dt->ruby_pos;

	//

	for(pi = (RubyItem *)dat->str.list_ruby.top; pi; pi = (RubyItem *)pi->i.next)
	{
		//ルビ文字位置

		pctop = (uint32_t *)(dat->str.buf.buf + pi->ruby_pos);
		toplen = (dat->str.buf.cursize - pi->ruby_pos) / 4;

		//ルビ文字幅取得

		w = 0;
		rubylen = 0;

		for(pc = pctop, len = toplen; len; )
		{
			c = *(pc++);
			len--;

			if(c == CHAR_CMDVAL_RUBY_END) break;
			if(c & CHAR_F_COMMAND) continue;

			font = _get_font_gid(p, c, &c);

			if(_load_glyph_horz(font, _replace_horz_ruby(font, c, dt), &pos, NULL, 0))
			{
				w += pos.advance;
				rubylen++;
			}
		}

		if(rubylen == 0) continue;

		//X 位置

		if(w >= pi->parent_w)
		{
			//ルビ文字の方が大きい
			// :親文字に対して中央寄せ。

			xpos = (pi->parent_w - w) / 2;
			w = 0;
		}
		else
		{
			//ルビ文字の方が小さい
			// :余白を均等に配分

			w = pi->parent_w - w;

			if(rubylen == 1)
			{
				xpos = w / 2;
				w = 0;
			}
			else
			{
				n = w / (rubylen * 2); //上と下の余白
				xpos = n;
				w = w - n * 2; //間の余白幅
			}
		}

		//各文字描画

		i = 0;

		for(pc = pctop, len = toplen; len; )
		{
			c = *(pc++);
			len--;

			if(c == CHAR_CMDVAL_RUBY_END) break;
			if(c & CHAR_F_COMMAND) continue;

			font = _get_font_gid(p, c, &c);

			if(!w)
				n = 0;
			else
			{
				n = w * i / (rubylen - 1);
				i++;
			}

			xpos += _draw_glyph(font,
				pi->x + xpos + n, pi->y - rpos - font->mt.pixel_per_em,
				_replace_horz_ruby(font, c, dt), dat, 0);
		}
	}

	DrawFont_setSize(p, dt, imgdpi);
}

/* テキスト描画 (横書き) */

static void _drawtext_horz(DrawFont *p,int x,int y,int imgdpi,DrawTextData *dt,mFontDrawInfo *info)
{
	mFont *font;
	_drawdata dat;
	uint32_t c,cc;
	int w,type = 0,ruby_parent_w;

	if(!_set_drawdata(p, &dat, dt, x, y, info)) return;

	while(_get_next_char(&dat, &c))
	{
		if(c & CHAR_F_COMMAND)
		{
			//コマンド

			c &= 0xffff;

			switch(c)
			{
				//改行
				case CHAR_CMD_ID_ENTER:
					dat.relx = 0;
					dat.rely += p->font[0]->mt.height + dat.linesp;
					break;
				//半角幅
				case CHAR_CMD_ID_YOKO2_START:
					type = 1;
					break;
				//1/3幅
				case CHAR_CMD_ID_YOKO3_START:
					type = 2;
					break;
				//縦中横終了
				case CHAR_CMD_ID_YOKO_END:
					type = 0;
					break;
				//ルビ親文字列の開始
				// :文字幅の記録を開始, 描画位置をセット
				case CHAR_CMD_ID_RUBY_PARENT_START:
					type = 3;
					ruby_parent_w = 0;

					__DrawFont_setRubyParentPos(&dat.str, dat.pchar,
						dat.relx, dat.rely);
					break;
				//ルビ文字の開始
				// :文字幅をセット, ルビ終了までスキップ
				case CHAR_CMD_ID_RUBY_START:
					type = 0;

					if(ruby_parent_w) ruby_parent_w -= dat.charsp;
					
					__DrawFont_setRubyParentW(&dat.str, dat.pchar, ruby_parent_w);

					_skip_to_char_next(&dat, CHAR_CMDVAL_RUBY_END);
					break;
			/*
				//ルビ文字の開始
				// :ルビ終了までスキップ
				case CHAR_CMD_ID_RUBY_START:
					_skip_to_char_next(&dat, CHAR_CMDVAL_RUBY_END);
					break;
			*/
			}
		}
		else
		{
			font = _get_font_gid(p, c, &cc);

			//置き換え

			if(type)
			{
				if(type == 1)
					//半角幅
					cc = mOpenType_getGSUB_replaceSingle(font->sub->gsub_hwid, cc);
				else if(type == 2)
					//1/3幅
					cc = mOpenType_getGSUB_replaceSingle(font->sub->gsub_twid, cc);
			}

			//描画
			
			w = _draw_glyph(font, dat.relx, dat.rely, cc, &dat, 0);

			if(c & (CHAR_F_DAKUTEN | CHAR_F_HANDAKUTEN))
				_draw_dakuten(p, w, c, &dat, 0);

			if(w)
			{
				dat.relx += w + dat.charsp;

				//ルビ親文字列の場合、文字幅を加算
				
				if(type == 3) ruby_parent_w += w + dat.charsp;
			}
		}
	}

	//ルビ描画

	if(dat.str.list_ruby.top)
		_draw_horz_ruby(p, imgdpi, dt, &dat);

	//

	__DrawFont_freeString(&dat.str);
}


//========== 縦書き


/* 縦中横の描画
 * [!] 源ノフォントでは、横書きのレイアウトで描画するとY位置がずれるので、
 *  縦書きレイアウトで描画する。
 *
 * type: [0] そのまま [1] 半角幅 [2] 1/3幅 */

static void _draw_vert_tateyoko(DrawFont *p,int type,_drawdata *dat)
{
	mFont *font;
	mFTPos pos;
	uint32_t c,*ptop,*pc;
	int x,toplen,len,w = 0;

	ptop = dat->pchar;
	toplen = dat->len;

	//幅を取得
	// :間に改行などのコマンドがある場合、無視。

	for(pc = ptop, len = toplen; len;)
	{
		c = *(pc++);
		len--;
		
		if(c == CHAR_CMDVAL_YOKO_END) break;
		if(c & CHAR_F_COMMAND) continue;

		font = _get_font_gid(p, c, &c);

		if(_load_glyph_horz(font, _replace_tateyoko(font, type, c), &pos, NULL, 0))
			w += pos.advance;
	}

	//描画

	x = dat->relx + (p->font[0]->mt.pixel_per_em - w) / 2;

	for(pc = ptop, len = toplen; len; )
	{
		c = *(pc++);
		len--;
		
		if(c == CHAR_CMDVAL_YOKO_END) break;
		if(c & CHAR_F_COMMAND) continue;

		font = _get_font_gid(p, c, &c);

		x += _draw_glyph(font, x, dat->rely, _replace_tateyoko(font, type, c), dat, _DRAW_F_HORZ_IN_VERT);
	}

	//次の位置

	dat->pchar = pc;
	dat->len = len;

	//間に文字がない場合は、空扱い

	if(*ptop != CHAR_CMDVAL_YOKO_END)
		dat->rely += p->font[0]->mt.pixel_per_em + dat->charsp;
}

/* ルビ描画 (縦書き) */

static void _draw_vert_ruby(DrawFont *p,int imgdpi,DrawTextData *dt,_drawdata *dat)
{
	RubyItem *pi;
	mFont *font;
	uint32_t *pc,*pctop,c;
	int len,toplen,w,rubylen,n,ypos,i,rpos,x;
	mFTPos pos;

	DrawFont_setRubySize(p, dt, imgdpi);

	//ルビ位置
	// :% はベースフォントが対象

	if(dt->unit_ruby_pos == 0)
		rpos = p->font[0]->mt.pixel_per_em * dt->ruby_pos / 100;
	else
		rpos = dt->ruby_pos;

	//

	for(pi = (RubyItem *)dat->str.list_ruby.top; pi; pi = (RubyItem *)pi->i.next)
	{
		//ルビ文字位置

		pctop = (uint32_t *)(dat->str.buf.buf + pi->ruby_pos);
		toplen = (dat->str.buf.cursize - pi->ruby_pos) / 4;

		//ルビ文字幅取得

		w = 0;
		rubylen = 0;

		for(pc = pctop, len = toplen; len; )
		{
			c = *(pc++);
			len--;

			if(c == CHAR_CMDVAL_RUBY_END) break;
			if(c & CHAR_F_COMMAND) continue;

			font = _get_font_gid(p, c, &c);

			if(_load_glyph_vert(font, _replace_vert_ruby(font, c, dt), &pos, NULL, 0))
			{
				w += pos.advance;
				rubylen++;
			}
		}

		if(rubylen == 0) continue;

		//Y 位置

		if(w >= pi->parent_w)
		{
			//ルビ文字の方が大きい
			// :親文字に対して中央寄せ。

			ypos = (pi->parent_w - w) / 2;
			w = 0;
		}
		else
		{
			//ルビ文字の方が小さい
			// :余白を均等に配分

			w = pi->parent_w - w;

			if(rubylen == 1)
			{
				ypos = w / 2;
				w = 0;
			}
			else
			{
				n = w / (rubylen * 2); //上と下の余白
				ypos = n;
				w = w - n * 2; //間の余白幅
			}
		}

		//各文字描画

		x = pi->x + rpos;
		i = 0;

		for(pc = pctop, len = toplen; len; )
		{
			c = *(pc++);
			len--;

			if(c == CHAR_CMDVAL_RUBY_END) break;
			if(c & CHAR_F_COMMAND) continue;

			font = _get_font_gid(p, c, &c);

			if(!w)
				n = 0;
			else
			{
				n = w * i / (rubylen - 1);
				i++;
			}

			ypos += _draw_glyph(font, x , pi->y + ypos + n,
				_replace_vert_ruby(font, c, dt), dat, _DRAW_F_VERT);
		}
	}

	DrawFont_setSize(p, dt, imgdpi);
}

/* テキスト描画 (縦書き) */

static void _drawtext_vert(DrawFont *p,int x,int y,int imgdpi,DrawTextData *dt,mFontDrawInfo *info)
{
	mFont *font;
	_drawdata dat;
	uint32_t c,cc;
	int w,ruby_parent_h,type;

	if(!_set_drawdata(p, &dat, dt, x, y, info)) return;

	//メイン描画

	type = 0;
	ruby_parent_h = 0;

	while(_get_next_char(&dat, &c))
	{
		if(c & CHAR_F_COMMAND)
		{
			//コマンド

			c &= 0xffff;

			switch(c)
			{
				//改行
				case CHAR_CMD_ID_ENTER:
					dat.relx -= p->font[0]->mt.pixel_per_em + dat.linesp;
					dat.rely = 0;
					break;
				//縦中横
				case CHAR_CMD_ID_YOKO1_START:
				case CHAR_CMD_ID_YOKO2_START:
				case CHAR_CMD_ID_YOKO3_START:
					_draw_vert_tateyoko(p, c - CHAR_CMD_ID_YOKO1_START, &dat);
					break;
				//ルビ親文字列の開始
				// :文字幅の記録を開始, 描画位置をセット
				case CHAR_CMD_ID_RUBY_PARENT_START:
					type = 1;
					ruby_parent_h = 0;

					__DrawFont_setRubyParentPos(&dat.str, dat.pchar,
						dat.relx + p->font[0]->mt.pixel_per_em, dat.rely);
					break;
				//ルビ文字の開始
				// :文字幅をセット, ルビ終了までスキップ
				case CHAR_CMD_ID_RUBY_START:
					type = 0;

					if(ruby_parent_h) ruby_parent_h -= dat.charsp;
					
					__DrawFont_setRubyParentW(&dat.str, dat.pchar, ruby_parent_h);

					_skip_to_char_next(&dat, CHAR_CMDVAL_RUBY_END);
					break;
			}
		}
		else
		{
			font = _get_font_gid(p, c, &cc);

			//縦書き置換

			cc = mOpenType_getGSUB_replaceSingle(font->sub->gsub_vert, cc);

			//描画

			w = _draw_glyph(font, dat.relx, dat.rely, cc, &dat, _DRAW_F_VERT);

			//濁点/半濁点

			if(c & (CHAR_F_DAKUTEN | CHAR_F_HANDAKUTEN))
				_draw_dakuten(p, font->mt.pixel_per_em, c, &dat, _DRAW_F_VERT);

			//

			if(w)
			{
				dat.rely += w + dat.charsp;

				//ルビ親文字列の場合、高さを加算

				if(type == 1) ruby_parent_h += w + dat.charsp;
			}
		}
	}

	//ルビ描画

	if(dat.str.list_ruby.top)
		_draw_vert_ruby(p, imgdpi, dt, &dat);

	//

	__DrawFont_freeString(&dat.str);
}

/** テキスト描画 */

void DrawFont_drawText(DrawFont *p,int x,int y,int imgdpi,DrawTextData *dt,mFontDrawInfo *info)
{
	if(dt->flags & DRAWTEXT_F_VERT)
		//縦書き
		_drawtext_vert(p, x, y, imgdpi, dt, info);
	else
		//横書き
		_drawtext_horz(p, x, y, imgdpi, dt, info);
}


