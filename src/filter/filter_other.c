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
 * フィルタ処理: ほか
 **************************************/

#include "mlk.h"

#include "def_filterdraw.h"

#include "tileimage.h"
#include "def_tileimage.h"

#include "pv_filter_sub.h"


//============================
// 線画抽出
//============================


/* 8bit */

static mlkbool _pixfunc_lumtoalpha8(FilterDrawInfo *info,int x,int y,RGBA8 *col)
{
	if(!col->a)
		return FALSE;
	else
	{
		col->a = 255 - RGB_TO_LUM(col->r, col->g, col->b);
		col->r = col->g = col->b = 0;

		return TRUE;
	}
}

/* 16bit */

static mlkbool _pixfunc_lumtoalpha16(FilterDrawInfo *info,int x,int y,RGBA16 *col)
{
	if(!col->a)
		return FALSE;
	else
	{
		col->a = COLVAL_16BIT - RGB_TO_LUM(col->r, col->g, col->b);
		col->r = col->g = col->b = 0;

		return TRUE;
	}
}

/** 線画抽出 */

mlkbool FilterDraw_lum_to_alpha(FilterDrawInfo *info)
{
	return FilterSub_proc_pixel(info, _pixfunc_lumtoalpha8, _pixfunc_lumtoalpha16);
}


//============================
// 1px ドット線の補正
//============================


/** 1px ドット線の補正 */

