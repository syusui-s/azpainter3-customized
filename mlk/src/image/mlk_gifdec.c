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
 * mGIFDec
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_gifdec.h"
#include "mlk_io.h"
#include "mlk_util.h"


//--------------------

struct _mGIFDec
{
	mIO *io;

	uint8_t *palbuf[2],	//パレット [R-G-B-X] (0=global, 1=local)
		*imgbuf,		//最終的な画像のバッファ (8bit)
		*currow;

	int width,			//全体の幅
		height,			//全体の高さ
		bits,			//画像ビット数
		palnum[2],		//各パレット個数
		transparent,	//透過色インデックス (-1 でなし)
		remain_height;	//読み込みの残り行数

	uint8_t bkgnd,			//背景色インデックス
		palette_no,			//現在のイメージのパレット (0:global, 1:local)
		img_destroy_type;	//画像の破棄方法

	/*--- lzw ---*/

	uint8_t *lzwbuf, /* LZW 用バッファ まとめて確保 [block(255) + stack(4096) + suffix(4096) + prefix(4096x2)] */
		*lzw_curbuf;

	int offx,offy,		//画像のオフセット位置
		imgw,imgh,		//読み込み画像サイズ
		curx,cury,		//現在の出力座標
		lzw_leftbits,
		lzw_curbits,
		lzw_blockremain;
	uint8_t lzw_readbyte,
		interlace_pass;	//非インターレースなら0、インターレスなら1にしておく
};

//-------------------

//パレット番号
#define _PALETTENO_GLOBAL  0
#define _PALETTENO_LOCAL   1

//画像の破棄方法
enum
{
	_IMG_DESTROY_TYPE_NONE,
	_IMG_DESTROY_TYPE_STAY,
	_IMG_DESTROY_TYPE_FILL_BKGND,
	_IMG_DESTROY_TYPE_RESTORE
};

//--------------------

static int _decode_lzw(mGIFDec *p);

//--------------------



//===========================
// sub
//===========================


/** パレット読み込み */

static int _read_palette(mGIFDec *p,uint8_t flags,int palno)
{
	int num;
	uint8_t *ps,*pd,r,g,b;

	pd = p->palbuf[palno];

	//個数

	num = 1 << ((flags & 7) + 1);

	p->palnum[palno] = num;

	//読み込み

	if(mIO_readOK(p->io, pd, num * 3))
		return MLKERR_DAMAGED;

	//変換 (RGB => RGBX)

	ps = pd + (num - 1) * 3;
	pd += (num - 1) << 2;

	for(; num > 0; num--, ps -= 3, pd -= 4)
	{
		r = ps[0], g = ps[1], b = ps[2];
		pd[0] = r, pd[1] = g, pd[2] = b, pd[3] = 255;
	}

	return MLKERR_OK;
}

//===========================
// 各ブロック処理
//===========================


/** 画像の情報を読み込み */

static int _read_imageinfo(mGIFDec *p)
{
	uint8_t d[9],flags;
	int ret;

	if(mIO_readOK(p->io, d, 9))
		return MLKERR_DAMAGED;

	flags = d[8];

	//ローカルパレット

	if(!(flags & 0x80))
		p->palette_no = _PALETTENO_GLOBAL;
	else
	{
		ret = _read_palette(p, flags, _PALETTENO_LOCAL);
		if(ret) return ret;

		p->palette_no = _PALETTENO_LOCAL;
	}

	//背景塗りつぶし

	if(p->img_destroy_type == _IMG_DESTROY_TYPE_FILL_BKGND)
	{
		memset(p->imgbuf,
			(p->transparent != -1)? p->transparent: p->bkgnd,
			p->width * p->height);
	}

	//画像情報

	p->offx = mGetBufLE16(d);
	p->offy = mGetBufLE16(d + 2);
	p->imgw = mGetBufLE16(d + 4);
	p->imgh = mGetBufLE16(d + 6);
	p->interlace_pass = ((flags & 0x40) != 0);

	return MLKERR_OK;
}

/** グラフィック制御ブロックを読み込み */

static int _read_graphic_ctrl(mGIFDec *p)
{
	uint8_t d[5];

	if(mIO_readOK(p->io, d, 5) || d[0] < 4)
		return MLKERR_DAMAGED;

	//画像の破棄方法

	p->img_destroy_type = (d[1] >> 2) & 7;

	//透過色

	if(d[1] & 1)
		p->transparent = d[4];

	//余りをスキップ

	if(mIO_readSkip(p->io, d[0] - 4))
		return MLKERR_DAMAGED;

	return MLKERR_OK;
}

/** サブブロックをスキップ */

