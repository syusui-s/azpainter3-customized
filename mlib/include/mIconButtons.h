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

#ifndef MLIB_ICONBUTTONS_H
#define MLIB_ICONBUTTONS_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_ICONBUTTONS(p)  ((mIconButtons *)(p))

typedef struct _mToolTip mToolTip;

typedef struct
{
	uint32_t style;
	mList list;
	mImageList *imglist;
	void *itemOn;
	mToolTip *tooltip;
	int fpress;
	uint16_t trgroupid;
}mIconButtonsData;

typedef struct _mIconButtons
{
	mWidget wg;
	mIconButtonsData ib;
}mIconButtons;


enum MICONBUTTONS_STYLE
{
	MICONBUTTONS_S_TOOLTIP    = 1<<0,
	MICONBUTTONS_S_AUTO_WRAP  = 1<<1,
	MICONBUTTONS_S_VERT       = 1<<2,
	MICONBUTTONS_S_SEP_BOTTOM = 1<<3,
	MICONBUTTONS_S_DESTROY_IMAGELIST = 1<<4
};

enum MICONBUTTONS_ITEM_FLAGS
{
	MICONBUTTONS_ITEMF_BUTTON = 0,
	MICONBUTTONS_ITEMF_CHECKBUTTON = 1<<0,
	MICONBUTTONS_ITEMF_CHECKGROUP  = 1<<1,
	MICONBUTTONS_ITEMF_SEP      = 1<<2,
	MICONBUTTONS_ITEMF_DROPDOWN = 1<<3,
	MICONBUTTONS_ITEMF_WRAP     = 1<<4,
	MICONBUTTONS_ITEMF_DISABLE  = 1<<5,
	MICONBUTTONS_ITEMF_CHECKED  = 1<<6
};


void mIconButtonsDestroyHandle(mWidget *p);
void mIconButtonsDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mIconButtonsEventHandle(mWidget *wg,mEvent *ev);

mIconButtons *mIconButtonsNew(int size,mWidget *parent,uint32_t style);

void mIconButtonsSetImageList(mIconButtons *p,mImageList *img);
void mIconButtonsReplaceImageList(mIconButtons *p,mImageList *img);
void mIconButtonsSetTooltipTrGroup(mIconButtons *p,uint16_t gid);
void mIconButtonsCalcHintSize(mIconButtons *p);

void mIconButtonsDeleteAll(mIconButtons *p);
void mIconButtonsAdd(mIconButtons *p,int id,int img,int tooltip,uint32_t flags);
void mIconButtonsAddSep(mIconButtons *p);

void mIconButtonsSetCheck(mIconButtons *p,int id,int type);
mBool mIconButtonsIsCheck(mIconButtons *p,int id);
void mIconButtonsSetEnable(mIconButtons *p,int id,int type);

void mIconButtonsGetItemBox(mIconButtons *p,int id,mBox *box,mBool bRoot);

#ifdef __cplusplus
}
#endif

#endif
