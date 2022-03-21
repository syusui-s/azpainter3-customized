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

#ifndef MLK_ACCELERATOR_H
#define MLK_ACCELERATOR_H

#ifdef __cplusplus
extern "C" {
#endif

mAccelerator *mAcceleratorNew(void);
void mAcceleratorDestroy(mAccelerator *p);

void mAcceleratorSetDefaultWidget(mAccelerator *p,mWidget *wg);

void mAcceleratorAdd(mAccelerator *p,int cmdid,uint32_t key,mWidget *wg);
void mAcceleratorClear(mAccelerator *p);

uint32_t mAcceleratorGetKey(mAccelerator *p,int cmdid);
void mAcceleratorGetKeyText(mStr *str,uint32_t key);

uint32_t mAcceleratorGetAccelKey_keyevent(mEvent *ev);

#ifdef __cplusplus
}
#endif

#endif
