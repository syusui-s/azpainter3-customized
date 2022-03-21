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

/************************************
 * AppDraw
 * 通常画像の保存
 ************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_saveimage.h"

#include "def_draw.h"
#include "def_config.h"
#include "def_saveopt.h"

#include "imagecanvas.h"
#include "fileformat.h"


//---------------

//TIFF:圧縮タイプ
static const uint16_t g_tiff_comptype[] = {
	MSAVEOPT_TIFF_COMPRESSION_NONE,
	MSAVEOPT_TIFF_COMPRESSION_LZW, MSAVEOPT_TIFF_COMPRESSION_PACKBITS,
	MSAVEOPT_TIFF_COMPRESSION_DEFLATE
};

//---------------


/* Y1行を送る */

static mlkerr _save_setrow(mSaveImage *p,int y,uint8_t *buf,int line_bytes)
{
	uint8_t **ppbuf;

	ppbuf = (uint8_t **)p->param1;

	memcpy(buf, ppbuf[y], line_bytes);

	return MLKERR_OK;
}

/* プログレス */

static void _save_progress(mSaveImage *p,int percent)
{
	mPopupProgressThreadSetPos((mPopupProgress *)p->param2, percent);
}

/* PNG 透過色をセット */

static void _set_png_transparent(mSaveImageOpt *p,int dstbits,int samples)
{
	int x,y;
	uint8_t *buf;
	uint16_t *p16;

	//アルファチャンネル付きの場合は無効
	if(samples == 4) return;

	x = APPCONF->save.pt_tpcol.x;
	y = APPCONF->save.pt_tpcol.y;

	if(x != -1)
	{
		//上書き保存時は、リサイズされた場合があるため、位置をチェック

		if(x >= APPDRAW->imgw || y >= APPDRAW->imgh)
			return;

		//

		buf = APPDRAW->imgcanvas->ppbuf[y];

		if(dstbits == 8)
		{
			buf += x * samples;

			p->png.transR = buf[0];
			p->png.transG = buf[1];
			p->png.transB = buf[2];
		}
		else
		{
			p16 = (uint16_t *)buf + x * samples;

			p->png.transR = p16[0];
			p->png.transG = p16[1];
			p->png.transB = p16[2];
		}
	
		p->png.mask |= MSAVEOPT_PNG_MASK_TRANSPARENT;
	}
}

/* GIF セット */

static mlkerr _set_gif_info(AppDraw *p,mSaveImage *si,mSaveImageOpt *opt)
{
	int x,y;

	si->coltype = MSAVEIMAGE_COLTYPE_PALETTE;
	si->samples_per_pixel = 1;

	//パレット作成
	
	if(!mSaveImage_createPalette_fromRGB8_array(
		p->imgcanvas->ppbuf, p->imgw, p->imgh,
		&si->palette_buf, &si->palette_num))
	{
		if(!si->palette_buf)
			return MLKERR_ALLOC;
		else
		{
			//256色を超えた

			mFree(si->palette_buf);
			return -100;
		}
	}

	//パレットイメージに変換

	mSaveImage_convertImage_RGB8array_to_palette(
		p->imgcanvas->ppbuf, p->imgw, p->imgh,
		si->palette_buf, si->palette_num);

	//透過色

	x = APPCONF->save.pt_tpcol.x;
	y = APPCONF->save.pt_tpcol.y;

	if(x != -1
		&& (x < APPDRAW->imgw && y < APPDRAW->imgh))
	{
		opt->gif.mask = MSAVEOPT_GIF_MASK_TRANSPARENT;

		opt->gif.transparent = *(p->imgcanvas->ppbuf[y] + x);
	}

	return MLKERR_OK;
}

/** 画像ファイルに保存
 *
 * return: [-100] GIF で 257 色以上 */

mlkerr drawFile_save_imageFile(AppDraw *p,const char *filename,
	uint32_t format,int dstbits,int falpha,mPopupProgress *prog)
{
	mSaveImage si;
	mSaveImageOpt opt;
	mFuncSaveImage func;
	mlkerr ret;
	uint32_t val;
	uint16_t jpegsamp[] = {444,422,420};

	//情報

	mSaveImage_init(&si);

	si.open.type = MSAVEIMAGE_OPEN_FILENAME;
	si.open.filename = filename;
	si.coltype = MSAVEIMAGE_COLTYPE_RGB;
	si.width = p->imgw;
	si.height = p->imgh;
	si.bits_per_sample = dstbits;
	si.samples_per_pixel = (falpha)? 4: 3;
	si.reso_unit = MSAVEIMAGE_RESOUNIT_DPI;
	si.reso_horz = p->imgdpi;
	si.reso_vert = p->imgdpi;
	si.progress = _save_progress;
	si.setrow = _save_setrow;
	si.param1 = p->imgcanvas->ppbuf;
	si.param2 = prog;

	//保存関数,設定

	opt.mask = 0;

	if(format & FILEFORMAT_PNG)
	{
		//PNG
		
		func = mSaveImagePNG;

		opt.png.mask = MSAVEOPT_PNG_MASK_COMP_LEVEL;
		opt.png.comp_level = SAVEOPT_PNG_GET_LEVEL(APPCONF->save.png);

		_set_png_transparent(&opt, dstbits, si.samples_per_pixel);
	}
	else if(format & FILEFORMAT_JPEG)
	{
		//JPEG
		
		func = mSaveImageJPEG;

		val = APPCONF->save.jpeg;

		opt.jpg.mask = MSAVEOPT_JPEG_MASK_QUALITY | MSAVEOPT_JPEG_MASK_SAMPLING_FACTOR
			| MSAVEOPT_JPEG_MASK_PROGRESSION;

		opt.jpg.quality = SAVEOPT_JPEG_GET_QUALITY(val);
		opt.jpg.sampling_factor = jpegsamp[SAVEOPT_JPEG_GET_SAMPLING(val)];
		opt.jpg.progression = val & SAVEOPT_JPEG_F_PROGRESSIVE;
	}
	else if(format & FILEFORMAT_BMP)
	{
		//BMP
		
		func = mSaveImageBMP;
	}
	else if(format & FILEFORMAT_GIF)
	{
		//GIF
		
		func = mSaveImageGIF;

		ret = _set_gif_info(p, &si, &opt);
		if(ret) return ret;
	}
	else if(format & FILEFORMAT_TIFF)
	{
		//TIFF
		
		func = mSaveImageTIFF;

		opt.tiff.mask = MSAVEOPT_TIFF_MASK_COMPRESSION;
		opt.tiff.compression = g_tiff_comptype[SAVEOPT_TIFF_GET_COMPTYPE(APPCONF->save.tiff)];
	}
	else if(format & FILEFORMAT_WEBP)
	{
		//WEBP
		
		func = mSaveImageWEBP;

		val = APPCONF->save.webp;

		opt.webp.mask = MSAVEOPT_WEBP_MASK_LOSSY
			| MSAVEOPT_WEBP_MASK_LEVEL | MSAVEOPT_WEBP_MASK_QUALITY;

		opt.webp.lossy = val & SAVEOPT_WEBP_F_IRREVERSIBLE;
		opt.webp.level = SAVEOPT_WEBP_GET_LEVEL(val);
		opt.webp.quality = SAVEOPT_WEBP_GET_QUALITY(val);
	}
	else
		return MLKERR_UNSUPPORTED;
	
	//保存

	mPopupProgressThreadSetMax(prog, 100);
	mPopupProgressThreadSetPos(prog, 0);

	ret = (func)(&si, &opt);

	mFree(si.palette_buf);

	return ret;
}

