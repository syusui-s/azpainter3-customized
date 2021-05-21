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

/********************************
 * アプリケーション用カーソル
 ********************************/

enum APPCURSOR
{
	APPCURSOR_DRAW,	//fix
	APPCURSOR_WAIT,	//fix

	APPCURSOR_DND_ITEM,
	APPCURSOR_HAND,
	APPCURSOR_HAND_GRAB,
	APPCURSOR_ROTATE,
	APPCURSOR_MOVE,
	APPCURSOR_SPOIT,
	APPCURSOR_SELECT,
	APPCURSOR_SEL_MOVE,
	APPCURSOR_STAMP,
	APPCURSOR_ZOOM_DRAG,

	APPCURSOR_LEFT_TOP,
	APPCURSOR_RIGHT_TOP,
	APPCURSOR_RESIZE_HORZ,
	APPCURSOR_RESIZE_VERT
};

void AppCursor_init(void);
void AppCursor_free(void);

void AppCursor_setDrawCursor(const char *filename,int hotx,int hoty);

mCursor AppCursor_getWaitCursor(void);
mCursor AppCursor_getForCanvas(int no);
mCursor AppCursor_getForDrag(int no);
mCursor AppCursor_getForDialog(int no);

