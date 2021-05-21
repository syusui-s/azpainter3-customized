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
 * GIF 読み込み
 *****************************************/

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_gifdec.h"



/** イメージ部分まで読み込み */

static int _read_info(mGIFDec *gif,mLoadImage *pli)
{
	mSize size;
	int ret,n;

	//開く

	switch(pli->open.type)
	{
		case MLOADIMAGE_OPEN_FILENAME:
			ret = mGIFDec_openFile(gif, pli->open.filename);
			break;
		case MLOADIMAGE_OPEN_FP:
			ret = mGIFDec_openFILEptr(gif, pli->open.fp);
			break;
		case MLOADIMAGE_OPEN_BUF:
			ret = mGIFDec_openBuf(gif, pli->open.buf, pli->open.size);
			break;
		default:
			ret = 0;
			break;
	}

	if(!ret) return MLKERR_OPEN;

	//ヘッダ読み込み

	ret = mGIFDec_readHeader(gif);
	if(ret) return ret;

	//画像サイズ

	mGIFDec_getImageSize(gif, &size);

	//最初のイメージ位置へ

	ret = mGIFDec_moveNextImage(gif);

	if(ret == -1)
		return MLKERR_INVALID_VALUE;
	else if(ret)
		return ret;

	//---- mLoadImage にセット

	pli->width  = size.w;
	pli->height = size.h;
	pli->bits_per_sample = 8;

	//カラータイプ

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		pli->coltype = MLOADIMAGE_COLTYPE_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		pli->coltype = MLOADIMAGE_COLTYPE_RGBA;
	else
		pli->coltype = MLOADIMAGE_COLTYPE_PALETTE;

	//透過色

	n = mGIFDec_getTransRGB(gif);
	if(n != -1)
	{
		pli->trns.flag = 1;
		pli->trns.r = MLK_RGB_R(n);
		pli->trns.g = MLK_RGB_G(n);
		pli->trns.b = MLK_RGB_B(n);
	}

	return MLKERR_OK;
}


//=================================
// main
//=================================


/* 終了 */

static void _gif_close(mLoadImage *pli)
{
	if(pli->handle)
	{
		mGIFDec_close((mGIFDec *)pli->handle);
		pli->handle = NULL;
	}

	//handle 以外を解放
	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _gif_open(mLoadImage *pli)
{
	mGIFDec *p;

	//mGIFDec

	p = mGIFDec_new();
	if(!p) return MLKERR_ALLOC;

	pli->handle = (void *)p;

	//情報読み込み

	return _read_info(p, pli);
}

/* イメージ読み込み */

static mlkerr _gif_getimage(mLoadImage *pli)
{
	mGIFDec *gif = (mGIFDec *)pli->handle;
	uint8_t **ppbuf,*palbuf;
	int format,num,ret,prog,prog_cur,height,i;
	mlkbool to_a0;

	//イメージ読み込み

	ret = mGIFDec_getImage(gif);
	if(ret) return ret;

	//パレットセット

	palbuf = mGIFDec_getPalette(gif, &num);

	ret = mLoadImage_setPalette(pli, palbuf, 256 * 4, num);
	if(ret) return ret;

	//フォーマット

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		format = MGIFDEC_FORMAT_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		format = MGIFDEC_FORMAT_RGBA;
	else
		format = MGIFDEC_FORMAT_RAW;

	//透過色を A=0 にするか

	to_a0 = (format == MGIFDEC_FORMAT_RGBA
		&& (pli->flags & MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA));

	//イメージセット

	ppbuf = pli->imgbuf;
	height = pli->height;
	prog_cur = 0;

	for(i = 0; i < height; i++)
	{
		if(!mGIFDec_getNextLine(gif, *ppbuf, format, to_a0))
			break;

		ppbuf++;

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

	return MLKERR_OK;
}

/**@ GIF 判定と関数セット */

mlkbool mLoadImage_checkGIF(mLoadImageType *p,uint8_t *buf,int size)
{
	if(!buf || (size >= 3 && buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F'))
	{
		p->format_tag = MLK_MAKE32_4('G','I','F',' ');
		p->open = _gif_open;
		p->getimage = _gif_getimage;
		p->close = _gif_close;
		
		return TRUE;
	}

	return FALSE;
}
