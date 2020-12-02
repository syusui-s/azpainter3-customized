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

#ifndef MLIB_TRANS_H
#define MLIB_TRANS_H

#define M_TR_G(id)      mTransSetGroup(id)
#define M_TR_T(id)      mTransGetText(id)
#define M_TR_T2(gid,id) mTransGetText2(gid, id)

#define M_TRGROUP_SYS   0xffff

enum M_TRSYS
{
	M_TRSYS_OK = 1,
	M_TRSYS_CANCEL,
	M_TRSYS_YES,
	M_TRSYS_NO,
	M_TRSYS_SAVE,
	M_TRSYS_SAVENO,
	M_TRSYS_ABORT,
	M_TRSYS_NOTSHOW_THISMES,
	M_TRSYS_OPEN,
	M_TRSYS_FILENAME,
	M_TRSYS_TYPE,
	M_TRSYS_PREVIEW,
	M_TRSYS_FONTSTYLE,
	M_TRSYS_FONTSIZE,
	M_TRSYS_FONTWEIGHT,
	M_TRSYS_FONTSLANT,
	M_TRSYS_FONTRENDER,
	M_TRSYS_FONTPREVIEWTEXT,
	M_TRSYS_FILESIZE,
	M_TRSYS_FILEMODIFY,
	M_TRSYS_SHOW_HIDDEN_FILES,

	M_TRSYS_TITLE_OPENFILE = 1000,
	M_TRSYS_TITLE_SAVEFILE,
	M_TRSYS_TITLE_SELECTDIR,
	M_TRSYS_TITLE_SELECTCOLOR,
	M_TRSYS_TITLE_SELECTFONT,

	M_TRSYS_MES_OVERWRITE = 2000,
	M_TRSYS_MES_FILENAME_INCORRECT
};

#ifdef __cplusplus
extern "C" {
#endif

void mTransSetGroup(uint16_t groupid);
const char *mTransGetText(uint16_t strid);
const char *mTransGetTextRaw(uint16_t strid);
const char *mTransGetTextDef(uint16_t strid);
const char *mTransGetText2(uint16_t groupid,uint16_t strid);
const char *mTransGetText2Raw(uint16_t groupid,uint16_t strid);

#ifdef __cplusplus
}
#endif

#endif
