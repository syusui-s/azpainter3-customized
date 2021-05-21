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

#ifndef MLK_PAGER_H
#define MLK_PAGER_H

#define MLK_PAGER(p)  ((mPager *)(p))
#define MLK_PAGER_DEF MLK_CONTAINER_DEF mPagerData pg;

typedef struct _mPagerInfo mPagerInfo;

typedef mlkbool (*mFuncPagerCreate)(mPager *p,mPagerInfo *);
typedef mlkbool (*mFuncPagerSetData)(mPager *p,void *pagedat,void *src);
typedef mlkbool (*mFuncPagerGetData)(mPager *p,void *pagedat,void *dst);
typedef void (*mFuncPagerDestroy)(mPager *p,void *pagedat);
typedef int (*mFuncPagerEvent)(mPager *p,mEvent *ev,void *pagedat);

struct _mPagerInfo
{
	void *pagedat;
	mFuncPagerSetData setdata;
	mFuncPagerGetData getdata;
	mFuncPagerDestroy destroy;
	mFuncPagerEvent event;
};

typedef struct
{
	mPagerInfo info;
	void *data;
}mPagerData;

struct _mPager
{
	MLK_CONTAINER_DEF
	mPagerData pg;
};


#ifdef __cplusplus
extern "C" {
#endif

mPager *mPagerNew(mWidget *parent,int size);
mPager *mPagerCreate(mWidget *parent,uint32_t flayout,uint32_t margin_pack);

void mPagerDestroy(mWidget *wg);
int mPagerHandle_event(mWidget *wg,mEvent *ev);

void *mPagerGetDataPtr(mPager *p);
void mPagerSetDataPtr(mPager *p,void *data);
mlkbool mPagerGetPageData(mPager *p);
mlkbool mPagerSetPageData(mPager *p);

void mPagerDestroyPage(mPager *p);
mlkbool mPagerSetPage(mPager *p,mFuncPagerCreate create);

#ifdef __cplusplus
}
#endif

#endif
