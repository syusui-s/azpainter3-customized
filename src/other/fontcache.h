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

/***************************
 * フォントキャッシュ
 ***************************/

//フォントのサブデータ

struct _mFTSubData
{
	int is_cid;			//CID フォントか
	mOT_VORG vorg;		//縦書き原点位置
	uint8_t *gsub_vert,	//縦書き置き換え
		*gsub_ruby,		//ルビ文字置き換え
		*gsub_hwid,		//プロポーショナル -> 等幅
		*gsub_twid;		//1/3幅
};


void FontCache_init(void);
void FontCache_free(void);

mFont *FontCache_loadFont(const char *filename,int index);
mFont *FontCache_loadFont_family(const char *family,const char *style);

void FontCache_releaseFont(mFont *font);
