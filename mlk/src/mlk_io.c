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

/************************************
 * mIO : ファイルとバッファの入出力
 ************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mlk.h"
#include "mlk_io.h"
#include "mlk_stdio.h"


//----------------

struct _mIO
{
	FILE *fp;

	uint8_t *bufcur,*buftop;
	mlksize bufsize,remain;

	off_t top_pos;	//先頭位置 (FILE* の場合、開いた時の位置)
	
	int close_fp,  	//終了時に fp を閉じるか (ファイルから開いたか)
		endian;		//エンディアン
};

#define _create_io()   (mIO *)mMalloc0(sizeof(mIO))

//----------------



/**@ 閉じる */

void mIO_close(mIO *p)
{
	if(p)
	{
		if(p->fp && p->close_fp)
			fclose(p->fp);
	
		mFree(p);
	}
}

/**@ 開く (ファイルパスから) */

mIO *mIO_openReadFile(const char *filename)
{
	mIO *p;

	if(!filename) return NULL;

	p = _create_io();
	if(!p) return NULL;

	p->fp = mFILEopen(filename, "rb");
	if(!p->fp)
	{
		mFree(p);
		return NULL;
	}

	p->close_fp = 1;

	return p;
}

/**@ 開く (バッファから) */

mIO *mIO_openReadBuf(const void *buf,mlksize bufsize)
{
	mIO *p;

	p = _create_io();
	if(!p) return NULL;

	p->bufcur = p->buftop = (uint8_t *)buf;
	p->bufsize = p->remain = bufsize;

	return p;
}

/**@ 開く (FILE * から)
 *
 * @d:mIO_close() 時、fp は開いたままとなる。
 * @p:fp  FILE * */

mIO *mIO_openReadFILEptr(void *fp)
{
	mIO *p;

	p = _create_io();
	if(!p) return NULL;
	
	p->fp = (FILE *)fp;
	
	p->top_pos = ftello(p->fp);
	if(p->top_pos == -1) p->top_pos = 0;

	return p;
}

/**@ エンディアンタイプセット
 *
 * @d:設定しなかった場合、デフォルトは HOST。 */

void mIO_setEndian(mIO *p,int type)
{
	p->endian = type;
}

/**@ FILE* を取得 */

void *mIO_getFILE(mIO *p)
{
	return p->fp;
}


//===================


/**@ 現在位置を取得
 *
 * @d:FILE* から開いた場合、開いた時点からの相対的な位置が返る。 */

mlksize mIO_getPos(mIO *p)
{
	if(p->fp)
		return ftello(p->fp) - p->top_pos;
	else
		return p->bufcur - p->buftop;
}

/**@ 先頭から位置を移動
 *
 * @d:FILE* から開いた場合、開いた時点からの相対的な位置を指定する。
 * @r:0 で成功、それ以外で失敗 */

int mIO_seekTop(mIO *p,mlksize pos)
{
	if(p->fp)
	{
		return (fseeko(p->fp, pos + p->top_pos, SEEK_SET) != 0);
	}
	else
	{
		if(pos > p->bufsize) pos = p->bufsize;

		p->bufcur = p->buftop + pos;
		p->remain = p->bufsize - pos;

		return 0;
	}
}

/**@ 現在位置から移動
 *
 * @d:負の移動も可能
 * @r:0 で成功、それ以外で失敗 */

int mIO_seekCur(mIO *p,mlkfoff seek)
{
	if(p->fp)
	{
		return (fseeko(p->fp, seek, SEEK_CUR) != 0);
	}
	else
	{
		mlksize pos;

		pos = p->bufcur - p->buftop;

		if(seek < 0 && pos < -seek)
			//戻る時に位置がマイナスになる場合
			pos = 0;
		else if(seek > 0 && p->remain < seek)
			//進む時に終端を超える場合
			pos += p->remain;
		else
			pos += seek;

		p->bufcur = p->buftop + pos;
		p->remain = p->bufsize - pos;

		return 0;
	}
}


//===================


/**@ 読み込み
 *
 * @g:読み込み
 *
 * @r:読み込んだバイト数 */

int mIO_read(mIO *p,void *buf,int32_t size)
{
	if(p->fp)
		return fread(buf, 1, size, p->fp);
	else
	{
		if(size > p->remain) size = p->remain;

		memcpy(buf, p->bufcur, size);

		p->bufcur += size;
		p->remain -= size;

		return size;
	}
}

