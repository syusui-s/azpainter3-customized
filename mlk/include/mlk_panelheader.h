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

#ifndef MLK_PANELHEADER_H
#define MLK_PANELHEADER_H

#define MLK_PANELHEADER(p)  ((mPanelHeader *)(p))
#define MLK_PANELHEADER_DEF mWidget wg; mPanelHeaderData ph;

typedef struct
{
	const char *text;
	uint32_t fstyle;
	int fpress;
}mPanelHeaderData;

struct _mPanelHeader
{
	mWidget wg;
	mPanelHeaderData ph;
};

enum MPANELHEADER_STYLE
{
	MPANELHEADER_S_HAVE_CLOSE = 1<<0,
	MPANELHEADER_S_HAVE_STORE = 1<<1,
	MPANELHEADER_S_HAVE_SHUT = 1<<2,
	MPANELHEADER_S_HAVE_RESIZE = 1<<3,
	MPANELHEADER_S_STORE_LEAVED = 1<<4,
	MPANELHEADER_S_SHUT_CLOSED = 1<<5,
	MPANELHEADER_S_RESIZE_DISABLED = 1<<6,

	MPANELHEADER_S_ALL_BUTTONS = MPANELHEADER_S_HAVE_CLOSE
		| MPANELHEADER_S_HAVE_STORE | MPANELHEADER_S_HAVE_SHUT | MPANELHEADER_S_HAVE_RESIZE
};

enum MPANELHEADER_NOTIFY
{
	MPANELHEADER_N_PRESS_BUTTON
};

enum MPANELHEADER_BTT
{
	MPANELHEADER_BTT_CLOSE = 1,
	MPANELHEADER_BTT_STORE,
	MPANELHEADER_BTT_SHUT,
	MPANELHEADER_BTT_RESIZE
};


#ifdef __cplusplus
extern "C" {
#endif

mPanelHeader *mPanelHeaderNew(mWidget *parent,int size,uint32_t fstyle);

void mPanelHeaderDestroy(mWidget *p);
int mPanelHeaderHandle_event(mWidget *wg,mEvent *ev);
void mPanelHeaderHandle_draw(mWidget *p,mPixbuf *pixbuf);
void mPanelHeaderHandle_calcHint(mWidget *wg);

void mPanelHeaderSetTitle(mPanelHeader *p,const char *title);
void mPanelHeaderSetShut(mPanelHeader *p,int type);
void mPanelHeaderSetResize(mPanelHeader *p,int type);

#ifdef __cplusplus
}
#endif

#endif
