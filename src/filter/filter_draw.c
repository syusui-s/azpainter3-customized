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

/**************************************
 * フィルタ処理: 描画
 **************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rand.h"

#include "tileimage.h"

#include "def_filterdraw.h"
#include "pv_filter_sub.h"

#include "perlin_noise.h"


//===========================
// 雲模様
//===========================


/* 8bit */

static mlkbool _proc_cloud_8bit(FilterDrawInfo *info)
{
	PerlinNoise *perlin;
	int i,ix,iy,xx,yy,n;
	double resmul;
	mlkbool to_alpha;
	RGB8 col1,col2;
	RGBA8 col;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	//PerlinNoise 初期化

	perlin = PerlinNoise_new(info->val_bar[0] / 400.0, info->val_bar[1] * 0.01, info->rand);
	if(!perlin) return FALSE;

	//

	resmul = info->val_bar[2] * 5 + 38;

	//色

	to_alpha = FALSE;

	switch(info->val_combo[0])
	{
		//黒/白
		case 0:
			col1.r = col1.g = col1.b = 0;
			col2.r = col2.g = col2.b = 255;
			break;
		//描画色/背景色
		case 1:
			col1 = info->rgb_drawcol.c8;
			col2 = info->rgb_bkgnd.c8;
			break;
		//黒+アルファ値
		default:
			col.v32 = 0;
			to_alpha = TRUE;
			break;
	}

	//

	col.a = 255;

	for(iy = info->rc.y1, yy = 0; iy <= info->rc.y2; iy++, yy++)
	{
		for(ix = info->rc.x1, xx = 0; ix <= info->rc.x2; ix++, xx++)
		{
			n = (int)(PerlinNoise_getNoise(perlin, xx, yy) * resmul + 128);

			if(n < 0) n = 0;
			else if(n > 255) n = 255;

			if(to_alpha)
				col.a = n;
			else
			{
				for(i = 0; i < 3; i++)
					col.ar[i] = (col2.ar[i] - col1.ar[i]) * n / 255 + col1.ar[i];
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}
	
	PerlinNoise_free(perlin);

	return TRUE;
}

/* 16bit */

static mlkbool _proc_cloud_16bit(FilterDrawInfo *info)
{
	PerlinNoise *perlin;
	int i,ix,iy,xx,yy,n;
	double resmul;
	mlkbool to_alpha;
	RGB16 col1,col2;
	RGBA16 col;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	//PerlinNoise 初期化

	perlin = PerlinNoise_new(info->val_bar[0] / 400.0, info->val_bar[1] * 0.01, info->rand);
	if(!perlin) return FALSE;

	//

	resmul = info->val_bar[2] * 600 + 5000;

	//色

	to_alpha = FALSE;

	switch(info->val_combo[0])
	{
		//黒/白
		case 0:
			col1.r = col1.g = col1.b = 0;
			col2.r = col2.g = col2.b = COLVAL_16BIT;
			break;
		//描画色/背景色
		case 1:
			col1 = info->rgb_drawcol.c16;
			col2 = info->rgb_bkgnd.c16;
			break;
		//黒+アルファ値
		default:
			col.v64 = 0;
			to_alpha = TRUE;
			break;
	}

	//

	col.a = COLVAL_16BIT;

	for(iy = info->rc.y1, yy = 0; iy <= info->rc.y2; iy++, yy++)
	{
		for(ix = info->rc.x1, xx = 0; ix <= info->rc.x2; ix++, xx++)
		{
			n = (int)(PerlinNoise_getNoise(perlin, xx, yy) * resmul + 0x4000);

			if(n < 0) n = 0;
			else if(n > COLVAL_16BIT) n = COLVAL_16BIT;

			if(to_alpha)
				col.a = n;
			else
			{
				for(i = 0; i < 3; i++)
					col.ar[i] = ((col2.ar[i] - col1.ar[i]) * n >> 15) + col1.ar[i];
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}
	
	PerlinNoise_free(perlin);

	return TRUE;
}

/** 雲模様 */

mlkbool FilterDraw_draw_cloud(FilterDrawInfo *info)
{
	if(info->bits == 8)
		return _proc_cloud_8bit(info);
	else
		return _proc_cloud_16bit(info);
}


//===========================
//
//===========================


/** アミトーン描画 */

mlkbool FilterDraw_draw_amitone(FilterDrawInfo *info)
{
	int density,c,ix,iy,ixx,iyy,fantialias,bits,maxval;
	double rr,dcos,dsin,len,len_half,dsx,dsy,xx,yy,cx,cy,dyy;
	uint64_t col;
	TileImageSetPixelFunc setpix;

	//val_bar: [0] サイズ [1] 濃度 [2] 角度

	FilterSub_getPixelFunc(&setpix);

	FilterSub_getDrawColor_type(info, info->val_combo[0], &col);

	bits = info->bits;
	maxval = (bits == 8)? 255: COLVAL_16BIT;
	len = info->val_bar[0] * 0.1;
	density = info->val_bar[1];
	fantialias = info->val_ckbtt[0];

	rr = info->val_bar[2] / 180.0 * MLK_MATH_PI;
	dcos = cos(rr);
	dsin = sin(rr);

	c = (density > 50)? 100 - density: density;		//濃度 50% 以上は反転
	rr = sqrt(len * len * (c * 0.01) / MLK_MATH_PI);	//点の半径
	rr *= rr;

	len_half = len * 0.5;

	//--------

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		dsx = info->rc.x1;
		dsy = iy;

		xx = dsx * dcos - dsy * dsin;
		yy = dsx * dsin + dsy * dcos;

		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//ボックスの中心位置

			cx = floor(xx / len) * len + len_half;
			cy = floor(yy / len) * len + len_half;

			//

			if(fantialias)
			{
				//アンチエイリアス (5x5)

				c = 0;

				for(iyy = 0, dsy = yy - cy; iyy < 5; iyy++, dsy += 0.2)
				{
					dyy = dsy * dsy;
				
					for(ixx = 0, dsx = xx - cx; ixx < 5; ixx++, dsx += 0.2)
					{
						if(dsx * dsx + dyy < rr)
							c++;
					}
				}

				if(bits == 8)
					c = c * 255 / 25;
				else
					c = (c << 15) / 25;
			}
			else
			{
				//非アンチエイリアス

				dsx = xx - cx;
				dsy = yy - cy;

				if(dsx * dsx + dsy * dsy < rr)
					c = maxval;
				else
					c = 0;
			}

			//50% 以上は反転

			if(density > 50)
				c = maxval - c;

			//

			if(bits == 8)
				*((uint8_t *)&col + 3) = c;
			else
				*((uint16_t *)&col + 3) = c;

			(setpix)(info->imgdst, ix, iy, &col);

			//

			xx += dcos;
			yy += dsin;
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}

/** ランダムに点描画 */

mlkbool FilterDraw_draw_randpoint(FilterDrawInfo *info)
{
	FilterDrawPointInfo dat;
	FilterSubFunc_drawpoint_setpix setpix;
	mRandSFMT *rand;
	int amount,rmax,rmin,rlen,opamax,opamin,opalen,ix,iy,n;

	//FilterDrawPointInfo

	dat.info = info;
	dat.imgdst = info->imgdst;
	dat.coltype = info->val_combo[1];
	dat.masktype = 0;

	FilterSub_drawpoint_init(&dat, TILEIMAGE_PIXELCOL_NORMAL);
	FilterSub_drawpoint_getPixelFunc(info->val_combo[0], &setpix);

	//

	amount = info->val_bar[0];
	rmax = info->val_bar[1];
	rmin = 256 - (info->val_bar[3] << 8) / 100;
	rlen = 256 - rmin;
	opamax = info->val_bar[2];
	opamin = 256 - (info->val_bar[4] << 8) / 100;
	opalen = 256 - opamin;
	rand = info->rand;

	//プレビュー時は背景としてソースをコピー

	FilterSub_copySrcImage_forPreview(info);

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			//量調整
			
			if((mRandSFMT_getUint32(rand) & 8191) >= amount)
				continue;

			//半径

			n = (mRandSFMT_getIntRange(rand, 0, 256) * rlen >> 8) + rmin;
			dat.radius = rmax * n >> 8;

			//濃度 (0-100)

			n = (mRandSFMT_getIntRange(rand, 0, 256) * opalen >> 8) + opamin;
			dat.density = opamax * n >> 8;

			//セット

			if(dat.radius > 0 && dat.density)
				(setpix)(ix, iy, &dat);
		}

		FilterSub_prog_substep_inc(info);
	}
	
	return TRUE;
}

/** 縁に沿って点描画 */

mlkbool FilterDraw_draw_edgepoint(FilterDrawInfo *info)
{
	FilterDrawPointInfo dat;
	FilterSubFunc_drawpoint_setpix setpix;
	TileImage *imgsrc;
	int type,ix,iy;

	type = info->val_combo[0];

	//ソース

	imgsrc = NULL;

	if(info->val_ckbtt[0])
		imgsrc = FilterSub_getCheckedLayerImage();

	if(!imgsrc) imgsrc = info->imgsrc;

	//FilterDrawPointInfo
	// :不透明部分の外側の場合は、アルファ値比較上書きで描画

	dat.info = info;
	dat.imgsrc = imgsrc;
	dat.imgdst = info->imgdst;
	dat.radius = info->val_bar[0];
	dat.density = info->val_bar[1];
	dat.coltype = info->val_combo[2];
	dat.masktype = type + 1;

	FilterSub_drawpoint_init(&dat,
		(type == 0)? TILEIMAGE_PIXELCOL_COMPARE_A: TILEIMAGE_PIXELCOL_NORMAL);
	
	FilterSub_drawpoint_getPixelFunc(info->val_combo[1], &setpix);
	
	//プレビュー時はソースをコピー
	// :点を描画しない部分はソースのままにするため

	FilterSub_copySrcImage_forPreview(info);

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			if(type)
			{
				//不透明部分の内側

				if(TileImage_isPixel_opaque(imgsrc, ix, iy))
					continue;

				if(TileImage_isPixel_transparent(imgsrc, ix - 1, iy)
					&& TileImage_isPixel_transparent(imgsrc, ix + 1, iy)
					&& TileImage_isPixel_transparent(imgsrc, ix, iy - 1)
					&& TileImage_isPixel_transparent(imgsrc, ix, iy + 1))
					continue;
			}
			else
			{
				//不透明部分の外側

				if(TileImage_isPixel_transparent(imgsrc, ix, iy))
					continue;

				if(TileImage_isPixel_opaque(imgsrc, ix - 1, iy)
					&& TileImage_isPixel_opaque(imgsrc, ix + 1, iy)
					&& TileImage_isPixel_opaque(imgsrc, ix, iy - 1)
					&& TileImage_isPixel_opaque(imgsrc, ix, iy + 1))
					continue;
			}

			//セット

			(setpix)(ix, iy, &dat);
		}

		FilterSub_prog_substep_inc(info);
	}
	
	return TRUE;
}

