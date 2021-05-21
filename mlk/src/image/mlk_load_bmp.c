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
 * BMP 読み込み
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_io.h"
#include "mlk_util.h"
#include "mlk_imageconv.h"


//--------------------

typedef struct
{
	mIO *io;
	
	uint8_t *rawbuf,	//Y1行分の生バッファ
		*palbuf,		//パレットバッファ
		*rlebuf,		//RLE:入力バッファ
		*rlebuf_end,	//RLE:入力データの終端位置
		*rlebuf_cur;	//RLE:展開の現在位置

	int32_t width,
		height,
		bits,			//ビット数 (1,4,8,16,24,32)
		palnum,			//パレット数
		pitch,			//Y1行バイト数
		maxR,maxG,maxB,maxA;	//各色の最大値

	uint32_t compression,	//圧縮タイプ
		infoheader_size,	//INFOHEADER のサイズ
		maskR,maskG,maskB,maskA;	//RGBA のビットマスク (0 = 値は常に0。A の場合、0 なら不透明)

	uint8_t fbottomup,   //1=ボトムアップ
		is_rle_end_file, //RLE:ファイルの最後まで読み込んだか
		is_rle_end_bmp,  //RLE:EOB が出現したか
		shiftR,shiftG,shiftB,shiftA;	//シフト数
}bmpdata;

//--------------------

#define BI_RGB        0
#define BI_RLE8       1
#define BI_RLE4       2
#define BI_BITFIELDS  3

#define RLE_BUFSIZE   1024

//--------------------



//=================================
// 情報部分読み込み
//=================================


/* ビットフィールドのマスク値から、シフト数と値の最大値取得 */

static void _get_maskvalue(uint32_t mask,uint8_t *shift,int32_t *max)
{
	int s,m;

	if(mask == 0)
		s = m = 0;
	else
	{
		s = mGetOnBitPosL(mask);
		m = (1 << mGetOffBitPosL(mask >> s)) - 1;
	}

	*shift = s;
	*max = m;
}

/** ファイルヘッダ読み込み
 *
 * return: エラーコード */

static int _read_fileheader(bmpdata *p,mLoadImage *pli)
{
	uint8_t d[2];

	//ヘッダ判定

	if(mIO_readOK(p->io, d, 2)
		|| d[0] != 'B' || d[1] != 'M')
		return MLKERR_FORMAT_HEADER;

	//情報ヘッダまでスキップ
	
	if(mIO_readSkip(p->io, 12))
		return MLKERR_DAMAGED;
	else
		return MLKERR_OK;
}

/** ビットフィールド読み込みと、情報のセット
 *
 * readbytes: 読み込んだバイト数
 * return: エラーコード */

static int _read_bitfield(bmpdata *p,int *readbytes)
{
	uint8_t d[16];
	int size;

	if(p->compression == BI_BITFIELDS)
	{
		//ビットフィールドあり
		//(V4 以降ならアルファ値のマスクもある)

		size = (p->infoheader_size >= 108)? 16: 12;

		if(mIO_readOK(p->io, d, size))
			return MLKERR_DAMAGED;

		p->maskR = mGetBufLE32(d);
		p->maskG = mGetBufLE32(d + 4);
		p->maskB = mGetBufLE32(d + 8);

		if(size == 16)
			p->maskA = mGetBufLE32(d + 12);

		*readbytes = size;
	}
	else
	{
		//デフォルト値

		if(p->bits == 16)
		{
			p->maskR = 0x7c00;
			p->maskG = 0x03e0;
			p->maskB = 0x001f;
		}
		else
		{
			p->maskR = 0xff0000;
			p->maskG = 0x00ff00;
			p->maskB = 0x0000ff;
		}
	}

	//各パラメータ

	_get_maskvalue(p->maskR, &p->shiftR, &p->maxR);
	_get_maskvalue(p->maskG, &p->shiftG, &p->maxG);
	_get_maskvalue(p->maskB, &p->shiftB, &p->maxB);
	_get_maskvalue(p->maskA, &p->shiftA, &p->maxA);

	return MLKERR_OK;
}

/** 情報ヘッダ読み込み */

