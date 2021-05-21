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
 * イメージ変換
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_imageconv.h"


/**@ RGB[A] 8bit の R と B を入れ替える
 *
 * @d:RGB[A] <-> BGR[A] 変換を行う。
 * @p:bytes 1px のバイト数 (3 or 4) */

void mImageConv_swap_rb_8(uint8_t *buf,uint32_t width,int bytes)
{
	uint8_t a,b;

	for(; width; width--, buf += bytes)
	{
		a = buf[0], b = buf[2];
		buf[0] = b, buf[2] = a;
	}
}

/**@ RGB 8bit から RGBA 8bit への拡張
 *
 * @d:同じバッファ上でアルファ値を追加して拡張する。\
 * アルファ値は 255 となる。 */

void mImageConv_rgb8_to_rgba8_extend(uint8_t *buf,uint32_t width)
{
	uint8_t *ps,*pd,r,g,b;

	ps = buf + (width - 1) * 3;
	pd = buf + ((width - 1) << 2);

	for(; width; width--, ps -= 3, pd -= 4)
	{
		r = ps[0], g = ps[1], b = ps[2];

		pd[0] = r;
		pd[1] = g;
		pd[2] = b;
		pd[3] = 255;
	}
}


//====================================
// RGBX からの変換
//====================================


/**@ RGBX 8bit から RGB 8bit へ変換 */

void mImageConv_rgbx8_to_rgb8(uint8_t *dst,const uint8_t *src,uint32_t width)
{
	for(; width; width--, dst += 3, src += 4)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
	}
}

/**@ RGBX 8bit から GRAY 8bit へ変換
 *
 * @d:R の値を使う。 */

void mImageConv_rgbx8_to_gray8(uint8_t *dst,const uint8_t *src,uint32_t width)
{
	for(; width; width--, src += 4)
		*(dst++) = *src;
}

/**@ RGBX 8bit から GRAY 1bit へ変換
 *
 * @d:ビットは上位から順にセット。\
 * R の最上位ビットが ON かどうかで判定する。 */

void mImageConv_rgbx8_to_gray1(uint8_t *dst,const uint8_t *src,uint32_t width)
{
	uint8_t f = 0x80,val = 0;

	for(; width; width--, src += 4)
	{
		if(*src & 0x80)
			val |= f;

		f >>= 1;
		if(!f)
		{
			*(dst++) = val;
			f = 0x80, val = 0;
		}
	}

	//残り

	if(f != 0x80)
		*dst = val;
}


//====================================
// RGB などへの変換 (主に読み込み用)
//====================================


/**@ グレイスケール 1,2,4,8 bit からの変換
 *
 * @d:8bit GRAY/RGB/RGBA への変換に対応 */

void mImageConv_gray_1_2_4_8(mImageConv *p)
{
	const uint8_t *ps;
	uint8_t *pd;
	int bits,shift,mask,type,c,mul,reverse;
	uint32_t i;

	ps = (const uint8_t *)p->srcbuf;
	pd = (uint8_t *)p->dstbuf;

	type = p->convtype;
	bits = p->srcbits;
	shift = 8 - bits;
	mask = (1 << bits) - 1;
	mul = (bits == 1)? 255: 255 / ((1 << bits) - 1);
	reverse = ((p->flags & MIMAGECONV_FLAGS_REVERSE) != 0);

	//コピー

	if(type == MIMAGECONV_CONVTYPE_NONE
		&& bits == 8 && !reverse)
	{
		memcpy(pd, ps, p->width);
		return;
	}

	//

	for(i = p->width; i; i--)
	{
		c = (*ps >> shift) & mask;

		if(reverse) c = mask - c;

		c *= mul;

		//

		if(type == MIMAGECONV_CONVTYPE_NONE)
			//8bit
			*(pd++) = c;
		else
		{
			//RGB/RGBA
			
			pd[0] = pd[1] = pd[2] = c;

			if(type == MIMAGECONV_CONVTYPE_RGBA)
				pd[3] = 255, pd += 4;
			else
				pd += 3;
		}

		//
	
		shift -= bits;
		if(shift < 0)
		{
			shift = 8 - bits;
			ps++;
		}
	}
}

