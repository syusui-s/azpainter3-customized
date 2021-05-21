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

#ifndef MLK_PV_WINDOW_H
#define MLK_PV_WINDOW_H

//(focus_ready_type)

#define MWINDOW_FOCUS_READY_TYPE_FOCUSOUT_WINDOW 0	//ウィンドウのフォーカスアウトによる
#define MWINDOW_FOCUS_READY_TYPE_NO_FOCUS 1			//ウィンドウにフォーカスがない状態でセットされた

//状態フラグのマクロ

#define MLK_WINDOW_IS_MAP(p)    ((p)->win.fstate & MWINDOW_STATE_MAP)
#define MLK_WINDOW_IS_UNMAP(p)  (!((p)->win.fstate & MWINDOW_STATE_MAP))
#define MLK_WINDOW_ISNOT_NORMAL_SIZE(p) ((p)->win.fstate & (MWINDOW_STATE_MAXIMIZED|MWINDOW_STATE_FULLSCREEN))
#define MLK_WINDOW_IS_NORMAL_SIZE(p)    (!((p)->win.fstate & (MWINDOW_STATE_MAXIMIZED|MWINDOW_STATE_FULLSCREEN)))


/*--- mTooltip ---*/

typedef struct
{
	mWidgetLabelText txt;
}mTooltipData;

struct _mTooltip
{
	mWidget wg;
	mContainerData ct;
	mWindowData win;
	mPopupData pop;
	mTooltipData ttip;
};


/*--- func ---*/

/* mlk_pv_window.c */

mWindow *__mWindowNew(mWidget *parent,int size);

void __mWindowUpdateRootRect(mWindow *p,mRect *rc);
void __mWindowUpdateRoot(mWindow *p,int x,int y,int w,int h);
void __mWindowUpdateRect(mWindow *p,mRect *rc);
void __mWindowUpdate(mWindow *p,int x,int y,int w,int h);

void __mWindowGetSize_excludeDecoration(mWindow *p,int w,int h,mSize *size);
void __mWindowOnResize_setNormal(mWindow *p,int w,int h);
void __mWindowOnResize_configure(mWindow *p,int w,int h);
void __mWindowOnConfigure_state(mWindow *p,uint32_t mask);

mlkbool __mWindowSetFocus(mWindow *win,mWidget *focus,int from);
mWidget *__mWindowGetStateEnable_first(mWindow *win,uint32_t flags,mlkbool is_focus);
mWidget *__mWindowGetTakeFocus_first(mWindow *win);
mWidget *__mWindowGetTakeFocus_next(mWindow *win);
mWidget *__mWindowGetDefaultFocusWidget(mWindow *win,int *dst_from);
mlkbool __mWindowMoveFocusNext(mWindow *win);
mWidget *__mWindowGetDNDDropTarget(mWindow *p,int x,int y);

void __mPopupGetRect(mRect *rcdst,mRect *rc,int w,int h,uint32_t f,int flip);

void __mWindowDecoSetInfo(mWindow *p,mWindowDecoInfo *dst);
void __mWindowDecoRunSetWidth(mWindow *p,int type);
void __mWindowDecoRunUpdate(mWindow *p,int type);

/* mlk_menuwindow.c */

mMenuItem *__mMenuWindowRunPopup(mWidget *parent,int x,int y,mBox *box,uint32_t flags,mMenu *menu,mWidget *send);
mMenuItem *__mMenuWindowRunMenuBar(mMenuBar *menubar,mBox *box,mMenu *menu,mMenuItem *item);
void __mMenuWindowMoveMenuBarItem(mMenuItem *item);

#endif
