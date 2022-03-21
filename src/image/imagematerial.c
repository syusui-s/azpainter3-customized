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
 * ImageMaterial
 * 画像素材用イメージ
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_str.h"
#include "mlk_pixbuf.h"

#include "imagematerial.h"
#include "image32.h"


/*
  - Y1行は 4byte 単位。
  - テクスチャは 8bit、ブラシ画像は 8bit or 32bit。
  - 32bit の場合は R-G-B-A 順。
*/

//-------------------

#define _TOGRAY(r,g,b)  ((r * 77 + g * 150 + b * 29) >> 8)

//-------------------


/** 解放 */

void ImageMaterial_free(ImageMaterial *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/** 作成 (クリアはされない) */

ImageMaterial *ImageMaterial_new(int w,int h,int bits)
{
	ImageMaterial *p;
	int pitch;

	p = (ImageMaterial *)mMalloc0(sizeof(ImageMaterial));
	if(!p) return NULL;

	pitch = (w * (bits / 8) + 3) & ~3;

	p->buf = (uint8_t *)mMalloc(pitch * h);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	p->width = w;
	p->height = h;
	p->bits = bits;
	p->pitch = pitch;

	return p;
}

/** クリア */

void ImageMaterial_clear(ImageMaterial *p)
{
	memset(p->buf, 0, p->pitch * p->height);
}

/** テクスチャ画像 (8bit) として色を取得 */

uint8_t ImageMaterial_getPixel_forTexture(ImageMaterial *p,int x,int y)
{
	int w,h;

	w = p->width;
	h = p->height;

	if(x < 0)
		x += (-x / w + 1) * w;

	if(y < 0)
		y += (-y / h + 1) * h;

	return *(p->buf + (y % h) * p->pitch + (x % w));
}

/* Image32 からグレイスケール濃度画像セット
 *
 * src 全体を指定位置にセット */

static void _set_image_8bit(ImageMaterial *p,Image32 *src,int dx,int dy)
{
	uint8_t *pd,*ps;
	int ix,iy,pitchd,sw;

	sw = src->w;

	ps = src->buf;
	pd = p->buf + dy * p->pitch + dx;
	pitchd = p->pitch - sw;

	for(iy = src->h; iy; iy--)
	{
		for(ix = sw; ix; ix--, ps += 4)
			*(pd++) = 255 - _TOGRAY(ps[0], ps[1], ps[2]);

		pd += pitchd;
	}
}

/* Image32 から 32bit 画像セット
 *
 * src 全体を指定位置にセット */

static void _set_image_32bit(ImageMaterial *p,Image32 *src,int dx,int dy)
{
	uint8_t *pd,*ps;
	int iy,pitchd,pitchs,size;

	ps = src->buf;
	pd = p->buf + dy * p->pitch + dx * 4;
	pitchs = src->pitch;
	pitchd = p->pitch;
	size = src->w * 4;

	for(iy = src->h; iy; iy--)
	{
		memcpy(pd, ps, size);

		pd += pitchd;
		ps += pitchs;
	}
}

/** テクスチャ画像として読み込み
 *
 * RGB色をグレイスケールにして、濃度として取得 */

ImageMaterial *ImageMaterial_loadTexture(mStr *strfname)
{
	ImageMaterial *p;
	Image32 *src;

	//Image32 読み込み

	src = Image32_loadFile(strfname->buf, IMAGE32_LOAD_F_BLEND_WHITE);
	if(!src) return NULL;

	//作成

	p = ImageMaterial_new(src->w, src->h, 8);

	if(p)
	{
		p->type = IMAGEMATERIAL_TYPE_TEXTURE;
		
		_set_image_8bit(p, src, 0, 0);
	}
	
	Image32_free(src);
	
	return p;
}

/** ブラシ用画像として作成
 *
 * - src が正方形でない場合は、正方形に調整される。
 * - ファイル名の先頭が "_c_" でカラー画像。 */

ImageMaterial *ImageMaterial_loadBrush(mStr *strfname)
{
	ImageMaterial *p;
	Image32 *src;
	mStr str = MSTR_INIT;
	int dx,dy,w,h,type;

	//タイプ判別
	// 0=グレイスケール濃淡, 1=RGBA カラー

	type = 0;

	mStrPathGetBasename(&str, strfname->buf);

	if(mStrCompareEq_len(&str, "_c_", 3))
		type = 1;

	mStrFree(&str);

	//Image32 読み込み

	src = Image32_loadFile(strfname->buf,
		(type == 0)? IMAGE32_LOAD_F_BLEND_WHITE: 0);

	if(!src) return NULL;

	//サイズ (大きい方のサイズで正方形)

	w = src->w;
	h = src->h;

	if(w < h)
		w = h;
	else
		h = w;

	//作成

	p = ImageMaterial_new(w, h, (type == 0)? 8: 32);

	if(p)
	{
		p->type = IMAGEMATERIAL_TYPE_BRUSH;

		ImageMaterial_clear(p);

		dx = (w - src->w) / 2;
		dy = (h - src->h) / 2;

		if(type == 0)
			_set_image_8bit(p, src, dx, dy);
		else
			//32bit
			_set_image_32bit(p, src, dx, dy);
	}
	
	Image32_free(src);
	
	return p;
}


//==========================
// プレビュー
//==========================


/** プレビューを描画 */

void ImageMaterial_drawPreview(ImageMaterial *p,mPixbuf *pixbuf,int x,int y,int w,int h)
{
	if(p->type == IMAGEMATERIAL_TYPE_TEXTURE)
		ImageMaterial_drawPreview_texture(p, pixbuf, x, y, w, h);
	else
		ImageMaterial_drawPreview_brush(p, pixbuf, x, y, w, h);
}

/** mPixbuf にテクスチャをプレビュー描画
 *
 * 等倍でタイル状に並べる */

void ImageMaterial_drawPreview_texture(ImageMaterial *p,mPixbuf *pixbuf,int x,int y,int w,int h)
{
	mBox box;
	int ix,iy,sw,sh,pitchd,pitchs,bpp,fx,fy,fxleft;
	uint8_t *pd,*psY,c;
	mFuncPixbufSetBuf setpix;

	if(!mPixbufClip_getBox_d(pixbuf, &box, x, y, w, h))
		return;

	sw = p->width;
	sh = p->height;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->pixel_bytes;
	pitchd = pixbuf->line_bytes - box.w * bpp;
	pitchs = p->pitch;

	mPixbufGetFunc_setbuf(pixbuf, &setpix);

	fxleft = (box.x - x) % sw;
	fy = (box.y - y) % sh;

	psY = p->buf + fy * pitchs;

	//

	for(iy = box.h; iy; iy--)
	{
		fx = fxleft;

		for(ix = box.w; ix; ix--, pd += bpp)
		{
			c = 255 - *(psY + fx);
			
			(setpix)(pd, mRGBtoPix_sep(c,c,c));

			fx++;
			if(fx == sw) fx = 0;
		}

		fy++;
		if(fy == sh)
			fy = 0, psY = p->buf;
		else
			psY += pitchs;

		pd += pitchd;
	}
}

/** mPixbuf にブラシ画像プレビュー描画
 *
 * 範囲内に収まるように縮小(ニアレストネイバー)。左上寄せ。 */

void ImageMaterial_drawPreview_brush(ImageMaterial *p,mPixbuf *pixbuf,int x,int y,int w,int h)
{
	uint8_t *pd,*psY,*ps;
	int bits,bpp,pitchd,pitchs,finc,ix,iy,fx,fy,n,srcsize,r,g,b,a;
	uint32_t colex,col;
	mBox box;
	mFuncPixbufSetBuf setpix;

	if(!mPixbufClip_getBox_d(pixbuf, &box, x, y, w, h))
		return;

	pd = mPixbufGetBufPtFast(pixbuf, box.x, box.y);
	bpp = pixbuf->pixel_bytes;
	pitchd = pixbuf->line_bytes - box.w * bpp;
	srcsize = p->width;
	pitchs = p->pitch;
	bits = p->bits;

	mPixbufGetFunc_setbuf(pixbuf, &setpix);

	//

	n = (w < h)? w: h;

	finc = (int)((double)srcsize / n * (1<<16));

	colex = mRGBtoPix_sep(128,128,128);

	//

	for(iy = box.h, fy = 0; iy; iy--, fy += finc)
	{
		n = fy >> 16;
		psY = (n < srcsize)? p->buf + n * pitchs: NULL;

		for(ix = box.w, fx = 0; ix; ix--, fx += finc, pd += bpp)
		{
			n = fx >> 16;

			if(!psY || n >= srcsize)
				(setpix)(pd, colex);
			else
			{
				if(bits == 8)
				{
					n = 255 - *(psY + n);
					col = mRGBtoPix_sep(n,n,n);
				}
				else
				{
					ps = psY + (n << 2);
					a = ps[3];

					r = ((ps[0] - 220) * a >> 8) + 220;
					g = ((ps[1] - 220) * a >> 8) + 220;
					b = ((ps[2] - 220) * a >> 8) + 220;
					
					col = mRGBtoPix_sep(r, g, b);
				}
				
				(setpix)(pd, col);
			}
		}

		pd += pitchd;
	}
}