static int _read_infoheader(bmpdata *p,mLoadImage *pli)
{
	uint8_t d[36];
	uint32_t size,clrused;
	int32_t width,height;
	int n,ret;

	//ヘッダサイズ

	if(mIO_read32(p->io, &size))
		return MLKERR_DAMAGED;

	if(size != 40 && size != 52 && size != 56 && size != 108 && size != 124)
		return MLKERR_UNSUPPORTED;

	p->infoheader_size = size;

	//36 byte 読み込み

	if(mIO_readOK(p->io, d, 36))
		return MLKERR_DAMAGED;

	//------ 基本情報

	//圧縮形式

	p->compression = mGetBufLE32(d + 12);

	if(p->compression > BI_BITFIELDS)
		return MLKERR_UNSUPPORTED;

	//幅・高さ

	width  = (int32_t)mGetBufLE32(d);
	height = (int32_t)mGetBufLE32(d + 4);

	if(width <= 0 || height == 0)
		return MLKERR_INVALID_VALUE;

	if(height > 0)
		p->fbottomup = TRUE;
	else
		//トップダウン
		height = -height;

	p->width = width;
	p->height = height;

	//ビット数

	n = mGetBufLE16(d + 10);

	if(n != 1 && n != 4 && n != 8 && n != 16 && n != 24 && n != 32)
		return MLKERR_INVALID_VALUE;

	p->bits = n;

	p->pitch = (((width * n + 7) >> 3) + 3) & (~3);

	//解像度

	pli->reso_unit = MLOADIMAGE_RESOUNIT_DPM;
	pli->reso_horz = mGetBufLE32(d + 20);
	pli->reso_vert = mGetBufLE32(d + 24);

	//パレット数

	clrused = mGetBufLE32(d + 28);

	if(p->bits >= 16)
		//16bit 以上
		n = 0;
	else if(clrused == 0)
		//自動
		n = 1 << p->bits;
	else
	{
		n = clrused;

		//最大値より多い
		if(n > (1 << p->bits))
			return MLKERR_INVALID_VALUE;
	}

	p->palnum = n;

	//------- 以降データ

	//ビットフィールド読み込みと情報セット

	n = 0;

	if(p->bits == 16 || p->bits == 32)
	{
		ret = _read_bitfield(p, &n);
		if(ret) return ret;
	}

	/* ヘッダサイズが 40 なら、
	 * ビットフィールドはヘッダに含まない。 */

	if(size == 40) n = 0;

	//残りのヘッダをスキップ

	if(mIO_readSkip(p->io, size - 40 - n))
		return MLKERR_DAMAGED;

	//16bit 以上の場合、カラーテーブルをスキップ

	if(p->bits >= 16 && clrused)
	{
		if(mIO_readSkip(p->io, clrused << 2))
			return MLKERR_DAMAGED;
	}

	//------- mLoadImage にセット

	pli->width = width;
	pli->height = height;
	pli->bits_per_sample = 8;

	//カラータイプ

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		n = MLOADIMAGE_COLTYPE_RGB;
	else if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		n = MLOADIMAGE_COLTYPE_RGBA;
	else if(p->bits <= 8)
		n = MLOADIMAGE_COLTYPE_PALETTE;
	else if(p->maskA)
		n = MLOADIMAGE_COLTYPE_RGBA;
	else
		//32bit でアルファ値無効の場合、RGB
		n = MLOADIMAGE_COLTYPE_RGB;

	pli->coltype = n;

	return MLKERR_OK;
}

/** 基本情報を読み込み */

static int _read_info(bmpdata *p,mLoadImage *pli)
{
	int ret;

	//ファイルヘッダ

	ret = _read_fileheader(p, pli);
	if(ret) return ret;

	//情報ヘッダ

	ret = _read_infoheader(p, pli);
	if(ret) return ret;

	return MLKERR_OK;
}


//===========================
// RLE 展開
//===========================


/** RLE バッファに入力データ読み込み
 *
 * return: エラーコード */

