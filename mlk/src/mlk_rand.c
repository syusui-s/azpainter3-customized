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
 * 乱数生成
 *****************************************/

#include "mlk.h"
#include "mlk_rand.h"
#include "mlk_simd.h"


//===========================
// XorShift (128bit)
//===========================


static mRandXor g_xorshift;


/**@ 初期化
 *
 * @g:XorShift
 * @d:作業用のデータを初期化する
 * 
 * @p:p 作業用で使うデータ。\
 * NULL で、内部の静的変数を使う。\
 * スレッドで使う場合などは、それぞれで用意して指定する。
 * @p:seed 乱数種。time() などの値を使う。 */

void mRandXor_init(mRandXor *p,uint32_t seed)
{
	uint32_t x,y,z,w;

	if(!p) p = &g_xorshift;

	x = 1812433253U * (seed ^ (seed >> 30)) + 1;
	y = 1812433253U * (x ^ (x >> 30)) + 2;
	z = 1812433253U * (y ^ (y >> 30)) + 3;
	w = 1812433253U * (z ^ (z >> 30)) + 4;

	p->x = x;
	p->y = y;
	p->z = z;
	p->w = w;
}

/**@ UINT32 の乱数値を取得 */

uint32_t mRandXor_getUint32(mRandXor *p)
{
	uint32_t t,x,w;

	if(!p) p = &g_xorshift;

	x = p->x;
	w = p->w;

	t = x ^ (x << 11);

	p->x = p->y;
	p->y = p->z;
	p->z = w;

	p->w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));

	return p->w;
}

/**@ 指定範囲の int 値取得 */

int mRandXor_getIntRange(mRandXor *p,int min,int max)
{
	if(!p) p = &g_xorshift;

	if(min >= max)
		return min;
	else
		return (int)(mRandXor_getDouble(p) * (max - min + 1)) + min;
}

/**@ double 値取得
 *
 * @d:0.0〜1.0 未満の double 値取得 */

double mRandXor_getDouble(mRandXor *p)
{
	if(!p) p = &g_xorshift;

	return mRandXor_getUint32(p) * (1.0 / (UINT32_MAX + 1.0));
}


//==============================================================
// SFMT
// # http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/index-jp.html
//==============================================================


#define SFMT_NUM128 156
#define SFMT_NUM32  624

struct _mRandSFMT
{
	uint32_t u[SFMT_NUM32];
	int32_t index;
};


#if MLK_ENABLE_SSE2
//==== SSE2

static void _sfmt_gen_rand(mRandSFMT *p)
{
	int i;
	__m128i *pm,r1,r2,mask,a,v,x,y,z;

	mask = _mm_set_epi32(0xbffffff6, 0xbffaffff, 0xddfecb7f, 0xdfffffef);

	pm = (__m128i *)p->u;
	r1 = pm[SFMT_NUM128 - 2];
	r2 = pm[SFMT_NUM128 - 1];

	for(i = 0; i < SFMT_NUM128 - 122; i++, pm++)
	{
		a = _mm_load_si128(pm);
		y = _mm_load_si128(pm + 122);
		y = _mm_srli_epi32(y, 11);
		z = _mm_srli_si128(r1, 1);
		v = _mm_slli_epi32(r2, 18);
		z = _mm_xor_si128(z, a);
		z = _mm_xor_si128(z, v);
		x = _mm_slli_si128(a, 1);
		y = _mm_and_si128(y, mask);
		z = _mm_xor_si128(z, x);
		z = _mm_xor_si128(z, y);
		_mm_store_si128(pm, z);

		r1 = r2;
		r2 = z;
	}

	for(; i < SFMT_NUM128; i++, pm++)
	{
		a = _mm_load_si128(pm);
		y = _mm_load_si128(pm + 122 - SFMT_NUM128);
		y = _mm_srli_epi32(y, 11);
		z = _mm_srli_si128(r1, 1);
		v = _mm_slli_epi32(r2, 18);
		z = _mm_xor_si128(z, a);
		z = _mm_xor_si128(z, v);
		x = _mm_slli_si128(a, 1);
		y = _mm_and_si128(y, mask);
		z = _mm_xor_si128(z, x);
		z = _mm_xor_si128(z, y);
		_mm_store_si128(pm, z);

		r1 = r2;
		r2 = z;
	}
}

#else
//==== 非 SSE2

/* 128bit 値を左に 8bit シフト */

static void _lshift128(uint32_t *dst,uint32_t *src)
{
	uint64_t th,tl,oh,ol;

	th = ((uint64_t)src[3] << 32) | src[2];
	tl = ((uint64_t)src[1] << 32) | src[0];

	oh = th << 8;
	ol = tl << 8;
	oh |= tl >> (64 - 8);

	dst[0] = (uint32_t)ol;
	dst[1] = (uint32_t)(ol >> 32);
	dst[2] = (uint32_t)oh;
	dst[3] = (uint32_t)(oh >> 32);
}

/* 128bit 値を右に 8bit シフト */

