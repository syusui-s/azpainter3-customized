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
 * WebP 保存
 *****************************************/

#include <stdio.h>
#include <webp/encode.h>

#include "mlk.h"
#include "mlk_saveimage.h"


//---------------

typedef struct
{
	WebPConfig conf;
	WebPPicture pic;
	FILE *fp;
}webpdata;

//---------------


/* 書き込み関数 */

static int _write_func(const uint8_t *data,size_t size,const WebPPicture *pic)
{
	return (fwrite(data, 1, size, (FILE *)pic->custom_ptr) == size);
}

/* 経過関数 */

static int _progress_hook(int percent,const WebPPicture *pic)
{
	mSaveImage *si = (mSaveImage *)pic->user_data;

	(si->progress)(si, percent);
	
	return 1;
}

/* ライン変換 RGB/RGBA => ARGB (uint32) */

static void _convert_line(mSaveImage *si,uint32_t *buf)
{
	uint8_t *ps;
	int i;

	ps = (uint8_t *)buf;

	if(si->samples_per_pixel == 3)
	{
		//RGB => ARGB

		i = si->width - 1;

		ps += i * 3;
		buf += i;

		for(; i >= 0; i--)
		{
			*(buf--) = ((uint32_t)ps[0] << 16) | (ps[1] << 8) | ps[2] | 0xff000000;
			ps -= 3;
		}
	}
	else
	{
		//RGBA => ARGB

		for(i = si->width; i > 0; i--)
		{
			*(buf++) = ((uint32_t)ps[3] << 24) | (ps[0] << 16) | (ps[1] << 8) | ps[2];
			ps += 4;
		}
	}
}

/* 圧縮のプリセット値取得
 *
 * level = -1 で不可逆  */

static void _get_preset(mSaveOptWEBP *opt,int *level,float *quality,int *preset)
{
	//デフォルトで可逆
	*level = 6;
	*quality = 75;
	*preset = 0;
	
	if(opt)
	{
		if(opt->mask & MSAVEOPT_WEBP_MASK_LEVEL)
			*level = opt->level;

		if(opt->mask & MSAVEOPT_WEBP_MASK_QUALITY)
			*quality = opt->quality;

		if(opt->mask & MSAVEOPT_WEBP_MASK_PRESET)
			*preset = opt->preset;

		//不可逆か
		
		if((opt->mask & MSAVEOPT_WEBP_MASK_LOSSY) && opt->lossy)
			*level = -1;
	}
}

/* メイン処理 */

static int _main_proc(webpdata *p,mSaveImage *si,mSaveOptWEBP *opt)
{
	int ret,level,i,pitch,preset;
	float quality;
	uint32_t *buf;
	WebPPicture *pic = &p->pic;

	//picture 初期化

	if(!WebPPictureInit(pic))
		return MLKERR_ALLOC;

	pic->use_argb = 1;
	pic->width = si->width;
	pic->height = si->height;

	if(!WebPPictureAlloc(pic))
		return MLKERR_ALLOC;

	//設定初期化

	_get_preset(opt, &level, &quality, &preset);

	if(level == -1)
		//不可逆
		WebPConfigPreset(&p->conf, preset, quality);
	else
	{
		//可逆
		WebPConfigInit(&p->conf);
		WebPConfigLosslessPreset(&p->conf, level);
	}

	//設定検証

	if(!WebPValidateConfig(&p->conf))
		return MLKERR_INVALID_VALUE;

	//開く

	p->fp = (FILE *)mSaveImage_openFile(si);
	if(!p->fp) return MLKERR_OPEN;

	//書き込み関数

	pic->writer = _write_func;
	pic->custom_ptr = p->fp;

	//経過関数

	if(si->progress)
	{
		pic->progress_hook = _progress_hook;
		pic->user_data = si;
	}

	//イメージ取得

	buf = pic->argb;
	pitch = si->width * si->samples_per_pixel;

	for(i = 0; i < si->height; i++)
	{
		//RGB or RGBA で取得
	
		ret = (si->setrow)(si, i, (uint8_t *)buf, pitch);
		if(ret) return ret;

		//変換

		_convert_line(si, buf);

		//

		buf += pic->argb_stride;
	}

	//エンコード

	if(!WebPEncode(&p->conf, pic))
		return MLKERR_ENCODE;

	return MLKERR_OK;
}


//========================


/**@ WebP 保存 */

mlkerr mSaveImageWEBP(mSaveImage *si,void *opt)
{
	webpdata *p;
	int ret;

	//最大サイズ

	if(si->width > MSAVEIMAGE_MAXSIZE_WEBP
		|| si->height > MSAVEIMAGE_MAXSIZE_WEBP)
		return MLKERR_MAX_SIZE;

	//

	p = (webpdata *)mMalloc0(sizeof(webpdata));
	if(!p) return MLKERR_OK;

	ret = _main_proc(p, si, (mSaveOptWEBP *)opt);

	//

	WebPPictureFree(&p->pic);

	mSaveImage_closeFile(si, p->fp);

	mFree(p);

	return ret;
}