/**@ グレイスケール 16bit からの変換
 *
 * @d:8/16bit GRAY/RGB/RGBA への変換に対応。\
 * ソースはホストのバイト順。 */

void mImageConv_gray16(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd8;
	uint16_t *pd16,c;
	int type,to8bit,reverse;
	uint32_t i;

	ps   = (const uint16_t *)p->srcbuf;
	pd8  = (uint8_t *)p->dstbuf;
	pd16 = (uint16_t *)pd8;

	type = p->convtype;
	to8bit = (p->dstbits == 8);
	reverse = ((p->flags & MIMAGECONV_FLAGS_REVERSE) != 0);

	//コピー

	if(type == MIMAGECONV_CONVTYPE_NONE
		&& !to8bit && !reverse)
	{
		memcpy(pd8, ps, p->width * 2);
		return;
	}

	//

	for(i = p->width; i; i--)
	{
		c = *(ps++);

		if(reverse) c = 0xffff - c;

		//セット

		if(to8bit)
		{
			//8bit

			c >>= 8;

			if(type == MIMAGECONV_CONVTYPE_NONE)
				*(pd8++) = c;
			else
			{
				pd8[0] = pd8[1] = pd8[2] = c;

				if(type == MIMAGECONV_CONVTYPE_RGBA)
					pd8[3] = 255, pd8 += 4;
				else
					pd8 += 3;
			}
		}
		else
		{
			//16bit
			
			if(type == MIMAGECONV_CONVTYPE_NONE)
				*(pd16++) = c;
			else
			{
				pd16[0] = pd16[1] = pd16[2] = c;

				if(type == MIMAGECONV_CONVTYPE_RGBA)
					pd16[3] = 0xffff, pd16 += 4;
				else
					pd16 += 3;
			}
		}
	}
}

/**@ パレットカラー 1,2,4,8 bit からの変換
 *
 * @d:8bit PAL/RGB/RGBA への変換に対応。 */

void mImageConv_palette_1_2_4_8(mImageConv *p)
{
	const uint8_t *ps,*ppal;
	uint8_t *pd;
	int bits,shift,mask,type,c;
	uint32_t i;

	ps = (const uint8_t *)p->srcbuf;
	pd = (uint8_t *)p->dstbuf;

	type = p->convtype;
	bits = p->srcbits;
	shift = 8 - bits;
	mask = (1 << bits) - 1;

	//コピー

	if(type == MIMAGECONV_CONVTYPE_NONE && bits == 8)
	{
		memcpy(pd, ps, p->width);
		return;
	}

	//

	for(i = p->width; i; i--)
	{
		c = (*ps >> shift) & mask;

		if(type == MIMAGECONV_CONVTYPE_NONE)
			//8bit
			*(pd++) = c;
		else
		{
			//RGB/RGBA
			
			ppal = p->palbuf + (c << 2);

			pd[0] = ppal[0];
			pd[1] = ppal[1];
			pd[2] = ppal[2];

			if(type == MIMAGECONV_CONVTYPE_RGBA)
				pd[3] = ppal[3], pd += 4;
			else
				pd += 3;
		}

		//
	
		shift -= bits;
		if(shift < 0)
		{
			shift = 8 - bits;
			ps++;
		}
	}
}

/**@ RGB 各 5bit (16bit パック) から RGB[A] 8bit への変換
 *
 * @d:SRC_ALPHA が ON で、最上位ビットをアルファ値として扱う。 */

