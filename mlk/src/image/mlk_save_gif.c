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
 * GIF 保存
 *****************************************/

#include "mlk.h"
#include "mlk_saveimage.h"
#include "mlk_gifenc.h"


//---------------

typedef struct
{
	mGIFEnc *gifenc;

	uint8_t *rowbuf,	//Y1行バッファ
		*palbuf;		//パレットバッファ (RGB)
}gifdata;

//---------------



/** 初期化 */

static int _init(gifdata *p,mSaveImage *si,mSaveOptGIF *opt)
{
	mGIFEncInfo info;
	mGIFEncImage img;

	//パレットを R-G-B に変換

	p->palbuf = mSaveImage_createPaletteRGB(si);
	if(!p->palbuf) return MLKERR_ALLOC;

	//GIF エンコーダ作成

	info.width = si->width;
	info.height = si->height;
	info.bkgnd_index = 0;
	info.bits = mGIFEnc_getBitsFromColnum(si->palette_num);
	info.palette_buf = p->palbuf;
	info.palette_num = si->palette_num;

	p->gifenc = mGIFEnc_new(&info);
	if(!p->gifenc) return MLKERR_ALLOC;

	//開く

	switch(si->open.type)
	{
		case MSAVEIMAGE_OPEN_FILENAME:
			if(!mGIFEnc_openFile(p->gifenc, si->open.filename))
				return MLKERR_OPEN;
			break;
		case MSAVEIMAGE_OPEN_FP:
			mGIFEnc_openFILEptr(p->gifenc, si->open.fp);
			break;
		default:
			return MLKERR_OPEN;
	}

	//ヘッダ書き込み

	if(!mGIFEnc_writeHeader(p->gifenc))
		return MLKERR_IO;

	//透過色あり時、画像制御ブロック

	if(opt && (opt->mask & MSAVEOPT_GIF_MASK_TRANSPARENT)
		&& opt->transparent >= 0)
	{
		if(!mGIFEnc_writeGrpCtrl(p->gifenc, opt->transparent, 0, 0, FALSE))
			return MLKERR_IO;
	}

	//Y1行バッファ確保

	p->rowbuf = (uint8_t *)mMalloc(si->width);
	if(!p->rowbuf) return MLKERR_ALLOC;

	//画像開始

	img.offx = img.offy = 0;
	img.width = si->width;
	img.height = si->height;
	img.palette_buf = NULL;
	img.palette_num = 0;

	if(!mGIFEnc_startImage(p->gifenc, &img))
		return MLKERR_IO;

	return MLKERR_OK;
}

/** メイン処理 */

static int _main_proc(gifdata *p,mSaveImage *si,mSaveOptGIF *opt)
{
	int ret,i,size,height,last_prog,new_prog;
	mFuncSaveImageProgress progress;

	//初期化

	ret = _init(p, si, opt);
	if(ret) return ret;

	//イメージ

	size = si->width;
	progress = si->progress;
	height = si->height;
	last_prog = 0;

	for(i = 0; i < height; i++)
	{
		//取得

		ret = (si->setrow)(si, i, p->rowbuf, size);
		if(ret) return ret;

		//書き込み

		if(!mGIFEnc_writeImage(p->gifenc, p->rowbuf, size))
			return MLKERR_IO;

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

	//終了

	if(!mGIFEnc_endImage(p->gifenc)
		|| !mGIFEnc_writeEnd(p->gifenc))
		return MLKERR_IO;

	return MLKERR_OK;
}


//========================


/**@ GIF 保存 */

mlkerr mSaveImageGIF(mSaveImage *si,void *opt)
{
	gifdata dat;
	int ret;

	mMemset0(&dat, sizeof(gifdata));

	ret = _main_proc(&dat, si, (mSaveOptGIF *)opt);

	//

	mGIFEnc_close(dat.gifenc);

	mFree(dat.rowbuf);
	mFree(dat.palbuf);

	return ret;
}
