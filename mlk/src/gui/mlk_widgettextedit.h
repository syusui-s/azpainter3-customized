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

#ifndef MLK_WIDGET_TEXT_EDIT_H
#define MLK_WIDGET_TEXT_EDIT_H

typedef struct _mWidgetTextEdit mWidgetTextEdit;

typedef struct
{
	uint32_t *text;
	int len,width,is_sel;
}mWidgetTextEditState;

enum
{
	MWIDGETTEXTEDIT_KEYPROC_CHANGE_CURSOR = 1,
	MWIDGETTEXTEDIT_KEYPROC_CHANGE_TEXT,
	MWIDGETTEXTEDIT_KEYPROC_UPDATE
};


void mWidgetTextEdit_free(mWidgetTextEdit *p);
void mWidgetTextEdit_init(mWidgetTextEdit *p,mWidget *widget,mlkbool multi_line);

mlkbool mWidgetTextEdit_resize(mWidgetTextEdit *p,int len);
void mWidgetTextEdit_empty(mWidgetTextEdit *p);
mlkbool mWidgetTextEdit_isEmpty(mWidgetTextEdit *p);

mlkbool mWidgetTextEdit_isSelected_atPos(mWidgetTextEdit *p,int pos);
void mWidgetTextEdit_moveCursor_toBottom(mWidgetTextEdit *p);
int mWidgetTextEdit_multi_getPosAtPixel(mWidgetTextEdit *p,int x,int y,int lineh);
int mWidgetTextEdit_getLineState(mWidgetTextEdit *p,int top,int end,mWidgetTextEditState *dst);

void mWidgetTextEdit_setText(mWidgetTextEdit *p,const char *text);
mlkbool mWidgetTextEdit_insertText(mWidgetTextEdit *p,const char *text,char ch);

mlkbool mWidgetTextEdit_selectAll(mWidgetTextEdit *p);
mlkbool mWidgetTextEdit_deleteSelText(mWidgetTextEdit *p);
void mWidgetTextEdit_deleteText(mWidgetTextEdit *p,int pos,int len);
void mWidgetTextEdit_expandSelect(mWidgetTextEdit *p,int pos);
void mWidgetTextEdit_moveCursorPos(mWidgetTextEdit *p,int pos,int select);
void mWidgetTextEdit_dragExpandSelect(mWidgetTextEdit *p,int pos);

mlkbool mWidgetTextEdit_paste(mWidgetTextEdit *p);
void mWidgetTextEdit_copy(mWidgetTextEdit *p);
mlkbool mWidgetTextEdit_cut(mWidgetTextEdit *p);

int mWidgetTextEdit_keyProc(mWidgetTextEdit *p,uint32_t key,uint32_t state,int enable_edit);

#endif
