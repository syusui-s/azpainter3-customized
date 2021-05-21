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

#ifndef MLK_EXPANDER_H
#define MLK_EXPANDER_H

#define MLK_EXPANDER(p)  ((mExpander *)(p))
#define MLK_EXPANDER_DEF MLK_CONTAINER_DEF mExpanderData exp;

typedef struct
{
	char *text;
	uint32_t fstyle;
	mWidgetRect rcpad;
	int header_height,
		expanded;
}mExpanderData;

struct _mExpander
{
	MLK_CONTAINER_DEF
	mExpanderData exp;
};


enum MEXPANDER_STYLE
{
	MEXPANDER_S_COPYTEXT = 1<<0,
	MEXPANDER_S_SEP_TOP = 1<<1,
	MEXPANDER_S_DARK = 1<<2
};

enum MEXPANDER_NOTIFY
{
	MEXPANDER_N_TOGGLE
};


#ifdef __cplusplus
extern "C" {
#endif

mExpander *mExpanderNew(mWidget *parent,int size,uint32_t fstyle);
mExpander *mExpanderCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text);

void mExpanderDestroy(mWidget *p);
void mExpanderHandle_calcHint(mWidget *p);
int mExpanderHandle_event(mWidget *wg,mEvent *ev);
void mExpanderHandle_draw(mWidget *wg,mPixbuf *pixbuf);

void mExpanderSetText(mExpander *p,const char *text);
void mExpanderSetPadding_pack4(mExpander *p,uint32_t pack);
void mExpanderToggle(mExpander *p,int type);

#ifdef __cplusplus
}
#endif

#endif
