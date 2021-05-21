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
 * JPEG 読み込み
 *****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strcmp
#include <setjmp.h>

#include <jpeglib.h>

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_imageconv.h"
#include "mlk_stdio.h"
#include "mlk_util.h"


//--------------------

typedef struct
{
	struct jpeg_error_mgr jerr;
	jmp_buf jmpbuf;
	mLoadImage *loadimg;
	int ferr;
}_myjpeg_err;


typedef struct
{
	struct jpeg_decompress_struct jpg;
	_myjpeg_err jpgerr;

	FILE *fp;
	int close_fp;	//終了時に fp を閉じるか
}jpegdata;

//--------------------



//==========================
// エラー関数
//==========================


/** エラー終了時 */

static void _jpeg_error_exit(j_common_ptr ptr)
{
	_myjpeg_err *p = (_myjpeg_err *)ptr->err;
	char msg[JMSG_LENGTH_MAX];

	p->ferr = 1;

	//エラー文字列を取得

	(p->jerr.format_message)(ptr, msg);

	p->loadimg->errmessage = mStrdup(msg);

	//setjmp の位置に飛ぶ
	/* jpeg_finish_decompress() 時に来た場合、
	 * longjmp() が Segmentation fault になるので、回避 */

	if(ptr->global_state != 210) //DSTATE_STOPPING
		longjmp(p->jmpbuf, 1);
}

/** メッセージ表示 */

static void _jpeg_output_message(j_common_ptr p)
{

}


//=================================
// 情報読み込み
//=================================


/** 解像度単位を取得 */

static int _get_resolution_unit(int unit)
{
	if(unit == 0)
		return MLOADIMAGE_RESOUNIT_ASPECT;
	else if(unit == 1)
		return MLOADIMAGE_RESOUNIT_DPI;
	else if(unit == 2)
		return MLOADIMAGE_RESOUNIT_DPCM;
	else
		return MLOADIMAGE_RESOUNIT_NONE;
}

/** EXIF 値取得 */

static uint32_t _get_exif_value(uint8_t **pp,int size,uint8_t bigendian)
{
	uint8_t *p = *pp;

	*pp += size;

	if(bigendian)
	{
		if(size == 2)
			return (p[0] << 8) | p[1];
		else
			return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	}
	else
	{
		if(size == 2)
			return (p[1] << 8) | p[0];
		else
			return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	}
}

/** EXIF から解像度取得 */

static void _get_exif_resolution(jpegdata *p,mLoadImage *pli)
{
	struct jpeg_marker_struct *plist;
	uint8_t *ps,*ps2,*tifftop,endian,flags = 0;
	int fnum,tag,dattype,unit;
	uint32_t pos,horz,vert;

	for(plist = p->jpg.marker_list; plist; plist = plist->next)
	{
		if(plist->marker == 0xe1 && plist->data_length >= 16)
		{
			ps = (uint8_t *)plist->data;
			tifftop = ps + 6;

			if(strcmp((char *)ps, "Exif") != 0) return;

			endian = (ps[6] == 'M');  //0=little-endian, 1=big-endian

			ps += 14;

			//IFD-0th

			fnum = _get_exif_value(&ps, 2, endian);

			for(; fnum > 0 && flags != 7; fnum--)
			{
				tag = _get_exif_value(&ps, 2, endian);
				dattype = _get_exif_value(&ps, 2, endian);
				ps += 4;
				pos = _get_exif_value(&ps, 4, endian);

				if(tag == 0x011a && dattype == 5)
				{
					//解像度水平

					ps2 = tifftop + pos;
					horz = _get_exif_value(&ps2, 4, endian);
					flags |= 1;
				}
				else if(tag == 0x011b && dattype == 5)
				{
					//解像度垂直

					ps2 = tifftop + pos;
					vert = _get_exif_value(&ps2, 4, endian);
					flags |= 2;
				}
				else if(tag == 0x0128 && dattype == 3)
				{
					//解像度単位 (short)
					/* 1:none 2:dpi 3:dpcm */

					unit = pos;
					flags |= 4;
				}
			}

			//終了

			if(flags == 7 && (unit == 2 || unit == 3))
			{
				pli->reso_unit = _get_resolution_unit(unit - 1);
				pli->reso_horz = horz;
				pli->reso_vert = vert;
			}

			break;
		}
	}
}

/** ヘッダ部分読み込み */

