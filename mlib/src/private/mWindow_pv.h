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

#ifndef MLIB_WINDOW_PV_H
#define MLIB_WINDOW_PV_H

#ifdef __cplusplus
extern "C" {
#endif

int __mWindowNew(mWindow *p);
void __mWindowDestroy(mWindow *p);

void __mWindowShow(mWindow *p);
void __mWindowHide(mWindow *p);
void __mWindowMinimize(mWindow *p,int type);
void __mWindowMaximize(mWindow *p,int type);

void __mWindowActivate(mWindow *p);
void __mWindowMove(mWindow *p,int x,int y);
void __mWindowResize(mWindow *p,int w,int h);

mBool __mWindowGrabPointer(mWindow *p,mCursor cur,int device_id);
void __mWindowUngrabPointer(mWindow *p);
mBool __mWindowGrabKeyboard(mWindow *p);
void __mWindowUngrabKeyboard(mWindow *p);

void __mWindowSetCursor(mWindow *win,mCursor cur);
void __mWindowSetIcon(mWindow *win,mImageBuf *img);

//

void __mWindowOnResize(mWindow *p,int w,int h);

#ifdef __cplusplus
}
#endif

#endif
