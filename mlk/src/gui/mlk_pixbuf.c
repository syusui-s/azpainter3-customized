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
 * mPixbuf
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_pixbuf.h"
#include "mlk_rectbox.h"
#include "mlk_guicol.h"
#include "mlk_imagebuf.h"

#include "mlk_pv_gui.h"
#include "mlk_pv_pixbuf.h"


/*-------------
[data]
  mPixbufBase
    - mPixbuf
    - mPixbufPrivate
  バッグエンド用データ
---------------*/


//-----------------

#define _GETPTBUF_ABS(p,x,y)  (p->buf + (y) * p->line_bytes + (x) * p->pixel_bytes)

//各イメージのビットパターン

const static uint8_t g_pat_arrow_down_5x3[] = { 0x1f,0x0e,0x04 },
	g_pat_arrow_up_5x3[] = { 0x04,0x0e,0x1f },
	g_pat_arrow_left_3x5[] = { 0x04,0x06,0x07,0x06,0x04 },
	g_pat_arrow_right_3x5[] = { 0x01,0x03,0x07,0x03,0x01 },

	g_pat_arrow_down_7x4[] = { 0x7f,0x3e,0x1c,0x08 },
	g_pat_arrow_up_7x4[] = { 0x08,0x1c,0x3e,0x7f },
	g_pat_arrow_left_4x7[] = { 0x08,0x0c,0x0e,0x0f,0x0e,0x0c,0x08 },
	g_pat_arrow_right_4x7[] = { 0x01,0x03,0x07,0x0f,0x07,0x03,0x01 },

	g_pat_arrow_down_9x5[] = { 0xff,0x01,0xfe,0x00,0x7c,0x00,0x38,0x00,0x10,0x00 },
	g_pat_arrow_up_9x5[] = { 0x10,0x00,0x38,0x00,0x7c,0x00,0xfe,0x00,0xff,0x01 },
	g_pat_arrow_left_5x9[] = { 0x10,0x18,0x1c,0x1e,0x1f,0x1e,0x1c,0x18,0x10 },
	g_pat_arrow_right_5x9[] = { 0x01,0x03,0x07,0x0f,0x1f,0x0f,0x07,0x03,0x01 },

	g_pat_checked[] = { 0x00,0x02,0x00,0x03,0x80,0x03,0xc3,0x03,0xe7,0x01,0xff,0x00,0x7e,0x00,0x3c,0x00,0x18,0x00 },
	g_pat_menu_checked[] = { 0x80,0xc0,0x60,0x31,0x1b,0x0e,0x04 },
	g_pat_menu_radio[] = { 0x0e,0x1f,0x1f,0x1f,0x0e },

	//13x13
	g_pat_radiobox_off_2bit[] = { 0x0a,0x00,0x80,0x02,0xf2,0xff,0x3f,0x02,0xfc,0xff,0xff,0x00,0xfc,0xff,0xff,0x00,
		0xfc,0xff,0xff,0x00,0xfc,0xff,0xff,0x00,0xfc,0xff,0xff,0x00,0xfc,0xff,0xff,0x00,
		0xfc,0xff,0xff,0x00,0xfc,0xff,0xff,0x00,0xfc,0xff,0xff,0x00,0xf2,0xff,0x3f,0x02,
		0x0a,0x00,0x80,0x02 },
	g_pat_radiobox_on_2bit[] = { 0x0a,0x00,0x80,0x02,0xf2,0xff,0x3f,0x02,0xfc,0xff,0xff,0x00,0xfc,0x55,0xfd,0x00,
		0x7c,0x55,0xf5,0x00,0x7c,0x55,0xf5,0x00,0x7c,0x55,0xf5,0x00,0x7c,0x55,0xf5,0x00,
		0x7c,0x55,0xf5,0x00,0xfc,0x55,0xfd,0x00,0xfc,0xff,0xff,0x00,0xf2,0xff,0x3f,0x02,
		0x0a,0x00,0x80,0x02 },

	//42x7 = ['0'-'9'] (w4), ['.'] (w2)
	g_pat_number_5x7[] = {
		0x6f,0xff,0xf9,0xff,0xff,0x00,0x49,0x88,0x19,0x91,0x99,0x00,
		0x49,0x88,0x19,0x91,0x99,0x00,0x49,0xff,0xff,0x8f,0xff,0x00,
		0x49,0x81,0x88,0x89,0x89,0x00,0x49,0x81,0x88,0x89,0x89,0x03,
		0xef,0xff,0xf8,0x8f,0x8f,0x03 };

//-----------------


//====================================
// sub
//====================================


/** root クリッピング範囲セット
 *
 * rc: 絶対位置。NULL でクリア
 * return: FALSE でイメージ範囲外 */

mlkbool __mPixbufSetClipRoot(mPixbuf *pixbuf,mRect *rc)
{
	mPixbufBase *p = MPIXBUFBASE(pixbuf);

	if(!rc)
		//クリア
		p->pv.fclip = 0;
	else
	{
		//セット

		if(!mRectClipBox_d(rc, 0, 0, pixbuf->width, pixbuf->height))
			return FALSE;
		else
		{
			p->pv.rcclip = *rc;
			p->pv.rcclip_root = *rc;
			p->pv.fclip = MPIXBUF_CLIPF_HAVE | MPIXBUF_CLIPF_HAVE_ROOT;
		}
	}

	return TRUE;
}

/** 直線描画
 *
 * exclude_end: TRUE で終点を描画しない */

static void _draw_line(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix,mlkbool exclude_end)
{
	int sx,sy,dx,dy,a,a1,e;

	if(x1 == x2 && y1 == y2 && !exclude_end)
	{
		mPixbufSetPixel(p, x1, y1, pix);
		return;
	}

	//

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;
	sx = (x1 <= x2)? 1: -1;
	sy = (y1 <= y2)? 1: -1;

	if(dx >= dy)
	{
		a  = 2 * dy;
		a1 = a - 2 * dx;
		e  = a - dx;

		while(x1 != x2)
		{
			mPixbufSetPixel(p, x1, y1, pix);

			if(e >= 0)
				y1 += sy, e += a1;
			else
				e += a;

			x1 += sx;
		}
	}
	else
	{
		a  = 2 * dx;
		a1 = a - 2 * dy;
		e  = a - dy;

		while(y1 != y2)
		{
			mPixbufSetPixel(p, x1, y1, pix);

			if(e >= 0)
				x1 += sx, e += a1;
			else
				e += a;

			y1 += sy;
		}
	}

	if(!exclude_end)
		mPixbufSetPixel(p, x1, y1, pix);
}


//====================================
// 単体関数
//====================================


/**@ PIX -> RGB 色変換 */

mRgbCol mPixtoRGB(mPixCol c)
{
	return (MLKAPP->bkend.pixtorgb)(c);
}

/**@ RGB -> PIX 色変換 */

mPixCol mRGBtoPix(mRgbCol c)
{
	return (MLKAPP->bkend.rgbtopix)(MLK_RGB_R(c), MLK_RGB_G(c), MLK_RGB_B(c));
}

/**@ RGB -> PIX 色変換 */

mPixCol mRGBtoPix_sep(uint8_t r,uint8_t g,uint8_t b)
{
	return (MLKAPP->bkend.rgbtopix)(r, g, b);
}

/**@ RGB 値をアルファ値で合成して、PIX 色取得
 *
 * @p:a アルファ値 (0〜256) */

mPixCol mRGBtoPix_blend256(mRgbCol src,mRgbCol dst,int a)
{
	src = mBlendRGB_a256(src, dst, a);

	return (MLKAPP->bkend.rgbtopix)(MLK_RGB_R(src), MLK_RGB_G(src), MLK_RGB_B(src));
}


//====================================
// 色のセット関数
//====================================


/**@ setbuf 関数 (アルファ合成)
 *
 * @p:col 下位 24bit は RGB 値。上位 8bit が不透明度 (0-128)。 */

