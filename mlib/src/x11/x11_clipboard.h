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

#ifndef MLIB_X11_CLIPBOARD_H
#define MLIB_X11_CLIPBOARD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mX11Clipboard mX11Clipboard;

typedef struct
{
	void *buf;
	uint32_t size;
	Window winid;
	Atom prop,type;
	mBool multiple;
}mX11Clipboard_cb;

enum
{
	MX11CLIPB_DATTYPE_NONE,
	MX11CLIPB_DATTYPE_TEXT
};


mX11Clipboard *mX11ClipboardNew(void);
void mX11ClipboardDestroy(mX11Clipboard *p);
void mX11ClipboardFreeDat(mX11Clipboard *p);

mBool mX11ClipboardSetDat(mX11Clipboard *p,
	mWindow *win,int dattype,const void *datbuf,uint32_t datsize,
	Atom *atom_list,int atom_num,
	int (*callback)(mX11Clipboard *,mX11Clipboard_cb *));

char *mX11ClipboardGetText(mX11Clipboard *p,mWindow *win);

mBool mX11ClipboardSaveManager(mX11Clipboard *p,mWindow *win,int dattype);

void mX11ClipboardSelectionRequest(mX11Clipboard *p,XSelectionRequestEvent *ev);

#ifdef __cplusplus
}
#endif

#endif
