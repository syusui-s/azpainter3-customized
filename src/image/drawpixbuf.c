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
 * mPixbuf の描画関数
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_pixbuf.h"
#include "mlk_guicol.h"
#include "mlk_rectbox.h"

#include "canvasinfo.h"


/** クリッピング付き直線描画 (合成)
 *
 * col: RGBA (A=0-128)
 * rc: クリッピング範囲
 *
 * クリッピング範囲によって描画線が異ならないように計算。 */

void drawpixbuf_line_clip(mPixbuf *p,
	int x1,int y1,int x2,int y2,uint32_t col,const mRect *rc)
{
	int dx,dy,tmp;
	int64_t f,inc;
	mRect rcbox,rcclip;

	rcclip = *rc;

	//rcbox = 直線を囲む矩形

	if(x1 < x2)
		rcbox.x1 = x1, rcbox.x2 = x2;
	else
		rcbox.x1 = x2, rcbox.x2 = x1;

	if(y1 < y2)
		rcbox.y1 = y1, rcbox.y2 = y2;
	else
		rcbox.y1 = y2, rcbox.y2 = y1;

	//rcbox がクリッピング範囲外

	if(rcbox.x2 < rcclip.x1 || rcbox.x1 > rcclip.x2
		|| rcbox.y2 < rcclip.y1 || rcbox.y1 > rcclip.y2)
		return;

	//---- 形に合わせて描画

	if(x1 == x2 && y1 == y2)
	{
		//1px

		mPixbufBlendPixel_a128(p, x1, y1, col);
	}
	else if(y1 == y2)
	{
		//横線

		x1 = rcbox.x1, x2 = rcbox.x2;

		if(x1 < rcclip.x1) x1 = rcclip.x1;
		if(x2 > rcclip.x2) x2 = rcclip.x2;

		mPixbufLineH_blend256(p, x1, y1, x2 - x1 + 1, col, (col >> 24) << 1); //A=0-256
	}
	else if(x1 == x2)
	{
		//縦線

		y1 = rcbox.y1, y2 = rcbox.y2;

		if(y1 < rcclip.y1) y1 = rcclip.y1;
		if(y2 > rcclip.y2) y2 = rcclip.y2;

		mPixbufLineV_blend256(p, x1, y1, y2 - y1 + 1, col, (col >> 24) << 1);
	}
	else
	{
		//直線

		dx = rcbox.x2 - rcbox.x1;
		dy = rcbox.y2 - rcbox.y1;

		if(dx > dy)
		{
			//----- 横長

			//x1 -> x2 の方向にする

			if(x1 > x2)
			{
				x1 = rcbox.x1, x2 = rcbox.x2;
				tmp = y1, y1 = y2, y2 = tmp;
			}

			inc = ((int64_t)(y2 - y1) << 16) / dx;
			f = (int64_t)y1 << 16;

			//X クリッピング

			if(x1 < rcclip.x1)
			{
				f += inc * (rcclip.x1 - x1);
				x1 = rcclip.x1;
			}

			if(x2 > rcclip.x2) x2 = rcclip.x2;

			//

			tmp = (f + (1<<15)) >> 16;

			if(y1 < y2)
			{
				//----- Y が下方向

				//開始位置が範囲外

				if(tmp > rcclip.y2) return;

				//範囲内まで進める

				while(tmp < rcclip.y1)
				{
					x1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; x1 <= x2; x1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp > rcclip.y2) break;

					mPixbufBlendPixel_a128(p, x1, tmp, col);
				}
			}
			else
			{
				//----- Y が上方向

				//開始位置が範囲外

				if(tmp < rcclip.y1) return;

				//範囲内まで進める

				while(tmp > rcclip.y2)
				{
					x1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; x1 <= x2; x1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp < rcclip.y1) break;

					mPixbufBlendPixel_a128(p, x1, tmp, col);
				}
			}
		}
		else
		{
			//------ 縦長

			//y1 -> y2 の方向にする

			if(y1 > y2)
			{
				y1 = rcbox.y1, y2 = rcbox.y2;
				tmp = x1, x1 = x2, x2 = tmp;
			}

			inc = ((int64_t)(x2 - x1) << 16) / dy;
			f = (int64_t)x1 << 16;

			//Y クリッピング

			if(y1 < rcclip.y1)
			{
				f += inc * (rcclip.y1 - y1);
				y1 = rcclip.y1;
			}

			if(y2 > rcclip.y2) y2 = rcclip.y2;

			//

			tmp = (f + (1<<15)) >> 16;

			if(x1 < x2)
			{
				//----- X が右方向

				//開始位置が範囲外

				if(tmp > rcclip.x2) return;

				//範囲内まで進める

				while(tmp < rcclip.x1)
				{
					y1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; y1 <= y2; y1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp > rcclip.x2) break;

					mPixbufBlendPixel_a128(p, tmp, y1, col);
				}
			}
			else
			{
				//----- X が左方向

				//開始位置が範囲外

				if(tmp < rcclip.x1) return;

				//範囲内まで進める

				while(tmp > rcclip.x2)
				{
					y1++;
					f += inc;

					tmp = (f + (1<<15)) >> 16;
				}

				//描画

				for(; y1 <= y2; y1++, f += inc)
				{
					tmp = (f + (1<<15)) >> 16;
					if(tmp < rcclip.x1) break;

					mPixbufBlendPixel_a128(p, tmp, y1, col);
				}
			}
		}
	}
}

