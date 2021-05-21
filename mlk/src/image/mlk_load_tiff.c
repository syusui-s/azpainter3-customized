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
 * TIFF 読み込み
 *****************************************/

#include <string.h>
#include <tiffio.h>

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_io.h"
#include "mlk_file.h"
#include "mlk_util.h"
#include "mlk_imageconv.h"


//---------------

typedef struct
{
	mIO *io;
	mlkfoff filesize;

	TIFF *tiff;
	uint8_t *palbuf,
		*rowbuf,
		*planebuf;

	int coltype,
		width,
		bits,
		palnum,
		planes;	//個別プレーンのイメージの場合、プレーン数。0 で通常
}tiffdata;

enum
{
	COLTYPE_GRAY_BLACK = 0,	//0=黒
	COLTYPE_GRAY_WHITE,		//0=白
	COLTYPE_PALETTE,
	COLTYPE_RGB,
	COLTYPE_RGBA,
	COLTYPE_CMYK
};

#define _PUT_DEBUG  0

//---------------


//=================================
// TIFF 関数
//=================================


/** 読み込み */

static tsize_t _proc_read(thandle_t handle,tdata_t buf,tsize_t size)
{
	return mIO_read(((tiffdata *)handle)->io, buf, size);
}

/** シーク */

static toff_t _proc_seek(thandle_t handle,toff_t off,int seek)
{
	tiffdata *p = (tiffdata *)handle;

	switch(seek)
	{
		//SET
		case 0:
			if(mIO_seekTop(p->io, off)) return -1;
			break;
		//CUR
		case 1:
			if(mIO_seekCur(p->io, off)) return -1;
			break;
		//END
		default:
			return -1;
	}

	return mIO_getPos(p->io);
}

/** 閉じる */

static int _proc_close(thandle_t handle)
{
	return 0;
}

/** サイズ取得 */

static toff_t _proc_size(thandle_t handle)
{
	return ((tiffdata *)handle)->filesize;
}

static int _proc_mapfile(thandle_t handle,tdata_t *p,toff_t *psize)
{
	return 0;
}

static void _proc_unmapfile(thandle_t handle,tdata_t p,toff_t size)
{

}


//=================================
// sub
//=================================


/** 解像度取得 */

static void _get_resolution(TIFF *tiff,mLoadImage *pli)
{
	uint16_t unit;
	float x,y;

	//default = inch
	TIFFGetFieldDefaulted(tiff, TIFFTAG_RESOLUTIONUNIT, &unit);

	if((unit == RESUNIT_INCH || unit == RESUNIT_CENTIMETER)
		&& TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &x)
		&& TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &y))
	{
		pli->reso_unit = (unit == RESUNIT_INCH)?
			MLOADIMAGE_RESOUNIT_DPI: MLOADIMAGE_RESOUNIT_DPCM;

		pli->reso_horz = (int)(x + 0.5);
		pli->reso_vert = (int)(y + 0.5);
	}
}

/** カラータイプごとの判定
 *
 * return: エラーコード */

