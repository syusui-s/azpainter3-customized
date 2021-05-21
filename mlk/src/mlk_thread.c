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

/*****************************************
 * スレッド (UNIX)
 *****************************************/

#include <pthread.h>

#include "mlk.h"
#include "mlk_thread.h"


#define _MUTEX(p)  ((pthread_mutex_t *)(p))
#define _COND(p)   ((pthread_cond_t *)(p))



//*******************************
// mThread
//*******************************



/* スレッド関数 */

static void *_thread_run(void *arg)
{
	mThread *p = (mThread *)arg;

	//スレッドID をセット

	mThreadMutexLock(p->mutex);
	p->id = (intptr_t)pthread_self();
	mThreadMutexUnlock(p->mutex);

	//wait を抜ける

	mThreadCondSignal(p->cond);

	//登録された関数へ

	(p->run)(p);

	return 0;
}


/**@ 削除 */

void mThreadDestroy(mThread *p)
{
	if(p)
	{
		mThreadMutexDestroy(p->mutex);
		mThreadCondDestroy(p->cond);

		mFree(p);
	}
}

/**@ スレッドデータ作成
 *
 * @g:mThread
 *
 * @p:size 構造体の全体サイズ。\
 * 任意のデータを含める場合、mThread + データの構造体とする。\
 * mThread のサイズ以下なら、自動で最小サイズとなる。
 * @p:run スレッドの実行関数
 * @p:param ユーザー定義値。\
 * mThread::param にセットされる。\
 * 任意のデータが一つだけなら、構造体を新しくする必要はない。 */

mThread *mThreadNew(int size,void (*run)(mThread *),void *param)
{
	mThread *p;

	if(size < sizeof(mThread))
		size = sizeof(mThread);

	p = (mThread *)mMalloc0(size);
	if(!p) return NULL;

	p->run = run;
	p->param = param;
	p->mutex = mThreadMutexNew();
	p->cond = mThreadCondNew();

	return p;
}

/**@ スレッド開始
 *
 * @r:FALSE で失敗。\
 * すでに実行されている場合も FALSE となる。 */

mlkbool mThreadRun(mThread *p)
{
	pthread_t id;

	if(!p || p->id) return FALSE;

	//開始

	if(pthread_create(&id, NULL, _thread_run, p) != 0)
		return FALSE;

	//ID がセットされるまで待つ

	mThreadMutexLock(p->mutex);

	if(!p->id)
		mThreadCondWait(p->cond, p->mutex);

	mThreadMutexUnlock(p->mutex);

	return TRUE;
}

/**@ スレッドが終了するまで待つ
 *
 * @d:スレッドが終了した場合、id は 0 になる。
 * 
 * @r:TRUE で正常に終了した、または終了している */

mlkbool mThreadWait(mThread *p)
{
	if(!p || !p->id) return TRUE;

	if(pthread_join((pthread_t)p->id, NULL) != 0)
		return FALSE;
	else
	{
		p->id = 0;
		return TRUE;
	}
}


//*******************************
// mThreadMutex
//*******************************


/**@ 削除 */

void mThreadMutexDestroy(mThreadMutex p)
{
	if(p)
	{
		pthread_mutex_destroy(_MUTEX(p));

		mFree(p);
	}
}

/**@ mutex 作成
 *
 * @g:mThreadMutex
 *
 * @d:mutex をロックしている間は、
 * 他のスレッドがその部分を実行できなくなる。\
 * 他のスレッドと共有して使われるデータを参照/設定する場合などに使う。
 *
 * @r:NULL で失敗 */

mThreadMutex mThreadMutexNew(void)
{
	pthread_mutex_t *p;

	//確保

	p = (pthread_mutex_t *)mMalloc0(sizeof(pthread_mutex_t));
	if(!p) return NULL;

	//初期化

	pthread_mutex_init(p, NULL);

	return (mThreadMutex)p;
}

/**@ ロックする
 *
 * @d:他のスレッドでロックされている場合、
 * すべてのロックが解除されるまでブロックされる。\
 * 他のスレッドでロックされていない場合は、何もしない。 */

void mThreadMutexLock(mThreadMutex p)
{
	if(p) pthread_mutex_lock(_MUTEX(p));
}

/**@ ロック解除 */

void mThreadMutexUnlock(mThreadMutex p)
{
	if(p) pthread_mutex_unlock(_MUTEX(p));
}


//*******************************
// mThreadCond
//*******************************


/**@ 削除 */

void mThreadCondDestroy(mThreadCond p)
{
	if(p)
	{
		pthread_cond_destroy(_COND(p));
		mFree(p);
	}
}

/**@ cond 作成
 *
 * @g:mThreadCond
 *
 * @d:他のスレッドとの同期に使う。\
 * 他のスレッドで何かが起こるまで待ちたい場合、wait で待つ。\
 * トリガーとなるスレッドでは、singal または broadcast を使って、
 * 待っているスレッドを再開させる。 */

mThreadCond mThreadCondNew(void)
{
	pthread_cond_t *p;

	p = (pthread_cond_t *)mMalloc0(sizeof(pthread_cond_t));
	if(!p) return NULL;

	pthread_cond_init(p, NULL);

	return (mThreadCond)p;
}

/**@ wait で待っているスレッドの一つを再開させる
 *
 * @d:待っているスレッドがない場合は、何もしない。\
 * 複数スレッドが待っている場合、いずれか一つが再開されるが、
 * どれが再開するかはわからない。 */

void mThreadCondSignal(mThreadCond p)
{
	if(p) pthread_cond_signal(_COND(p));
}

/**@ wait で待っているすべてのスレッドを再開させる
 *
 * @d:待っているスレッドがない場合は、何もしない。 */

void mThreadCondBroadcast(mThreadCond p)
{
	if(p) pthread_cond_broadcast(_COND(p));
}

/**@ 待つ
 *
 * @d:他のスレッドで signal or broadcast が呼ばれるまで、ブロックする。\
 * wait の前に mutex ロックを行うこと。\
 * wait 後は、mutex がロックされた状態で戻る。 */

void mThreadCondWait(mThreadCond p,mThreadMutex mutex)
{
	if(p) pthread_cond_wait(_COND(p), _MUTEX(mutex));
}

