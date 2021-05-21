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
 * TIFF 保存
 *****************************************/

#include <stdio.h>
#include <tiffio.h>

#include "mlk.h"
#include "mlk_saveimage.h"


//---------------

typedef struct
{
	TIFF *tiff;
	uint8_t *rowbuf;
	uint16_t *palbuf;  //R,G,B
}tiffdata;

//---------------


/** パレットセット */

static int _set_palette(tiffdata *p,mSaveImage *si)
{
	uint16_t *buf,*pr,*pg,*pb;
	uint8_t *ps;
	int num,i;

	num = 1 << si->bits_per_sample;

	//確保

	buf = p->palbuf = (uint16_t *)mMalloc0(2 * 3 * num);
	if(!buf) return MLKERR_ALLOC;

	pr = buf;
	pg = buf + num;
	pb = buf + num * 2;

	//変換 (RGBX 8bit => RGB 16bit)

	ps = si->palette_buf;

	for(i = si->palette_num; i > 0; i--)
	{
		*(pr++) = ps[0] * 257;
		*(pg++) = ps[1] * 257;
		*(pb++) = ps[2] * 257;

		ps += 4;
	}

	//セット

	TIFFSetField(p->tiff, TIFFTAG_COLORMAP,
		buf, buf + num, buf + num * 2);

	return MLKERR_OK;
}

/** フィールドセット */

static int _set_fields(tiffdata *p,mSaveImage *si,mSaveOptTIFF *opt)
{
	TIFF *tiff = p->tiff;
	int n,resoh,resov,ret;
	uint16_t u16;

	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, si->width);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, si->height);

	TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, si->bits_per_sample);
	TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, si->samples_per_pixel);

	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

	//圧縮

	n = COMPRESSION_LZW;

	if(opt && (opt->mask & MSAVEOPT_TIFF_MASK_COMPRESSION))
		n = opt->compression;

	TIFFSetField(tiff, TIFFTAG_COMPRESSION, n);

	//カラータイプ

	switch(si->coltype)
	{
		//GRAY
		case MSAVEIMAGE_COLTYPE_GRAY:
			n = PHOTOMETRIC_MINISBLACK;
			break;
		//PALETTE
		case MSAVEIMAGE_COLTYPE_PALETTE:
			n = PHOTOMETRIC_PALETTE;

			ret = _set_palette(p, si);
			if(ret) return ret;
			break;
		//RGB
		case MSAVEIMAGE_COLTYPE_RGB:
			n = PHOTOMETRIC_RGB;

			if(si->samples_per_pixel == 4)
			{
				u16 = EXTRASAMPLE_ASSOCALPHA;
				TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &u16);
			}
			break;
		//CMYK
		case MSAVEIMAGE_COLTYPE_CMYK:
			n = PHOTOMETRIC_SEPARATED;

			TIFFSetField(tiff, TIFFTAG_INKSET, INKSET_CMYK);
			break;
		default:
			return MLKERR_INVALID_VALUE;
	}

	TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, n);

	//解像度

	if(mSaveImage_getDPI(si, &resoh, &resov))
		TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	else
	{
		TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
		resoh = resov = 1;
	}

	TIFFSetField(tiff, TIFFTAG_XRESOLUTION, (float)resoh);
	TIFFSetField(tiff, TIFFTAG_YRESOLUTION, (float)resov);

	//ストリップ

	TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tiff, si->height));

	//ICC プロファイル

	if(opt && (opt->mask & MSAVEOPT_TIFF_MASK_ICCPROFILE))
		TIFFSetField(tiff, TIFFTAG_ICCPROFILE, opt->profile_size, opt->profile_buf);

	return MLKERR_OK;
}

/** メイン処理 */

static int _main_proc(tiffdata *p,mSaveImage *si,mSaveOptTIFF *opt)
{
	int ret,i,pitch,height,last_prog,new_prog;
	mFuncSaveImageProgress progress;
	uint8_t *rowbuf;

	//開く

	switch(si->open.type)
	{
		case MSAVEIMAGE_OPEN_FILENAME:
			p->tiff = TIFFOpen(si->open.filename, "w");
			break;
		case MSAVEIMAGE_OPEN_FP:
			p->tiff = TIFFFdOpen(fileno((FILE *)si->open.fp), "", "w");
			break;
	}

	if(!p->tiff) return MLKERR_OPEN;

	//フィールドセット

	ret = _set_fields(p, si, opt);
	if(ret) return ret;

	//ラインバッファ

	pitch = TIFFScanlineSize(p->tiff);

	rowbuf = p->rowbuf = (uint8_t *)mMalloc(pitch);
	if(!rowbuf) return MLKERR_ALLOC;

	//エンコード

	progress = si->progress;
	height = si->height;
	last_prog = 0;

	for(i = 0; i < height; i++)
	{
		//取得

		ret = (si->setrow)(si, i, rowbuf, pitch);
		if(ret) return ret;

		//書き込み

		if(TIFFWriteScanline(p->tiff, rowbuf, i, 0) != 1)
			return MLKERR_ENCODE;

		//経過

		if(progress)
		{
			new_prog = (i + 1) * 100 / height;

			if(new_prog != last_prog)
			{
				(progress)(si, new_prog);
				last_prog = new_prog;
			}
		}
	}

	return MLKERR_OK;
}


//========================


/**@ TIFF 保存 */

mlkerr mSaveImageTIFF(mSaveImage *si,void *opt)
{
	tiffdata dat;
	int ret;

	mMemset0(&dat, sizeof(tiffdata));

	ret = _main_proc(&dat, si, (mSaveOptTIFF *)opt);

	//

	if(dat.tiff)
		TIFFClose(dat.tiff);

	mFree(dat.rowbuf);
	mFree(dat.palbuf);

	return ret;
}

