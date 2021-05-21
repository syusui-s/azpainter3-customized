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

#ifndef MLK_CURSOR_H
#define MLK_CURSOR_H

enum MCURSOR_TYPE
{
	MCURSOR_TYPE_NONE,
	MCURSOR_TYPE_DEFAULT,
	MCURSOR_TYPE_WE_ARROW,
	MCURSOR_TYPE_NS_ARROW,
	MCURSOR_TYPE_NWSE_ARROW,
	MCURSOR_TYPE_NESW_ARROW,
	MCURSOR_TYPE_RESIZE_W,
	MCURSOR_TYPE_RESIZE_E,
	MCURSOR_TYPE_RESIZE_N,
	MCURSOR_TYPE_RESIZE_S,
	MCURSOR_TYPE_RESIZE_NW,
	MCURSOR_TYPE_RESIZE_NE,
	MCURSOR_TYPE_RESIZE_SW,
	MCURSOR_TYPE_RESIZE_SE,
	MCURSOR_TYPE_RESIZE_COL,
	MCURSOR_TYPE_RESIZE_ROW,

	MCURSOR_TYPE_MAX
};

enum MCURSOR_RESIZE_FLAGS
{
	MCURSOR_RESIZE_TOP    = 1,
	MCURSOR_RESIZE_BOTTOM = 2,
	MCURSOR_RESIZE_LEFT   = 4,
	MCURSOR_RESIZE_RIGHT  = 8,
	MCURSOR_RESIZE_TOP_LEFT = 5,
	MCURSOR_RESIZE_TOP_RIGHT = 9,
	MCURSOR_RESIZE_BOTTOM_LEFT = 6,
	MCURSOR_RESIZE_BOTTOM_RIGHT = 10
};

#ifdef __cplusplus
extern "C" {
#endif

void mCursorFree(mCursor p);
mCursor mCursorLoad(const char *name);
mCursor mCursorLoad_names(const char *names);
mCursor mCursorLoad_type(int type);
mCursor mCursorCreate1bit(const uint8_t *buf);
mCursor mCursorCreateEmpty(void);
mCursor mCursorCreateRGBA(int width,int height,int hotspot_x,int hotspot_y,const uint8_t *buf);
mCursor mCursorLoadFile_png(const char *filename,int hotspot_x,int hotspot_y);

int mCursorGetType_resize(int flags);

mCursor mCursorCache_getCursor_type(int type);
void mCursorCache_addCursor(mCursor cur);
void mCursorCache_release(mCursor cur);

#ifdef __cplusplus
}
#endif

#endif
