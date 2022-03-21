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
 * TileImage : 画像ファイル関連
 *****************************************/

#include <stdio.h>
#include <string.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"
#include "mlk_loadimage.h"
#include "mlk_saveimage.h"
#include "mlk_zlib.h"
#include "mlk_stdio.h"
#include "mlk_file.h"
#include "mlk_util.h"

#include "def_tileimage.h"
#include "tileimage.h"
#include "pv_tileimage.h"

#include "imagecanvas.h"
#include "fileformat.h"



/** 指定ビット数に変換するためのテーブルを作成 */

void *TileImage_createToBits_table(int bits)
{
	if(bits == 8)
		return (void *)TileImage_create16fixto8_table();
	else
		return (void *)TileImage_create8to16fix_table();
}

/** 8bit -> 固定小数15bit 変換用のテーブルを作成 */

uint16_t *TileImage_create8to16fix_table(void)
{
	uint16_t *buf,*pd;
	int i;

	pd = buf = (uint16_t *)mMalloc(256 * 2);
	if(!buf) return NULL;

	for(i = 0; i < 256; i++)
		*(pd++) = (uint16_t)(i / 255.0 * 0x8000 + 0.5);

	return buf;
}

/** 16bit -> 固定小数15bit 変換用のテーブルを作成 */

uint16_t *TileImage_create16to16fix_table(void)
{
	uint16_t *buf,*pd;
	int i;

	pd = buf = (uint16_t *)mMalloc((1<<16) * 2);
	if(!buf) return NULL;

	for(i = 0; i < (1<<16); i++)
		*(pd++) = (uint16_t)((double)i / 0xffff * 0x8000 + 0.5);

	return buf;
}

/** 固定小数15bit -> 8bit 変換用のテーブルを作成 */

uint8_t *TileImage_create16fixto8_table(void)
{
	uint8_t *buf,*pd;
	int i;

	pd = buf = (uint8_t *)mMalloc((1<<15) + 1);
	if(!buf) return NULL;

	for(i = 0; i <= (1<<15); i++)
		*(pd++) = (uint8_t)((double)i / (1<<15) * 255 + 0.5);

	return buf;
}

/** 固定小数15bit -> 16bit 変換用のテーブルを作成 */

uint16_t *TileImage_create16fixto16_table(void)
{
	uint16_t *buf,*pd;
	int i;

	pd = buf = (uint16_t *)mMalloc(((1<<15) + 1) * 2);
	if(!buf) return NULL;

	for(i = 0; i <= (1<<15); i++)
		*(pd++) = (uint16_t)((double)i / 0x8000 * 0xffff + 0.5);

	return buf;
}


//===================================
// 読み込み時のイメージバッファ処理
//===================================


/* 8bit アルファ値無効処理 */

static void _loadimgbuf_convert_8bit(uint8_t **ppbuf,int width,int height)
{
	int ix,iy;
	uint8_t *pd;

	for(iy = height; iy; iy--, ppbuf++)
	{
		pd = *ppbuf + 3;

		for(ix = width; ix; ix--, pd += 4)
			*pd = 255;
	}
}

/* 8bit -> 16bit 処理 */

static void _loadimgbuf_convert_8to16(uint8_t **ppbuf,int width,int height,mlkbool ignore_alpha)
{
	int ix,iy,right;
	uint8_t *pd8;
	uint16_t *pd16,*tbl,c[4];

	tbl = TileImage_create8to16fix_table();
	if(!tbl) return;

	right = (width - 1) * 4;

	for(iy = height; iy; iy--, ppbuf++)
	{
		//右端から処理
		pd8 = *ppbuf + right;
		pd16 = (uint16_t *)*ppbuf + right;

		for(ix = width; ix; ix--, pd8 -= 4, pd16 -= 4)
		{
			c[0] = tbl[pd8[0]];
			c[1] = tbl[pd8[1]];
			c[2] = tbl[pd8[2]];
			c[3] = tbl[(ignore_alpha)? 255: pd8[3]];

			*((uint64_t *)pd16) = *((uint64_t *)c);
		}
	}

	mFree(tbl);
}

/* 16bit -> 16bit(固定小数15bit) 処理 */

