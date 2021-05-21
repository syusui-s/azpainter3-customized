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

/***************************
 * パレットリスト
 ***************************/

#define PALETTE_MAX_COLNUM  8192	//最大色数

/* パレットリストアイテム */

typedef struct
{
	mListItem i;

	char *name;			//名前
	uint8_t *palbuf;	//色バッファ (R-G-B)
	int palnum;			//色数
	uint16_t cellw,cellh,	//色の表示サイズ(px)
		xmaxnum;		//横の最大表示数 (0 で幅に合わせる)
}PaletteListItem;


void PaletteList_setModify(void);

void PaletteList_init(mList *list);
PaletteListItem *PaletteList_add(mList *list,const char *name);
void PaletteList_loadConfig(mList *list);
void PaletteList_saveConfig(mList *list);

void PaletteListItem_resize(PaletteListItem *pi,int num);
uint32_t PaletteListItem_getColor(PaletteListItem *pi,int no);
void PaletteListItem_setColor(PaletteListItem *pi,int no,uint32_t col);
void PaletteListItem_setAllColor(PaletteListItem *pi,uint8_t r,uint8_t g,uint8_t b);
void PaletteListItem_gradation(PaletteListItem *pi,int no1,int no2);
void PaletteListItem_setFromImage(PaletteListItem *pi,uint8_t *buf,int w,int h);

mlkbool PaletteListItem_loadFile(PaletteListItem *pi,const char *filename,mlkbool append);
mlkbool PaletteListItem_saveFile_apl(PaletteListItem *pi,const char *filename);
mlkbool PaletteListItem_saveFile_aco(PaletteListItem *pi,const char *filename);

