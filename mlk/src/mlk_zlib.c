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
 * mZlib
 *****************************************/
/* -lz */

#include <stdio.h>
#include <zlib.h>

#include "mlk.h"
#include "mlk_zlib.h"


//-----------------------

struct _mZlib
{
	z_stream z;
	FILE *fp;
	uint8_t *buf;		//作業用バッファ
	int is_decode;		//デコードか
	uint32_t bufsize,	//作業用バッファのサイズ
		size;			//ENC:圧縮後のサイズ、DEC:入力の圧縮後サイズ
};

//-----------------------




//===========================
// sub
//===========================


/* 共通初期化
 *
 * bufsize : 0 以下で適当なサイズ
 * level : 圧縮レベル。負の値で展開
 * memlevel : [enc] 0 以下で deflateInit() を使う */

static mZlib *_init(int bufsize,
	int level,int windowbits,int memlevel,int strategy)
{
	mZlib *p;
	z_stream *pz;
	int ret,is_decode;

	is_decode = (level < 0);

	//確保

	p = (mZlib *)mMalloc0(sizeof(mZlib));
	if(!p) return NULL;

	p->is_decode = is_decode;

	//zlib 初期化

	pz = &p->z;

	if(is_decode)
	{
		//展開

		if(windowbits)
			ret = inflateInit2(pz, windowbits);
		else
			ret = inflateInit(pz);
	}
	else
	{
		//圧縮
		
		if(memlevel <= 0)
			ret = deflateInit(pz, level);
		else
			ret = deflateInit2(pz, level, Z_DEFLATED, windowbits, memlevel, strategy);
	}

	if(ret != Z_OK)
	{
		mFree(p);
		return NULL;
	}

	//作業用バッファ

	if(bufsize <= 0) bufsize = 8 * 1024;

	p->buf = (uint8_t *)mMalloc(bufsize);
	if(!p->buf)
	{
		if(is_decode)
			inflateEnd(pz);
		else
			deflateEnd(pz);
		
		mFree(p);
		return NULL;
	}

	p->bufsize = bufsize;

	return p;
}

/* [enc] 書き込み処理 */