mlkbool FilterDraw_dot_thinning(FilterDrawInfo *info)
{
	TileImage *img = info->imgdst;
	int ix,iy,jx,jy,n,pos,flag,erasex,erasey;;
	uint64_t col,coltp;
	uint8_t c[25];
	mRect rc;

	rc = info->rc;
	coltp = 0;

	FilterSub_prog_setMax(info, 50);

	//------ phase1 (3x3 余分な点を消す)

	FilterSub_prog_substep_begin(info, 25, info->box.h);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			//透明なら処理なし

			if(TileImage_isPixel_transparent(img, ix, iy)) continue;

			//3x3 範囲: 不透明か

			for(jy = -1, pos = 0; jy <= 1; jy++)
				for(jx = -1; jx <= 1; jx++, pos++)
					c[pos] = TileImage_isPixel_opaque(img, ix + jx, iy + jy);

			//上下、左右が 0 と 1 の組み合わせでない場合

			if(!(c[3] ^ c[5]) || !(c[1] ^ c[7]))
				continue;

			//

			flag = FALSE;
			n = c[0] + c[2] + c[6] + c[8];

			if(n == 0)
				//斜めがすべて 0
				flag = TRUE;
			else if(n == 1)
			{
				//斜めのうちひとつだけ 1 で他が 0 の場合、
				//斜めの点の左右/上下どちらに点があるか

				if(c[0])
					flag = c[1] ^ c[3];
				else if(c[2])
					flag = c[1] ^ c[5];
				else if(c[6])
					flag = c[3] ^ c[7];
				else
					flag = c[5] ^ c[7];
			}

			//消す

			if(flag)
				TileImage_setPixel_draw_direct(img, ix, iy, &coltp);
		}

		FilterSub_prog_substep_inc(info);
	}

	//------- phase2 (5x5 不自然な線の補正)

	FilterSub_prog_substep_begin(info, 25, info->box.h);

	for(iy = rc.y1; iy <= rc.y2; iy++)
	{
		for(ix = rc.x1; ix <= rc.x2; ix++)
		{
			//不透明なら処理なし

			if(TileImage_isPixel_opaque(img, ix, iy)) continue;

			//5x5 範囲

			for(jy = -2, pos = 0; jy <= 2; jy++)
				for(jx = -2; jx <= 2; jx++, pos++)
					c[pos] = TileImage_isPixel_opaque(img, ix + jx, iy + jy);

			//3x3 内に点が 4 つあるか

			n = 0;

			for(jy = 0, pos = 6; jy < 3; jy++, pos += 5 - 3)
			{
				for(jx = 0; jx < 3; jx++, pos++)
					n += c[pos];
			}

			if(n != 4) continue;

			//各判定

			flag = 0;

			if(c[6] + c[7] + c[13] + c[18] == 4)
			{
				if(c[1] + c[2] + c[3] + c[9] + c[14] + c[19] == 0)
				{
					erasex = 1;
					erasey = -1;
					flag = 1;
				}
			}
			else if(c[8] + c[13] + c[16] + c[17] == 4)
			{
				if(c[9] + c[14] + c[19] + c[21] + c[22] + c[23] == 0)
				{
					erasex = 1;
					erasey = 1;
					flag = 1;
				}
			}
			else if(c[7] + c[8] + c[11] + c[16] == 4)
			{
				if(c[1] + c[2] + c[3] + c[5] + c[10] + c[15]  == 0)
				{
					erasex = -1;
					erasey = -1;
					flag = 1;
				}
			}
			else if(c[6] + c[11] + c[17] + c[18] == 4)
			{
				if(c[5] + c[10] + c[15] + c[21] + c[22] + c[23] == 0)
				{
					erasex = -1;
					erasey = 1;
					flag = 1;
				}
			}

			//セット

			if(flag)
			{
				TileImage_getPixel(img, ix + erasex, iy, &col);
				TileImage_setPixel_draw_direct(img, ix, iy, &col);

				TileImage_setPixel_draw_direct(img, ix + erasex, iy, &coltp);
				TileImage_setPixel_draw_direct(img, ix, iy + erasey, &coltp);
			}
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}


//============================
// 縁取り
//============================


/** 縁取り */

mlkbool FilterDraw_hemming(FilterDrawInfo *info)
{
	TileImage *imgsrc,*imgdst,*imgref,*imgdraw;
	int i,ix,iy,bits,a;
	mRect rc;
	uint64_t col,coldraw;
	TileImageSetPixelFunc setpix;

	imgdraw = info->imgdst;
	bits = info->bits;

	FilterSub_getDrawColor_type(info, info->val_combo[0], &coldraw);

	//imgref = 判定元イメージ

	imgref = NULL;

	if(info->val_ckbtt[0])
		imgref = FilterSub_getCheckedLayerImage();

	if(!imgref) imgref = info->imgsrc;

	//準備

	FilterSub_setPixelColFunc(TILEIMAGE_PIXELCOL_NORMAL);

	setpix = (info->in_dialog)? TileImage_setPixel_new_colfunc: TileImage_setPixel_draw_direct;

	FilterSub_copySrcImage_forPreview(info);

	//

	i = info->val_bar[0] * 10;
	if(info->val_ckbtt[1]) i += 10;

	FilterSub_prog_setMax(info, i);

	//------- 縁取り描画
	// imgsrc を参照し、imgdraw と imgdst へ描画

	rc = info->rc;

	imgsrc = imgdst = NULL;

	for(i = 0; i < info->val_bar[0]; i++)
	{
		//現在のイメージをソース用としてコピー

		TileImage_free(imgsrc);
	
		imgsrc = TileImage_newClone((i == 0)? imgref: imgdst);
		if(!imgsrc) goto ERR;

		//描画先イメージ作成

		TileImage_free(imgdst);

		imgdst = TileImage_newFromRect(imgsrc->type, &rc);
		if(!imgdst) goto ERR;

		//imgsrc を参照し、点を上下左右に合成

		FilterSub_prog_substep_begin(info, 10, info->box.h);

		for(iy = rc.y1; iy <= rc.y2; iy++)
		{
			for(ix = rc.x1; ix <= rc.x2; ix++)
			{
				TileImage_getPixel(imgsrc, ix, iy, &col);

				if(bits == 8)
				{
					a = *((uint8_t *)&col + 3);
					*((uint8_t *)&coldraw + 3) = a;
				}
				else
				{
					a = *((uint16_t *)&col + 3);
					*((uint16_t *)&coldraw + 3) = a;
				}

				if(a)
				{
					//imgdst

					TileImage_setPixel_new_colfunc(imgdst, ix - 1, iy, &coldraw);
					TileImage_setPixel_new_colfunc(imgdst, ix + 1, iy, &coldraw);
					TileImage_setPixel_new_colfunc(imgdst, ix, iy - 1, &coldraw);
					TileImage_setPixel_new_colfunc(imgdst, ix, iy + 1, &coldraw);

					//imgdraw

					(setpix)(imgdraw, ix - 1, iy, &coldraw);
					(setpix)(imgdraw, ix + 1, iy, &coldraw);
					(setpix)(imgdraw, ix, iy - 1, &coldraw);
					(setpix)(imgdraw, ix, iy + 1, &coldraw);
				}
			}

			FilterSub_prog_substep_inc(info);
		}
	}

	TileImage_free(imgsrc);
	TileImage_free(imgdst);

	//------- 元画像を切り抜く
	// info->imgsrc は info->imgdst から複製した状態なので、
	// カレントレイヤが対象でも問題ない。

	if(info->val_ckbtt[1])
	{
		FilterSub_setPixelColFunc(TILEIMAGE_PIXELCOL_ERASE);

		FilterSub_prog_substep_begin(info, 10, info->box.h);

		for(iy = rc.y1; iy <= rc.y2; iy++)
		{
			for(ix = rc.x1; ix <= rc.x2; ix++)
			{
				TileImage_getPixel(imgref, ix, iy, &col);

				if(bits == 8)
					a = *((uint8_t *)&col + 3);
				else
					a = *((uint16_t *)&col + 3);

				if(a)
					(setpix)(imgdraw, ix, iy, &col);
			}

			FilterSub_prog_substep_inc(info);
		}
	}
	
	return TRUE;

ERR:
	TileImage_free(imgsrc);
	TileImage_free(imgdst);
	return FALSE;
}


//============================
// 立体枠
//============================


/** 立体枠 */

mlkbool FilterDraw_3dframe(FilterDrawInfo *info)
{
	TileImage *img = info->imgdst;
	int i,j,w,h,cnt,fw,smooth,vs,v,xx,yy;

	FilterSub_getImageSize(&w, &h);
	
	fw = info->val_bar[0];
	smooth = info->val_ckbtt[0];

	//横線

	if(info->bits == 8)
		vs = (smooth)? 135: 83;
	else
		vs = (smooth)? 17039: 10485;
	
	if(info->val_ckbtt[1]) vs = -vs;

	v = vs;

	for(i = 0, cnt = w; i < fw; i++, cnt -= 2)
	{
		if((i << 1) >= h) break;

		if(smooth) v = vs - vs * i / fw;

		//上

		xx = i;
		yy = i;

		for(j = 0; j < cnt; j++)
			TileImage_setPixel_draw_addsub(img, xx + j, yy, v);

		//下

		xx++;
		yy = h - 1 - i;

		for(j = 0; j < cnt - 2; j++)
			TileImage_setPixel_draw_addsub(img, xx + j, yy, -v);
	}

	//縦線

	if(info->bits == 8)
		vs = (smooth)? 105: 64;
	else
		vs = (smooth)? 13435: 8192;

	if(info->val_ckbtt[1]) vs = -vs;

	v = vs;

	for(i = 0, cnt = h - 1; i < fw; i++, cnt -= 2)
	{
		if((i << 1) >= w) break;

		if(smooth) v = vs - vs * i / fw;

		//上

		xx = i;
		yy = i + 1;

		for(j = 0; j < cnt; j++)
			TileImage_setPixel_draw_addsub(img, xx, yy + j, v);

		//下

		xx = w - 1 - i;

		for(j = 0; j < cnt; j++)
			TileImage_setPixel_draw_addsub(img, xx, yy + j, -v);
	}

	return TRUE;
}


//============================
// シフト
//============================


/** シフト */

mlkbool FilterDraw_shift(FilterDrawInfo *info)
{
	int ix,iy,sx,sy,mx,my,w,h;
	uint64_t col;
	TileImageSetPixelFunc setpix;

	FilterSub_getImageSize(&w, &h);
	FilterSub_getPixelFunc(&setpix);

	mx = info->val_bar[0];
	my = info->val_bar[1];

	for(iy = 0; iy < h; iy++)
	{
		sy = iy - my;
		if(sy < 0) sy = h + sy % h;
		if(sy >= h) sy %= h;
	
		for(ix = 0; ix < w; ix++)
		{
			sx = ix - mx;
			if(sx < 0) sx = w + sx % w;
			if(sx >= w) sx %= w;

			TileImage_getPixel(info->imgsrc, sx, sy, &col);

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}