void mPixbufSetBufFunc_blend128(uint8_t *buf,mPixCol col)
{
	int r,g,b,a;
	uint32_t dcol;

	dcol = (MLKAPP->bkend.pixbuf_getbuf)(buf);
	dcol = (MLKAPP->bkend.pixtorgb)(dcol);

	r = MLK_RGB_R(dcol);
	g = MLK_RGB_G(dcol);
	b = MLK_RGB_B(dcol);

	a = (uint8_t)(col >> 24);
	r = ((MLK_RGB_R(col) - r) * a >> 7) + r;
	g = ((MLK_RGB_G(col) - g) * a >> 7) + g;
	b = ((MLK_RGB_B(col) - b) * a >> 7) + b;
	
	(MLKAPP->bkend.pixbuf_setbufcol)(buf, mRGBtoPix_sep(r, g, b));
}


//====================================
// main
//====================================


/**@ 解放 */

void mPixbufFree(mPixbuf *p)
{
	if(p)
	{
		(MLKAPP->bkend.pixbuf_deleteimg)(p);
		
		mFree(p);
	}
}

/**@ 作成 */

mPixbuf *mPixbufCreate(int w,int h,uint8_t flags)
{
	mPixbuf *p;
	
	if(w < 1) w = 1;
	if(h < 1) h = 1;
	
	p = (MLKAPP->bkend.pixbuf_alloc)();
	if(!p) return NULL;

	MPIXBUFBASE(p)->pv.fcreate = flags;

	if(!(MLKAPP->bkend.pixbuf_create)(p, w, h))
	{
		mFree(p);
		return NULL;
	}

	MPIXBUFBASE(p)->pv.setbuf = MLKAPP->bkend.pixbuf_setbufcol;
	
	return p;
}

/**@ サイズ変更
 *
 * @p:stepw,steph  単位。\
 * 実際のサイズはこの値の倍数になる。0 以下で指定サイズ。
 * @r:FALSE で失敗 */

mlkbool mPixbufResizeStep(mPixbuf *p,int w,int h,int stepw,int steph)
{
	int neww,newh;

	if(w < 1) w = 1;
	if(h < 1) h = 1;

	if(stepw > 0)
		neww = (w + stepw - 1) / stepw * stepw;
	else
		neww = w;

	if(steph > 0)
		newh = (h + steph - 1) / steph * steph;
	else
		newh = h;
	
	return (MLKAPP->bkend.pixbuf_resize)(p, w, h, neww, newh);
}

/**@ オフセット位置セット
 * 
 * @p:pt NULL 以外で、元のオフセット位置が入る */

void mPixbufSetOffset(mPixbuf *p,int x,int y,mPoint *pt)
{
	mPixbufBase *pb = MPIXBUFBASE(p);

	if(pt)
	{
		pt->x = pb->pv.offsetX;
		pt->y = pb->pv.offsetY;
	}

	pb->pv.offsetX = x;
	pb->pv.offsetY = y;
}

/**@ ピクセル描画を XOR モードにする */

void mPixbufSetPixelModeXor(mPixbuf *p)
{
	MPIXBUFBASE(p)->pv.setbuf = MLKAPP->bkend.pixbuf_setbufxor;
}

/**@ ピクセル描画を通常モードにする */

void mPixbufSetPixelModeCol(mPixbuf *p)
{
	MPIXBUFBASE(p)->pv.setbuf = MLKAPP->bkend.pixbuf_setbufcol;
}

/**@ バッファに色をセットする関数を変更 */

void mPixbufSetFunc_setbuf(mPixbuf *p,mFuncPixbufSetBuf func)
{
	MPIXBUFBASE(p)->pv.setbuf = func;
}

/**@ バッファに色をセットする関数を取得 */

void mPixbufGetFunc_setbuf(mPixbuf *p,mFuncPixbufSetBuf *dst)
{
	*dst = MPIXBUFBASE(p)->pv.setbuf;
}

/**@ バッファから色を取得する関数を取得 */

void mPixbufGetFunc_getbuf(mPixbuf *p,mFuncPixbufGetBuf *dst)
{
	*dst = MLKAPP->bkend.pixbuf_getbuf;
}


//===========================
// クリッピング
//===========================


/**@ クリッピング範囲をクリアする
 *
 * @g:クリッピング */

void mPixbufClip_clear(mPixbuf *p)
{
	mPixbufBase *pb = MPIXBUFBASE(p);

	if(pb->pv.fclip & MPIXBUF_CLIPF_HAVE_ROOT)
	{
		//root がある場合は、root の範囲で再セット

		pb->pv.rcclip = pb->pv.rcclip_root;
		pb->pv.fclip = MPIXBUF_CLIPF_HAVE | MPIXBUF_CLIPF_HAVE_ROOT;
	}
	else
		pb->pv.fclip = 0;
}

/**@ クリッピング範囲セット
 *
 * @r:描画可能な範囲があるか */

mlkbool mPixbufClip_setBox_d(mPixbuf *p,int x,int y,int w,int h)
{
	mPixbufBase *pb = MPIXBUFBASE(p);
	mlkbool ret;
	mRect rc;

	mRectSetBox_d(&rc, x + pb->pv.offsetX, y + pb->pv.offsetY, w, h);

	if(pb->pv.fclip & MPIXBUF_CLIPF_HAVE_ROOT)
		//root 範囲あり: root の範囲内に
		ret = mRectClipRect(&rc, &pb->pv.rcclip_root);
	else
		//イメージ全体の範囲内に
		ret = mRectClipBox_d(&rc, 0, 0, p->width, p->height);

	pb->pv.rcclip = rc;
	pb->pv.fclip |= MPIXBUF_CLIPF_HAVE;

	//範囲外ならフラグセット

	if(ret)
		pb->pv.fclip &= ~MPIXBUF_CLIPF_OUT;
	else
		pb->pv.fclip |= MPIXBUF_CLIPF_OUT;

	return ret;
}

/**@ クリッピング範囲セット (mBox)
 *
 * @r:描画範囲があるか */

mlkbool mPixbufClip_setBox(mPixbuf *p,const mBox *box)
{
	return mPixbufClip_setBox_d(p, box->x, box->y, box->w, box->h);
}

/**@ 指定位置がクリッピング範囲内にあるか (絶対位置)
 *
 * @d:クリッピング範囲が設定されていない場合は、イメージ範囲で判定。 */

mlkbool mPixbufClip_isPointIn_abs(mPixbuf *p,int x,int y)
{
	mPixbufBase *pb = MPIXBUFBASE(p);
	mRect rc;

	if(pb->pv.fclip & MPIXBUF_CLIPF_HAVE)
	{
		//クリッピング範囲あり

		if(pb->pv.fclip & MPIXBUF_CLIPF_OUT) return FALSE;

		rc = pb->pv.rcclip;
	}
	else
		//イメージ範囲
		mRectSetBox_d(&rc, 0, 0, p->width, p->height);

	//範囲内か

	return (rc.x1 <= x && x <= rc.x2 && rc.y1 <= y && y <= rc.y2);
}

/**@ 指定範囲をクリッピング範囲内に収めて mRect 取得
 *
 * @r:描画可能な範囲があるか */

mlkbool mPixbufClip_getRect_d(mPixbuf *p,mRect *rcdst,int x,int y,int w,int h)
{
	mPixbufBase *pb = MPIXBUFBASE(p);
	mRect rc;
	int offx,offy;
	mlkbool ret;

	if(pb->pv.fclip & MPIXBUF_CLIPF_OUT) return FALSE;

	offx = pb->pv.offsetX;
	offy = pb->pv.offsetY;

	rc.x1 = x + offx;
	rc.y1 = y + offy;
	rc.x2 = rc.x1 + w - 1;
	rc.y2 = rc.y1 + h - 1;

	if(pb->pv.fclip & MPIXBUF_CLIPF_HAVE)
		ret = mRectClipRect(&rc, &pb->pv.rcclip);
	else
		ret = mRectClipBox_d(&rc, 0, 0, p->width, p->height);

	if(ret)
	{
		rc.x1 -= offx;
		rc.x2 -= offx;
		rc.y1 -= offy;
		rc.y2 -= offy;

		*rcdst = rc;
	}

	return ret;
}

/**@ 指定範囲をクリッピング範囲内に収めて mBox で取得 */

mlkbool mPixbufClip_getBox_d(mPixbuf *p,mBox *box,int x,int y,int w,int h)
{
	mRect rc;

	if(!mPixbufClip_getRect_d(p, &rc, x, y, w, h))
		return FALSE;
	else
	{
		box->x = rc.x1, box->y = rc.y1;
		box->w = rc.x2 - rc.x1 + 1;
		box->h = rc.y2 - rc.y1 + 1;

		return TRUE;
	}
}

