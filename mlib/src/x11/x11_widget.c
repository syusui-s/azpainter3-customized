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
 * <X11> mWidget 関数
 *****************************************/

#include "mSysX11.h"

#include "mWindowDef.h"
#include "mAppTimer.h"


/**

@addtogroup widget
@{

*/


/** ウィジェット座標を指定ウィジェットの相対座標に変換
 * 
 * from,to : NULL でルート */

void mWidgetMapPoint(mWidget *from,mWidget *to,mPoint *pt)
{
	Window id,id_from;
	int x,y;
	
	if(!from)
		id_from = MAPP_SYS->root_window;
	else
	{
		id_from = WINDOW_XID(from->toplevel);

		pt->x += from->absX;
		pt->y += from->absY;
	}
	
	//
	
	XTranslateCoordinates(XDISP,
		id_from, (to)? WINDOW_XID(to->toplevel): MAPP_SYS->root_window,
		pt->x, pt->y, &x, &y, &id);
		
	if(to)
	{
		x -= to->absX;
		y -= to->absY;
	}

	pt->x = x, pt->y = y;
}


/** タイマー追加
 * 
 * 同じ ID のタイマーが存在する場合は置き換わる。
 * 
 * @param msec 間隔 (ミリセカンド) */

void mWidgetTimerAdd(mWidget *p,uint32_t timerid,uint32_t msec,intptr_t param)
{
	mAppTimerAppend(&MAPP_SYS->listTimer, p, timerid, msec, param);
}

/** 指定タイマーが存在するか */

mBool mWidgetTimerIsExist(mWidget *p,uint32_t timerid)
{
	return mAppTimerIsExist(&MAPP_SYS->listTimer, p, timerid);
}

/** タイマー削除
 * 
 * @return タイマーを削除したか */

mBool mWidgetTimerDelete(mWidget *p,uint32_t timerid)
{
	return mAppTimerDelete(&MAPP_SYS->listTimer, p, timerid);
}

/** タイマーすべて削除 */

void mWidgetTimerDeleteAll(mWidget *p)
{
	mAppTimerDeleteWidget(&MAPP_SYS->listTimer, p);
}

/** 現在のカーソル位置を指定ウィジェットの座標で取得
 *
 * @param p NULL でルート座標 */

void mWidgetGetCursorPos(mWidget *p,mPoint *pt)
{
	Window win;
	int x,y,rx,ry;
	unsigned int btt;

	XQueryPointer(XDISP,
		(p)? WINDOW_XID(p->toplevel): MAPP_SYS->root_window,
		&win, &win, &rx, &ry, &x, &y, &btt);

	if(!p)
	{
		pt->x = rx;
		pt->y = ry;
	}
	else
	{
		pt->x = x - p->absX;
		pt->y = y - p->absY;
	}
}

/** @} */
