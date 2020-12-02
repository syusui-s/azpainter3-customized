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

#ifndef MLIB_UTIL_FILE_H
#define MLIB_UTIL_FILE_H

typedef struct _mFileStat mFileStat;

enum MREADFILEFULL_FLAGS
{
	MREADFILEFULL_FILENAME_LOCALE = 1<<0,
	MREADFILEFULL_ADD_NULL = 1<<1,
	MREADFILEFULL_ACCEPT_EMPTY = 1<<2
};

#ifdef __cplusplus
extern "C" {
#endif

mBool mIsFileExist(const char *filename,mBool bDirectory);
mBool mGetFileStat(const char *filename,mFileStat *dst);
mBool mCreateDir(const char *path);
mBool mCreateDirHome(const char *pathadd);
mBool mDeleteFile(const char *filename);
mBool mDeleteDir(const char *path);

mBool mReadFileFull(const char *filename,uint8_t flags,mBuf *dstbuf);
mBool mReadFileHead(const char *filename,void *buf,int size);

mBool mCopyFile(const char *src,const char *dst,int bufsize);

#ifdef __cplusplus
}
#endif

#endif
