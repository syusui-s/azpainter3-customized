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
 * PNG 読み込み
 *****************************************/

#include <png.h>
//[!] setjmp.h はインクルードしない。コンパイルエラーが出る場合がある

#include "mlk.h"
#include "mlk_loadimage.h"
#include "mlk_io.h"
#include "mlk_util.h"


//--------------------

typedef struct
{
	mIO *io;

	png_struct *png;
	png_info *pnginfo;

	mLoadImage *pli;
	uint8_t *palbuf;
	int palnum,
		prog_cur,
		fconvpal;	//透過付きパレット->RGBA 変換を行う
}pngdata;

//--------------------



//=======================
// PNG 関数
//=======================


/* PNG エラー関数 */

static void _png_error_func(png_struct *png,png_const_charp mes)
{
	mLoadImage *pli;

	pli = (mLoadImage *)png_get_error_ptr(png);

	pli->errmessage = mStrdup(mes);

	longjmp(png_jmpbuf(png), 1);
}

/* PNG 警告関数 */

static void _png_warn_func(png_struct *png,png_const_charp mes)
{

}

/* 読み込みコールバック */

static void _png_read_func(png_struct *png,png_byte *data,png_size_t size)
{
	pngdata *p = (pngdata *)png_get_io_ptr(png);

	if(mIO_readOK(p->io, data, size))
		png_error(png, "I/O error");
}

/* 進捗用
 *
 * row: 非インターレースの場合、1〜高さ。インターレースの場合、不規則な値。
 * pass: インターレースの場合、0〜7 が順番に来る */

static void _png_read_row_callback(png_struct *png,png_uint_32 row,int pass)
{
	pngdata *p = (pngdata *)png_get_io_ptr(png);
	int n;

	if(png_get_interlace_type(png, p->pnginfo) == PNG_INTERLACE_ADAM7)
		n = pass * 100 / 7;
	else
		n = row * 100 / p->pli->height;

	if(n != p->prog_cur)
	{
		p->prog_cur = n;
		(p->pli->progress)(p->pli, n);
	}
}


//=======================
// 情報読み込み
//=======================


/* ヘッダ処理 */

static int _proc_header(pngdata *p,mLoadImage *pli)
{
	uint8_t d[8];

	//ヘッダ判定

	if(mIO_readOK(p->io, d, 8) || png_sig_cmp(d, 0, 8))
		return MLKERR_FORMAT_HEADER;

	png_set_sig_bytes(p->png, 8);

	return MLKERR_OK;
}

/* 解像度取得 */

static void _get_resolution(pngdata *p,mLoadImage *pli)
{
	png_uint_32 h,v;
	int type;

	pli->reso_unit = MLOADIMAGE_RESOUNIT_NONE;

	if(png_get_pHYs(p->png, p->pnginfo, &h, &v, &type))
	{
		if(type == PNG_RESOLUTION_METER)
			pli->reso_unit = MLOADIMAGE_RESOUNIT_DPM;
		else
			pli->reso_unit = MLOADIMAGE_RESOUNIT_UNKNOWN;

		pli->reso_horz = h;
		pli->reso_vert = v;
	}
}

/* パレット読み込み */

static int _read_palette(pngdata *p,mLoadImage *pli)
{
	uint8_t *ps,*pd;
	int num,i;

	if(!png_get_PLTE(p->png, p->pnginfo, (png_color **)&ps, &num))
		return MLKERR_UNKNOWN;

	//確保

	if(num > 256) num = 256;

	pd = p->palbuf = (uint8_t *)mMalloc0(256 * 4);
	if(!pd) return MLKERR_ALLOC;

	p->palnum = num;

	//RGB => RGBA

	for(i = 256; i > 0; i--)
	{
		pd[0] = ps[0];
		pd[1] = ps[1];
		pd[2] = ps[2];
		pd[3] = 255;

		ps += 3;
		pd += 4;
	}

	return MLKERR_OK;
}

/* 透過色読み込み */

