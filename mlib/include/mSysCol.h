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

#ifndef MLIB_SYSCOL_H
#define MLIB_SYSCOL_H

enum MSYSCOL_INDEX
{
	MSYSCOL_WHITE,

	MSYSCOL_FACE,
	MSYSCOL_FACE_FOCUS,
	MSYSCOL_FACE_SELECT,
	MSYSCOL_FACE_SELECT_LIGHT,
	MSYSCOL_FACE_DARK,
	MSYSCOL_FACE_DARKER,
	MSYSCOL_FACE_LIGHTEST,

	MSYSCOL_FRAME,
	MSYSCOL_FRAME_FOCUS,
	MSYSCOL_FRAME_DARK,
	MSYSCOL_FRAME_LIGHT,

	MSYSCOL_TEXT,
	MSYSCOL_TEXT_REVERSE,
	MSYSCOL_TEXT_DISABLE,
	MSYSCOL_TEXT_SELECT,

	MSYSCOL_MENU_FACE,
	MSYSCOL_MENU_FRAME,
	MSYSCOL_MENU_SEP,
	MSYSCOL_MENU_SELECT,
	MSYSCOL_MENU_TEXT,
	MSYSCOL_MENU_TEXT_DISABLE,
	MSYSCOL_MENU_TEXT_SHORTCUT,

	MSYSCOL_ICONBTT_FACE_SELECT,
	MSYSCOL_ICONBTT_FRAME_SELECT,

	MSYSCOL_NUM
};

extern uint32_t g_mSysCol[MSYSCOL_NUM * 2];

#define MSYSCOL(name)     g_mSysCol[MSYSCOL_ ## name]
#define MSYSCOL_RGB(name) g_mSysCol[MSYSCOL_NUM + MSYSCOL_ ## name]

#endif
