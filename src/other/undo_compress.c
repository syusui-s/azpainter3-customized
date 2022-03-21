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

/********************************
 * アンドゥ用圧縮/展開
 ********************************/

#include <stdint.h>
#include <string.h>


/** 圧縮 (8bit)
 *
 * return: 圧縮後のサイズ (-1 で、元サイズより大きくなった) */

int undo_encode8(uint8_t *dst,uint8_t *src,int size)
{
	uint8_t *p1,*pend;
	int mode = 0,len,n,dstsize = 0;

	pend = src + size;

	while(src != pend)
	{
		if(mode == 0)
		{
			//非連続

			for(p1 = src + 1; p1 != pend && *p1 != p1[-1]; p1++);

			len = p1 - src;

			while(len >= 255)
			{
				dstsize += 256;
				if(dstsize >= size) return -1;
			
				*(dst++) = 255;
				memcpy(dst, src, 255);

				len -= 255;
				src += 255;
				dst += 255;
			}

			dstsize += 1 + len;
			if(dstsize >= size) return -1;

			*(dst++) = len;
			memcpy(dst, src, len);
			dst += len;
		}
		else
		{
			//連続

			for(p1 = src + 1; p1 != pend && *p1 == p1[-1]; p1++);

			len = p1 - src;

			if(len < 255)
			{
				dstsize++;
				if(dstsize >= size) return -1;
			
				*(dst++) = len;
			}
			else
			{
				//255 以上の場合
				
				n = len / 255;

				dstsize += n + 1;
				if(dstsize >= size) return -1;
				
				memset(dst, 255, n);
				dst += n;

				*(dst++) = len - n * 255;
			}
		}

		src = p1;
		mode = !mode;
	}

	return dstsize;
}

/** 展開 (8bit) */

void undo_decode8(uint8_t *dst,uint8_t *src,int size)
{
	uint8_t *pend,last = 0;
	int mode = 0,len;

	pend = src + size;

	while(src != pend)
	{
		len = *(src++);

		if(len)
		{
			if(mode == 0)
			{
				//非連続

				memcpy(dst, src, len);
				src += len;
				dst += len;

				last = src[-1];
			}
			else
			{
				//連続

				memset(dst, last, len);
				dst += len;
			}
		}

		if(len != 255)
			mode = !mode;
	}
}


//====================


/** 圧縮 (16bit)
 *
 * return: 圧縮後のサイズ (-1 で、元サイズより大きくなった) */

int undo_encode16(uint8_t *dst,uint8_t *src,int size)
{
	uint16_t *ps,*p1,*pend;
	int mode = 0,len,n,dstsize = 0;

	ps = (uint16_t *)src;
	pend = (uint16_t *)(src + size);

	while(ps != pend)
	{
		if(mode == 0)
		{
			//非連続

			for(p1 = ps + 1; p1 != pend && *p1 != p1[-1]; p1++);

			len = p1 - ps;

			while(len >= 255)
			{
				dstsize += 1 + 255 * 2;
				if(dstsize >= size) return -1;
			
				*(dst++) = 255;
				memcpy(dst, ps, 255 * 2);

				len -= 255;
				ps += 255;
				dst += 255 * 2;
			}

			//

			dstsize += 1 + len * 2;
			if(dstsize >= size) return -1;

			*(dst++) = len;
			memcpy(dst, ps, len * 2);
			dst += len * 2;
		}
		else
		{
			//連続

			for(p1 = ps + 1; p1 != pend && *p1 == p1[-1]; p1++);

			len = p1 - ps;

			if(len < 255)
			{
				dstsize++;
				if(dstsize >= size) return -1;
			
				*(dst++) = len;
			}
			else
			{
				//255 以上の場合
				
				n = len / 255;

				dstsize += n + 1;
				if(dstsize >= size) return -1;
				
				memset(dst, 255, n);
				dst += n;

				*(dst++) = len - n * 255;
			}
		}

		ps = p1;
		mode = !mode;
	}

	return dstsize;
}

/** 展開 (16bit) */

void undo_decode16(uint8_t *dst,uint8_t *src,int size)
{
	uint8_t *pend;
	uint16_t *pd,last = 0;
	int mode = 0,len,i;

	pd = (uint16_t *)dst;
	pend = src + size;

	while(src != pend)
	{
		len = *(src++);

		if(len)
		{
			if(mode == 0)
			{
				//非連続

				memcpy(pd, src, len * 2);
				src += len * 2;
				pd += len;

				last = pd[-1];
			}
			else
			{
				//連続

				for(i = len; i; i--)
					*(pd++) = last;
			}
		}

		if(len != 255)
			mode = !mode;
	}
}