/** 枠線 */

mlkbool FilterDraw_draw_frame(FilterDrawInfo *info)
{
	int i,w,h,x,y;
	uint64_t col;

	FilterSub_getDrawColor_type(info, info->val_combo[0], &col);

	FilterSub_getImageSize(&w, &h);

	x = y = 0;

	for(i = info->val_bar[0]; i && w > 0 && h > 0; i--, x++, y++)
	{
		TileImage_drawBox(info->imgdst, x, y, w, h, &col);

		w -= 2;
		h -= 2;
	}

	return TRUE;
}

/** 水平線 & 垂直線 */

mlkbool FilterDraw_draw_horzvertLine(FilterDrawInfo *info)
{
	int min_w,min_it,len_w,len_it,pos,w,ix,iy;
	mRect rc;
	uint64_t col;
	TileImageSetPixelFunc setpix;
	mRandSFMT *rand;

	FilterSub_getPixelFunc(&setpix);
	FilterSub_getDrawColor_type(info, info->val_combo[0], &col);

	rc = info->rc;
	rand = info->rand;

	min_w = info->val_bar[0];
	min_it = info->val_bar[2];
	len_w = info->val_bar[1] - min_w;
	len_it = info->val_bar[3] - min_it;

	if(len_w < 0) len_w = 0;
	if(len_it < 0) len_it = 0;

	FilterSub_prog_setMax(info, 2);

	//横線

	if(info->val_ckbtt[0])
	{
		pos = rc.y1;

		while(pos <= rc.y2)
		{
			//太さ

			w = mRandSFMT_getIntRange(rand, 0, len_w) + min_w;
			if(pos + w > rc.y2) w = rc.y2 - pos + 1;

			//描画

			for(iy = 0; iy < w; iy++)
			{
				for(ix = rc.x1; ix <= rc.x2; ix++)
					(setpix)(info->imgdst, ix, pos + iy, &col);
			}

			//次の位置

			pos += mRandSFMT_getIntRange(rand, 0, len_it) + min_it + w;
		}
	}

	FilterSub_prog_inc(info);
	
	//縦線

	if(info->val_ckbtt[1])
	{
		pos = rc.x1;

		while(pos <= rc.x2)
		{
			//太さ

			w = mRandSFMT_getIntRange(rand, 0, len_w) + min_w;
			if(pos + w > rc.x2) w = rc.x2 - pos + 1;

			//描画

			for(iy = rc.y1; iy <= rc.y2; iy++)
			{
				for(ix = 0; ix < w; ix++)
					(setpix)(info->imgdst, pos + ix, iy, &col);
			}

			//次の位置

			pos += mRandSFMT_getIntRange(rand, 0, len_it) + min_it + w;
		}
	}

	FilterSub_prog_inc(info);

	return TRUE;
}

/** チェック柄 */

mlkbool FilterDraw_draw_plaid(FilterDrawInfo *info)
{
	int colw,roww,colmod,rowmod,ix,iy,xf,yf;
	uint64_t col;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);
	FilterSub_getDrawColor_type(info, info->val_combo[0], &col);

	//

	colw = info->val_bar[0];
	roww = info->val_bar[1];

	if(info->val_ckbtt[0]) roww = colw;

	colmod = colw << 1;
	rowmod = roww << 1;

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		yf = (iy % rowmod) / roww;
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			xf = (ix % colmod) / colw;

			if((xf ^ yf) == 0)
				(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}
	
	return TRUE;
}

