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
 * ブラシサイズリストデータ
 *****************************************/

#include <stdlib.h>
#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_buf.h"
#include "mlk_str.h"
#include "mlk_stdio.h"


/* - 1つのデータは 2byte。
 * - 値の小さい順に並んでいる。
 * - 変更されたときのみ保存。 */

//-----------------

#define _MAXNUM      500
#define _EXPAND_SIZE (32 * 2)
#define _SIZE_MIN    1
#define _SIZE_MAX    9999  //4桁まで

#define _CONFIG_FILENAME  "brushsize.dat"

//-----------------



/** 個数を取得 */

int BrushSizeList_getNum(mBuf *p)
{
	return p->cursize / 2;
}

/** 指定位置のデータを取得 */

int BrushSizeList_getValue(mBuf *p,int pos)
{
	return *((uint16_t *)p->buf + pos);
}

/** 文字列からデータ追加
 *
 * 数字と '.' 以外の文字で区切って複数。 */

void BrushSizeList_addFromText(mBuf *p,char *text)
{
	char *top,*end;
	uint16_t *pbuf;
	int val,num,pos,i,n;

	for(top = text; *top; top = end)
	{
		//先頭位置 (数字) を検索

		for(; *top < '0' || *top > '9'; top++);

		//サイズ値

		val = (int)(strtod(top, &end) * 10 + 0.5);

		if(val < _SIZE_MIN)
			val = _SIZE_MIN;
		else if(val > _SIZE_MAX)
			val = _SIZE_MAX;

		//挿入位置
		// :値の小さい順。同じ値の場合は追加しない。

		pbuf = (uint16_t *)p->buf;
		num = pos = p->cursize / 2;

		for(i = 0; i < num; i++)
		{
			n = *(pbuf++);

			//同じ値がある場合
			if(n == val) { pos = -1; break; }

			//挿入位置
			if(n > val) { pos = i; break; }
		}

		if(pos == -1) continue;

		if(num >= _MAXNUM) return;

		//バッファを新規確保

		if(!p->buf)
		{
			if(!mBufAlloc(p, 32 * 2, _EXPAND_SIZE))
				return;
		}

		//追加 (データ領域を確保)
		
		if(!mBufAppend(p, &val, 2))
			return;

		//挿入位置を空ける

		pbuf = (uint16_t *)p->buf + num;

		for(i = num - pos; i > 0; i--, pbuf--)
			*pbuf = pbuf[-1];

		//セット

		*pbuf = val;
	}
}

/** 指定位置のデータを削除 */

void BrushSizeList_deleteAtPos(mBuf *p,int pos)
{
	uint16_t *pd;
	int i;

	pd = (uint16_t *)p->buf + pos;

	for(i = p->cursize / 2 - 1 - pos; i > 0; i--, pd++)
		*pd = pd[1];

	mBufBack(p, 2);
}


//===============================
// ファイル
//===============================


/* ファイル開く */

static FILE *_open_file(char *mode)
{
	mStr str = MSTR_INIT;
	FILE *fp;

	mGuiGetPath_config(&str, _CONFIG_FILENAME);

	fp = mFILEopen(str.buf, mode);

	mStrFree(&str);

	return fp;
}

/** 読み込み */

void BrushSizeList_loadConfigFile(mBuf *p)
{
	FILE *fp;
	uint8_t ver;
	uint16_t num;

	fp = _open_file("rb");
	if(!fp) return;

	if(mFILEreadStr_compare(fp, "AZPTBRSIZE")
		|| mFILEreadByte(fp, &ver)
		|| ver != 0
		|| mFILEreadBE16(fp, &num))
		goto END;

	if(num)
	{
		if(num > _MAXNUM) num = _MAXNUM;
		
		//確保

		if(!mBufAlloc(p, num * 2, _EXPAND_SIZE))
			goto END;

		//読み込み

		num = mFILEreadArrayBE16(fp, p->buf, num);

		mBufSetCurrent(p, num * 2);
	}

END:
	fclose(fp);
}

/** 書き込み */

void BrushSizeList_saveConfigFile(mBuf *p)
{
	FILE *fp;
	int num;

	fp = _open_file("wb");
	if(!fp) return;

	num = p->cursize / 2;

	fputs("AZPTBRSIZE", fp);

	mFILEwriteByte(fp, 0);

	mFILEwriteBE16(fp, num);

	mFILEwriteArrayBE16(fp, p->buf, num);

	fclose(fp);
}

