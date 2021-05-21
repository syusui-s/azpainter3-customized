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
 * GIF エンコード
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk.h"
#include "mlk_gifenc.h"
#include "mlk_stdio.h"
#include "mlk_util.h"


//------------------

struct _mGIFEnc
{
	mGIFEncInfo info;
	
	FILE *fp;	
	mlkbool fp_close;

	uint8_t *lzw_out;		//LZW 出力バッファ (1ブロック分。[0]長さ [1..255]データ)
	uint32_t *lzw_hashtbl;	//LZW ハッシュテーブル
	int clearcode,
		endcode,
		runcode,
		runbits,
		depth,
		maxcode,
		shift,
		outpos,		//lzw_out の出力位置 (出力したサイズ)
		curcode;
	uint32_t curval;
};

//------------------

#define LZW_HASHTBL_SIZE      8192
#define LZW_HASHTBL_KEY_MASK  (LZW_HASHTBL_SIZE - 1)

//------------------


//=========================
// LZW
//=========================


/** バイト書き出し */

static mlkbool _lzw_putbyte(mGIFEnc *p,int ch)
{
	//1byte 出力

	p->lzw_out[1 + p->outpos] = ch;
	p->outpos++;

	//ブロックのバッファが一杯なら書き出し

	if(p->outpos == 255)
	{
		p->lzw_out[0] = 255;
		p->outpos = 0;

		if(fwrite(p->lzw_out, 1, 256, p->fp) != 256)
			return FALSE;
	}

	return TRUE;
}

/** コード書き出し */

static mlkbool _lzw_putcode(mGIFEnc *p,int code)
{
	p->curval |= code << p->shift;
	p->shift += p->runbits;

	while(p->shift >= 8)
	{
		if(!_lzw_putbyte(p, p->curval & 0xff))
			return FALSE;

		p->curval >>= 8;
		p->shift -= 8;
	}

	if(p->runcode >= p->maxcode && code <= 4095)
	{
		p->runbits++;
		p->maxcode = 1 << p->runbits;
	}

	return TRUE;
}

/** ハッシュテーブル検索 */

static int _lzw_search_hash(mGIFEnc *p,uint32_t key)
{
	int hkey = ((key >> 12) ^ key) & LZW_HASHTBL_KEY_MASK;
	uint32_t htkey;

	while(1)
	{
		htkey = p->lzw_hashtbl[hkey] >> 12;

		if(htkey == 0xfffff)
			break;
		else if(key == htkey)
			return p->lzw_hashtbl[hkey] & 0xfff;
		else
			hkey = (hkey + 1) & LZW_HASHTBL_KEY_MASK;
	}

	return -1;
}

/** LZW 初期化 */

static mlkbool _lzw_init(mGIFEnc *p)
{
	memset(p->lzw_hashtbl, 0xff, LZW_HASHTBL_SIZE * 4);

	p->depth     = (p->info.bits < 2)? 2: p->info.bits;
	p->clearcode = 1 << p->depth;
	p->endcode   = p->clearcode + 1;
	p->runcode   = p->endcode + 1;
	p->runbits   = p->depth + 1;
	p->maxcode   = 1 << p->runbits;
	p->shift     = 0;
	p->curval    = 0;
	p->outpos    = 0;
	p->curcode   = -1;

	//LZW 最小ビット

	if(mFILEwriteByte(p->fp, p->depth))
		return FALSE;

	return _lzw_putcode(p, p->clearcode);
}

/** LZW 終了 */

static mlkbool _lzw_finish(mGIFEnc *p)
{
	int size;

	if(!_lzw_putcode(p, p->curcode)
		|| !_lzw_putcode(p, p->endcode))
		return FALSE;

	//残りの値を出力
	
	while(p->shift > 0)
	{
		if(!_lzw_putbyte(p, p->curval & 0xff))
			return FALSE;

		p->curval >>= 8;
		p->shift -= 8;
	}

	//残りデータを書き込み

	if(p->outpos)
	{
		p->lzw_out[0] = p->outpos;

		size = p->outpos + 1;
		if(fwrite(p->lzw_out, 1, size, p->fp) != size)
			return FALSE;
	}

	//サブブロック終了

	return (mFILEwriteByte(p->fp, 0) == 0);
}

/** LZW 圧縮 */

