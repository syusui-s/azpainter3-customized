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

#ifndef MLIB_DOCKWIDGET_H
#define MLIB_DOCKWIDGET_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mDockWidget mDockWidget;

typedef mWidget *(*mDockWidgetCallbackCreate)(mDockWidget *,int id,mWidget *parent);
typedef int (*mDockWidgetCallbackArrange)(mDockWidget *,int id1,int id2);


typedef struct _mDockWidgetState
{
	uint32_t flags;
	int dockH;
	mBox boxWin;
}mDockWidgetState;


enum MDOCKWIDGET_STYLE
{
	MDOCKWIDGET_S_HAVE_CLOSE  = 1<<0,
	MDOCKWIDGET_S_HAVE_SWITCH = 1<<1,
	MDOCKWIDGET_S_HAVE_FOLD   = 1<<2,
	MDOCKWIDGET_S_EXPAND      = 1<<3,
	MDOCKWIDGET_S_NO_FOCUS    = 1<<4,

	MDOCKWIDGET_S_ALLBUTTON = MDOCKWIDGET_S_HAVE_CLOSE | MDOCKWIDGET_S_HAVE_SWITCH | MDOCKWIDGET_S_HAVE_FOLD
};

enum MDOCKWIDGET_FLAGS
{
	MDOCKWIDGET_F_EXIST   = 1<<0,
	MDOCKWIDGET_F_VISIBLE = 1<<1,
	MDOCKWIDGET_F_WINDOW  = 1<<2,
	MDOCKWIDGET_F_FOLDED  = 1<<3
};

enum MDOCKWIDGET_NOTIFY
{
	MDOCKWIDGET_NOTIFY_CLOSE = 1,
	MDOCKWIDGET_NOTIFY_TOGGLE_SWITCH,
	MDOCKWIDGET_NOTIFY_TOGGLE_FOLD
};


/*------*/

void mDockWidgetDestroy(mDockWidget *p);
mDockWidget *mDockWidgetNew(int id,uint32_t style);

void mDockWidgetSetCallback_create(mDockWidget *p,mDockWidgetCallbackCreate func);
void mDockWidgetSetCallback_arrange(mDockWidget *p,mDockWidgetCallbackArrange func);

void mDockWidgetSetWindowInfo(mDockWidget *p,mWindow *owner,uint32_t winstyle);
void mDockWidgetSetDockParent(mDockWidget *p,mWidget *parent);
void mDockWidgetSetState(mDockWidget *p,mDockWidgetState *info);
void mDockWidgetSetTitle(mDockWidget *p,const char *title);
void mDockWidgetSetParam(mDockWidget *p,intptr_t param);
void mDockWidgetSetFont(mDockWidget *p,mFont *font);
void mDockWidgetSetNotifyWidget(mDockWidget *p,mWidget *wg,int id);

void mDockWidgetGetState(mDockWidget *p,mDockWidgetState *state);
mBool mDockWidgetIsCreated(mDockWidget *p);
mBool mDockWidgetIsVisibleContents(mDockWidget *p);
mBool mDockWidgetCanTakeFocus(mDockWidget *p);
mBool mDockWidgetIsWindowMode(mDockWidget *p);
mWindow *mDockWidgetGetWindow(mDockWidget *p);
intptr_t mDockWidgetGetParam(mDockWidget *p);

void mDockWidgetCreateWidget(mDockWidget *p);
void mDockWidgetShowWindow(mDockWidget *p);
void mDockWidgetShow(mDockWidget *p,int type);
void mDockWidgetSetVisible(mDockWidget *p,int type);
void mDockWidgetSetWindowMode(mDockWidget *p,int type);
void mDockWidgetRelocate(mDockWidget *p,mBool relayout);

#ifdef __cplusplus
}
#endif

#endif
