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
 * PopupThread
 *
 * スレッド処理中にポップアップのプログレスバーを表示する
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_popup_progress.h"
#include "mlk_progressbar.h"
#include "mlk_thread.h"

#include "popup_thread.h"
#include "mainwindow.h"

/*
 * スレッド中、
 * ステータスバーのヘルプ部分か、
 * メインウィンドウ左下 (ステータスバー非表示時) にプログレスバーを表示する。
 */

//-------------------------

typedef struct
{
	MLK_POPUPPROGRESS_DEF

	void *data;
	PopupThreadFunc func;
	int ret;
}PopupThread;

//-------------------------


/* スレッド関数 */

static void _thread_func(mThread *th)
{
	PopupThread *p = (PopupThread *)th->param;

	p->ret = (p->func)(MLK_POPUPPROGRESS(p), p->data);

	mPopupProgressThreadEnd(MLK_POPUPPROGRESS(p));
}


/** スレッド実行
 *
 * data: スレッド関数に渡すデータポインタ
 * func: スレッド実行関数
 * return: スレッド関数の戻り値。-1 でエラー */

int PopupThread_run(void *data,PopupThreadFunc func)
{
	PopupThread *p;
	mWidget *wg;
	mBox box;
	int ret;

	p = (PopupThread *)mPopupProgressNew(sizeof(PopupThread), MPROGRESSBAR_S_FRAME);
	if(!p) return -1;

	p->data = data;
	p->func = func;

	//スレッド実行

	wg = MainWindow_getProgressBarPos(&box);

	mPopupProgressRun(MLK_POPUPPROGRESS(p), wg, 0, -(p->pg.progress->wg.hintH), &box,
		MPOPUP_F_LEFT | MPOPUP_F_BOTTOM | MPOPUP_F_GRAVITY_TOP, 130, _thread_func);

	//終了

	ret = p->ret;

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