/** 白黒破線の矩形枠描画 (フィルタプレビュー用) */

void drawpixbuf_dashBox_mono(mPixbuf *pixbuf,int x,int y,int w,int h)
{
	mPixCol col[2];
	int i,n,pos = 0;

	col[0] = 0;
	col[1] = MGUICOL_PIX(WHITE);

	//上

	for(i = 0; i < w; i++, pos++)
		mPixbufSetPixel(pixbuf, x + i, y, col[(pos >> 1) & 1]);

	//右

	n = x + w - 1;

	for(i = 1; i < h; i++, pos++)
		mPixbufSetPixel(pixbuf, n, y + i, col[(pos >> 1) & 1]);

	//下

	n = y + h - 1;

	for(i = w - 2; i >= 0; i--, pos++)
		mPixbufSetPixel(pixbuf, x + i, n, col[(pos >> 1) & 1]);

	//左

	for(i = h - 2; i > 0; i--, pos++)
		mPixbufSetPixel(pixbuf, x, y + i, col[(pos >> 1) & 1]);
}

/** キャンバスにグリッド線描画
 *
 * boxdst: 描画先の mPixbuf 範囲
 * boximg: boxdst の範囲に相当するイメージ座標での範囲
 * col: 上位8bitに濃度 (0-128) */

void drawpixbuf_grid(mPixbuf *pixbuf,mBox *boxdst,mBox *boximg,
	int gridw,int gridh,uint32_t col,CanvasDrawInfo *info)
{
	mRect rcclip;
	int top,bottom,i;
	double dinc[4],x1,y1,x2,y2,incx,incy;

	//pixbuf のクリッピング範囲 (rect)
	
	mRectSetBox(&rcclip, boxdst);

	//イメージ座標が +1 した時のキャンバス座標変化値

	CanvasDrawInfo_getImageIncParam(info, dinc);

	//--------- 縦線

	//イメージ X 位置の先頭と終端
	top = boximg->x / gridw * gridw;
	bottom = (boximg->x + boximg->w - 1) / gridw * gridw;;

	//イメージ X 先頭におけるキャンバス位置
	CanvasDrawInfo_image_to_canvas(info, top, 0, &x1, &y1);
	CanvasDrawInfo_image_to_canvas(info, top, info->imgh, &x2, &y2);

	incx = dinc[0] * gridw;
	incy = dinc[1] * gridw;

	for(i = top; i <= bottom; i += gridw)
	{
		//X = 0 の線は描画しない
		if(i) drawpixbuf_line_clip(pixbuf, x1, y1, x2, y2, col, &rcclip);

		x1 += incx, y1 += incy;
		x2 += incx, y2 += incy;
	}

	//--------- 横線

	top = boximg->y / gridh * gridh;
	bottom = (boximg->y + boximg->h - 1) / gridh * gridh;

	CanvasDrawInfo_image_to_canvas(info, 0, top, &x1, &y1);
	CanvasDrawInfo_image_to_canvas(info, info->imgw, top, &x2, &y2);

	incx = dinc[2] * gridh;
	incy = dinc[3] * gridh;

	for(i = top; i <= bottom; i += gridh)
	{
		if(i) drawpixbuf_line_clip(pixbuf, x1, y1, x2, y2, col, &rcclip);

		x1 += incx, y1 += incy;
		x2 += incx, y2 += incy;
	}
}

