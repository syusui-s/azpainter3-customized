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

#ifndef MLK_PANEL_H
#define MLK_PANEL_H

#define MLK_PANEL(p)  ((mPanel *)(p))

typedef mWidget *(*mFuncPanelCreate)(mPanel *p,int id,mWidget *parent);
typedef void (*mFuncPanelDestroy)(mPanel *p,void *exdat);
typedef int (*mFuncPanelSort)(mPanel *p,int id1,int id2);

typedef struct _mPanelState
{
	uint32_t flags;
	int height;
	mToplevelSaveState winstate;
}mPanelState;


enum MPANEL_STYLE
{
	MPANEL_S_HAVE_CLOSE = 1<<0,
	MPANEL_S_HAVE_STORE = 1<<1,
	MPANEL_S_HAVE_SHUT  = 1<<2,
	MPANEL_S_HAVE_RESIZE = 1<<3,
	MPANEL_S_EXPAND_HEIGHT = 1<<4,
	MPANEL_S_NO_EXPAND_LAST = 1<<5,
	MPANEL_S_NO_RELAYOUT = 1<<6,

	MPANEL_S_ALL_BUTTONS = MPANEL_S_HAVE_CLOSE | MPANEL_S_HAVE_STORE | MPANEL_S_HAVE_SHUT | MPANEL_S_HAVE_RESIZE
};

enum MPANEL_FLAGS
{
	MPANEL_F_CREATED = 1<<0,
	MPANEL_F_VISIBLE = 1<<1,
	MPANEL_F_WINDOW_MODE = 1<<2,
	MPANEL_F_SHUT_CLOSED = 1<<3,
	MPANEL_F_RESIZE_DISABLED = 1<<4
};

enum MPANEL_ACTION
{
	MPANEL_ACT_CLOSE = 1,
	MPANEL_ACT_TOGGLE_STORE,
	MPANEL_ACT_TOGGLE_SHUT,
	MPANEL_ACT_TOGGLE_RESIZE
};


#ifdef __cplusplus
extern "C" {
#endif

mPanel *mPanelNew(int id,int exsize,uint32_t fstyle);
void mPanelDestroy(mPanel *p);

void mPanelSetFunc_create(mPanel *p,mFuncPanelCreate func);
void mPanelSetFunc_destroy(mPanel *p,mFuncPanelDestroy func);
void mPanelSetFunc_sort(mPanel *p,mFuncPanelSort func);

void mPanelSetWindowInfo(mPanel *p,mWindow *parent,uint32_t winstyle);
void mPanelSetStoreParent(mPanel *p,mWidget *parent);
void mPanelSetState(mPanel *p,mPanelState *info);
void mPanelSetTitle(mPanel *p,const char *title);
void mPanelSetParam1(mPanel *p,intptr_t param);
void mPanelSetParam2(mPanel *p,intptr_t param);
void mPanelSetFont(mPanel *p,mFont *font);
void mPanelSetNotifyWidget(mPanel *p,mWidget *wg);

void mPanelGetState(mPanel *p,mPanelState *state);
mlkbool mPanelIsCreated(mPanel *p);
mlkbool mPanelIsVisibleContents(mPanel *p);
mlkbool mPanelIsWindowMode(mPanel *p);
mlkbool mPanelIsStoreVisible(mPanel *p);
mToplevel *mPanelGetWindow(mPanel *p);
mWidget *mPanelGetContents(mPanel *p);
void *mPanelGetExPtr(mPanel *p);
intptr_t mPanelGetParam1(mPanel *p);
intptr_t mPanelGetParam2(mPanel *p);

void mPanelCreateWidget(mPanel *p);
void mPanelShowWindow(mPanel *p);
void mPanelSetCreate(mPanel *p,int type);
void mPanelSetVisible(mPanel *p,int type);
void mPanelSetWindowMode(mPanel *p,int type);
void mPanelReLayout(mPanel *p);
void mPanelStoreReLayout(mPanel *p);
void mPanelStoreReArrange(mPanel *p,mlkbool relayout);

#ifdef __cplusplus
}
#endif

#endif