static int _rle_read_input(bmpdata *p)
{
	int remain,read;

	remain = p->rlebuf_end - p->rlebuf_cur;

	if(p->is_rle_end_file)
	{
		//ファイルの終端まで読み込んだ状態の場合
		/* 最後に必ず EOB (2byte) があるので、2byte 以下ならデータが足りないとみなす */

		return (remain < 2)? MLKERR_DAMAGED: MLKERR_OK;
	}
	else if(remain <= 257)
	{
		//バッファの残りが 257byte 以下なら読み込み
		/* ひとかたまりのデータの最大が 257byte (非連続値: 情報2byte + データ255byte) なので、
		 * 最大サイズ分を読み込んでおく */

		//残りを先頭へ

		memmove(p->rlebuf, p->rlebuf_cur, remain);

		//読み込み
		//(圧縮後のサイズがわからないので、バッファサイズ分読み込み)

		read = mIO_read(p->io, p->rlebuf + remain, RLE_BUFSIZE - remain);

		//読み込みサイズより少ない場合は、ファイルの終端とみなす

		if(read < RLE_BUFSIZE - remain)
			p->is_rle_end_file = TRUE;

		remain += read;
		if(remain < 2) return MLKERR_DAMAGED;
		
		p->rlebuf_end = p->rlebuf + remain;
		p->rlebuf_cur = p->rlebuf;
	}

	return MLKERR_OK;
}

/** Y1行 RLE 展開 (4bit) */

static int _decompress_rle4(bmpdata *p)
{
	uint8_t *pd,*ps,b1,b2,val;
	int ret,width,x = 0,len,size,size2;

	pd = p->rawbuf;
	width = p->width;

	//4bit の場合、1byte に 2px 分があるため、奇数幅時は +1px を許容する
	if(width & 1) width++;

	while(1)
	{
		//読み込み

		ret = _rle_read_input(p);
		if(ret) return ret;

		//2byte 読み込み

		b1 = p->rlebuf_cur[0];
		b2 = p->rlebuf_cur[1];
		p->rlebuf_cur += 2;

		/* [!] 4bit の場合は、2px 単位のデータとなる。
		 * そのため、奇数px数の場合は余分に 1px 分のデータが含まれる。 */

		if(b1 != 0)
		{
			//---- 連続データ ([1byte:px数 (1-255)] [1byte:値])

			len = b1;
			if(x + len > width) return MLKERR_DECODE;

			/* 先頭 x 位置が奇数の場合、1px分出力し、
			 * 連続値の 4bit 分を上下入れ替える。 */

			if(x & 1)
			{
				*(pd++) |= b2 >> 4;
				b2 = (b2 >> 4) | ((b2 & 15) << 4);
				len--;
			}

			//2px単位 (1byte) で出力

			for(; len > 0; len -= 2)
				*(pd++) = b2;

			//下位4bitを余分に出力した場合は、戻る

			if(len == -1)
				*(--pd) &= 0xf0;

			x += b1;
		}
		else if(b2 == 0 || b2 == 1)
		{
			//行終わり [0, 0] or イメージ終わり [0, 1]

			memset(pd, 0, p->pitch - (x + 1) / 2);
			
			if(b2 == 1) p->is_rle_end_bmp = TRUE;

			break;
		}
		else if(b2 == 2)
			//位置移動 [0, 2] は非対応
			return MLKERR_UNSUPPORTED;
		else
		{
			//----- 非連続データ
			//[0] [1byte:px数 (4-254)] [データ (2byte境界)]

			len = b2;
			size = (len + 1) >> 1;
			size2 = (size + 1) & (~1);  //2byte境界サイズ

			if(x + len > width) return MLKERR_DECODE;

			if(p->rlebuf_end - p->rlebuf_cur < size2)
				return MLKERR_DAMAGED;

			ps = p->rlebuf_cur;

			if(x & 1)
			{
				//先頭 x 位置が奇数

				for(; len > 0; len -= 2)
				{
					val = *(ps++);

					*(pd++) |= val >> 4;
					if(len != 1) *pd = (val & 15) << 4;
				}
			}
			else
			{
				//先頭 x 位置が偶数

				memcpy(pd, ps, size);

				pd += size;

				if(len & 1)
					*(--pd) &= 0xf0;
			}
			
			x += b2;
			p->rlebuf_cur += size2;
		}
	}

	return MLKERR_OK;
}

/** Y1行を RLE 展開 (8bit) */

