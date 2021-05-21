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

#ifndef MLK_ICONBAR_H
#define MLK_ICONBAR_H

#define MLK_ICONBAR(p)  ((mIconBar *)(p))
#define MLK_ICONBAR_DEF mWidget wg;	mIconBarData ib;

typedef struct
{
	mImageList *imglist;
	struct _mIconBarItem *item_sel;
	mTooltip *tooltip;
	mList list;
	uint32_t fstyle;
	int autowrap_width,
		padding;
	uint16_t trgroup;
	uint8_t press;
}mIconBarData;

struct _mIconBar
{
	mWidget wg;
	mIconBarData ib;
};

enum MICONBAR_STYLE
{
	MICONBAR_S_TOOLTIP    = 1<<0,
	MICONBAR_S_AUTO_WRAP  = 1<<1,
	MICONBAR_S_VERT       = 1<<2,
	MICONBAR_S_SEP_BOTTOM = 1<<3,
	MICONBAR_S_BUTTON_FRAME = 1<<4,
	MICONBAR_S_DESTROY_IMAGELIST = 1<<5
};

enum MICONBAR_ITEM_FLAGS
{
	MICONBAR_ITEMF_BUTTON = 0,
	MICONBAR_ITEMF_CHECKBUTTON = 1<<0,
	MICONBAR_ITEMF_CHECKGROUP  = 1<<1,
	MICONBAR_ITEMF_SEP      = 1<<2,
	MICONBAR_ITEMF_DROPDOWN = 1<<3,
	MICONBAR_ITEMF_WRAP     = 1<<4,
	MICONBAR_ITEMF_DISABLE  = 1<<5,
	MICONBAR_ITEMF_CHECKED  = 1<<6
};


#ifdef __cplusplus
extern "C" {
#endif

mIconBar *mIconBarNew(mWidget *parent,int size,uint32_t fstyle);
mIconBar *mIconBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mIconBarDestroy(mWidget *p);
void mIconBarHandle_calcHint(mWidget *p);
void mIconBarHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mIconBarHandle_event(mWidget *wg,mEvent *ev);

void mIconBarSetPadding(mIconBar *p,int padding);
void mIconBarSetImageList(mIconBar *p,mImageList *img);
void mIconBarSetTooltipTrGroup(mIconBar *p,uint16_t gid);

void mIconBarDeleteAll(mIconBar *p);
void mIconBarAdd(mIconBar *p,int id,int img,int tooltip,uint32_t flags);
void mIconBarAddSep(mIconBar *p);

void mIconBarSetCheck(mIconBar *p,int id,int type);
mlkbool mIconBarIsChecked(mIconBar *p,int id);
void mIconBarSetEnable(mIconBar *p,int id,int type);

mlkbool mIconBarGetItemBox(mIconBar *p,int id,mBox *box);

#ifdef __cplusplus
}
#endif

#endif