/** キャンバスに分割線描画 */

void drawpixbuf_grid_split(mPixbuf *pixbuf,mBox *boxdst,mBox *boximg,
	int splith,int splitv,uint32_t col,CanvasDrawInfo *info)
{
	mRect rcclip;
	int i;
	double x1,y1,x2,y2,pos,gridw;

	//pixbuf のクリッピング範囲 (rect)
	
	mRectSetBox(&rcclip, boxdst);

	//縦線

	pos = gridw = (double)info->imgw / splith;

	for(i = splith - 1; i > 0; i--, pos += gridw)
	{
		CanvasDrawInfo_image_to_canvas(info, pos, 0, &x1, &y1);
		CanvasDrawInfo_image_to_canvas(info, pos, info->imgh, &x2, &y2);

		drawpixbuf_line_clip(pixbuf, x1, y1, x2, y2, col, &rcclip);
	}

	//横線

	pos = gridw = (double)info->imgh / splitv;

	for(i = splitv - 1; i > 0; i--, pos += gridw)
	{
		CanvasDrawInfo_image_to_canvas(info, 0, pos, &x1, &y1);
		CanvasDrawInfo_image_to_canvas(info, info->imgw, pos, &x2, &y2);

		drawpixbuf_line_clip(pixbuf, x1, y1, x2, y2, col, &rcclip);
	}
}

/** (定規ガイド用) 中心位置の十字線描画
 *
 * x,y: 中心位置
 * len: 半径 */

void drawpixbuf_crossline_blend(mPixbuf *p,int x,int y,int len,uint32_t col)
{
	mRect rc;
	int i,add;
	uint8_t *pd;

	i = len * 2 + 1;

	if(!mPixbufClip_getRect_d(p, &rc, x - len, y - len, i, i))
		return;

	//横線

	if(rc.y1 <= y && y <= rc.y2)
	{
		pd = mPixbufGetBufPtFast(p, rc.x1, y);
		add = p->pixel_bytes;
		
		for(i = rc.x1; i <= rc.x2; i++, pd += add)
		{
			if(i != x)
				mPixbufBlendPixel_a128_buf(p, pd, col);
		}
	}

	//縦線

	if(rc.x1 <= x && x <= rc.x2)
	{
		pd = mPixbufGetBufPtFast(p, x, rc.y1);
		add = p->line_bytes;

		for(i = rc.y1; i <= rc.y2; i++, pd += add)
		{
			if(i != y)
				mPixbufBlendPixel_a128_buf(p, pd, col);
		}
	}

	//中心点

	mPixbufBlendPixel_a128(p, x, y, col);
}

/** (定規ガイド用) 楕円描画 */

void drawpixbuf_ellipse_blend(mPixbuf *p,int cx,int cy,double rd,double radio,uint32_t col)
{
	int i,x,y,bx,by;
	double xx,yy,dcos,dsin,xr,yr;
	mRect rc;

	if(!mPixbufClip_getRect_d(p, &rc, cx - 81, cy - 81, 163, 163))
		return;

	//

	dcos = cos(rd);
	dsin = sin(rd);

	if(radio < 1)
		xr = 80, yr = 80 * radio;
	else
		xr = 80 / radio, yr = 80;

	bx = (int)(xr * dcos + cx + 0.5);
	by = (int)(xr * dsin + cy + 0.5);
	rd = MLK_MATH_PI * 2 / 30;

	//

	mPixbufSetFunc_setbuf(p, mPixbufSetBufFunc_blend128);

	for(i = 0; i < 30; i++, rd += MLK_MATH_PI * 2 / 30)
	{
		xx = xr * cos(rd);
		yy = yr * sin(rd);

		x = (int)(xx * dcos - yy * dsin + cx + 0.5);
		y = (int)(xx * dsin + yy * dcos + cy + 0.5);

		mPixbufLine_excludeEnd(p, bx, by, x, y, col);
		
		bx = x, by = y;
	}

	mPixbufSetPixelModeCol(p);
}

