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

/*****************************************
 * キー関連
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mDef.h"
#include "mKeyDef.h"


//-------------------

static const char *keyname1[] = {
	"Break", "Pause", "Clear", "PrintScreen", "Escape",
	"NumLock", "ScrollLock", "CapsLock",
	"Enter", "Backspace", "Tab", "Delete", "Insert",
	"Shift", "Ctrl", "Alt", "Super", "Menu"
},
*keyname2[] = {
	"Space", "PageUp", "PageDown", "End", "Home",
	"Left", "Up", "Right", "Down"
},
*keyname3[] = {
	"Num(*)", "Num(+)", "Num(,)", "Num(-)", "Num(.)", "Num(/)"
};

//-------------------


int mKeyCodeToName(uint32_t c,char *buf,int bufsize)
{
	char m[32];

	m[0] = m[1] = 0;

	if(c >= MKEY_BREAK && c <= MKEY_MENU)
		strcpy(m, keyname1[c - MKEY_BREAK]);
	else if(c >= MKEY_SPACE && c <= MKEY_DOWN)
		strcpy(m, keyname2[c - MKEY_SPACE]);
	else if((c >= MKEY_0 && c <= MKEY_9) || (c >= MKEY_A && c <= MKEY_Z))
		m[0] = c;
	else if(c >= MKEY_NUM0 && c <= MKEY_NUM9)
		sprintf(m, "NUM%d", c - MKEY_NUM0);
	else if(c >= MKEY_NUM_MUL && c <= MKEY_NUM_DIV)
		strcpy(m, keyname3[c - MKEY_NUM_MUL]);
	else if(c >= MKEY_F1 && c <= MKEY_F24)
		sprintf(m, "F%d", c - MKEY_F1 + 1);

	//セット
	
	return mStrcpy(buf, m, bufsize);
}

