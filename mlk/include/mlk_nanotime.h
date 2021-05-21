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

#ifndef MLK_NANOTIME_H
#define MLK_NANOTIME_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mNanoTime
{
	uint64_t sec;
	uint32_t ns;
}mNanoTime;

void mNanoTimeGet(mNanoTime *nt);
void mNanoTimeAdd(mNanoTime *nt,uint64_t ns);
void mNanoTimeAdd_ms(mNanoTime *nt,uint32_t ms);
int mNanoTimeCompare(const mNanoTime *nt1,const mNanoTime *nt2);
mlkbool mNanoTimeSub(mNanoTime *dst,const mNanoTime *nt1,const mNanoTime *nt2);
uint32_t mNanoTimeToMilliSec(const mNanoTime *nt);
void mNanoTimePutProcess(const mNanoTime *nt);

#ifdef __cplusplus
}
#endif

#endif