static void _loadimgbuf_convert_16to16(uint8_t **ppbuf,int width,int height,mlkbool ignore_alpha)
{
	int ix,iy;
	uint16_t *pd,*tbl;

	tbl = TileImage_create16to16fix_table();
	if(!tbl) return;

	for(iy = height; iy; iy--, ppbuf++)
	{
		pd = (uint16_t *)*ppbuf;

		for(ix = width; ix; ix--, pd += 4)
		{
			pd[0] = tbl[pd[0]];
			pd[1] = tbl[pd[1]];
			pd[2] = tbl[pd[2]];
			pd[3] = tbl[(ignore_alpha)? 0xffff: pd[3]];
		}
	}

	mFree(tbl);
}

/** 読み込まれた画像イメージの変換処理 (RGBA)
 *
 * ビット数をキャンバスの状態に合わせて変換し、オプションを適用。
 * イメージの値は、8bit:255, 16bit:0xffff。
 *
 * ppbuf: ソース & 変換後のバッファ
 * srcbits: 画像のビット数。8 or 16
 * ignore_alpha: アルファチャンネル無効 (すべて完全不透明に) */

void TileImage_loadimgbuf_convert(uint8_t **ppbuf,int width,int height,
	int srcbits,mlkbool ignore_alpha)
{
	if(TILEIMGWORK->bits == 8)
	{
		//8bit: "アルファ値無効"なしの場合は、何もしない

		if(!ignore_alpha) return;

		_loadimgbuf_convert_8bit(ppbuf, width, height);
	}
	else if(srcbits == 8)
		//8bit -> 16bit(15fix)
		_loadimgbuf_convert_8to16(ppbuf, width, height, ignore_alpha);
	else
		//16bit -> 16bit(15fix)
		_loadimgbuf_convert_16to16(ppbuf, width, height, ignore_alpha);
}


//===========================================
// イメージバッファからタイルイメージに変換
//===========================================


/** イメージバッファから TileImage に変換 (RGBA)
 *
 * - RGBA -> TileImage の現在のタイプに変換される (RGBA/GRAY のみ)。
 * - TileImage は、ソースのサイズ分、配列を確保してあること。
 * - イメージは、現在のビット数の値 (255/0x8000) に変換されていること。
 * - イメージは RGBA として、アルファ値もセットされていること。 */

void TileImage_convertFromImage(TileImage *p,uint8_t **ppsrc,int srcw,int srch,
	mPopupProgress *prog,int prog_subnum)
{
	uint8_t **ppdst,*tilebuf;
	int ix,iy,xx,yy,w,h;
	TileImageColFunc_RGBAtoTile func_conv;
	TileImageColFunc_isTransparentTile func_isempty;

	//作業用タイル

	tilebuf = TileImage_allocTile(p);
	if(!tilebuf) return;

	//

	ppdst = p->ppbuf;

	func_conv = TILEIMGWORK->colfunc[p->type].rgba_to_tile;
	func_isempty = TILEIMGWORK->colfunc[p->type].is_transparent_tile;

	mPopupProgressThreadSubStep_begin(prog, prog_subnum, p->tilew * p->tileh);

	//

	for(iy = p->tileh, yy = 0; iy; iy--, yy += 64)
	{
		h = 64;
		if(yy + 64 > srch) h = srch - yy;
	
		for(ix = p->tilew, xx = 0; ix; ix--, xx += 64, ppdst++)
		{
			w = 64;
			if(xx + 64 > srcw) w = srcw - xx;

			//64x64 でなければクリア

			if(w != 64 || h != 64)
				TileImage_clearTile(p, tilebuf);

			//tilebuf に変換

			(func_conv)(tilebuf, ppsrc, xx, w, h);

			//すべて透明でなければ、確保してコピー

			if(!(func_isempty)(tilebuf))
			{
				if(TileImage_allocTile_atptr(p, ppdst))
					TileImage_copyTile(p, *ppdst, tilebuf);
			}

			mPopupProgressThreadSubStep_inc(prog);
		}

		ppsrc += 64;
	}

	mFree(tilebuf);
}

/** ImageCanvas のバッファから TileImage に変換 (RGBA) */

void TileImage_convertFromCanvas(TileImage *p,ImageCanvas *src,mPopupProgress *prog,int prog_subnum)
{
	TileImage_convertFromImage(p, src->ppbuf, src->width, src->height,
		prog, prog_subnum);
}


//===============================
// 画像読み込み
//===============================


/* mLoadImage 用進捗 (0-100) */

static void _loadimg_progress(mLoadImage *p,int prog)
{
	mPopupProgressThreadSetPos((mPopupProgress *)p->param, prog);
}

