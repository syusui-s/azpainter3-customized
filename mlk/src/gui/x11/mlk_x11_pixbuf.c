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
 * <X11> mPixbuf
 *****************************************/

#include <stdlib.h>

#define MLKX11_INC_UTIL
#define MLKX11_INC_XSHM
#include "mlk_x11.h"

#include "mlk_pixbuf.h"
#include "mlk_widget_def.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_pixbuf.h"


//-------------------------

#define _PIXBUF(p)  ((_pixbuf *)(p))

typedef struct
{
	mPixbuf b;
	mPixbufPrivate pv;

	XImage *ximg;

#if !defined(MLK_X11_NO_XSHM)
	XShmSegmentInfo xshminfo;
#endif

}_pixbuf;

//-------------------------


//====================================
// ピクセルバッファ関数
//====================================


/** 32bit */

static void _setbuf32(uint8_t *pd,mPixCol col)
{
	*((uint32_t *)pd) = col;
}

/** 32bit XOR */

static void _setbuf32_xor(uint8_t *pd,mPixCol col)
{
	*((uint32_t *)pd) ^= 0xffffffff;
}

/** 24bit */

static void _setbuf24(uint8_t *pd,mPixCol col)
{
#ifdef MLK_BIG_ENDIAN
	pd[0] = (uint8_t)(col >> 16);
	pd[1] = (uint8_t)(col >> 8);
	pd[2] = (uint8_t)col;
#else
	pd[0] = (uint8_t)col;
	pd[1] = (uint8_t)(col >> 8);
	pd[2] = (uint8_t)(col >> 16);
#endif
}

/** 24bit XOR */

static void _setbuf24_xor(uint8_t *pd,mPixCol col)
{
	pd[0] ^= 0xff;
	pd[1] ^= 0xff;
	pd[2] ^= 0xff;
}

/** 16bit */

static void _setbuf16(uint8_t *pd,mPixCol col)
{
	*((uint16_t *)pd) = col;
}

/** 16bit XOR */

static void _setbuf16_xor(uint8_t *pd,mPixCol col)
{
	*((uint16_t *)pd) ^= 0xffff;
}

/** 色取得 - 32bit */

static mPixCol _getbuf32(uint8_t *buf)
{
	return *((uint32_t *)buf);
}

/** 色取得 - 24bit */

static mPixCol _getbuf24(uint8_t *buf)
{
#ifdef MLK_BIG_ENDIAN
	return ((uint32_t)buf[0] << 16) | (buf[1] << 8) | buf[2];
#else
	return ((uint32_t)buf[2] << 16) | (buf[1] << 8) | buf[0];
#endif
}

/** 色取得 - 16bit */

static mPixCol _getbuf16(uint8_t *buf)
{
	return *((uint16_t *)buf);
}


//==========================
// sub
//==========================


/** XImage 作成 */

static XImage *_create_ximage(_pixbuf *p,int w,int h)
{
	mAppX11 *app = MLKAPPX11;
	XImage *ximg = NULL;

	//-------- XShm

#if !defined(MLK_X11_NO_XSHM)

	int shmid;

	if(app->fsupport & MLKX11_SUPPORT_XSHM)
	{
		//XImage
		
		ximg = XShmCreateImage(app->display, CopyFromParent,
					app->depth, ZPixmap, NULL, &p->xshminfo, w, h);

		//バッファ (共有メモリ)
		
		if(ximg)
		{
			shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * h, IPC_CREAT | 0777);
			
			if(shmid < 0)
			{
				//失敗時は通常 XImage を作る
				
				XDestroyImage(ximg);
				ximg = NULL;
			}
			else
			{
				p->xshminfo.shmid = shmid;
				p->xshminfo.shmaddr = ximg->data = (char *)shmat(shmid, 0, 0);
				p->xshminfo.readOnly = 0;
				
				XShmAttach(app->display, &p->xshminfo);
			}
		}
	}

