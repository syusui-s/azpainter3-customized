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

/*************************************
 * テキスト描画用フォント (freetype)
 *************************************/

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H
#include FT_OUTLINE_H
#include FT_OPENTYPE_VALIDATE_H

#include "mDef.h"
#include "mGui.h"
#include "mFont.h"
#include "mFontConfig.h"
#include "mFreeType.h"
#include "mFTGSUB.h"
#include "mUtilCharCode.h"

#include "DrawFont.h"


//----------------

/** フォントデータ */

struct _DrawFont
{
	FT_Face face;
	void *vert_tbl;		//縦書き文字テーブル

	int baseline,
		horzHeight,vertHeight;	//横/縦書きそれぞれの高さ
	mBool has_vert;		//縦書きレイアウトを持っているか

	mFreeTypeInfo info;
	uint32_t fLoadGlyph_default;
};

typedef struct
{
	int left,top,advance;
}_glyph_drawmt;

//----------------

static FT_Library g_ftlib = NULL;

//----------------

enum
{
	GETGLYPH_F_VERT = 1<<0,
};

static mBool _getBitmapGlyph(FT_Library lib,DrawFont *p,uint32_t code,int flags,_glyph_drawmt *drawmt);
static void _drawChar(DrawFont *p,int x,int y,DrawFontInfo *info);

//----------------



//============================
// sub
//============================


/** フォント作成時、各データセット */

static void _setFontData(DrawFont *p,FT_Face face)
{
	void *gsub;
	mFreeTypeMetricsInfo metrics;

	//縦書き情報があるか

	p->has_vert = FT_HAS_VERTICAL(face);

	//縦書きグリフテーブル

	gsub = mFreeTypeGetGSUB(face);

	if(gsub)
	{
		p->vert_tbl = mFTGSUB_getVertTable(gsub);
		
		FT_OpenType_Free(face, (FT_Bytes)gsub);
	}

	//ベースライン & 横書き高さ

	mFreeTypeGetMetricsInfo(g_ftlib, face, &p->info, &metrics);

	p->baseline   = metrics.baseline;
	p->horzHeight = metrics.height;
	p->vertHeight = face->size->metrics.y_ppem;
}

/** 26:6 の固定小数点数を丸めて整数値へ */

int _round_fix6(int32_t n)
{
	if(n >= 0)
		return (n + 32) >> 6;
	else
		return (n - 32) >> 6;
}


//============================
// main
//============================


/** 起動時の初期化 */

mBool DrawFont_init()
{
	//mlib のシステムで使っている FT_Library を共有して使う
	
	g_ftlib = (FT_Library)mGetFreeTypeLib();

	return TRUE;
}

/** 終了時の解放
 *
 * (将来的に他のプラットフォームに対応する場合のために一応定義しておく) */

void DrawFont_finish()
{

}


/** フォント解放 */

void DrawFont_free(DrawFont *p)
{
	if(p)
	{
		FT_Done_Face(p->face);

		mFree(p->vert_tbl);
		mFree(p);
	}
}

/** フォント作成
 *
 * \param dpi 0以下で fontconfig の値 */

DrawFont *DrawFont_create(mFontInfo *info,int dpi)
{
	DrawFont *p;
	mFcPattern *pat;
	FT_Face face = NULL;
	char *file;
	int index;
	double size;

	//mFontInfo とマッチするパターン

	pat = mFontConfigMatch(info);
	if(!pat) return NULL;
	
	//フォントファイル読み込み (FT_Face)
	
	if(mFontConfigGetFileInfo(pat, &file, &index))
		goto ERR;
		
	if(FT_New_Face(g_ftlib, file, index, &face))
		goto ERR;
	
	//DrawFont 確保
	
	p = (DrawFont *)mMalloc(sizeof(DrawFont), TRUE);
	if(!p) goto ERR;
	
	p->face = face;

	//FcPattern から情報取得

	mFreeTypeGetInfoByFontConfig(&p->info, pat, info);

	p->fLoadGlyph_default = p->info.fLoadGlyph;

	mFontConfigPatternFree(pat);

	//文字高さセット (FreeType)

	size = p->info.size;

	if(dpi <= 0)
		dpi = p->info.dpi;

	if(size < 0)
		//px
		FT_Set_Pixel_Sizes(face, 0, -size);
	else
		//pt
		FT_Set_Char_Size(face, 0, (int)(size * 64 + 0.5), dpi, dpi);

	//他データセット

	_setFontData(p, face);

	return p;

ERR:
	if(face) FT_Done_Face(face);
	mFontConfigPatternFree(pat);
	
	return NULL;
}

