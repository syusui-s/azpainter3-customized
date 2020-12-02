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

#ifndef MLIB_X11_EVENT_SUB_H
#define MLIB_X11_EVENT_SUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	XEvent *xev;
	mEvent *ev;
	mWindow *win;
	mAppRunDat *rundat;
	int modalskip;
}_X11_EVENT;

#define _IS_BLOCK_USERACTION  (MAPP->flags & MAPP_FLAGS_BLOCK_USER_ACTION)


mWindow *mX11Event_getWindow(Window id);
void mX11Event_setTime(XEvent *p);
uint32_t mX11Event_convertState(unsigned int state);
void mX11Event_change_NET_WM_STATE(_X11_EVENT *p);
void mX11Event_onMap(mWindow *p);

mWidget *mX11Event_procButton_PressRelease(_X11_EVENT *p,
	int x,int y,int rootx,int rooty,int btt,uint32_t state,Time time,mBool press,int *evtype);

mWidget *mX11Event_procMotion(_X11_EVENT *p,int x,int y,int rootx,int rooty,uint32_t state);

#ifdef __cplusplus
}
#endif

#endif
