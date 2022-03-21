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

#ifndef MLK_LINEEDIT_H
#define MLK_LINEEDIT_H

#define MLK_LINEEDIT(p)  ((mLineEdit *)(p))
#define MLK_LINEEDIT_DEF mWidget wg; mLineEditData le;

typedef struct
{
	mWidgetTextEdit dat;
	mWidgetLabelText label;
	uint32_t fstyle;
	int scrx,
		pos_last,
		num_min, num_max, num_dig,
		spin_val,
		fpress;
}mLineEditData;

struct _mLineEdit
{
	mWidget wg;
	mLineEditData le;
};


enum MLINEEDIT_STYLE
{
	MLINEEDIT_S_NOTIFY_CHANGE = 1<<0,
	MLINEEDIT_S_NOTIFY_ENTER  = 1<<1,
	MLINEEDIT_S_READ_ONLY = 1<<2,
	MLINEEDIT_S_SPIN     = 1<<3,
	MLINEEDIT_S_NO_FRAME = 1<<4
};

enum MLINEEDIT_NOTIFY
{
	MLINEEDIT_N_CHANGE,
	MLINEEDIT_N_ENTER
};


#ifdef __cplusplus
extern "C" {
#endif

mLineEdit *mLineEditNew(mWidget *parent,int size,uint32_t fstyle);
mLineEdit *mLineEditCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);
void mLineEditDestroy(mWidget *p);

void mLineEditHandle_calcHint(mWidget *p);
void mLineEditHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mLineEditHandle_event(mWidget *wg,mEvent *ev);

void mLineEditSetLabelText(mLineEdit *p,const char *text,int copy);
void mLineEditSetNumStatus(mLineEdit *p,int min,int max,int dig);
void mLineEditSetWidth_textlen(mLineEdit *p,int len);
void mLineEditSetSpinValue(mLineEdit *p,int val);
void mLineEditSetReadOnly(mLineEdit *p,int type);

void mLineEditSetText(mLineEdit *p,const char *text);
void mLineEditSetText_utf32(mLineEdit *p,const mlkuchar *text);
void mLineEditSetInt(mLineEdit *p,int n);
void mLineEditSetNum(mLineEdit *p,int num);
void mLineEditSetDouble(mLineEdit *p,double d,int dig);

mlkbool mLineEditIsEmpty(mLineEdit *p);
void mLineEditGetTextStr(mLineEdit *p,mStr *str);
int mLineEditGetInt(mLineEdit *p);
double mLineEditGetDouble(mLineEdit *p);
int mLineEditGetNum(mLineEdit *p);

void mLineEditSelectAll(mLineEdit *p);
void mLineEditMoveCursor_toRight(mLineEdit *p);

#ifdef __cplusplus
}
#endif

#endif