#endif

	//------ 通常 XImage

	if(!ximg)
	{
		//XImage
	
		ximg = XCreateImage(app->display, CopyFromParent,
					app->depth, ZPixmap, 0, NULL, w, h, 32, 0);
		
		if(!ximg) return NULL;
		
		//イメージバッファ確保
		//[!] XDestroyImage() では free() を使って解放されるので、通常の malloc() を使う。
		
		ximg->data = (char *)malloc(ximg->bytes_per_line * h);
		
		if(!ximg->data)
		{
			XDestroyImage(ximg);
			return NULL;
		}
	}

	return ximg;
}


//==========================
// backend 関数
//==========================


/** 構造体確保 */

mPixbuf *__mX11PixbufAlloc(void)
{
	return (mPixbuf *)mMalloc0(sizeof(_pixbuf));
}

/** イメージ削除
 *
 * pixbuf 自体は残す。リサイズ時にイメージを再作成できるように。 */

void __mX11PixbufDeleteImage(mPixbuf *pixbuf)
{
	_pixbuf *p = _PIXBUF(pixbuf);

	//XShm

#if !defined(MLK_X11_NO_XSHM)

	if(p->xshminfo.shmaddr)
	{
		XShmDetach(MLKX11_DISPLAY, &p->xshminfo);
	
		XDestroyImage(p->ximg);
	
		shmdt(p->xshminfo.shmaddr);

		p->ximg = NULL;
		p->xshminfo.shmaddr = NULL;
	}

#endif

	//XImage

	if(p->ximg)
	{
		//[!] buf は free() で解放される
		
		XDestroyImage(p->ximg);
		p->ximg = NULL;
	}

	//
	
	pixbuf->buf = NULL;
}

/** イメージ作成 */

mlkbool __mX11PixbufCreate(mPixbuf *pixbuf,int w,int h)
{
	_pixbuf *p = _PIXBUF(pixbuf);
	XImage *ximg;
	mAppBackend *bkend = &MLKAPP->bkend;

	//XImage
		
	ximg = _create_ximage(p, w, h);
	if(!ximg) return FALSE;

	p->ximg = ximg;

	pixbuf->width = w;
	pixbuf->height = h;
	pixbuf->buf = (uint8_t *)ximg->data;
	pixbuf->pixel_bytes = (ximg->bits_per_pixel + 7) >> 3;
	pixbuf->line_bytes = ximg->bytes_per_line;

	//関数

	if(!bkend->pixbuf_setbufcol)
	{
		if(pixbuf->pixel_bytes == 4)
		{
			bkend->pixbuf_setbufcol = _setbuf32;
			bkend->pixbuf_setbufxor = _setbuf32_xor;
			bkend->pixbuf_getbuf = _getbuf32;
		}
		else if(pixbuf->pixel_bytes == 3)
		{
			bkend->pixbuf_setbufcol = _setbuf24;
			bkend->pixbuf_setbufxor = _setbuf24_xor;
			bkend->pixbuf_getbuf = _getbuf24;
		}
		else
		{
			bkend->pixbuf_setbufcol = _setbuf16;
			bkend->pixbuf_setbufxor = _setbuf16_xor;
			bkend->pixbuf_getbuf = _getbuf16;
		}
	}

	return TRUE;
}

/** リサイズ */

mlkbool __mX11PixbufResize(mPixbuf *pixbuf,int w,int h,int neww,int newh)
{
	if(pixbuf->width == neww && pixbuf->height == newh)
		return TRUE;
	else
	{
		__mX11PixbufDeleteImage(pixbuf);

		return __mX11PixbufCreate(pixbuf, neww, newh);
	}
}

/** ウィンドウイメージを画面に転送 */

void __mX11PixbufRender(mWindow *win,int x,int y,int w,int h)
{
	_pixbuf *p = _PIXBUF(win->win.pixbuf);
	mAppX11 *app = MLKAPPX11;

#if !defined(MLK_X11_NO_XSHM)

	if(p->xshminfo.shmaddr)
	{
		XShmPutImage(app->display, MLKX11_WINDATA(win)->winid, app->gc_def,
			p->ximg, x, y, x, y, w, h, False);

		return;
	}

#endif

	if(p->ximg)
	{
		XPutImage(app->display, MLKX11_WINDATA(win)->winid, app->gc_def,
			p->ximg, x, y, x, y, w, h);
	}
}

