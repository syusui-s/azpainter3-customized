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

#ifndef MLK_KEYDEF_H
#define MLK_KEYDEF_H

enum MKEY
{
	MKEY_UNKNOWN = 0,

	MKEY_SPACE = 0x20,

	MKEY_BACKSPACE = 0xff08,
	MKEY_TAB = 0xff09,
	MKEY_LF = 0xff0a,
	MKEY_CLEAR = 0xff0b,
	MKEY_ENTER = 0xff0d,

	MKEY_PAUSE = 0xff13,
	MKEY_SCROLL_LOCK = 0xff14,
	MKEY_ESCAPE = 0xff1b,

	MKEY_KANJI = 0xff21,
	MKEY_MUHENKAN = 0xff22,
	MKEY_HENKAN = 0xff23,
	MKEY_HIRAGANA_KATAKANA = 0xff27,
	MKEY_ZENKAKU_HANKAKU = 0xff2a,
	MKEY_EISU_TOGGLE = 0xff30,

	MKEY_HOME = 0xff50,
	MKEY_LEFT,
	MKEY_UP,
	MKEY_RIGHT,
	MKEY_DOWN,
	MKEY_PAGE_UP,
	MKEY_PAGE_DOWN,
	MKEY_END,
	MKEY_BEGIN,

	MKEY_SELECT = 0xff60,
	MKEY_PRINT = 0xff61,
	MKEY_EXECUTE = 0xff62,
	MKEY_INSERT = 0xff63,
	MKEY_UNDO = 0xff65,
	MKEY_REDO = 0xff66,
	MKEY_MENU = 0xff67,
	MKEY_FIND = 0xff68,
	MKEY_CANCEL = 0xff69,
	MKEY_HELP = 0xff6a,
	MKEY_BREAK = 0xff6b,
	MKEY_NUM_LOCK = 0xff7f,

	MKEY_KP_SPACE = 0xff80,
	MKEY_KP_TAB = 0xff89,
	MKEY_KP_ENTER = 0xff8d,
	MKEY_KP_F1 = 0xff91,
	MKEY_KP_F2 = 0xff92,
	MKEY_KP_F3 = 0xff93,
	MKEY_KP_F4 = 0xff94,
	MKEY_KP_HOME = 0xff95,
	MKEY_KP_LEFT,
	MKEY_KP_UP,
	MKEY_KP_RIGHT,
	MKEY_KP_DOWN,
	MKEY_KP_PAGE_UP,
	MKEY_KP_PAGE_DOWN,
	MKEY_KP_END,
	MKEY_KP_BEGIN,
	MKEY_KP_INSERT,
	MKEY_KP_DELETE,

	MKEY_KP_MUL = 0xffaa,
	MKEY_KP_ADD = 0xffab,
	MKEY_KP_SEP = 0xffac,
	MKEY_KP_SUB = 0xffad,
	MKEY_KP_DECIMAL = 0xffae,
	MKEY_KP_DIVIDE = 0xffaf,

	MKEY_KP_0 = 0xffb0,
	MKEY_KP_1,
	MKEY_KP_2,
	MKEY_KP_3,
	MKEY_KP_4,
	MKEY_KP_5,
	MKEY_KP_6,
	MKEY_KP_7,
	MKEY_KP_8,
	MKEY_KP_9,

	MKEY_KP_EQUAL = 0xffbd,

	MKEY_F1	= 0xffbe,
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
	MKEY_F24,
	MKEY_F25,
	MKEY_F26,
	MKEY_F27,
	MKEY_F28,
	MKEY_F29,
	MKEY_F30,
	MKEY_F31,
	MKEY_F32,
	MKEY_F33,
	MKEY_F34,
	MKEY_F35,

	MKEY_SHIFT_L = 0xffe1,
	MKEY_SHIFT_R,
	MKEY_CTRL_L,
	MKEY_CTRL_R,
	MKEY_CAPS_LOCK,
	MKEY_SHIFT_LOCK,
	MKEY_META_L,
	MKEY_META_R,
	MKEY_ALT_L,
	MKEY_ALT_R,
	MKEY_LOGO_L,
	MKEY_LOGO_R,
	MKEY_HYPER_L,
	MKEY_HYPER_R,

	MKEY_DELETE = 0xffff
};

#ifdef __cplusplus
extern "C" {
#endif

int mKeyGetArrowDir(uint32_t key);
mlkbool mKeyIsArrow(uint32_t key);
mlkbool mKeyIsAlphabet(uint32_t key);

uint32_t mKeycodeToKey(uint32_t code);
mlkbool mKeycodeGetName(mStr *str,uint32_t code);

#ifdef __cplusplus
}
#endif

#endif