/** ファイルから画像読み込み
 *
 * dst_size: NULL 以外で、画像のサイズが入る */

mlkerr TileImage_loadFile(TileImage **ppdst,const char *filename,uint32_t format,
	mSize *dst_size,mPopupProgress *prog)
{
	mLoadImage li;
	mLoadImageType tp;
	TileImage *img;
	mlkerr ret;

	//mLoadImage
	// :キャンバスが8bitなら、常に8bitイメージ。
	// :16bitの場合は、8bit or 16bit。

	mLoadImage_init(&li);

	li.open.type = MLOADIMAGE_OPEN_FILENAME;
	li.open.filename = filename;
	li.convert_type = MLOADIMAGE_CONVERT_TYPE_RGBA;
	li.flags = MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA;
	li.progress = _loadimg_progress;
	li.param = prog;

	if(TILEIMGWORK->bits == 16)
		li.flags |= MLOADIMAGE_FLAGS_ALLOW_16BIT;

	//フォーマット

	FileFormat_getLoadImageType(format, &tp);

	//開く

	ret = (tp.open)(&li);
	if(ret) goto ERR;

	mPopupProgressThreadSetMax(prog, 100 + 10);

	//イメージ作成

	img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, li.width, li.height);
	if(!img)
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//読み込み用バッファ
	// :キャンバスのビットに合わせる。

	if(!mLoadImage_allocImage(&li, li.width * (TILEIMGWORK->bits / 8) * 4))
	{
		ret = MLKERR_ALLOC;
		goto ERR;
	}

	//読み込み

	ret = (tp.getimage)(&li);
	if(ret) goto ERR;

	//イメージの処理

	TileImage_loadimgbuf_convert(li.imgbuf, li.width, li.height,
		li.bits_per_sample, FALSE);

	//タイルイメージに変換

	TileImage_convertFromImage(img,
		li.imgbuf, li.width, li.height, prog, 10);

	//

	*ppdst = img;

	if(dst_size)
	{
		dst_size->w = li.width;
		dst_size->h = li.height;
	}
	
	ret = MLKERR_OK;

ERR:
	(tp.close)(&li);

	mLoadImage_freeImage(&li);

	return ret;
}


//=================================
// PNG 保存
//=================================


typedef struct
{
	mSaveImage i;
	
	uint8_t *table;
}_saveimage_png;


/* プログレス */

static void _savepng_progress(mSaveImage *p,int percent)
{
	mPopupProgressThreadSetPos((mPopupProgress *)p->param2, percent);
}

/* Y1行を送る */

static mlkerr _savepng_setrow(mSaveImage *p,int y,uint8_t *buf,int line_bytes)
{
	_saveimage_png *si = (_saveimage_png *)p;
	TileImage *img;
	uint8_t *table;
	int ix,w;
	RGBA16 c16;

	img = (TileImage *)p->param1;
	w = p->width;

	if(TILEIMGWORK->bits == 8)
	{
		//8bit
		
		for(ix = 0; ix < w; ix++, buf += 4)
			TileImage_getPixel(img, ix, y, buf);
	}
	else
	{
		//16bit (8bit に変換)

		table = si->table;
		
		for(ix = 0; ix < w; ix++, buf += 4)
		{
			TileImage_getPixel(img, ix, y, &c16);

			buf[0] = table[c16.r];
			buf[1] = table[c16.g];
			buf[2] = table[c16.b];
			buf[3] = table[c16.a];
		}
	}

	return MLKERR_OK;
}

/** キャンバスイメージ範囲を、PNG (RGBA) に保存 */

mlkerr TileImage_savePNG_rgba(TileImage *p,const char *filename,int dpi,
	mPopupProgress *prog)
{
	_saveimage_png si;
	mlkerr ret;

	mMemset0(&si, sizeof(_saveimage_png));

	si.i.open.type = MSAVEIMAGE_OPEN_FILENAME;
	si.i.open.filename = filename;
	si.i.width = TILEIMGWORK->imgw;
	si.i.height = TILEIMGWORK->imgh;
	si.i.bits_per_sample = 8;
	si.i.samples_per_pixel = 4;
	si.i.coltype = MSAVEIMAGE_COLTYPE_RGB;
	si.i.reso_unit = MSAVEIMAGE_RESOUNIT_DPI;
	si.i.reso_horz = dpi;
	si.i.reso_vert = dpi;
	si.i.progress = _savepng_progress;
	si.i.setrow = _savepng_setrow;

	si.i.param1 = p;
	si.i.param2 = prog;

	//16bit時、変換テーブル

	if(TILEIMGWORK->bits == 16)
	{
		si.table = TileImage_create16fixto8_table();
		if(!si.table) return MLKERR_ALLOC;
	}

	//保存

	mPopupProgressThreadSetMax(prog, 100);

	ret = mSaveImagePNG((mSaveImage *)&si, NULL);

	mFree(si.table);

	return ret;
}