void mImageConv_rgb555(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd;
	uint32_t i,c;
	int r,g,b,a,src_alpha,dst_alpha;

	ps = (const uint16_t *)p->srcbuf;
	pd = (uint8_t *)p->dstbuf;

	src_alpha = ((p->flags & MIMAGECONV_FLAGS_SRC_ALPHA) != 0);

	dst_alpha = (p->convtype == MIMAGECONV_CONVTYPE_RGBA
		|| (p->convtype == MIMAGECONV_CONVTYPE_NONE && src_alpha));

	a = 255;

	for(i = p->width; i; i--)
	{
		c = *(ps++);

		r = ((c >> 10) & 31) << 3;
		g = ((c >> 5) & 31) << 3;
		b = (c & 31) << 3;

		if(r > 255) r = 255;
		if(g > 255) g = 255;
		if(b > 255) b = 255;

		if(src_alpha)
			a = (c & 0x8000)? 255: 0;

		//

		pd[0] = r;
		pd[1] = g;
		pd[2] = b;

		if(dst_alpha)
			pd[3] = a, pd += 4;
		else
			pd += 3;
	}
}

/**@ RGB 8bit から R-G-B-[A] への変換 */

void mImageConv_rgb8(mImageConv *p)
{
	const uint8_t *ps;
	uint8_t *pd;
	int dst_alpha,is_bgr;
	uint32_t i;

	ps = (const uint8_t *)p->srcbuf;
	pd = (uint8_t *)p->dstbuf;

	is_bgr = ((p->flags & MIMAGECONV_FLAGS_SRC_BGRA) != 0);
	dst_alpha = (p->convtype == MIMAGECONV_CONVTYPE_RGBA);

	//コピー

	if(!dst_alpha && !is_bgr)
	{
		memcpy(pd, ps, p->width * 3);
		return;
	}

	//

	for(i = p->width; i; i--)
	{
		if(is_bgr)
		{
			pd[0] = ps[2];
			pd[1] = ps[1];
			pd[2] = ps[0];
		}
		else
		{
			pd[0] = ps[0];
			pd[1] = ps[1];
			pd[2] = ps[2];
		}

		if(dst_alpha)
			pd[3] = 255, pd += 4;
		else
			pd += 3;
		
		ps += 3;
	}
}

/**@ RGBA 8bit から R-G-B-[A] への変換
 *
 * @d:INVALID_ALPHA の場合、src のアルファ値は使われず、
 * 生データでの出力は RGB とする。 */

void mImageConv_rgba8(mImageConv *p)
{
	const uint8_t *ps;
	uint8_t *pd;
	int dst_alpha,is_bgr,invalid_alpha;
	uint32_t i;

	ps = (const uint8_t *)p->srcbuf;
	pd = (uint8_t *)p->dstbuf;

	is_bgr = ((p->flags & MIMAGECONV_FLAGS_SRC_BGRA) != 0);
	invalid_alpha = ((p->flags & MIMAGECONV_FLAGS_INVALID_ALPHA) != 0);

	//出力が RGBA か

	dst_alpha = (p->convtype == MIMAGECONV_CONVTYPE_RGBA
		|| (p->convtype == MIMAGECONV_CONVTYPE_NONE && !invalid_alpha));

	//コピー

	if(dst_alpha && !is_bgr && !invalid_alpha)
	{
		memcpy(pd, ps, p->width * 4);
		return;
	}

	//

	for(i = p->width; i; i--)
	{
		if(is_bgr)
		{
			pd[0] = ps[2];
			pd[1] = ps[1];
			pd[2] = ps[0];
		}
		else
		{
			pd[0] = ps[0];
			pd[1] = ps[1];
			pd[2] = ps[2];
		}

		if(dst_alpha)
		{
			pd[3] = (invalid_alpha)? 255: ps[3];
			pd += 4;
		}
		else
			pd += 3;
		
		ps += 4;
	}
}

/**@ RGB[A] 16bit を R-G-B-[A] 8/16bit に変換
 *
 * @d:SRC_ALPHA が ON で、ソースは RGBA。\
 * ソースはホストのバイト順。 */

