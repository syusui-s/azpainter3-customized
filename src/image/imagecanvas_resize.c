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
 * ImageCanvas: リサイズ処理
 *****************************************/

#include <math.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"

#include "imagecanvas.h"


//----------------

typedef double (*weightfunc)(double);

enum
{
	METHOD_NEAREST,
	METHOD_MITCHELL,
	METHOD_LAGRANGE,
	METHOD_LANCZOS2,
	METHOD_LANCZOS3,
	METHOD_SPLINE16,
	METHOD_SPLINE36,
	METHOD_BLACKMAN2,
	METHOD_BLACKMAN3
};

/* リサイズ用パラメータ */

typedef struct
{
	double *pweight; //重み
	uint16_t *pindex;
	int tap;
}_param;

//----------------


//======================
// 重み関数
//======================


/** mitchell */

static double _weight_mitchell(double d)
{
	if (d < 1.0)
		return (7.0 / 6.0 * d - 2) * d * d + 8.0 / 9.0;
	else if(d < 2.0)
		return (d - 2) * (d - 2) * (7 * d - 8) / -18;
	else
		return 0;
}

/** lagrange */

static double _weight_lagrange(double d)
{
	if(d < 1.0)
		return (d - 2) * (d + 1) * (d - 1) * 0.5;
	else if(d < 2.0)
		return -(d - 3) * (d - 2) * (d - 1) / 6;
	else
		return 0;
}

/** lanczos2 */

static double _weight_lanczos2(double d)
{
	if(d < MLK_MATH_DBL_EPSILON)
		//d = ほぼ 0
		return 1.0;
	else if(d >= 2.0)
		return 0.0;
	else
	{
		d *= MLK_MATH_PI;
		return sin(d) * sin(d / 2.0) / (d * d / 2.0);
	}
}

/** lanczos3 */

static double _weight_lanczos3(double d)
{
	if(d < MLK_MATH_DBL_EPSILON)
		return 1.0;
	else if(d >= 3.0)
		return 0.0;
	else
	{
		d *= MLK_MATH_PI;
		return sin(d) * sin(d / 3.0) / (d * d / 3.0);
	}
}

/** spline16 */

static double _weight_spline16(double d)
{
	if(d < 1.0)
		return (d - 1) * (5 * d * d - 4 * d - 5) / 5;
	else if(d < 2)
		return (5 * d - 12) * (d - 1) * (d - 2) / -15;
	else
		return 0;
}

/** spline36 */

static double _weight_spline36(double d)
{
	if(d < 1.0)
		return (d - 1) * (247 * d * d - 206 * d - 209) / 209;
	else if(d < 2.0)
		return (19 * d - 45) * (d - 1) * (d - 2) * 6 / -209;
	else if(d < 3.0)
		return (19 * d - 64) * (d - 2) * (d - 3) / 209;
	else
		return 0;
}

/** blackmansinc2 */

static double _weight_blackmansinc2(double d)
{
	if(d < MLK_MATH_DBL_EPSILON)
		return 1.0;
	else if(d >= 2.0)
		return 0;
	else
	{
		d *= MLK_MATH_PI;
	
		return (0.42 + 0.5 * cos(2.0 / 3.0 * d) + 0.08 * cos(4.0 / 3.0 * d))
			* (sin(d) / d);
	}
}

/** blackmansinc3 */

static double _weight_blackmansinc3(double d)
{
	if(d < MLK_MATH_DBL_EPSILON)
		return 1.0;
	else if(d >= 3.0)
		return 0;
	else
	{
		d *= MLK_MATH_PI;
	
		return (0.42 + 0.5 * cos(d * 0.5) + 0.08 * cos(d))
			* (sin(d) / d);
	}
}


/* 重み関数 */

static const weightfunc g_weightfuncs[] = {
	_weight_mitchell, _weight_lagrange, _weight_lanczos2,
	_weight_lanczos3, _weight_spline16, _weight_spline36,
	_weight_blackmansinc2, _weight_blackmansinc3
};


//======================
// パラメータ
//======================


/* パラメータデータ解放 */

static void _param_free(_param *p)
{
	mFree(p->pweight);
	mFree(p->pindex);

	p->pweight = NULL;
	p->pindex = NULL;
}

/* パラメータバッファ確保
 *
 * return: 0 以外でエラー */

static int _param_alloc(_param *p,int dstw)
{
	p->pweight = (double *)mMalloc(sizeof(double) * dstw * p->tap);
	p->pindex = (uint16_t *)mMalloc(2 * dstw * p->tap);

	return (!p->pweight || !p->pindex);
}