/** ヒンティングを変更 */

void DrawFont_setHinting(DrawFont *p,int type)
{
	if(p)
	{
		if(type == 0)
			p->info.fLoadGlyph = p->fLoadGlyph_default;
		else
			mFreeTypeSetInfo_hinting(&p->info, type - 1);
	}
}

/** テキスト描画
 *
 * x,y は文字の左上位置 */

void DrawFont_drawText(DrawFont *p,int x,int y,const char *text,DrawFontInfo *info)
{
	_glyph_drawmt dmt;
	uint32_t ucs;
	int ret,xx,yy,daku_y,daku_x;
	uint8_t daku_type,daku_have_left = 0;

	if(!p || !text) return;

	daku_type = info->dakuten_combine;
	y += p->baseline;

	xx = x;
	yy = y;

	if(info->flags & DRAWFONT_F_VERT)
	{
		//縦書き

		while(*text)
		{
			if(*text == '\n')
			{
				xx -= p->vertHeight + info->line_space;
				yy = y;
				daku_have_left = 0;
				 
				text++;
			}
			else
			{
				ret = mUTF8ToUCS4Char(text, -1, &ucs, &text);
				if(ret < 0) break;
				else if(ret > 0) continue;

				//濁点/半濁点を合成用文字に置換

				if(daku_type)
				{
					if(ucs == 0x309B)
						ucs = 0x3099;
					else if(ucs == 0x309C)
						ucs = 0x309A;
				}

				//描画

				if((ucs == 0x3099 || ucs == 0x309A) && daku_type)
				{
					//合成用濁点/半濁点

					if(daku_have_left)
					{
						if(_getBitmapGlyph(g_ftlib, p, ucs,
							(daku_type == DRAWFONT_DAKUTEN_REPLACE_HORZ)? 0: GETGLYPH_F_VERT, &dmt))
						{
							_drawChar(p, daku_x + dmt.left, daku_y + dmt.top, info);

							daku_have_left = 0;
						}
					}
				}
				else
				{
					//通常
										
					if(_getBitmapGlyph(g_ftlib, p, ucs, GETGLYPH_F_VERT, &dmt))
					{
						_drawChar(p, xx + dmt.left, yy + dmt.top, info);

						daku_have_left = 1;
						daku_x = xx;
						daku_y = yy;

						if(daku_type == DRAWFONT_DAKUTEN_REPLACE_HORZ)
							daku_x += p->face->size->metrics.y_ppem;
						
						if(dmt.advance)
							yy += dmt.advance + info->char_space;
					}
				}
			}
		}
	}
	else
	{
		//横書き

		while(*text)
		{
			if(*text == '\n')
			{
				xx = x;
				yy += p->horzHeight + info->line_space;
				daku_have_left = 0;
				 
				text++;
			}
			else
			{
				ret = mUTF8ToUCS4Char(text, -1, &ucs, &text);
				if(ret < 0) break;
				else if(ret > 0) continue; 

				//濁点/半濁点を合成用文字に置換

				if(daku_type)
				{
					if(ucs == 0x309B)
						ucs = 0x3099;
					else if(ucs == 0x309C)
						ucs = 0x309A;
				}

				//

				if(!_getBitmapGlyph(g_ftlib, p, ucs, 0, &dmt))
					continue;
				
				if((ucs == 0x3099 || ucs == 0x309A) && daku_type)
				{
					//濁点合成

					if(daku_have_left)
					{
						_drawChar(p, daku_x + dmt.left, daku_y + dmt.top, info);

						daku_have_left = 0;
					}
				}
				else
				{
					//通常
					
					_drawChar(p, xx + dmt.left, yy + dmt.top, info);

					daku_have_left = 1;
					daku_x = xx;
					daku_y = yy;
					
					//送り幅が0なら、字間は加算しない
					if(dmt.advance)
						xx += dmt.advance + info->char_space;
				}
			}
		}
	}
}


