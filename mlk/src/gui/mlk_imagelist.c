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
 * mImageList
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_imagelist.h"
#include "mlk_imagebuf.h"
#include "mlk_loadimage.h"
#include "mlk_pixbuf.h"
#include "mlk_string.h"
#include "mlk_guicol.h"
#include "mlk_util.h"

#if !defined(MLK_NO_ICONTHEME)
#include "mlk_icontheme.h"
#endif


//========================
// sub
//========================


/* PNG から読み込み
 *
 * pngbuf != NULL で、バッファから読み込み */

static mImageList *_load_png(const char *filename,const uint8_t *pngbuf,int bufsize,int eachw)
{
	mLoadImage li;
	mLoadImageType type;
	mImageList *img = NULL;
	char *path = NULL;
	int ret;

	mLoadImage_init(&li);
	mLoadImage_checkPNG(&type, NULL, NULL);

	//open

	if(pngbuf)
	{
		li.open.type = MLOADIMAGE_OPEN_BUF;
		li.open.buf = pngbuf;
		li.open.size = bufsize;
	}
	else
	{
		path = mGuiGetPath_data_sp(filename);

		li.open.type = MLOADIMAGE_OPEN_FILENAME;
		li.open.filename = path;
	}

	li.convert_type = MLOADIMAGE_CONVERT_TYPE_RGBA;
	li.flags = MLOADIMAGE_FLAGS_TRANSPARENT_TO_ALPHA;

	ret = (type.open)(&li);

	mFree(path);

	if(ret) goto ERR;

	//mImageList 作成

	img = mImageListNew(li.width, li.height, eachw);
	if(!img) goto ERR;

	//読み込み用バッファ

	if(!mLoadImage_allocImageFromBuf(&li, img->buf, img->pitch))
		goto ERR;

	//読み込み

	ret = (type.getimage)(&li);

	(type.close)(&li);

	mFree(li.imgbuf);

	if(ret)
	{
		mImageListFree(img);
		return NULL;
	}

	return img;

	//エラー
ERR:
	(type.close)(&li);
	mImageListFree(img);
	return NULL;
}


//========================
// main
//========================


/**@ 解放 */

void mImageListFree(mImageList *p)
{
	if(p)
	{
		mFree(p->buf);
		mFree(p);
	}
}

/**@ 作成
 *
 * @d:イメージのクリアは行われないため、必要なら明示的にクリアする。 */