static int _read_transparent(pngdata *p,mLoadImage *pli,int coltype,int bits)
{
	uint8_t *ps,*pd,*ptp,a;
	int num,i;
	png_color_16 *tcol;

	if(!png_get_tRNS(p->png, p->pnginfo, &ps, &num, &tcol))
		return MLKERR_UNKNOWN;

	//パレットカラー以外時

	if(tcol && coltype != PNG_COLOR_TYPE_PALETTE)
	{
		if(coltype == PNG_COLOR_TYPE_GRAY
			|| coltype == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			i = tcol->gray;

			switch(bits)
			{
				case 1: i *= 255; break;
				case 2: i *= 85; break;
				case 4: i *= 17; break;
			}
			
			pli->trns.r = pli->trns.g = pli->trns.b = i;
		}
		else
		{
			//16bit の場合あり
			pli->trns.r = tcol->red;
			pli->trns.g = tcol->green;
			pli->trns.b = tcol->blue;
		}

		pli->trns.flag = 1;
	}

	//パレットにアルファ値セット

	if(ps && p->palbuf)
	{
		pd = p->palbuf + 3;
		ptp = NULL;
		
		for(i = num; i > 0; i--)
		{
			*pd = a = *(ps++);

			//最初の A=0 の位置
			if(a == 0 && !ptp)
				ptp = pd - 3;
			
			pd += 4;
		}

		//最初の A=0 の色を透過色にセット

		if(ptp)
		{
			pli->trns.flag = 1;
			pli->trns.r = ptp[0];
			pli->trns.g = ptp[1];
			pli->trns.b = ptp[2];
		}
	}

	return MLKERR_OK;
}

/* PNG 情報読み込み＆設定 */

static int _proc_info(pngdata *p,mLoadImage *pli)
{
	png_struct *png;
	png_info *pnginfo;
	png_uint_32 width,height,tRNS;
	int n,depth,coltype,interlace;

	png = p->png;
	pnginfo = p->pnginfo;

	//----- チャンクから情報取得

	//IHDR チャンク取得

	if(!png_get_IHDR(png, pnginfo, &width, &height,
			&depth, &coltype, &interlace, NULL, NULL))
		return MLKERR_UNKNOWN;

	//PLTE 読み込み

	if(coltype == PNG_COLOR_TYPE_PALETTE)
	{
		n = _read_palette(p, pli);
		if(n) return n;
	}

	//tRNS チャンクデータがあるか (なければ 0)

	tRNS = png_get_valid(png, pnginfo, PNG_INFO_tRNS);

	//tRNS 読み込み

	if(tRNS)
	{
		n = _read_transparent(p, pli, coltype, depth);
		if(n) return n;
	}

	//----- イメージ展開方法セット

	//16bit => 8bit

	if(depth == 16 && !(pli->flags & MLOADIMAGE_FLAGS_ALLOW_16BIT))
	{
		png_set_strip_16(png);
		depth = 8;

		//透過色

		if(pli->trns.flag)
		{
			pli->trns.r >>= 8;
			pli->trns.g >>= 8;
			pli->trns.b >>= 8;
		}
	}

	//8bit 未満 => 8bit

	if(depth < 8)
	{
		if(coltype == PNG_COLOR_TYPE_GRAY)
			png_set_expand_gray_1_2_4_to_8(png);
		else
			png_set_packing(png);
		
		depth = 8;
	}

	//変換 (=> RGB/RGBA)

	if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGB
		|| pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
	{
		if(coltype == PNG_COLOR_TYPE_PALETTE && tRNS
			&& pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA
			&& !(pli->flags & MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA))
		{
			//パレット + tRNS の場合は、RGBA 変換を行うと、透過色がアルファ値変換されるため、
			//透過色をそのままの色で取得したい場合は、手動で変換する必要がある。

			p->fconvpal = TRUE;
		}
		else
		{
			//---- libpng での変換
			
			n = 0; //アルファ値があるか
		
			switch(coltype)
			{
				case PNG_COLOR_TYPE_PALETTE:
					//[!] 透過色がアルファ値になる
					png_set_expand(png);
					if(tRNS) n = 1;
					break;
				case PNG_COLOR_TYPE_RGBA:
					n = 1;
					break;
				case PNG_COLOR_TYPE_GRAY_ALPHA:
					n = 1;
				case PNG_COLOR_TYPE_GRAY:
					png_set_gray_to_rgb(png);
					break;
			}

			if(pli->convert_type == MLOADIMAGE_CONVERT_TYPE_RGBA)
			{
				if(tRNS && (pli->flags & MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA))
					//透過色をアルファ値に
					png_set_tRNS_to_alpha(png);
				else if(!n)
					//アルファ値がない場合、追加
					png_set_add_alpha(png, (depth == 16)? 0xffff: 0xff, PNG_FILLER_AFTER);

				coltype = PNG_COLOR_TYPE_RGBA;
			}
			else
			{
				//アルファ値除去
				if(n) png_set_strip_alpha(png);

				coltype = PNG_COLOR_TYPE_RGB;
			}
		}
	}

	//16bit バイト順入れ替え (ホストのバイト順にする)

	if(depth == 16 && mIsByteOrderLE())
		png_set_swap(png);

	//インターレース処理

	if(interlace == PNG_INTERLACE_ADAM7)
		png_set_interlace_handling(png);

	//----- 反映させる

	png_read_update_info(png, pnginfo);

	//----- mLoadImage にセット

	pli->width = width;
	pli->height = height;
	pli->bits_per_sample = depth;

	//カラータイプ

	if(coltype == PNG_COLOR_TYPE_RGB)
		n = MLOADIMAGE_COLTYPE_RGB;
	else if(coltype == PNG_COLOR_TYPE_RGB_ALPHA || p->fconvpal)
		n = MLOADIMAGE_COLTYPE_RGBA;
	else if(coltype == PNG_COLOR_TYPE_GRAY)
		n = MLOADIMAGE_COLTYPE_GRAY;
	else if(coltype == PNG_COLOR_TYPE_GRAY_ALPHA)
		n = MLOADIMAGE_COLTYPE_GRAY_A;
	else
		n = MLOADIMAGE_COLTYPE_PALETTE;

	pli->coltype = n;

	return MLKERR_OK;
}