/* 縮小パラメータセット
 *
 * return: 0 以外でエラー */

static int _set_param_down(_param *p,int srcw,int dstw,int range,weightfunc wfunc)
{
	int i,j,pos,tap;
	double *dwork,*pw,dsum,dscale,dscale_rev,d;
	uint16_t *pi;

	//1px あたりの処理ピクセル

	tap = p->tap = (int)((double)srcw / dstw * range * 2 + 0.5);

	//バッファ確保

	if(_param_alloc(p, dstw)) return 1;

	//作業用バッファ

	dwork = (double *)mMalloc(sizeof(double) * tap);
	if(!dwork)
	{
		_param_free(p);
		return 1;
	}

	//--------------

	pw = p->pweight;
	pi = p->pindex;

	dscale = (double)dstw / srcw;
	dscale_rev = (double)srcw / dstw;

	for(i = 0; i < dstw; i++)
	{
		pos = floor((i - range + 0.5) * dscale_rev + 0.5);
		dsum = 0;

		for(j = 0; j < tap; j++, pi++)
		{
			if(pos < 0)
				*pi = 0;
			else if(pos >= srcw)
				*pi = srcw - 1;
			else
				*pi = pos;

			d = fabs((pos + 0.5) * dscale - (i + 0.5));

			dwork[j] = (wfunc)(d);

			dsum += dwork[j];
			pos++;
		}

		for(j = 0; j < tap; j++)
			*(pw++) = dwork[j] / dsum;
	}

	mFree(dwork);

	return 0;
}

/* 拡大パラメータセット */

static int _set_param_up(_param *p,int srcw,int dstw,int range,weightfunc wfunc)
{
	int i,j,tap,npos;
	double *dwork,*pw,dsum,dscale,dpos,dmid;
	uint16_t *pi;

	tap = p->tap = range * 2;

	//バッファ確保

	if(_param_alloc(p, dstw)) return 1;

	//作業用バッファ

	dwork = (double *)mMalloc(sizeof(double) * tap);
	if(!dwork)
	{
		_param_free(p);
		return 1;
	}
	
	//--------------

	pw = p->pweight;
	pi = p->pindex;

	dscale = (double)srcw / dstw;
	dmid = tap * 0.5 + 0.5;

	for(i = 0; i < dstw; i++)
	{
		dpos = (i + 0.5) * dscale;
		npos = floor(dpos - dmid);
		dpos = npos + 0.5 - dpos;

		dsum = 0;

		for(j = 0; j < tap; j++, pi++)
		{
			if(npos < 0)
				*pi = 0;
			else if(npos >= srcw)
				*pi = srcw - 1;
			else
				*pi = npos;

			dwork[j] = (wfunc)(fabs(dpos));

			dsum += dwork[j];
			dpos += 1.0;
			npos++;
		}

		for(j = 0; j < tap; j++)
			*(pw++) = dwork[j] / dsum;
	}

	mFree(dwork);

	return 0;
}

/* パラメータセット */

static int _set_param(_param *p,int srcw,int dstw,int method)
{
	weightfunc func;
	int range;

	//半径

	if(method == METHOD_LANCZOS3
		|| method == METHOD_SPLINE36
		|| method == METHOD_BLACKMAN3)
		range = 3;
	else
		range = 2;

	//

	func = g_weightfuncs[method - 1];

	if(dstw < srcw)
		return _set_param_down(p, srcw, dstw, range, func);
	else
		return _set_param_up(p, srcw, dstw, range, func);
}


//===========================
// リサイズ
//===========================


/* (8bit) 水平リサイズ */