static int _decompress_rle8(bmpdata *p)
{
	uint8_t *pd,b1,b2;
	int ret,width,x = 0,size;

	pd = p->rawbuf;
	width = p->width;

	while(1)
	{
		//データ読み込み

		ret = _rle_read_input(p);
		if(ret) return ret;

		//2byte 読み込み

		b1 = p->rlebuf_cur[0];
		b2 = p->rlebuf_cur[1];
		p->rlebuf_cur += 2;

		//

		if(b1 != 0)
		{
			//連続データ
			//[1byte:連続px数 (1-255)] [1byte:値]

			if(x + b1 > width) return MLKERR_DECODE;

			memset(pd, b2, b1);

			pd += b1;
			x += b1;
		}
		else if(b2 == 0 || b2 == 1)
		{
			//行終わり [0, 0] or イメージ終わり [0, 1]

			memset(pd, 0, p->pitch - x);

			if(b2 == 1) p->is_rle_end_bmp = TRUE;
			
			break;
		}
		else if(b2 == 2)
			//位置移動 [0, 2] は非対応
			return MLKERR_UNSUPPORTED;
		else
		{
			//非連続データ
			//[0] [1byte:px数 (3-255)] [データ (2byte境界)]

			if(x + b2 > width) return MLKERR_DECODE;

			size = (b2 + 1) & (~1);

			if(p->rlebuf_end - p->rlebuf_cur < size)
				return MLKERR_DAMAGED;

			memcpy(pd, p->rlebuf_cur, b2);

			p->rlebuf_cur += size;

			pd += b2;
			x += b2;
		}
	}

	return MLKERR_OK;
}


//=================================
// イメージ処理
//=================================


/** パレット読み込み */

static int _read_palette(bmpdata *p,mLoadImage *pli)
{
	uint8_t *pb,tmp;
	int i,num;

	num = 1 << p->bits;

	//確保

	pb = p->palbuf = (uint8_t *)mMalloc0(num * 4);
	if(!pb) return MLKERR_ALLOC;

	//読み込み

	if(mIO_readOK(p->io, pb, p->palnum * 4))
		return MLKERR_DAMAGED;

	//RGB 順入れ替え (BGRX => RGBX)

	for(i = num; i > 0; i--, pb += 4)
	{
		tmp = pb[0];
		pb[0] = pb[2];
		pb[2] = tmp;
		pb[3] = 255;
	}

	//mLoadImage にセット

	return mLoadImage_setPalette(pli, p->palbuf, num * 4, p->palnum);
}

/** メモリ確保 */

static mlkbool _alloc_buf(bmpdata *p)
{
	//Y1行、生データ

	p->rawbuf = (uint8_t *)mMalloc(p->pitch);
	if(!p->rawbuf) return FALSE;

	//RLE 用

	if(p->compression == BI_RLE4 || p->compression == BI_RLE8)
	{
		p->rlebuf = (uint8_t *)mMalloc(RLE_BUFSIZE);
		if(!p->rlebuf) return FALSE;

		p->rlebuf_end = p->rlebuf_cur = p->rlebuf;
	}

	return TRUE;
}

/** ライン変換 (16/32bit ビットフィールド) */

static void _convline_bitfields(bmpdata *p,mLoadImage *pli,uint8_t *pd)
{
	uint8_t *ps,r,g,b,a,have_alpha;
	int i,f32;
	uint32_t c;

	ps = p->rawbuf;
	f32 = (p->bits == 32);
	have_alpha = (pli->coltype == MLOADIMAGE_COLTYPE_RGBA);

	for(i = p->width; i > 0; i--)
	{
		//色値取得
		
		if(f32)
		{
			c = ((uint32_t)ps[3] << 24) | (ps[2] << 16) | (ps[1] << 8) | ps[0];
			ps += 4;
		}
		else
		{
			c = (ps[1] << 8) | ps[0];
			ps += 2;
		}

		//RGB 取得

		r = g = b = 0;
		a = 255;

		if(p->maskR) r = ((c & p->maskR) >> p->shiftR) * 255 / p->maxR;
		if(p->maskG) g = ((c & p->maskG) >> p->shiftG) * 255 / p->maxG;
		if(p->maskB) b = ((c & p->maskB) >> p->shiftB) * 255 / p->maxB;
		if(p->maskA) a = ((c & p->maskA) >> p->shiftA) * 255 / p->maxA;

		//セット

		pd[0] = r;
		pd[1] = g;
		pd[2] = b;

		if(have_alpha)
		{
			pd[3] = a;
			pd += 4;
		}
		else
			pd += 3;
	}
}

