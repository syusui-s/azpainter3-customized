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

#ifndef MLK_X11_EVENT_H
#define MLK_X11_EVENT_H

typedef struct
{
	XEvent *xev;
	mWindow *win;		//イベントの対象ウィンドウ

	int waitev_type;		//待ちイベントタイプ (MLKX11_WAITEVENT_*)
	mlkbool waitev_check,	//待ちイベントをチェックするか (各イベントごとで対象のウィンドウか)
		waitev_flag;		//待ちイベントが来たかどうかの結果フラグ
}_X11_EVENT;


/* mlk_x11_event_sub.c */

mWindow *mX11Event_getWindow(mAppX11 *p,Window id);
void mX11Event_setTime(mAppX11 *p,XEvent *ev);
int mX11Event_convertButton(int btt);
uint32_t mX11Event_convertState(unsigned int state);

char *mX11Event_key_getIMString(_X11_EVENT *p,XKeyEvent *xev,KeySym *keysym,char *dst_char,int *dst_len);
mlkbool mX11Event_checkDblClk(mWindow *win,int x,int y,int btt,Time time);
int mX11Event_proc_button(_X11_EVENT *p,int btt,uint32_t state,int press);

void mX11Event_clientmessage_wmprotocols(_X11_EVENT *p);
void mX11Event_property_netwmstate(_X11_EVENT *p);
void mX11Event_map_setRequest(mAppX11 *p,mWindow *win);

#endif
