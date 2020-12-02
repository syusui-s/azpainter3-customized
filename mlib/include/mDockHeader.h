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

#ifndef MLIB_DOCKHEADER_H
#define MLIB_DOCKHEADER_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_DOCKHEADER(p)  ((mDockHeader *)(p))

typedef struct
{
	uint32_t style;
	int fpress;
	const char *title;
}mDockHeaderData;

typedef struct _mDockHeader
{
	mWidget wg;
	mDockHeaderData dh;
}mDockHeader;

enum
{
	MDOCKHEADER_S_HAVE_CLOSE  = 1<<0,
	MDOCKHEADER_S_HAVE_SWITCH = 1<<1,
	MDOCKHEADER_S_HAVE_FOLD   = 1<<2,
	MDOCKHEADER_S_FOLDED      = 1<<3,
	MDOCKHEADER_S_SWITCH_DOWN = 1<<4
};

#define MDOCKHEADER_N_BUTTON  0

enum
{
	MDOCKHEADER_BTT_CLOSE = 1,
	MDOCKHEADER_BTT_SWITCH,
	MDOCKHEADER_BTT_FOLD
};


void mDockHeaderDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mDockHeaderEventHandle(mWidget *wg,mEvent *ev);
void mDockHeaderCalcHintHandle(mWidget *wg);

mDockHeader *mDockHeaderNew(int size,mWidget *parent,uint32_t style);

void mDockHeaderSetTitle(mDockHeader *p,const char *title);
void mDockHeaderSetFolded(mDockHeader *p,int type);

#ifdef __cplusplus
}
#endif

#endif
