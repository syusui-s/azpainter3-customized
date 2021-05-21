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

#ifndef MLK_LABEL_H
#define MLK_LABEL_H

#define MLK_LABEL(p)  ((mLabel *)(p))
#define MLK_LABEL_DEF mWidget wg; mLabelData lb;

typedef struct
{
	mWidgetLabelText txt;
	uint32_t fstyle;
}mLabelData;

struct _mLabel
{
	mWidget wg;
	mLabelData lb;
};

enum MLABEL_STYLE
{
	MLABEL_S_COPYTEXT = 1<<0,
	MLABEL_S_RIGHT  = 1<<1,
	MLABEL_S_CENTER = 1<<2,
	MLABEL_S_BOTTOM = 1<<3,
	MLABEL_S_MIDDLE = 1<<4,
	MLABEL_S_BORDER = 1<<5
};


#ifdef __cplusplus
extern "C" {
#endif

mLabel *mLabelNew(mWidget *parent,int size,uint32_t fstyle);
mLabel *mLabelCreate(mWidget *parent,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text);
void mLabelDestroy(mWidget *p);

void mLabelHandle_calcHint(mWidget *p);
void mLabelHandle_draw(mWidget *p,mPixbuf *pixbuf);

void mLabelSetText(mLabel *p,const char *text);
void mLabelSetText_copy(mLabel *p,const char *text);
void mLabelSetText_int(mLabel *p,int val);
void mLabelSetText_floatint(mLabel *p,int val,int dig);

#ifdef __cplusplus
}
#endif

#endif
