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
 * PSD 読み込み (mLoadImage)
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_imageconv.h"
#include "mlk_psd.h"


//--------------------

typedef struct
{
	mPSDLoad *psd;
	uint8_t *rowbuf;
	int bits,
		colmode,
		chnum;
}psdload;

//--------------------



/** 開く処理 */

static mlkerr _proc_open(psdload *p,mLoadImage *pli)
{
	mPSDLoad *psd;
	mPSDHeader hd;
	int ret,h,v;

	psd = mPSDLoad_new();
	if(!psd) return MLKERR_ALLOC;

	p->psd = psd;

	//開く

	switch(pli->open.type)
	{
		case MLOADIMAGE_OPEN_FILENAME:
			ret = mPSDLoad_openFile(psd, pli->open.filename);
			break;
		case MLOADIMAGE_OPEN_FP:
			ret = mPSDLoad_openFILEptr(psd, pli->open.fp);
			break;
		default:
			ret = MLKERR_OPEN;
			break;
	}

	if(ret) return ret;
	
	//ヘッダ読み込み

	ret = mPSDLoad_readHeader(psd, &hd);
	if(ret) return ret;

	p->bits = hd.bits;
	p->colmode = hd.colmode;
	p->chnum = hd.img_channels;

	//情報セット

	pli->width = hd.width;
	pli->height = hd.height;
	pli->bits_per_sample = (hd.bits == 16
		&& (pli->flags & MLOADIMAGE_FLAGS_ALLOW_16BIT))? 16: 8;

	//カラータイプ

	if(hd.colmode == MPSD_COLMODE_CMYK)
	{
		//CMYK
		
		if(!(pli->flags & MLOADIMAGE_FLAGS_ALLOW_CMYK))
			return MLKERR_UNSUPPORTED;

		pli->coltype = MLOADIMAGE_COLTYPE_CMYK;
		pli->convert_type = MLOADIMAGE_CONVERT_TYPE_RAW;
		p->chnum = 4; //アルファ値は除く
	}
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		//変換:RGB
		pli->coltype = MLOADIMAGE_COLTYPE_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		//変換:RGBA
		pli->coltype = MLOADIMAGE_COLTYPE_RGBA;
	else if(hd.colmode == MPSD_COLMODE_RGB)
	{
		//RGB/A

		pli->coltype = (hd.img_channels == 3)?
			MLOADIMAGE_COLTYPE_RGB: MLOADIMAGE_COLTYPE_RGBA;
	}
	else
	{
		//GRAY/MONO

		pli->coltype = (hd.img_channels == 2)?
			MLOADIMAGE_COLTYPE_GRAY_A: MLOADIMAGE_COLTYPE_GRAY;
	}

	//リソース

	ret = mPSDLoad_readResource(psd);
	if(ret) return ret;

	if(mPSDLoad_res_getResoDPI(psd, &h, &v))
	{
		pli->reso_unit = MLOADIMAGE_RESOUNIT_DPI;
		pli->reso_horz = h;
		pli->reso_vert = v;
	}

	return MLKERR_OK;
}


//==========================
// main
//==========================


/* 終了 */

static void _psd_close(mLoadImage *pli)
{
	psdload *p = (psdload *)pli->handle;

	if(p)
	{
		mPSDLoad_close(p->psd);

		mFree(p->rowbuf);
	}

	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _psd_open(mLoadImage *pli)
{
	int ret;

	ret = mLoadImage_createHandle(pli, sizeof(psdload), -1);
	if(ret) return ret;

	return _proc_open((psdload *)pli->handle, pli);
}

/* イメージ読み込み */

static mlkerr _psd_getimage(mLoadImage *pli)
{
	psdload *p = (psdload *)pli->handle;
	mPSDLoad *psd;
	uint8_t **ppbuf;
	mFuncImageConv func;
	mImageConv conv;
	int ret,ich,iy,chnum,prog,prog_cur,prog_max,prog_cnt;

	psd = p->psd;
	chnum = p->chnum;

	//バッファ確保

	p->rowbuf = (uint8_t *)mMalloc(mPSDLoad_getImageChRowSize(psd));
	if(!p->rowbuf) return MLKERR_ALLOC;

	//変換情報

	mLoadImage_setImageConv(pli, &conv);
	
	conv.srcbuf = p->rowbuf;
	conv.srcbits = p->bits;

	if(p->colmode == MPSD_COLMODE_RGB)
	{
		//RGB

		func = (p->bits == 16)? mImageConv_sepch_rgb_rgba_16: mImageConv_sepch_rgb_rgba_8;

		if(chnum == 4) conv.flags |= MIMAGECONV_FLAGS_SRC_ALPHA;
	}
	else if(p->colmode == MPSD_COLMODE_GRAYSCALE)
	{
		//GRAY

		if(p->bits == 16)
			func = (chnum == 2)? mImageConv_sepch_gray_a_16: mImageConv_gray16;
		else
			func = (chnum == 2)? mImageConv_sepch_gray_a_8: mImageConv_gray_1_2_4_8;
	}
	else if(p->colmode == MPSD_COLMODE_MONO)
	{
		//MONO
		func = mImageConv_gray_1_2_4_8;
		conv.flags |= MIMAGECONV_FLAGS_REVERSE;
	}
	else
	{
		//CMYK

		func = (p->bits == 16)? mImageConv_sepch_cmyk16: mImageConv_sepch_cmyk8;
		conv.flags |= MIMAGECONV_FLAGS_REVERSE;
		chnum = 4;
	}

	//一枚絵イメージ開始

	ret = mPSDLoad_startImage(psd);
	if(ret) return ret;

	//各チャンネルごとに読み込み

	prog_cur = 0;
	prog_cnt = 1;
	prog_max = chnum * pli->height;

	for(ich = 0; ich < chnum; ich++)
	{
		ppbuf = pli->imgbuf;
		conv.chno = ich;

		for(iy = pli->height; iy > 0; iy--)
		{
			ret = mPSDLoad_readImageRowCh(psd, p->rowbuf);
			if(ret) return ret;

			conv.dstbuf = *(ppbuf++);
			(func)(&conv);

			//進捗

			if(pli->progress)
			{
				prog = prog_cnt * 100 / prog_max;

				if(prog != prog_cur)
				{
					prog_cur = prog;
					(pli->progress)(pli, prog);
				}

				prog_cnt++;
			}
		}
	}

	return MLKERR_OK;
}

/**@ PSD 判定と関数セット */

mlkbool mLoadImage_checkPSD(mLoadImageType *p,uint8_t *buf,int size)
{
	if(!buf
		|| (size >= 4 && strncmp((char *)buf, "8BPS", 4) == 0))
	{
		p->format_tag = MLK_MAKE32_4('P','S','D',' ');
		p->open = _psd_open;
		p->getimage = _psd_getimage;
		p->close = _psd_close;
		
		return TRUE;
	}

	return FALSE;
}

/**@ (PSD) ICC プロファイル取得
 * 
 * @p:psize データサイズが格納される
 * @r:確保されたバッファが返る (NULL で失敗) */

uint8_t *mLoadImagePSD_getICCProfile(mLoadImage *p,uint32_t *psize)
{
	return mPSDLoad_res_getICCProfile( ((psdload *)p->handle)->psd, psize);
}
