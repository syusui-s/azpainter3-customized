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

#ifndef MLIB_INPUTACCELKEY_H
#define MLIB_INPUTACCELKEY_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_INPUTACCELKEY(p)  ((mInputAccelKey *)(p))

typedef struct
{
	char *text;
	uint32_t style,
		key,keyprev;
	mBool bKeyDown;
}mInputAccelKeyData;

typedef struct _mInputAccelKey
{
	mWidget wg;
	mInputAccelKeyData ak;
}mInputAccelKey;


enum MINPUTACCELKEY_STYLE
{
	MINPUTACCELKEY_S_NOTIFY_CHANGE = 1<<0
};

enum MINPUTACCELKEY_NOTIFY
{
	MINPUTACCELKEY_N_CHANGE_KEY
};


void mInputAccelKeyDestroyHandle(mWidget *p);
void mInputAccelKeyCalcHintHandle(mWidget *p);
void mInputAccelKeyDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mInputAccelKeyEventHandle(mWidget *wg,mEvent *ev);

mInputAccelKey *mInputAccelKeyNew(int size,mWidget *parent,uint32_t style);
mInputAccelKey *mInputAccelKeyCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginb4);

uint32_t mInputAccelKey_getKey(mInputAccelKey *p);
void mInputAccelKey_setKey(mInputAccelKey *p,uint32_t key);

#ifdef __cplusplus
}
#endif

#endif
