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

#ifndef MLIB_APPTIMER_H
#define MLIB_APPTIMER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mNanoTime mNanoTime;

mBool mAppTimerGetMinTime(mList *list,mNanoTime *nt_min);
void mAppTimerProc(mList *list);

void mAppTimerAppend(mList *list,mWidget *wg,uint32_t timerid,uint32_t msec,intptr_t param);
mBool mAppTimerDelete(mList *list,mWidget *wg,uint32_t timerid);
void mAppTimerDeleteWidget(mList *list,mWidget *wg);
mBool mAppTimerIsExist(mList *list,mWidget *wg,uint32_t timerid);

#ifdef __cplusplus
}
#endif

#endif
