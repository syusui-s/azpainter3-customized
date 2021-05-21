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

#ifndef MLK_MULTIEDITPAGE_H
#define MLK_MULTIEDITPAGE_H

#define MLK_MULTIEDITPAGE(p)  ((mMultiEditPage *)(p))

typedef struct
{
	mWidgetTextEdit *pedit;
	int drag_pos,
		fpress;
}mMultiEditPageData;

typedef struct _mMultiEditPage
{
	MLK_SCROLLVIEWPAGE_DEF
	mMultiEditPageData mep;
}mMultiEditPage;


#ifdef __cplusplus
extern "C" {
#endif

mMultiEditPage *mMultiEditPageNew(mWidget *parent,int size,mWidgetTextEdit *pedit);

void mMultiEditPage_setScrollInfo(mMultiEditPage *p);
void mMultiEditPage_onChangeText(mMultiEditPage *p);
void mMultiEditPage_onChangeCursorPos(mMultiEditPage *p);
void mMultiEditPage_getIMPos(mMultiEditPage *p,mPoint *pt);

#ifdef __cplusplus
}
#endif

#endif