/* 情報読み込み */

static int _read_info(pngdata *p,mLoadImage *pli)
{
	png_struct *png;
	png_info *pnginfo;
	int ret;

	p->pli = pli;

	//png_struct 作成

	png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			pli, _png_error_func, _png_warn_func);

	if(!png) return MLKERR_ALLOC;

	p->png = png;

	//png_info 作成

	pnginfo = png_create_info_struct(png);
	if(!pnginfo) return MLKERR_ALLOC;

	p->pnginfo = pnginfo;

	//setjmp

	if(setjmp(png_jmpbuf(png)))
		return MLKERR_LONGJMP;

	//読み込み関数

	png_set_read_fn(png, p, _png_read_func);

	//CRC エラー時の対処
	// :重要チャンクに関しては、デフォルトの状態だとクラッシュするので、
	// :CRC は無視してそのまま使う。
	// :補助チャンクはデフォルトで警告、データを捨てるようになっている。

	png_set_crc_action(png, PNG_CRC_QUIET_USE, PNG_CRC_DEFAULT);

	//進捗用

	png_set_read_status_fn(png, (pli->progress)? _png_read_row_callback: NULL);

	//ヘッダチェック

	ret = _proc_header(p, pli);
	if(ret) return ret;

	//情報読み込み

	png_read_info(png, pnginfo);

	//解像度取得

	_get_resolution(p, pli);

	//読み込み＆設定

	return _proc_info(p, pli);
}

/* 透過付きパレット8bit -> RGBA 変換
 *
 * - 透過色のアルファ値変換なし時。
 * - A=0 のパレットは、A=255 にする。それ以外のアルファ値はそのまま。 */