//=================================
// 描画サブ
//=================================


/** 文字コードからビットマップグリフ取得 */

mBool _getBitmapGlyph(FT_Library lib,DrawFont *p,uint32_t code,int flags,
	_glyph_drawmt *drawmt)
{
	FT_Face face = p->face;
	mFreeTypeInfo *info = &p->info;
	FT_UInt gindex;
	uint32_t loadflags;
	
	//コードからグリフインデックス取得
	
	gindex = FT_Get_Char_Index(face, code);
	if(gindex == 0) return FALSE;

	//縦書きグリフに置換

	if(flags & GETGLYPH_F_VERT)
		gindex = mFTGSUB_getVertGlyph(p->vert_tbl, gindex);
	
	//グリフスロットにロード

	loadflags = info->fLoadGlyph;
	
	if((flags & GETGLYPH_F_VERT) && p->has_vert)
		loadflags |= FT_LOAD_VERTICAL_LAYOUT;
	
	if(FT_Load_Glyph(face, gindex, loadflags))
		return FALSE;

	//

	if(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
	{
		//------ アウトライン

		//太字化

		if(info->flags & MFTINFO_F_EMBOLDEN)
			FT_Outline_Embolden(&face->glyph->outline, 1<<6);

		//斜体
		
		if(info->flags & MFTINFO_F_MATRIX)
			FT_Outline_Transform(&face->glyph->outline, &info->matrix);

		//ビットマップに変換

		if(FT_Render_Glyph(face->glyph, info->nRenderMode))
			return FALSE;
	}
	else
	{
		//------ ビットマップ

		//太字化

		if(info->flags & MFTINFO_F_EMBOLDEN)
		{
			FT_GlyphSlot_Own_Bitmap(face->glyph);
			FT_Bitmap_Embolden(g_ftlib, &face->glyph->bitmap, 1<<6, 0);
		}
	}

	//

	drawmt->left = face->glyph->bitmap_left;
	drawmt->top = -(face->glyph->bitmap_top);

	if(!(flags & GETGLYPH_F_VERT))
		//横書き
		drawmt->advance = _round_fix6(face->glyph->advance.x);
	else if(p->has_vert)
		//縦書き:フォントに縦書きレイアウトあり
		drawmt->advance = _round_fix6(face->glyph->advance.y);
	else
		//縦書き:縦書きレイアウトなし -> 全角幅
		drawmt->advance = face->size->metrics.y_ppem;

	return TRUE;
}

/** 1文字描画 */

void _drawChar(DrawFont *p,int x,int y,DrawFontInfo *info)
{
	FT_Bitmap *bm;
	uint8_t *pbuf,*pb;
	int ix,iy,w,h,pitch,f;

	bm = &p->face->glyph->bitmap;;
	
	w = bm->width;
	h = bm->rows;
	pbuf  = bm->buffer;
	pitch = bm->pitch;
	
	if(pitch < 0) pbuf += -pitch * (h - 1);

	//

	if(bm->pixel_mode == FT_PIXEL_MODE_MONO)
	{
		//1bit モノクロ

		for(iy = 0; iy < h; iy++, pbuf += pitch)
		{
			for(ix = 0, f = 0x80, pb = pbuf; ix < w; ix++)
			{
				if(*pb & f)
					(info->setpixel)(x + ix, y + iy, 255, info->param);
				
				f >>= 1;
				if(!f) { f = 0x80; pb++; }
			}
		}
	}
	else if(bm->pixel_mode == FT_PIXEL_MODE_GRAY)
	{
		//8bit グレイスケール
		
		pitch -= w;

		for(iy = 0; iy < h; iy++, pbuf += pitch)
		{
			for(ix = 0; ix < w; ix++, pbuf++)
			{
				if(*pbuf)
					(info->setpixel)(x + ix, y + iy, *pbuf, info->param);
			}
		}
	}
}