static void _8bit_resize_horz(_param *p,ImageCanvas *imgsrc,ImageCanvas *imgdst,mPopupProgress *prog)
{
	uint8_t **ppsrc,**ppdst,*pd,*ps,*psY;
	int ix,iy,i,n,tap,dstw;
	uint16_t *pi;
	double *pw,c[3],dw;

	ppsrc = imgsrc->ppbuf;
	ppdst = imgdst->ppbuf;
	dstw = imgdst->width;
	tap = p->tap;

	for(iy = imgdst->height; iy; iy--)
	{
		pd = *(ppdst++);
		psY = *(ppsrc++);
		pw = p->pweight;
		pi = p->pindex;

		for(ix = dstw; ix; ix--, pd += 4)
		{
			c[0] = c[1] = c[2] = 0;

			for(i = tap; i; i--, pw++, pi++)
			{
				ps = psY + (*pi << 2);
				dw = *pw;

				c[0] += ps[0] * dw;
				c[1] += ps[1] * dw;
				c[2] += ps[2] * dw;
			}

			//

			for(i = 0; i < 3; i++)
			{
				n = lround(c[i]);
				
				if(n < 0) n = 0;
				else if(n > 255) n = 255;

				pd[i] = n;
			}
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/* (8bit) 垂直リサイズ */

static void _8bit_resize_vert(_param *p,ImageCanvas *imgsrc,ImageCanvas *imgdst,mPopupProgress *prog)
{
	uint8_t **ppdst,**ppsrc,*pd,*ps;
	int ix,iy,i,n,tap,xpos,dstw;
	uint16_t *pi;
	double *pw,c[3],dw;

	ppsrc = imgsrc->ppbuf;
	ppdst = imgdst->ppbuf;
	dstw = imgdst->width;

	pw = p->pweight;
	pi = p->pindex;
	tap = p->tap;

	for(iy = imgdst->height; iy; iy--)
	{
		pd = *(ppdst++);
		xpos = 0;
	
		for(ix = dstw; ix; ix--, pd += 4, xpos += 4)
		{
			c[0] = c[1] = c[2] = 0;

			for(i = 0; i < tap; i++)
			{
				ps = ppsrc[pi[i]] + xpos;
				dw = pw[i];

				c[0] += ps[0] * dw;
				c[1] += ps[1] * dw;
				c[2] += ps[2] * dw;
			}

			//

			for(i = 0; i < 3; i++)
			{
				n = lround(c[i]);
				
				if(n < 0) n = 0;
				else if(n > 255) n = 255;

				pd[i] = n;
			}
		}

		pw += tap;
		pi += tap;

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/* (16bit) 水平リサイズ */

static void _16bit_resize_horz(_param *p,ImageCanvas *imgsrc,ImageCanvas *imgdst,mPopupProgress *prog)
{
	uint16_t **ppsrc,**ppdst,*pd,*ps,*psY;
	int ix,iy,i,tap,dstw,n;
	uint16_t *pi;
	double *pw,c[3],dw;

	ppsrc = (uint16_t **)imgsrc->ppbuf;
	ppdst = (uint16_t **)imgdst->ppbuf;
	dstw = imgdst->width;
	tap = p->tap;

	for(iy = imgdst->height; iy; iy--)
	{
		pd = *(ppdst++);
		psY = *(ppsrc++);
		pw = p->pweight;
		pi = p->pindex;

		for(ix = dstw; ix; ix--, pd += 4)
		{
			c[0] = c[1] = c[2] = 0;

			for(i = tap; i; i--, pw++, pi++)
			{
				ps = psY + (*pi << 2);
				dw = *pw;

				c[0] += ps[0] * dw;
				c[1] += ps[1] * dw;
				c[2] += ps[2] * dw;
			}

			//

			for(i = 0; i < 3; i++)
			{
				n = lround(c[i]);
				
				if(n < 0) n = 0;
				else if(n > 0x8000) n = 0x8000;

				pd[i] = n;
			}
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/* (16bit) 垂直リサイズ */

static void _16bit_resize_vert(_param *p,ImageCanvas *imgsrc,ImageCanvas *imgdst,mPopupProgress *prog)
{
	uint16_t **ppdst,**ppsrc,*pd,*ps;
	int ix,iy,i,n,tap,xpos,dstw;
	uint16_t *pi;
	double *pw,c[3],dw;

	ppsrc = (uint16_t **)imgsrc->ppbuf;
	ppdst = (uint16_t **)imgdst->ppbuf;
	dstw = imgdst->width;

	pw = p->pweight;
	pi = p->pindex;
	tap = p->tap;

	for(iy = imgdst->height; iy; iy--)
	{
		pd = *(ppdst++);
		xpos = 0;
	
		for(ix = dstw; ix; ix--, pd += 4, xpos += 4)
		{
			c[0] = c[1] = c[2] = 0;

			for(i = 0; i < tap; i++)
			{
				ps = ppsrc[pi[i]] + xpos;
				dw = pw[i];

				c[0] += ps[0] * dw;
				c[1] += ps[1] * dw;
				c[2] += ps[2] * dw;
			}

			//

			for(i = 0; i < 3; i++)
			{
				n = lround(c[i]);
				
				if(n < 0) n = 0;
				else if(n > 0x8000) n = 0x8000;

				pd[i] = n;
			}
		}

		pw += tap;
		pi += tap;

		mPopupProgressThreadSubStep_inc(prog);
	}
}


//===========================
// ニアレストネイバー
//===========================


/* (8bit) ニアレストネイバー */

static void _8bit_resize_nearest(ImageCanvas *src,ImageCanvas *dst,mPopupProgress *prog)
{
	uint8_t **ppdst,**ppsrc;
	uint32_t *pd,*psY;
	int ix,iy,neww,newh;
	double dscalex,dscaley;

	ppdst = dst->ppbuf;
	ppsrc = src->ppbuf;
	neww = dst->width;
	newh = dst->height;

	dscalex = (double)src->width / neww;
	dscaley = (double)src->height / newh;

	for(iy = 0; iy < newh; iy++)
	{
		pd = (uint32_t *)*(ppdst++);
		psY = (uint32_t *)*(ppsrc + (int)(iy * dscaley));
	
		for(ix = 0; ix < neww; ix++)
		{
			*(pd++) = *(psY + (int)(ix * dscalex));
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/* (16bit) ニアレストネイバー */

static void _16bit_resize_nearest(ImageCanvas *src,ImageCanvas *dst,mPopupProgress *prog)
{
	uint8_t **ppdst,**ppsrc;
	uint64_t *pd,*psY;
	int ix,iy,neww,newh;
	double dscalex,dscaley;

	ppdst = dst->ppbuf;
	ppsrc = src->ppbuf;
	neww = dst->width;
	newh = dst->height;

	dscalex = (double)src->width / neww;
	dscaley = (double)src->height / newh;

	for(iy = 0; iy < newh; iy++)
	{
		pd = (uint64_t *)*(ppdst++);
		psY = (uint64_t *)*(ppsrc + (int)(iy * dscaley));
	
		for(ix = 0; ix < neww; ix++)
		{
			*(pd++) = *(psY + (int)(ix * dscalex));
		}

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/* ニアレストネイバー処理 */

ImageCanvas *_proc_nearest(ImageCanvas *src,int neww,int newh,mPopupProgress *prog,int stepnum)
{
	ImageCanvas *dst;

	//作成

	dst = ImageCanvas_new(neww, newh, src->bits);
	if(!dst)
	{
		ImageCanvas_free(src);
		return NULL;
	}

	//リサイズ

	mPopupProgressThreadSubStep_begin(prog, stepnum, newh);

	if(src->bits == 8)
		_8bit_resize_nearest(src, dst, prog);
	else
		_16bit_resize_nearest(src, dst, prog);

	ImageCanvas_free(src);

	return dst;
}


//===========================
// main
//===========================


/** リサイズ
 *
 * src は常に解放される。
 *
 * return: メモリが足りない場合 NULL */

ImageCanvas *ImageCanvas_resize(ImageCanvas *src,int neww,int newh,int method,
	mPopupProgress *prog,int stepnum)
{
	ImageCanvas *dst,*tmp;
	_param param;
	int bits,err = 1;

	//ニアレストネイバー

	if(method == METHOD_NEAREST)
		return _proc_nearest(src, neww, newh, prog, stepnum);

	//----------

	dst = NULL;
	bits = src->bits;

	mMemset0(&param, sizeof(_param));

	mPopupProgressThreadSubStep_begin(prog, stepnum, src->height + newh);

	//水平リサイズ結果用イメージ

	tmp = ImageCanvas_new(neww, src->height, bits);
	if(!tmp) goto ERR;

	//水平リサイズ

	if(_set_param(&param, src->width, neww, method)) goto ERR;

	if(bits == 8)
		_8bit_resize_horz(&param, src, tmp, prog);
	else
		_16bit_resize_horz(&param, src, tmp, prog);

	_param_free(&param);

	//src を解放

	ImageCanvas_free(src);
	src = NULL;

	//結果イメージ作成

	dst = ImageCanvas_new(neww, newh, bits);
	if(!dst) goto ERR;

	//垂直リサイズ

	if(_set_param(&param, tmp->height, newh, method)) goto ERR;

	if(bits == 8)
		_8bit_resize_vert(&param, tmp, dst, prog);
	else
		_16bit_resize_vert(&param, tmp, dst, prog);

	_param_free(&param);

	//---------

	err = 0;
	
ERR:
	ImageCanvas_free(src);
	ImageCanvas_free(tmp);

	if(err)
	{
		ImageCanvas_free(dst);
		dst = NULL;
	}

	return dst;
}

