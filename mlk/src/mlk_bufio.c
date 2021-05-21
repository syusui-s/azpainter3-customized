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
 * mBufIO : バッファの入出力
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_bufio.h"
#include "mlk_util.h"



/**@ 初期化
 *
 * @p:buf  バッファ
 * @p:bufsize バッファサイズ
 * @p:endian バッファのエンディアン */

void mBufIO_init(mBufIO *p,void *buf,mlksize bufsize,int endian)
{
	p->cur = p->top = (uint8_t *)buf;
	p->bufsize = bufsize;
	p->endian = endian;
}

/**@ 現在位置を取得 */

mlksize mBufIO_getPos(mBufIO *p)
{
	return p->cur - p->top;
}

/**@ 残りサイズ取得 */

mlksize mBufIO_getRemain(mBufIO *p)
{
	return p->bufsize - (p->cur - p->top);
}

/**@ 残りのサイズが指定バイト以上あるか
 *
 * @r:0 で指定バイト以上ある */

int mBufIO_isRemain(mBufIO *p,mlksize size)
{
	return (p->bufsize - (p->cur - p->top) < size);
}

/**@ 指定サイズ移動 (進行方向のみ)
 *
 * @r:0 以外の場合、残りが size 未満で移動できなかった */

int mBufIO_seek(mBufIO *p,int32_t size)
{
	if(mBufIO_isRemain(p, size))
		return 1;
	else
	{
		p->cur += size;
		return 0;
	}
}

/**@ 位置をセット
 *
 * @r:0 以外で、バッファサイズを超えるため、エラー */

int mBufIO_setPos(mBufIO *p,mlksize pos)
{
	if(pos >= p->bufsize)
		return 1;
	else
	{
		p->cur = p->top + pos;
		return 0;
	}
}

/**@ 位置をセット (変更前の位置を取得)
 *
 * @d:plast に変更前の位置をセットした後、位置を変更する。
 * @p:plast 変更前の位置が格納される
 * @r:mBufIO_setPos() と同じ */

int mBufIO_setPos_get(mBufIO *p,mlksize pos,mlksize *plast)
{
	*plast = p->cur - p->top;

	return mBufIO_setPos(p, pos);
}


//=============================
// 読み込み
//=============================


/**@ 読み込み
 *
 * @g:読み込み
 *
 * @r:読み込んだバイト数 */

int32_t mBufIO_read(mBufIO *p,void *buf,int32_t size)
{
	mlksize remain;

	remain = p->bufsize - (p->cur - p->top);

	if(size > remain) size = remain;

	memcpy(buf, p->cur, size);
	p->cur += size;

	return size;
}

/**@ 読み込み
 *
 * @r:0 で、サイズ分をすべて読み込めた。\
 * それ以外で、データが足りない。 */

int mBufIO_readOK(mBufIO *p,void *buf,int32_t size)
{
	return (mBufIO_read(p, buf, size) != size);
}

/**@ 1バイト読み込み
 *
 * @r:0 で成功 */

int mBufIO_readByte(mBufIO *p,void *buf)
{
	return (mBufIO_read(p, buf, 1) != 1);
}

/**@ 16bit 読み込み
 *
 * @r:0 で成功 */

int mBufIO_read16(mBufIO *p,void *buf)
{
	uint8_t *ps;
	uint16_t v;

	if(mBufIO_read(p, buf, 2) != 2) return 1;

	ps = (uint8_t *)buf;

	switch(p->endian)
	{
		case MBUFIO_ENDIAN_LITTLE:
			v = (ps[1] << 8) | ps[0];
			break;
		case MBUFIO_ENDIAN_BIG:
			v = (ps[0] << 8) | ps[1];
			break;
		default:
			v = *((uint16_t *)ps);
			break;
	}

	*((uint16_t *)buf) = v;

	return 0;
}

/**@ 32bit 読み込み */

