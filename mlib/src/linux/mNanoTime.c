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
 * <Linux> mNanoTime
 *****************************************/

/* glibc 2.17 以前では、clock_gettime() を使う場合 librt のリンクが必要 */


#if defined(__APPLE__) && defined(__MACH__)
/* MacOS */

#include <mach/mach_time.h>
#define __MDEF_MAC

#else
/* Other */

#include <unistd.h>

#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
/* clock_gettime() */
# include <time.h>
# define __MDEF_CLOCK_GETTIME
#else
/* gettimeofday() */
# include <sys/time.h>
# define __MDEF_GETTIMEOFDAY
#endif

#endif

//------

#include "mDef.h"
#include "mNanoTime.h"


/**
@addtogroup nanotime
 
@{
*/

/** 現在のナノ時間を取得 */

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
	nt->nsec = t % ((uint64_t)1000 * 1000 * 1000);

#elif defined(__MDEF_CLOCK_GETTIME)

	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	nt->sec = ts.tv_sec;
	nt->nsec = ts.tv_nsec;

#else

	struct timeval tv;

	gettimeofday(&tv, NULL);

	nt->sec = tv.tv_sec;
	nt->nsec = tv.tv_usec * 1000;

#endif
}

/** @} */
