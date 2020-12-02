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

#ifndef MLIB_THREAD_H
#define MLIB_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mThread mThread;
typedef void *mMutex;

struct _mThread
{
	intptr_t handle,param;
	void (*func)(mThread *);
	mMutex mutex;
};


void mThreadDestroy(mThread *p);
mThread *mThreadNew(int size,void (*func)(mThread *),intptr_t param);
mBool mThreadRun(mThread *p);
mBool mThreadWait(mThread *p);

void mMutexDestroy(mMutex p);
mMutex mMutexNew(void);
void mMutexLock(mMutex p);
void mMutexUnlock(mMutex p);

#ifdef __cplusplus
}
#endif

#endif
