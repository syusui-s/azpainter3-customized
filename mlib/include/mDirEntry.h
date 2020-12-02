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

#ifndef MLIB_DIRENTRY_H
#define MLIB_DIRENTRY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mDirEntry mDirEntry;
typedef struct _mFileStat mFileStat;


void mDirEntryClose(mDirEntry *p);
mDirEntry *mDirEntryOpen(const char *path);
mBool mDirEntryRead(mDirEntry *p);

const char *mDirEntryGetFileName(mDirEntry *p);
void mDirEntryGetFileName_str(mDirEntry *p,mStr *str,mBool bFullPath);

mBool mDirEntryGetStat(mDirEntry *p,mFileStat *dst);

mBool mDirEntryIsDirectory(mDirEntry *p);
mBool mDirEntryIsSpecName(mDirEntry *p);
mBool mDirEntryIsToParent(mDirEntry *p);
mBool mDirEntryIsHiddenFile(mDirEntry *p);
mBool mDirEntryIsEqExt(mDirEntry *p,const char *ext);

#ifdef __cplusplus
}
#endif

#endif
