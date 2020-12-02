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
 * GIF 保存
 *****************************************/

#include <stdio.h>

#include "mDef.h"
#include "mSaveImage.h"
#include "mGIFEncode.h"


//--------------------

typedef struct
{
	mSaveImage *param;

	mGIFEncode *gifenc;
	uint8_t *rowbuf,
		*palbuf;

	int transparent;
}SAVEGIF;

//--------------------


/** 初期化 */

static int _init(SAVEGIF *p,mSaveImage *param)
{
	mGIFEncodeGlobalInfo global;
	mGIFEncodeImage img;
	int ret;

	//パレット

	p->palbuf = mSaveImage_convertPalette_RGB(param);
	if(!p->palbuf) return MSAVEIMAGE_ERR_ALLOC;

	//GIF エンコーダ作成

	global.width = param->width;
	global.height = param->height;
	global.bkgnd_index = 0;
	global.bits = mGIFEncode_colnumToBits(param->palette_num);
	global.palette_buf = p->palbuf;
	global.palette_num = param->palette_num;

	p->gifenc = mGIFEncode_create(&global);
	if(!p->gifenc) return MSAVEIMAGE_ERR_ALLOC;

	//開く

	switch(param->output.type)
	{
		case MSAVEIMAGE_OUTPUT_TYPE_PATH:
			if(!mGIFEncode_open_filename(p->gifenc, param->output.filename))
				return MSAVEIMAGE_ERR_OPENFILE;
			break;
		case MSAVEIMAGE_OUTPUT_TYPE_STDIO:
			mGIFEncode_open_stdio(p->gifenc, param->output.fp);
			break;
	}

	//ヘッダ

	mGIFEncode_writeHeader(p->gifenc);

	//設定

	p->transparent = -1;

	if((ret = (param->set_option)(param)))
		return ret;

	//画像制御ブロック (透過色)

	if(p->transparent >= 0)
		mGIFEncode_writeGrpCtrl(p->gifenc, p->transparent, 0, 0, FALSE);

	//画像開始

	img.offsetx = img.offsety = 0;
	img.width = param->width;
	img.height = param->height;
	img.palette_buf = NULL;
	img.palette_num = 0;

	mGIFEncode_startImage(p->gifenc, &img);

	//Y1行バッファ確保

	p->rowbuf = (uint8_t *)mMalloc(param->width, FALSE);
	if(!p->rowbuf) return MSAVEIMAGE_ERR_ALLOC;

	return 0;
}

/** メイン処理 */

static int _main_proc(SAVEGIF *p)
{
	mSaveImage *param = p->param;
	int ret,i;

	//初期化

	if((ret = _init(p, param)))
		return ret;

	//画像

	for(i = param->height; i > 0; i--)
	{
		//取得

		ret = (param->send_row)(param, p->rowbuf, param->width);
		if(ret) return ret;

		//書き込み

		mGIFEncode_writeImage(p->gifenc, p->rowbuf, param->width);
	}

	mGIFEncode_endImage(p->gifenc);
	mGIFEncode_writeEnd(p->gifenc);

	return 0;
}


/**

@ingroup saveimage
@{

*/


/** GIF ファイルに保存
 *
 * 入力データは 8bit パレットのみ。 */

mBool mSaveImageGIF(mSaveImage *param)
{
	SAVEGIF *p;
	int ret;

	//確保

	p = (SAVEGIF *)mMalloc(sizeof(SAVEGIF), TRUE);
	if(!p) return FALSE;

	p->param = param;
	param->internal = p;

	//処理

	ret = _main_proc(p);

	if(ret)
	{
		if(ret > 0)
			mSaveImage_setMessage_errno(param, ret);
	}

	//解放

	mGIFEncode_close(p->gifenc);
	
	mFree(p->rowbuf);
	mFree(p->palbuf);
	mFree(p);

	return (ret == 0);
}

/** 透過色のインデックスをセット */

void mSaveImageGIF_setTransparent(mSaveImage *p,int index)
{
	((SAVEGIF *)p->internal)->transparent = index;
}

/* @} */
