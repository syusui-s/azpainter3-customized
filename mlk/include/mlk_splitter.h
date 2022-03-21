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

#ifndef MLK_SPLITTER_H
#define MLK_SPLITTER_H

#define MLK_SPLITTER(p)     ((mSplitter *)(p))
#define MLK_SPLITTER_DEF(p) mWidget wg; mSplitterData spl;

typedef struct
{
	mWidget *wgprev,*wgnext;
	int prev_min,prev_max,prev_cur,
		next_min,next_max,next_cur;
	intptr_t param;
}mSplitterTarget;

typedef int (*mFuncSplitterGetTarget)(mSplitter *,mSplitterTarget *);


typedef struct
{
	uint32_t fstyle;
	int press_pos,
		drag_diff,
		fpointer;
	mSplitterTarget info;
	mFuncSplitterGetTarget get_target;
}mSplitterData;

struct _mSplitter
{
	mWidget wg;
	mSplitterData spl;
};


enum MSPLITTER_STYLE
{
	MSPLITTER_S_HORZ = 0,
	MSPLITTER_S_VERT = 1<<0
};

enum MSPLITTER_NOTIFY
{
	MSPLITTER_N_MOVED
};


#ifdef __cplusplus
extern "C" {
#endif

mSplitter *mSplitterNew(mWidget *parent,int size,uint32_t fstyle);
void mSplitterSetFunc_getTarget(mSplitter *p,mFuncSplitterGetTarget func,intptr_t param);

void mSplitterDestroy(mWidget *p);
void mSplitterHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mSplitterHandle_event(mWidget *wg,mEvent *ev);

#ifdef __cplusplus
}
#endif

#endif
