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

#ifndef MLK_INPUTACCEL_H
#define MLK_INPUTACCEL_H

#define MLK_INPUTACCEL(p)  ((mInputAccel *)(p))
#define MLK_INPUTACCEL_DEF mWidget wg; mInputAccelData ia;

typedef struct
{
	mStr str;
	uint32_t fstyle,
		key;
}mInputAccelData;

struct _mInputAccel
{
	mWidget wg;
	mInputAccelData ia;
};


enum MINPUTACCEL_STYLE
{
	MINPUTACCEL_S_NOTIFY_CHANGE = 1<<0,
	MINPUTACCEL_S_NOTIFY_ENTER = 1<<1
};

enum MINPUTACCEL_NOTIFY
{
	MINPUTACCEL_N_CHANGE,
	MINPUTACCEL_N_ENTER
};


#ifdef __cplusplus
extern "C" {
#endif

mInputAccel *mInputAccelNew(mWidget *parent,int size,uint32_t fstyle);
mInputAccel *mInputAccelCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mInputAccelDestroy(mWidget *p);
void mInputAccelHandle_calcHint(mWidget *p);
void mInputAccelHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mInputAccelHandle_event(mWidget *wg,mEvent *ev);

uint32_t mInputAccelGetKey(mInputAccel *p);
void mInputAccelSetKey(mInputAccel *p,uint32_t key);

#ifdef __cplusplus
}
#endif

#endif
