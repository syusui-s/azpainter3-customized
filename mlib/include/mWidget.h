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

#ifndef MLIB_WIDGET_H
#define MLIB_WIDGET_H

#define MWIDGET_NOTIFYWIDGET_RAW  ((mWidget *)1)

#ifdef __cplusplus
extern "C" {
#endif

mWidget *mWidgetNew(int size,mWidget *parent);
mWidget *mWidgetDestroy(mWidget *p);

void mWidgetHandleDestroy(mWidget *p);

void mWidgetHandleFunc_draw_drawBkgnd(mWidget *p,mPixbuf *pixbuf);
void mWidgetHandleFunc_draw_boxFrame(mWidget *p,mPixbuf *pixbuf);
void mWidgetHandleFunc_drawBkgnd_fillFace(mWidget *p,mPixbuf *pixbuf,mBox *box);

mBool mWidgetIsTreeChild(mWidget *p,mWidget *root);
void mWidgetMoveTree_first(mWidget *p);

mBool mWidgetIsVisible(mWidget *p);
mBool mWidgetIsVisibleReal(mWidget *p);

mFont *mWidgetGetFont(mWidget *p);
int mWidgetGetFontHeight(mWidget *p);
mWidget *mWidgetGetUnderWidget(mWidget *root,int x,int y);
mWidget *mWidgetGetNotifyWidget(mWidget *p);
mWidget *mWidgetGetNotifyWidgetRaw(mWidget *p);
void mWidgetGetCursorPos(mWidget *p,mPoint *pt);

mBool mWidgetIsContain(mWidget *p,int x,int y);
mBool mWidgetIsCursorIn(mWidget *p);
mWidget *mWidgetFindByID(mWidget *root,int id);

void mWidgetGetBox(mWidget *p,mBox *box);
void mWidgetGetRootBox(mWidget *p,mBox *box);
void mWidgetGetLayoutMaxSize(mWidget *p,mSize *size);

void mWidgetEnable(mWidget *p,int type);
void mWidgetShow(mWidget *p,int type);
mBool mWidgetMoveResize(mWidget *p,int x,int y,int w,int h);
void mWidgetMove(mWidget *p,int x,int y);
mBool mWidgetResize(mWidget *p,int w,int h);

mBool mWidgetSetFocus(mWidget *p);
void mWidgetSetFocus_update(mWidget *p,mBool force);
void mWidgetNoTakeFocus_under(mWidget *p);

void mWidgetMapPoint(mWidget *from,mWidget *to,mPoint *pt);
void mWidgetSetCursor(mWidget *wg,mCursor cur);

void mWidgetCalcHintSize(mWidget *p);
void mWidgetLayout(mWidget *p);
void mWidgetReLayout(mWidget *p);

void mWidgetSetMargin_one(mWidget *p,int val);
void mWidgetSetMargin_b4(mWidget *p,uint32_t val);
void mWidgetSetHintOverW_fontTextWidth(mWidget *p,const char *text);
void mWidgetSetInitSize_fontHeight(mWidget *p,int wmul,int hmul);

void mWidgetDrawBkgnd(mWidget *p,mBox *box);
void mWidgetDrawBkgnd_force(mWidget *p,mBox *box);
mPixbuf *mWidgetBeginDirectDraw(mWidget *p);
void mWidgetEndDirectDraw(mWidget *p,mPixbuf *pixbuf);

void mWidgetUpdate(mWidget *p);
void mWidgetUpdateBox_d(mWidget *p,int x,int y,int w,int h);
void mWidgetUpdateBox_box(mWidget *p,mBox *box);

mEvent *mWidgetAppendEvent(mWidget *p,int type);
mEvent *mWidgetAppendEvent_only(mWidget *p,int type);
void mWidgetAppendEvent_notify(mWidget *wg,mWidget *from,int type,intptr_t param1,intptr_t param2);
void mWidgetAppendEvent_notify_id(mWidget *wg,mWidget *from,int id,int type,intptr_t param1,intptr_t param2);
void mWidgetAppendEvent_command(mWidget *wg,int id,intptr_t param,int by);

mBool mWidgetGrabPointer(mWidget *p);
mBool mWidgetGrabPointer_cursor(mWidget *p,mCursor cur);
mBool mWidgetGrabPointer_device_cursor(mWidget *p,int deviceid,mCursor cur);
void mWidgetUngrabPointer(mWidget *p);
mBool mWidgetGrabKeyboard(mWidget *p);
void mWidgetUngrabKeyboard(mWidget *p);

void mWidgetTimerAdd(mWidget *p,uint32_t timerid,uint32_t msec,intptr_t param);
void mWidgetTimerAdd_unexist(mWidget *p,uint32_t timerid,uint32_t msec,intptr_t param);
mBool mWidgetTimerDelete(mWidget *p,uint32_t timerid);
void mWidgetTimerDeleteAll(mWidget *p);
mBool mWidgetTimerIsExist(mWidget *p,uint32_t timerid);

void mWidgetTreeAppend(mWidget *p,mWidget *parent);
void mWidgetTreeInsert(mWidget *p,mWidget *parent,mWidget *ins);
void mWidgetTreeRemove(mWidget *p);
void mWidgetTreeMove(mWidget *p,mWidget *parent,mWidget *ins);

#ifdef __cplusplus
}
#endif

#endif