/** 選択範囲輪郭用に点を描画 */

static const uint8_t g_pat_sel[8] = { 0xf0,0x78,0x3c,0x1e,0x0f,0x87,0xc3,0xe1 };

void drawpixbuf_setPixel_selectEdge(mPixbuf *pixbuf,int x,int y,const mRect *rcclip)
{
	if(x >= rcclip->x1 && x <= rcclip->x2 && y >= rcclip->y1 && y <= rcclip->y2)
	{
		mPixbufSetPixel(pixbuf, x, y,
			(g_pat_sel[y & 7] & (1 << (x & 7)))? 0: MGUICOL_PIX(WHITE));
	}
}

/** 選択範囲輪郭用に線を描画 */

void drawpixbuf_line_selectEdge(mPixbuf *pixbuf,int x1,int y1,int x2,int y2,const mRect *rcclip)
{
	int dx,dy,f,d,add = 1;

	if(x1 == x2 && y1 == y2)
	{
		drawpixbuf_setPixel_selectEdge(pixbuf, x1, y1, rcclip);
		return;
	}

	dx = (x1 < x2)? x2 - x1: x1 - x2;
	dy = (y1 < y2)? y2 - y1: y1 - y2;

	if(dx > dy)
	{
		f = y1 << 16;
		d = ((y2 - y1) << 16) / (x2 - x1);
		if(x1 > x2) add = -1, d = -d;

		while(x1 != x2)
		{
			drawpixbuf_setPixel_selectEdge(pixbuf, x1, (f + 0x8000) >> 16, rcclip);
			f  += d;
			x1 += add;
		}
	}
	else
	{
		f = x1 << 16;
		d = ((x2 - x1) << 16) / (y2 - y1);
		if(y1 > y2) add = -1, d = -d;

		while(y1 != y2)
		{
			drawpixbuf_setPixel_selectEdge(pixbuf, (f + 0x8000) >> 16, y1, rcclip);
			f  += d;
			y1 += add;
		}
	}
}

/** 矩形選択用の枠を描画
 *
 * paste: 貼付け中 */

void drawpixbuf_boxsel_frame(mPixbuf *pixbuf,const mBox *boximg,CanvasDrawInfo *info,int angle0,mlkbool paste)
{
	mRect rcclip;
	mPoint pt[4];
	double x1,y1,x2,y2,dtmp;
	uint32_t col,col_in;
	int i,subx;

	col_in = (paste)? 0x8088efff: 0x80ffffff;

	//描画先のクリッピング範囲 (rect)
	
	mRectSetBox(&rcclip, &info->boxdst);

	//範囲の外側 3px 分に線を引く

	x1 = boximg->x;
	y1 = boximg->y;
	x2 = boximg->x + boximg->w;
	y2 = boximg->y + boximg->h;

	if(angle0)
	{
		//------ 回転なし
		// :キャンバスの状態によっては1pxずれる。

		CanvasDrawInfo_image_to_canvas_pt(info, x1, y1, pt);
		CanvasDrawInfo_image_to_canvas_pt(info, x2, y2, pt + 1);

		subx = (info->mirror)? 1: -1;

		//黒->白->黒 の線を描画

		for(i = 0; i < 3; i++)
		{
			col = (i == 1)? col_in: 0x80000000;
		
			drawpixbuf_line_clip(pixbuf, pt[0].x, pt[0].y, pt[1].x, pt[0].y, col, &rcclip);
			drawpixbuf_line_clip(pixbuf, pt[1].x, pt[0].y, pt[1].x, pt[1].y, col, &rcclip);
			drawpixbuf_line_clip(pixbuf, pt[1].x, pt[1].y, pt[0].x, pt[1].y, col, &rcclip);
			drawpixbuf_line_clip(pixbuf, pt[0].x, pt[1].y, pt[0].x, pt[0].y, col, &rcclip);

			pt[0].x += subx, pt[0].y--;
			pt[1].x -= subx, pt[1].y++;
		}
	}
	else
	{
		//------- 回転あり

		dtmp = info->param->scalediv;

		for(i = 0; i < 3; i++)
		{
			//４隅の点をキャンバス座標に変換

			CanvasDrawInfo_image_to_canvas_pt(info, x1, y1, pt);
			CanvasDrawInfo_image_to_canvas_pt(info, x2, y1, pt + 1);
			CanvasDrawInfo_image_to_canvas_pt(info, x2, y2, pt + 2);
			CanvasDrawInfo_image_to_canvas_pt(info, x1, y2, pt + 3);

			//描画

			col = (i == 1)? col_in: 0x80000000;
		
			drawpixbuf_line_clip(pixbuf, pt[0].x, pt[0].y, pt[1].x, pt[1].y, col, &rcclip);
			drawpixbuf_line_clip(pixbuf, pt[1].x, pt[1].y, pt[2].x, pt[2].y, col, &rcclip);
			drawpixbuf_line_clip(pixbuf, pt[2].x, pt[2].y, pt[3].x, pt[3].y, col, &rcclip);
			drawpixbuf_line_clip(pixbuf, pt[3].x, pt[3].y, pt[0].x, pt[0].y, col, &rcclip);

			//1px 拡大

			x1 -= dtmp;
			y1 -= dtmp;
			x2 += dtmp;
			y2 += dtmp;
		}
	}
}

