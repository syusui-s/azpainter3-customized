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
 * mFont (pixbuf に描画)
 *****************************************/

#include "mlk.h"
#include "mlk_font.h"
#include "mlk_pixbuf.h"


//-------------

#define _PARAM(p)  ((mFontDrawParam_pixbuf *)(p))

//-------------


//================================
// 描画関数
//================================


/** mono */

static void _setpix_mono(int x,int y,void *p)
{
	mPixbufSetPixel(_PARAM(p)->pixbuf, x, y, _PARAM(p)->pixcol);
}

/** gray */

static void _setpix_gray(int x,int y,int a,void *p)
{
	mPixbufBlendPixel(_PARAM(p)->pixbuf, x, y, _PARAM(p)->rgbcol, a);
}

/** LCD */

static void _setpix_lcd(int x,int y,int ra,int ga,int ba,void *p)
{
	mPixbufBlendPixel_lcd(_PARAM(p)->pixbuf, x, y, _PARAM(p)->rgbcol, ra, ga, ba);
}

/** 下線 */

static void _drawunderline(int x,int y,int w,int h,void *p)
{
	mPixbufFillBox(_PARAM(p)->pixbuf, x, y, w, h, _PARAM(p)->pixcol);
}

//-----------------

static mFontDrawInfo g_drawinfo = {
	.setpix_mono = _setpix_mono,
	.setpix_gray = _setpix_gray,
	.setpix_lcd = _setpix_lcd,
	.draw_underline = _drawunderline
};


//================================
// main
//================================


/**@ mPixbuf 描画用の mFontDrawInfo を取得
 *
 * @d:値は変更されないため、スレッドから使用しても良い。 */

mFontDrawInfo *mFontGetDrawInfo_pixbuf(void)
{
	return &g_drawinfo;
}

/**@ mPixbuf 描画時用のパラメータ値をセット */

void mFontSetDrawParam_pixbuf(mFontDrawParam_pixbuf *p,mPixbuf *pixbuf,mRgbCol col)
{
	p->pixbuf = pixbuf;
	p->rgbcol = col;
	p->pixcol = mRGBtoPix(col);
}

/**@ mPixbuf にテキスト描画 */

void mFontDrawText_pixbuf(mFont *p,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col)
{
	mFontDrawParam_pixbuf param;

	mFontSetDrawParam_pixbuf(&param, img, col);

	mFontDrawText(p, x, y, text, len, &g_drawinfo, &param);
}

/**@ mPixbuf にテキスト描画 (ホットキー処理付き) */

void mFontDrawText_pixbuf_hotkey(mFont *p,mPixbuf *img,int x,int y,
	const char *text,int len,mRgbCol col)
{
	mFontDrawParam_pixbuf param;

	mFontSetDrawParam_pixbuf(&param, img, col);

	mFontDrawText_hotkey(p, x, y, text, len, &g_drawinfo, &param);
}

/**@ mPixbuf にテキスト描画 (UTF-32) */

void mFontDrawText_pixbuf_utf32(mFont *p,mPixbuf *img,int x,int y,
	const mlkuchar *text,int len,mRgbCol col)
{
	mFontDrawParam_pixbuf param;

	mFontSetDrawParam_pixbuf(&param, img, col);

	mFontDrawText_utf32(p, x, y, text, len, &g_drawinfo, &param);
}
