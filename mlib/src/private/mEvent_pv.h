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

#ifndef MLIB_EVENT_PV_H
#define MLIB_EVENT_PV_H

#ifdef __cplusplus
extern "C" {
#endif

void __mEventAppendEnterLeave(mWidget *src,mWidget *dst);
void __mEventUngrabPointer(mWidget *p);

mWidget *__mEventGetPointerWidget(mWindow *win,int x,int y);
mWidget *__mEventGetKeySendWidget(mWindow *win,mWindow *popup);

mWidget *__mEventProcKey(mWindow *win,mWindow *popup,uint32_t keycode,uint32_t state,int press);

void __mEventAppendFocusOut_ungrab(mWindow *win);
void __mEventProcFocus(mWindow *win,int focusin);

void __mEventPutDebug(mEvent *p);

mBool __mAppAfterEvent(void);

#ifdef __cplusplus
}
#endif

#endif
