/*$
 Copyright (C) 2013-2021 Azel.

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

#ifndef MLK_FILE_H
#define MLK_FILE_H

typedef int mFile;
typedef struct _mFileText mFileText;

#define MFILE_NONE  (-1)

enum MREADFILEFULL_FLAGS
{
	MREADFILEFULL_APPEND_0  = 1<<0,
	MREADFILEFULL_ACCEPT_EMPTY = 1<<1
};


#ifdef __cplusplus
extern "C" {
#endif

mlkerr mFileClose(mFile file);
mlkerr mFileOpen_read(mFile *file,const char *filename);
mlkerr mFileOpen_write(mFile *file,const char *filename,int perm);

mlkfoff mFileGetSize(mFile file);

mlkfoff mFileGetPos(mFile file);
mlkerr mFileSetPos(mFile file,mlkfoff pos);
mlkerr mFileSeekCur(mFile file,mlkfoff seek);
mlkerr mFileSeekEnd(mFile file,mlkfoff seek);

int32_t mFileRead(mFile file,void *buf,int32_t size);
mlkerr mFileRead_full(mFile file,void *buf,int32_t size);

int32_t mFileWrite(mFile file,const void *buf,int32_t size);
mlkerr mFileWrite_full(mFile file,const void *buf,int32_t size);

/*----*/

mlkbool mIsExistPath(const char *path);
mlkbool mIsExistFile(const char *path);
mlkbool mIsExistDir(const char *path);
mlkbool mGetFileStat(const char *path,mFileStat *dst);
mlkbool mGetFileSize(const char *path,mlkfoff *dst);
mlkerr mCreateDir(const char *path,int perm);
mlkerr mCreateDir_parents(const char *path,int perm);
mlkbool mDeleteFile(const char *path);
mlkbool mDeleteDir(const char *path);

/* util */

mlkerr mReadFileFull_alloc(const char *filename,uint32_t flags,uint8_t **ppdst,int32_t *psize);
mlkerr mReadFileHead(const char *filename,void *buf,int32_t size);
mlkerr mCopyFile(const char *srcpath,const char *dstpath);
mlkerr mWriteFile_fromBuf(const char *filename,const void *buf,int32_t size);

mlkerr mFileText_readFile(mFileText **dst,const char *filename);
void mFileText_end(mFileText *p);
char *mFileText_nextLine(mFileText *p);
char *mFileText_nextLine_skip(mFileText *p);

#ifdef __cplusplus
}
#endif

#endif
