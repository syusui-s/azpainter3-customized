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

#ifndef MLIB_WIDGET_PV_H
#define MLIB_WIDGET_PV_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mWidgetLabelTextRowInfo
{
	int pos,len,width;
}mWidgetLabelTextRowInfo;


/*---- mWidget ----*/

mWidget *__mWidgetGetTreeNext(mWidget *p);
mWidget *__mWidgetGetTreeNextPass(mWidget *p);
mWidget *__mWidgetGetTreeNextPass_root(mWidget *p,mWidget *root);
mWidget *__mWidgetGetTreeNext_root(mWidget *p,mWidget *root);

mWidget *__mWidgetGetTreeNext_follow_ui(mWidget *p,uint32_t follow_mask,uint32_t run_mask);
mWidget *__mWidgetGetTreeNext_follow_uidraw(mWidget *p);

void __mWidgetSetTreeUpper_ui(mWidget *p,uint32_t flags);

mWidget *__mWidgetGetTreeNext_state(mWidget *p,mWidget *root,uint32_t mask);

mBool __mWidgetGetClipRect(mWidget *wg,mRect *rcdst);

void __mWidgetOnResize(mWidget *p);
void __mWidgetCalcHint(mWidget *p);

int __mWidgetGetLayoutW(mWidget *p);
int __mWidgetGetLayoutH(mWidget *p);
int __mWidgetGetLayoutW_space(mWidget *p);
int __mWidgetGetLayoutH_space(mWidget *p);
void __mWidgetGetLayoutCalcSize(mWidget *p,mSize *hint,mSize *init);

void __mWidgetSetFocus(mWidget *p,int by);
void __mWidgetKillFocus(mWidget *p,int by);
void __mWidgetRemoveFocus(mWidget *p);
void __mWidgetRemoveFocus_byDisable(mWidget *p);

mWidgetLabelTextRowInfo *__mWidgetGetLabelTextRowInfo(const char *text);
void __mWidgetGetLabelTextSize(mWidget *p,const char *text,mWidgetLabelTextRowInfo *buf,mSize *dstsize);

/*---- mWindow ----*/

mWidget *__mWindowGetDefaultFocusWidget(mWindow *win);
mBool __mWindowSetFocus(mWindow *win,mWidget *focus,int by);
mBool __mWindowMoveNextFocus(mWindow *win);

#ifdef __cplusplus
}
#endif

#endif
