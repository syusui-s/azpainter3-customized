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

#ifndef MLK_WINDOW_DECO_H
#define MLK_WINDOW_DECO_H

typedef struct _mWindowDecoInfo mWindowDecoInfo;

struct _mWindowDecoInfo
{
	int width,
		height,
		left,top,right,bottom;
};


enum MWINDOW_DECO_SETWIDTH_TYPE
{
	MWINDOW_DECO_SETWIDTH_TYPE_NORMAL,
	MWINDOW_DECO_SETWIDTH_TYPE_MAXIMIZED,
	MWINDOW_DECO_SETWIDTH_TYPE_FULLSCREEN
};

enum MWINDOW_DECO_UPDATE_TYPE
{
	MWINDOW_DECO_UPDATE_TYPE_RESIZE,
	MWINDOW_DECO_UPDATE_TYPE_ACTIVE,
	MWINDOW_DECO_UPDATE_TYPE_TITLE
};


#ifdef __cplusplus
extern "C" {
#endif

void mWindowDeco_setWidth(mWindow *p,int left,int top,int right,int bottom);
void mWindowDeco_updateBox(mWindow *p,int x,int y,int w,int h);

#ifdef __cplusplus
}
#endif

#endif
