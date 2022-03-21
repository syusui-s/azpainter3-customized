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
 * mLoadImage
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_io.h"
#include "mlk_util.h"
#include "mlk_imageconv.h"


//==============================
// 内部で使うもの
//==============================


/**@ mIO を読み込み用に開く */

mIO *mLoadImage_openIO(mLoadImageOpen *p)
{
	mIO *io = NULL;

	switch(p->type)
	{
		case MLOADIMAGE_OPEN_FILENAME:
			io = mIO_openReadFile(p->filename);
			break;
		case MLOADIMAGE_OPEN_FP:
			io = mIO_openReadFILEptr(p->fp);
			break;
		case MLOADIMAGE_OPEN_BUF:
			io = mIO_openReadBuf(p->buf, p->size);
			break;
	}

	return io;
}

/**@ 各読み込み用のバッファ作成と mIO を開く
 * 
 * @d:各読み込み用に使うデータバッファを確保して、ファイルを開く。\
 * データの構造体の先頭は mIO * にすること。\
 * pli->handle に確保したポインタがセットされる。
 *
 * @p:size 構造体のデータサイズ
 * @p:endian ファイルのエンディアン。MIO_ENDIAN_*。\
 * 負の値で、mIO を使わない。
 * @r:エラーコード */

mlkerr mLoadImage_createHandle(mLoadImage *pli,int size,int endian)
{
	void *p;
	mIO **pio;

	//確保

	p = mMalloc0(size);
	if(!p)
	{
		pli->handle = NULL;
		return MLKERR_ALLOC;
	}

	pli->handle = p;

	//開く

	if(endian >= 0)
	{
		pio = (mIO **)p;

		*pio = mLoadImage_openIO(&pli->open);
		if(!(*pio)) return MLKERR_OPEN;

		mIO_setEndian(*pio, endian);
	}

	return MLKERR_OK;
}

/**@ 各読み込みの終了処理
 *
 * @d:handle と errmessage を解放する。\
 * また、パレットをコピーさせていない場合は palette_buf を NULL にする。\
 * \
 * handle を独自で確保している場合は、
 * あらかじめ解放の上、handle = NULL にしておくこと。 */

void mLoadImage_closeHandle(mLoadImage *p)
{
	mFree(p->errmessage);
	mFree(p->handle);

	p->handle = NULL;
	p->errmessage = NULL;

	if(!(p->flags & MLOADIMAGE_FLAGS_COPY_PALETTE))
		p->palette_buf = NULL;
}

/**@ パレットデータをセット
 *
 * @d:フラグでパレットをコピーするように指定されている場合は、
 * 確保してコピー。\
 * それ以外はそのままポインタをセット。
 * 
 * @p:buf NULL なら何もしない
 * @p:size バッファのサイズ
 * @p:num  セットするパレット個数
 * @r:エラーコード */

mlkerr mLoadImage_setPalette(mLoadImage *p,uint8_t *buf,int size,int palnum)
{
	if(buf)
	{
		p->palette_num = palnum;

		if(p->flags & MLOADIMAGE_FLAGS_COPY_PALETTE)
		{
			//コピー

			p->palette_buf = (uint8_t *)mMemdup(buf, size);
			if(!p->palette_buf) return MLKERR_ALLOC;
		}
		else
			p->palette_buf = buf;
	}

	return MLKERR_OK;
}

/**@ mLoadImage の値を元に、mImageConv の値をセット
 *
 * @d:srcbits は 8。dstbits は bits_per_sample から。\
 * convtype は、RGB/RGBA/なしのいずれか。\
 * dstbuf,srcbuf,srcbits,endian,flags は必要に応じでセットする。 */

