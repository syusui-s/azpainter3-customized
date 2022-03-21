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

#ifndef MLK_CHECKBUTTON_H
#define MLK_CHECKBUTTON_H

#define MLK_CHECKBUTTON(p)  ((mCheckButton *)(p))
#define MLK_CHECKBUTTON_DEF mWidget wg; mCheckButtonData ckbtt;

typedef struct
{
	mWidgetLabelText txt;
	uint32_t fstyle,
		flags;
	int grabbed_key;
}mCheckButtonData;

struct _mCheckButton
{
	mWidget wg;
	mCheckButtonData ckbtt;
};

enum MCHECKBUTTON_STYLE
{
	MCHECKBUTTON_S_COPYTEXT = 1<<0,
	MCHECKBUTTON_S_RADIO = 1<<1,
	MCHECKBUTTON_S_RADIO_GROUP = 1<<2,
	MCHECKBUTTON_S_BUTTON = 1<<3
};

enum MCHECKBUTTON_FLAGS
{
	MCHECKBUTTON_F_CHECKED = 1<<0,
	MCHECKBUTTON_F_GRABBED = 1<<1,
	MCHECKBUTTON_F_CHANGED = 1<<2
};

enum MCHECKBUTTON_NOTIFY
{
	MCHECKBUTTON_N_CHANGE
};


#ifdef __cplusplus
extern "C" {
#endif

mCheckButton *mCheckButtonNew(mWidget *parent,int size,uint32_t fstyle);
mCheckButton *mCheckButtonCreate(mWidget *parent,int id,
	uint32_t flayout,uint32_t margin_pack,uint32_t fstyle,const char *text,int checked);

void mCheckButtonDestroy(mWidget *p);
void mCheckButtonHandle_calcHint(mWidget *p);
void mCheckButtonHandle_draw(mWidget *p,mPixbuf *pixbuf);
int mCheckButtonHandle_event(mWidget *wg,mEvent *ev);

mlkbool mCheckButtonIsRadioType(mCheckButton *p);
int mCheckButtonGetCheckBoxSize(void);
void mCheckButtonSetText(mCheckButton *p,const char *text);
void mCheckButtonSetText_copy(mCheckButton *p,const char *text);
mlkbool mCheckButtonSetState(mCheckButton *p,int type);
mlkbool mCheckButtonSetGroupSel(mCheckButton *p,int no);
int mCheckButtonGetGroupSel(mCheckButton *p);
mlkbool mCheckButtonIsChecked(mCheckButton *p);
mCheckButton *mCheckButtonGetRadioInfo(mCheckButton *start,mCheckButton **pptop,mCheckButton **ppend);

#ifdef __cplusplus
}
#endif

#endif