mImageList *mImageListNew(int w,int h,int eachw)
{
	mImageList *p;

	p = (mImageList *)mMalloc(sizeof(mImageList));
	if(!p) return NULL;

	if(eachw > w) eachw = w;

	p->w = w;
	p->h = h;
	p->pitch = w * 4;
	p->eachw = eachw;
	p->num = w / eachw;

	//イメージバッファ

	p->buf = (uint8_t *)mMalloc(w * h * 4);
	if(!p->buf)
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/**@ PNG ファイルから読み込み
 *
 * @p:filename "!/" で始まっている場合は、アプリのデータディレクトリからの相対位置
 * @p:eachw  一つの幅 */

mImageList *mImageListLoadPNG(const char *filename,int eachw)
{
	return _load_png(filename, NULL, 0, eachw);
}

/**@ バッファから PNG データ読み込み */

mImageList *mImageListLoadPNG_buf(const uint8_t *buf,int bufsize,int eachw)
{
	return _load_png(NULL, buf, bufsize, eachw);
}

/**@ 1bit イメージから、白黒で作成
 *
 * @d:Y1列はバイト境界。ビットは下位から上位の順。\
 *  ビットONで黒、OFFは白。*/

mImageList *mImageListCreate_1bit_mono(const uint8_t *buf,int w,int h,int eachw)
{
	mImageList *p;
	int ix,iy,pitch;
	uint32_t col[2],*pd;
	const uint8_t *ps;
	uint8_t f;

	p = mImageListNew(w, h, eachw);
	if(!p) return NULL;

	col[0] = 0xffffffff;
	col[1] = 0xff000000;

	pd = (uint32_t *)p->buf;
	pitch = (w + 7) >> 3;

	for(iy = h; iy; iy--, buf += pitch)
	{
		ps = buf;
		f = 1;
		
		for(ix = w; ix; ix--)
		{
			*(pd++) = col[((*ps & f) != 0)];

			f <<= 1;
			if(!f) f = 1, ps++;
		}
	}

	return p;
}

/**@ 1bit イメージデータから作成
 *
 * @d:Y1列はバイト境界。ビットは下位から上位の順。\
 *  ビットONでテキスト色、OFFは透明。*/

mImageList *mImageListCreate_1bit_textcol(const uint8_t *buf,int w,int h,int eachw)
{
	mImageList *p;
	int ix,iy,pitch;
	uint32_t col[2],*pd;
	const uint8_t *ps;
	uint8_t f;

	p = mImageListNew(w, h, eachw);
	if(!p) return NULL;

	col[0] = 0;
	col[1] = mRGBAtoHostOrder(MGUICOL_RGB(TEXT) | 0xff000000);

	pd = (uint32_t *)p->buf;
	pitch = (w + 7) >> 3;

	for(iy = h; iy; iy--, buf += pitch)
	{
		ps = buf;
		f = 1;
		
		for(ix = w; ix; ix--)
		{
			*(pd++) = col[((*ps & f) != 0)];

			f <<= 1;
			if(!f) f = 1, ps++;
		}
	}

	return p;
}

#if !defined(MLK_NO_ICONTHEME)

/**@ アイコンテーマから、指定位置にアイコンを読み込み
 *
 * @p:pos 読み込んだアイコンをセットする位置
 * @p:names ヌル文字で区切った複数アイコン名。最後はヌル文字が2つ続く。
 * @r:すべて読み込まれたら TRUE、一つでも失敗したら FALSE */

mlkbool mImageListLoad_icontheme(mImageList *p,int pos,mIconTheme theme,const char *names,int iconsize)
{
	mImageBuf *img;
	mlkbool ret = TRUE;

	for(; *names; names += strlen(names) + 1, pos++)
	{
		img = mIconTheme_getIcon_imagebuf(theme, names, iconsize);
		if(!img)
			ret = FALSE;
		else
		{
			mImageListSetOne_imagebuf(p, pos, img);
			mImageBuf_free(img);
		}
	}

	return ret;
}

/**@ アイコンテーマから複数アイコンを読み込み、イメージリストを作成
 *
 * @p:names ヌル文字で区切った複数アイコン名。最後はヌル文字が2つ続く。\
 *  名前の数から、自動でイメージサイズが決定される。 */

mImageList *mImageListCreate_icontheme(mIconTheme theme,const char *names,int iconsize)
{
	mImageList *p;
	int num;

	//個数

	num = mStringGetNullSplitNum(names);
	if(num == 0) return NULL;

	//作成

	p = mImageListNew(iconsize * num, iconsize, iconsize);
	if(!p) return NULL;

	mImageListClear(p);

	//読み込み

	mImageListLoad_icontheme(p, 0, theme, names, iconsize);

	return p;
}
#endif

/**@ イメージをクリア (透明にする) */

void mImageListClear(mImageList *p)
{
	memset(p->buf, 0, p->pitch * p->h);
}

/**@ 指定色を透過色として、A=0 にする */

void mImageListSetTransparentColor(mImageList *p,mRgbCol col)
{
	uint8_t *pd;
	int ix,iy;
	uint32_t c;

	if(!p) return;

	pd = p->buf;
	col &= 0xffffff;

	for(iy = p->h; iy; iy--)
	{
		for(ix = p->w; ix; ix--, pd += 4)
		{
			c = MLK_RGB(pd[0], pd[1], pd[2]);

			if(c == col)
				pd[3] = 0;
		}
	}
}

/**@ 白黒画像を、テキスト色と透明に置き換え
 *
 * @d:白が透明、それ以外がテキスト色になる。 */

void mImageListReplaceTextColor_mono(mImageList *p)
{
	uint8_t *pd,r,g,b;
	uint32_t c,cnt;

	if(!p) return;

	pd = p->buf;

	c = MGUICOL_RGB(TEXT);
	r = MLK_RGB_R(c);
	g = MLK_RGB_G(c);
	b = MLK_RGB_B(c);

	for(cnt = p->w * p->h; cnt; cnt--, pd += 4)
	{
		if(pd[0] == 255 && pd[1] == 255 && pd[2] == 255)
			//白は透明
			pd[3] = 0;
		else
			pd[0] = r, pd[1] = g, pd[2] = b;
	}
}

/**@ mPixbuf に描画
 *
 * @p:index 描画する画像のインデックス位置 (0〜) */

void mImageListPutPixbuf(mImageList *p,
	mPixbuf *pixbuf,int dx,int dy,int index,uint32_t flags)
{
	mPixbufClipBlt info;
	mFuncPixbufSetBuf func;
	uint8_t *pd,*ps,disable;
	int ix,iy,pitchs,r,g,b,a,dr,dg,db;

	if(!p || index < 0 || index >= p->num) return;

	//クリッピング

	pd = mPixbufClip_getBltInfo_srcpos(pixbuf, &info, dx, dy, p->eachw, p->h,
		index * p->eachw, 0);
	if(!pd) return;
	
	//

	mPixbufGetFunc_setbuf(pixbuf, &func);

	ps = p->buf + info.sy * p->pitch + (info.sx << 2);
	pitchs = p->pitch - (info.w << 2);

	disable = ((flags & MIMAGELIST_PUTF_DISABLE) != 0);

	//

	for(iy = info.h; iy > 0; iy--)
	{
		for(ix = info.w; ix > 0; ix--)
		{
			a = ps[3];
			if(disable) a >>= 2;

			if(a)
			{
				r = ps[0];
				g = ps[1];
				b = ps[2];

				if(a != 255)
				{
					mPixbufGetPixelBufRGB(pixbuf, pd, &dr, &dg, &db);

					r = ((r - dr) * a / 255) + dr;
					g = ((g - dg) * a / 255) + dg;
					b = ((b - db) * a / 255) + db;
				}
			
				(func)(pd, mRGBtoPix_sep(r, g, b));
			}
		
			pd += info.bytes;
			ps += 4;
		}

		pd += info.pitch_dst;
		ps += pitchs;
	}
}

/**@ mImageBuf のイメージを単体アイコンとして index の位置にコピー
 *
 * @d:ソースは、32bit & 幅・高さはイメージリストの単体サイズと同じであること。 */

void mImageListSetOne_imagebuf(mImageList *p,int index,mImageBuf *src)
{
	uint8_t *ps,*pd;
	int iy,pitchs;

	if(src->width != p->eachw || src->height != p->eachw || src->pixel_bytes != 4
		|| index >= p->num)
		return;

	ps = src->buf;
	pd = p->buf + p->eachw * index * 4;

	pitchs = src->line_bytes;

	for(iy = p->eachw; iy; iy--)
	{
		memcpy(pd, ps, pitchs);

		ps += pitchs;
		pd += p->pitch;
	}
}

