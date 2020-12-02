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

#ifndef MLIB_LINEEDIT_H
#define MLIB_LINEEDIT_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_LINEEDIT(p)  ((mLineEdit *)(p))

typedef struct _mEditTextBuf mEditTextBuf;

typedef struct
{
	mEditTextBuf *buf;

	uint32_t style;
	int curX,scrX,
		fpress,posLast,
		numMin,numMax,numDig,spin_val;
	uint8_t padding;
}mLineEditData;

typedef struct _mLineEdit
{
	mWidget wg;
	mLineEditData le;
}mLineEdit;


enum MLINEEDIT_STYLE
{
	MLINEEDIT_S_NOTIFY_CHANGE = 1<<0,
	MLINEEDIT_S_NOTIFY_ENTER  = 1<<1,
	MLINEEDIT_S_READONLY = 1<<2,
	MLINEEDIT_S_SPIN     = 1<<3,
	MLINEEDIT_S_NO_FRAME = 1<<4
};

enum MLINEEDIT_NOTIFY
{
	MLINEEDIT_N_CHANGE,
	MLINEEDIT_N_ENTER
};


void mLineEditDestroyHandle(mWidget *p);
void mLineEditCalcHintHandle(mWidget *p);
void mLineEditDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mLineEditEventHandle(mWidget *wg,mEvent *ev);

mLineEdit *mLineEditCreate(mWidget *parent,int id,uint32_t style,uint32_t fLayout,uint32_t marginB4);

mLineEdit *mLineEditNew(int size,mWidget *parent,uint32_t style);

void mLineEditSetNumStatus(mLineEdit *p,int min,int max,int dig);
void mLineEditSetWidthByLen(mLineEdit *p,int len);

void mLineEditSetText(mLineEdit *p,const char *text);
void mLineEditSetText_ucs4(mLineEdit *p,const uint32_t *text);
void mLineEditSetInt(mLineEdit *p,int n);
void mLineEditSetNum(mLineEdit *p,int num);
void mLineEditSetDouble(mLineEdit *p,double d,int dig);

mBool mLineEditIsEmpty(mLineEdit *p);
void mLineEditGetTextStr(mLineEdit *p,mStr *str);
int mLineEditGetInt(mLineEdit *p);
double mLineEditGetDouble(mLineEdit *p);
int mLineEditGetNum(mLineEdit *p);

void mLineEditSelectAll(mLineEdit *p);
void mLineEditCursorToRight(mLineEdit *p);

#ifdef __cplusplus
}
#endif

#endif