//=================================
// 選択範囲内イメージを PNG 保存
//=================================


typedef struct
{
	mSaveImage i;

	TileImage *img,*imgsel;
	uint8_t *tblbuf; //16->8bit変換テーブル
	int left,top;
}_saveimg_selpng;


/* プログレス */

static void _saveselpng_progress(mSaveImage *p,int percent)
{
	mPopupProgressThreadSetPos((mPopupProgress *)p->param1, percent);
}

/* Y1行を送る */

static int _saveselpng_setrow(mSaveImage *saveimg,int y,uint8_t *buf,int line_bytes)
{
	_saveimg_selpng *p = (_saveimg_selpng *)saveimg;
	TileImage *img,*selimg;
	uint8_t *tblbuf;
	uint16_t *ps16;
	int ix,x,bits;
	uint8_t colbuf[8];

	img = p->img;
	selimg = p->imgsel;
	tblbuf = p->tblbuf;
	x = p->left;
	y += p->top;
	bits = TILEIMGWORK->bits;
	ps16 = (uint16_t *)colbuf;

	for(ix = saveimg->width; ix; ix--, x++, buf += 4)
	{
		if(TileImage_isPixel_transparent(selimg, x, y))
		{
			*((uint32_t *)buf) = 0;
		}
		else
		{
			TileImage_getPixel(img, x, y, &colbuf);

			if(bits == 8)
				*((uint32_t *)buf) = *((uint32_t *)colbuf);
			else
			{
				buf[0] = tblbuf[ps16[0]];
				buf[1] = tblbuf[ps16[1]];
				buf[2] = tblbuf[ps16[2]];
				buf[3] = tblbuf[ps16[3]];
			}
		}
	}

	return MLKERR_OK;
}

/** 選択範囲内イメージを PNG (RGBA) に保存 */

mlkerr TileImage_savePNG_select_rgba(TileImage *img,TileImage *selimg,
	const char *filename,int dpi,const mRect *rc,mPopupProgress *prog)
{
	_saveimg_selpng si;
	mlkerr ret;

	mMemset0(&si, sizeof(_saveimg_selpng));

	si.i.open.type = MSAVEIMAGE_OPEN_FILENAME;
	si.i.open.filename = filename;
	si.i.width = rc->x2 - rc->x1 + 1;
	si.i.height = rc->y2 - rc->y1 + 1;
	si.i.bits_per_sample = 8;
	si.i.samples_per_pixel = 4;
	si.i.coltype = MSAVEIMAGE_COLTYPE_RGB;
	si.i.reso_unit = MSAVEIMAGE_RESOUNIT_DPI;
	si.i.reso_horz = dpi;
	si.i.reso_vert = dpi;
	si.i.progress = _saveselpng_progress;
	si.i.setrow = _saveselpng_setrow;
	si.i.param1 = prog;

	si.img = img;
	si.imgsel = selimg;
	si.left = rc->x1;
	si.top = rc->y1;

	//16->8bit 変換テーブル

	if(TILEIMGWORK->bits == 16)
	{
		si.tblbuf = TileImage_create16fixto8_table();
		if(!si.tblbuf) return MLKERR_ALLOC;
	}

	//保存

	mPopupProgressThreadSetMax(prog, 100);

	ret = mSaveImagePNG((mSaveImage *)&si, NULL);

	mFree(si.tblbuf);

	return ret;
}


//================================
// 作業用イメージファイル
// (選択範囲 コピー時)
//================================


/** 作業用イメージファイル保存 */

