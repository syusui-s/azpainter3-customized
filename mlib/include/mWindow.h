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

#ifndef MLIB_WINDOW_H
#define MLIB_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

mWidget *mWindowNew(int size,mWindow *owner,uint32_t style);

mMenu *mWindowGetMenuInMenuBar(mWindow *p);
mWidget *mWindowGetFocusWidget(mWindow *p);

void mWindowActivate(mWindow *p);
void mWindowSetTitle(mWindow *p,const char *title);

void mWindowEnableDND(mWindow *p);
void mWindowEnablePenTablet(mWindow *p);

void mWindowSetIconFromFile(mWindow *p,const char *filename);
void mWindowSetIconFromBufPNG(mWindow *p,const void *buf,uint32_t bufsize);

mBool mWindowIsMinimized(mWindow *p);
mBool mWindowIsMaximized(mWindow *p);

mBool mWindowMinimize(mWindow *p,int type);
mBool mWindowMaximize(mWindow *p,int type);
void mWindowKeepAbove(mWindow *p,int type);
mBool mWindowFullScreen(mWindow *p,int type);

void mWindowMoveAdjust(mWindow *p,int x,int y,mBool workarea);
void mWindowMoveCenter(mWindow *p,mWindow *win);
void mWindowShowInit(mWindow *p,mBox *box,mBox *defbox,int defval,mBool show,mBool maximize);
void mWindowShowInitPos(mWindow *p,mPoint *pt,int defx,int defy,int defval,mBool show,mBool maximize);

void mWindowMoveResizeShow(mWindow *p,int w,int h);
void mWindowMoveResizeShow_hintSize(mWindow *p);

void mWindowAdjustPosDesktop(mWindow *p,mPoint *pt,mBool workarea);

void mWindowGetFrameWidth(mWindow *p,mRect *rc);
void mWindowGetFrameRootPos(mWindow *p,mPoint *pt);
void mWindowGetFullSize(mWindow *p,mSize *size);
void mWindowGetFullBox(mWindow *p,mBox *box);
void mWindowGetSaveBox(mWindow *p,mBox *box);

void mWindowUpdateRect(mWindow *p,mRect *rc);

#ifdef __cplusplus
}
#endif

#endif