/**@ mBox の範囲をクリッピング範囲内に収めて mBox で取得
 *
 * @p:boxdst 範囲のセット先。boxdst = boxsrc でも可。
 * @p:boxsrc クリッピングする範囲 */

mlkbool mPixbufClip_getBox(mPixbuf *p,mBox *boxdst,const mBox *boxsrc)
{
	return mPixbufClip_getBox_d(p, boxdst,
			boxsrc->x, boxsrc->y, boxsrc->w, boxsrc->h);
}

/**@ 指定範囲をクリッピング範囲内に収めて、転送用の情報を取得
 *
 * @r:転送先の左上のバッファ位置。NULL で範囲外。 */

uint8_t *mPixbufClip_getBltInfo(mPixbuf *p,mPixbufClipBlt *dst,int x,int y,int w,int h)
{
	mBox box;

	if(!mPixbufClip_getBox_d(p, &box, x, y, w, h)) return NULL;

	dst->sx = box.x - x;
	dst->sy = box.y - y;
	dst->w  = box.w;
	dst->h  = box.h;
	dst->bytes = p->pixel_bytes;
	dst->pitch_dst = p->line_bytes - box.w * p->pixel_bytes;

	return mPixbufGetBufPtFast(p, box.x, box.y);
}

/**@ 指定範囲をクリッピング範囲内に収めて、転送用の情報を取得
 *
 * @p:x,y,w,h 転送先範囲
 * @p:sx,sy 転送元の位置
 * @r:転送先左上のバッファ位置。NULL で範囲外。 */

uint8_t *mPixbufClip_getBltInfo_srcpos(mPixbuf *p,mPixbufClipBlt *dst,
	int x,int y,int w,int h,int sx,int sy)
{
	mBox box;

	if(!mPixbufClip_getBox_d(p, &box, x, y, w, h)) return NULL;

	dst->sx = sx + (box.x - x);
	dst->sy = sy + (box.y - y);
	dst->w  = box.w;
	dst->h  = box.h;
	dst->bytes = p->pixel_bytes;
	dst->pitch_dst = p->line_bytes - box.w * p->pixel_bytes;

	return mPixbufGetBufPtFast(p, box.x, box.y);
}


//===========================
// 位置
//===========================


/**@ ピクセルバッファ位置取得
 *
 * @g:ピクセル
 *
 * @d:クリッピング範囲は関係ない。
 * 
 * @r:範囲外は NULL */

uint8_t *mPixbufGetBufPt(mPixbuf *p,int x,int y)
{
	x += MPIXBUFBASE(p)->pv.offsetX;
	y += MPIXBUFBASE(p)->pv.offsetY;

	if(x >= 0 && x < p->width && y >= 0 && y < p->height)
		return _GETPTBUF_ABS(p, x, y);
	else
		return NULL;
}

/**@ ピクセルバッファ位置取得
 *
 * @d:範囲判定なしの高速用。\
 * オフセット位置は適用される。 */

uint8_t *mPixbufGetBufPtFast(mPixbuf *p,int x,int y)
{
	return p->buf + (y + MPIXBUFBASE(p)->pv.offsetY) * p->line_bytes
			+ (x + MPIXBUFBASE(p)->pv.offsetX) * p->pixel_bytes;
}

/**@ ピクセルバッファ位置取得 (絶対位置指定)
 *
 * @d:オフセット位置関係なく、バッファの絶対位置で指定する。
 * 
 * @r:範囲外は NULL */

uint8_t *mPixbufGetBufPt_abs(mPixbuf *p,int x,int y)
{
	if(x >= 0 && x < p->width && y >= 0 && y < p->height)
		return p->buf + y * p->line_bytes + x * p->pixel_bytes;
	else
		return NULL;
}

/**@ ピクセルバッファ位置取得 (絶対位置・範囲判定なし)
 *
 * @d:指定位置がイメージの範囲内であるかどうかの判定は行わないため、
 *  常に範囲内であるとわかっている場合のみ使う。 */

uint8_t *mPixbufGetBufPtFast_abs(mPixbuf *p,int x,int y)
{
	return p->buf + y * p->line_bytes + x * p->pixel_bytes;
}

/**@ ピクセルバッファ位置取得 (クリッピング処理あり)
 *
 * @r:クリッピング範囲外なら NULL */

uint8_t *mPixbufGetBufPt_clip(mPixbuf *p,int x,int y)
{
	x += MPIXBUFBASE(p)->pv.offsetX;
	y += MPIXBUFBASE(p)->pv.offsetY;

	if(!mPixbufClip_isPointIn_abs(p, x, y))
		return NULL;

	return p->buf + y * p->line_bytes + x * p->pixel_bytes;
}

/**@ 指定位置の色を RGB で取得
 *
 * @d:クリッピング範囲は適用されない。範囲外なら 0 が返る。 */

mRgbCol mPixbufGetPixelRGB(mPixbuf *p,int x,int y)
{
	uint32_t col;

	x += MPIXBUFBASE(p)->pv.offsetX;
	y += MPIXBUFBASE(p)->pv.offsetY;

	if(x < 0 || x >= p->width || y < 0 || y >= p->height)
		return 0;
	else
	{
		col = (MLKAPP->bkend.pixbuf_getbuf)(_GETPTBUF_ABS(p, x, y));
		
		return (MLKAPP->bkend.pixtorgb)(col);
	}
}

/**@ ピクセルバッファ位置から RGB の各値を取得 */

void mPixbufGetPixelBufRGB(mPixbuf *p,uint8_t *buf,int *pr,int *pg,int *pb)
{
	uint32_t col;

	col = (MLKAPP->bkend.pixbuf_getbuf)(buf);
	col = (MLKAPP->bkend.pixtorgb)(col);

	*pr = MLK_RGB_R(col);
	*pg = MLK_RGB_G(col);
	*pb = MLK_RGB_B(col);
}


//===========================
// 描画
//===========================


/**@ 点を描画 (PIXCOL)
 *
 * @g:描画 */

void mPixbufSetPixel(mPixbuf *p,int x,int y,mPixCol pix)
{
	uint8_t *buf;

	buf = mPixbufGetBufPt_clip(p, x, y);
	if(buf)
		(MPIXBUFBASE(p)->pv.setbuf)(buf, pix);
}

/**@ 点を描画 (RGB) */

void mPixbufSetPixelRGB(mPixbuf *p,int x,int y,mRgbCol rgb)
{
	uint8_t *buf;

	buf = mPixbufGetBufPt_clip(p, x, y);
	if(buf)
		(MPIXBUFBASE(p)->pv.setbuf)(buf, mRGBtoPix(rgb));
}

/**@ 点を描画 (RGB、各値を指定) */

void mPixbufSetPixelRGB_sep(mPixbuf *p,int x,int y,uint8_t r,uint8_t g,uint8_t b)
{
	uint8_t *buf;

	buf = mPixbufGetBufPt_clip(p, x, y);
	if(buf)
		(MPIXBUFBASE(p)->pv.setbuf)(buf, mRGBtoPix_sep(r, g, b));
}

/**@ 複数の点を描画 */

void mPixbufSetPixels(mPixbuf *p,mPoint *pt,int num,mPixCol pix)
{
	mFuncPixbufSetBuf setbuf = MPIXBUFBASE(p)->pv.setbuf;
	uint8_t *buf;
	
	for(; num > 0; num--, pt++)
	{
		buf = mPixbufGetBufPt_clip(p, pt->x, pt->y);
		if(buf)
			(setbuf)(buf, pix);
	}
}

/**@ RGB 色と指定位置の色をアルファ合成してセット
 *
 * @p:a アルファ値 (0-255) */

void mPixbufBlendPixel(mPixbuf *p,int x,int y,mRgbCol rgb,int a)
{
	mRgbCol bg;
	int r,g,b;

	bg = mPixbufGetPixelRGB(p, x, y);
	
	r = MLK_RGB_R(bg);
	g = MLK_RGB_G(bg);
	b = MLK_RGB_B(bg);
	
	r = (MLK_RGB_R(rgb) - r) * a / 255 + r;
	g = (MLK_RGB_G(rgb) - g) * a / 255 + g;
	b = (MLK_RGB_B(rgb) - b) * a / 255 + b;

	mPixbufSetPixelRGB_sep(p, x, y, r, g, b);
}

