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

#ifndef MLK_SCROLLVIEW_H
#define MLK_SCROLLVIEW_H

#define MLK_SCROLLVIEW(p)      ((mScrollView *)(p))
#define MLK_SCROLLVIEWPAGE(p)  ((mScrollViewPage *)(p))
#define MLK_SCROLLVIEW_DEF      mWidget wg; mScrollViewData sv;
#define MLK_SCROLLVIEWPAGE_DEF  mWidget wg; mScrollViewPageData svp;

typedef void (*mFuncScrollViewPage_getScrollPage)(mWidget *p,int horz,int vert,mSize *dst);

typedef struct
{
	mScrollBar *scrh,*scrv;
	mScrollViewPage *page;
	uint32_t fstyle;
}mScrollViewData;

struct _mScrollView
{
	mWidget wg;
	mScrollViewData sv;
};

typedef struct
{
	mFuncScrollViewPage_getScrollPage getscrollpage;
}mScrollViewPageData;

struct _mScrollViewPage
{
	mWidget wg;
	mScrollViewPageData svp;
};

enum MSCROLLVIEW_STYLE
{
	MSCROLLVIEW_S_HORZ     = 1<<0,
	MSCROLLVIEW_S_VERT     = 1<<1,
	MSCROLLVIEW_S_FIX_HORZ = 1<<2,
	MSCROLLVIEW_S_FIX_VERT = 1<<3,
	MSCROLLVIEW_S_FRAME    = 1<<4,
	MSCROLLVIEW_S_SCROLL_NOTIFY_SELF = 1<<5,
	
	MSCROLLVIEW_S_HORZVERT = MSCROLLVIEW_S_HORZ | MSCROLLVIEW_S_VERT,
	MSCROLLVIEW_S_FIX_HORZVERT = MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FIX_HORZ | MSCROLLVIEW_S_FIX_VERT,
	MSCROLLVIEW_S_HORZVERT_FRAME = MSCROLLVIEW_S_HORZVERT | MSCROLLVIEW_S_FRAME
};

enum MSCROLLVIEWPAGE_NOTIFY
{
	MSCROLLVIEWPAGE_N_SCROLL_ACTION_HORZ,
	MSCROLLVIEWPAGE_N_SCROLL_ACTION_VERT
};


#ifdef __cplusplus
extern "C" {
#endif

/* mScrollView */

mScrollView *mScrollViewNew(mWidget *parent,int size,uint32_t fstyle);

void mScrollViewDestroy(mWidget *wg);
void mScrollViewHandle_resize(mWidget *wg);
void mScrollViewHandle_layout(mWidget *wg);
void mScrollViewHandle_draw(mWidget *wg,mPixbuf *pixbuf);

void mScrollViewSetPage(mScrollView *p,mScrollViewPage *page);
void mScrollViewSetFixBar(mScrollView *p,int type);
void mScrollViewLayout(mScrollView *p);
void mScrollViewSetScrollStatus_horz(mScrollView *p,int min,int max,int page);
void mScrollViewSetScrollStatus_vert(mScrollView *p,int min,int max,int page);
int mScrollViewGetScrollShowStatus(mScrollView *p);
void mScrollViewEnableScrollBar(mScrollView *p,int type);

/* mScrollViewPage */

mScrollViewPage *mScrollViewPageNew(mWidget *parent,int size);

void mScrollViewPageHandle_resize(mWidget *wg);

void mScrollViewPage_setHandle_getScrollPage(mScrollViewPage *p,mFuncScrollViewPage_getScrollPage handle);
mlkbool mScrollViewPage_setScrollPage_horz(mScrollViewPage *p,int page);
mlkbool mScrollViewPage_setScrollPage_vert(mScrollViewPage *p,int page);

void mScrollViewPage_getScrollPos(mScrollViewPage *p,mPoint *pt);
int mScrollViewPage_getScrollPos_horz(mScrollViewPage *p);
int mScrollViewPage_getScrollPos_vert(mScrollViewPage *p);
void mScrollViewPage_getScrollBar(mScrollViewPage *p,mScrollBar **scrh,mScrollBar **scrv);
mScrollBar *mScrollViewPage_getScrollBar_horz(mScrollViewPage *p);
mScrollBar *mScrollViewPage_getScrollBar_vert(mScrollViewPage *p);

#ifdef __cplusplus
}
#endif

#endif
