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

#ifndef MLK_FONTBUTTON_H
#define MLK_FONTBUTTON_H

#include "mlk_button.h"
#include "mlk_fontinfo.h"
#include "mlk_sysdlg.h"

#define MLK_FONTBUTTON(p)  ((mFontButton *)(p))
#define MLK_FONTBUTTON_DEF MLK_BUTTON_DEF mFontButtonData fbt;

typedef struct
{
	mFontInfo info;
	uint32_t flags;
}mFontButtonData;

struct _mFontButton
{
	MLK_BUTTON_DEF
	mFontButtonData fbt;
};

enum MFONTBUTTON_STYLE
{
	MFONTBUTTON_S_ALL_INFO = 1<<0
};

enum MFONTBUTTON_NOTIFY
{
	MFONTBUTTON_N_CHANGE_FONT
};


#ifdef __cplusplus
extern "C" {
#endif

mFontButton *mFontButtonNew(mWidget *parent,int size,uint32_t fstyle);
mFontButton *mFontButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle);

void mFontButtonDestroy(mWidget *p);

void mFontButtonSetFlags(mFontButton *p,uint32_t flags);
void mFontButtonSetInfo_text(mFontButton *p,const char *text);
void mFontButtonGetInfo_text(mFontButton *p,mStr *str);
void mFontButtonSetInfo(mFontButton *p,const mFontInfo *src);
void mFontButtonGetInfo(mFontButton *p,mFontInfo *dst);

#ifdef __cplusplus
}
#endif

#endif