/**@ RGB 色と指定位置の色をアルファ合成してセット (LCD 用)
 *
 * @p:ra,ga,ba 各色のアルファ値 (0-255) */

void mPixbufBlendPixel_lcd(mPixbuf *p,int x,int y,mRgbCol rgb,uint8_t ra,uint8_t ga,uint8_t ba)
{
	mRgbCol bg;
	int r,g,b;

	bg = mPixbufGetPixelRGB(p, x, y);
	
	r = MLK_RGB_R(bg);
	g = MLK_RGB_G(bg);
	b = MLK_RGB_B(bg);
	
	r = (MLK_RGB_R(rgb) - r) * ra / 255 + r;
	g = (MLK_RGB_G(rgb) - g) * ga / 255 + g;
	b = (MLK_RGB_B(rgb) - b) * ba / 255 + b;

	mPixbufSetPixelRGB_sep(p, x, y, r, g, b);
}

/**@ RGB 値と合成して点をセット
 *
 * @p:col 下位 24bit は RGB 値。上位 8bit が不透明度 (0-128)。 */

void mPixbufBlendPixel_a128(mPixbuf *p,int x,int y,uint32_t col)
{
	uint8_t *buf;

	buf = mPixbufGetBufPt_clip(p, x, y);
	if(buf)
		mPixbufBlendPixel_a128_buf(p, buf, col);
}

/**@ RGB 値と合成して点をセット (バッファ位置指定)
 *
 * @p:col 下位 24bit は RGB 値。上位 8bit が不透明度 (0-128)。 */

void mPixbufBlendPixel_a128_buf(mPixbuf *p,uint8_t *buf,uint32_t col)
{
	int r,g,b,a;

	mPixbufGetPixelBufRGB(p, buf, &r, &g, &b);

	a = (uint8_t)(col >> 24);
	r = ((MLK_RGB_R(col) - r) * a >> 7) + r;
	g = ((MLK_RGB_G(col) - g) * a >> 7) + g;
	b = ((MLK_RGB_B(col) - b) * a >> 7) + b;
	
	(MPIXBUFBASE(p)->pv.setbuf)(buf, mRGBtoPix_sep(r, g, b));
}


//------------------


/**@ バッファに水平線描画
 *
 * @d:クリッピングなどは行われない。 */

void mPixbufBufLineH(mPixbuf *p,uint8_t *buf,int w,mPixCol pix)
{
	mFuncPixbufSetBuf setbuf = MPIXBUFBASE(p)->pv.setbuf;
	int bpp = p->pixel_bytes;

	for(; w > 0; w--, buf += bpp)
		(setbuf)(buf, pix);
}

/**@ バッファに垂直線描画
 *
 * @d:クリッピングなどは行われない。 */

void mPixbufBufLineV(mPixbuf *p,uint8_t *buf,int h,mPixCol pix)
{
	mFuncPixbufSetBuf setbuf = MPIXBUFBASE(p)->pv.setbuf;
	int pitch = p->line_bytes;

	for(; h > 0; h--, buf += pitch)
		(setbuf)(buf, pix);
}

/**@ 水平線描画 */

void mPixbufLineH(mPixbuf *p,int x,int y,int w,mPixCol pix)
{
	mRect rc;

	if(mPixbufClip_getRect_d(p, &rc, x, y, w, 1))
	{
		mPixbufBufLineH(p,
			mPixbufGetBufPtFast(p, rc.x1, rc.y1), rc.x2 - rc.x1 + 1, pix);
	}
}

/**@ 垂直線描画 */

void mPixbufLineV(mPixbuf *p,int x,int y,int h,mPixCol pix)
{
	mRect rc;

	if(mPixbufClip_getRect_d(p, &rc, x, y, 1, h))
	{
		mPixbufBufLineV(p,
			mPixbufGetBufPtFast(p, rc.x1, rc.y1), rc.y2 - rc.y1 + 1, pix);
	}
}

/**@ 水平線描画 (RGB 合成)
 *
 * @p:a アルファ値 (0-256) */

void mPixbufLineH_blend256(mPixbuf *p,int x,int y,int w,mRgbCol rgb,int a)
{
	mFuncPixbufSetBuf func;
	mRect rc;
	uint8_t *pd;
	int ix,r,g,b,sr,sg,sb;

	if(!mPixbufClip_getRect_d(p, &rc, x, y, w, 1))
		return;

	func = MPIXBUFBASE(p)->pv.setbuf;
	
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);

	sr = MLK_RGB_R(rgb);
	sg = MLK_RGB_G(rgb);
	sb = MLK_RGB_B(rgb);

	for(ix = rc.x1; ix <= rc.x2; ix++, pd += p->pixel_bytes)
	{
		mPixbufGetPixelBufRGB(p, pd, &r, &g, &b);
	
		r = ((sr - r) * a >> 8) + r;
		g = ((sg - g) * a >> 8) + g;
		b = ((sb - b) * a >> 8) + b;
	
		(func)(pd, mRGBtoPix_sep(r, g, b));
	}
}

/**@ 垂直線描画 (RGB 合成) */

void mPixbufLineV_blend256(mPixbuf *p,int x,int y,int h,mRgbCol rgb,int a)
{
	mFuncPixbufSetBuf func;
	mRect rc;
	uint8_t *pd;
	int iy,r,g,b,sr,sg,sb;

	if(!mPixbufClip_getRect_d(p, &rc, x, y, 1, h))
		return;
	
	func = MPIXBUFBASE(p)->pv.setbuf;

	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);

	sr = MLK_RGB_R(rgb);
	sg = MLK_RGB_G(rgb);
	sb = MLK_RGB_B(rgb);

	for(iy = rc.y1; iy <= rc.y2; iy++, pd += p->line_bytes)
	{
		mPixbufGetPixelBufRGB(p, pd, &r, &g, &b);
	
		r = ((sr - r) * a >> 8) + r;
		g = ((sg - g) * a >> 8) + g;
		b = ((sb - b) * a >> 8) + b;
	
		(func)(pd, mRGBtoPix_sep(r, g, b));
	}
}

/**@ 直線描画 */

void mPixbufLine(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix)
{
	_draw_line(p, x1, y1, x2, y2, pix, FALSE);
}

/**@ 直線描画 (終点を描画しない) */

void mPixbufLine_excludeEnd(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix)
{
	_draw_line(p, x1, y1, x2, y2, pix, TRUE);
}

/**@ 各点をつなげた直線を描画
 *
 * @d:各終点は描画しない。\
 * 最後の点と最初の点を結んで閉じる。
 * 
 * @p:num 点の数 (最低3個) */

void mPixbufLines_close(mPixbuf *p,mPoint *pt,int num,mPixCol pix)
{
	int i;

	if(num < 3) return;

	for(i = 0; i < num - 1; i++)
		_draw_line(p, pt[i].x, pt[i].y, pt[i + 1].x, pt[i + 1].y, pix, TRUE);

	//最後

	_draw_line(p, pt[num - 1].x, pt[num - 1].y, pt[0].x, pt[0].y, pix, TRUE);
}

/**@ 四角形枠 */

void mPixbufBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	mRect rc;
	int ww,hh;

	if(!mPixbufClip_getRect_d(p, &rc, x, y, w, h)) return;
	
	ww = rc.x2 - rc.x1 + 1;
	hh = rc.y2 - rc.y1 + 1;

	//左
	
	if(rc.x1 == x)
		mPixbufBufLineV(p, mPixbufGetBufPtFast(p, rc.x1, rc.y1), hh, pix);
	
	//右
	
	if(rc.x2 == x + w - 1)
		mPixbufBufLineV(p, mPixbufGetBufPtFast(p, rc.x2, rc.y1), hh, pix);
	
	//上
		
	if(rc.y1 == y)
		mPixbufBufLineH(p, mPixbufGetBufPtFast(p, rc.x1, rc.y1), ww, pix);
	
	//下
	
	if(rc.y2 == y + h - 1)
		mPixbufBufLineH(p, mPixbufGetBufPtFast(p, rc.x1, rc.y2), ww, pix);
}

