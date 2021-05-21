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
 * ナノ時間操作関数
 *****************************************/

/* [Linux]
 * glibc 2.17 以前では、clock_gettime() を使う場合、librt のリンクが必要 */

#include "mlk_platform.h"

//----------

#if defined(MLK_PLATFORM_MACOS)
/* MacOS */
#include <mach/mach_time.h>
#define __MDEF_MAC

#else
/* Other */

#include <unistd.h>

#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
/* clock_gettime() */
#include <time.h>
#define __MDEF_CLOCK_GETTIME

#else
/* gettimeofday() */
#include <sys/time.h>
#define __MDEF_GETTIMEOFDAY

#endif

#endif

//----------

#include "mlk.h"
#include "mlk_nanotime.h"


/**@ 現在のナノ時間を取得 */

void mNanoTimeGet(mNanoTime *nt)
{
#if defined(__MDEF_MAC)
	/* MacOS */

	static mach_timebase_info_data_t info = {0,0};
	uint64_t t;

	if(info.denom == 0)
		mach_timebase_info(&info);

	t = mach_absolute_time() * info.numer / info.denom;

	nt->sec = t / ((uint64_t)1000 * 1000 * 1000);
	nt->ns  = t % ((uint64_t)1000 * 1000 * 1000);

#elif defined(__MDEF_CLOCK_GETTIME)

	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	nt->sec = ts.tv_sec;
	nt->ns  = ts.tv_nsec;

#else

	struct timeval tv;

	gettimeofday(&tv, NULL);

	nt->sec = tv.tv_sec;
	nt->ns  = tv.tv_usec * 1000;

#endif
}

/**@ ナノ秒を追加 */

void mNanoTimeAdd(mNanoTime *nt,uint64_t ns)
{
	ns += nt->ns;
	
	nt->sec += ns / ((uint64_t)1000 * 1000 * 1000);
	nt->ns = ns % ((uint64_t)1000 * 1000 * 1000);
}

/**@ ミリ秒を追加 */

void mNanoTimeAdd_ms(mNanoTime *nt,uint32_t ms)
{
	mNanoTimeAdd(nt, (uint64_t)ms * 1000 * 1000);
}

/**@ 2つの時間を比較
 *
 * @r:0 で nt1 == nt2。\
 * -1 で nt1 < nt2。\
 * 1 で nt1 > nt2。 */

int mNanoTimeCompare(const mNanoTime *nt1,const mNanoTime *nt2)
{
	if(nt1->sec < nt2->sec)
		return -1;
	else if(nt1->sec > nt2->sec)
		return 1;
	else
	{
		//秒が同じ

		if(nt1->ns < nt2->ns)
			return -1;
		else if(nt1->ns > nt2->ns)
			return 1;
		else
			return 0;
	}
}

/**@ 時間の差を取得
 * 
 * @d:nt1 - nt2 の時間差を取得する。\
 * nt1 の方が小さい場合は失敗する。
 *
 * @p:dst n1 または n2 と同じポインタでも構わない
 * @r:nt1 の方が小さい場合、FALSE */

mlkbool mNanoTimeSub(mNanoTime *dst,const mNanoTime *nt1,const mNanoTime *nt2)
{
	if(mNanoTimeCompare(nt1, nt2) < 0)
		return FALSE;
	else
	{
		dst->sec = nt1->sec - nt2->sec;
		
		if(nt1->ns >= nt2->ns)
			dst->ns = nt1->ns - nt2->ns;
		else
		{
			dst->sec--;
			dst->ns = (int64_t)(nt1->ns - nt2->ns) + 1000 * 1000 * 1000;
		}
		
		return TRUE;
	}
}

/**@ ミリ秒に変換 */

uint32_t mNanoTimeToMilliSec(const mNanoTime *nt)
{
	return nt->sec * 1000 + nt->ns / (1000 * 1000);
}

/**@ 経過時間を表示
 *
 * @d:nt を開始時間とし、現在時間からその時間を引いて、経過時間を stderr に表示する。 */

void mNanoTimePutProcess(const mNanoTime *nt)
{
	mNanoTime cur;

	mNanoTimeGet(&cur);
	mNanoTimeSub(&cur, &cur, nt);

	if(cur.sec)
		mDebug("%u sec %u ns\n", (uint32_t)cur.sec, cur.ns);
	else
		mDebug("%u ns\n", cur.ns);
}

