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

#ifndef MLIB_TEXTREAD_H
#define MLIB_TEXTREAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mTextRead mTextRead;

void mTextReadEnd(mTextRead *p);
mTextRead *mTextRead_readFile(const char *filename);

char *mTextReadGetLine(mTextRead *p);
char *mTextReadGetLine_skipEmpty(mTextRead *p);

#ifdef __cplusplus
}
#endif

#endif
