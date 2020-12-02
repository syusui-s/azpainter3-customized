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
 * <X11> mCursor
 *****************************************/

#include "mSysX11.h"
#include "mCursor.h"


/**
@defgroup cursor mCursor
@brief カーソル

@ingroup group_system
@{

@file mCursor.h
@enum MCURSOR_DEF
*/


/** 解放 */

void mCursorFree(mCursor cur)
{
	if(cur)
		XFreeCursor(XDISP, cur);
}

/** データから作成(白黒)
 *
 * イメージデータは、下位ビットから上位ビットへの方向で。 @n
 *
 * [0byte] 幅 @n
 * [1byte] 高さ @n
 * [2byte] hotx @n
 * [3byte] hoty @n
 * [4~]    イメージデータ(1bit) 0=白 1=黒 @n
 * [4+(w+7)/8*h] マスクデータ(1bit) 0=透明 1=不透明
 */

mCursor mCursorCreateMono(const uint8_t *buf)
{
	int w,h;
	Pixmap img,mask;
	XColor col[2];
	Cursor curid = 0;

	w = buf[0];
	h = buf[1];

	//Pixmap 作成

	img  = XCreateBitmapFromData(XDISP, MAPP_SYS->root_window,
				(char *)buf + 4, w, h);

	mask = XCreateBitmapFromData(XDISP, MAPP_SYS->root_window,
				(char *)buf + 4 + ((w + 7) >> 3) * h, w, h);

	if(!img || !mask) goto END;

	//カーソル作成

	col[0].pixel = XBlackPixel(XDISP, MAPP_SYS->screen);
	col[0].flags = DoRed | DoGreen | DoBlue;
	col[0].red = col[0].green = col[0].blue = 0;

	col[1].pixel = XWhitePixel(XDISP, MAPP_SYS->screen);
	col[1].flags = DoRed | DoGreen | DoBlue;
	col[1].red = col[1].green = col[1]. blue = 0xffff;

	curid = XCreatePixmapCursor(XDISP, img, mask, col, col + 1, buf[2], buf[3]);

END:
	if(img) XFreePixmap(XDISP, img);
	if(mask) XFreePixmap(XDISP, mask);

	return curid;
}

/** 標準カーソル取得 */

mCursor mCursorGetDefault(int type)
{
	mCursor cur;

	switch(type)
	{
		case MCURSOR_DEF_HSPLIT:
			cur = MAPP_SYS->cursor_hsplit;
			break;
		case MCURSOR_DEF_VSPLIT:
			cur = MAPP_SYS->cursor_vsplit;
			break;
		default:
			cur = 0;
			break;
	}

	return cur;
}

/** @} */