/** イメージ読み込み */

static int _read_image(bmpdata *p,mLoadImage *pli)
{
	uint8_t **ppbuf,*rawbuf;
	mFuncImageConv funcconv;
	mImageConv conv;
	int32_t pitch,i,ret,height,prog_cur,prog;
	mlkbool fRLE;

	ppbuf = pli->imgbuf;
	rawbuf = p->rawbuf;
	height = p->height;

	if(p->fbottomup)
		ppbuf += height - 1;
	
	pitch = p->pitch;
	fRLE = (p->compression == BI_RLE4 || p->compression == BI_RLE8);

	//変換パラメータ

	mLoadImage_setImageConv(pli, &conv);

	conv.srcbuf = rawbuf;
	conv.srcbits = p->bits;
	conv.flags = MIMAGECONV_FLAGS_SRC_BGRA;

	if(p->maskA == 0)
		conv.flags |= MIMAGECONV_FLAGS_INVALID_ALPHA;

	//変換関数

	if(p->bits <= 8)
		funcconv = mImageConv_palette_1_2_4_8;
	else if(p->bits == 24)
		funcconv = mImageConv_rgb8;
	else if(p->bits == 32
		&& p->maskR == 0xff0000 && p->maskG == 0x00ff00 && p->maskB == 0x0000ff)
		funcconv = mImageConv_rgba8;
	else
		//bitfield
		funcconv = NULL;

	//---------

	prog_cur = 0;

	for(i = 0; i < height; i++)
	{
		//rawbuf に生データ読み込み

		if(fRLE)
		{
			//RLE 圧縮

			if(p->is_rle_end_bmp)
				//途中で EOB が出た場合は、残りすべて 0
				memset(rawbuf, 0, pitch);
			else
			{
				if(p->compression == BI_RLE4)
					ret = _decompress_rle4(p);
				else
					ret = _decompress_rle8(p);

				if(ret) return ret;
			}
		}
		else
		{
			//無圧縮

			if(mIO_readOK(p->io, rawbuf, pitch))
				return MLKERR_DAMAGED;
		}

		//変換

		if(funcconv)
		{
			conv.dstbuf = *ppbuf;
			(funcconv)(&conv);
		}
		else
			_convline_bitfields(p, pli, *ppbuf);

		//

		if(p->fbottomup)
			ppbuf--;
		else
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

	return MLKERR_OK;
}


//=================================
// main
//=================================


/* 終了 */

static void _bmp_close(mLoadImage *pli)
{
	bmpdata *p = (bmpdata *)pli->handle;

	if(p)
	{
		mIO_close(p->io);

		mFree(p->palbuf);
		mFree(p->rawbuf);
		mFree(p->rlebuf);
	}

	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _bmp_open(mLoadImage *pli)
{
	int ret;

	//作成・開く

	ret = mLoadImage_createHandle(pli, sizeof(bmpdata), MIO_ENDIAN_LITTLE);
	if(ret) return ret;

	//基本情報読み込み

	return _read_info((bmpdata *)pli->handle, pli);
}

/* イメージ読み込み */

static mlkerr _bmp_getimage(mLoadImage *pli)
{
	bmpdata *p = (bmpdata *)pli->handle;
	int ret;

	//パレット読み込み

	if(p->bits <= 8)
	{
		ret = _read_palette(p, pli);
		if(ret) return ret;
	}

	//メモリ確保

	if(!_alloc_buf(p))
		return MLKERR_ALLOC;

	//読み込み

	return _read_image(p, pli);
}

/**@ BMP 判定と関数セット */

mlkbool mLoadImage_checkBMP(mLoadImageType *p,uint8_t *buf,int size)
{
	if(!buf || (size >= 2 && buf[0] == 'B' && buf[1] == 'M'))
	{
		p->format_tag = MLK_MAKE32_4('B','M','P',' ');
		p->open = _bmp_open;
		p->getimage = _bmp_getimage;
		p->close = _bmp_close;
		
		return TRUE;
	}

	return FALSE;
}

