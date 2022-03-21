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

/**********************************
 * TileImage: 内部用処理
 **********************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rectbox.h"
#include "mlk_simd.h"

#include "def_tileimage.h"
#include "tileimage.h"

#include "pv_tileimage.h"


//---------------

//タイルサイズ
static const int g_tilesize_8bit[4] = {64*64*4, 64*64*2, 64*64, 64*64/8},
	g_tilesize_16bit[4] = {64*64*8, 64*64*4, 64*64*2, 64*64/8};

//---------------


//==========================
// 作成
//==========================


/** TileImage 作成 */

TileImage *__TileImage_create(int type,int tilew,int tileh)
{
	TileImage *p;

	p = (TileImage *)mMalloc0(sizeof(TileImage));
	if(!p) return NULL;

	p->type = type;
	p->tilew = tilew;
	p->tileh = tileh;

	__TileImage_setTileSize(p);

	//タイルバッファ確保

	if(!__TileImage_allocTileBuf(p))
	{
		mFree(p);
		return NULL;
	}

	return p;
}

/** タイルサイズをセット */

void __TileImage_setTileSize(TileImage *p)
{
	if(TILEIMGWORK->bits == 8)
		p->tilesize = g_tilesize_8bit[p->type];
	else
		p->tilesize = g_tilesize_16bit[p->type];
}


//==========================
// タイルバッファ
//==========================


/** タイルバッファを確保
 *
 * tilew,tileh はあらかじめセットしておく。 */

mlkbool __TileImage_allocTileBuf(TileImage *p)
{
	p->ppbuf = (uint8_t **)mMalloc0(p->tilew * p->tileh * sizeof(void *));

	return (p->ppbuf != 0);
}

/** リサイズなど用に新規タイル配列確保 */

uint8_t **__TileImage_allocTileBuf_new(int w,int h)
{
	return (uint8_t **)mMalloc0(w * h * sizeof(void *));
}

/** タイル配列リサイズ
 * 
 * 拡張処理のみ。サイズの縮小処理は行わない。
 * 左/上方向への拡張ならば、オフセット位置をずらす。
 *
 * movx,movy: 負の値の場合、タイル位置の移動数。0 以上なら右/下方向への拡張。
 * return: FALSE で失敗時は、配列はそのまま。 */

mlkbool __TileImage_resizeTileBuf(TileImage *p,int movx,int movy,int neww,int newh)
{
	uint8_t **ppnew,**ppd;
	int ix,iy,tx,ty,srcw,srch;

	if(movx > 0) movx = 0;
	if(movy > 0) movy = 0;

	srcw = p->tilew;
	srch = p->tileh;

	//変更なし

	if(neww == srcw && newh == srch)
		return TRUE;

	//新規確保

	ppnew = __TileImage_allocTileBuf_new(neww, newh);
	if(!ppnew) return FALSE;

	//元データコピー

	ppd = ppnew;

	for(iy = newh, ty = movy; iy > 0; iy--, ty++)
	{
		for(ix = neww, tx = movx; ix > 0; ix--, tx++, ppd++)
		{
			if(tx >= 0 && tx < srcw && ty >= 0 && ty < srch)
				*ppd = *(p->ppbuf + ty * srcw + tx);
		}
	}

	//入れ替え

	mFree(p->ppbuf);

	p->ppbuf = ppnew;
	p->tilew = neww;
	p->tileh = newh;
	p->offx += movx * 64;
	p->offy += movy * 64;

	return TRUE;
}

/** 指定イメージと同じ構成になるように、タイル配列リサイズ
 *
 * src の配列リサイズ後、すぐに実行すること。 */

mlkbool __TileImage_resizeTileBuf_clone(TileImage *p,TileImage *src)
{
	return __TileImage_resizeTileBuf(p,
		(src->offx - p->offx) / 64, (src->offy - p->offy) / 64,
		src->tilew, src->tileh);
}


//==============================
// 合成用
//==============================


/** タイル毎の合成時用の情報セット
 *
 * px,py: タイル左上の px 位置
 * rcclip: px クリッピング範囲 */

void __TileImage_setBlendInfo(TileImageBlendInfo *info,int px,int py,const mRect *rcclip)
{
	int dx,dy;

	dx = px, dy = py;

	info->sx = info->sy = 0;
	info->w = info->h = 64;

	//X 左端調整

	if(dx < rcclip->x1)
	{
		info->w  -= rcclip->x1 - dx;
		info->sx += rcclip->x1 - dx;
		dx = rcclip->x1;
	}

	//Y 上端調整

	if(dy < rcclip->y1)
	{
		info->h  -= rcclip->y1 - dy;
		info->sy += rcclip->y1 - dy;
		dy = rcclip->y1;
	}

	//右下調整

	if(dx + info->w > rcclip->x2) info->w = rcclip->x2 - dx;
	if(dy + info->h > rcclip->y2) info->h = rcclip->y2 - dy;

	//

	info->dx = dx, info->dy = dy;
}


//==============================
// ほか
//==============================


/** 濃度 (0〜100) の値を、色値に変換 */

int __TileImage_density_to_colval(int v,mlkbool rev)
{
	int n;
	
	if(TILEIMGWORK->bits == 8)
	{
		n = (int)(v / 100.0 * 255 + 0.5);
		if(rev) n = 255 - n;
	}
	else
	{
		n = (int)(v / 100.0 * 0x8000 + 0.5);
		if(rev) n = 0x8000 - n;
	}

	return n;
}

/** イメージ回転後の範囲を取得
 *
 * width x height の画像を、中心を原点として回転した時の、描画範囲。
 * (0,0) を原点とする。 */

void __TileImage_getRotateRect(mRect *rcdst,int width,int height,double dcos,double dsin)
{
	mPoint pt[4],pt2;
	mRect rc;
	int i,ctx,cty;

	ctx = width / 2;
	cty = height / 2;

	//回転前の位置

	pt[0].x = -ctx, pt[0].y = -cty;
	pt[1].x = -ctx, pt[1].y = cty;
	pt[2].x = ctx, pt[2].y = -cty;
	pt[3].x = ctx, pt[3].y = cty;

	//回転後の位置

	for(i = 0; i < 4; i++)
	{
		pt2.x = (int)(pt[i].x * dcos - pt[i].y * dsin);
		pt2.y = (int)(pt[i].x * dsin + pt[i].y * dcos);

		if(i == 0)
			mRectSetPoint(&rc, &pt2);
		else
			mRectIncPoint(&rc, pt2.x, pt2.y);
	}

	//少し拡張

	mRectDeflate(&rc, 2, 2);

	*rcdst = rc;
}