/** 1px四角形枠を描画 */

void drawpixbuf_rectframe(mPixbuf *pixbuf,const mRect *rcimg,CanvasDrawInfo *info,int angle0,uint32_t col)
{
	mRect rcclip;
	mPoint pt[4];
	double x1,y1,x2,y2;

	if(mRectIsEmpty(rcimg)) return;

	//描画先のクリッピング範囲 (rect)
	
	mRectSetBox(&rcclip, &info->boxdst);

	//

	x1 = rcimg->x1;
	y1 = rcimg->y1;
	x2 = rcimg->x2 + 1;
	y2 = rcimg->y2 + 1;

	if(angle0)
	{
		//------ 回転なし

		CanvasDrawInfo_image_to_canvas_pt(info, x1, y1, pt);
		CanvasDrawInfo_image_to_canvas_pt(info, x2, y2, pt + 1);

		drawpixbuf_line_clip(pixbuf, pt[0].x, pt[0].y, pt[1].x, pt[0].y, col, &rcclip);
		drawpixbuf_line_clip(pixbuf, pt[1].x, pt[0].y, pt[1].x, pt[1].y, col, &rcclip);
		drawpixbuf_line_clip(pixbuf, pt[1].x, pt[1].y, pt[0].x, pt[1].y, col, &rcclip);
		drawpixbuf_line_clip(pixbuf, pt[0].x, pt[1].y, pt[0].x, pt[0].y, col, &rcclip);
	}
	else
	{
		//------- 回転あり

		//４隅の点をキャンバス座標に変換

		CanvasDrawInfo_image_to_canvas_pt(info, x1, y1, pt);
		CanvasDrawInfo_image_to_canvas_pt(info, x2, y1, pt + 1);
		CanvasDrawInfo_image_to_canvas_pt(info, x2, y2, pt + 2);
		CanvasDrawInfo_image_to_canvas_pt(info, x1, y2, pt + 3);

		//描画

		drawpixbuf_line_clip(pixbuf, pt[0].x, pt[0].y, pt[1].x, pt[1].y, col, &rcclip);
		drawpixbuf_line_clip(pixbuf, pt[1].x, pt[1].y, pt[2].x, pt[2].y, col, &rcclip);
		drawpixbuf_line_clip(pixbuf, pt[2].x, pt[2].y, pt[3].x, pt[3].y, col, &rcclip);
		drawpixbuf_line_clip(pixbuf, pt[3].x, pt[3].y, pt[0].x, pt[0].y, col, &rcclip);
	}
}