/**@ 読み込み
 *
 * @r:0 で指定サイズ分を読み込んだ。それ以外で失敗か足りない */

int mIO_readOK(mIO *p,void *buf,int32_t size)
{
	return (mIO_read(p, buf, size) != size);
}

/**@ 指定サイズ分を読み込んで捨てる
 *
 * @d:パイプでつながった標準入力の場合はシークができないので、
 * スキップ分は読み込む必要がある。\
 * ファイルから開いた場合は、シークで移動する。
 * @r:0 で成功、それ以外で失敗 */

int mIO_readSkip(mIO *p,int32_t size)
{
	uint8_t d[64];
	int n;

	if(size <= 0) return 0;

	if(p->close_fp)
	{
		//シーク

		return (fseek(p->fp, size, SEEK_CUR) != 0);
	}
	else
	{
		//読み込み
		
		while(size > 0)
		{
			n = (size < 64)? size: 64;

			if(mIO_read(p, d, n) != n)
				return 1;

			size -= n;
		}
	}

	return 0;
}

/**@ 1バイト読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mIO_readByte(mIO *p,void *buf)
{
	return (mIO_read(p, buf, 1) != 1);
}

/**@ 16bit 読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mIO_read16(mIO *p,void *buf)
{
	uint8_t d[2];
	uint16_t v;

	if(mIO_read(p, d, 2) != 2)
		return 1;

	switch(p->endian)
	{
		case MIO_ENDIAN_LITTLE:
			v = (d[1] << 8) | d[0];
			break;
		case MIO_ENDIAN_BIG:
			v = (d[0] << 8) | d[1];
			break;
		default:
			v = *((uint16_t *)d);
			break;
	}

	*((uint16_t *)buf) = v;

	return 0;
}

/**@ 32bit 数値読み込み
 *
 * @r:0 で成功、それ以外で失敗 */

int mIO_read32(mIO *p,void *buf)
{
	uint8_t d[4];
	uint32_t v;

	if(mIO_read(p, d, 4) != 4)
		return 1;

	switch(p->endian)
	{
		case MIO_ENDIAN_LITTLE:
			v = ((uint32_t)d[3] << 24) | (d[2] << 16) | (d[1] << 8) | d[0];
			break;
		case MIO_ENDIAN_BIG:
			v = ((uint32_t)d[0] << 24) | (d[1] << 16) | (d[2] << 8) | d[3];
			break;
		default:
			v = *((uint32_t *)d);
			break;
	}

	*((uint32_t *)buf) = v;

	return 0;
}

/**@ 指定フォーマットで複数読み込み
 *
 * @p:format
 * @tbl>
 * |||数字||フォーマットの前に置くと、指定数の配列||
 * |||b||8bit||
 * |||h||16bit||
 * |||i||32bit||
 * |||s||文字列 (最後に NULL 文字が追加される)||
 * |||S||前に数字を指定して、指定バイト分をスキップ||
 * @tbl<
 *
 * @r:0 で成功、それ以外で失敗 */

int mIO_readFormat(mIO *p,const char *format,...)
{
	va_list ap;
	char c;
	int num,ret = 1;
	uint8_t *pd8;
	uint16_t *pd16;
	uint32_t *pd32;

	va_start(ap, format);

	while(1)
	{
		c = *(format++);
		if(!c) break;

		//数字 (配列数)

		num = 0;

		if(c >= '0' && c <= '9')
		{
			do
			{
				num = num * 10 + (c - '0');

				c = *(format++);
				if(!c) goto END;
			} while(c >= '0' && c <= '9');
		}

		if(num <= 0) num = 1;

		//
		
		switch(c)
		{
			//i (32bit)
			case 'i':
				pd32 = va_arg(ap, uint32_t *);

				for(; num; num--, pd32++)
				{
					if(mIO_read32(p, pd32))
						goto ERR;
				}
				break;

			//h (16bit)
			case 'h':
				pd16 = va_arg(ap, uint16_t *);

				for(; num; num--, pd16++)
				{
					if(mIO_read16(p, pd16))
						goto ERR;
				}
				break;
		
			//b,s (8bit)
			case 'b':
			case 's':
				pd8 = va_arg(ap, uint8_t *);

				if(mIO_readOK(p, pd8, num))
					goto ERR;
				
				if(c == 's')
					pd8[num] = 0;
				break;

			//skip
			case 'S':
				if(mIO_readSkip(p, num))
					goto ERR;
				break;
		}
	}

END:
	ret = 0;

ERR:
	va_end(ap);

	return ret;
}