int mBufIO_read32(mBufIO *p,void *buf)
{
	uint8_t *ps;
	uint32_t v;

	if(mBufIO_read(p, buf, 4) != 4) return 1;

	ps = (uint8_t *)buf;

	switch(p->endian)
	{
		case MBUFIO_ENDIAN_LITTLE:
			v = ((uint32_t)ps[3] << 24) | (ps[2] << 16) | (ps[1] << 8) | ps[0];
			break;
		case MBUFIO_ENDIAN_BIG:
			v = ((uint32_t)ps[0] << 24) | (ps[1] << 16) | (ps[2] << 8) | ps[3];
			break;
		default:
			v = *((uint32_t *)ps);
			break;
	}

	*((uint32_t *)buf) = v;

	return 0;
}

/**@ 16bit 配列読み込み
 *
 * @p:cnt 値の数
 * @r:0 で成功 */

int mBufIO_read16_array(mBufIO *p,void *buf,int cnt)
{
	if(mBufIO_read(p, buf, cnt * 2) != cnt * 2)
		return 1;
	else
	{
	#if !defined(MLK_BIG_ENDIAN)
		mSwapByte_16bit(buf, cnt);
	#endif
	
		return 0;
	}
}

/**@ 32bit 配列読み込み
 *
 * @p:cnt 値の数
 * @r:0 で成功 */

int mBufIO_read32_array(mBufIO *p,void *buf,int cnt)
{
	if(mBufIO_read(p, buf, cnt * 4) != cnt * 4)
		return 1;
	else
	{
	#if !defined(MLK_BIG_ENDIAN)
		mSwapByte_32bit(buf, cnt);
	#endif
	
		return 0;
	}
}

/**@ 16bit 取得
 *
 * @r:バッファが足りない場合は 0 */

uint16_t mBufIO_get16(mBufIO *p)
{
	uint16_t v;

	if(mBufIO_read16(p, &v))
		return 0;
	else
		return v;
}

/**@ 32bit 取得 */

uint32_t mBufIO_get32(mBufIO *p)
{
	uint32_t v;

	if(mBufIO_read32(p, &v))
		return 0;
	else
		return v;
}


//=============================
// 書き込み
//=============================


/**@ 書き込み
 *
 * @g:書き込み
 *
 * @d:バッファが足りない場合は、終端まで書き込み。
 * @r:書き込んだバイト数 */

int32_t mBufIO_write(mBufIO *p,const void *buf,int32_t size)
{
	mlksize remain;

	remain = p->bufsize - (p->cur - p->top);

	if(size > remain) size = remain;

	memcpy(p->cur, buf, size);
	p->cur += size;

	return size;
}

/**@ 16bit 書き込み
 *
 * @r:0 で成功 */

int mBufIO_write16(mBufIO *p,void *buf)
{
	uint8_t b[2];
	uint16_t val = *((uint16_t *)buf);

	switch(p->endian)
	{
		case MBUFIO_ENDIAN_LITTLE:
			b[0] = (uint8_t)val;
			b[1] = (uint8_t)(val >> 8);
			break;
		case MBUFIO_ENDIAN_BIG:
			b[0] = (uint8_t)(val >> 8);
			b[1] = (uint8_t)val;
			break;
		default:
			*((uint16_t *)b) = val;
			break;
	}

	return (mBufIO_write(p, b, 2) != 2);
}

/**@ 32bit 書き込み */

int mBufIO_write32(mBufIO *p,void *buf)
{
	uint8_t b[4];
	uint32_t val = *((uint32_t *)buf);

	switch(p->endian)
	{
		case MBUFIO_ENDIAN_LITTLE:
			b[0] = (uint8_t)val;
			b[1] = (uint8_t)(val >> 8);
			b[2] = (uint8_t)(val >> 16);
			b[3] = (uint8_t)(val >> 24);
			break;
		case MBUFIO_ENDIAN_BIG:
			b[0] = (uint8_t)(val >> 24);
			b[1] = (uint8_t)(val >> 16);
			b[2] = (uint8_t)(val >> 8);
			b[3] = (uint8_t)val;
			break;
		default:
			*((uint32_t *)b) = val;
			break;
	}

	return (mBufIO_write(p, b, 4) != 4);
}

/**@ 16bit 書き込み (数値指定) */

int mBufIO_set16(mBufIO *p,uint16_t val)
{
	return mBufIO_write16(p, &val);
}

/**@ 32bit 書き込み (数値指定) */

int mBufIO_set32(mBufIO *p,uint32_t val)
{
	return mBufIO_write32(p, &val);
}