static int _check_coltype(tiffdata *p,mLoadImage *pli,uint16_t pm)
{
	uint16_t bits,samples,u16,*p16;
	int coltype;

	//1pxのサンプル数 (default = 1)
	TIFFGetFieldDefaulted(p->tiff, TIFFTAG_SAMPLESPERPIXEL, &samples);

	//ビット数 (default = 1)
	TIFFGetFieldDefaulted(p->tiff, TIFFTAG_BITSPERSAMPLE, &bits);

	switch(pm)
	{
		//2値、グレイスケール
		case PHOTOMETRIC_MINISWHITE:
		case PHOTOMETRIC_MINISBLACK:
		case PHOTOMETRIC_MASK:
			if(bits != 1 && bits != 2 && bits != 4 && bits != 8 && bits != 16)
				return MLKERR_UNSUPPORTED;

			coltype = (pm == PHOTOMETRIC_MINISBLACK)? COLTYPE_GRAY_BLACK: COLTYPE_GRAY_WHITE;
			break;

		//パレット
		case PHOTOMETRIC_PALETTE:
			if(bits != 1 && bits != 2 && bits != 4 && bits != 8)
				return MLKERR_UNSUPPORTED;

			coltype = COLTYPE_PALETTE;
			break;

		//RGB
		case PHOTOMETRIC_RGB:
			if((bits != 8 && bits != 16)
				|| (samples != 3 && samples != 4))
				return MLKERR_UNSUPPORTED;

			//samples = 4 で、追加サンプルがアルファ値か

			if(samples == 4)
			{
				if(TIFFGetField(p->tiff, TIFFTAG_EXTRASAMPLES, &u16, &p16) == 0
					|| *p16 != EXTRASAMPLE_ASSOCALPHA)
					return MLKERR_UNSUPPORTED;
			}

			coltype = (samples == 4)? COLTYPE_RGBA: COLTYPE_RGB;
			break;

		//CMYK (8,16 bit)
		default:
			//対応しない
			if(!(pli->flags & MLOADIMAGE_FLAGS_ALLOW_CMYK))
				return MLKERR_UNSUPPORTED;
		
			if((bits != 8 && bits != 16) || samples != 4)
				return MLKERR_UNSUPPORTED;

			TIFFGetFieldDefaulted(p->tiff, TIFFTAG_INKSET, &u16);
			if(u16 != INKSET_CMYK) return MLKERR_UNSUPPORTED;

			coltype = COLTYPE_CMYK;

			//常に生データ
			pli->convert_type = MLOADIMAGE_CONVERT_TYPE_RAW;
			break;
	}

	p->coltype = coltype;
	p->bits = bits;

	//RGB/CMYK、個別のプレーンに分かれている

	if((pm == PHOTOMETRIC_RGB || pm == PHOTOMETRIC_SEPARATED)
		&& TIFFGetField(p->tiff, TIFFTAG_PLANARCONFIG, &u16)
		&& u16 == PLANARCONFIG_SEPARATE)
	{
		p->planes = samples;
	}

	return MLKERR_OK;
}

/** 開いた時の処理 */

static int _proc_open(tiffdata *p,mLoadImage *pli)
{
	TIFF *tiff;
	uint32_t width,height;
	uint16_t pm,u16;
	int ret;

	//ファイルサイズ

	if(pli->open.type == MLOADIMAGE_OPEN_FILENAME)
		mGetFileSize(pli->open.filename, &p->filesize);
	else
		p->filesize = pli->open.size;

	//エラー・警告表示しない

	TIFFSetErrorHandler(NULL);
	TIFFSetWarningHandler(NULL);

	//TIFF 開く

	tiff = p->tiff = TIFFClientOpen("", "rm", (thandle_t)p,
		_proc_read, _proc_read, _proc_seek, _proc_close,
		_proc_size, _proc_mapfile, _proc_unmapfile);

	if(!tiff) return MLKERR_ALLOC;

#if _PUT_DEBUG
	TIFFPrintDirectory(tiff, stdout, 0);
#endif

	//基本情報

	if(!TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width)
		|| !TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height)
		|| !TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &pm)
		|| pm > 5)
		return MLKERR_INVALID_VALUE;

	p->width = width;

	//非対応

	if(TIFFIsTiled(tiff)	//タイルイメージ
		//原点が左上でない
		|| (TIFFGetField(tiff, TIFFTAG_ORIENTATION, &u16) && u16 != ORIENTATION_TOPLEFT))
		return MLKERR_UNSUPPORTED;

	//カラータイプごとの判定

	ret = _check_coltype(p, pli, pm);
	if(ret) return ret;

	//mLoadImage にセット

	pli->width = width;
	pli->height = height;

	pli->bits_per_sample =
		(p->bits == 16 && (pli->flags & MLOADIMAGE_FLAGS_ALLOW_16BIT))? 16: 8;

	_get_resolution(tiff, pli);

	//カラータイプ

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		pli->coltype = MLOADIMAGE_COLTYPE_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		pli->coltype = MLOADIMAGE_COLTYPE_RGBA;
	else if(p->coltype == COLTYPE_GRAY_BLACK || p->coltype == COLTYPE_GRAY_WHITE)
		//2値/グレイスケール
		pli->coltype = MLOADIMAGE_COLTYPE_GRAY;
	else if(p->coltype == COLTYPE_PALETTE)
		//パレット
		pli->coltype = MLOADIMAGE_COLTYPE_PALETTE;
	else if(p->coltype == COLTYPE_CMYK)
		//CMYK
		pli->coltype = MLOADIMAGE_COLTYPE_CMYK;
	else
		//RGB
		pli->coltype = (p->coltype == COLTYPE_RGBA)? MLOADIMAGE_COLTYPE_RGBA: MLOADIMAGE_COLTYPE_RGB;

	return MLKERR_OK;
}

