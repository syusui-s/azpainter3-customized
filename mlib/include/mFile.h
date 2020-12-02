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

#ifndef MLIB_FILE_H
#define MLIB_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int mFile;

#define MFILE_NONE  (-1)

enum MFILEOPEN_FLAGS
{
	MFILEOPEN_FILENAME_LOCALE = 1<<0
};

/*------------*/

mBool mFileClose(mFile file);
mBool mFileOpenRead(mFile *file,const char *filename,uint32_t flags);
mBool mFileOpenWrite(mFile *file,const char *filename,uint32_t flags);

uintptr_t mFileGetSize(mFile file);
uint64_t mFileGetSizeLong(mFile file);

void mFileSetPos(mFile file,uintptr_t pos);
void mFileSeekCur(mFile file,int seek);
void mFileSeekEnd(mFile file,int seek);

int mFileRead(mFile file,void *buf,int size);
mBool mFileReadSize(mFile file,void *buf,int size);
uint32_t mFileGet32LE(mFile file);
uint32_t mFileGet32BE(mFile file);
mBool mFileReadCompareText(mFile file,const char *text);

int mFileWrite(mFile file,const void *buf,int size);
mBool mFileWriteSize(mFile file,const void *buf,int size);

#ifdef __cplusplus
}
#endif

#endif