void mImageConv_rgb_rgba_16(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd8;
	uint16_t *pd16,r,g,b,a;
	uint32_t i;
	uint8_t dst_alpha,to8bit,src_alpha;

	ps   = (const uint16_t *)p->srcbuf;
	pd8  = (uint8_t *)p->dstbuf;
	pd16 = (uint16_t *)pd8;

	dst_alpha = (p->convtype == MIMAGECONV_CONVTYPE_RGBA);
	src_alpha = ((p->flags & MIMAGECONV_FLAGS_SRC_ALPHA) != 0);
	to8bit = (p->dstbits == 8);

	//

	for(i = p->width; i; i--)
	{
		//16bit src

		r = ps[0];
		g = ps[1];
		b = ps[2];

		if(src_alpha)
			a = ps[3], ps += 4;
		else
			a = 0xffff, ps += 3;
		
		//セット

		if(to8bit)
		{
			//8bit
			
			pd8[0] = r >> 8;
			pd8[1] = g >> 8;
			pd8[2] = b >> 8;

			if(dst_alpha)
				pd8[3] = a >> 8, pd8 += 4;
			else
				pd8 += 3;
		}
		else
		{
			//16bit
			
			pd16[0] = r;
			pd16[1] = g;
			pd16[2] = b;

			if(dst_alpha)
				pd16[3] = a, pd16 += 4;
			else
				pd16 += 3;
		}
	}
}

/**@ CMYK 16bit から 8/16bit への変換
 *
 * @d:ソースは、ホストのバイト順。 */

void mImageConv_cmyk16(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd8;
	uint16_t *pd16,c,m,y,k;
	uint32_t i;
	int to8bit,reverse;

	ps   = (const uint16_t *)p->srcbuf;
	pd8  = (uint8_t *)p->dstbuf;
	pd16 = (uint16_t *)pd8;

	to8bit = (p->dstbits == 8);
	reverse = ((p->flags & MIMAGECONV_FLAGS_REVERSE) != 0);

	//コピー

	if(!to8bit && !reverse)
	{
		memcpy(pd8, ps, p->width * 8);
		return;
	}

	//

	for(i = p->width; i; i--)
	{
		//16bit src

		c = ps[0];
		m = ps[1];
		y = ps[2];
		k = ps[3];
		
		ps += 4;

		//反転

		if(reverse)
		{
			c = 0xffff - c;
			m = 0xffff - m;
			y = 0xffff - y;
			k = 0xffff - k;
		}

		//セット

		if(to8bit)
		{
			//8bit
			
			pd8[0] = c >> 8;
			pd8[1] = m >> 8;
			pd8[2] = y >> 8;
			pd8[3] = k >> 8;

			pd8 += 4;
		}
		else
		{
			//16bit
			
			pd16[0] = c;
			pd16[1] = m;
			pd16[2] = y;
			pd16[3] = k;

			pd16 += 4;
		}
	}
}


//===============================
// チャンネル分離のデータから
//===============================


/**@ GRAY+A 8bit のチャンネル分離データから GRAY+A/RGB/RGBA へ
 *
 * @g:チャンネル分離データ
 *
 * @d:chno == 1 のソースがアルファ値となる */

void mImageConv_sepch_gray_a_8(mImageConv *p)
{
	uint8_t *pd,c;
	const uint8_t *ps;
	uint32_t i;

	pd = (uint8_t *)p->dstbuf;
	ps = (const uint8_t *)p->srcbuf;
	i = p->width;

	//

	if(p->convtype == MIMAGECONV_CONVTYPE_NONE)
	{
		//GRAY+A

		pd += p->chno;

		for(; i; i--, pd += 2)
			*pd = *(ps++);
	}
	else if(p->convtype == MIMAGECONV_CONVTYPE_RGB)
	{
		//RGB

		if(p->chno != 0) return;

		for(; i; i--, pd += 3)
		{
			c = *(ps++);
			pd[0] = pd[1] = pd[2] = c;
		}
	}
	else
	{
		//RGBA

		if(p->chno == 0)
		{
			for(; i; i--, pd += 4)
			{
				c = *(ps++);
				pd[0] = pd[1] = pd[2] = c;
			}
		}
		else
		{
			//Alpha

			for(pd += 3; i; i--, pd += 4)
				*pd = *(ps++);
		}
	}
}