/**@ 四角形枠 (ピクセル単位で描画)
 *
 * @d:mPixbufBox() は直線単位で描画するため、点が重なる部分がある。\
 * XOR で描画する場合はこちらを使う。 */

void mPixbufBox_pixel(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	int i,n;

	//横線

	n = y + h - 1;

	for(i = 0; i < w; i++)
	{
		mPixbufSetPixel(p, x + i, y, pix);

		if(h > 1) mPixbufSetPixel(p, x + i, n, pix);
	}

	//縦線

	n = x + w - 1;

	for(i = 1; i < h - 1; i++)
	{
		mPixbufSetPixel(p, x, y + i, pix);

		if(w > 1) mPixbufSetPixel(p, n, y + i, pix);
	}
}

/**@ 楕円枠描画 */

void mPixbufEllipse(mPixbuf *p,int x1,int y1,int x2,int y2,mPixCol pix)
{
	double dx,dy,e,e2;
	int a,b,b1;

	if(x1 == x2 && y1 == y2)
	{
		mPixbufSetPixel(p, x1, y1, pix);
		return;
	}

	a = (x1 < x2)? x2 - x1: x1 - x2;
	b = (y1 < y2)? y2 - y1: y1 - y2;

	if(a >= b)
	{
		//横長

		b1 = b & 1;
		dx = 4.0 * (1 - a) * b * b;
		dy = 4.0 * (b1 + 1) * a * a;
		e = dx + dy + b1 * a * a;

		if(x1 > x2) x1 = x2, x2 += a;
		if(y1 > y2) y1 = y2;

		y1 += (b + 1) / 2;
		y2 = y1 - b1;

		a *= 8 * a;
		b1 = 8 * b * b;

		do
		{
			mPixbufSetPixel(p, x2, y1, pix);
			if(x1 != x2) mPixbufSetPixel(p, x1, y1, pix);
			if(y1 != y2) mPixbufSetPixel(p, x1, y2, pix);
			if(x1 != x2 && y1 != y2) mPixbufSetPixel(p, x2, y2, pix);

			e2 = 2 * e;
			if(e2 <= dy) { y1++; y2--; e += dy += a; }
			if(e2 >= dx || 2 * e > dy) { x1++; x2--; e += dx += b1; }
		} while(x1 <= x2);
	}
	else
	{
		//縦長

		b1 = a & 1;
		dy = 4.0 * (1 - b) * a * a;
		dx = 4.0 * (b1 + 1) * b * b;
		e = dx + dy + b1 * b * b;

		if(y1 > y2) y1 = y2, y2 += b;
		if(x1 > x2) x1 = x2;

		x1 += (a + 1) / 2;
		x2 = x1 - b1;

		b *= 8 * b;
		b1 = 8 * a * a;

		do
		{
			mPixbufSetPixel(p, x2, y1, pix);
			if(x1 != x2) mPixbufSetPixel(p, x1, y1, pix);
			if(y1 != y2) mPixbufSetPixel(p, x1, y2, pix);
			if(x1 != x2 && y1 != y2) mPixbufSetPixel(p, x2, y2, pix);

			e2 = 2 * e;
			if(e2 <= dx) { x1++; x2--; e += dx += b; }
			if(e2 >= dy || 2 * e > dx) { y1++; y2--; e += dy += b1; }
		} while(y1 <= y2);
	}
}

/**@ 全体を埋める
 *
 * @d:※クリッピング関係なく全体を埋めるため、ウィジェット描画時には使わないこと。 */

void mPixbufFill(mPixbuf *p,mPixCol pix)
{
	uint8_t *pd;
	int i,pitch;
	
	//Y1行分を埋める
	
	mPixbufBufLineH(p, p->buf, p->width, pix);
	
	//残りは上1行を順にコピー
	
	pd = p->buf;
	pitch = p->line_bytes;
	
	for(i = p->height - 1; i > 0; i--, pd += pitch)
		memcpy(pd + pitch, pd, pitch);
}

/**@ 四角形塗りつぶし */

void mPixbufFillBox(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	mBox box;
	uint8_t *pd;
	int i,pitch;
	
	if(!mPixbufClip_getBox_d(p, &box, x, y, w, h)) return;

	//Y1行分を塗りつぶし
	
	pd = mPixbufGetBufPtFast(p, box.x, box.y);

	mPixbufBufLineH(p, pd, box.w, pix);

	//以降は前の行をコピー

	w = box.w * p->pixel_bytes;
	pitch = p->line_bytes;

	for(i = box.h - 1; i > 0; i--, pd += pitch)
		memcpy(pd + pitch, pd, w);
}

/**@ 2x2 のパターンで四角形塗りつぶし
 *
 * @d:[* -]\
 * [- *] のパターンで塗りつぶす。\
 * 原点は常に mPixbuf の (0,0)。 */

void mPixbufFillBox_pat2x2(mPixbuf *p,int x,int y,int w,int h,mPixCol pix)
{
	mRect rc;
	uint8_t *pd;
	mFuncPixbufSetBuf func;
	int ix,iy,pitch,bytes,fy;
	
	if(!mPixbufClip_getRect_d(p, &rc, x, y, w, h)) return;
	
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);
	bytes = p->pixel_bytes;
	pitch = p->line_bytes - (rc.x2 - rc.x1 + 1) * bytes;

	func = MPIXBUFBASE(p)->pv.setbuf;

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		fy = iy & 1;
		
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += bytes)
		{
			if(!((ix & 1) ^ fy))
				(func)(pd, pix);
		}

		pd += pitch;
	}
}


//============================
// ビットイメージ
//============================


/**@ 1bit イメージから描画
 * 
 * @p:pat ビットは下位から順。Y1行ごとにバイト境界。
 * @p:patw,path パターンのイメージ幅と高さ
 * @p:coloff bit OFF の場合の色。(mPixCol)-1 で透過。
 * @p:colon  bit ON の場合の色。(mPixCol)-1 で透過。 */

void mPixbufDraw1bitPattern(mPixbuf *p,int x,int y,
	const uint8_t *pat,int patw,int path,mPixCol coloff,mPixCol colon)
{
	mPixbufClipBlt info;
	uint8_t *pd,f,fleft;
	const uint8_t *ps,*ps_y;
	mFuncPixbufSetBuf setbuf;
	int ix,iy,pitch_src;
	mPixCol col;
	
	pd = mPixbufClip_getBltInfo(p, &info, x, y, patw, path);
	if(!pd) return;

	setbuf = MPIXBUFBASE(p)->pv.setbuf;

	pitch_src = (patw + 7) >> 3;

	ps_y = pat + info.sy * pitch_src + (info.sx >> 3);
	fleft = 1 << (info.sx & 7);
	
	for(iy = info.h; iy; iy--)
	{
		ps = ps_y;
		f  = fleft;
	
		for(ix = info.w; ix; ix--, pd += info.bytes)
		{
			col = (*ps & f)? colon: coloff;

			if(col != (mPixCol)-1)
				(setbuf)(pd, col);
			
			f <<= 1;
			if(!f) f = 1, ps++;
		}

		pd += info.pitch_dst;
		ps_y += pitch_src;
	}
}

/**@ 1bit イメージから描画 (2色。マスクあり)
 *
 * @p:bufcol  色のデータ
 * @p:bufmask マスクデータ。bit OFF で透過、bit ON で描画。 */

void mPixbufDraw1bitPattern_2col_mask(mPixbuf *p,int x,int y,
	const uint8_t *bufcol,const uint8_t *bufmask,int patw,int path,
	mPixCol coloff,mPixCol colon)
{
	mPixbufClipBlt info;
	mFuncPixbufSetBuf setbuf;
	uint8_t *pd,f,fleft;
	const uint8_t *pscol,*psmask;
	int ix,iy,pitch_src,bufpos;
	
	pd = mPixbufClip_getBltInfo(p, &info, x, y, patw, path);
	if(!pd) return;

	setbuf = MPIXBUFBASE(p)->pv.setbuf;

	pitch_src = (patw + 7) >> 3;
	bufpos = info.sy * pitch_src + (info.sx >> 3);
	fleft = 1 << (info.sx & 7);
	
	for(iy = info.h; iy; iy--)
	{
		pscol = bufcol + bufpos;
		psmask = bufmask + bufpos;
		f = fleft;
	
		for(ix = info.w; ix; ix--, pd += info.bytes)
		{
			if(*psmask & f)
				(setbuf)(pd, (*pscol & f)? colon: coloff);
			
			f <<= 1;
			if(!f) f = 1, pscol++, psmask++;
		}

		pd += info.pitch_dst;
		bufpos += pitch_src;
	}
}