static int _skip_subblock(mGIFDec *p)
{
	uint8_t size;

	while(1)
	{
		if(mIO_readByte(p->io, &size))
			return MLKERR_DAMAGED;

		if(size == 0) break;

		if(mIO_readSkip(p->io, size))
			return MLKERR_DAMAGED;
	}

	return MLKERR_OK;
}

/** 次の画像ブロックへ移動し、画像情報を読み込み
 *
 * return: エラーコード */

static int _move_nextimage(mGIFDec *p)
{
	uint8_t sig,subsig;
	int ret;

	while(mIO_readByte(p->io, &sig) == 0)
	{
		switch(sig)
		{
			//画像ブロック
			case 0x2c:
				return _read_imageinfo(p);

			//終端
			case 0x3b:
				return -1;

			//サブブロック付きのブロック
			case 0x21:
				//ブロック識別子
				
				if(mIO_readByte(p->io, &subsig))
					return MLKERR_DAMAGED;

				//グラフィック制御ブロック

				if(subsig == 0xf9)
				{
					ret = _read_graphic_ctrl(p);
					if(ret) return ret;
				}

				//次のブロックまでスキップ
				
				ret = _skip_subblock(p);
				if(ret) return ret;
				break;

			//ほか
			default:
				return MLKERR_UNSUPPORTED;
		}
	}

	return -1;
}


//===========================
// main
//===========================


/**@ 閉じる */

void mGIFDec_close(mGIFDec *p)
{
	if(p)
	{
		mIO_close(p->io);

		mFree(p->lzwbuf);
		mFree(p->imgbuf);
		mFree(p->palbuf[0]);
		mFree(p->palbuf[1]);
		
		mFree(p);
	}
}

/**@ GIF デコーダ作成
 *
 * @r:NULL で失敗  */

mGIFDec *mGIFDec_new(void)
{
	mGIFDec *p;

	//確保

	p = (mGIFDec *)mMalloc0(sizeof(mGIFDec));
	if(!p) return NULL;

	//パレットバッファ確保

	p->palbuf[0] = (uint8_t *)mMalloc0(256 * 4);
	p->palbuf[1] = (uint8_t *)mMalloc0(256 * 4);

	if(!p->palbuf[0] || !p->palbuf[1])
	{
		mGIFDec_close(p);
		return NULL;
	}

	//LZW バッファ確保

	p->lzwbuf = (uint8_t *)mMalloc(255 + 4096 * 2 + 4096 * 2);

	if(!p->lzwbuf)
	{
		mGIFDec_close(p);
		return NULL;
	}

	return p;
}

/**@ ファイルを開く */

mlkbool mGIFDec_openFile(mGIFDec *p,const char *filename)
{
	p->io = mIO_openReadFile(filename);
	return (p->io != NULL);
}

/**@ バッファを開く */

mlkbool mGIFDec_openBuf(mGIFDec *p,const void *buf,mlksize bufsize)
{
	p->io = mIO_openReadBuf(buf, bufsize);
	return (p->io != NULL);
}

/**@ FILE * から開く */

mlkbool mGIFDec_openFILEptr(mGIFDec *p,void *fp)
{
	p->io = mIO_openReadFILEptr(fp);
	return (p->io != NULL);
}

/**@ ヘッダを読み込み */

mlkerr mGIFDec_readHeader(mGIFDec *p)
{
	uint8_t d[13],flags;
	int ret;

	if(mIO_readOK(p->io, d, 13))
		return MLKERR_DAMAGED;

	//ヘッダ ("GIF87a" or "GIF89a")

	if(d[0] != 'G' || d[1] != 'I' || d[2] != 'F'
		|| d[3] != '8'
		|| (d[4] != '7' && d[4] != '9')
		|| d[5] != 'a')
		return MLKERR_FORMAT_HEADER;

	//幅・高さ

	p->width  = mGetBufLE16(d + 6);
	p->height = mGetBufLE16(d + 8);

	//フラグ

	flags = d[10];

	//ビット数

	p->bits = ((flags >> 4) & 7) + 1;

	//背景色

	p->bkgnd = d[11];

	//グローバルパレット

	if(flags & 0x80)
	{
		ret = _read_palette(p, flags, _PALETTENO_GLOBAL);
		if(ret) return ret;
	}

	//画像バッファ確保

	p->imgbuf = (uint8_t *)mMalloc(p->width * p->height);
	if(!p->imgbuf) return MLKERR_ALLOC;

	return MLKERR_OK;
}

/**@ 画像の全体サイズを取得
 *
 * @d:ヘッダ読み込み後に行う。 */

