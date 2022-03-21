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

/*****************************************
 * mPopupProgress
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_popup_progress.h"
#include "mlk_progressbar.h"
#include "mlk_thread.h"



/**@ 作成
 *
 * @p:progress_style プログレスバーのスタイル */

mPopupProgress *mPopupProgressNew(int size,uint32_t progress_style)
{
	mPopupProgress *p;

	if(size < sizeof(mPopupProgress))
		size = sizeof(mPopupProgress);
	
	p = (mPopupProgress *)mPopupNew(size, MPOPUP_S_NO_GRAB | MPOPUP_S_NO_EVENT);
	if(!p) return NULL;

	//プログレスバー

	p->pg.progress = mProgressBarCreate(MLK_WIDGET(p), 0, MLF_EXPAND_W, 0, progress_style);

	return p;
}

/**@ 実行
 *
 * @d:ポップアップを表示し、スレッドを開始して、メインループを実行する。\
 * スレッド内で mPopupProgressThreadEnd() が呼ばれて、メインループが終了するまで返らない。\
 * \
 * mThread の param には、mPopupProgress のポインタが入っている。\
 * mPopupProgress は、この関数が戻った後で削除すること。
 *
 * @p:parent 表示の基準となるウィジェット
 * @p:width ウィンドウの幅 */

void mPopupProgressRun(mPopupProgress *p,mWidget *parent,int x,int y,mBox *box,uint32_t popup_flags,
	int width,void (*threadfunc)(mThread *))
{
	mThread *th;

	th = mThreadNew(0, threadfunc, p);
	if(!th) return;

	//プログレスバーの高さ計算

	mProgressBarHandle_calcHint(MLK_WIDGET(p->pg.progress));

	//ポップアップ表示

	mPopupShow(MLK_POPUP(p), parent, x, y, width, (p->pg.progress)->wg.hintH, box,
		popup_flags);

	//実行

	mGuiSetBlockUserAction(TRUE);

	mThreadRun(th);
	mGuiRun();

	mGuiSetBlockUserAction(FALSE);

	//終了

	mThreadWait(th);
	mThreadDestroy(th);
}

/**@ [スレッド] 終了
 *
 * @d:スレッド関数内で実行する。メインループを終了させる。\
 * スレッドのロックは自動で行われるので、ロックの範囲外で実行すること。
 *
 * @p:p NULL なら、何もしない */

void mPopupProgressThreadEnd(mPopupProgress *p)
{
	if(!p) return;
	
	mGuiThreadLock();

	mGuiQuit();
	mGuiThreadWakeup();

	mGuiThreadUnlock();
}

/**@ [スレッド] バーの最大値をセット
 *
 * @p:p NULL なら、何もしない */

void mPopupProgressThreadSetMax(mPopupProgress *p,int max)
{
	if(p)
	{
		mGuiThreadLock();

		(p->pg.progress)->pb.max = max;

		mGuiThreadUnlock();
	}
}

/**@ [スレッド] プログレスバーの位置をセット */

void mPopupProgressThreadSetPos(mPopupProgress *p,int pos)
{
	if(p)
	{
		mGuiThreadLock();

		if(mProgressBarSetPos(p->pg.progress, pos))
			mGuiThreadWakeup();

		mGuiThreadUnlock();
	}
}

/**@ [スレッド] プログレスバー位置を+1 */

void mPopupProgressThreadIncPos(mPopupProgress *p)
{
	if(p)
	{
		mGuiThreadLock();
		
		mProgressBarIncPos(p->pg.progress);
		mGuiThreadWakeup();

		mGuiThreadUnlock();
	}
}

/**@ [スレッド] 現在位置に加算 */

void mPopupProgressThreadAddPos(mPopupProgress *p,int add)
{
	if(p)
	{
		mGuiThreadLock();

		if(mProgressBarAddPos(p->pg.progress, add))
			mGuiThreadWakeup();

		mGuiThreadUnlock();
	}
}

/**@ [スレッド] サブステップ開始 */

void mPopupProgressThreadSubStep_begin(mPopupProgress *p,int stepnum,int max)
{
	if(p)
	{
		mGuiThreadLock();
		mProgressBarSubStep_begin(p->pg.progress, stepnum, max);
		mGuiThreadUnlock();
	}
}

/**@ [スレッド] サブステップ開始 (1ステップのみ) */

void mPopupProgressThreadSubStep_begin_onestep(mPopupProgress *p,int stepnum,int max)
{
	if(p)
	{
		mGuiThreadLock();
		mProgressBarSubStep_begin_onestep(p->pg.progress, stepnum, max);
		mGuiThreadUnlock();
	}
}

/**@ [スレッド] サブステップのカウントを +1 */

void mPopupProgressThreadSubStep_inc(mPopupProgress *p)
{
	if(p)
	{
		mGuiThreadLock();

		if(mProgressBarSubStep_inc(p->pg.progress))
			mGuiThreadWakeup();

		mGuiThreadUnlock();
	}
}

