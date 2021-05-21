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
 * フィルタ処理: ピクセレート
 **************************************/

#include <math.h>

#include "mlk.h"
#include "mlk_rand.h"

#include "tileimage.h"

#include "def_filterdraw.h"
#include "pv_filter_sub.h"


/** モザイク */

mlkbool FilterDraw_mozaic(FilterDrawInfo *info)
{
	int ix,iy,ixx,iyy,size,bits;
	double dweight,d[4];
	uint64_t col;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	bits = info->bits;
	size = info->val_bar[0];
	dweight = 1.0 / (size * size);

	//

	FilterSub_prog_substep_begin_onestep(info, 50, (info->box.h + size - 1) / size);

	for(iy = info->rc.y1; iy <= info->rc.y2; iy += size)
	{
		for(ix = info->rc.x1; ix <= info->rc.x2; ix += size)
		{
			//平均色

			d[0] = d[1] = d[2] = d[3] = 0;

			for(iyy = 0; iyy < size; iyy++)
			{
				for(ixx = 0; ixx < size; ixx++)
				{
					FilterSub_advcol_add_point(info->imgsrc, ix + ixx, iy + iyy,
						d, bits, info->clipping);
				}
			}

			FilterSub_advcol_getColor(d, dweight, &col, bits);

			//セット

			for(iyy = 0; iyy < size; iyy++)
			{
				if(iy + iyy > info->rc.y2) break;
			
				for(ixx = 0; ixx < size; ixx++)
				{
					if(ix + ixx > info->rc.x2) break;
				
					(setpix)(info->imgdst, ix + ixx, iy + iyy, &col);
				}
			}
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}


//=======================
// 水晶
//=======================


typedef struct
{
	int cx,cy,		//ブロックの中心位置(px)
		colnum;		//平均色使用時、ブロック内の色数
	double dcol[4];	//平均色使用時、ブロック内の色の総和
	uint64_t col;	//ブロックの色
}_crystal_dat;


/* イメージ位置からブロックのポインタ取得
 *
 * (xpos,ypos) の位置のブロックの 3x3 周囲から、(x,y) と中心の距離が一番近いものを選択 */

static _crystal_dat *_crystal_get_block(_crystal_dat *buf,
	int x,int y,int xpos,int ypos,int xnum,int ynum)
{
	_crystal_dat *p;
	int ix,iy,minx,miny,minlen,len;

	minlen = -1;

	for(iy = ypos - 1; iy <= ypos + 1; iy++)
	{
		if(iy < 0 || iy >= ynum) continue;

		for(ix = xpos - 1; ix <= xpos + 1; ix++)
		{
			if(ix < 0 || ix >= xnum) continue;

			p = buf + iy * xnum + ix;

			len = (p->cx - x) * (p->cx - x) + (p->cy - y) * (p->cy - y);

			if(minlen < 0 || len < minlen)
			{
				minlen = len;
				minx = ix;
				miny = iy;
			}
		}
	}

	return buf + miny * xnum + minx;
}

/** 水晶 */

mlkbool FilterDraw_crystal(FilterDrawInfo *info)
{
	_crystal_dat *workbuf,*p;
	mRandSFMT *rand;
	int size,xnum,ynum,ix,iy,x,y,xx,yy,bits,xpos,ypos;
	TileImageSetPixelFunc setpix;

	FilterSub_getPixelFunc(&setpix);

	size = info->val_bar[0];
	bits = info->bits;

	//ブロック数

	xnum = (info->box.w + size - 1) / size;
	ynum = (info->box.h + size - 1) / size;

	//作業用データ確保

	workbuf = (_crystal_dat *)mMalloc0(sizeof(_crystal_dat) * xnum * ynum);
	if(!workbuf) return FALSE;

	//各ブロックの中心位置をランダムで決定

	p = workbuf;
	rand = info->rand;

	for(iy = 0, yy = info->box.y; iy < ynum; iy++, yy += size)
	{
		for(ix = 0, xx = info->box.x; ix < xnum; ix++, xx += size, p++)
		{
			x = xx + mRandSFMT_getIntRange(rand, 0, size - 1);
			y = yy + mRandSFMT_getIntRange(rand, 0, size - 1);

			if(x > info->rc.x2) x = info->rc.x2;
			if(y > info->rc.y2) y = info->rc.y2;

			p->cx = x;
			p->cy = y;
		}
	}

	//平均色を使う場合、各ブロック内の色を加算

	if(info->val_ckbtt[0])
	{
		for(iy = info->rc.y1, ypos = yy = 0; iy <= info->rc.y2; iy++)
		{
			for(ix = info->rc.x1, xpos = xx = 0; ix <= info->rc.x2; ix++)
			{
				p = _crystal_get_block(workbuf, ix, iy, xx, yy, xnum, ynum);

				FilterSub_advcol_add_point(info->imgsrc, ix, iy, p->dcol, bits, info->clipping);

				p->colnum++;

				xpos++;
				if(xpos >= size) xx++, xpos = 0;
			}

			ypos++;
			if(ypos >= size) yy++, ypos = 0;
		}
	}

	//各ブロックの色を決定

	p = workbuf;

	for(iy = 0; iy < ynum; iy++)
	{
		for(ix = 0; ix < xnum; ix++, p++)
		{
			if(info->val_ckbtt[0])
				FilterSub_advcol_getColor(p->dcol, 1.0 / p->colnum, &p->col, bits);
			else
				TileImage_getPixel(info->imgsrc, p->cx, p->cy, &p->col);
		}
	}

	//色をセット

	for(iy = info->rc.y1, ypos = yy = 0; iy <= info->rc.y2; iy++)
	{
		for(ix = info->rc.x1, xpos = xx = 0; ix <= info->rc.x2; ix++)
		{
			p = _crystal_get_block(workbuf, ix, iy, xx, yy, xnum, ynum);

			(setpix)(info->imgdst, ix, iy, &p->col);

			xpos++;
			if(xpos >= size) xx++, xpos = 0;
		}

		ypos++;
		if(ypos >= size) yy++, ypos = 0;

		FilterSub_prog_substep_inc(info);
	}

	mFree(workbuf);

	return TRUE;
}


//======================


/** ハーフトーン */

mlkbool FilterDraw_halftone(FilterDrawInfo *info)
{
	int i,c,ix,iy,ixx,iyy,bits,colmax,cval[3];
	mlkbool fgray,fantialias;
	double len,len_div,len_half,rr,rrdiv,dcos[3],dsin[3],dx,dy,xx[3],yy[3],dx2,dy2,dyy;
	uint64_t col;
	TileImageSetPixelFunc setpix;
	RGBA8 *pcol8;
	RGBA16 *pcol16;

	//val_bar: [0] サイズ [1-3] 角度

	FilterSub_getPixelFunc(&setpix);

	bits = info->bits;
	len = info->val_bar[0] * 0.1;
	fantialias = info->val_ckbtt[1];
	fgray = info->val_ckbtt[2];

	if(bits == 8)
	{
		pcol8 = (RGBA8 *)&col;
		colmax = 255;
	}
	else
	{
		pcol16 = (RGBA16 *)&col;
		colmax = 0x8000;
	}

	len_div = 1.0 / len;
	len_half = len * 0.5;
	rrdiv = len * len / MLK_MATH_PI / colmax;

	//RGB 各角度の sin,cos 値

	for(i = 0; i < 3; i++)
	{
		c = info->val_bar[1 + i];

		//G,B を R と同じ角度に
		if(i && info->val_ckbtt[0])
			c = info->val_bar[1];

		rr = c / 180.0 * MLK_MATH_PI;
		dcos[i] = cos(rr);
		dsin[i] = sin(rr);
	}

	//

	for(iy = info->rc.y1; iy <= info->rc.y2; iy++)
	{
		//左端における各位置
	
		dx = info->rc.x1;
		dy = iy;

		for(i = 0; i < 3; i++)
		{
			xx[i] = dx * dcos[i] - dy * dsin[i];
			yy[i] = dx * dsin[i] + dy * dcos[i];
		}

		//
	
		for(ix = info->rc.x1; ix <= info->rc.x2; ix++)
		{
			TileImage_getPixel(info->imgsrc, ix, iy, &col);

			//色

			if(bits == 8)
			{
				c = pcol8->a;

				if(c)
				{
					if(fgray)
						cval[0] = cval[1] = cval[2] = RGB_TO_LUM(pcol8->r, pcol8->g, pcol8->b);
					else
					{
						cval[0] = pcol8->r;
						cval[1] = pcol8->g;
						cval[2] = pcol8->b;
					}
				}
			}
			else
			{
				c = pcol16->a;

				if(c)
				{
					if(fgray)
						cval[0] = cval[1] = cval[2] = RGB_TO_LUM(pcol16->r, pcol16->g, pcol16->b);
					else
					{
						cval[0] = pcol16->r;
						cval[1] = pcol16->g;
						cval[2] = pcol16->b;
					}
				}
			}

			//透明の場合はスキップ

			if(!c)
			{
				for(i = 0; i < 3; i++)
				{
					xx[i] += dcos[i];
					yy[i] += dsin[i];
				}
				continue;
			}

			//RGB

			for(i = 0; i < 3; i++)
			{
				//ボックスの中心からの距離

				dx = xx[i] - (floor(xx[i] * len_div) * len + len_half);
				dy = yy[i] - (floor(yy[i] * len_div) * len + len_half);

				//RGB 値から半径取得

				rr = sqrt(cval[i] * rrdiv);
				rr *= rr;

				//色取得

				if(cval[i] == colmax)
					c = colmax;
				else if(fantialias)
				{
					//アンチエイリアス
					// :サブピクセルごとに計算しないと、
					// :次のボックスにまたがっている部分が問題になる

					c = 0;

					for(iyy = 0, dy2 = dy; iyy < 5; iyy++, dy2 += 0.2)
					{
						if(dy2 >= len_half) dy2 -= len;
						dyy = dy2 * dy2;

						for(ixx = 0, dx2 = dx; ixx < 5; ixx++, dx2 += 0.2)
						{
							if(dx2 >= len_half) dx2 -= len;

							if(dx2 * dx2 + dyy < rr) c += colmax;
						}
					}

					c /= 25;
				}
				else
				{
					//非アンチエイリアス

					if(dx * dx + dy * dy < rr)
						c = colmax;
					else
						c = 0;
				}

				cval[i] = c;

				//次の位置

				xx[i] += dcos[i];
				yy[i] += dsin[i];
			}

			//セット

			if(bits == 8)
			{
				pcol8->r = cval[0];
				pcol8->g = cval[1];
				pcol8->b = cval[2];
			}
			else
			{
				pcol16->r = cval[0];
				pcol16->g = cval[1];
				pcol16->b = cval[2];
			}

			(setpix)(info->imgdst, ix, iy, &col);
		}

		FilterSub_prog_substep_inc(info);
	}

	return TRUE;
}

