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

#ifndef MLIB_EDITTEXTBUF_H
#define MLIB_EDITTEXTBUF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mEditTextBuf
{
	mWidget *widget;
	uint32_t *text;
	uint16_t *widthbuf;
	int textLen,
		allocLen,
		curPos,
		curLine,
		selTop,
		selEnd,
		maxwidth,
		maxlines,
		multi_curx,
		multi_curline;
	mBool bMulti;
}mEditTextBuf;

enum
{
	MEDITTEXTBUF_KEYPROC_CURSOR = 1,
	MEDITTEXTBUF_KEYPROC_TEXT,
	MEDITTEXTBUF_KEYPROC_UPDATE
};


mEditTextBuf *mEditTextBuf_new(mWidget *widget,mBool multiline);
void mEditTextBuf_free(mEditTextBuf *p);

mBool mEditTextBuf_resize(mEditTextBuf *p,int len);
void mEditTextBuf_empty(mEditTextBuf *p);
mBool mEditTextBuf_isEmpty(mEditTextBuf *p);

mBool mEditTextBuf_isSelectAtPos(mEditTextBuf *p,int pos);
void mEditTextBuf_cursorToBottom(mEditTextBuf *p);
int mEditTextBuf_getPosAtPixel_forMulti(mEditTextBuf *p,int x,int y,int lineh);

void mEditTextBuf_setText(mEditTextBuf *p,const char *text);
mBool mEditTextBuf_insertText(mEditTextBuf *p,const char *text,char ch);

int mEditTextBuf_keyProc(mEditTextBuf *p,uint32_t key,uint32_t state,int editok);

mBool mEditTextBuf_selectAll(mEditTextBuf *p);
mBool mEditTextBuf_deleteSelText(mEditTextBuf *p);
void mEditTextBuf_deleteText(mEditTextBuf *p,int pos,int len);
void mEditTextBuf_expandSelect(mEditTextBuf *p,int pos);
void mEditTextBuf_moveCursorPos(mEditTextBuf *p,int pos,int select);
void mEditTextBuf_dragExpandSelect(mEditTextBuf *p,int pos);

mBool mEditTextBuf_paste(mEditTextBuf *p);
void mEditTextBuf_copy(mEditTextBuf *p);
mBool mEditTextBuf_cut(mEditTextBuf *p);

#ifdef __cplusplus
}
#endif

#endif
