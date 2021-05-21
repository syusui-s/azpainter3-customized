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

/************************************
 * グラデーションデータ
 ************************************/

typedef struct _RGBcombo RGBcombo;

#define GRADITEM(p)     ((GradItem *)(p))

#define GRAD_LIST_MAX_NUM  250  //アイテム最大数 [!] 255個まで
#define GRAD_DAT_MAX_POINT 30	//ポイントの最大数

#define GRAD_DAT_F_LOOP        1	//繰り返し
#define GRAD_DAT_F_SINGLE_COL  2	//単色

#define GRAD_DAT_POS_BIT   10		//位置のビット数
#define GRAD_DAT_POS_VAL   (1 << GRAD_DAT_POS_BIT)
#define GRAD_DAT_POS_MASK  ((1<<11) - 1) //位置を取得するマスク (RGB色の位置時)

#define GRAD_DAT_ALPHA_BIT  10	//アルファ値のビット数


typedef struct _GradItem
{
	mListItem i;
	uint8_t *buf;
	int size,
		id; //識別ID。追加時に使用されていないIDが使われる。
}GradItem;


/* list */

void GradationList_init(mList *list);

void GradationList_loadConfig(mList *list);
void GradationList_saveConfig(mList *list);

void GradationList_loadFile(mList *list,const char *filename);
void GradationList_saveFile(mList *list,const char *filename);

GradItem *GradationList_newItem(mList *list);
GradItem *GradationList_copyItem(mList *list,GradItem *item);

GradItem *GradationList_getItem_id(mList *list,int id);
uint8_t *GradationList_getBuf_fromID(mList *list,int id);

/* GradItem */

void GradItem_setData(GradItem *p,uint8_t flags,int colnum,int anum,uint8_t *colbuf,uint8_t *abuf);
void GradItem_drawPixbuf(mPixbuf *pixbuf,int x,int y,int w,int h,uint8_t *colbuf,uint8_t *abuf,int single_col);

/* other */

const uint8_t *Gradation_getData_draw_to_bkgnd(void);
const uint8_t *Gradation_getData_black_to_white(void);
const uint8_t *Gradation_getData_white_to_black(void);

uint8_t *Gradation_convertData_draw_8bit(const uint8_t *src,RGBcombo *drawcol,RGBcombo *bkgndcol);
uint8_t *Gradation_convertData_draw_16bit(const uint8_t *src,RGBcombo *drawcol,RGBcombo *bkgndcol);

