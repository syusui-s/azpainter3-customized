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
 * テキスト描画用データ
 *****************************************/

typedef struct _DrawTextData DrawTextData;

/* def_draw.h 内でインクルードされる */

struct _DrawTextData
{
	mStr str_text,		//テキスト文字列
		str_font,		//フォント名/フォントファイル名
		str_style;		//フォントスタイル
	int32_t font_param,		//選択フォントのパラメータ ([登録フォント] ID [フォント指定] インデックス)
		fontsize,		//フォントサイズ ([pt] 1=0.1 [px] 1=1)
		rubysize,		//ルビサイズ
		dpi,			//DPI
		char_space,		//字間
		line_space,		//行間
		ruby_pos,		//ルビ位置
		angle;			//回転角度
	uint32_t flags;		//フラグ
	uint8_t fontsel,	//フォント選択 ([0]一覧 [1]登録フォント [2]フォント指定)
		unit_fontsize,	//フォントサイズの単位
		unit_rubysize,	//ルビサイズの単位
		unit_char_space,
		unit_line_space,
		unit_ruby_pos,	//ルビ位置
		hinting;		//ヒンティング
};

enum
{
	DRAWTEXT_FONTSEL_LIST,
	DRAWTEXT_FONTSEL_REGIST,
	DRAWTEXT_FONTSEL_FILE
};

enum
{
	DRAWTEXT_UNIT_FONTSIZE_PT,
	DRAWTEXT_UNIT_FONTSIZE_PX
};

enum
{
	DRAWTEXT_UNIT_RUBYSIZE_PERS,
	DRAWTEXT_UNIT_RUBYSIZE_PT,
	DRAWTEXT_UNIT_RUBYSIZE_PX
};

enum
{
	DRAWTEXT_F_ENABLE_DPI = 1<<0,		//DPI 指定有効
	DRAWTEXT_F_DISABLE_AUTOHINT = 1<<1,	//オートヒント無効
	DRAWTEXT_F_MONO = 1<<2,				//モノクロ2値
	DRAWTEXT_F_VERT = 1<<3,				//縦書き
	DRAWTEXT_F_ENABLE_FORMAT = 1<<4,	//特殊表記有効
	DRAWTEXT_F_BOLD = 1<<5,				//太字化
	DRAWTEXT_F_ITALIC = 1<<6,			//斜体化
	DRAWTEXT_F_ENABLE_BITMAP = 1<<7,	//埋め込みビットマップ有効
	DRAWTEXT_F_NO_RUBY_GLYPH = 1<<8		//ルビ用グリフを使わない
};

/* draw_main.c */
void DrawTextData_free(DrawTextData *p);
void DrawTextData_copy(DrawTextData *dst,const DrawTextData *src);

