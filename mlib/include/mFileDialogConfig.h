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

#ifndef MLIB_FILEDIALOGCONFIG_H
#define MLIB_FILEDIALOGCONFIG_H

/* mAppDef.h, mIniRead.c, mIniWrite.c */

enum MFILEDIALOGCONFIG_VAL
{
	MFILEDIALOGCONFIG_VAL_FLAGS,
	MFILEDIALOGCONFIG_VAL_WIN_WIDTH,
	MFILEDIALOGCONFIG_VAL_WIN_HEIGHT,

	MFILEDIALOGCONFIG_VAL_NUM
};

enum MFILEDIALOGCONFIG_SORTTYPE
{
	MFILEDIALOGCONFIG_SORTTYPE_FILENAME,
	MFILEDIALOGCONFIG_SORTTYPE_FILESIZE,
	MFILEDIALOGCONFIG_SORTTYPE_MODIFY
};

enum MFILEDIALOGCONFIG_FLAGS
{
	MFILEDIALOGCONFIG_FLAGS_SORT_UP = 1<<3,
	MFILEDIALOGCONFIG_FLAGS_SHOW_HIDDEN_FILES = 1<<4
};

#define MFILEDIALOGCONFIG_GET_SORT_TYPE(n)  ((n) & 7)

#endif
