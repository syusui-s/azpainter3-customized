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

#ifndef MLK_GROUPBOX_H
#define MLK_GROUPBOX_H

#define MLK_GROUPBOX(p)  ((mGroupBox *)(p))
#define MLK_GROUPBOX_DEF mWidget wg; mContainerData ct; mGroupBoxData gb;

typedef struct
{
	mWidgetLabelText txt;
	mWidgetRect padding;
	uint32_t fstyle;
}mGroupBoxData;

struct _mGroupBox
{
	mWidget wg;
	mContainerData ct;
	mGroupBoxData gb;
};

enum MGROUPBOX_STYLE
{
	MGROUPBOX_S_COPYTEXT = 1<<0
};


#ifdef __cplusplus
extern "C" {
#endif

mGroupBox *mGroupBoxNew(mWidget *parent,int size,uint32_t fstyle);
mGroupBox *mGroupBoxCreate(mWidget *parent,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text);
void mGroupBoxDestroy(mWidget *wg);

void mGroupBoxHandle_calcHint(mWidget *wg);
void mGroupBoxHandle_draw(mWidget *wg,mPixbuf *pixbuf);

void mGroupBoxSetPadding_pack4(mGroupBox *p,uint32_t pack);
void mGroupBoxSetText(mGroupBox *p,const char *text);
void mGroupBoxSetText_copy(mGroupBox *p,const char *text);

#ifdef __cplusplus
}
#endif

#endif
