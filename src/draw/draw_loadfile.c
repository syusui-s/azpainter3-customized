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
 * AppDraw: 画像ファイル読み込み
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_loadimage.h"
#include "mlk_file.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_draw_sub.h"

#include "imagecanvas.h"
#include "tileimage.h"
#include "layerlist.h"
#include "layeritem.h"
#include "fileformat.h"

#include "draw_main.h"


/* mLoadImage 用進捗 (0-100) */

static void _loadimg_progress(mLoadImage *p,int prog)
{
	mPopupProgressThreadSetPos((mPopupProgress *)p->param, prog);
}

/** 画像ファイル読み込み (レイヤ非対応の通常画像)
 *
 * errmes: エラーメッセージがある場合、文字列が確保される */

mlkerr drawImage_loadFile(AppDraw *p,const char *filename,
	uint32_t format,LoadImageOption *opt,mPopupProgress *prog,char **errmes)
{
	mLoadImage li;
	mLoadImageType tp;
	NewCanvasValue canv;
	int n;
	mlkerr ret;

	*errmes = NULL;

	//mLoadImage

	mLoadImage_init(&li);

	li.open.type = MLOADIMAGE_OPEN_FILENAME;
	li.open.filename = filename;
	li.convert_type = MLOADIMAGE_CONVERT_TYPE_RGBA;
	li.flags = MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA;
	li.progress = _loadimg_progress;
	li.param = prog;

	if(opt->bits == 16)
		li.flags |= MLOADIMAGE_FLAGS_ALLOW_16BIT;

	FileFormat_getLoadImageType(format, &tp);

	//開く

	ret = (tp.open)(&li);
	if(ret) goto ERR;

	//サイズ制限

	if(li.width > IMAGE_SIZE_MAX || li.height > IMAGE_SIZE_MAX)
	{
		ret = MLKERR_MAX_SIZE;
		goto ERR;
	}

	//新規キャンバス

	canv.size.w = li.width;
	canv.size.h = li.height;

	if(mLoadImage_getDPI(&li, &n, &n))
		canv.dpi = n;
	else
		canv.dpi = 96;

	canv.bit = opt->bits;
	canv.layertype = -1;
	canv.imgbkcol = 0xffffff;

	if(!drawImage_newCanvas(p, &canv))
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//新規レイヤ

	p->curlayer = LayerList_addLayer_image(p->layerlist, NULL,
		LAYERTYPE_RGBA, li.width, li.height);

	if(!p->curlayer)
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	LayerItem_setName_layerno(p->curlayer, 0);

	//読み込み (ImageCanvas のバッファに)

	mPopupProgressThreadSetMax(prog, 100 + 10);

	li.imgbuf = p->imgcanvas->ppbuf;

	ret = (tp.getimage)(&li);
	if(ret) goto ERR;

	//イメージの処理

	TileImage_loadimgbuf_convert(li.imgbuf, li.width, li.height,
		li.bits_per_sample, opt->ignore_alpha);

	//タイルイメージに変換

	TileImage_convertFromCanvas(p->curlayer->img, p->imgcanvas, prog, 10);

	//

	(tp.close)(&li);

	return MLKERR_OK;

	//---- error
ERR:
	if(li.errmessage)
		*errmes = mStrdup(li.errmessage);

	(tp.close)(&li);
	return ret;
}

/** 画像読み込みエラー時の処理
 *
 * return: FALSE=現状のまま変更なし, TRUE=更新 */

mlkbool drawImage_loadError(AppDraw *p)
{
	LayerItem *top;

	top = LayerList_getTopItem(p->layerlist);

	if(!top)
	{
		//レイヤが一つもない場合は、新規キャンバス

		drawImage_newCanvas(p, NULL);
		return TRUE;
	}
	else if(!p->curlayer)
	{
		//レイヤは存在するが、カレントレイヤがない場合、
		//レイヤの読み込み途中のエラーのため、読み込んだ部分まで有効にする

		p->curlayer = top;
		return TRUE;
	}
	else
	{
		//新規キャンバスが作成されていれば更新、
		//作成前のエラーなら現状維持

		return p->fnewcanvas;
	}
}


//============================
// FileFormat
//============================


/** ファイルのヘッダからフォーマット取得 */

uint32_t FileFormat_getFromFile(const char *filename)
{
	mLoadImageType tp;
	uint8_t d[12];
	const uint8_t pnghead[8] = {0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a};

	if(mReadFileHead(filename, d, 12) == MLKERR_OK)
	{
		if(memcmp(d, "AZPDATA", 7) == 0)
		{
			//APD

			if(d[7] == 3)
				return FILEFORMAT_APD | FILEFORMAT_APD_v4;
			else if(d[7] == 2)
				return FILEFORMAT_APD | FILEFORMAT_APD_v3;
			else if(d[7] < 2)
				return FILEFORMAT_APD | FILEFORMAT_APD_v1v2;
			else
				return FILEFORMAT_UNKNOWN;
		}
		else if(memcmp(d, "AZDWDAT", 7) == 0)
			return FILEFORMAT_ADW;

		else if(memcmp(d, "8BPS", 4) == 0)
			return FILEFORMAT_PSD;

		else if(memcmp(d, pnghead, 8) == 0)
			return FILEFORMAT_PNG;

		else if(d[0] == 0xff && d[1] == 0xd8)
			return FILEFORMAT_JPEG;

		else if(d[0] == 'G' && d[1] == 'I' && d[2] == 'F')
			return FILEFORMAT_GIF;

		else if(d[0] == 'B' && d[1] == 'M')
			return FILEFORMAT_BMP;

		else if(mLoadImage_checkTIFF(&tp, d, 12))
			return FILEFORMAT_TIFF;

		else if(mLoadImage_checkWEBP(&tp, d, 12))
			return FILEFORMAT_WEBP;
	}

	return FILEFORMAT_UNKNOWN;
}

/** フォーマットタイプから、読み込み情報取得 */

mlkbool FileFormat_getLoadImageType(uint32_t format,void *dst)
{
	mLoadImageType *p = (mLoadImageType *)dst;

	if(format & FILEFORMAT_PNG)
		//PNG
		mLoadImage_checkPNG(p, NULL, 0);
	else if(format & FILEFORMAT_JPEG)
		//JPEG
		mLoadImage_checkJPEG(p, NULL, 0);
	else if(format & FILEFORMAT_GIF)
		//GIF
		mLoadImage_checkGIF(p, NULL, 0);
	else if(format & FILEFORMAT_BMP)
		//BMP
		mLoadImage_checkBMP(p, NULL, 0);
	else if(format & FILEFORMAT_TIFF)
		//TIFF
		mLoadImage_checkTIFF(p, NULL, 0);
	else if(format & FILEFORMAT_WEBP)
		//WEBP
		mLoadImage_checkWEBP(p, NULL, 0);
	else if(format & FILEFORMAT_PSD)
		//PSD
		mLoadImage_checkPSD(p, NULL, 0);
	else
		return FALSE;

	return TRUE;
}