/** パレット読み込み */

static int _read_palette(tiffdata *p,mLoadImage *pli)
{
	uint16_t *pr,*pg,*pb;
	uint8_t *ppal;
	int num,i;

	//カラーマップ取得

	if(TIFFGetField(p->tiff, TIFFTAG_COLORMAP, &pr, &pg, &pb) == 0)
		return MLKERR_INVALID_VALUE;

	num = 1 << p->bits;

	//バッファ確保

	ppal = p->palbuf = (uint8_t *)mMalloc(num << 2);
	if(!ppal) return MLKERR_ALLOC;

	p->palnum = num;

	//変換

	for(i = num; i > 0; i--, ppal += 4)
	{
		ppal[0] = *pr >> 8;
		ppal[1] = *pg >> 8;
		ppal[2] = *pb >> 8;
		ppal[3] = 255;

		pr++; pg++; pb++;
	}

	//セット

	return mLoadImage_setPalette(pli, p->palbuf, num * 4, num);
}

/** イメージ用バッファ確保 */

static int _alloc_buf(tiffdata *p)
{
	p->rowbuf = (uint8_t *)mMalloc(TIFFRasterScanlineSize(p->tiff));
	if(!p->rowbuf) return MLKERR_ALLOC;

	//プレーンバッファ

	if(p->planes)
	{
		p->planebuf = (uint8_t *)mMalloc(TIFFScanlineSize(p->tiff));
		if(!p->planebuf) return MLKERR_ALLOC;
	}

	return MLKERR_OK;
}

/** 変換情報取得 */

static void _get_convert_info(tiffdata *p,mLoadImage *pli,mImageConv *conv,mFuncImageConv *func)
{
	mLoadImage_setImageConv(pli, conv);

	conv->srcbuf = p->rowbuf;
	conv->srcbits = p->bits;

	switch(p->coltype)
	{
		//GRAY 1,2,4,8,16bit
		case COLTYPE_GRAY_BLACK:
		case COLTYPE_GRAY_WHITE:
			if(p->coltype == COLTYPE_GRAY_WHITE)
				conv->flags |= MIMAGECONV_FLAGS_REVERSE;

			*func = (p->bits == 16)? mImageConv_gray16: mImageConv_gray_1_2_4_8;
			break;

		//PALETTE
		case COLTYPE_PALETTE:
			*func = mImageConv_palette_1_2_4_8;
			break;
		//RGB
		case COLTYPE_RGB:
			*func = (p->bits == 16)? mImageConv_rgb_rgba_16: mImageConv_rgb8;
			break;
		//RGBA
		case COLTYPE_RGBA:
			conv->flags |= MIMAGECONV_FLAGS_SRC_ALPHA;
			*func = (p->bits == 16)? mImageConv_rgb_rgba_16: mImageConv_rgba8;
			break;
		//CMYK
		case COLTYPE_CMYK:
			if(p->bits == 16 && pli->bits_per_sample == 8)
				//16bit => 8bit
				*func = mImageConv_cmyk16;
			else
				//8,16bit copy
				*func = NULL;
			break;

		default:
			*func = NULL;
			break;
	}
}

/** プレーンイメージを rowbuf にセット */

static void _set_plane_image(tiffdata *p,int no)
{
	uint8_t *pd8,*ps8;
	uint16_t *pd16,*ps16;
	int i,add;

	add = p->planes;

	if(p->bits == 8)
	{
		//8bit

		pd8 = p->rowbuf + no;
		ps8 = p->planebuf;

		for(i = p->width; i > 0; i--, pd8 += add)
			*pd8 = *(ps8++);
	}
	else
	{
		//16bit

		pd16 = (uint16_t *)p->rowbuf + no;
		ps16 = (uint16_t *)p->planebuf;

		for(i = p->width; i > 0; i--, pd16 += add)
			*pd16 = *(ps16++);
	}
}

/** ラインイメージを取得 */

