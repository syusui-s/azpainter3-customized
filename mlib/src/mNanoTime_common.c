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
 * mNanoTime - 共通
 *****************************************/

#include "mDef.h"
#include "mNanoTime.h"


/**
@defgroup nanotime mNanoTime
@brief ナノ時間関数

@ingroup group_util
@{

@file mNanoTime.h
*/


/** 指定ナノ秒を追加 */

void mNanoTimeAdd(mNanoTime *nt,uint64_t nsec)
{
	nsec += nt->nsec;
	
	nt->sec += nsec / (1000 * 1000 * 1000);
	nt->nsec = nsec % (1000 * 1000 * 1000);
}

/** 指定ミリ秒を追加 */

void mNanoTimeAddMilliSec(mNanoTime *nt,int msec)
{
	mNanoTimeAdd(nt, (uint64_t)msec * 1000 * 1000);
}

/** 時間を比較
 * 
 * @retval -1 nt1 の方が小さい
 * @retval 1  nt1 の方が大きい
 * @retval 0  nt1 == nt2 */

int mNanoTimeCompare(mNanoTime *nt1,mNanoTime *nt2)
{
	if(nt1->sec < nt2->sec)
		return -1;
	else if(nt1->sec > nt2->sec)
		return 1;
	else
	{
		//秒が同じ

		if(nt1->nsec < nt2->nsec)
			return -1;
		else if(nt1->nsec > nt2->nsec)
			return 1;
		else
			return 0;
	}
}

/** nt1 から nt2 を引いた値を取得
 * 
 * nt1 の方が小さい場合は失敗する。
 * 
 * @return nt1 の方が小さい場合 FALSE */

mBool mNanoTimeSub(mNanoTime *dst,mNanoTime *nt1,mNanoTime *nt2)
{
	if(mNanoTimeCompare(nt1, nt2) < 0)
		return FALSE;
	else
	{
		dst->sec = nt1->sec - nt2->sec;
		
		if(nt1->nsec >= nt2->nsec)
			dst->nsec = nt1->nsec - nt2->nsec;
		else
		{
			dst->sec--;
			dst->nsec = (int64_t)(nt1->nsec - nt2->nsec) + 1000 * 1000 * 1000;
		}
		
		return TRUE;
	}
}

/* @} */
