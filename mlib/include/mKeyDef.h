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

#ifndef MLIB_KEYDEF_H
#define MLIB_KEYDEF_H

enum MKEY
{
	MKEY_UNKNOWN,

	MKEY_BREAK,
	MKEY_PAUSE,
	MKEY_CLEAR,
	MKEY_PRINTSCREEN,
	MKEY_ESCAPE,

	MKEY_NUM_LOCK,
	MKEY_SCROLL_LOCK,
	MKEY_CAPS_LOCK,

	MKEY_ENTER,
	MKEY_BACKSPACE,
	MKEY_TAB,
	MKEY_DELETE,
	MKEY_INSERT,

	MKEY_SHIFT,
	MKEY_CONTROL,
	MKEY_ALT,
	MKEY_SUPER,
	MKEY_MENU,

	MKEY_SPACE = 0x20,

	MKEY_PAGEUP,
	MKEY_PAGEDOWN,
	MKEY_END,
	MKEY_HOME,
	MKEY_LEFT,
	MKEY_UP,
	MKEY_RIGHT,
	MKEY_DOWN,

	MKEY_0 = 0x30,
	MKEY_1,
	MKEY_2,
	MKEY_3,
	MKEY_4,
	MKEY_5,
	MKEY_6,
	MKEY_7,
	MKEY_8,
	MKEY_9,

	MKEY_A = 0x41,
	MKEY_Z = 0x5a,

	MKEY_NUM0 = 0x60,
	MKEY_NUM1,
	MKEY_NUM2,
	MKEY_NUM3,
	MKEY_NUM4,
	MKEY_NUM5,
	MKEY_NUM6,
	MKEY_NUM7,
	MKEY_NUM8,
	MKEY_NUM9,
	MKEY_NUM_MUL,
	MKEY_NUM_ADD,
	MKEY_NUM_SEP,
	MKEY_NUM_SUB,
	MKEY_NUM_DECIMAL,
	MKEY_NUM_DIV,

	MKEY_F1	= 0x70,
	MKEY_F2,
	MKEY_F3,
	MKEY_F4,
	MKEY_F5,
	MKEY_F6,
	MKEY_F7,
	MKEY_F8,
	MKEY_F9,
	MKEY_F10,
	MKEY_F11,
	MKEY_F12,
	MKEY_F13,
	MKEY_F14,
	MKEY_F15,
	MKEY_F16,
	MKEY_F17,
	MKEY_F18,
	MKEY_F19,
	MKEY_F20,
	MKEY_F21,
	MKEY_F22,
	MKEY_F23,
	MKEY_F24
};

#endif