static int _read_info(jpegdata *p,mLoadImage *pli)
{
	j_decompress_ptr jpg;
	int colspace,coltype;

	jpg = &p->jpg;

	//エラー設定
	/* デフォルトではエラー時にプロセスが終了するので、longjmp を行うようにする */

	jpg->err = jpeg_std_error(&p->jpgerr.jerr);

	p->jpgerr.jerr.error_exit = _jpeg_error_exit;
	p->jpgerr.jerr.output_message = _jpeg_output_message;
	p->jpgerr.loadimg = pli;

	//エラー時

	if(setjmp(p->jpgerr.jmpbuf))
		return MLKERR_LONGJMP;

	//構造体初期化

	jpeg_create_decompress(jpg);

	//------ 各タイプ別の初期化

	switch(pli->open.type)
	{
		//ファイル
		case MLOADIMAGE_OPEN_FILENAME:
			p->fp = mFILEopen(pli->open.filename, "rb");
			if(!p->fp) return MLKERR_OPEN;

			jpeg_stdio_src(jpg, p->fp);
			p->close_fp = TRUE;
			break;
		//FILE *
		case MLOADIMAGE_OPEN_FP:
			jpeg_stdio_src(jpg, (FILE *)pli->open.fp);
			break;
		//バッファ
		case MLOADIMAGE_OPEN_BUF:
			jpeg_mem_src(jpg, (unsigned char *)pli->open.buf, pli->open.size);
			break;
		default:
			return MLKERR_OPEN;
	}

	//-------- 情報

	//保存するマーカー

	jpeg_save_markers(jpg, 0xe1, 0xffff);  //EXIF
	jpeg_save_markers(jpg, JPEG_APP0 + 2, 0xffff); //ICC profile

	//ヘッダ読み込み

	if(jpeg_read_header(jpg, TRUE) != JPEG_HEADER_OK)
		return MLKERR_FORMAT_HEADER;

	//色空間

	colspace = jpg->jpeg_color_space;

	//CMYK 時

	if(colspace == JCS_CMYK || colspace == JCS_YCCK)
	{
		//対応しない

		if(!(pli->flags & MLOADIMAGE_FLAGS_ALLOW_CMYK))
			return MLKERR_UNSUPPORTED;
	
		//常に生データ

		pli->convert_type = MLOADIMAGE_CONVERT_TYPE_RAW;
	}

	//mLoadImage にセット

	pli->width  = jpg->image_width;
	pli->height = jpg->image_height;
	pli->bits_per_sample = 8;

	//

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		coltype = MLOADIMAGE_COLTYPE_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		coltype = MLOADIMAGE_COLTYPE_RGBA;
	else if(colspace == JCS_GRAYSCALE)
		coltype = MLOADIMAGE_COLTYPE_GRAY;
	else if(colspace == JCS_CMYK || colspace == JCS_YCCK)
		coltype = MLOADIMAGE_COLTYPE_CMYK;
	else
		coltype = MLOADIMAGE_COLTYPE_RGB;

	pli->coltype = coltype;

	//------- 開始

	//出力フォーマット

	if(colspace == JCS_CMYK || colspace == JCS_YCCK)
		jpg->out_color_space = JCS_CMYK;
	else if(coltype == MLOADIMAGE_COLTYPE_GRAY)
		jpg->out_color_space = JCS_GRAYSCALE;
	else
		jpg->out_color_space = JCS_RGB;

	//展開開始

	if(!jpeg_start_decompress(jpg))
		return MLKERR_ALLOC;

	//----- マーカー情報

	//解像度

	if(jpg->saw_JFIF_marker)
	{
		//JFIF
		pli->reso_horz = jpg->X_density;
		pli->reso_vert = jpg->Y_density;
		pli->reso_unit = _get_resolution_unit(jpg->density_unit);
	}
	else
		//EXIF
		_get_exif_resolution(p, pli);

	return MLKERR_OK;
}


//=================================
// main
//=================================


/* 終了 */

static void _jpeg_close(mLoadImage *pli)
{
	jpegdata *p = (jpegdata *)pli->handle;

	if(p)
	{
		jpeg_destroy_decompress(&p->jpg);

		if(p->fp && p->close_fp)
			fclose(p->fp);
	}

	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _jpeg_open(mLoadImage *pli)
{
	int ret;

	//作成

	ret = mLoadImage_createHandle(pli, sizeof(jpegdata), -1);
	if(ret) return ret;

	//情報読み込み

	return _read_info((jpegdata *)pli->handle, pli);
}

/* イメージ読み込み */

static mlkerr _jpeg_getimage(mLoadImage *pli)
{
	jpegdata *p = (jpegdata *)pli->handle;
	uint8_t **ppbuf;
	int i,cnt,to_rgba,height,prog,prog_cur;

	//読み込み

	ppbuf = pli->imgbuf;
	to_rgba = (pli->coltype == MLOADIMAGE_COLTYPE_RGBA);
	height = p->jpg.output_height;
	prog_cur = 0;

	for(i = 0; i < height; i++)
	{
		jpeg_read_scanlines(&p->jpg, (JSAMPARRAY)ppbuf, 1);

		//RGBA の場合、RGB -> RGBA

		if(to_rgba)
			mImageConv_rgb8_to_rgba8_extend(*ppbuf, pli->width);

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
	
	//CMYK の場合、値を反転

	if(pli->coltype == MLOADIMAGE_COLTYPE_CMYK)
	{
		ppbuf = pli->imgbuf;
		cnt = pli->width << 2;
	
		for(i = pli->height; i > 0; i--)
		{
			mReverseVal_8bit(*ppbuf, cnt);

			ppbuf++;
		}
	}

	//終了

	jpeg_finish_decompress(&p->jpg);

	return (p->jpgerr.ferr)? MLKERR_DECODE: MLKERR_OK;
}

/**@ JPEG 判定と関数セット */

mlkbool mLoadImage_checkJPEG(mLoadImageType *p,uint8_t *buf,int size)
{
	if(!buf || (size >= 2 && buf[0] == 0xff && buf[1] == 0xd8))
	{
		p->format_tag = MLK_MAKE32_4('J','P','E','G');
		p->open = _jpeg_open;
		p->getimage = _jpeg_getimage;
		p->close = _jpeg_close;
		
		return TRUE;
	}

	return FALSE;
}

