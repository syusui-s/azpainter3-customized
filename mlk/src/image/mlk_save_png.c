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
 * PNG 保存
 *****************************************/

#include <stdio.h>
#include <string.h>
#include <png.h>

#include "mlk.h"
#include "mlk_saveimage.h"
#include "mlk_util.h"


//---------------

typedef struct
{
	png_struct *png;
	png_info *pnginfo;

	FILE *fp;
	uint8_t *rowbuf,
		*palbuf;
}pngdata;

//---------------



/** PNG エラー関数 */

static void _png_error_func(png_struct *png,png_const_charp mes)
{
	longjmp(png_jmpbuf(png), 1);
}

/** PNG 警告関数 */

static void _png_warn_func(png_struct *png,png_const_charp mes)
{

}


//========================


/** PNG 初期化 */

static int _init_png(pngdata *p,mSaveImage *si)
{
	png_struct *png;

	//png_struct

	png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		0, _png_error_func, _png_warn_func);

	if(!png) return MLKERR_ALLOC;

	p->png = png;

	//png_info

	p->pnginfo = png_create_info_struct(png);
	if(!p->pnginfo) return MLKERR_ALLOC;

	//

	if(setjmp(png_jmpbuf(png)))
		return MLKERR_LONGJMP;

	//開く

	p->fp = (FILE *)mSaveImage_openFile(si);
	if(!p->fp) return MLKERR_OPEN;

	png_init_io(png, p->fp);

	return 0;
}

/** IHDR セット */

static void _set_IHDR(pngdata *p,mSaveImage *si)
{
	int type,bits,samples;

	bits = si->bits_per_sample;
	samples = si->samples_per_pixel;

	//カラータイプ

	switch(si->coltype)
	{
		//GRAY
		case MSAVEIMAGE_COLTYPE_GRAY:
			type = (samples == 2)? PNG_COLOR_TYPE_GRAY_ALPHA: PNG_COLOR_TYPE_GRAY;
			break;
		//PALETTE
		case MSAVEIMAGE_COLTYPE_PALETTE:
			type = PNG_COLOR_TYPE_PALETTE;
			break;
		//RGB
		case MSAVEIMAGE_COLTYPE_RGB:
			type = (samples == 4)? PNG_COLOR_TYPE_RGB_ALPHA: PNG_COLOR_TYPE_RGB;
			break;
		default:
			return;
	}

	png_set_IHDR(p->png, p->pnginfo,
		si->width, si->height, bits, type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
}

/** 透過色設定 */

static void _set_transparent(pngdata *p,mSaveImage *si,mSaveOptPNG *opt)
{
	png_color_16 pngcol;
	uint8_t *buf;
	int ind;

	switch(si->coltype)
	{
		//GRAY (ビットに合わせる)
		case MSAVEIMAGE_COLTYPE_GRAY:
			pngcol.gray = opt->transparent;
			png_set_tRNS(p->png, p->pnginfo, NULL, 0, &pngcol);
			break;

		//RGB
		case MSAVEIMAGE_COLTYPE_RGB:
			pngcol.red = opt->transR;
			pngcol.green = opt->transG;
			pngcol.blue = opt->transB;
			
			png_set_tRNS(p->png, p->pnginfo, NULL, 0, &pngcol);
			break;

		//パレット
		/* 各パレットに対応するアルファ値の配列。
		 * 透過色のパレットまで 255。透過色は 0 とし、以降は省略。 */
		case MSAVEIMAGE_COLTYPE_PALETTE:
			ind = opt->transparent;

			if(ind >= si->palette_num) return;
			
			buf = (uint8_t *)mMalloc(ind + 1);
			if(!buf) return;

			memset(buf, 255, ind);
			buf[ind] = 0;

			png_set_tRNS(p->png, p->pnginfo, buf, ind + 1, 0);

			mFree(buf);
			break;
	}
}

/** オプション設定 */

static void _set_option(pngdata *p,mSaveImage *si,mSaveOptPNG *opt)
{
	//圧縮レベル

	if(opt->mask & MSAVEOPT_PNG_MASK_COMP_LEVEL)
		png_set_compression_level(p->png, opt->comp_level);

	//透過色

	if(opt->mask & MSAVEOPT_PNG_MASK_TRANSPARENT)
		_set_transparent(p, si, opt);
}

/** メイン処理 */

static int _main_proc(pngdata *p,mSaveImage *si,mSaveOptPNG *opt)
{
	int ret,i,height,pitch,resoh,resov,swap_byte,swap_cnt,
		last_prog,new_prog;
	uint8_t *rowbuf;
	mFuncSaveImageProgress progress;

	//PNG 初期化

	ret = _init_png(p, si);
	if(ret) return ret;

	//IHDR セット

	_set_IHDR(p, si);

	//パレット

	if(si->coltype == MSAVEIMAGE_COLTYPE_PALETTE)
	{
		p->palbuf = mSaveImage_createPaletteRGB(si);
		if(!p->palbuf) return MLKERR_ALLOC;

		png_set_PLTE(p->png, p->pnginfo, (png_colorp)p->palbuf, si->palette_num);
	}

	//解像度

	if(mSaveImage_getDPM(si, &resoh, &resov))
		png_set_pHYs(p->png, p->pnginfo, resoh, resov, PNG_RESOLUTION_METER);

	//オプション

	if(opt)
		_set_option(p, si, opt);

	//情報書き込み

	png_write_info(p->png, p->pnginfo);

	//-----------

	//Y1行バッファ確保

	pitch = png_get_rowbytes(p->png, p->pnginfo);

	rowbuf = p->rowbuf = (uint8_t *)mMalloc(pitch);
	if(!rowbuf) return MLKERR_ALLOC;

	//16bit バイト交換

	swap_byte = FALSE;

	if(si->bits_per_sample == 16 && mIsByteOrderLE())
	{
		swap_byte = TRUE;
		swap_cnt = si->width * si->samples_per_pixel;
	}

	//イメージ書き込み

	png_set_filter(p->png, 0, PNG_NO_FILTERS);

	progress = si->progress;
	height = si->height;
	last_prog = 0;

	for(i = 0; i < height; i++)
	{
		//取得
		
		ret = (si->setrow)(si, i, rowbuf, pitch);
		if(ret) return ret;

		//バイト交換

		if(swap_byte)
			mSwapByte_16bit(rowbuf, swap_cnt);

		//書き込み

		png_write_row(p->png, (png_bytep)rowbuf);

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

	png_write_end(p->png, NULL);

	return MLKERR_OK;
}


//========================


/**@ PNG 保存 */

mlkerr mSaveImagePNG(mSaveImage *si,void *opt)
{
	pngdata dat;
	int ret;

	mMemset0(&dat, sizeof(pngdata));

	ret = _main_proc(&dat, si, (mSaveOptPNG *)opt);

	//終了

	if(dat.png)
		png_destroy_write_struct(&dat.png, (dat.pnginfo)? &dat.pnginfo: NULL);

	mSaveImage_closeFile(si, dat.fp);

	mFree(dat.rowbuf);
	mFree(dat.palbuf);

	return ret;
}