/**@ 横に連続した 1bit イメージから、指定位置のパターンを複数描画
 *
 * @p:img 1bit イメージのバッファ
 * @p:imgw,imgh イメージ全体の幅と高さ
 * @p:eachw 一つのパターンの幅
 * @p:index 描画するパターンのインデックス位置 (複数)。255 で終端。 */

void mPixbufDraw1bitPattern_list(mPixbuf *p,int x,int y,
	const uint8_t *img,int imgw,int imgh,int eachw,
	const uint8_t *index,mPixCol col)
{
	const uint8_t *ps,*psY;
	int ix,iy,pitchs,sx;
	uint8_t f,fleft;

	pitchs = (imgw + 7) >> 3;

	for(; *index != 255; x += eachw)
	{
		sx = *(index++) * eachw;
		if(sx + eachw > imgw) break;
		
		psY = img + (sx >> 3);
		fleft = 1 << (sx & 7);

		for(iy = 0; iy < imgh; iy++, psY += pitchs)
		{
			for(ix = 0, ps = psY, f = fleft; ix < eachw; ix++)
			{
				if(*ps & f)
					mPixbufSetPixel(p, x + ix, y + iy, col);

				f <<= 1;
				if(!f) f = 1, ps++;
			}
		}
	}
}

/**@ 2bit のパターンから描画 (4色。透過可能)
 *
 * @p:pat ビットは下位から並ぶ。
 * @p:pcol 4色分の色。値が (mPixCol)-1 で描画しない。 */

void mPixbufDraw2bitPattern(mPixbuf *p,int x,int y,const uint8_t *pat,int patw,int path,mPixCol *pcol)
{
	mPixbufClipBlt info;
	const uint8_t *ps,*ps_y;
	uint8_t *pd,shift,shift_left;
	mFuncPixbufSetBuf setbuf;
	int ix,iy,pitch_src;
	mPixCol col;
	
	pd = mPixbufClip_getBltInfo(p, &info, x, y, patw, path);
	if(!pd) return;
		
	setbuf = MPIXBUFBASE(p)->pv.setbuf;

	pitch_src = (patw + 3) >> 2;

	ps_y = pat + info.sy * pitch_src + (info.sx >> 2);
	shift_left = (info.sx & 3) << 1;
	
	for(iy = info.h; iy; iy--)
	{
		ps = ps_y;
		shift = shift_left;
	
		for(ix = info.w; ix; ix--, pd += info.bytes)
		{
			col = pcol[(*ps >> shift) & 3];

			if(col != (mPixCol)-1)
				(setbuf)(pd, col);
			
			shift += 2;
			if(shift == 8) { shift = 0; ps++; }
		}

		pd += info.pitch_dst;
		ps_y += pitch_src;
	}
}

/**@ 横に連続した 2bit イメージから、指定位置のパターンを複数描画
 *
 * @p:index 255 で終了
 * @p:pcol  4色分の色。全ビットONで透過。 */

void mPixbufDraw2bitPattern_list(mPixbuf *p,int x,int y,
	const uint8_t *img,int imgw,int imgh,int eachw,const uint8_t *index,mPixCol *pcol)
{
	const uint8_t *ps,*psY;
	int ix,iy,pitchs,sx;
	uint8_t bit,bitleft;
	mPixCol col;

	pitchs = (imgw + 3) >> 2;

	for(; *index != 255; x += eachw)
	{
		sx = *(index++) * eachw;
		if(sx + eachw > imgw) break;

		//
		
		psY = img + (sx >> 2);
		bitleft = (sx & 3) << 1;

		for(iy = 0; iy < imgh; iy++, psY += pitchs)
		{
			ps = psY;
			bit = bitleft;
			
			for(ix = 0; ix < eachw; ix++)
			{
				col = pcol[(*ps >> bit) & 3];

				if(col != (mPixCol)-1)
					mPixbufSetPixel(p, x + ix, y + iy, col);

				bit += 2;
				if(bit == 8) { bit = 0; ps++; }
			}
		}
	}
}

/**@ 数字とドットをパターンから描画
 *
 * @d:イメージパターンは定義済みのものが使われる。\
 *  数字は 5x7。ドットは 3x7。
 *
 * @p:text ASCII 文字列で、数字とドットを指定。 */

void mPixbufDrawNumberPattern_5x7(mPixbuf *p,int x,int y,const char *text,mPixCol col)
{
	const uint8_t *ps,*psY;
	int w,sx,ix,iy;
	uint8_t f,fleft;

	for(; *text; text++)
	{
		if(*text == '.')
			sx = 4 * 10, w = 2;
		else if(*text >= '0' && *text <= '9')
			sx = (*text - '0') * 4, w = 4;
		else
			continue;

		//

		psY = g_pat_number_5x7 + (sx >> 3);
		fleft = 1 << (sx & 7);

		for(iy = 0; iy < 7; iy++)
		{
			ps = psY;
			f = fleft;
			
			for(ix = 0; ix < w; ix++)
			{
				if(*ps & f)
					mPixbufSetPixel(p, x + ix, y + iy, col);

				f <<= 1;
				if(!f) f = 1, ps++;
			}

			psY += 6;
		}

		x += w + 1;
	}
}


//======================
// 転送
//======================


/**@ mPixbuf から mPixbuf へ転送
 *
 * @g:転送
 *
 * @d:ソースの位置とサイズは範囲内であること。
 *
 * @p:w,h 負の値で src の幅、高さ */

void mPixbufBlt(mPixbuf *dst,int dx,int dy,mPixbuf *src,int sx,int sy,int w,int h)
{
	mPixbufClipBlt info;
	uint8_t *ps,*pd;
	int iy,size,pitchd,pitchs;

	if(w < 0) w = src->width;
	if(h < 0) h = src->height;

	pd = mPixbufClip_getBltInfo_srcpos(dst, &info, dx, dy, w, h, sx, sy);
	if(!pd) return;

	ps = mPixbufGetBufPt(src, info.sx, info.sy);
	size = info.bytes * info.w;
	pitchd = dst->line_bytes;
	pitchs = src->line_bytes;

	for(iy = info.h; iy > 0; iy--)
	{
		memcpy(pd, ps, size);

		pd += pitchd;
		ps += pitchs;
	}
}

/**@ mImageBuf から転送
 *
 * @p:w,h  負の値で、mImageBuf の幅と高さ */

void mPixbufBlt_imagebuf(mPixbuf *p,int dx,int dy,mImageBuf *src,int sx,int sy,int w,int h)
{
	mPixbufClipBlt info;
	mFuncPixbufSetBuf func;
	uint8_t *ps,*pd;
	int ix,iy,pitchs,srcbytes;

	if(w < 0) w = src->width;
	if(h < 0) h = src->height;

	pd = mPixbufClip_getBltInfo_srcpos(p, &info, dx, dy, w, h, sx, sy);
	if(!pd) return;

	//

	func = MPIXBUFBASE(p)->pv.setbuf;
	ps = MLK_IMAGEBUF_GETBUFPT(src, info.sx, info.sy);
	srcbytes = src->pixel_bytes;
	pitchs = src->line_bytes - info.w * srcbytes;

	for(iy = info.h; iy > 0; iy--)
	{
		for(ix = info.w; ix > 0; ix--)
		{
			(func)(pd, mRGBtoPix_sep(ps[0], ps[1], ps[2]));
		
			ps += srcbytes;
			pd += info.bytes;
		}

		ps += pitchs;
		pd += info.pitch_dst;
	}
}

/**@ 1bit イメージから転送
 *
 * @p:srcw ソースイメージ全体の幅
 * @p:coloff bit=0 の時の色
 * @p:colon  bit=1 の時の色 */

