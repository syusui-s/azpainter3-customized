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

/************************************************
 * グラデーション編集ダイアログのウィジェット
 ************************************************/

typedef struct _GradEditBar GradEditBar;

enum
{
	GRADEDITBAR_N_CHANGE_CURRENT,	//カレントポイントの変更
	GRADEDITBAR_N_MODIFY	//データが変更された
};


GradEditBar *GradEditBar_new(mWidget *parent,int id,mlkbool mode_alpha,uint8_t *buf);

void GradEditBar_setAnotherModeBuf(GradEditBar *p,uint8_t *buf);
void GradEditBar_setSingleColor(GradEditBar *p,int type);

int GradEditBar_getPointNum(GradEditBar *p);
uint8_t *GradEditBar_getBuf(GradEditBar *p);
int GradEditBar_getCurrentPos(GradEditBar *p);
int GradEditBar_getCurrentColor(GradEditBar *p,uint32_t *col);
int GradEditBar_getCurrentOpacity(GradEditBar *p);
int GradEditBar_getMaxSplitNum_toRight(GradEditBar *p);

mlkbool GradEditBar_setCurrentPos(GradEditBar *p,int pos);
void GradEditBar_setOpacity(GradEditBar *p,int val);
void GradEditBar_setColorType(GradEditBar *p,int type);
void GradEditBar_setColorRGB(GradEditBar *p,uint32_t col);

void GradEditBar_moveCurrentPoint(GradEditBar *p,mlkbool right);
void GradEditBar_deletePoint(GradEditBar *p,int index);
void GradEditBar_moveCurrentPos_middle(GradEditBar *p);
void GradEditBar_evenAllPoints(GradEditBar *p);
void GradEditBar_reversePoints(GradEditBar *p);
void GradEditBar_splitPoint_toRight(GradEditBar *p,int num);