static mlkbool _lzw_encode(mGIFEnc *p,uint8_t *buf,int bufsize)
{
	uint8_t *ps,*psend;
	int val,ccode,newcode,hkey;
	uint32_t newkey;

	ps = buf;
	psend = buf + bufsize;

	ccode = p->curcode;
	if(ccode < 0) ccode = *(ps++);

	while(ps < psend)
	{
		val = *(ps++);

		newkey  = ((uint32_t)ccode << p->depth) + val;
		newcode = _lzw_search_hash(p, newkey);

		if(newcode >= 0)
			ccode = newcode;
		else
		{
			if(!_lzw_putcode(p, ccode))
				return FALSE;

			ccode = val;

			if(p->runcode >= 4095)
			{
				if(!_lzw_putcode(p, p->clearcode))
					return FALSE;

				p->runcode = p->endcode + 1;
				p->runbits = p->depth + 1;
				p->maxcode = 1 << p->runbits;

				memset(p->lzw_hashtbl, 0xff, LZW_HASHTBL_SIZE * 4);
			}
			else
			{
				hkey = ((newkey >> 12) ^ newkey) & LZW_HASHTBL_KEY_MASK;

				while((p->lzw_hashtbl[hkey] >> 12) != 0xfffff)
					hkey = (hkey + 1) & LZW_HASHTBL_KEY_MASK;

				p->lzw_hashtbl[hkey] = (newkey << 12) | (p->runcode & 0xfff);
				p->runcode++;
			}
		}
	}

	p->curcode = ccode;

	return TRUE;
}


//=========================
// sub
//=========================


/** パレット書き込み */

static mlkbool _write_palette(mGIFEnc *p,uint8_t *buf,int num,int bits)
{
	int size;

	//パレットデータ

	size = num * 3;

	if(fwrite(buf, 1, size, p->fp) != size)
		return FALSE;

	//余白 (最大数 - 実際のパレット数)

	size = ((1 << bits) - num) * 3;
	
	if(size && mFILEwrite0(p->fp, size))
		return FALSE;

	return TRUE;
}


//=========================
// main
//=========================


/**@ 閉じる */

void mGIFEnc_close(mGIFEnc *p)
{
	if(p)
	{
		if(p->fp)
		{
			if(p->fp_close)
				fclose(p->fp);
			else
				fflush(p->fp);
		}

		mFree(p->lzw_out);
		mFree(p->lzw_hashtbl);
		mFree(p);
	}
}

/**@ GIF エンコーダ作成
 *
 * @p:info  画像全体の情報。\
 * このデータは内部にコピーされるので、実行後はデータを保持する必要はない。\
 * ただし、情報内のポインタ値は、そのまま値としてコピーする。
 * @r:NULL で失敗 */

mGIFEnc *mGIFEnc_new(mGIFEncInfo *info)
{
	mGIFEnc *p;

	//mGIFEnc

	p = (mGIFEnc *)mMalloc0(sizeof(mGIFEnc));
	if(!p) return NULL;

	p->info = *info;

	//LZW 用バッファ

	p->lzw_out = (uint8_t *)mMalloc(256);
	p->lzw_hashtbl = (uint32_t *)mMalloc(LZW_HASHTBL_SIZE * 4);

	if(!p->lzw_out || !p->lzw_hashtbl)
	{
		mGIFEnc_close(p);
		return NULL;
	}

	return p;
}

/**@ 出力先としてファイルを開く */

mlkbool mGIFEnc_openFile(mGIFEnc *p,const char *filename)
{
	p->fp = mFILEopen(filename, "wb");
	p->fp_close = TRUE;

	return (p->fp != NULL);
}

/**@ すでに開いているファイルポインタ (FILE *) を出力先とする */

mlkbool mGIFEnc_openFILEptr(mGIFEnc *p,void *fp)
{
	p->fp = (FILE *)fp;

	return TRUE;
}

/**@ 色数からイメージのビット数を取得
 *
 * @r:1〜8 */

int mGIFEnc_getBitsFromColnum(int num)
{
	int i,bits;

	for(i = 128, bits = 8; bits > 1; i >>= 1, bits--)
	{
		if(num > i) return bits;
	}

	return bits;
}

/**@ ヘッダを書き込み */