void mLoadImage_setImageConv(mLoadImage *p,mImageConv *dst)
{
	mMemset0(dst, sizeof(mImageConv));

	dst->palbuf = p->palette_buf;
	dst->srcbits = 8;
	dst->dstbits = p->bits_per_sample;
	dst->width = p->width;

	//変換タイプ

	if(p->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB)
		dst->convtype = MIMAGECONV_CONVTYPE_RGB;
	else if(p->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
		dst->convtype = MIMAGECONV_CONVTYPE_RGBA;
	else
		dst->convtype = MIMAGECONV_CONVTYPE_NONE;
}


//==============================


/**@ mLoadImage 構造体を初期化
 *
 * @d:ゼロクリアする。 */

void mLoadImage_init(mLoadImage *p)
{
	memset(p, 0, sizeof(mLoadImage));
}

/**@ ファイルヘッダからフォーマットを判定し、必要な情報を取得
 *
 * @d:TGA の場合、ヘッダに識別子がないため、正確な判定が行えない。\
 * TGA に対応する場合は、判定を最後にすること。
 * 
 * @p:dst いずれかに一致した場合、そのフォーマットの情報が格納される
 * @p:open ヘッダを読み込むための開く情報
 * @p:checks 読み込みに対応する各フォーマットのチェック関数を配列で並べる。\
 * 値が NULL で終了。\
 * 値が MLOADIMAGE_CHECKFORMAT_SKIP で、スキップ。
 * @p:headsize 読み込むヘッダの最大サイズ (0 以下で 17 byte)\
 * PNG: 8byte、WEBP: 12byte、TGA: 17byte
 * @r:エラーコード。\
 * いずれにも一致しなかった場合、MLKERR_UNSUPPORTED が返る。 */

mlkerr mLoadImage_checkFormat(mLoadImageType *dst,
	mLoadImageOpen *open,mFuncLoadImageCheck *checks,int headsize)
{
	mIO *io;
	uint8_t *buf;
	mlkbool ret;

	if(headsize <= 0) headsize = 17;

	//ヘッダバッファ

	buf = (uint8_t *)mMalloc(headsize);
	if(!buf) return MLKERR_ALLOC;

	//先頭バイト読み込み

	io = mLoadImage_openIO(open);
	if(!io)
	{
		mFree(buf);
		return MLKERR_OPEN;
	}

	headsize = mIO_read(io, buf, headsize);

	mIO_close(io);

	//各判定

	for(ret = FALSE; *checks; checks++)
	{
		if(*checks == MLOADIMAGE_CHECKFORMAT_SKIP)
			continue;
	
		ret = (checks[0])(dst, buf, headsize);
		if(ret) break;
	}

	//

	mFree(buf);

	return (ret)? MLKERR_OK: MLKERR_UNSUPPORTED;
}

/**@ イメージのバッファを解放
 *
 * @d:imgbuf 自体と、各ラインのバッファを解放する。\
 * mLoadImage_allocImage() で確保されたバッファの解放で使う。 */

void mLoadImage_freeImage(mLoadImage *p)
{
	uint8_t **ppbuf;
	int i;

	ppbuf = p->imgbuf;

	if(ppbuf)
	{
		for(i = p->height; i > 0; i--, ppbuf++)
			mFree(*ppbuf);

		mFree(p->imgbuf);
	}
}

/**@ 読み込み用のイメージバッファを確保
 *
 * @d:実行後、imgbuf と line_bytes が上書きされる。
 * 
 * @p:line_bytes Y1行のバイト数。\
 *  0 以下で、カラータイプから計算 (4byte単位)。
 * @r:すべて確保できたか */

mlkbool mLoadImage_allocImage(mLoadImage *p,int line_bytes)
{
	uint8_t **ppbuf;
	int i;

	//Y1行バイト数

	if(line_bytes <= 0)
	{
		line_bytes = mLoadImage_getLineBytes(p);
		line_bytes = (line_bytes + 3) & (~3);
	}

	p->line_bytes = line_bytes;

	//イメージバッファ

	ppbuf = p->imgbuf = (uint8_t **)mMalloc0(p->height * sizeof(void*));
	if(!ppbuf) return FALSE;

	for(i = p->height; i > 0; i--, ppbuf++)
	{
		*ppbuf = (uint8_t *)mMalloc(line_bytes);
		if(!(*ppbuf))
		{
			mLoadImage_freeImage(p);
			return FALSE;
		}
	}

	return TRUE;
}

/**@ 読み込み用のイメージバッファを確保
 *
 * @d:一つのバッファでピクセルが連続しているイメージから、ポインタをセットする。\
 * imgbuf を確保し、各ラインには、buf 内のポインタをセットする。\
 * ※ imgbuf の各ラインは確保されたバッファではないので、解放時は mFree(imgbuf) とする。
 * mLoadImage_freeImage() は使わないこと。
 * 
 * @p:line_bytes buf の Y1行のバイト数
 * @r:imgbuf が確保できたか */

mlkbool mLoadImage_allocImageFromBuf(mLoadImage *p,void *buf,int line_bytes)
{
	uint8_t **ppbuf,*ps;
	int i;

	ppbuf = (uint8_t **)mMalloc(sizeof(void *) * p->height);
	if(!ppbuf) return FALSE;

	p->imgbuf = ppbuf;
	p->line_bytes = line_bytes;

	//ポインタセット

	ps = (uint8_t *)buf;

	for(i = p->height; i > 0; i--)
	{
		*(ppbuf++) = ps;
	
		ps += line_bytes;
	}

	return TRUE;
}

/**@ Y1行のバイト数を計算
 *
 * @r:必要な最小限のバイト数 */

int mLoadImage_getLineBytes(mLoadImage *p)
{
	int ret;

	ret = p->width;

	if(p->bits_per_sample == 16)
		ret *= 2;

	switch(p->coltype)
	{
		case MLOADIMAGE_COLTYPE_RGB:
			ret *= 3;
			break;
		case MLOADIMAGE_COLTYPE_RGBA:
		case MLOADIMAGE_COLTYPE_CMYK:
			ret *= 4;
			break;
		case MLOADIMAGE_COLTYPE_GRAY_A:
			ret *= 2;
			break;
	}

	return ret;
}

/**@ 解像度を DPI 単位で取得
 *
 * @r:DPI で取得できない場合 FALSE */

mlkbool mLoadImage_getDPI(mLoadImage *p,int *horz,int *vert)
{
	int h,v;

	h = p->reso_horz;
	v = p->reso_vert;

	if(h <= 0 || v <= 0) return FALSE;

	switch(p->reso_unit)
	{
		//dot/inch
		case MLOADIMAGE_RESOUNIT_DPI:
			*horz = h;
			*vert = v;
			return TRUE;
		//dot/meter
		case MLOADIMAGE_RESOUNIT_DPM:
			*horz = mDPMtoDPI(h);
			*vert = mDPMtoDPI(v);
			return TRUE;
		//dot/centimeter
		case MLOADIMAGE_RESOUNIT_DPCM:
			*horz = mDPMtoDPI(h * 100);
			*vert = mDPMtoDPI(v * 100);
			return TRUE;
	}

	return FALSE;
}


//==============================
// EXIF
//==============================

typedef struct
{
	const uint8_t *cur,*top;
	int remain,
		size,
		fbig;
}_exifval;


/* 解像度単位を取得 */

static int _exif_getunit(int unit)
{
	if(unit == 2)
		return MLOADIMAGE_RESOUNIT_DPI;
	else if(unit == 3)
		return MLOADIMAGE_RESOUNIT_DPCM;
	else
		return MLOADIMAGE_RESOUNIT_NONE;
}

/* EXIF 値取得
 *
 * pos: 値のオフセット位置。
 *  0 で現在位置。値分だけ進める。
 *  負の値で現在位置からのオフセット。位置はそのまま。 */

static int _exif_getval(_exifval *p,int pos,int vsize,void *dst)
{
	const uint8_t *ps;
	int remain;

	if(pos <= 0)
	{
		//現在位置から
		
		ps = p->cur + pos;
		remain = p->remain - pos;

		if(pos == 0)
		{
			p->cur += vsize;
			p->remain -= vsize;
		}
	}
	else
	{
		//指定オフセット
		
		if(pos >= p->size) return 1;

		ps = p->top + pos;
		remain = p->size - pos;
	}

	if(remain < vsize) return 1;

	if(vsize == 2)
	{
		if(p->fbig)
			*((uint16_t *)dst) = mGetBufBE16(ps);
		else
			*((uint16_t *)dst) = mGetBufLE16(ps);
	}
	else
	{
		if(p->fbig)
			*((uint32_t *)dst) = mGetBufBE32(ps);
		else
			*((uint32_t *)dst) = mGetBufLE32(ps);
	}

	return 0;
}

/**@ EXIF データから解像度取得
 *
 * @d:先頭が "Exif\\0\\0" で始まる場合と、"MM" or "II" で始まる場合の両方に対応。
 * 
 * @r:セットされた値のフラグ。\
 *  bit0=resoH, bit1=resoV, bit2=unit */

int mLoadImage_getEXIF_resolution(mLoadImage *p,const uint8_t *buf,int size)
{
	_exifval v;
	uint16_t fnum,tag,dattype,v16;
	uint32_t cnt,pos;
	int flags = 0;

	//Exif

	if(size >= 6 && memcmp(buf, "Exif\0\0", 6) == 0)
		buf += 6, size -= 6;

	if(size < 10) return 0;

	//エンディアン

	if(memcmp(buf, "MM\0*", 4) == 0)
		v.fbig = 1;
	else if(memcmp(buf, "II*\0", 4) == 0)
		v.fbig = 0;
	else
		return 0;

	//

	v.top = buf;
	v.size = size;
	v.cur = buf + 8;
	v.remain = size - 8;

	//IFD

	_exif_getval(&v, 0, 2, &fnum);

	for(; fnum && flags != 7; fnum--)
	{
		if(_exif_getval(&v, 0, 2, &tag)
			|| _exif_getval(&v, 0, 2, &dattype)
			|| _exif_getval(&v, 0, 4, &cnt)
			|| _exif_getval(&v, 0, 4, &pos))
			break;
		
		if(tag == 0x011a && dattype == 5)
		{
			//解像度水平

			if(_exif_getval(&v, pos, 4, &pos) == 0)
			{
				p->reso_horz = pos;
				flags |= 1;
			}
		}
		else if(tag == 0x011b && dattype == 5)
		{
			//解像度垂直

			if(_exif_getval(&v, pos, 4, &pos) == 0)
			{
				p->reso_vert = pos;
				flags |= 2;
			}
		}
		else if(tag == 0x0128 && dattype == 3)
		{
			//解像度単位 (short)
			// 2:dpi 3:dpcm

			_exif_getval(&v, -4, 2, &v16);

			p->reso_unit = _exif_getunit(v16);

			flags |= 4;
		}
	}

	return flags;
}

