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

#ifndef MLIB_X11_XINPUT2_H
#define MLIB_X11_XINPUT2_H

#if defined(MLIB_ENABLE_PENTABLET) && defined(HAVE_XEXT_XINPUT2)

#ifdef __cplusplus
extern "C" {
#endif

int mX11XI2_init();

void mX11XI2_pt_selectEvent(Window id);
mBool mX11XI2_pt_grab(Window id,Cursor cursor,int device_id);
void mX11XI2_pt_ungrab();

uint32_t mX11XI2_getEventInfo(void *event,double *pressure,mBool *pentablet);

#ifdef __cplusplus
}
#endif

#endif

#endif
