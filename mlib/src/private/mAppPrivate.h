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

#ifndef MLIB_APPPRIVATE_H
#define MLIB_APPPRIVATE_H

#include "mListDef.h"
#include "mTranslation.h"

typedef struct _mAppRunDat mAppRunDat;

struct _mAppRunDat
{
	mAppRunDat *runBack;
	mWindow *modal,*popup;
	mBool bQuit;
};

struct _mAppPrivate
{
	mList listEvent;

	mAppRunDat *runCurrent;
	int runLevel;

	mTranslation transDef,
		transApp;
	const char *trans_filename;
};

#endif