void mPixbufBlt_1bit(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,
	int srcw,mPixCol coloff,mPixCol colon)
{
	mPixbufClipBlt info;
	mFuncPixbufSetBuf func;
	uint8_t *pd,fleft,f;
	const uint8_t *ps,*psY;
	int ix,iy,pitchs;
	
	pd = mPixbufClip_getBltInfo_srcpos(p, &info, dx, dy, w, h, sx, sy);
	if(!pd) return;

	func = MPIXBUFBASE(p)->pv.setbuf;
	
	pitchs = (srcw + 7) >> 3;
	psY = srcimg + info.sy * pitchs + (info.sx >> 3);
	fleft = 1 << (info.sx & 7);
	
	for(iy = info.h; iy; iy--)
	{
		ps = psY;
		f = fleft;
	
		for(ix = info.w; ix; ix--, pd += info.bytes)
		{
			(func)(pd, (*ps & f)? colon: coloff);

			f <<= 1;
			if(!f) f = 1, ps++;
		}

		pd += info.pitch_dst;
		psY += pitchs;
	}
}

/**@ 2bit イメージから転送
 *
 * @p:pcol 最大４色分の色 */

void mPixbufBlt_2bit(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw,mPixCol *pcol)
{
	mPixbufClipBlt info;
	mFuncPixbufSetBuf func;
	uint8_t *pd;
	const uint8_t *ps,*ps_y;
	int ix,iy,pitchs,bit_left,bit;
	
	pd = mPixbufClip_getBltInfo_srcpos(p, &info, dx, dy, w, h, sx, sy);
	if(!pd) return;

	func = MPIXBUFBASE(p)->pv.setbuf;

	pitchs = (srcw + 3) >> 2;
	ps_y = srcimg + info.sy * pitchs + (info.sx >> 2);
	bit_left = (info.sx & 3) << 1;
	
	for(iy = info.h; iy; iy--)
	{
		ps = ps_y;
		bit = bit_left;
	
		for(ix = info.w; ix; ix--, pd += info.bytes)
		{
			(func)(pd, pcol[(*ps >> bit) & 3]);

			bit += 2;
			if(bit == 8) { bit = 0; ps++; }
		}

		pd += info.pitch_dst;
		ps_y += pitchs;
	}
}

/**@ 8bit グレイスケールのイメージデータから転送 */

void mPixbufBlt_gray8(mPixbuf *p,int dx,int dy,
	const uint8_t *srcimg,int sx,int sy,int w,int h,int srcw)
{
	mPixbufClipBlt info;
	mFuncPixbufSetBuf func;
	uint8_t *pd;
	const uint8_t *ps;
	int pitchs,ix,iy,c;

	pd = mPixbufClip_getBltInfo_srcpos(p, &info, dx, dy, w, h, sx, sy);
	if(!pd) return;

	func = MPIXBUFBASE(p)->pv.setbuf;
	ps = srcimg + info.sy * srcw + info.sx;
	pitchs = srcw - info.w;

	for(iy = info.h; iy; iy--)
	{
		for(ix = info.w; ix; ix--, pd += info.bytes)
		{
			c = *(ps++);
			
			(func)(pd, mRGBtoPix_sep(c,c,c));
		}

		pd += info.pitch_dst;
		ps += pitchs;
	}
}

/**@ mImageBuf 画像全体をタイル上に並べる
 *
 * @d:mPixbuf のオフセット位置を左上原点とする。 */

void mPixbufBltFill_imagebuf(mPixbuf *p,int x,int y,int w,int h,mImageBuf *src)
{
	mFuncPixbufSetBuf func;
	mRect rc;
	uint8_t *pd,*psY,*ps;
	int ix,iy,sw,pitchd,dstbytes,srcbytes,sx,sxleft,sy;

	if(!p || !src) return;

	if(!mPixbufClip_getRect_d(p, &rc, x, y, w, h)) return;

	//

	func = MPIXBUFBASE(p)->pv.setbuf;
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);
	dstbytes = p->pixel_bytes;
	pitchd = p->line_bytes - (rc.x2 - rc.x1 + 1) * dstbytes;

	sw = src->width;
	srcbytes = src->pixel_bytes;

	sxleft = rc.x1 % sw;
	sy = rc.y1 % src->height;
	psY = src->buf + src->line_bytes * sy;

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		sx = sxleft;
		ps = psY + sx * srcbytes;
	
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += dstbytes)
		{
			(func)(pd, mRGBtoPix_sep(ps[0], ps[1], ps[2]));

			//次のソース位置

			sx++;
			if(sx == sw)
			{
				sx = 0;
				ps = psY;
			}
			else
				ps += srcbytes;
		}

		pd += pitchd;

		//次のソースY位置

		sy++;
		if(sy == src->height)
		{
			sy = 0;
			psY = src->buf;
		}
		else
			psY += src->line_bytes;
	}
}

/**@ mImgaeBuf 画像全体を拡大縮小して転送 (オーバーサンプリング)
 *
 * @p:overnum オーバーサンプリング数 */

void mPixbufBltScale_oversamp_imagebuf(mPixbuf *p,int x,int y,int w,int h,
	mImageBuf *src,int overnum)
{
	mRect rc;
	mFuncPixbufSetBuf func;
	int ix,iy,jx,jy,fx,fy,fxleft,fincx,fincy,
		pitchs,srcbytes,sw,sh,pitchd,dstbytes,
		n,f,r,g,b,div,fincx2,fincy2;
	double ratex,ratey;
	uint8_t *pd,*psY,*ps;
	intptr_t *tablebuf;

	if(!p || !src) return;

	//拡大縮小なしの場合は通常転送

	if(w == src->width && h == src->height)
	{
		mPixbufBlt_imagebuf(p, x, y, src, 0, 0, w, h);
		return;
	}

	//

	if(!mPixbufClip_getRect_d(p, &rc, x, y, w, h)) return;

	//テーブルバッファ

	tablebuf = (intptr_t *)mMalloc(sizeof(intptr_t) * overnum * 2);
	if(!tablebuf) return;

	//

	func = MPIXBUFBASE(p)->pv.setbuf;
	pd = mPixbufGetBufPtFast(p, rc.x1, rc.y1);
	dstbytes = p->pixel_bytes;
	pitchd = p->line_bytes - (rc.x2 - rc.x1 + 1) * dstbytes;

	sw = src->width;
	sh = src->height;
	pitchs = src->line_bytes;
	srcbytes = src->pixel_bytes;

	div = overnum * overnum;

	ratex = (double)sw / w;
	ratey = (double)sh / h;

	fxleft = (int)((rc.x1 - x) * ratex * (1 << 16));
	fy = (int)((rc.y1 - y) * ratey * (1 << 16));
	fincx = (int)(ratex * (1 << 16));
	fincy = (int)(ratey * (1 << 16));
	fincx2 = (int)(ratex / overnum * (1 << 16));
	fincy2 = (int)(ratey / overnum * (1 << 16));

	//

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		fx = fxleft;

		//Y テーブル作成 (Yバッファ位置)

		for(jy = 0, f = fy; jy < overnum; jy++, f += fincy2)
		{
			n = f >> 16;
			if(n >= sh) n = sh - 1;
			
			tablebuf[jy] = (intptr_t)(src->buf + pitchs * n);
		}

		//------ X
	
		for(ix = rc.x1; ix <= rc.x2; ix++, pd += dstbytes, fx += fincx)
		{
			//X テーブル作成 (Xバイト位置)

			for(jx = 0, f = fx; jx < overnum; jx++, f += fincx2)
			{
				n = f >> 16;
				if(n >= sw) n = sw - 1;
				
				tablebuf[overnum + jx] = n * srcbytes;
			}
		
			//オーバーサンプリング

			r = g = b = 0;

			for(jy = 0; jy < overnum; jy++)
			{
				psY = (uint8_t *)tablebuf[jy];
			
				for(jx = 0; jx < overnum; jx++)
				{
					ps = psY + tablebuf[overnum + jx];

					r += ps[0];
					g += ps[1];
					b += ps[2];
				}
			}

			r /= div;
			g /= div;
			b /= div;

			//

			(func)(pd, mRGBtoPix_sep(r, g, b));
		}

		pd += pitchd;
		fy += fincy;
	}

	mFree(tablebuf);
}


//======================
// UI 描画
//======================


/**@ 指定色に影を付けた背景を描画
 *
 * @p:colno GUI 色の番号 */

