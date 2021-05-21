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
 * Image32
 * RGBA 8bit イメージ
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_pixbuf.h"
#include "mlk_rectbox.h"
#include "mlk_util.h"
#include "mlk_loadimage.h"

#include "image32.h"
#include "appresource.h"


/*
  - R-G-B-A のバイト順で並んでいる。
*/

//------------------

mlkbool loadThumbCheck_apd(mLoadImageType *p,uint8_t *buf,int size);

//サムネイル用チェック関数
static mFuncLoadImageCheck g_loadcheck_thumb[] = {
	loadThumbCheck_apd,
	mLoadImage_checkPNG, mLoadImage_checkJPEG, mLoadImage_checkBMP,
	mLoadImage_checkGIF, mLoadImage_checkTIFF, mLoadImage_checkWEBP,
	mLoadImage_checkPSD, 0
};

//------------------


//============================
// Image32
//============================


/** 解放 */

void Image32_free(Image32 *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成 */

Image32 *Image32_new(int w,int h)
{
	Image32 *p;

	p = (Image32 *)mMalloc0(sizeof(Image32));
	if(!p) return NULL;

	p->w = w;
	p->h = h;
	p->pitch = w * 4;

	//バッファ確保

	p->buf = (uint8_t *)mMalloc(p->pitch * h);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/** 指定位置のポインタを取得
 *
 * return: NULL で範囲外 */

uint8_t *Image32_getPixelBuf(Image32 *p,int x,int y)
{
	if(x < 0 || y < 0 || x >= p->w || y >= p->h)
		return NULL;
	else
		return p->buf + y * p->pitch + x * 4;
}

/** 画像ファイルを読み込み */

Image32 *Image32_loadFile(const char *filename,uint8_t flags)
{
	mLoadImage li;
	mLoadImageType ltype;
	Image32 *img = NULL;
	int ret,maxsize,failed = TRUE;

	mLoadImage_init(&li);

	li.open.type = MLOADIMAGE_OPEN_FILENAME;
	li.open.filename = filename;
	li.convert_type = MLOADIMAGE_CONVERT_TYPE_RGBA;
	li.flags = MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA;

	//フォーマット

	if(flags & IMAGE32_LOAD_F_THUMBNAIL)
	{
		if(mLoadImage_checkFormat(&ltype, &li.open, g_loadcheck_thumb, 12))
			return NULL;
	}
	else
	{
		if(mLoadImage_checkFormat(&ltype, &li.open,
			(mFuncLoadImageCheck *)AppResource_getLoadImageChecks_normal(), 12))
			return NULL;
	}

	//開く

	ret = (ltype.open)(&li);
	if(ret) goto END;

	//サイズ制限

	maxsize = (flags & IMAGE32_LOAD_F_THUMBNAIL)? 8000: 20000;

	if(li.width > maxsize || li.height > maxsize)
		goto END;

	//イメージ作成

	img = Image32_new(li.width, li.height);
	if(!img) goto END;

	//imgbuf セット

	if(!mLoadImage_allocImageFromBuf(&li, img->buf, img->pitch))
		goto END;

	//読み込み

	ret = (ltype.getimage)(&li);
	if(ret) goto END;

	//終了

	failed = FALSE;

END:
	mFree(li.imgbuf);
	
	(ltype.close)(&li);

	if(failed)
	{
		Image32_free(img);
		img = NULL;
	}

	//白と合成

	if(img && (flags & IMAGE32_LOAD_F_BLEND_WHITE))
		Image32_blendBkgnd(img, 0xffffff);

	return img;
}

/** 指定色を背景としてアルファ合成 */

void Image32_blendBkgnd(Image32 *p,uint32_t col)
{
	uint8_t *pd;
	int ix,iy,dr,dg,db,a;

	dr = MLK_RGB_R(col);
	dg = MLK_RGB_G(col);
	db = MLK_RGB_B(col);

	pd = p->buf;

	for(iy = p->h; iy; iy--)
	{
		for(ix = p->w; ix; ix--, pd += 4)
		{
			a = pd[3];

			if(a != 255)
			{
				if(a == 0)
				{
					pd[0] = dr;
					pd[1] = dg;
					pd[2] = db;
				}
				else
				{
					pd[0] = (pd[0] - dr) * a / 255 + dr;
					pd[1] = (pd[1] - dg) * a / 255 + dg;
					pd[2] = (pd[2] - db) * a / 255 + db;
				}
			}
		}
	}
}

/** 指定色でクリア */

void Image32_clear(Image32 *p,uint32_t col)
{
	uint32_t *pd;
	int ix,iy;

	if(!p) return;

	col = mRGBAtoHostOrder(col);

	pd = (uint32_t *)p->buf;

	for(iy = p->h; iy; iy--)
	{
		for(ix = p->w; ix; ix--)
			*(pd++) = col;
	}
}

/** 画像全体を mPixbuf に描画 */

void Image32_putPixbuf(Image32 *p,mPixbuf *dst,int dx,int dy)
{
	mPixbufClipBlt info;
	uint8_t *pd,*ps;
	int ix,iy,pitchs;
	mFuncPixbufSetBuf setpix;

	if(!p) return;

	pd = mPixbufClip_getBltInfo(dst, &info, dx, dy, p->w, p->h);
	if(!pd) return;

	mPixbufGetFunc_setbuf(dst, &setpix);

	ps = Image32_getPixelBuf(p, info.sx, info.sy);
	pitchs = p->pitch - info.w * 4;
	
	for(iy = info.h; iy; iy--)
	{
		for(ix = info.w; ix; ix--)
		{
			(setpix)(pd, mRGBtoPix_sep(ps[0], ps[1], ps[2]));
		
			pd += info.bytes;
			ps += 4;
		}

		pd += info.pitch_dst;
		ps += pitchs;
	}
}

/** 画像を転送 */

void Image32_blt(Image32 *dst,int dx,int dy,
	Image32 *src,int sx,int sy,int w,int h)
{
	uint8_t *ps,*pd;
	int iy,size;

	pd = Image32_getPixelBuf(dst, dx, dy);
	ps = Image32_getPixelBuf(src, sx, sy);
	size = w * 4;

	for(iy = h; iy; iy--)
	{
		memcpy(pd, ps, size);

		ps += src->pitch;
		pd += dst->pitch;
	}
}

/* リサイズ描画 */

static void _draw_resize(Image32 *dst,Image32 *src,int dx,int dy,int dw,int dh)
{
	uint8_t *ps,*pd,*psY[8];
	int ix,iy,jx,jy,n,pitchd,sw,sh,xpos[8],r,g,b;
	int64_t fx,fy,fadd,fadd2,ff;

	sw = src->w;
	sh = src->h;

	pd = Image32_getPixelBuf(dst, dx, dy);
	pitchd = dst->pitch - dw * 4;

	fadd = (int64_t)((double)src->h / dh * (1<<28) + 0.5);
	fadd2 = fadd >> 3;

	//

	for(iy = dh, fy = 0; iy; iy--, fy += fadd)
	{
		for(jy = 0, ff = fy; jy < 8; jy++, ff += fadd2)
		{
			n = ff >> 28;
			if(n >= sh) n = sh - 1;

			psY[jy] = src->buf + n * src->pitch;
		}

		//X
	
		for(ix = dw, fx = 0; ix; ix--, pd += 4, fx += fadd)
		{
			for(jx = 0, ff = fx; jx < 8; jx++, ff += fadd2)
			{
				n = ff >> 28;
				if(n >= sw) n = sw - 1;

				xpos[jx] = n * 4;
			}

			r = g = b = 0;

			for(jy = 0; jy < 8; jy++)
			{
				for(jx = 0; jx < 8; jx++)
				{
					ps = psY[jy] + xpos[jx];
					
					r += ps[0];
					g += ps[1];
					b += ps[2];
				}
			}

			pd[0] = r >> 6;
			pd[1] = g >> 6;
			pd[2] = b >> 6;
		}

		pd += pitchd;
	}
}

/** src の画像を dst にリサイズして描画
 *
 * dst の範囲に収まるように描画される。
 * 等倍以上にはならない。
 * 背景はあらかじめセットしておくこと。 */

void Image32_drawResize(Image32 *dst,Image32 *src)
{
	mBox box;

	if(!dst || !src) return;

	//位置とサイズ

	mBoxSet(&box, 0, 0, src->w, src->h);
	mBoxResize_keepaspect(&box, dst->w, dst->h, TRUE);

	//

	if(box.w == src->w && box.h == src->h)
		//等倍
		Image32_blt(dst, box.x, box.y, src, 0, 0, box.w, box.h);
	else
		//リサイズ
		_draw_resize(dst, src, box.x, box.y, box.w, box.h);
}


//============================================
// キャンバス描画 (イメージビューア用)
//============================================


#define FIXF_BIT  28
#define FIXF_VAL  ((int64_t)1 << FIXF_BIT)

typedef struct
{
	int srcw,srch,bpp,pitchd;
	mPixCol pixbkgnd;
	int64_t fincx,fincy,fxleft,fytop;
	mFuncPixbufSetBuf setpix;
}_drawcanvasparam;


/* 共通処理
 *
 * box: 描画先の範囲を入れておく。クリッピング後の範囲が入る。
 * return: 描画先先頭バッファ */

static uint8_t *_drawcanvas_init(Image32 *src,mPixbuf *dst,mBox *box,
	Image32CanvasInfo *info,_drawcanvasparam *param)
{
	double dscale,dscalex;
	int64_t fincx,fincy;

	//クリッピング

	if(!mPixbufClip_getBox(dst, box, box)) return NULL;

	//

	param->srcw = src->w;
	param->srch = src->h;

	param->bpp = dst->pixel_bytes;
	param->pitchd = dst->line_bytes - box->w * param->bpp;
	param->pixbkgnd = mRGBtoPix(info->bkgndcol);

	mPixbufGetFunc_setbuf(dst, &param->setpix);

	//

	dscale = dscalex = info->scalediv;

	fincx = fincy = (int64_t)(dscale * FIXF_VAL + 0.5);

	if(info->mirror)
	{
		fincx = -fincx;
		dscalex = -dscalex;
	}

	param->fincx = fincx;
	param->fincy = fincy;

	param->fxleft = (int64_t)((info->scroll_x * dscalex + info->origin_x) * FIXF_VAL) + box->x * fincx;
	param->fytop = (int64_t)((info->scroll_y * dscale + info->origin_y) * FIXF_VAL) + box->y * fincy;

	return mPixbufGetBufPtFast(dst, box->x, box->y);
}

/** キャンバス描画 (ニアレストネイバー)
 *
 * boxdst: 描画先の描画範囲 */

void Image32_drawCanvas_nearest(Image32 *src,mPixbuf *dst,
	mBox *boxdst,Image32CanvasInfo *info)
{
	uint8_t *pd,*psY,*ps;
	int ix,iy,n;
	int64_t fx,fy;
	mBox box;
	_drawcanvasparam dat;

	box = *boxdst;

	pd = _drawcanvas_init(src, dst, &box, info, &dat);
	if(!pd) return;

	//

	fy = dat.fytop;

	for(iy = box.h; iy > 0; iy--, fy += dat.fincy)
	{
		n = fy >> FIXF_BIT;

		//Y が範囲外

		if(fy < 0 || n >= dat.srch)
		{
			mPixbufBufLineH(dst, pd, box.w, dat.pixbkgnd);
			pd += dst->line_bytes;
			continue;
		}

		//X

		psY = src->buf + n * src->pitch;

		for(ix = box.w, fx = dat.fxleft; ix > 0; ix--)
		{
			n = fx >> FIXF_BIT;

			if(fx < 0 || n >= dat.srcw)
				//範囲外
				(dat.setpix)(pd, dat.pixbkgnd);
			else
			{
				ps = psY + n * 4;
			
				(dat.setpix)(pd, mRGBtoPix_sep(ps[0], ps[1], ps[2]));
			}

			fx += dat.fincx;
			pd += dat.bpp;
		}

		pd += dat.pitchd;
	}
}

/** キャンバス描画 (8x8 オーバーサンプリング) */

void Image32_drawCanvas_oversamp(Image32 *src,mPixbuf *dst,
	mBox *boxdst,Image32CanvasInfo *info)
{
	uint8_t *pd,*tbl_psY[8],*ps;
	int ix,iy,jx,jy,n,tblX[8],r,g,b;
	mBox box;
	int64_t fincx2,fincy2,fx,fy,f;
	_drawcanvasparam dat;

	box = *boxdst;

	pd = _drawcanvas_init(src, dst, &box, info, &dat);
	if(!pd) return;

	fincx2 = dat.fincx >> 3;
	fincy2 = dat.fincy >> 3;

	//

	fy = dat.fytop;

	for(iy = box.h; iy > 0; iy--, fy += dat.fincy)
	{
		n = fy >> FIXF_BIT;

		//Y が範囲外

		if(n < 0 || n >= dat.srch)
		{
			mPixbufBufLineH(dst, pd, box.w, dat.pixbkgnd);
			pd += dst->line_bytes;
			continue;
		}

		//Y テーブル

		for(jy = 0, f = fy; jy < 8; jy++, f += fincy2)
		{
			n = f >> FIXF_BIT;
			if(n >= dat.srch) n = dat.srch - 1;

			tbl_psY[jy] = src->buf + n * src->pitch;
		}

		//---- X

		for(ix = box.w, fx = dat.fxleft; ix > 0; ix--)
		{
			n = fx >> FIXF_BIT;

			if(n < 0 || n >= dat.srcw)
				//範囲外
				(dat.setpix)(pd, dat.pixbkgnd);
			else
			{
				//X テーブル

				for(jx = 0, f = fx; jx < 8; jx++, f += fincx2)
				{
					n = f >> FIXF_BIT;
					if(n >= dat.srcw) n = dat.srcw - 1;

					tblX[jx] = n * 4;
				}

				//オーバーサンプリング

				r = g = b = 0;

				for(jy = 0; jy < 8; jy++)
				{
					for(jx = 0; jx < 8; jx++)
					{
						ps = tbl_psY[jy] + tblX[jx];

						r += ps[0];
						g += ps[1];
						b += ps[2];
					}
				}
			
				(dat.setpix)(pd, mRGBtoPix_sep(r >> 6, g >> 6, b >> 6));
			}

			fx += dat.fincx;
			pd += dat.bpp;
		}

		pd += dat.pitchd;
	}
}
