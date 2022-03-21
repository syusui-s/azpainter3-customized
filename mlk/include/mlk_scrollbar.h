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

#ifndef MLK_SCROLLBAR_H
#define MLK_SCROLLBAR_H

#define MLK_SCROLLBAR(p)  ((mScrollBar *)(p))
#define MLK_SCROLLBAR_DEF mWidget wg; mScrollBarData sb;
#define MLK_SCROLLBAR_WIDTH  15

typedef void (*mFuncScrollBarHandle_action)(mWidget *p,int pos,uint32_t flags);

typedef struct
{
	mFuncScrollBarHandle_action action;
	uint32_t fstyle;
	int min, max, page, pos,
		range, drag_diff, fgrab;
}mScrollBarData;

struct _mScrollBar
{
	mWidget wg;
	mScrollBarData sb;
};

enum MSCROLLBAR_STYLE
{
	MSCROLLBAR_S_HORZ = 0,
	MSCROLLBAR_S_VERT = 1<<0
};

enum MSCROLLBAR_NOTIFY
{
	MSCROLLBAR_N_ACTION
};

enum MSCROLLBAR_ACTION_FLAGS
{
	MSCROLLBAR_ACTION_F_CHANGE_POS = 1<<0,
	MSCROLLBAR_ACTION_F_PRESS = 1<<1,
	MSCROLLBAR_ACTION_F_DRAG  = 1<<2,
	MSCROLLBAR_ACTION_F_RELEASE = 1<<3
};


#ifdef __cplusplus
extern "C" {
#endif

mScrollBar *mScrollBarNew(mWidget *parent,int size,uint32_t fstyle);
mScrollBar *mScrollBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);
void mScrollBarDestroy(mWidget *p);

void mScrollBarHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mScrollBarHandle_event(mWidget *wg,mEvent *ev);

mlkbool mScrollBarIsPos_top(mScrollBar *p);
mlkbool mScrollBarIsPos_bottom(mScrollBar *p);
int mScrollBarGetPos(mScrollBar *p);
int mScrollBarGetPage(mScrollBar *p);
int mScrollBarGetMaxPos(mScrollBar *p);

void mScrollBarSetHandle_action(mScrollBar *p,mFuncScrollBarHandle_action action);
void mScrollBarSetStatus(mScrollBar *p,int min,int max,int page);
int mScrollBarSetPage(mScrollBar *p,int page);
mlkbool mScrollBarSetPos(mScrollBar *p,int pos);
mlkbool mScrollBarSetPos_bottom(mScrollBar *p);
mlkbool mScrollBarAddPos(mScrollBar *p,int val);

#ifdef __cplusplus
}
#endif

#endif
