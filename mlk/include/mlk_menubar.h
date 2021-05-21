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

#ifndef MLK_MENUBAR_H
#define MLK_MENUBAR_H

#define MLK_MENUBAR(p)  ((mMenuBar *)(p))
#define MLK_MENUBAR_DEF mWidget wg; mMenuBarData menubar;

typedef struct
{
	mMenu *menu;
	mMenuItem *item_sel;
	uint32_t fstyle;
}mMenuBarData;

struct _mMenuBar
{
	mWidget wg;
	mMenuBarData menubar;
};

enum MMENUBAR_STYLE
{
	MMENUBAR_S_BORDER_BOTTOM = 1<<0
};

enum MMENUBAR_ARRAY16
{
	MMENUBAR_ARRAY16_END = 0xffff,
	MMENUBAR_ARRAY16_SEP = 0xfffe,
	MMENUBAR_ARRAY16_SUB_START = 0xfffd,
	MMENUBAR_ARRAY16_SUB_END = 0xfffc,
	MMENUBAR_ARRAY16_CHECK = 0x8000,
	MMENUBAR_ARRAY16_RADIO = 0x4000
};


#ifdef __cplusplus
extern "C" {
#endif

mMenuBar *mMenuBarNew(mWidget *parent,int size,uint32_t fstyle);
mMenuBar *mMenuBarCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);
void mMenuBarDestroy(mWidget *p);

void mMenuBarHandle_calcHint(mWidget *p);
int mMenuBarHandle_event(mWidget *p,mEvent *ev);
void mMenuBarHandle_draw(mWidget *p,mPixbuf *pixbuf);

mMenu *mMenuBarGetMenu(mMenuBar *p);
void mMenuBarSetMenu(mMenuBar *p,mMenu *menu);
void mMenuBarSetSubmenu(mMenuBar *p,int id,mMenu *submenu);

void mMenuBarCreateMenuTrArray16(mMenuBar *p,const uint16_t *buf);
void mMenuBarCreateMenuTrArray16_radio(mMenuBar *p,const uint16_t *buf);

#ifdef __cplusplus
}
#endif

#endif
