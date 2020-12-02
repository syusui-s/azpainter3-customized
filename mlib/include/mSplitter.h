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

#ifndef MLIB_SPLITTER_H
#define MLIB_SPLITTER_H

#include "mWidgetDef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define M_SPLITTER(p)  ((mSplitter *)(p))

typedef struct _mSplitter mSplitter;

typedef struct
{
	mWidget *wgprev,*wgnext;
	int prev_min,prev_max,prev_cur,
		next_min,next_max,next_cur;
	intptr_t param;
}mSplitterTargetInfo;

typedef int (*mSplitterCallbackGetTarget)(mSplitter *,mSplitterTargetInfo *);


typedef struct
{
	uint32_t style;
	int presspos,dragdiff;
	uint8_t fdrag;
	mSplitterTargetInfo info;
	mSplitterCallbackGetTarget func_target;
}mSplitterData;

struct _mSplitter
{
	mWidget wg;
	mSplitterData spl;
};


enum MSPLITTER_STYLE
{
	MSPLITTER_S_HORZ = 0,
	MSPLITTER_S_VERT = 1<<0,
	MSPLITTER_S_NOTIFY_MOVED = 1<<1
};

enum MSPLITTER_NOTIFY
{
	MSPLITTER_N_MOVED
};


void mSplitterDrawHandle(mWidget *p,mPixbuf *pixbuf);
int mSplitterEventHandle(mWidget *wg,mEvent *ev);

mSplitter *mSplitterNew(int size,mWidget *parent,uint32_t style);
void mSplitterSetCallback_getTarget(mSplitter *p,mSplitterCallbackGetTarget func,intptr_t param);

#ifdef __cplusplus
}
#endif

#endif
