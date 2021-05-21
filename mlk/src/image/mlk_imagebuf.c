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
 * mImageBuf 
 *****************************************/

#include <string.h>

#include "mlk.h"
#include "mlk_imagebuf.h"
#include "mlk_loadimage.h"



/* Y1行のバイト数取得 */

static int _get_line_bytes(int w,int bits,int line_bytes)
{
	//0 以下で自動

	if(line_bytes <= 0)
	{
		w = w * (bits >> 3);

		if(line_bytes == 0)
			line_bytes = w;
		else
		{
			//指定バイト単位
			line_bytes = -line_bytes;
			line_bytes = ((w + line_bytes - 1) / line_bytes) * line_bytes;
		}
	}

	return line_bytes;
}


//=============================
// mImageBuf
//=============================


/**@ 解放 */

void mImageBuf_free(mImageBuf *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/**@ 作成
 *
 * @g:mImageBuf
 *
 * @p:bits イメージのビット数 (24, 32)
 * @p:line_bytes Y1行のバイト数。\
 * 0 でビット数から自動計算 (余分なバイトはなし)\
 * 負の値で、自動計算の上、指定バイト単位にする (-4 なら 4byte 単位)
 * @r:NULL でメモリが足りない */

mImageBuf *mImageBuf_new(int w,int h,int bits,int line_bytes)
{
	mImageBuf *p;

	if(bits != 24 && bits != 32)
		bits = 24;

	p = (mImageBuf *)mMalloc(sizeof(mImageBuf));
	if(!p) return NULL;

	line_bytes = _get_line_bytes(w, bits, line_bytes);

	p->width = w;
	p->height = h;
	p->pixel_bytes = bits >> 3;
	p->line_bytes = line_bytes;

	//イメージバッファ確保

	p->buf = (uint8_t *)mMalloc(line_bytes * h);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/**@ 画像読み込み
 *
 * @p:line_bytes mImageBuf_new と同じ */

mImageBuf *mImageBuf_loadImage(mLoadImageOpen *open,mLoadImageType *type,
	int bits,int line_bytes)
{
	mLoadImage li;
	mImageBuf *img = NULL;
	int ret,failed = TRUE;

	if(bits != 24 && bits != 32)
		bits = 24;

	//mLoadImage

	mLoadImage_init(&li);

	li.open = *open;

	li.convert_type = (bits == 24)?
		MLOADIMAGE_CONVERT_TYPE_RGB: MLOADIMAGE_CONVERT_TYPE_RGBA;

	li.flags = MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA;

	//開く

	ret = (type->open)(&li);
	if(ret) goto END;

	//サイズ

	if(li.width > 20000 || li.height > 20000)
		goto END;

	//mImageBuf 作成

	img = mImageBuf_new(li.width, li.height, bits, line_bytes);
	if(!img) goto END;

	//imgbuf セット

	if(!mLoadImage_allocImageFromBuf(&li, img->buf, img->line_bytes))
		goto END;

	//読み込み

	ret = (type->getimage)(&li);
	if(ret) goto END;

	//終了

	failed = FALSE;

END:
	mFree(li.imgbuf);
	
	(type->close)(&li);

	if(failed)
	{
		mImageBuf_free(img);
		img = NULL;
	}

	return img;
}

/**@ 指定色を背景色としてアルファ合成し、RGB 色にする */

void mImageBuf_blendColor(mImageBuf *p,mRgbCol col)
{
	uint8_t *pd,*pdY;
	int ix,iy,r,g,b,a;

	if(p->pixel_bytes != 4) return;

	pdY = p->buf;

	r = MLK_RGB_R(col);
	g = MLK_RGB_G(col);
	b = MLK_RGB_B(col);

	for(iy = p->height; iy; iy--)
	{
		pd = pdY;
		
		for(ix = p->width; ix; ix--, pd += 4)
		{
			a = pd[3];
			
			pd[0] = (pd[0] - r) * a / 255 + r;
			pd[1] = (pd[1] - g) * a / 255 + g;
			pd[2] = (pd[2] - b) * a / 255 + b;
			pd[3] = 255;
		}

		pdY += p->line_bytes;
	}
}


//=============================
// mImageBuf2
//=============================


/* イメージバッファ確保
 *
 * alignment: 負の値で通常メモリ確保 */

static int _imgbuf2_alloc_image(mImageBuf2 *p,int alignment)
{
	uint8_t **ppbuf,*buf;
	int i,size;

	ppbuf = p->ppbuf = (uint8_t **)mMalloc0(p->height * sizeof(void *));
	if(!ppbuf) return 1;

	size = p->line_bytes;

	for(i = p->height; i > 0; i--, ppbuf++)
	{
		buf = (uint8_t *)mMallocAlign(size, alignment);
		if(!buf) return 1;

		*ppbuf = buf;
	}

	return 0;
}

/* mImageBuf2 作成 */

static mImageBuf2 *_imgbuf2_new(int w,int h,int bits,int line_bytes,int alignment)
{
	mImageBuf2 *p;

	p = (mImageBuf2 *)mMalloc(sizeof(mImageBuf2));
	if(!p) return NULL;

	line_bytes = _get_line_bytes(w, bits, line_bytes);

	p->width = w;
	p->height = h;
	p->pixel_bytes = bits >> 3;
	p->line_bytes = line_bytes;

	//イメージバッファ確保

	if(_imgbuf2_alloc_image(p, alignment))
	{
		mImageBuf2_free(p);
		return NULL;
	}

	return p;
}


/**@ イメージバッファの解放 */

void mImageBuf2_freeImage(mImageBuf2 *p)
{
	uint8_t **ppbuf = p->ppbuf;
	int i;

	if(ppbuf)
	{
		for(i = p->height; i > 0; i--, ppbuf++)
			mFree(*ppbuf);
	
		mFree(p->ppbuf);
		p->ppbuf = NULL;
	}
}

/**@ 解放 */

void mImageBuf2_free(mImageBuf2 *p)
{
	if(p)
	{
		mImageBuf2_freeImage(p);
		mFree(p);
	}
}

/**@ mImageBuf2 作成
 *
 * @g:mImageBuf2
 *
 * @p:bits イメージのビット数 (制限なし)
 * @p:line_bytes Y1行のバイト数。\
 * 0 でビット数から自動計算 (余分なバイトはなし)\
 * 負の値で、自動計算の上、指定バイト単位にする
 * @r:NULL でメモリが足りない */

mImageBuf2 *mImageBuf2_new(int w,int h,int bits,int line_bytes)
{
	return _imgbuf2_new(w, h, bits, line_bytes, -1);
}

/**@ mImageBuf2 作成
 *
 * @d:各Yのバッファは、アラインメントして確保する。
 * @p:alignment アラインメントのバイト数 */

mImageBuf2 *mImageBuf2_new_align(int w,int h,int bits,int line_bytes,int alignment)
{
	return _imgbuf2_new(w, h, bits, line_bytes, alignment);
}

/**@ イメージを切り取り (バッファは極力維持)
 *
 * @d:Y ラインのバッファは、元のまま維持する (余分なサイズが残る)。\
 * ppbuf は切り取り後の状態に調整する。\
 * また、width/height/line_bytes は新しい値に変更される。
 * @r:FALSE でメモリ確保に失敗、または、パラメータが正しくない。\
 * 失敗時は、元のイメージの状態のまま維持される。 */

mlkbool mImageBuf2_crop_keep(mImageBuf2 *p,
	int left,int top,int width,int height,int line_bytes)
{
	uint8_t **ppsrc,**ppnew;
	int i,n;

	if(left + width > p->width || top + height > p->height)
		return FALSE;

	//縦方向

	if(height != p->height)
	{
		//新しい ppbuf

		ppnew = (uint8_t **)mMalloc(height * sizeof(void *));
		if(!ppnew) return FALSE;

		ppsrc = p->ppbuf;

		//上を切り取り

		for(i = 0; i < top; i++)
			mFree(ppsrc[i]);

		//下を切り取り

		n = p->height;

		for(i = top + height; i < n; i++)
			mFree(ppsrc[i]);

		//コピー
		
		for(i = 0; i < height; i++)
			ppnew[i] = ppsrc[top + i];

		mFree(p->ppbuf);

		p->ppbuf = ppnew;
		p->height = height;
	}

	//横方向

	if(width != p->width)
	{
		ppnew = p->ppbuf;
		n = _get_line_bytes(width, p->pixel_bytes * 8, line_bytes);

		//左方向に詰める

		if(left > 0)
		{
			left *= p->pixel_bytes;
		
			for(i = height; i; i--, ppnew++)
				memmove(*ppnew, *ppnew + left, n);
		}

		p->width = width;
		p->line_bytes = n;
	}

	return TRUE;
}
