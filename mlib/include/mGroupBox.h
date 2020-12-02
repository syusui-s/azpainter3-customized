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

#ifndef MLIB_GROUPBOX_H
#define MLIB_GROUPBOX_H

#include "mContainerDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_GROUPBOX(p)  ((mGroupBox *)(p))

typedef struct
{
	char *label;
	int labelW,labelH;
}mGroupBoxData;

typedef struct _mGroupBox
{
	mWidget wg;
	mContainerData ct;
	mGroupBoxData gb;
}mGroupBox;


mGroupBox *mGroupBoxNew(int size,mWidget *parent,uint32_t style);
mGroupBox *mGroupBoxCreate(mWidget *parent,uint32_t style,uint32_t fLayout,uint32_t marginB4,const char *label);

void mGroupBoxSetLabel(mGroupBox *p,const char *text);

#ifdef __cplusplus
}
#endif

#endif