/**@ GRAY+A 16bit のチャンネル分離データから GRAY+A/RGB/RGBA:8/16bit へ
 *
 * @d:chno == 1 のソースがアルファ値となる。\
 * ソースのエンディアンはホスト順。 */

void mImageConv_sepch_gray_a_16(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd8;
	uint16_t *pd16,c;
	uint32_t i;
	int type,chno,add,to8bit;

	//RGB 変換時、アルファ値は必要ない

	if(p->convtype == MIMAGECONV_CONVTYPE_RGB && p->chno == 1)
		return;

	//

	ps = (const uint16_t *)p->srcbuf;
	pd8 = (uint8_t *)p->dstbuf;
	pd16 = (uint16_t *)pd8;

	type = p->convtype;
	chno = p->chno;
	to8bit = (p->dstbits == 8);

	if(type == MIMAGECONV_CONVTYPE_NONE)
		pd16 += chno, pd8 += chno, add = 2;
	else
		add = (type == MIMAGECONV_CONVTYPE_RGB)? 3: 4;

	//

	for(i = p->width; i; i--)
	{
		c = *(ps++);

		if(to8bit)
		{
			//8bit

			c >>= 8;

			if(type == MIMAGECONV_CONVTYPE_NONE)
				*pd8 = c;
			else
			{
				//RGB[A]

				if(chno == 1)
					pd8[3] = c;
				else
					pd8[0] = pd8[1] = pd8[2] = c;
			}

			pd8 += add;
		}
		else
		{
			//16bit

			if(type == MIMAGECONV_CONVTYPE_NONE)
				*pd16 = c;
			else
			{
				//RGB[A]

				if(chno == 1)
					pd16[3] = c;
				else
					pd16[0] = pd16[1] = pd16[2] = c;
			}

			pd16 += add;
		}
	}
}

/**@ RGB[A] 8bit のチャンネル分離データから RGB/RGBA へ
 *
 * @d:SRC_ALPHA が ON で、変換元にアルファ値がある。\
 * chno は、0=R, 1=G, 2=B, 3=A。 */

void mImageConv_sepch_rgb_rgba_8(mImageConv *p)
{
	uint8_t *pd;
	const uint8_t *ps;
	uint32_t i;
	int type;

	pd = (uint8_t *)p->dstbuf;
	ps = (const uint8_t *)p->srcbuf;
	i = p->width;
	type = p->convtype;

	//変換なしの場合

	if(type == MIMAGECONV_CONVTYPE_NONE)
	{
		type = (p->flags & MIMAGECONV_FLAGS_SRC_ALPHA)?
			MIMAGECONV_CONVTYPE_RGBA: MIMAGECONV_CONVTYPE_RGB;
	}

	//

	if(type == MIMAGECONV_CONVTYPE_RGB)
	{
		//RGB

		if(p->chno == 3) return;

		for(pd += p->chno; i; i--, pd += 3)
			*pd = *(ps++);
	}
	else
	{
		//RGBA

		for(pd += p->chno; i; i--, pd += 4)
			*pd = *(ps++);

		//元がアルファなしの場合、chno == 0 時にアルファ値をセット

		if(p->chno == 0 && !(p->flags & MIMAGECONV_FLAGS_SRC_ALPHA))
		{
			pd = (uint8_t *)p->dstbuf + 3;
			i = p->width;
			
			for(; i; i--, pd += 4)
				*pd = 255;
		}
	}
}

/**@ RGB[A] 16bit のチャンネル分離データから RGB/RGBA:8/16bit へ
 *
 * @d:SRC_ALPHA が ON で、変換元にアルファ値がある。\
 * ソースのエンディアンはホスト順。\
 * chno は、0=R, 1=G, 2=B, 3=A。 */

