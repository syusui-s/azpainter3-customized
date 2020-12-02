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

/*****************************************
 * mPopupProgress
 *****************************************/

#include "mDef.h"

#include "mPopupProgress.h"

#include "mGui.h"
#include "mWidget.h"
#include "mWindow.h"
#include "mThread.h"



/**
@defgroup popupprogress mPopupProgress
@brief ポップアップ型のプログレスバー

- スレッドで処理をしている間に表示するポップアップ。
- 表示している間、すべてのウィンドウはマウスやキー押しなどのユーザーアクションを受け付けない。

@ingroup group_widget
@{

@file mPopupProgress.h
@def M_POPUPPROGRESS(p)
@struct mPopupProgress
@struct mPopupProgressData
*/


/** 作成
 *
 * @param w バーの幅
 * @param progress_style プログレスバーのスタイル */

mPopupProgress *mPopupProgressNew(int size,int w,uint32_t progress_style)
{
	mPopupProgress *p;
	mProgressBar *pg;

	if(size < sizeof(mPopupProgress)) size = sizeof(mPopupProgress);
	
	p = (mPopupProgress *)mWindowNew(size, NULL,
			MWINDOW_S_POPUP | MWINDOW_S_NO_IM);
	if(!p) return NULL;

	//プログレスバー

	pg = mProgressBarNew(0, M_WIDGET(p), progress_style);

	if(!pg)
	{
		mWidgetDestroy(M_WIDGET(p));
		return NULL;
	}

	p->pg.progress = pg;

	pg->wg.fLayout = MLF_EXPAND_W;

	//リサイズ

	mGuiCalcHintSize();

	mWidgetResize(M_WIDGET(p), w, p->wg.hintH);

	return p;
}

/** 実行
 *
 * mThread の param に mPopupProgress のポインタが入っている。\n
 * スレッド内で mPopupProgressThreadEnd() を呼ぶと終了する。\n
 * mPopupProgress はこの後で削除すること。 */

void mPopupProgressRun(mPopupProgress *p,int x,int y,void (*threadfunc)(mThread *))
{
	mThread *th;

	th = mThreadNew(0, threadfunc, (intptr_t)p);
	if(!th) return;

	mWidgetMove(M_WIDGET(p), x, y);
	mWidgetShow(M_WIDGET(p), 1);

	//実行

	mAppBlockUserAction(TRUE);

	mThreadRun(th);
	mAppRun();

	mAppBlockUserAction(FALSE);

	//終了

	mThreadWait(th);
	mThreadDestroy(th);
}

/** 終了 (スレッド側から) */

void mPopupProgressThreadEnd(void)
{
	mAppMutexLock();
	mAppQuit();
	mAppWakeUpEvent();
	mAppMutexUnlock();
}

/** 最大値をセット (スレッド側から) */

void mPopupProgressThreadSetMax(mPopupProgress *p,int max)
{
	if(p)
	{
		mAppMutexLock();

		(p->pg.progress)->pb.max = max;

		mAppMutexUnlock();
	}
}

/** プログレスバー位置をセット (スレッド側から) */

void mPopupProgressThreadSetPos(mPopupProgress *p,int pos)
{
	if(p)
	{
		mAppMutexLock();

		if(mProgressBarSetPos(p->pg.progress, pos))
			mAppWakeUpEvent();

		mAppMutexUnlock();
	}
}

/** プログレスバー位置を+1 (スレッド側から) */

void mPopupProgressThreadIncPos(mPopupProgress *p)
{
	if(p)
	{
		mAppMutexLock();
		mProgressBarIncPos(p->pg.progress);
		mAppWakeUpEvent();
		mAppMutexUnlock();
	}
}

/** 現在位置に加算 */

void mPopupProgressThreadAddPos(mPopupProgress *p,int add)
{
	if(p)
	{
		mAppMutexLock();

		if(mProgressBarAddPos(p->pg.progress, add))
			mAppWakeUpEvent();

		mAppMutexUnlock();
	}
}

/** サブステップ開始 */

void mPopupProgressThreadBeginSubStep(mPopupProgress *p,int stepnum,int max)
{
	if(p)
	{
		mAppMutexLock();
		mProgressBarBeginSubStep(p->pg.progress, stepnum, max);
		mAppMutexUnlock();
	}
}

/** サブステップ開始 (１ステップのみ) */

void mPopupProgressThreadBeginSubStep_onestep(mPopupProgress *p,int stepnum,int max)
{
	if(p)
	{
		mAppMutexLock();
		mProgressBarBeginSubStep_onestep(p->pg.progress, stepnum, max);
		mAppMutexUnlock();
	}
}

/** サブステップのカウントを +1 */

void mPopupProgressThreadIncSubStep(mPopupProgress *p)
{
	if(p)
	{
		mAppMutexLock();

		if(mProgressBarIncSubStep(p->pg.progress))
			mAppWakeUpEvent();

		mAppMutexUnlock();
	}
}

/** @} */
