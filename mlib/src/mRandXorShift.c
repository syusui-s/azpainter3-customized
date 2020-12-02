/*$
 Copyright (C) 2013-2020 Azel.

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
 * 乱数生成 - XorShift (128bit)
 *****************************************/

#include <stdint.h>

static uint32_t g_xorshift[4] = {123456789, 362436069, 521288629, 88675123};


/****************//**

@defgroup rand_xorshift mRandXorShift
@brief 乱数生成 (XorShift 128bit)

@ingroup group_etc
@{

@file mRandXorShift.h

****************/


/** 初期化 */

void mRandXorShift_init(uint32_t seed)
{
	int i;

	g_xorshift[0] = 1812433253U * (seed ^ (seed >> 30)) + 1;

	for(i = 0; i <= 2; i++)
		g_xorshift[i + 1] = 1812433253U * (g_xorshift[i] ^ (g_xorshift[i] >> 30)) + (i + 2);
}

/** 32bit 値取得 */

uint32_t mRandXorShift_getUint32(void)
{
	uint32_t t;

	t = g_xorshift[0] ^ (g_xorshift[0] << 11);
	g_xorshift[0] = g_xorshift[1];
	g_xorshift[1] = g_xorshift[2];
	g_xorshift[2] = g_xorshift[3];
	g_xorshift[3] = (g_xorshift[3] ^ (g_xorshift[3] >> 19)) ^ (t ^ (t >> 8));

	return g_xorshift[3];
}

/** 0.0〜1.0 未満の double 値取得 */

double mRandXorShift_getDouble(void)
{
	return mRandXorShift_getUint32() * (1.0 / (UINT32_MAX + 1.0));
}

/** 指定範囲の int 値取得 */

int mRandXorShift_getIntRange(int min,int max)
{
	if(min >= max)
		return min;
	else
		return (int)(mRandXorShift_getDouble() * (max - min + 1)) + min;
}

/* @} */
