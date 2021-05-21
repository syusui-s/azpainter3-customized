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

/****************************
 * テーブルデータ
 ****************************/

#include <stdlib.h>
#include <math.h>

#include "mlk.h"

#include "table_data.h"


//------------------

TableData *g_app_tabledata = NULL;

typedef struct
{
	int index;
	double val;
}_thval;

//------------------


/** 解放 */

void TableData_free(void)
{
	TableData *p = g_app_tabledata;

	if(p)
	{
		mFree(p->ptonetbl);

		mFree(p);

		g_app_tabledata = NULL;
	}
}

/** 初期化 */

void TableData_init(void)
{
	TableData *p;
	double *pd;
	int i;

	p = g_app_tabledata = (TableData *)mMalloc0(sizeof(TableData));

	//sin テーブル

	pd = p->sintbl;

	for(i = 0; i < 512; i++)
		*(pd++) = sin(i / 256.0 * MLK_MATH_PI);

	//トーン化テーブルバッファ

	p->ptonetbl = (uint8_t *)mMalloc(TABLEDATA_TONE_WIDTH * TABLEDATA_TONE_WIDTH * 2);
}

/** レイヤトーン化用テーブル作成 */

void TableData_setToneTable(int bits)
{
	if(g_app_tabledata->tone_bits == bits) return;

	ToneTable_setData(g_app_tabledata->ptonetbl, TABLEDATA_TONE_WIDTH, bits);

	g_app_tabledata->tone_bits = bits;
}


//=====================


/* _thval ソート関数 */

static int _tone_sort(const void *va,const void *vb)
{
	const _thval *a = (_thval *)va;
	const _thval *b = (_thval *)vb;

	if(a->val < b->val)
		return -1;
	else if(a->val > b->val)
		return 1;
	else
		return 0;
}

/** トーンテーブルのデータセット */

void ToneTable_setData(uint8_t *tblbuf,int width,int bits)
{
	_thval *thbuf,*pth;
	uint8_t *pd8;
	uint16_t *pd16;
	int ww,ix,iy,index;
	double dx,dy,div,val;

	ww = width * width;

	//作業用バッファ

	thbuf = (_thval *)mMalloc(sizeof(_thval) * ww);
	if(!thbuf) return;

	//

	pth = thbuf;
	index = 0;
	div = 1.0 / (width - 1);

	for(iy = 0; iy < width; iy++)
	{
		//中心からの距離 (-1〜1) の絶対値
		dy = fabs((iy * div - 0.5) * 2);
	
		for(ix = 0; ix < width; ix++)
		{
			dx = fabs((ix * div - 0.5) * 2);

			//val = 中心が 1.0、縁が 0、外側が負

			if(dx + dy > 1.0)
				//外側はひし形
				val = ((dy - 1) * (dy - 1) + (dx - 1) * (dx - 1)) - 1;
			else
				//内側は円
				val = 1.0 - (dx * dx + dy * dy);
			
			if(val > 1)
				val = 1;
			else if(val < -1)
				val = -1;

			//

			pth->index = index++;
			pth->val = val;
			pth++;
		}
	}

	//val の小さい順にソート

	qsort(thbuf, ww, sizeof(_thval), _tone_sort);

	//しきい値バッファをセット

	if(bits == 8)
	{
		pd8 = tblbuf;
		
		for(ix = 0; ix < ww; ix++)
			pd8[thbuf[ix].index] = ix * 255 / ww;
	}
	else
	{
		pd16 = (uint16_t *)tblbuf;
		
		for(ix = 0; ix < ww; ix++)
			pd16[thbuf[ix].index] = ((int64_t)ix << 15) / ww;
	}

	//

	mFree(thbuf);
}

