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

#ifndef MLIB_FONTBUTTON_H
#define MLIB_FONTBUTTON_H

#include "mButton.h"
#include "mFont.h"

#define M_FONTBUTTON(p)  ((mFontButton *)(p))

typedef struct
{
	mFontInfo info;
	uint32_t mask;
}mFontButtonData;

typedef struct _mFontButton
{
	mWidget wg;
	mButtonData btt;
	mFontButtonData fbt;
}mFontButton;


enum MFONTBUTTON_NOTIFY
{
	MFONTBUTTON_N_CHANGEFONT
};

/*--------*/

#ifdef __cplusplus
extern "C" {
#endif

void mFontButtonDestroyHandle(mWidget *p);

mFontButton *mFontButtonCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mFontButton *mFontButtonNew(int size,mWidget *parent,uint32_t style);

void mFontButtonSetMask(mFontButton *p,uint32_t mask);
void mFontButtonSetInfoFormat(mFontButton *p,const char *text);

void mFontButtonGetInfoFormat_str(mFontButton *p,mStr *str);

#ifdef __cplusplus
}
#endif

#endif
