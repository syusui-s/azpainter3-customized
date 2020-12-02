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

#ifndef MLIB_TEXTBUTTONS_H
#define MLIB_TEXTBUTTONS_H

#include "mWidgetDef.h"
#include "mListDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_TEXTBUTTONS(p)  ((mTextButtons *)(p))

typedef struct
{
	mList list;
	int focusno,
		flags,presskey;
}mTextButtonsData;

typedef struct _mTextButtons
{
	mWidget wg;
	mTextButtonsData tb;
}mTextButtons;


enum MTEXTBUTTONS_NOTIFY
{
	MTEXTBUTTONS_N_PRESS
};


void mTextButtonsDestroyHandle(mWidget *p);
void mTextButtonsCalcHintHandle(mWidget *p);
void mTextButtonsDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mTextButtonsEventHandle(mWidget *wg,mEvent *ev);

mTextButtons *mTextButtonsNew(int size,mWidget *parent,uint32_t style);

void mTextButtonsDeleteAll(mTextButtons *p);
void mTextButtonsAddButton(mTextButtons *p,const char *text);
void mTextButtonsAddButtonsTr(mTextButtons *p,int idtop,int num);

#ifdef __cplusplus
}
#endif

#endif
