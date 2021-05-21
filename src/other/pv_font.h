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

/*******************************
 * フォント 内部用
 *******************************/

typedef struct
{
	mBuf buf;
	mList list_ruby;
}DrawFontString;

typedef struct
{
	mListItem i;
	int parent_pos,	//RUBY_PARENT_START の次のバッファ位置
		ruby_pos,	//RUBY_START の次のバッファ位置
		x,y,		//親文字列の位置
		parent_w;	//親文字列の幅
}RubyItem;

#define CHAR_GET_CODE(c)  ((c) & 0xffffff)

/* 文字フラグ */

#define CHAR_F_COMMAND  0x80000000	//ON で下位はコマンドID
#define CHAR_F_CID      (1<<30)		//ON=CID, OFF=Unicode
#define CHAR_F_DAKUTEN  (1<<29)		//濁点
#define CHAR_F_HANDAKUTEN (1<<28)	//半濁点

/* コマンド値 */

enum
{
	CHAR_CMD_ID_ENTER,			//改行
	CHAR_CMD_ID_YOKO1_START,	//縦中横開始
	CHAR_CMD_ID_YOKO2_START,	//縦中横開始(半角幅)
	CHAR_CMD_ID_YOKO3_START,	//縦中横開始(1/3幅)
	CHAR_CMD_ID_YOKO_END,		//縦中横終了
	CHAR_CMD_ID_RUBY_PARENT_START,	//ルビ親文字列の開始
	CHAR_CMD_ID_RUBY_START,		//ルビ文字の開始
	CHAR_CMD_ID_RUBY_END		//ルビ文字の終了
};

#define CHAR_CMDVAL_ENTER		(CHAR_F_COMMAND)
#define CHAR_CMDVAL_YOKO_END	(CHAR_F_COMMAND + CHAR_CMD_ID_YOKO_END)
#define CHAR_CMDVAL_RUBY_END	(CHAR_F_COMMAND + CHAR_CMD_ID_RUBY_END)

/*------*/

void __DrawFont_getString(DrawFontString *dst,const char *text,int enable_format);
void __DrawFont_freeString(DrawFontString *p);

void __DrawFont_setRubyParentPos(DrawFontString *p,uint32_t *ptr,int x,int y);
void __DrawFont_setRubyParentW(DrawFontString *p,uint32_t *ptr,int width);

