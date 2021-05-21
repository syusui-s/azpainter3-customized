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

#ifndef MLK_GUISTYLE_H
#define MLK_GUISTYLE_H

#include "mlk_guicol.h"
#include "mlk_fontinfo.h"

typedef struct _mGuiStyleInfo
{
	mRgbCol col[MGUICOL_NUM];
	mFontInfo fontinfo;
	mStr strIcontheme;
}mGuiStyleInfo;

void mGuiStyle_free(mGuiStyleInfo *p);
void mGuiStyle_load(const char *filename,mGuiStyleInfo *info);
void mGuiStyle_loadColor(const char *filename);
void mGuiStyle_save(const char *filename,mGuiStyleInfo *info);

void mGuiStyle_getCurrentInfo(mGuiStyleInfo *info);
void mGuiStyle_getCurrentColor(mGuiStyleInfo *info);
void mGuiStyle_setColor(mGuiStyleInfo *info);

#endif
