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

#ifndef MLK_DIR_H
#define MLK_DIR_H

typedef struct _mDir mDir;

#ifdef __cplusplus
extern "C" {
#endif

mDir *mDirOpen(const char *path);
void mDirClose(mDir *p);
mlkbool mDirNext(mDir *p);

const char *mDirGetFilename(mDir *p);
void mDirGetFilename_str(mDir *p,mStr *str,mlkbool fullpath);

mlkbool mDirGetStat(mDir *p,mFileStat *dst);

mlkbool mDirIsDirectory(mDir *p);
mlkbool mDirIsSpecName(mDir *p);
mlkbool mDirIsSpecName_parent(mDir *p);
mlkbool mDirIsHiddenFile(mDir *p);
mlkbool mDirCompareExt(mDir *p,const char *ext);

#ifdef __cplusplus
}
#endif

#endif