static void _convert_pal_to_rgba(pngdata *p,mLoadImage *pli)
{
	uint8_t **ppbuf,*pd,*ps,*ppal,*palbuf;
	int ix,iy,src_right,dst_right;

	palbuf = p->palbuf;

	//パレットの A=0 を 255 に

	pd = palbuf + 3;

	for(ix = p->palnum; ix; ix--, pd += 4)
	{
		if(*pd == 0)
			*pd = 255;
	}

	//RGBA 変換

	ppbuf = pli->imgbuf;
	src_right = pli->width - 1;
	dst_right = src_right * 4;

	for(iy = pli->height; iy; iy--)
	{
		pd = ps = *(ppbuf++);
		ps += src_right;
		pd += dst_right;
	
		for(ix = pli->width; ix; ix--, pd -= 4, ps--)
		{
			ppal = palbuf + (*ps << 2);

			*((uint32_t *)pd) = *((uint32_t *)ppal);
		}
	}
}


//=================================
// main
//=================================


/* 終了 */

static void _png_close(mLoadImage *pli)
{
	pngdata *p = (pngdata *)pli->handle;

	if(p)
	{
		//png_struct, png_info 破棄

		if(p->png)
		{
			png_destroy_read_struct(&p->png,
				(p->pnginfo)? &p->pnginfo: NULL, NULL);
		}

		//

		mIO_close(p->io);
		
		mFree(p->palbuf);
	}

	mLoadImage_closeHandle(pli);
}

/* 開く */

static mlkerr _png_open(mLoadImage *pli)
{
	int ret;

	//作成・開く

	ret = mLoadImage_createHandle(pli, sizeof(pngdata), MIO_ENDIAN_BIG);
	if(ret) return ret;

	//情報読み込み

	return _read_info((pngdata *)pli->handle, pli);
}

/* イメージ読み込み */

static mlkerr _png_getimage(mLoadImage *pli)
{
	pngdata *p = (pngdata *)pli->handle;
	int ret;

	//パレットセット

	ret = mLoadImage_setPalette(pli, p->palbuf, 256 * 4, p->palnum);
	if(ret) return ret;

	//イメージ

	png_read_image(p->png, pli->imgbuf);
	png_read_end(p->png, NULL);

	//パレット->RGBA 変換

	if(p->fconvpal)
		_convert_pal_to_rgba(p, pli);

	return MLKERR_OK;
}

/**@ PNG 判定と関数セット */

mlkbool mLoadImage_checkPNG(mLoadImageType *p,uint8_t *buf,int size)
{
	if(!buf || (size >= 8 && png_sig_cmp(buf, 0, 8) == 0))
	{
		p->format_tag = MLK_MAKE32_4('P','N','G',' ');
		p->open = _png_open;
		p->getimage = _png_getimage;
		p->close = _png_close;
		
		return TRUE;
	}

	return FALSE;
}

/**@ PNG ガンマ値取得
 *
 * @p:dst ガンマ値に 100,000 を掛けた値が格納される
 * @r:TRUE で取得できた */

mlkbool mLoadImagePNG_getGamma(mLoadImage *pli,uint32_t *dst)
{
	pngdata *p = (pngdata *)pli->handle;
	png_fixed_point gamma;

	if(p && png_get_gAMA_fixed(p->png, p->pnginfo, &gamma))
	{
		*dst = gamma;
		return TRUE;
	}

	return FALSE;
}

/**@ (PNG) ICC プロファイル取得
 *
 * @p:psize データサイズが格納される
 * @r:確保されたバッファが返る (NULL で失敗) */

uint8_t *mLoadImagePNG_getICCProfile(mLoadImage *pli,uint32_t *psize)
{
	pngdata *p = (pngdata *)pli->handle;
	char *name;
	int comptype;
	png_bytep buf;
	png_uint_32 len;

	if(!png_get_iCCP(p->png, p->pnginfo, &name, &comptype, &buf, &len)
		|| !buf)
		return NULL;
	else
	{
		*psize = len;
		return mMemdup(buf, len);
	}
}
