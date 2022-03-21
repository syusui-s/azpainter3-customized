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

/*****************************************
 * <X11> カーソル
 *****************************************/

#define MLKX11_INC_XCURSOR
#include "mlk_x11.h"

#include "mlk_cursor.h"


/** カーソル解放 */

void __mX11CursorFree(mCursor cur)
{
	if(cur)
		XFreeCursor(MLKX11_DISPLAY, (Cursor)cur);
}

/** デフォルトテーマから指定名のカーソル読み込み
 *
 * return: 読み込めなかったら NULL */

mCursor __mX11CursorLoad(const char *name)
{
	return XcursorLibraryLoadCursor(MLKX11_DISPLAY, name);
}

/** 1bit データからカーソル作成 */

mCursor __mX11CursorCreate1bit(const uint8_t *buf)
{
	Display *disp = MLKX11_DISPLAY;
	int w,h;
	Pixmap img,mask;
	XColor col[2];
	Cursor cur = 0;

	w = buf[0];
	h = buf[1];

	//Pixmap 作成

	img = XCreateBitmapFromData(disp, MLKX11_ROOTWINDOW,
			(char *)buf + 4, w, h);

	mask = XCreateBitmapFromData(disp, MLKX11_ROOTWINDOW,
			(char *)buf + 4 + ((w + 7) >> 3) * h, w, h);

	//カーソル作成

	if(img && mask)
	{
		col[0].pixel = 0;
		col[0].flags = DoRed | DoGreen | DoBlue;
		col[0].red = col[0].green = col[0].blue = 0;

		col[1].pixel = XWhitePixel(disp, MLKAPPX11->screen);
		col[1].flags = DoRed | DoGreen | DoBlue;
		col[1].red = col[1].green = col[1]. blue = 0xffff;

		cur = XCreatePixmapCursor(disp, img, mask, col, col + 1, buf[2], buf[3]);
	}

	//

	if(img) XFreePixmap(disp, img);
	if(mask) XFreePixmap(disp, mask);

	return cur;
}

/** RGBA イメージからカーソル作成 */

mCursor __mX11CursorCreateRGBA(int width,int height,int hotspot_x,int hotspot_y,const uint8_t *buf)
{
	XcursorImage *img;
	XcursorPixel *pd;
	int ix,iy;
	mCursor cur;

	//イメージ作成

	img = XcursorImageCreate(width, height);
	if(!img) return 0;

	img->xhot = hotspot_x;
	img->yhot = hotspot_y;

	//変換

	pd = img->pixels;

	for(iy = height; iy; iy--)
	{
		for(ix = width; ix; ix--, buf += 4)
		{
			*(pd++) = ((uint32_t)buf[3] << 24) | (buf[0] << 16) | (buf[1] << 8) | buf[2];
		}
	}

	//カーソルに変換

	cur = XcursorImageLoadCursor(MLKX11_DISPLAY, img);

	XcursorImageDestroy(img);

	return cur;
}

