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

#ifndef MLK_PROGRESSBAR_H
#define MLK_PROGRESSBAR_H

#define MLK_PROGRESSBAR(p)  ((mProgressBar *)(p))
#define MLK_PROGRESSBAR_DEF mWidget wg; mProgressBar pb;

typedef struct
{
	char *text;
	uint32_t fstyle;
	int min, max, pos,
		sub_step, sub_max, sub_toppos,
		sub_curcnt, sub_curstep, sub_nextcnt;
}mProgressBarData;

struct _mProgressBar
{
	mWidget wg;
	mProgressBarData pb;
};

enum MPROGRESSBAR_STYLE
{
	MPROGRESSBAR_S_FRAME = 1<<0,
	MPROGRESSBAR_S_TEXT  = 1<<1,
	MPROGRESSBAR_S_TEXT_PERS = 1<<2,
};


#ifdef __cplusplus
extern "C" {
#endif

mProgressBar *mProgressBarNew(mWidget *parent,int size,uint32_t fstyle);
mProgressBar *mProgressBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);
void mProgressBarDestroy(mWidget *p);

void mProgressBarHandle_calcHint(mWidget *p);
void mProgressBarHandle_draw(mWidget *p,mPixbuf *pixbuf);

void mProgressBarSetStatus(mProgressBar *p,int min,int max,int pos);
void mProgressBarSetText(mProgressBar *p,const char *text);
mlkbool mProgressBarSetPos(mProgressBar *p,int pos);
void mProgressBarIncPos(mProgressBar *p);
mlkbool mProgressBarAddPos(mProgressBar *p,int add);

void mProgressBarSubStep_begin(mProgressBar *p,int num,int maxcnt);
void mProgressBarSubStep_begin_onestep(mProgressBar *p,int num,int maxcnt);
mlkbool mProgressBarSubStep_inc(mProgressBar *p);

#ifdef __cplusplus
}
#endif

#endif
