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

#ifndef MLK_WINDOW_H
#define MLK_WINDOW_H

typedef struct _mCIManager mCIManager;

typedef void (*mFuncPopupQuit)(mWidget *p,mlkbool is_cansel);


enum MTOPLEVEL_RESIZE_TYPE
{
	MTOPLEVEL_RESIZE_TYPE_TOP = 1,
	MTOPLEVEL_RESIZE_TYPE_BOTTOM = 2,
	MTOPLEVEL_RESIZE_TYPE_LEFT = 4,
	MTOPLEVEL_RESIZE_TYPE_RIGHT = 8,
	MTOPLEVEL_RESIZE_TYPE_TOP_LEFT = 5,
	MTOPLEVEL_RESIZE_TYPE_TOP_RIGHT = 9,
	MTOPLEVEL_RESIZE_TYPE_BOTTOM_LEFT = 6,
	MTOPLEVEL_RESIZE_TYPE_BOTTOM_RIGHT = 10
};

enum MPOPUP_FLAGS
{
	MPOPUP_F_LEFT = 1<<0,
	MPOPUP_F_TOP = 1<<1,
	MPOPUP_F_RIGHT  = 1<<2,
	MPOPUP_F_BOTTOM = 1<<3,
	MPOPUP_F_GRAVITY_LEFT = 1<<4,
	MPOPUP_F_GRAVITY_TOP = 1<<5,
	MPOPUP_F_GRAVITY_RIGHT = 1<<6,
	MPOPUP_F_GRAVITY_BOTTOM = 1<<7,
	MPOPUP_F_FLIP_X = 1<<8,
	MPOPUP_F_FLIP_Y = 1<<9,
	MPOPUP_F_FLIP_XY = MPOPUP_F_FLIP_X | MPOPUP_F_FLIP_Y,

	MPOPUP_F_MENU_SEND_COMMAND = 1<<16
};

enum MTOOLTIP_FLAGS
{
	MTOOLTIP_F_COPYTEXT = 1<<0
};


#ifdef __cplusplus
extern "C" {
#endif

/* mWindow */

mWidget *mWindowGetFocus(mWindow *p);
mlkbool mWindowIsToplevel(mWindow *p);
mlkbool mWindowIsActivate(mWindow *p);
mlkbool mWindowIsOtherThanModal(mWindow *p);
void mWindowResizeShow_initSize(mWindow *p);
void mWindowSetCursor(mWindow *p,mCursor cursor);
void mWindowResetCursor(mWindow *p);

/* mToplevel */

mToplevel *mToplevelNew(mWindow *parent,int size,uint32_t fstyle);

void mToplevelAttachMenuBar(mToplevel *p,mMenuBar *menubar);
mMenu *mToplevelGetMenu_menubar(mToplevel *p);
void mToplevelAttachAccelerator(mToplevel *p,mAccelerator *accel);
void mToplevelDestroyAccelerator(mToplevel *p);
mAccelerator *mToplevelGetAccelerator(mToplevel *p);

void mToplevelSetTitle(mToplevel *p,const char *title);
void mToplevelSetIconPNG_file(mToplevel *p,const char *filename);
void mToplevelSetIconPNG_buf(mToplevel *p,const void *buf,uint32_t bufsize);

mlkbool mToplevelIsMaximized(mToplevel *p);
mlkbool mToplevelIsFullscreen(mToplevel *p);
void mToplevelMinimize(mToplevel *p);
mlkbool mToplevelMaximize(mToplevel *p,int type);
mlkbool mToplevelFullscreen(mToplevel *p,int type);

void mToplevelStartDragMove(mToplevel *p);
void mToplevelStartDragResize(mToplevel *p,int type);

void mToplevelGetSaveState(mToplevel *p,mToplevelSaveState *st);
void mToplevelSetSaveState(mToplevel *p,mToplevelSaveState *st);

/* mDialog */

mDialog *mDialogNew(mWindow *parent,int size,uint32_t fstyle);
intptr_t mDialogRun(mDialog *p,mlkbool destroy);
void mDialogEnd(mDialog *p,intptr_t ret);

int mDialogEventDefault(mWidget *wg,mEvent *ev);
int mDialogEventDefault_okcancel(mWidget *wg,mEvent *ev);

/* mPopup */

mPopup *mPopupNew(int size,uint32_t fstyle);
mlkbool mPopupShow(mPopup *p,mWidget *parent,int x,int y,int w,int h,mBox *box,uint32_t flags);
mlkbool mPopupRun(mPopup *p,mWidget *parent,int x,int y,int w,int h,mBox *box,uint32_t flags);
void mPopupQuit(mPopup *p,mlkbool is_cancel);
void mPopupSetHandle_quit(mPopup *p,mFuncPopupQuit handle);

int mPopupEventDefault(mWidget *wg,mEvent *ev);

/* mTooltip */

mTooltip *mTooltipShow(mTooltip *p,mWidget *parent,int x,int y,mBox *box,uint32_t popup_flags,
	const char *text,uint32_t tooltip_flags);

/* mPopupListView */

mlkbool mPopupListView_run(mWidget *parent,
	int x,int y,mBox *box,uint32_t flags,
	mCIManager *manager,int itemh,int width,int maxh);

#ifdef __cplusplus
}
#endif

#endif