void mPixbufDrawBkgndShadow(mPixbuf *p,int x,int y,int w,int h,int colno)
{
	int i,sh,yy,dr,dg,db,r,g,b,c;

	sh = h / 2;

	//上半分

	mPixbufFillBox(p, x, y, w, h - sh, mGuiCol_getPix(colno));

	//下半分

	c = mGuiCol_getRGB(colno);
	dr = MLK_RGB_R(c);
	dg = MLK_RGB_G(c);
	db = MLK_RGB_B(c);
	yy = y + h - sh;

	for(i = 0; i < sh; i++)
	{
		c = (i + 1) * 32 / sh;

		r = dr + (-dr * c >> 8);
		g = dg + (-dg * c >> 8);
		b = db + (-db * c >> 8);

		mPixbufLineH(p, x, yy + i, w, mRGBtoPix_sep(r,g,b));
	}
}

/**@ 指定色に影を付けた背景を描画 (上下逆)
 *
 * @p:colno GUI 色の番号 */

void mPixbufDrawBkgndShadowRev(mPixbuf *p,int x,int y,int w,int h,int colno)
{
	int i,sh,dr,dg,db,r,g,b,c;

	sh = h / 2;

	//下半分

	mPixbufFillBox(p, x, y + sh, w, h - sh, mGuiCol_getPix(colno));

	//上半分

	c = mGuiCol_getRGB(colno);
	dr = MLK_RGB_R(c);
	dg = MLK_RGB_G(c);
	db = MLK_RGB_B(c);

	for(i = 0; i < sh; i++)
	{
		c = (sh - i) * 32 / sh;

		r = dr + (-dr * c >> 8);
		g = dg + (-dg * c >> 8);
		b = db + (-db * c >> 8);

		mPixbufLineH(p, x, y + i, w, mRGBtoPix_sep(r,g,b));
	}
}

/**@ ボタン描画
 *
 * @g:UI 描画 */

void mPixbufDrawButton(mPixbuf *p,int x,int y,int w,int h,uint8_t flags)
{
	int no;

	//表面

	if(flags & MPIXBUF_DRAWBTT_DISABLED)
		no = MGUICOL_FACE_DISABLE;
	else if(flags & MPIXBUF_DRAWBTT_PRESSED)
		no = MGUICOL_BUTTON_FACE_PRESS;
	else if(flags & MPIXBUF_DRAWBTT_DEFAULT_BUTTON)
		no = MGUICOL_BUTTON_FACE_DEFAULT;
	else
		no = MGUICOL_BUTTON_FACE;

	if(no == MGUICOL_BUTTON_FACE_PRESS)
		mPixbufDrawBkgndShadowRev(p, x, y, w, h, no);
	else
		mPixbufDrawBkgndShadow(p, x, y, w, h, no);

	//枠

	if(flags & MPIXBUF_DRAWBTT_DISABLED)
		no = MGUICOL_FRAME_DISABLE;
	else
		no = MGUICOL_FRAME;

	mPixbufBox(p, x, y, w, h, mGuiCol_getPix(no));

	//フォーカス枠

	if((flags & MPIXBUF_DRAWBTT_FOCUSED)
		&& !(flags & MPIXBUF_DRAWBTT_DISABLED))
		mPixbufBox(p, x + 1, y + 1, w - 2, h - 2, MGUICOL_PIX(FRAME_FOCUS));
}

/**@ チェックボックス描画 (13x13) */

void mPixbufDrawCheckBox(mPixbuf *p,int x,int y,uint32_t flags)
{
	mPixCol colbk,colt,colf,colar[4];

	//色

	colf = MGUICOL_PIX(FRAME);

	if(flags & MPIXBUF_DRAWCKBOX_DISABLE)
	{
		colbk = MGUICOL_PIX(FACE_DISABLE);
		colt  = MGUICOL_PIX(TEXT_DISABLE);
	}
	else
	{
		colbk = MGUICOL_PIX(FACE_TEXTBOX);
		colt  = MGUICOL_PIX(TEXT);
	}

	//描画

	if(flags & MPIXBUF_DRAWCKBOX_RADIO)
	{
		//ラジオ

		colar[0] = colf;
		colar[1] = colt;
		colar[2] = (mPixCol)-1;
		colar[3] = colbk;

		mPixbufDraw2bitPattern(p, x, y,
			(flags & MPIXBUF_DRAWCKBOX_CHECKED)? g_pat_radiobox_on_2bit: g_pat_radiobox_off_2bit,
			13, 13, colar);
	}
	else
	{
		//通常
	
		mPixbufBox(p, x, y, 13, 13, colf);
		mPixbufFillBox(p, x + 1, y + 1, 11, 11, colbk);

		if(flags & MPIXBUF_DRAWCKBOX_CHECKED)
			mPixbufDrawChecked(p, x + 2, y + 2, colt);
	}
}

/**@ 3D枠描画
 *
 * @p:col_lt 左と上の色
 * @p:col_rb 右と下の色 */

void mPixbufDraw3DFrame(mPixbuf *p,int x,int y,int w,int h,mPixCol col_lt,mPixCol col_rb)
{
	mPixbufLineH(p, x, y, w, col_lt);
	mPixbufLineV(p, x, y + 1, h - 1, col_lt);

	mPixbufLineH(p, x + 1, y + h - 1, w - 1, col_rb);
	mPixbufLineV(p, x + w - 1, y + 1, h - 2, col_rb);
}

/**@ チェックボタン用のチェック図形を描画 (10x9) */

void mPixbufDrawChecked(mPixbuf *p,int x,int y,mPixCol col)
{
	mPixbufDraw1bitPattern(p, x, y, g_pat_checked, 10, 9, MPIXBUF_TPCOL, col);
}

/**@ メニュー用チェック図形を描画 (8x7) */

void mPixbufDrawMenuChecked(mPixbuf *p,int x,int y,mPixCol col)
{
	mPixbufDraw1bitPattern(p, x, y, g_pat_menu_checked, 8, 7, MPIXBUF_TPCOL, col);
}

/**@ メニュー用ラジオ描画 (5x5) */

void mPixbufDrawMenuRadio(mPixbuf *p,int x,int y,mPixCol col)
{
	mPixbufDraw1bitPattern(p, x, y, g_pat_menu_radio, 5, 5, MPIXBUF_TPCOL, col);
}

/**@ 下矢印描画
 *
 * @d:長い方のサイズは、短い長さを s とすると、s * 2 - 1 となる。
 *
 * @p:x,y 左上位置
 * @p:size 短い方の長さ (3,4,5) */

void mPixbufDrawArrowDown(mPixbuf *p,int x,int y,int size,mPixCol col)
{
	const uint8_t *pat[] = {
		g_pat_arrow_down_5x3, g_pat_arrow_down_7x4, g_pat_arrow_down_9x5
	};

	if(size >= 3 && size <= 5)
		mPixbufDraw1bitPattern(p, x, y, pat[size - 3], size * 2 - 1, size, MPIXBUF_TPCOL, col);
}

/**@ 上矢印描画 */

void mPixbufDrawArrowUp(mPixbuf *p,int x,int y,int size,mPixCol col)
{
	const uint8_t *pat[] = {
		g_pat_arrow_up_5x3, g_pat_arrow_up_7x4, g_pat_arrow_up_9x5
	};

	if(size >= 3 && size <= 5)
		mPixbufDraw1bitPattern(p, x, y, pat[size - 3], size * 2 - 1, size, MPIXBUF_TPCOL, col);
}

/**@ 左矢印描画 */

void mPixbufDrawArrowLeft(mPixbuf *p,int x,int y,int size,mPixCol col)
{
	const uint8_t *pat[] = {
		g_pat_arrow_left_3x5, g_pat_arrow_left_4x7, g_pat_arrow_left_5x9
	};

	if(size >= 3 && size <= 5)
		mPixbufDraw1bitPattern(p, x, y, pat[size - 3], size, size * 2 - 1, MPIXBUF_TPCOL, col);
}

/**@ 右矢印描画 */

void mPixbufDrawArrowRight(mPixbuf *p,int x,int y,int size,mPixCol col)
{
	const uint8_t *pat[] = {
		g_pat_arrow_right_3x5, g_pat_arrow_right_4x7, g_pat_arrow_right_5x9
	};

	if(size >= 3 && size <= 5)
		mPixbufDraw1bitPattern(p, x, y, pat[size - 3], size, size * 2 - 1, MPIXBUF_TPCOL, col);
}
