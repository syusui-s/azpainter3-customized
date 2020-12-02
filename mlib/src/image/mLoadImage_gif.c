/*$
 Copyright (C) 2013-2020 Azel.

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
 * GIF 読み込み関数
 *****************************************/

#include <setjmp.h>

#include "mDef.h"
#include "mLoadImage.h"
#include "mGIFDecode.h"


//--------------------

typedef struct
{
	mLoadImage *param;

	mGIFDecode *gif;
	uint8_t *rowbuf;

	jmp_buf jmpbuf;
}_LOADGIF;


#define RET_OK         0
#define RET_ERR_NOMES  -1

//--------------------


/** エラー関数 */

static void _gif_error_func(mGIFDecode *gif,const char *mes,void *param)
{
	_LOADGIF *p = (_LOADGIF *)param;

	mLoadImage_setMessage(p->param, mes);

	longjmp(p->jmpbuf, 1);
}

/** イメージ部分まで処理 */

static int _read_headers(_LOADGIF *p)
{
	mLoadImage *param = p->param;
	mLoadImageInfo *info = &p->param->info;
	mSize size;
	int ret;

	//GIF デコーダ作成

	p->gif = mGIFDecode_create();
	if(!p->gif) return MLOADIMAGE_ERR_ALLOC;

	mGIFDecode_setErrorFunc(p->gif, _gif_error_func, p);

	//setjmp

	if(setjmp(p->jmpbuf))
		return RET_ERR_NOMES;

	//開く

	switch(param->src.type)
	{
		case MLOADIMAGE_SOURCE_TYPE_PATH:
			ret = mGIFDecode_open_filename(p->gif, param->src.filename);
			break;
		case MLOADIMAGE_SOURCE_TYPE_BUF:
			ret = mGIFDecode_open_buf(p->gif, param->src.buf, param->src.bufsize);
			break;
		case MLOADIMAGE_SOURCE_TYPE_STDIO:
			ret = mGIFDecode_open_stdio(p->gif, param->src.fp);
			break;
	}

	if(!ret) return MLOADIMAGE_ERR_OPENFILE;

	//ヘッダ読み込み

	mGIFDecode_readHeader(p->gif, &size);

	//画像サイズ

	if((param->max_width > 0 && size.w > param->max_width)
		|| (param->max_height > 0 && size.h > param->max_height))
		return MLOADIMAGE_ERR_LARGE_SIZE;

	//最初のイメージ取得

	if(mGIFDecode_nextImage(p->gif) == 0)
		return MLOADIMAGE_ERR_NONE_IMAGE;

	//情報セット

	info->width  = size.w;
	info->height = size.h;
	info->sample_bits = 8;
	info->raw_sample_bits = 8;
	info->raw_bits = 8;
	info->raw_coltype = MLOADIMAGE_COLTYPE_PALETTE;
	info->raw_pitch = size.w;
	info->resolution_unit = MLOADIMAGE_RESOLITION_UNIT_NONE;
	info->transparent_col = mGIFDecode_getTransparentColor(p->gif);

	info->palette_buf = mGIFDecode_getPalette(p->gif, &info->palette_num);
	
	return RET_OK;
}

/** メイン処理 */

static int _main_proc(_LOADGIF *p)
{
	mLoadImage *param = p->param;
	int ret,i,format,pitch;
	mBool trans_to_a0;
	
	//イメージ部分まで処理

	if((ret = _read_headers(p)))
		return ret;

	//Y1行バッファ確保

	pitch = mLoadImage_getPitch(param);

	p->rowbuf = (uint8_t *)mMalloc(pitch, FALSE);
	if(!p->rowbuf) return MLOADIMAGE_ERR_ALLOC;

	//getinfo()

	if(param->getinfo)
	{
		ret = (param->getinfo)(param, &param->info);
		if(ret) return ret;
	}

	//GIF 取得フォーマット

	if(param->format == MLOADIMAGE_FORMAT_RAW)
		format = MGIFDECODE_FORMAT_RAW;
	else
		format = (param->format == MLOADIMAGE_FORMAT_RGB)? MGIFDECODE_FORMAT_RGB: MGIFDECODE_FORMAT_RGBA;

	trans_to_a0 = !(param->flags & MLOADIMAGE_FLAGS_LEAVE_TRANSPARENT);

	//イメージ

	for(i = param->info.height; i > 0; i--)
	{
		//読み込み
		
		mGIFDecode_getNextLine(p->gif, p->rowbuf, format, trans_to_a0);

		//getrow()

		ret = (param->getrow)(param, p->rowbuf, pitch);
		if(ret) return ret;
	}

	return RET_OK;
}


/** GIF 読み込み
 *
 * @ingroup loadimage */

mBool mLoadImageGIF(mLoadImage *param)
{
	_LOADGIF *p;
	int ret;

	//確保

	p = (_LOADGIF *)mMalloc(sizeof(_LOADGIF), TRUE);
	if(!p) return FALSE;

	p->param = param;

	//処理

	ret = _main_proc(p);

	if(ret != RET_OK)
	{
		if(ret == RET_ERR_NOMES)
		{
			if(!param->message)
				mLoadImage_setMessage(param, "error");
		}
		else
			mLoadImage_setMessage_errno(param, ret);
	}

	//解放

	mGIFDecode_close(p->gif);
	mFree(p->rowbuf);
	mFree(p);

	return (ret == RET_OK);
}