mlkbool TileImage_saveTmpImageFile(TileImage *p,const char *filename)
{
	FILE *fp;
	mZlib *zenc = NULL;
	uint8_t **pptile;
	mRect rc;
	uint32_t tilenum,encsize;
	int tx,ty,ix,iy;
	uint16_t tilepos[2];
	long fpos;
	mlkbool ret = FALSE;

	//開く

	fp = mFILEopen(filename, "wb");
	if(!fp) return FALSE;
	
	//タイプ、線の色

	if(mFILEwriteByte(fp, p->type)
		|| mFILEwriteOK(fp, &p->col, sizeof(RGBcombo)))
		goto ERR;

	//------ タイル情報

	//タイルイメージが存在する範囲

	TileImage_getHaveImageRect_pixel(p, &rc, &tilenum);

	//空イメージの場合

	if(tilenum == 0)
	{
		if(mFILEwrite0(fp, 4 * 6)) goto ERR;
		
		fclose(fp);
		return TRUE;
	}

	//範囲、タイル数

	if(mFILEwriteOK(fp, &rc, sizeof(mRect))
		|| mFILEwriteOK(fp, &tilenum, 4))
		goto ERR;

	//圧縮サイズ(仮)

	fpos = ftell(fp);

	if(mFILEwrite0(fp, 4)) goto ERR;

	//zlib

	zenc = mZlibEncNew(0, 6, -15, 8, 0);
	if(!zenc) goto ERR;

	mZlibSetIO_stdio(zenc, fp);

	//タイルイメージ

	pptile = p->ppbuf;
	ty = (p->offy - rc.y1) >> 6;

	for(iy = p->tileh; iy; iy--, ty++)
	{
		tx = (p->offx - rc.x1) >> 6;
	
		for(ix = p->tilew; ix; ix--, tx++, pptile++)
		{
			if(!(*pptile)) continue;

			//タイル位置とタイルデータ

			tilepos[0] = tx;
			tilepos[1] = ty;

			if(mZlibEncSend(zenc, tilepos, 4)
				|| mZlibEncSend(zenc, *pptile, p->tilesize))
				goto ERR;
		}
	}

	if(mZlibEncFinish(zenc)) goto ERR;

	//圧縮サイズ

	encsize = mZlibEncGetSize(zenc);

	fseek(fp, fpos, SEEK_SET);

	if(mFILEwriteOK(fp, &encsize, 4)) goto ERR;

	fseek(fp, 0, SEEK_END);

	//----------

	ret = TRUE;

ERR:
	mZlibFree(zenc);

	fclose(fp);

	if(!ret) mDeleteFile(filename);

	return ret;
}

/** 作業用イメージファイル読み込み
 *
 * [!] ビット数が現在の値と異なる場合がある。 */

TileImage *TileImage_loadTmpImageFile(const char *filename,int srcbits,int dstbits)
{
	TileImage *p = NULL;
	FILE *fp;
	mZlib *zdec = NULL;
	void *tblbuf = NULL;
	uint8_t **pptile,*tilebuf = NULL;
	uint8_t type,err = TRUE;
	uint32_t tilenum,encsize;
	uint16_t tilepos[2];
	int srctilesize;
	RGBcombo col;
	mRect rc;

	//開く

	fp = mFILEopen(filename, "rb");
	if(!fp) return NULL;

	//タイプ、線の色

	if(mFILEreadByte(fp, &type)
		|| mFILEreadOK(fp, &col, sizeof(RGBcombo)))
		goto END;

	//情報

	if(mFILEreadOK(fp, &rc, sizeof(mRect))
		|| mFILEreadOK(fp, &tilenum, 4)
		|| mFILEreadOK(fp, &encsize, 4))
		goto END;

	//空イメージの場合

	if(tilenum == 0)
	{
		p = TileImage_new(type, 1, 1);
		err = FALSE;
		goto END;
	}
	
	//イメージ作成

	p = TileImage_newFromRect(type, &rc);
	if(!p) goto END;

	//zlib

	zdec = mZlibDecNew(0, -15);
	if(!zdec) goto END;

	mZlibSetIO_stdio(zdec, fp);
	mZlibDecSetSize(zdec, encsize);

	//ビット数変換時

	if(type != TILEIMAGE_COLTYPE_ALPHA1BIT
		&& srcbits != dstbits)
	{
		srctilesize = TileImage_global_getTileSize(type, srcbits);

		tblbuf = TileImage_createToBits_table(dstbits);

		tilebuf = (uint8_t *)mMalloc(srctilesize);

		if(!tblbuf || !tilebuf) goto END;
	}

	//------ タイルデータ

	for(; tilenum; tilenum--)
	{
		//タイル位置
	
		if(mZlibDecRead(zdec, tilepos, 4)) goto END;

		//タイル確保

		pptile = TILEIMAGE_GETTILE_BUFPT(p, tilepos[0], tilepos[1]);

		if(!TileImage_allocTile_atptr(p, pptile))
			goto END;

		//タイルイメージ

		if(!tblbuf)
		{
			if(mZlibDecRead(zdec, *pptile, p->tilesize))
				goto END;
		}
		else
		{
			//ビット数変換時
			
			if(mZlibDecRead(zdec, tilebuf, srctilesize))
				goto END;

			TileImage_copyTile_convert(p, *pptile, tilebuf, tblbuf, dstbits);
		}
	}

	mZlibDecFinish(zdec);

	//----------

	err = FALSE;

END:
	mFree(tblbuf);
	mFree(tilebuf);
	mZlibFree(zdec);

	fclose(fp);

	//エラー時はイメージ解放

	if(!err)
	{
		p->col = col;
		return p;
	}
	else
	{
		TileImage_free(p);
		return NULL;
	}
}


