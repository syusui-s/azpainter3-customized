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
 * PackBits エンコード/デコード
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_packbits.h"


//=========================
// decode
//=========================
/*
  insize : workbuf 内にある未読込のサイズ
  inpos  : workbuf 先頭からの現在の読み込み位置
*/


/**@ PackBits デコード
 *
 * @d:常に encsize のサイズ分を処理する。\
 * デコード後は、encsize にデコードされたサイズが入る。\
 * デコードサイズが bufsize に達した場合、
 * 残っている入力データはスキップされる。
 * @r:エラーコード */

mlkerr mPackBits_decode(mPackBits *p)
{
	uint8_t *inbuf,*outbuf;
	uint32_t dst_remain,src_remain,insize,inpos,size;
	int ret,len;

	insize = inpos = 0;

	inbuf = p->workbuf;
	outbuf = p->buf;
	dst_remain = p->bufsize;
	src_remain = p->encsize;

	while(src_remain || insize)
	{
		//出力が最大サイズに達した場合、
		//残りの入力をスキップ

		if(dst_remain == 0)
		{
			insize = 0;

			if(src_remain)
			{
				size = p->worksize;
				if(size > src_remain) size = src_remain;

				ret = (p->readwrite)(p, inbuf, size);
				if(ret) return ret;

				src_remain -= size;
			}
			
			continue;
		}
	
		//読み込み
		//(末尾を除き、最小 129 byte)

		if(src_remain && insize < 129)
		{
			//未処理データを先頭に移動

			if(insize)
				memcpy(inbuf, inbuf + inpos, insize);

			inpos = 0;

			//読み込み

			size = p->worksize - insize;
			if(size > src_remain) size = src_remain;

			ret = (p->readwrite)(p, inbuf + insize, size);
			if(ret) return ret;

			insize += size;
			src_remain -= size;
		}

		//長さ (1byte)

		len = inbuf[inpos];
		inpos++;
		insize--;

		//

		if(len >= 128)
		{
			//連続データ

			len = 257 - len;

			if(len > dst_remain) return MLKERR_OVER_DATA;
			if(insize == 0) return MLKERR_NEED_MORE;

			memset(outbuf, inbuf[inpos], len);

			inpos++;
			insize--;
		}
		else
		{
			//非連続データ

			len++;

			if(len > dst_remain) return MLKERR_OVER_DATA;
			if(len > insize) return MLKERR_NEED_MORE;

			memcpy(outbuf, inbuf + inpos, len);

			inpos += len;
			insize -= len;
		}

		outbuf += len;
		dst_remain -= len;
	}

	p->encsize = p->bufsize - dst_remain;

	return MLKERR_OK;
}


//=========================
// encode
//=========================


/* 出力 */

static int _put_encode(mPackBits *p,uint8_t *buf,int size,uint32_t *poutpos)
{
	uint32_t outpos;
	int ret;

	outpos = *poutpos;

	//出力バッファを超える場合、書き込み

	if(outpos + size > p->worksize)
	{
		ret = (p->readwrite)(p, p->workbuf, outpos);
		if(ret) return ret;

		outpos = 0;
	}

	//コピー

	memcpy(p->workbuf + outpos, buf, size);

	//

	*poutpos = outpos + size;

	return MLKERR_OK;
}

/**@ PackBits エンコード
 *
 * @r:エラーコード */

mlkerr mPackBits_encode(mPackBits *p)
{
	uint8_t *ps,*psend,*pp,buf2[2],c;
	uint32_t outpos,encsize;
	int ret,len;

	outpos = encsize = 0;
	
	ps = p->buf;
	psend = p->buf + p->bufsize;

	while(ps < psend)
	{
		if(ps != psend - 1 && *ps == ps[1])
		{
			//---- 2個以上連続

			c = buf2[1] = *ps;

			//連続数 (2〜128)

			for(pp = ps + 2; pp < psend && pp - ps < 128 && *pp == c; pp++);

			len = pp - ps;
			
			buf2[0] = (uint8_t)(-(len - 1));

			ret = _put_encode(p, buf2, 2, &outpos);
			if(ret) return ret;

			encsize += 2;
		}
		else
		{
			//---- 非連続

			//2個連続する位置で終了 (1〜128)
	
			for(pp = ps + 1; pp < psend && pp - ps < 128; pp++)
			{
				if(pp[0] == pp[-1])
				{
					pp--;
					break;
				}
			}

			len = pp - ps;

			//長さ

			buf2[0] = len - 1;

			ret = _put_encode(p, buf2, 1, &outpos);
			if(ret) return ret;

			//データ

			ret = _put_encode(p, ps, len, &outpos);
			if(ret) return ret;

			encsize += 1 + len;
		}

		ps = pp;
	}

	//残りを出力

	if(outpos)
	{
		ret = (p->readwrite)(p, p->workbuf, outpos);
		if(ret) return ret;
	}

	p->encsize = encsize;

	return MLKERR_OK;
}