mlkbool mGIFEnc_writeHeader(mGIFEnc *p)
{
	mGIFEncInfo *pg = &p->info;
	uint8_t d[13],flags;
	int pal_bits;

	//フラグ

	flags = (pg->bits - 1) << 4;

	if(pg->palette_buf)
	{
		pal_bits = mGIFEnc_getBitsFromColnum(pg->palette_num);
		flags |= 0x80 | (pal_bits - 1);
	}

	//ヘッダ書き込み

	mSetBuf_format(d, "<shhbbb",
		"GIF89a",
		pg->width,
		pg->height,
		flags,
		pg->bkgnd_index,
		0);

	if(fwrite(d, 1, 13, p->fp) != 13)
		return FALSE;

	//グローバルパレット

	if(pg->palette_buf)
	{
		if(!_write_palette(p, pg->palette_buf, pg->palette_num, pal_bits))
			return FALSE;
	}

	return TRUE;
}

/**@ 画像制御の拡張ブロックを書き込む
 *
 * @d:適用したい画像の前に書き込む。
 *
 * @p:transparent 透過色のパレットインデックス。負の値でなし。
 * @p:bkgnd_type  背景の処理タイプ
 * @p:time 遅延時間 (10 ms 単位)
 * @p:user_input ユーザーからの入力を受け付けるか */

mlkbool mGIFEnc_writeGrpCtrl(mGIFEnc *p,
	int transparent,int bkgnd_type,uint16_t time,mlkbool user_input)
{
	uint8_t d[8] = {0x21,0xf9,0x04,0,0,0,0,0};

	//フラグ

	d[3] = (transparent >= 0)
		| ((user_input != 0) << 1)
		| (bkgnd_type << 2);

	//時間 (10ms 単位)

	d[4] = (uint8_t)time;
	d[5] = (uint8_t)(time >> 8);

	//透過色

	if(transparent >= 0)
		d[6] = transparent;

	return (fwrite(d, 1, 8, p->fp) == 8);
}

/**@ ループ用の拡張ブロックを書き込む
 *
 * @p:loopcnt ループ回数。0 で無限 */

mlkbool mGIFEnc_writeLoop(mGIFEnc *p,uint16_t loopcnt)
{
	uint8_t d[19] = {
		0x21,0xff,
		11,'N','E','T','S','C','A','P','E','2','.','0',
		3,1,0,0, 0};

	d[16] = (uint8_t)loopcnt;
	d[17] = (uint8_t)(loopcnt >> 8);

	return (fwrite(d, 1, 19, p->fp) == 19);
}

/**@ GIF の終端を書き込み
 *
 * @d:ファイルの最後に書き込む。 */

mlkbool mGIFEnc_writeEnd(mGIFEnc *p)
{
	return (mFILEwriteByte(p->fp, 0x3b) == 0);
}

/**@ イメージの書き込みの開始 */

mlkbool mGIFEnc_startImage(mGIFEnc *p,mGIFEncImage *img)
{
	uint8_t d[10];
	int pal_bits;

	//----- ヘッダ

	mSetBuf_format(d, "<bhhhh",
		0x2c, img->offx, img->offy, img->width, img->height);

	//フラグ

	if(!img->palette_buf)
		d[9] = 0;
	else
	{
		pal_bits = mGIFEnc_getBitsFromColnum(img->palette_num);
		d[9] = 0x80 | (pal_bits - 1);
	}

	if(fwrite(d, 1, 10, p->fp) != 10)
		return FALSE;

	//ローカルパレット

	if(img->palette_buf)
	{
		if(!_write_palette(p, img->palette_buf, img->palette_num, pal_bits))
			return FALSE;
	}

	//LZW 初期化

	return _lzw_init(p);
}

/**@ イメージを書き込み
 *
 * @d:イメージの先頭からデータを送る。\
 * イメージは 8bit 固定で、パレットインデックスの値。\
 * イメージ全体で width x height byte となる。\
 * 複数回呼び出して、少しずつデータを送ることも可能。 */

mlkbool mGIFEnc_writeImage(mGIFEnc *p,uint8_t *buf,int size)
{
	return _lzw_encode(p, buf, size);
}

/**@ イメージの書き込みを終了
 *
 * @d:すべてのイメージデータを送った後に実行する。 */

mlkbool mGIFEnc_endImage(mGIFEnc *p)
{
	return _lzw_finish(p);
}