void mImageConv_sepch_rgb_rgba_16(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd8;
	uint16_t *pd16,c;
	uint32_t i;
	int type,to8bit,add;

	type = p->convtype;

	//変換なしの場合

	if(type == MIMAGECONV_CONVTYPE_NONE)
	{
		type = (p->flags & MIMAGECONV_FLAGS_SRC_ALPHA)?
			MIMAGECONV_CONVTYPE_RGBA: MIMAGECONV_CONVTYPE_RGB;
	}

	//RGB 時、アルファ値は必要ない

	if(type == MIMAGECONV_CONVTYPE_RGB && p->chno == 3)
		return;

	//

	ps = (const uint16_t *)p->srcbuf;
	pd8 = (uint8_t *)p->dstbuf + p->chno;
	pd16 = (uint16_t *)p->dstbuf + p->chno;

	to8bit = (p->dstbits == 8);
	add = (type == MIMAGECONV_CONVTYPE_RGB)? 3: 4;

	//

	for(i = p->width; i; i--)
	{
		c = *(ps++);

		if(to8bit)
		{
			//8bit

			*pd8 = c >> 8;
			pd8 += add;
		}
		else
		{
			//16bit

			*pd16 = c;
			pd16 += add;
		}
	}

	//元が RGB で RGBA 変換の場合、chno == 0 時にアルファ値をセット

	if(p->chno == 0
		&& !(p->flags & MIMAGECONV_FLAGS_SRC_ALPHA)
		&& type == MIMAGECONV_CONVTYPE_RGBA)
	{
		i = p->width;

		if(to8bit)
		{
			pd8 = (uint8_t *)p->dstbuf + 3;

			for(; i; i--, pd8 += 4)
				*pd8 = 255;
		}
		else
		{
			pd16 = (uint16_t *)p->dstbuf + 3;

			for(; i; i--, pd16 += 4)
				*pd16 = 0xffff;
		}
	}
}

/**@ CMYK 8bit のチャンネル分離データから CMYK へ
 *
 * @d:chno は、0=C, 1=M, 2=Y, 3=K。 */

void mImageConv_sepch_cmyk8(mImageConv *p)
{
	const uint8_t *ps;
	uint8_t *pd,c;
	uint32_t i,reverse;

	pd = (uint8_t *)p->dstbuf + p->chno;
	ps = (const uint8_t *)p->srcbuf;
	reverse = ((p->flags & MIMAGECONV_FLAGS_REVERSE) != 0);

	for(i = p->width; i; i--, pd += 4)
	{
		c = *(ps++);
		if(reverse) c = 255 - c;
		
		*pd = c;
	}
}

/**@ CMYK 16bit のチャンネル分離データから CMYK 8/16bit へ
 *
 * @d:chno は、0=C, 1=M, 2=Y, 3=K。\
 * ソースのエンディアンはホスト順。 */

void mImageConv_sepch_cmyk16(mImageConv *p)
{
	const uint16_t *ps;
	uint8_t *pd8;
	uint16_t *pd16,c;
	uint32_t i,reverse;

	ps = (const uint16_t *)p->srcbuf;
	i = p->width;
	reverse = ((p->flags & MIMAGECONV_FLAGS_REVERSE) != 0);

	if(p->dstbits == 8)
	{
		//8bit
		
		pd8 = (uint8_t *)p->dstbuf + p->chno;

		for(; i; i--, pd8 += 4)
		{
			c = *(ps++);
			if(reverse) c = 0xffff - c;

			*pd8 = c >> 8;
		}
	}
	else
	{
		//16bit
		
		pd16 = (uint16_t *)p->dstbuf + p->chno;

		for(; i; i--, pd16 += 4)
		{
			c = *(ps++);
			if(reverse) c = 0xffff - c;

			*pd16 = c;
		}
	}
}