void mGIFDec_getImageSize(mGIFDec *p,mSize *size)
{
	size->w = p->width;
	size->h = p->height;
}

/**@ パレットデータを取得
 *
 * @d:イメージ位置移動後に実行する。
 *
 * @p:pnum  パレットの個数が格納される
 * @r:パレットバッファのポインタ (R-G-B-X 順)。\
 * 内部で確保されているポインタが返るため、解放しないこと。\
 * 256 個分のスペースが確保されている。 */

uint8_t *mGIFDec_getPalette(mGIFDec *p,int *pnum)
{
	*pnum = p->palnum[p->palette_no];

	return p->palbuf[p->palette_no];
}

/**@ 現在の画像の透過色をパレットインデックスで取得
 *
 * @d:イメージ位置移動後に実行する。
 * @r:パレットインデックス値。-1 で透過色なし。 */

int mGIFDec_getTransIndex(mGIFDec *p)
{
	if(p->transparent != -1 && p->transparent < p->palnum[p->palette_no])
		return p->transparent;
	else
		return -1;
}

/**@ 現在の画像の透過色を RGB 値で取得
 *
 * @d:イメージ位置移動後に実行する。
 * @r:RGB値。-1 で透過色なし。 */

int32_t mGIFDec_getTransRGB(mGIFDec *p)
{
	uint8_t *ppal;

	if(p->transparent != -1 && p->transparent < p->palnum[p->palette_no])
	{
		ppal = p->palbuf[p->palette_no] + (p->transparent << 2);

		return (ppal[0] << 16) | (ppal[1] << 8) | ppal[2];
	}

	return -1;
}

/**@ 次のイメージの位置へ移動
 *
 * @d:画像の情報も読み込むが、画像自体は読み込まない。
 * @r:エラーコード。-1 でイメージがない。 */

mlkerr mGIFDec_moveNextImage(mGIFDec *p)
{
	p->transparent = -1;
	p->img_destroy_type = _IMG_DESTROY_TYPE_FILL_BKGND;

	//次のイメージへ移動

	return _move_nextimage(p);
}

/**@ イメージを読み込み
 *
 * @d:mGIFDec_moveNextImage() の後に実行する。 */

mlkerr mGIFDec_getImage(mGIFDec *p)
{
	int ret;

	//LZW 展開

	ret = _decode_lzw(p);
	if(ret) return ret;

	//ライン取得準備

	p->currow = p->imgbuf;
	p->remain_height = p->height;

	return MLKERR_OK;
}

/**@ Y1行のイメージを指定フォーマットで取得
 *
 * @d:すでに読み込んでいるイメージから、Y1行のイメージデータを指定形式で取得。\
 * Y は 0 〜 height - 1 まで順番に読み込むことになる。
 * 
 * @p:format 取得するフォーマット
 * @p:trans_to_a0  フォーマットが RGBA で TRUE の場合、透過色を A=0 にする
 * @r:FALSE で、すでにすべて読み込んだ状態 */

mlkbool mGIFDec_getNextLine(mGIFDec *p,void *buf,int format,mlkbool trans_to_a0)
{
	uint8_t *ps,*pd,*pal,*palbuf;
	int i,c,tpcol;

	if(p->remain_height <= 0) return FALSE;

	//

	if(format == MGIFDEC_FORMAT_RAW)
		//生データ
		memcpy(buf, p->currow, p->width);
	else
	{
		//変換

		ps = p->currow;
		pd = (uint8_t *)buf;

		palbuf = p->palbuf[p->palette_no];
		tpcol = p->transparent;

		for(i = p->width; i > 0; i--)
		{
			c = *(ps++);
		
			pal = palbuf + (c << 2);

			pd[0] = pal[0];
			pd[1] = pal[1];
			pd[2] = pal[2];
			pd += 3;

			//A

			if(format == MGIFDEC_FORMAT_RGBA)
				*(pd++) = (trans_to_a0 && c == tpcol)? 0: 255;
		}
	}

	p->currow += p->width;
	p->remain_height--;

	return TRUE;
}


//========================
// LZW 展開
//========================


/** 次のバイトを取得 */

static mlkbool _lzw_nextbyte(mGIFDec *p)
{
	uint8_t size;

	//次のブロックを読み込み

	if(p->lzw_blockremain <= 0)
	{
		if(mIO_readByte(p->io, &size) || size == 0)
			return FALSE;

		if(mIO_readOK(p->io, p->lzwbuf, size))
			return FALSE;

		p->lzw_blockremain = size;
		p->lzw_curbuf = p->lzwbuf;
	}

	//次のバイト

	p->lzw_readbyte = *(p->lzw_curbuf++);

	p->lzw_blockremain--;

	return TRUE;
}

