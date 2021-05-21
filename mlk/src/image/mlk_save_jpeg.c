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
 * JPEG 保存
 *****************************************/

#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "mlk.h"
#include "mlk_saveimage.h"


//--------------------

/* エラー用 */
typedef struct
{
	struct jpeg_error_mgr jerr;
	jmp_buf jmpbuf;
}_jpeg_myerr;


typedef struct
{
	struct jpeg_compress_struct jpg;
	_jpeg_myerr jpgerr;

	FILE *fp;
	uint8_t *rowbuf;
}jpegdata;

//--------------------



/** JPEG エラー終了時 */

static void _jpeg_myerror_exit(j_common_ptr p)
{
	_jpeg_myerr *err = (_jpeg_myerr *)p->err;

	longjmp(err->jmpbuf, 1);
}

/** JPEG メッセージ表示 */

static void _jpeg_output_message(j_common_ptr p)
{

}


//================


/** 初期化 */

static int _init(jpegdata *p,mSaveImage *si)
{
	j_compress_ptr jpg;
	mlkbool grayscale;
	int h,v;

	jpg = &p->jpg;

	//エラー設定
	/* デフォルトではエラー時にプロセスが終了するので、longjmp を行うようにする */

	jpg->err = jpeg_std_error(&p->jpgerr.jerr);

	p->jpgerr.jerr.error_exit = _jpeg_myerror_exit;
	p->jpgerr.jerr.output_message = _jpeg_output_message;

	//エラー時

	if(setjmp(p->jpgerr.jmpbuf))
		return MLKERR_LONGJMP;

	//jpeg 構造体初期化

	jpeg_create_compress(jpg);

	//開く

	p->fp = (FILE *)mSaveImage_openFile(si);
	if(!p->fp) return MLKERR_OPEN;

	jpeg_stdio_dest(jpg, p->fp);

	//----------

	//基本情報セット

	grayscale = (si->coltype == MSAVEIMAGE_COLTYPE_GRAY);

	jpg->image_width = si->width;
	jpg->image_height = si->height;
	jpg->input_components = (grayscale)? 1: 3;
	jpg->in_color_space = (grayscale)? JCS_GRAYSCALE: JCS_RGB;

	jpeg_set_defaults(jpg);

	//解像度 (JFIF) - dpi

	if(mSaveImage_getDPI(si, &h, &v))
	{
		jpg->density_unit = 1;
		jpg->X_density = h;
		jpg->Y_density = v;
	}

	//ハフマンテーブル最適化

	jpg->optimize_coding = TRUE;

	return MLKERR_OK;
}

/** オプション設定 */

static void _set_option(j_compress_ptr jpg,mSaveOptJPEG *opt)
{
	//品質

	if(opt->mask & MSAVEOPT_JPEG_MASK_QUALITY)
		jpeg_set_quality(jpg, opt->quality, TRUE);

	//プログレッシブ

	if((opt->mask & MSAVEOPT_JPEG_MASK_PROGRESSION)
		&& opt->progression)
		jpeg_simple_progression(jpg);

	//サンプリング比

	if(opt->mask & MSAVEOPT_JPEG_MASK_SAMPLING_FACTOR)
	{
		int h,v;

		h = v = 1; //default = 444

		switch(opt->sampling_factor)
		{
			//横に 1/2
			case 422:
				h = 2;
				break;
			//横・縦に 1/2
			case 420:
				h = v = 2;
				break;
			//横に 1/4
			case 411:
				h = 4;
				break;
		}

		jpg->comp_info[0].h_samp_factor = h;
		jpg->comp_info[0].v_samp_factor = v;
	}
}

/** メイン処理 */

static int _main_proc(jpegdata *p,mSaveImage *si,mSaveOptJPEG *opt)
{
	j_compress_ptr jpg;
	uint8_t *rowbuf;
	mFuncSaveImageProgress progress;
	int ret,pitch,i,height,last_prog,new_prog;

	//初期化

	ret = _init(p, si);
	if(ret) return ret;

	jpg = &p->jpg;

	//オプション

	if(opt)
		_set_option(jpg, opt);

	//Y1行バッファ確保

	pitch = si->width;
	if(si->coltype == MSAVEIMAGE_COLTYPE_RGB) pitch *= 3;

	rowbuf = p->rowbuf = (uint8_t *)mMalloc(pitch);
	if(!rowbuf) return MLKERR_ALLOC;

	//イメージ開始

	jpeg_start_compress(jpg, TRUE);

	//イメージ書き込み

	progress = si->progress;
	height = si->height;
	last_prog = 0;

	for(i = 0; i < height; i++)
	{
		//取得
		
		ret = (si->setrow)(si, i, rowbuf, pitch);
		if(ret) return ret;

		//書き込み

		jpeg_write_scanlines(jpg, (JSAMPARRAY)&rowbuf, 1);

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

	//イメージ終了

	jpeg_finish_compress(jpg);

	return MLKERR_OK;
}


//========================


/**@ JPEG 保存 */

mlkerr mSaveImageJPEG(mSaveImage *si,void *opt)
{
	jpegdata dat;
	int ret;

	mMemset0(&dat, sizeof(jpegdata));

	ret = _main_proc(&dat, si, (mSaveOptJPEG *)opt);

	//終了

	jpeg_destroy_compress(&dat.jpg);

	mSaveImage_closeFile(si, dat.fp);

	mFree(dat.rowbuf);

	return ret;
}
