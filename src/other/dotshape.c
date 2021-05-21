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

/**********************************
 * ドットペン形状
 **********************************/

#include <string.h>

#include "mlk.h"

#include "dotshape.h"


//------------------

typedef struct
{
	uint8_t *buf;
	int type,		//現在のタイプ
		size,		//現在のサイズ(px)
		size_raw;	//指定時のサイズ
}DotShape;

static DotShape g_data;

#define _GET_BUFSIZE(n)   (((n) * (n) + 7) >> 3)

//------------------

/*
 * 1px = 1bit。
 * x は上位ビットから順。
 * y は x の終端ビットからそのまま続く。
 */



//======================
// 描画
//======================


/* 点を打つ */

static void _set_pixel(int x,int y)
{
	int pos = y * g_data.size + x;
	
	*(g_data.buf + (pos >> 3)) |= 1 << (7 - (pos & 7));
}

/* 水平線描画 */

static void _draw_lineh(int x1,int y,int x2)
{
	for(; x1 <= x2; x1++)
		_set_pixel(x1, y);
}

/* 垂直線描画 */

static void _draw_linev(int x,int y1,int y2)
{
	for(; y1 <= y2; y1++)
		_set_pixel(x, y1);
}

/* 右下 斜線描画 */

static void _draw_slant_down(int x,int y,int num)
{
	for(; num > 0; num--, x++, y++)
		_set_pixel(x, y);
}

/* 右上 斜線描画 */

static void _draw_slant_up(int x,int y,int num)
{
	for(; num > 0; num--, x++, y--)
		_set_pixel(x, y);
}


//======================
// 作成
//======================


/* 円 */

static void _create_circle(int size)
{
	int add,ct,x,y,e;

	add = !(size & 1);
	ct = size / 2 - add;

	x = 0, y = ct;
	e = 3 - 2 * y;

	while(x <= y)
	{
		_draw_lineh(ct - x, ct - y, ct + x + add);
		_draw_lineh(ct - y, ct - x, ct + y + add);
		_draw_lineh(ct - y, ct + x + add, ct + y + add);
		_draw_lineh(ct - x, ct + y + add, ct + x + add);

		if(e < 0)
			e += 4 * x + 6;
		else
		{
			e += 4 * (x - y) + 10;
			y--;
		}

		x++;
	}
}

/* 円枠 */

static void _create_circle_frame(int size)
{
	int add,ct,x,y,e,x1,y1,x2,y2;

	add = !(size & 1);
	ct = size / 2 - add;

	x = 0, y = ct;
	e = 3 - 2 * y;

	while(x <= y)
	{
		x1 = ct - x, x2 = ct + x + add;
		y1 = ct - y, y2 = ct + y + add;
	
		_set_pixel(x1, y1);
		_set_pixel(x2, y1);
		_set_pixel(x1, y2);
		_set_pixel(x2, y2);

		_set_pixel(y1, x1);
		_set_pixel(y2, x1);
		_set_pixel(y1, x2);
		_set_pixel(y2, x2);

		if(e < 0)
			e += 4 * x + 6;
		else
		{
			e += 4 * (x - y) + 10;
			y--;
		}

		x++;
	}
}

/* 四角 */

static void _create_square(int size)
{
	memset(g_data.buf, 255, _GET_BUFSIZE(size));
}

/* 四角枠 */

static void _create_square_frame(int size)
{
	size--;
	
	_draw_lineh(0, 0, size);
	_draw_lineh(0, size, size);
	_draw_linev(0, 0, size);
	_draw_linev(size, 0, size);
}

/* ダイヤ */

static void _create_dia(int size)
{
	int i,n,hf = size >> 1;

	for(i = 0, n = 0; i < size; i++)
	{
		_draw_lineh(hf - n, i, hf + n);

		if(i < hf)
			n++;
		else
			n--;
	}
}

/* ダイヤ枠 */

static void _create_dia_frame(int size)
{
	int hf = size >> 1;
	
	_draw_slant_up(0, hf, hf);
	_draw_slant_down(hf, 0, hf + 1);
	_draw_slant_up(hf, size - 1, hf);
	_draw_slant_down(0, hf, hf);
}

/* バツ */

static void _create_batu(int size)
{
	_draw_slant_up(0, size - 1, size);
	_draw_slant_down(0, 0, size);
}

/* 十字 */

static void _create_cross(int size)
{
	_draw_lineh(0, size >> 1, size - 1);
	_draw_linev(size >> 1, 0, size - 1);
}

/* キラキラ */

static void _create_kirakira(int size)
{
	int w,sp;

	_draw_lineh(0, size >> 1, size - 1);
	_draw_linev(size >> 1, 0, size - 1);

	if(size >= 5)
	{
		w = size * 2 / 3;
		if(!(w & 1)) w++;

		sp = (size - w) >> 1;
	
		_draw_slant_down(sp, sp, w);
		_draw_slant_up(sp, size - 1 - sp, w);
	}
}


//======================
// メイン
//======================


//データ作成関数
static void (*g_create_func[])(int) = {
	_create_circle, _create_circle_frame, _create_square, _create_square_frame,
	_create_dia, _create_dia_frame, _create_batu, _create_cross, _create_kirakira
};



/** 解放 */

void DotShape_free(void)
{
	mFree(g_data.buf);
}

/** 最初の初期化 */

int DotShape_init(void)
{
	g_data.buf = (uint8_t *)mMalloc(_GET_BUFSIZE(DOTSHAPE_MAX_SIZE));
	if(!g_data.buf) return 1;

	//1px データをセット

	*g_data.buf = 0x80;

	g_data.type = 0;
	g_data.size = g_data.size_raw = 1;

	return 0;
}

/** 形状作成 */

void DotShape_create(int type,int size)
{
	DotShape *p = &g_data;
	int size_raw;

	//現在作成されているものと同じか

	if(p->type == type
		&& (size == p->size || size == p->size_raw))
		return;

	//サイズを補正 (ダイヤ以上は奇数にする)

	size_raw = size;

	if(type >= DOTSHAPE_TYPE_DIA && (size & 1) == 0)
		size++;

	//

	p->type = type;
	p->size = size;
	p->size_raw = size_raw;

	//作成

	memset(p->buf, 0, _GET_BUFSIZE(size));

	if(size == 1)
		*p->buf = 0x80;
	else
		(g_create_func[type])(size);
}

/** バッファとサイズを取得 */

int DotShape_getData(uint8_t **ppbuf)
{
	*ppbuf = g_data.buf;

	return g_data.size;
}