//=============================
// 読み書き サブ
//=============================


/** 指定位置にタイルを作成し、読み込んだタイルをセット
 * (APD v2,v4)
 *
 * src: BE, 64x64, チャンネル別に別れている */

mlkbool TileImage_setTile_fromSave(TileImage *p,int tx,int ty,uint8_t *src)
{
	uint8_t *tile;

	tile = TileImage_getTileAlloc_atpos(p, tx, ty, FALSE);
	if(!tile) return FALSE;

	(TILEIMGWORK->colfunc[p->type].convert_from_save)(tile, src);

	return TRUE;
}

/** (APDv3 読み込み) タイル変換 */

void TileImage_converTile_APDv3(TileImage *p,uint8_t *dst,uint8_t *src)
{
	if(TILEIMGWORK->bits == 8 && p->type == TILEIMAGE_COLTYPE_RGBA)
	{
		//RGBA 8bit

		TileImage_copyTile(p, dst, src);
	}
	else
	{
		//16bit or A 1bit

		(TILEIMGWORK->colfunc[p->type].convert_from_save)(dst, src);
	}
}

/** (APDv4) イメージ保存時のタイル処理
 *
 * rc: イメージのあるpx範囲
 * buf: タイルサイズ + 4byte */

mlkerr TileImage_saveTiles_apd4(TileImage *p,mRect *rc,uint8_t *buf,
	mlkerr (*func)(TileImage *p,void *param),void *param)
{
	uint8_t **pptile;
	int ix,iy,tx,ty;
	mlkerr ret;
	TileImageColFunc_convert_forSave func_conv;

	func_conv = TILEIMGWORK->colfunc[p->type].convert_to_save;

	pptile = p->ppbuf;
	
	ty = (p->offy - rc->y1) >> 6;

	for(iy = p->tileh; iy; iy--, ty++)
	{
		tx = (p->offx - rc->x1) >> 6;
		
		for(ix = p->tilew; ix; ix--, tx++, pptile++)
		{
			if(!(*pptile)) continue;

			//タイル位置

			mSetBufBE16(buf, tx);
			mSetBufBE16(buf + 2, ty);

			//タイルデータ

			(func_conv)(buf + 4, *pptile);

			//呼び出し

			ret = (func)(p, param);
			if(ret) return ret;
		}
	}

	return MLKERR_OK;
}


//=============================
// PSD サブ
//=============================


/* チャンネルイメージセット (8bit) */

