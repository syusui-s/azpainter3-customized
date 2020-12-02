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

#ifndef MLIB_X11_WINDOW_H
#define MLIB_X11_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

void mX11WindowSetEventMask(mWindow *p,long mask,mBool append);
void mX11WindowSetWindowType(mWindow *p,const char *name);
void mX11WindowSetUserTime(mWindow *p,unsigned long time);
void mX11WindowSetDecoration(mWindow *p);

#ifdef __cplusplus
}
#endif

#endif
