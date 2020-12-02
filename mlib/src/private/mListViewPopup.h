/*$
 Copyright (C) 2013-2020 Azel.

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

#ifndef MLIB_LISTVIEW_POPUP_H
#define MLIB_LISTVIEW_POPUP_H

enum
{
	MLISTVIEWPOPUP_S_VERTSCR = 1<<0
};

#ifdef __cplusplus
extern "C" {
#endif

void mListViewPopupRun(mWidget *callwg,mLVItemMan *manager,
	uint32_t style,int x,int y,int w,int h,int itemh);

#ifdef __cplusplus
}
#endif

#endif
