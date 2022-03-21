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
 * サムネイル画像読み込み
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_loadimage.h"
#include "mlk_imageconv.h"
#include "mlk_io.h"
#include "mlk_zlib.h"

#include "apd_v4_format.h"



//===============================
// APD v4
//===============================


/* 開く */

static mlkerr _apd4_open(mLoadImage *pli)
{
	apd4load *apd;
	mlkerr ret;
	uint32_t id,size;
	mSize imgsize;

	//apd 開く

	ret = apd4load_open(&apd, pli->open.filename, NULL);
	if(ret) return ret;

	pli->handle = apd;

	//チャンク

	while(1)
	{
		ret = apd4load_nextChunk(apd, &id, &size);

		if(ret == -1)
			return MLKERR_UNSUPPORTED;
		else if(ret)
			return ret;

		//

		if(id == APD4_CHUNK_ID_THUMBNAIL)
		{
			ret = apd4load_readChunk_thumbnail(apd, &imgsize);
			if(ret) return ret;

			break;
		}
	}

	//情報

	pli->width = imgsize.w;
	pli->height = imgsize.h;
	pli->coltype = MLOADIMAGE_COLTYPE_RGBA;
	pli->bits_per_sample = 8;

	return MLKERR_OK;
}

/* 閉じる */

static void _apd4_close(mLoadImage *p)
{
	if(p->handle)
	{
		apd4load_close((apd4load *)p->handle);
		p->handle = NULL;
	}

	mLoadImage_closeHandle(p);
}

/* 画像の取得 */

static mlkerr _apd4_getimage(mLoadImage *pli)
{
	apd4load *apd = (apd4load *)pli->handle;
	uint8_t **ppbuf;
	int iy,width;
	mlkerr ret;

	ppbuf = pli->imgbuf;
	width = pli->width;

	for(iy = pli->height; iy; iy--, ppbuf++)
	{
		ret = apd4load_readChunk_thumbnail_image(apd, *ppbuf, width);
		if(ret) return ret;

		//RGB -> RGBA

		mImageConv_rgb8_to_rgba8_extend(*ppbuf, width);
	}
	
	return MLKERR_OK;
}


//===============================
// APD v3
//===============================

typedef struct
{
	mIO *io;
	uint32_t compsize;
}_load_apd3;


/* 開く */

static mlkerr _apd3_open(mLoadImage *pli)
{
	_load_apd3 *p;
	mIO *io;
	uint32_t width,height,id,size;
	mlkerr ret;

	ret = mLoadImage_createHandle(pli, sizeof(_load_apd3), MIO_ENDIAN_BIG);
	if(ret) return ret;

	p = (_load_apd3 *)pli->handle;

	io = p->io;

	//基本情報

	if(mIO_seekCur(io, 8)
		|| mIO_read32(io, &width)
		|| mIO_read32(io, &height)
		|| mIO_seekCur(io, 5))
		return MLKERR_DAMAGED;

	pli->width = width;
	pli->height = height;
	pli->coltype = MLOADIMAGE_COLTYPE_RGBA;
	pli->bits_per_sample = 8;

	//チャンク

	while(1)
	{
		if(mIO_read32(io, &id)
			|| id == MLK_MAKE32_4('E','N','D',' ')
			|| mIO_read32(io, &size))
			return MLKERR_DAMAGED;

		if(id == MLK_MAKE32_4('B','I','M','G'))
			break;

		if(mIO_seekCur(io, size))
			return MLKERR_DAMAGED;
	}

	//合成イメージ情報

	if(mIO_seekCur(io, 2)
		|| mIO_read32(io, &size))
		return MLKERR_DAMAGED;

	p->compsize = size;

	return MLKERR_OK;
}

/* 閉じる */

static void _apd3_close(mLoadImage *pli)
{
	_load_apd3 *p = (_load_apd3 *)pli->handle;

	if(p)
	{
		mIO_close(p->io);
	}

	mLoadImage_closeHandle(pli);
}

/* 画像の取得 */

static mlkerr _apd3_getimage(mLoadImage *pli)
{
	_load_apd3 *p = (_load_apd3 *)pli->handle;
	mZlib *zlib;
	uint8_t **ppbuf;
	int iy,width,pitch;
	mlkerr ret;

	zlib = mZlibDecNew(8192, 15);
	if(!zlib) return MLKERR_ALLOC;

	mZlibSetIO_stdio(zlib, mIO_getFILE(p->io));
	mZlibDecSetSize(zlib, p->compsize);

	//

	ppbuf = pli->imgbuf;
	width = pli->width;
	pitch = width * 3;

	for(iy = pli->height; iy; iy--, ppbuf++)
	{
		ret = mZlibDecRead(zlib, *ppbuf, pitch);
		if(ret) return ret;

		//RGB -> RGBA

		mImageConv_rgb8_to_rgba8_extend(*ppbuf, width);
	}

	mZlibFree(zlib);
	
	return MLKERR_OK;
}


//===============================
// サムネイルフォーマット判定
//==============================


/* APD */

mlkbool loadThumbCheck_apd(mLoadImageType *p,uint8_t *buf,int size)
{
	if(size < 7
		|| strncmp((char *)buf, "AZPDATA", 7)
		|| buf[7] > 3)
	{
		return FALSE;
	}
	else
	{
		p->format_tag = MLK_MAKE32_4('A','P','D',' ');

		switch(buf[7])
		{
			//ver4
			case 3:
				p->open = _apd4_open;
				p->getimage = _apd4_getimage;
				p->close = _apd4_close;
				break;
			//ver3
			case 2:
				p->open = _apd3_open;
				p->getimage = _apd3_getimage;
				p->close = _apd3_close;
				break;
			default:
				return FALSE;
		}
		
		return TRUE;
	}
}