static mlkbool _set_channel_image_8bit(TileImage *p,int chno,uint8_t **ppsrc,int srcw,int srch)
{
	uint8_t **pptile,*workbuf;
	uint8_t *ps,*pd,**ppsy;
	uint64_t *p64;
	int fgray,ix,iy,i,w,h,sw,sh,real_chno,xpos;

	fgray = (p->type == TILEIMAGE_COLTYPE_GRAY);

	real_chno = chno;
	if(chno < 0) real_chno = (fgray)? 1: 3;

	//タイルバッファ

	workbuf = (uint8_t *)mMalloc(64 * 64);
	if(!workbuf) return FALSE;

	//タイル単位で処理

	pptile = p->ppbuf;

	for(iy = p->tileh, sh = srch; iy; iy--, ppsrc += 64, sh -= 64)
	{
		xpos = 0;
		h = (sh > 64)? 64: sh;
		
		for(ix = p->tilew, sw = srcw; ix; ix--, pptile++, sw -= 64, xpos += 64)
		{
			//[色のチャンネル]
			// タイルが作成されていない場合、すべて透明なので、何もしない

			if(chno >= 0 && !(*pptile)) continue;
		
			//ソースの値を 64x64 範囲のデータで取得

			ppsy = ppsrc;
			pd = workbuf;
			w = (sw > 64)? 64: sw;

			if(w != 64 || h != 64)
				memset(workbuf, 0, 64 * 64);

			for(i = h; i; i--, ppsy++, pd += 64)
				memcpy(pd, *ppsy + xpos, w);

			//アルファ値の場合、タイル作成

			if(chno < 0)
			{
				//すべて 0 なら、空タイル

				p64 = (uint64_t *)workbuf;

				for(i = 64 * 64 / 8; i && !(*p64); i--, p64++);

				if(i == 0) continue;
			
				//タイル作成

				*pptile = TileImage_allocTile_clear(p);
				if(!(*pptile))
				{
					mFree(workbuf);
					return FALSE;
				}
			}

			//タイルにセット

			ps = workbuf;
			pd = *pptile + real_chno;

			if(fgray)
			{
				for(i = 64 * 64; i; i--, pd += 2)
					*pd = *(ps++);
			}
			else
			{
				for(i = 64 * 64; i; i--, pd += 4)
					*pd = *(ps++);
			}
		}
	}

	mFree(workbuf);

	return TRUE;
}

/* チャンネルイメージセット (16bit) */

static mlkbool _set_channel_image_16bit(TileImage *p,int chno,uint8_t **ppsrc,int srcw,int srch)
{
	uint8_t **pptile;
	uint16_t *workbuf,**ppsy,*pd,*ps;
	uint64_t *p64;
	int fgray,ix,iy,i,w,h,sw,sh,real_chno,xpos;

	fgray = (p->type == TILEIMAGE_COLTYPE_GRAY);

	real_chno = chno;
	if(chno < 0) real_chno = (fgray)? 1: 3;

	//タイルバッファ

	workbuf = (uint16_t *)mMalloc(64 * 64 * 2);
	if(!workbuf) return FALSE;

	//タイル単位で処理

	pptile = p->ppbuf;

	for(iy = p->tileh, sh = srch; iy; iy--, ppsrc += 64, sh -= 64)
	{
		xpos = 0;
		h = (sh > 64)? 64: sh;
		
		for(ix = p->tilew, sw = srcw; ix; ix--, pptile++, sw -= 64, xpos += 64)
		{
			//[色のチャンネル]
			// タイルが作成されていない場合、すべて透明なので、何もしない

			if(chno >= 0 && !(*pptile)) continue;
		
			//ソースの値を 64x64 範囲のデータで取得

			ppsy = (uint16_t **)ppsrc;
			pd = workbuf;
			w = (sw > 64)? 64: sw;

			if(w != 64 || h != 64)
				memset(workbuf, 0, 64 * 64 * 2);

			w *= 2;

			for(i = h; i; i--, ppsy++, pd += 64)
				memcpy(pd, *ppsy + xpos, w);

			//アルファ値の場合、タイル作成

			if(chno < 0)
			{
				//すべて 0 なら、空タイル

				p64 = (uint64_t *)workbuf;

				for(i = 64 * 64 * 2 / 8; i && !(*p64); i--, p64++);

				if(i == 0) continue;
			
				//タイル作成

				*pptile = TileImage_allocTile_clear(p);
				if(!(*pptile))
				{
					mFree(workbuf);
					return FALSE;
				}
			}

			//タイルにセット

			ps = workbuf;
			pd = (uint16_t *)*pptile + real_chno;

			w = (fgray)? 2: 4;

			for(i = 64 * 64; i; i--, pd += w, ps++)
				*pd = (int)((double)*ps / 0xffff * 0x8000 + 0.5);
		}
	}

	mFree(workbuf);

	return TRUE;
}

/** (PSD読み込み) チャンネルのイメージをセット
 *
 * [!] 色より先にアルファ値をセットすること。
 *
 * chno: [-1] アルファ値 [0,1,2] RGB */

mlkbool TileImage_setChannelImage(TileImage *p,int chno,uint8_t **ppsrc,int srcw,int srch)
{
	if(TILEIMGWORK->bits == 8)
		return _set_channel_image_8bit(p, chno, ppsrc, srcw, srch);
	else
		return _set_channel_image_16bit(p, chno, ppsrc, srcw, srch);
}

