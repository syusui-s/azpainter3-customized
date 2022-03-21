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

#ifndef MLK_PV_EVENT_H
#define MLK_PV_EVENT_H

//__mEventProcButton 時のフラグ
enum
{
	MEVENTPROC_BUTTON_F_PRESS = 1<<0,
	MEVENTPROC_BUTTON_F_DBLCLK = 1<<1,
	MEVENTPROC_BUTTON_F_ADD_PENTAB = 1<<2	//PENTAB イベントを生成
};

mlkbool __mEventIsModalSkip(mWindow *win);
mlkbool __mEventIsModalWindow(mWindow *win);
void __mEventOnUngrab(void);
char **__mEventGetURIList_ptr(uint8_t *datbuf);

void __mEventProcClose(mWindow *win);
void __mEventProcConfigure(mWindow *win,int w,int h,uint32_t state,uint32_t state_mask,uint32_t flags);
void __mEventProcFocusIn(mWindow *win);
void __mEventProcFocusOut(mWindow *win);
void __mEventProcEnter(mWindow *win,int fx,int fy);
void __mEventProcLeave(mWindow *win);
void __mEventReEnter(void);

mWidget *__mEventProcKey(mWindow *win,uint32_t key,uint32_t rawcode,uint32_t state,int press,mlkbool key_repeat);

void __mEventProcScroll(mWindow *win,int hval,int vval,int hstep,int vstep,uint32_t state);
mWidget *__mEventProcButton(mWindow *win,int fx,int fy,int btt,int rawbtt,uint32_t state,uint8_t flags);
mWidget *__mEventProcMotion(mWindow *win,int fx,int fy,uint32_t state,mlkbool add_pentab);

void __mEventProcButton_pentab(mWindow *win,
	double x,double y,double pressure,int btt,int rawbtt,uint32_t state,uint32_t evflags,uint8_t flags);
void __mEventProcMotion_pentab(mWindow *win,double x,double y,
	double pressure,uint32_t state,uint32_t evflags);

#endif
