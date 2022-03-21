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

#ifndef MLK_WIDGET_H
#define MLK_WIDGET_H

typedef struct _mWidgetLabelTextLineInfo mWidgetLabelTextLineInfo;
typedef struct _mWidgetLabelText mWidgetLabelText;

#define MWIDGET_EVENT_ADD_NOTIFY_SEND_RAW  ((mWidget *)1)

enum MWIDGETLABELTEXT_DRAW_FLAGS
{
	MWIDGETLABELTEXT_DRAW_F_RIGHT  = 1<<0,
	MWIDGETLABELTEXT_DRAW_F_CENTER = 1<<1
};


#ifdef __cplusplus
extern "C" {
#endif

/* mWidget */

mWidget *mWidgetNew(mWidget *parent,int size);
void mWidgetDestroy(mWidget *p);
void mWidgetDestroy_child(mWidget *p);

void mWidgetDrawHandle_bkgnd(mWidget *p,mPixbuf *pixbuf);
void mWidgetDrawHandle_bkgndAndFrame(mWidget *p,mPixbuf *pixbuf);
void mWidgetDrawHandle_frame(mWidget *p,mPixbuf *pixbuf);
void mWidgetDrawHandle_frameBlack(mWidget *p,mPixbuf *pixbuf);
void mWidgetDrawBkgndHandle_fillFace(mWidget *p,mPixbuf *pixbuf,mBox *box);

mlkbool mWidgetTreeIsUnder(mWidget *p,mWidget *root);
void mWidgetTreeAdd(mWidget *p,mWidget *parent);
void mWidgetTreeInsert(mWidget *p,mWidget *parent,mWidget *ins);
void mWidgetTreeRemove(mWidget *p);
void mWidgetTreeRemove_child(mWidget *p);
void mWidgetTreeMove(mWidget *p,mWidget *parent,mWidget *ins);
void mWidgetTreeMove_toFirst(mWidget *p,mWidget *parent);

mFont *mWidgetGetFont(mWidget *p);
int mWidgetGetFontHeight(mWidget *p);
int mWidgetGetFontLineHeight(mWidget *p);
void mWidgetGetBox_parent(mWidget *p,mBox *box);
void mWidgetGetBox_rel(mWidget *p,mBox *box);
void mWidgetGetSize_visible(mWidget *p,mSize *size);
int mWidgetGetWidth_visible(mWidget *p);
int mWidgetGetHeight_visible(mWidget *p);
mWidget *mWidgetGetWidget_atPoint(mWidget *root,int x,int y);
void mWidgetMapPoint(mWidget *src,mWidget *dst,mPoint *pt);
mlkbool mWidgetIsPointIn(mWidget *p,int x,int y);
mWidget *mWidgetFindFromID(mWidget *root,int id);

mlkbool mWidgetIsVisible(mWidget *p);
mlkbool mWidgetIsVisible_self(mWidget *p);
mlkbool mWidgetIsEnable(mWidget *p);

mlkbool mWidgetEnable(mWidget *p,int type);
mlkbool mWidgetShow(mWidget *p,int type);
void mWidgetMove(mWidget *p,int x,int y);
mlkbool mWidgetResize(mWidget *p,int w,int h);
mlkbool mWidgetMoveResize(mWidget *p,int x,int y,int w,int h);

mlkbool mWidgetSetFocus(mWidget *p);
void mWidgetSetFocus_redraw(mWidget *p,mlkbool force);
void mWidgetSetNoTakeFocus_under(mWidget *p);

void mWidgetSetRecalcHint(mWidget *p);
void mWidgetLayout(mWidget *p);
void mWidgetReLayout(mWidget *p);
void mWidgetLayout_redraw(mWidget *p);
mlkbool mWidgetResize_calchint_larger(mWidget *p);
void mWidgetSetMargin_same(mWidget *p,int val);
void mWidgetSetMargin_pack4(mWidget *p,uint32_t val);
void mWidgetSetInitSize_fontHeightUnit(mWidget *p,int wmul,int hmul);
void mWidgetSetConstruct(mWidget *p);
void mWidgetRunConstruct(mWidget *p);

int mWidgetGetDrawBox(mWidget *p,mBox *box);
void mWidgetGetDrawPos_abs(mWidget *p,mPoint *pt);
void mWidgetRedraw(mWidget *p);
void mWidgetRedrawBox(mWidget *p,mBox *box);
void mWidgetUpdateBox(mWidget *p,mBox *box);
void mWidgetUpdateBox_d(mWidget *p,int x,int y,int w,int h);

void mWidgetDrawBkgnd(mWidget *p,mBox *box);
void mWidgetDrawBkgnd_force(mWidget *p,mBox *box);

mPixbuf *mWidgetDirectDraw_begin(mWidget *p);
void mWidgetDirectDraw_end(mWidget *p,mPixbuf *pixbuf);

mWidget *mWidgetGetNotifyWidget(mWidget *p);
mWidget *mWidgetGetNotifyWidget_raw(mWidget *p);
mEvent *mWidgetEventAdd(mWidget *p,int type,int size);
void mWidgetEventAdd_base(mWidget *p,int type);
void mWidgetEventAdd_notify(mWidget *from,mWidget *send,int type,intptr_t param1,intptr_t param2);
void mWidgetEventAdd_notify_id(mWidget *from,mWidget *send,int id,int type,intptr_t param1,intptr_t param2);
void mWidgetEventAdd_command(mWidget *p,int id,intptr_t param,int from,void *from_ptr);

void mWidgetTimerAdd(mWidget *p,int id,uint32_t msec,intptr_t param);
mlkbool mWidgetTimerAdd_ifnothave(mWidget *p,int id,uint32_t msec,intptr_t param);
mlkbool mWidgetTimerIsHave(mWidget *p,int id);
mlkbool mWidgetTimerDelete(mWidget *p,int id);
void mWidgetTimerDeleteAll(mWidget *p);

mlkbool mWidgetSetCursor(mWidget *p,mCursor cur);
mlkbool mWidgetSetCursor_cache_type(mWidget *p,int curtype);
mlkbool mWidgetSetCursor_cache_cur(mWidget *p,mCursor cur);

mlkbool mWidgetGrabPointer(mWidget *p);
mlkbool mWidgetUngrabPointer(void);
mWidget *mWidgetGetGrabPointer(void);

/* mContainer */

mWidget *mContainerNew(mWidget *parent,int size);

mWidget *mContainerCreate(mWidget *parent,int type,int sep,uint32_t flayout,uint32_t margin_pack);
mWidget *mContainerCreateVert(mWidget *parent,int sep,uint32_t flayout,uint32_t margin_pack);
mWidget *mContainerCreateHorz(mWidget *parent,int sep,uint32_t flayout,uint32_t margin_pack);
mWidget *mContainerCreateGrid(mWidget *parent,int cols,int seph,int sepv,uint32_t flayout,uint32_t margin_pack);

void mContainerSetType(mContainer *p,int type);
void mContainerSetType_vert(mContainer *p,int sep);
void mContainerSetType_horz(mContainer *p,int sep);
void mContainerSetType_grid(mContainer *p,int cols,int seph,int sepv);

void mContainerSetPadding_same(mContainer *p,int val);
void mContainerSetPadding_pack4(mContainer *p,uint32_t val);
void mContainerSetSepPadding(mContainer *p,int sep,uint32_t pack);

mWidget *mContainerCreateButtons_okcancel(mWidget *parent,uint32_t padding_pack);
void mContainerAddButtons_okcancel(mWidget *parent);

/* mWidgetLabelText */

mWidgetLabelTextLineInfo *mWidgetLabelText_createLineInfo(const char *text);
void mWidgetLabelText_getSize(mWidget *p,const char *text,mWidgetLabelTextLineInfo *buf,mSize *dstsize);

void mWidgetLabelText_free(mWidgetLabelText *lt);
void mWidgetLabelText_set(mWidget *p,mWidgetLabelText *lt,const char *text,int fcopy);
void mWidgetLabelText_onCalcHint(mWidget *p,mWidgetLabelText *lt,mSize *size);
void mWidgetLabelText_draw(mWidgetLabelText *lt,mPixbuf *pixbuf,mFont *font,
	int x,int y,int w,mRgbCol col,uint32_t flags);

/* widget builder */

mlkbool mWidgetBuilder_create(mWidget *parent,const char *text);

#ifdef __cplusplus
}
#endif

#endif