static int _get_row_image(tiffdata *p,int y)
{
	int i;

	if(!p->planes)
	{
		if(TIFFReadScanline(p->tiff, p->rowbuf, y, 0) != 1)
			return MLKERR_DECODE;
	}
	else
	{
		//個別プレーン

		for(i = 0; i < p->planes; i++)
		{
			if(TIFFReadScanline(p->tiff, p->planebuf, y, i) != 1)
				return MLKERR_DECODE;

			_set_plane_image(p, i);
		}
	}

	return MLKERR_OK;
}


//=================================
// main
//=================================


/* 終了 */

static void _tiff_close(mLoadImage *pli)
{
	tiffdata *p = (tiffdata *)pli->handle;

	if(p)
	{
		if(p->tiff)
			TIFFClose(p->tiff);

		mFree(p->palbuf);
		mFree(p->rowbuf);
		mFree(p->planebuf);

		mIO_close(p->io);
	}

	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _tiff_open(mLoadImage *pli)
{
	int ret;

	ret = mLoadImage_createHandle(pli, sizeof(tiffdata), MIO_ENDIAN_BIG);
	if(ret) return ret;

	//処理

	return _proc_open((tiffdata *)pli->handle, pli);
}

/* イメージ読み込み */

static mlkerr _tiff_getimage(mLoadImage *pli)
{
	tiffdata *p = (tiffdata *)pli->handle;
	uint8_t *rowbuf,**ppbuf;
	mFuncImageConv funcconv;
	mImageConv conv;
	int y,ret,pitch,height,prog,prog_cur;

	//パレット読み込み

	if(p->coltype == COLTYPE_PALETTE)
	{
		ret = _read_palette(p, pli);
		if(ret) return ret;
	}

	//バッファ確保

	ret = _alloc_buf(p);
	if(ret) return ret;

	//変換情報

	_get_convert_info(p, pli, &conv, &funcconv);

	pitch = mLoadImage_getLineBytes(pli);

	//変換

	rowbuf = p->rowbuf;
	ppbuf = pli->imgbuf;
	height = pli->height;
	prog_cur = 0;

	for(y = 0; y < height; y++, ppbuf++)
	{
		ret = _get_row_image(p, y);
		if(ret) return ret;

		if(!funcconv)
			//CMYK 8,16bit raw
			memcpy(*ppbuf, rowbuf, pitch);
		else
		{
			conv.dstbuf = *ppbuf;
			(funcconv)(&conv);
		}

		//進捗

		if(pli->progress)
		{
			prog = (y + 1) * 100 / height;

			if(prog != prog_cur)
			{
				prog_cur = prog;
				(pli->progress)(pli, prog);
			}
		}
	}

	//解放

	mFree(p->rowbuf);
	mFree(p->planebuf);

	p->rowbuf = NULL;
	p->planebuf = NULL;

	return MLKERR_OK;
}

/**@ TIFF 判定と関数セット */

mlkbool mLoadImage_checkTIFF(mLoadImageType *p,uint8_t *buf,int size)
{
	uint16_t sig;

	if(buf)
	{
		if(size < 4) return FALSE;

		//エンディアン

		if(buf[0] == 'M' && buf[1] == 'M')
			sig = mGetBufBE16(buf + 2);
		else if(buf[0] == 'I' && buf[1] == 'I')
			sig = mGetBufLE16(buf + 2);
		else
			return FALSE;

		//42

		if(sig != 42) return FALSE;
	}

	p->format_tag = MLK_MAKE32_4('T','I','F','F');
	p->open = _tiff_open;
	p->getimage = _tiff_getimage;
	p->close = _tiff_close;
		
	return TRUE;
}

/**@ TIFF、ICC プロファイルを取得
 *
 * @r:確保されたバッファのポインタ。NULL でデータがないか、エラー。 */

uint8_t *mLoadImageTIFF_getICCProfile(mLoadImage *pli,uint32_t *psize)
{
	void *buf,*dstbuf;
	uint32_t size;

	if(!TIFFGetField(((tiffdata *)pli->handle)->tiff, TIFFTAG_ICCPROFILE, &size, &buf))
		return NULL;

	dstbuf = mMemdup(buf, size);
	*psize = size;

	return dstbuf;
}
