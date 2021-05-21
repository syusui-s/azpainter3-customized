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
 * 登録フォント
 *******************************/

#define REGFONT_MAX_NUM  512	//フォント最大数
#define REGFONT_CODE_F_RANGE 0x80000000
#define REGFONT_CODE_MASK    0x7fffffff

typedef struct
{
	uint8_t *charbuf;	//rep1+rep1
	mStr strname,
		str_basefont,
		str_repfont[2];
	uint16_t id,
		charflags[2],
		charsize[2];	//文字データのサイズ
	uint32_t index_base,
		index_rep[2];
}RegFontDat;

enum
{
	REGFONT_CHARF_LATIN = 1<<0,		//基本ラテン文字
	REGFONT_CHARF_HIRA = 1<<1,		//ひらがな
	REGFONT_CHARF_KANA = 1<<2,		//カタカナ
	REGFONT_CHARF_KANJI = 1<<3,		//漢字
	REGFONT_CHARF_KUDOKUTEN = 1<<4,	//句読点など
	REGFONT_CHARF_GAIJI = 1<<5,		//外字

	REGFONT_CHARF_CODE_CID = 1<<15,	//コード値はCID

	REGFONT_CHARF_MASK = 0x7fff
};

//-------------

void RegFont_init(void);
void RegFont_free(void);

void RegFont_loadConfigFile(void);
void RegFont_saveConfigFile(void);
mlkerr RegFont_loadFile(const char *filename);
void RegFont_saveFile(const char *filename);

mListItem *RegFont_getTopItem(void);
const char *RegFont_getNamePtr(mListItem *pi);
int RegFont_getItemID(mListItem *pi);

void RegFont_clear(void);
mListItem *RegFont_addDat(RegFontDat *p);
mListItem *RegFont_replaceDat(mListItem *pi,RegFontDat *dat);
mListItem *RegFont_findID(int id);

void RegFontDat_free(RegFontDat *p);
RegFontDat *RegFontDat_new(void);
RegFontDat *RegFontDat_dup(const RegFontDat *src);
RegFontDat *RegFontDat_make(mListItem *item);

void RegFontDat_setCharBuf(RegFontDat *p,int no,mBuf *buf);
mlkbool RegFontDat_getCodeText(RegFontDat *p,int no,mStr *str);
int RegFontDat_text_to_charbuf(mBuf *buf,const char *text,mlkbool hex);

mlkbool RegFontCode_checkFlags(uint32_t code,uint16_t flags);
mlkbool RegFontCode_checkBuf(uint32_t *buf,int len,uint32_t code);