static int _encode_write(mZlib *p,int size)
{
	if(p->fp)
	{
		if(fwrite(p->buf, 1, size, p->fp) == size)
			return MLKERR_OK;
		else
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/* [dec] バッファサイズ分を読み込み
 *
 * return: 読み込んだサイズ */

static int _decode_read(mZlib *p)
{
	int size;

	if(p->fp)
		size = fread(p->buf, 1, (p->size < p->bufsize)? p->size: p->bufsize, p->fp);
	else
		size = 0;

	//残りサイズ

	if(size > 0)
		p->size -= size;

	return size;
}


//=================================
// 共通
//=================================


/**@ 解放 */

void mZlibFree(mZlib *p)
{
	if(p)
	{
		if(p->is_decode)
			inflateEnd(&p->z);
		else
			deflateEnd(&p->z);
	
		mFree(p->buf);
		mFree(p);
	}
}

/**@ 入出力としてファイルポインタ (FILE *) をセット */

void mZlibSetIO_stdio(mZlib *p,void *fp)
{
	p->fp = (FILE *)fp;
}


//=================================
// 圧縮
//=================================


/**@ 圧縮用に作成
 *
 * @g:圧縮
 *
 * @p:bufsize 作業用バッファのサイズ (0 以下で適当なサイズ)
 * @p:level 圧縮率 (0-9)
 * @p:windowbits [8-15] zlib 形式 [16-31] gzip 形式 [-8...-15] zlib 形式ヘッダなし
 * @p:memlevel 1-9 (default: 8)
 * @p:strategy 圧縮タイプ (default: 0) */

mZlib *mZlibEncNew(int bufsize,
	int level,int windowbits,int memlevel,int strategy)
{
	mZlib *p;

	p = _init(bufsize, level, windowbits, memlevel, strategy);
	if(!p) return NULL;

	p->z.next_out  = p->buf;
	p->z.avail_out = p->bufsize;

	return p;
}

/**@ 圧縮用に作成 (圧縮率以外デフォルト) */

mZlib *mZlibEncNew_default(int bufsize,int level)
{
	return mZlibEncNew(bufsize, level, 0, -1, 0);
}

/**@ 圧縮後のサイズを取得 */

uint32_t mZlibEncGetSize(mZlib *p)
{
	return p->size;
}

/**@ zlib の圧縮状態をリセットする
 *
 * @d:同じ mZlib を使って別のデータを圧縮する時に使う。
 * @r:成功したか */

mlkbool mZlibEncReset(mZlib *p)
{
	p->z.next_out  = p->buf;
	p->z.avail_out = p->bufsize;
	p->size = 0;

	return (deflateReset(&p->z) == Z_OK);
}

/**@ データを送って圧縮する
 *
 * @d:作業用バッファに圧縮データを出力し、
 * バッファが一杯になったら出力先へ出力する。
 * @r:エラーコード */

mlkerr mZlibEncSend(mZlib *p,void *buf,uint32_t size)
{
	z_stream *z = &p->z;
	int ret,bufsize;

	bufsize = p->bufsize;

	z->next_in  = (unsigned char *)buf;
	z->avail_in = size;

	while(z->avail_in)
	{
		ret = deflate(z, Z_NO_FLUSH);

		//エラー

		if(ret != Z_OK && ret != Z_STREAM_END)
			return MLKERR_ENCODE;

		//バッファが一杯なので書き出し

		if(z->avail_out == 0)
		{
			ret = _encode_write(p, bufsize);
			if(ret) return ret;

			z->next_out  = p->buf;
			z->avail_out = bufsize;

			p->size += bufsize;
		}
	}

	return MLKERR_OK;
}

/**@ 圧縮処理を完了する
 *
 * @d:すべてのデータを送った後、必ず実行する。
 * @r:エラーコード */

mlkerr mZlibEncFinish(mZlib *p)
{
	z_stream *z = &p->z;
	int ret,bufsize;

	bufsize = p->bufsize;

	//残りデータの圧縮

	while(1)
	{
		ret = deflate(z, Z_FINISH);
		
		if(ret == Z_STREAM_END)
			break;
		else if(ret != Z_OK)
			return MLKERR_ENCODE;

		//書き出し

		if(z->avail_out == 0)
		{
			ret = _encode_write(p, bufsize);
			if(ret) return ret;

			z->next_out  = p->buf;
			z->avail_out = bufsize;

			p->size += bufsize;
		}
	}

	//バッファの残りを書き込み

	bufsize -= z->avail_out;

	if(bufsize)
	{
		ret = _encode_write(p, bufsize);
		if(ret) return ret;

		p->size += bufsize;
	}

	return MLKERR_OK;
}


//===========================
// 展開
//===========================


/**@ 展開用に作成
 *
 * @g:展開
 *
 * @p:bufsize 入力用のバッファサイズ (0 以下で適当なサイズ)
 * @p:windowbits 圧縮時と同じ (default: 15) */

mZlib *mZlibDecNew(int bufsize,int windowbits)
{
	return _init(bufsize, -1, windowbits, 0, 0);
}

/**@ 圧縮データのサイズをセット */

void mZlibDecSetSize(mZlib *p,uint32_t size)
{
	p->size = size;
}

/**@ 展開状態をリセット
 *
 * @d:同じ mZlib で他のデータを展開する時に使う */

void mZlibDecReset(mZlib *p)
{
	inflateReset(&p->z);

	p->size = 0;
}

/**@ すべてのデータを一回で展開
 *
 * @p:buf 出力先
 * @p:bufsize 出力バッファのサイズ
 * @p:insize 圧縮データサイズ
 * @r:エラーコード */

mlkerr mZlibDecReadOnce(mZlib *p,void *buf,int bufsize,uint32_t insize)
{
	z_stream *z = &p->z;
	int ret;

	p->size = insize;

	z->next_out  = (unsigned char *)buf;
	z->avail_out = bufsize;

	while(1)
	{
		//ファイルからバッファへ読み込み

		if(z->avail_in == 0 && p->size)
		{
			ret = _decode_read(p);
			if(ret <= 0) return MLKERR_IO;

			z->next_in  = p->buf;
			z->avail_in = ret;
		}

		//展開

		ret = inflate(z, Z_NO_FLUSH);

		if(ret == Z_STREAM_END)
			break;
		else if(ret != Z_OK)
			return MLKERR_DECODE;
	}

	//入力に残りがあれば読み込み

	while(p->size)
	{
		if(_decode_read(p) == 0)
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

/**@ 指定サイズ分を展開
 *
 * @p:size 取得したいデータサイズ
 * @r:エラーコード */

mlkerr mZlibDecRead(mZlib *p,void *buf,int size)
{
	z_stream *z = &p->z;
	int ret;

	z->next_out  = (unsigned char *)buf;
	z->avail_out = size;

	while(z->avail_out)
	{
		//ファイルからバッファへ読み込み
		// :終端のデータにおいて、avail_in が 0 で、p->size も 0 の場合がある (入力データはすでに内部にある)
		// :その場合、直接 inflate を呼ぶ。

		if(z->avail_in == 0 && p->size)
		{
			ret = _decode_read(p);
			if(ret <= 0) return MLKERR_IO;

			z->next_in  = p->buf;
			z->avail_in = ret;
		}

		//展開

		ret = inflate(z, Z_NO_FLUSH);

		if(ret == Z_STREAM_END)
			break;
		else if(ret != Z_OK)
			return MLKERR_DECODE;
	}

	return MLKERR_OK;
}

/**@ 読み込みを完了する
 *
 * @d:mZlibDecRead() で複数回に分けて読み込んだ場合、最後に実行すること。\
 * 入力データが残っている場合がある。
 * @r:エラーコード */

mlkerr mZlibDecFinish(mZlib *p)
{
	//入力データが残っている場合、すべて読み込む
	/* Z_STREAM_END が来ていない場合、すべてのデータが展開済みでも、
	 * 数バイト残っている場合がある。 */

	while(p->size)
	{
		if(_decode_read(p) <= 0)
			return MLKERR_IO;
	}

	return MLKERR_OK;
}