static void _rshift128(uint32_t *dst,uint32_t *src)
{
	uint64_t th,tl,oh,ol;

	th = ((uint64_t)src[3] << 32) | src[2];
	tl = ((uint64_t)src[1] << 32) | src[0];

	oh = th >> 8;
	ol = tl >> 8;
	ol |= th << (64 - 8);

	dst[0] = (uint32_t)ol;
	dst[1] = (uint32_t)(ol >> 32);
	dst[2] = (uint32_t)oh;
	dst[3] = (uint32_t)(oh >> 32);
}

static void _do_recursion(uint32_t *r,uint32_t *b,uint32_t *c,uint32_t *d)
{
	uint32_t x[4],y[4];

	_lshift128(x, r);
	_rshift128(y, c);

	r[0] = r[0] ^ x[0] ^ ((b[0] >> 11) & 0xdfffffefU) ^ y[0] ^ (d[0] << 18);
	r[1] = r[1] ^ x[1] ^ ((b[1] >> 11) & 0xddfecb7fU) ^ y[1] ^ (d[1] << 18);
	r[2] = r[2] ^ x[2] ^ ((b[2] >> 11) & 0xbffaffffU) ^ y[2] ^ (d[2] << 18);
	r[3] = r[3] ^ x[3] ^ ((b[3] >> 11) & 0xbffffff6U) ^ y[3] ^ (d[3] << 18);
}

static void _sfmt_gen_rand(mRandSFMT *p)
{
	int i;
	uint32_t *pu,*pr1,*pr2;

	//128bit (uint32_t * 4) 単位で処理

	pu = p->u;
	pr1 = pu + (SFMT_NUM32 - 8);
	pr2 = pu + (SFMT_NUM32 - 4);

	for(i = 0; i < SFMT_NUM128 - 122; i++)
	{
		_do_recursion(pu, pu + 122 * 4, pr1, pr2);
		pr1 = pr2;
		pr2 = pu;
		pu += 4;
	}

	for(; i < SFMT_NUM128; i++)
	{
		_do_recursion(pu, pu + (122 - SFMT_NUM128) * 4, pr1, pr2);
		pr1 = pr2;
		pr2 = pu;
		pu += 4;
	}
}

#endif

/**@ 作成
 *
 * @g:SFMT
 * 
 * @d:作業用データを確保する。\
 * 確保後は常に初期化を行うこと。
 * 
 * @r:NULL でメモリが足りない */

mRandSFMT *mRandSFMT_new(void)
{
	mRandSFMT *p;

#if MLK_ENABLE_SSE2
	p = (mRandSFMT *)mMallocAlign(sizeof(mRandSFMT), 128/8);
#else
	p = (mRandSFMT *)mMalloc(sizeof(mRandSFMT));
#endif

	return p;
}

/**@ 解放 */

void mRandSFMT_free(mRandSFMT *p)
{
	mFree(p);
}

/**@ 種をセットして初期化 */

void mRandSFMT_init(mRandSFMT *p,uint32_t seed)
{
	int i,j;
	uint32_t *pu,w,parity[4] = {1, 0, 0, 0x13c9e684U};

	p->index = SFMT_NUM32;

	//値の初期値

	pu = p->u;

	*(pu++) = seed;

	for(i = 1; i < SFMT_NUM32; i++, pu++)
		*pu = 1812433253UL * (pu[-1] ^ (pu[-1] >> 30)) + i;

	//

	pu = p->u;
	w = 0;

	for(i = 0; i < 4; i++)
		w ^= pu[i] & parity[i];

	for(i = 16; i; i >>= 1)
		w ^= w >> i;

	if(!(w & 1))
	{
		for(i = 0; i < 4; i++)
		{
			for(j = 0, w = 1; j < 32; j++)
			{
				if(w & parity[i])
				{
					pu[i] ^= w;
					return;
				}

				w = w << 1;
			}
		}
	}
}

/**@ UINT32 の値を取得 */

uint32_t mRandSFMT_getUint32(mRandSFMT *p)
{
	if(p->index >= SFMT_NUM32)
	{
		_sfmt_gen_rand(p);
		p->index = 0;
	}

	return p->u[p->index++];
}

/**@ UINT64 の値を取得 */

uint64_t mRandSFMT_getUint64(mRandSFMT *p)
{
	int index = p->index;

	if(index & 1) index++;

	if(index >= SFMT_NUM32)
	{
		_sfmt_gen_rand(p);
		index = 0;
	}

	p->index = index + 2;

	return ((uint64_t)p->u[index + 1] << 32) | p->u[index];
}

/**@ 指定範囲の int 値を取得 */

int mRandSFMT_getIntRange(mRandSFMT *p,int min,int max)
{
	if(min >= max)
		return min;
	else
		return (int)(mRandSFMT_getDouble(p) * (max - min + 1)) + min;
}

/**@ double 値を取得
 *
 * @d:0.0〜1.0 未満の値を取得 */

double mRandSFMT_getDouble(mRandSFMT *p)
{
	return mRandSFMT_getUint32(p) * (1.0 / (UINT32_MAX + 1.0));
}
