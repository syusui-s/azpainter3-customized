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
 * BMP 保存
 *****************************************/

#include <stdio.h>

#include "mlk.h"
#include "mlk_saveimage.h"
#include "mlk_stdio.h"
#include "mlk_util.h"
#include "mlk_imageconv.h"


//---------------

typedef struct
{
	FILE *fp;
	uint8_t *rowbuf;	//Y1行分のバッファ

	int bits,
		pitch,				//4byte 境界の余白あり
		pitch_nopad,		//余白なし
		infoheader_size;	//INFOHEADER サイズ
}bmpdata;

//---------------


//===========================
// 書き込み
//===========================


/** ファイルヘッダ書き込み */

static int _write_fileheader(bmpdata *p,mSaveImage *si)
{
	uint8_t d[14];
	uint32_t headsize;

	headsize = 14 + p->infoheader_size;

	if(p->bits <= 8)
		headsize += si->palette_num << 2;

	//

	mSetBuf_format(d, "<siii",
		"BM",
		headsize + p->pitch * si->height, //イメージサイズ
		0,
		headsize);	//イメージまでのオフセット

	if(fwrite(d, 1, 14, p->fp) == 14)
		return MLKERR_OK;
	else
		return MLKERR_IO;
}

/** 情報ヘッダ書き込み */

static int _write_infoheader(bmpdata *p,mSaveImage *si)
{
	uint8_t d[40],
		bitfields[16] = {0,0,0xff,0, 0,0xff,0,0, 0xff,0,0,0, 0,0,0,0xff};
	int resoh,resov;

	//解像度 (DPM)

	mSaveImage_getDPM(si, &resoh, &resov);

	//基本情報

	mSetBuf_format(d, "<iiihhiiiiii",
		p->infoheader_size,
		si->width,
		si->height,
		1,			//plane
		p->bits,	//ビット数
		(p->bits == 32)? 3: 0,	//compression (アルファ値ありなら BI_BITFIELDS)
		p->pitch * si->height,	//イメージサイズ
		resoh,
		resov,
		(p->bits <= 8)? si->palette_num: 0,	//パレット数
		0);		//重要な色数

	if(fwrite(d, 1, 40, p->fp) != 40)
		return MLKERR_IO;

	//アルファチャンネルありなら ver4 追加

	if(p->bits == 32)
	{
		//ビットフィールド & 他は0

		if(fwrite(bitfields, 1, 16, p->fp) != 16
			|| mFILEwrite0(p->fp, 108 - 40 - 16))
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/** パレット書き込み */

static int _write_palette(bmpdata *p,mSaveImage *si)
{
	int i,num,size,ret = 0;
	uint8_t *buf,*ps,*pd;

	num = si->palette_num;
	size = num << 2;

	buf = (uint8_t *)mMalloc(size);
	if(!buf) return MLKERR_ALLOC;

	//RGBX => BGRX に変換

	ps = si->palette_buf;
	pd = buf;

	for(i = num; i > 0; i--, ps += 4, pd += 4)
	{
		pd[0] = ps[2];
		pd[1] = ps[1];
		pd[2] = ps[0];
		pd[3] = 0;
	}

	if(fwrite(buf, 1, size, p->fp) != size)
		ret = MLKERR_IO;

	mFree(buf);

	return ret;
}


//===========================


/** 初期化 */

static int _init(bmpdata *p,mSaveImage *si)
{
	//ピクセルビット数

	if(si->coltype == MSAVEIMAGE_COLTYPE_PALETTE)
		p->bits = si->bits_per_sample;
	else
		p->bits = si->samples_per_pixel * 8;

	//INFOHEADER サイズ (アルファ値ありなら ver 4)

	p->infoheader_size = (p->bits == 32)? 108: 40;

	//Y1行バイト数

	p->pitch_nopad = (si->width * p->bits + 7) >> 3;
	p->pitch = (p->pitch_nopad + 3) & (~3);

	//Y1行バッファ
	/* 4byte 境界の余白を 0 にするため、ゼロクリアしておく */
	
	p->rowbuf = (uint8_t *)mMalloc0(p->pitch);
	if(!p->rowbuf) return MLKERR_ALLOC;

	return MLKERR_OK;
}

/** メイン処理 */

static int _main_proc(bmpdata *p,mSaveImage *si)
{
	int ret,i,height,last_prog,new_prog;
	uint8_t *rowbuf;
	mFuncSaveImageProgress progress;

	//開く

	p->fp = (FILE *)mSaveImage_openFile(si);
	if(!p->fp) return MLKERR_OPEN;

	//初期化

	ret = _init(p, si);
	if(ret) return ret;

	//ファイルヘッダ

	ret = _write_fileheader(p, si);
	if(ret) return ret;

	//情報ヘッダ

	ret = _write_infoheader(p, si);
	if(ret) return ret;

	//パレット

	if(p->bits <= 8)
	{
		ret = _write_palette(p, si);
		if(ret) return ret;
	}

	//イメージ

	rowbuf = p->rowbuf;
	progress = si->progress;
	height = si->height;
	last_prog = 0;

	for(i = 0; i < height; i++)
	{
		ret = (si->setrow)(si, height - 1 - i, rowbuf, p->pitch_nopad);
		if(ret) return ret;

		//RGB 変換

		if(p->bits >= 24)
			mImageConv_swap_rb_8(rowbuf, si->width, si->samples_per_pixel);

		//書き込み

		if(fwrite(rowbuf, 1, p->pitch, p->fp) != p->pitch)
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

	return MLKERR_OK;
}


//========================


/**@ BMP 保存 */

mlkerr mSaveImageBMP(mSaveImage *si,void *opt)
{
	bmpdata dat;
	int ret;

	mMemset0(&dat, sizeof(bmpdata));

	ret = _main_proc(&dat, si);

	//

	mSaveImage_closeFile(si, dat.fp);

	mFree(dat.rowbuf);

	return ret;
}
