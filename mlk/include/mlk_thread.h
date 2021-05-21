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

#ifndef MLK_THREAD_H
#define MLK_THREAD_H

struct _mThread
{
	intptr_t id;
	void (*run)(mThread *);
	void *param;
	mThreadMutex mutex;
	mThreadCond cond;
};

#ifdef __cplusplus
extern "C" {
#endif

mThread *mThreadNew(int size,void (*run)(mThread *),void *param);
void mThreadDestroy(mThread *p);
mlkbool mThreadRun(mThread *p);
mlkbool mThreadWait(mThread *p);

mThreadMutex mThreadMutexNew(void);
void mThreadMutexDestroy(mThreadMutex p);
void mThreadMutexLock(mThreadMutex p);
void mThreadMutexUnlock(mThreadMutex p);

mThreadCond mThreadCondNew(void);
void mThreadCondDestroy(mThreadCond p);
void mThreadCondSignal(mThreadCond p);
void mThreadCondBroadcast(mThreadCond p);
void mThreadCondWait(mThreadCond p,mThreadMutex mutex);

#ifdef __cplusplus
}
#endif

#endif
