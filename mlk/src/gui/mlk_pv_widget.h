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

#ifndef MLK_PV_WIDGET_H
#define MLK_PV_WIDGET_H

#define MLK_WIDGET_IS_TOPLEVEL(p)  (( ((mWidget *)(p))->ftype & MWIDGET_TYPE_WINDOW_TOPLEVEL) != 0)

/* mWidgetRect */

void __mWidgetRectSetSame(mWidgetRect *p,int val);
void __mWidgetRectSetPack4(mWidgetRect *p,uint32_t val);

/* mWidget */

void __mWidgetCreateInit(mWidget *p,int id,uint32_t flayout,uint32_t margin_pack);

mWidget *__mWidgetGetTreeNext(mWidget *p);
mWidget *__mWidgetGetTreeNextPass(mWidget *p);
mWidget *__mWidgetGetTreeNextPass_root(mWidget *p,mWidget *root);
mWidget *__mWidgetGetTreeNext_root(mWidget *p,mWidget *root);

mWidget *__mWidgetGetListSkipNum(mWidget *p,int dir);

void __mWidgetSetUIFlags_upper(mWidget *p,uint32_t flags);
void __mWidgetSetUIFlags_draw(mWidget *p);
mWidget *__mWidgetGetTreeNext_uiflag(mWidget *p,uint32_t follow_mask,uint32_t run_mask);
mWidget *__mWidgetGetTreeBottom_uiflag(mWidget *p,uint32_t follow_mask);
mWidget *__mWidgetGetTreePrev_uiflag(mWidget *p,uint32_t follow_mask,uint32_t run_mask);
mWidget *__mWidgetGetTreeNext_draw(mWidget *p);
mlkbool __mWidgetGetWindowRect(mWidget *p,mRect *dst,mPoint *ptoffset,mlkbool root);

void __mWidgetOnResize(mWidget *p);
mlkbool __mWidgetLayoutMoveResize(mWidget *p,int x,int y,int w,int h);
void __mWidgetCalcHint(mWidget *p);
int __mWidgetGetLayoutW(mWidget *p);
int __mWidgetGetLayoutH(mWidget *p);
int __mWidgetGetLayoutW_space(mWidget *p);
int __mWidgetGetLayoutH_space(mWidget *p);
void __mWidgetGetLayoutCalcSize(mWidget *p,mSize *hint,mSize *init);

void __mWidgetDrawBkgnd(mWidget *wg,mBox *box,mlkbool force);

void __mWidgetRemoveSelfFocus(mWidget *p);
void __mWidgetRemoveFocus_forDisabled(mWidget *p);
void __mWidgetLeave_forDisable(mWidget *p);
mlkbool __mWidgetMoveFocus_arrowkey(mWidget *p,uint32_t key);

#endif
