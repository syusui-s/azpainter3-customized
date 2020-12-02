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

#ifndef MLIB_MESSAGEBOX_H
#define MLIB_MESSAGEBOX_H

#ifdef __cplusplus
extern "C" {
#endif

enum MMESBOX_BUTTONS
{
	MMESBOX_OK      = 1<<0,
	MMESBOX_CANCEL  = 1<<1,
	MMESBOX_YES     = 1<<2,
	MMESBOX_NO      = 1<<3,
	MMESBOX_SAVE    = 1<<4,
	MMESBOX_SAVENO  = 1<<5,
	MMESBOX_ABORT   = 1<<6,
	MMESBOX_NOTSHOW = 1<<30,

	MMESBOX_OKCANCEL = MMESBOX_OK | MMESBOX_CANCEL,
	MMESBOX_YESNO = MMESBOX_YES | MMESBOX_NO
};

uint32_t mMessageBox(mWindow *owner,const char *title,const char *message,
	uint32_t btts,uint32_t defbtt);

void mMessageBoxErr(mWindow *owner,const char *message);
void mMessageBoxErrTr(mWindow *owner,uint16_t groupid,uint16_t strid);

#ifdef __cplusplus
}
#endif

#endif