/** 次のコードを取得
 *
 * return: -1 でデータが足りない */

static int _lzw_nextcode(mGIFDec *p)
{
	int ret;

	if(p->lzw_leftbits == 0)
	{
		if(!_lzw_nextbyte(p)) return -1;

		p->lzw_leftbits = 8;
	}

	ret = p->lzw_readbyte >> (8 - p->lzw_leftbits);

	//値が次のバイトをまたぐ場合

	while(p->lzw_curbits > p->lzw_leftbits)
	{
		if(!_lzw_nextbyte(p)) return -1;

		ret |= p->lzw_readbyte << p->lzw_leftbits;

		p->lzw_leftbits += 8;
	}

	p->lzw_leftbits -= p->lzw_curbits;

	return ret & ((1 << p->lzw_curbits) - 1);
}

/** 点をセット
 *
 * return: 1 ですべての点を埋めた */

static int _lzw_setpixel(mGIFDec *p,int c)
{
	int x,y;
	uint8_t pat[5][2] = {{1,0},{8,0},{8,4},{4,2},{2,1}};

	//色を置く

	x = p->offx + p->curx;
	y = p->offy + p->cury;

	if(x >= 0 && x < p->width && y >= 0 && y < p->height)
		*(p->imgbuf + y * p->width + x) = c;

	//次の位置

	p->curx++;

	if(p->curx >= p->imgw)
	{
		p->curx = 0;
		p->cury += pat[p->interlace_pass][0];

		while(p->cury >= p->imgh && p->interlace_pass >= 1 && p->interlace_pass <= 3)
		{
			p->interlace_pass++;
			p->cury = pat[p->interlace_pass][1];
		}

		if(p->cury >= p->imgh) return 1;
	}

	return 0;
}

/** 展開メイン処理 */

static int _lzw_main(mGIFDec *p,int minbits)
{
	uint8_t *sp,*stack,*suffix;
	uint16_t *prefix;
	int c,oc,fc,code;
	int clearcode,endcode,newcode,topslot,slot;

	stack  = p->lzwbuf + 255;
	suffix = p->lzwbuf + 255 + 4096;
	prefix = (uint16_t *)(p->lzwbuf + 255 + 4096 * 2);

	//

	topslot = 1 << p->lzw_curbits;
	clearcode = 1 << minbits;
	endcode = clearcode + 1;
	slot = newcode = endcode + 1;

	//

	sp = stack;
	oc = fc = 0;

	while(1)
	{
		c = _lzw_nextcode(p);
		if(c == -1) return MLKERR_DAMAGED;

		if(c == endcode)
			//終了コード
			break;
		else if(c == clearcode)
		{
			//クリアコード

			p->lzw_curbits = minbits + 1;
			slot = newcode;
			topslot = 1 << p->lzw_curbits;

			while((c = _lzw_nextcode(p)) == clearcode);

			if(c == -1) return MLKERR_DAMAGED;
			if(c == endcode) break;

			if(c >= slot) c = 0;

			oc = fc = c;

			if(_lzw_setpixel(p, c)) break;
		}
		else
		{
			code = c;

			if(code >= slot)
			{
				code = oc;
				*(sp++) = fc;
			}

			while(code >= newcode)
			{
				*(sp++) = suffix[code];
				code = prefix[code];
			}

			*(sp++) = code;

			if(slot < topslot)
			{
				suffix[slot] = fc = code;
				prefix[slot++] = oc;
				oc = c;
			}

			if(slot >= topslot)
			{
				if(p->lzw_curbits < 12)
				{
					topslot <<= 1;
					p->lzw_curbits++;
				}
			}

			while(sp > stack)
			{
				if(_lzw_setpixel(p, *(--sp)))
					return MLKERR_OK;
			}
		}
	}

	return MLKERR_OK;
}

/** LZW 展開開始 */

int _decode_lzw(mGIFDec *p)
{
	int ret;
	uint8_t minbits;

	//LZW 最小ビット長

	if(mIO_readByte(p->io, &minbits)
		|| minbits < 2 || minbits > 9)
		return MLKERR_DAMAGED;

	//処理

	p->lzw_leftbits = 0;
	p->lzw_blockremain = 0;
	p->lzw_curbits = minbits + 1;

	ret = _lzw_main(p, minbits);

	//成功時、ブロック終了のバイトをスキップ

	if(ret == MLKERR_OK)
	{
		if(mIO_readSkip(p->io, 1))
			ret = MLKERR_DAMAGED;
	}

	return ret;
}
