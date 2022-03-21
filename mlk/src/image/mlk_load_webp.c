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
 * WebP 読み込み
 *****************************************/

#include <string.h>

#include <webp/decode.h>

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_io.h"
#include "mlk_util.h"
#include "mlk_imageconv.h"


//---------------

typedef struct
{
	mIO *io;

	uint8_t *inbuf;		//入力バッファ
	int insize,			//現在の入力サイズ
		have_alpha;		//アルファ値があるか

	uint32_t filesize,	//ファイルサイズ
		total_input;	//総入力サイズ
}webpdata;

#define _INBUFSIZE  (8 * 1024)

//---------------


//=================================
// sub
//=================================


/** 入力バッファに読み込み */

static mlkbool _read_input(webpdata *p)
{
	uint32_t n;

	n = p->filesize - p->total_input;
	if(n > _INBUFSIZE) n = _INBUFSIZE;

	p->insize = mIO_read(p->io, p->inbuf, n);

	p->total_input += p->insize;

	return (p->insize != 0);
}

/** ヘッダ確認 */

static int _read_header(webpdata *p)
{
	uint8_t *buf = p->inbuf;
	uint32_t n;

	//ヘッダ

	if(mIO_readOK(p->io, buf, 12)
		|| strncmp((char *)buf, "RIFF", 4)
		|| strncmp((char *)buf + 8, "WEBP", 4))
		return MLKERR_FORMAT_HEADER;

	//ファイルサイズ

	p->filesize = mGetBufLE32(buf + 4) + 8;

	//残りを読み込み

	n = p->filesize;
	if(n > _INBUFSIZE) n = _INBUFSIZE;

	p->insize = mIO_read(p->io, buf + 12, n - 12);
	p->insize += 12;

	p->total_input = p->insize;

	return MLKERR_OK;
}

/** 開いた時の処理 */

static int _proc_open(webpdata *p,mLoadImage *pli)
{
	WebPBitstreamFeatures bf;
	int ret;

	//入力バッファ確保

	p->inbuf = (uint8_t *)mMalloc(_INBUFSIZE);
	if(!p->inbuf) return MLKERR_ALLOC;

	//ヘッダ確認

	ret = _read_header(p);
	if(ret) return ret;

	//情報取得

	if(WebPGetFeatures(p->inbuf, p->insize, &bf) != VP8_STATUS_OK)
		return MLKERR_FORMAT_HEADER;

	pli->width = bf.width;
	pli->height = bf.height;
	pli->bits_per_sample = 8;

	p->have_alpha = bf.has_alpha;

	//カラータイプ

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		pli->coltype = MLOADIMAGE_COLTYPE_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		pli->coltype = MLOADIMAGE_COLTYPE_RGBA;
	else
		pli->coltype = (bf.has_alpha)? MLOADIMAGE_COLTYPE_RGBA: MLOADIMAGE_COLTYPE_RGB;

	return MLKERR_OK;
}

/** デコード */

static int _decode(webpdata *p,mLoadImage *pli,WebPDecBuffer *decbuf)
{
	WebPIDecoder *dec;
	VP8StatusCode code;
	int ret;

	//出力バッファ

	WebPInitDecBuffer(decbuf);

	decbuf->colorspace = (p->have_alpha)? MODE_RGBA: MODE_RGB;
	decbuf->is_external_memory = 0;

	//デコーダ作成

	dec = WebPINewDecoder(decbuf);
	if(!dec) return MLKERR_ALLOC;

	//デコード

	while(1)
	{
		code = WebPIAppend(dec, p->inbuf, p->insize);

		if(code == VP8_STATUS_SUSPENDED)
		{
			if(!_read_input(p))
			{
				ret = MLKERR_DAMAGED;
				break;
			}
		}
		else
		{
			if(code == VP8_STATUS_OK)
				ret = MLKERR_OK;
			else
				ret = MLKERR_DECODE;

			break;
		}
	}

	//解放

	if(ret) WebPFreeDecBuffer(decbuf);

	WebPIDelete(dec);

	return ret;
}


//=================================
// main
//=================================


/* 終了 */

static void _webp_close(mLoadImage *pli)
{
	webpdata *p = (webpdata *)pli->handle;

	if(p)
	{
		mFree(p->inbuf);

		mIO_close(p->io);
	}

	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _webp_open(mLoadImage *pli)
{
	int ret;

	ret = mLoadImage_createHandle(pli, sizeof(webpdata), MIO_ENDIAN_LITTLE);
	if(ret) return ret;

	//処理

	return _proc_open((webpdata *)pli->handle, pli);
}

/* イメージ読み込み */

static mlkerr _webp_getimage(mLoadImage *pli)
{
	webpdata *p = (webpdata *)pli->handle;
	uint8_t **ppbuf,*ps;
	mFuncImageConv funcconv;
	mImageConv conv;
	WebPDecBuffer decbuf;
	int ret,i,height,prog,prog_cur;

	//デコード

	ret = _decode(p, pli, &decbuf);
	if(ret) return ret;

	//変換パラメータ

	mLoadImage_setImageConv(pli, &conv);

	funcconv = (p->have_alpha)? mImageConv_rgba8: mImageConv_rgb8;

	//変換

	ppbuf = pli->imgbuf;
	height = pli->height;
	ps = decbuf.u.RGBA.rgba;
	prog_cur = 0;

	for(i = 0; i < height; i++)
	{
		conv.srcbuf = ps;
		conv.dstbuf = *ppbuf;

		(funcconv)(&conv);

		//
	
		ppbuf++;
		ps += decbuf.u.RGBA.stride;

		//進捗

		if(pli->progress)
		{
			prog = (i + 1) * 100 / height;

			if(prog != prog_cur)
			{
				prog_cur = prog;
				(pli->progress)(pli, prog);
			}
		}
	}

	//

	WebPFreeDecBuffer(&decbuf);

	return ret;
}

/**@ WEBP 判定と関数セット */

mlkbool mLoadImage_checkWEBP(mLoadImageType *p,uint8_t *buf,int size)
{
	if(!buf || (size >= 12
		&& strncmp((char *)buf, "RIFF", 4) == 0
		&& strncmp((char *)buf + 8, "WEBP", 4) == 0))
	{
		p->format_tag = MLK_MAKE32_4('W','E','B','P');
		p->open = _webp_open;
		p->getimage = _webp_getimage;
		p->close = _webp_close;
		
		return TRUE;
	}

	return FALSE;
}

