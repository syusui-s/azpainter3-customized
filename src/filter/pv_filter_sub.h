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

/***************************************
 * フィルタサブ関数
 ***************************************/

#define COLVAL_16BIT  0x8000
#define RGB_TO_LUM(r,g,b)  (((r) * 77 + (g) * 150 + (b) * 29) >> 8)

/* 点描画の情報 */

typedef struct
{
	TileImage *imgdst,
		*imgsrc;
	int coltype,	//色のタイプ
		radius,		//半径(px)
		density,	//濃度 (0-100)
		masktype;	//縁取り時のマスク (0:なし, 1:透明部分には描画しない, 2:不透明部分には描画しない)
	double h,s,v;	//描画色のHSV
	FilterDrawInfo *info;
}FilterDrawPointInfo;

/* 3x3フィルタの情報 */

typedef struct
{
	double mul[9],divmul,add;
	mlkbool fgray;	//結果をグレイスケール化
}Filter3x3Info;

enum
{
	DRAWCOL_TYPE_DRAWCOL,
	DRAWCOL_TYPE_BKGNDCOL,
	DRAWCOL_TYPE_BLACK,
	DRAWCOL_TYPE_WHITE
};

enum
{
	LINERCOL_F_CLIPPING = 1,
	LINERCOL_F_LOOP = 2
};

//-----------------

typedef mlkbool (*FilterSubFunc_pixel8)(FilterDrawInfo *info,int x,int y,RGBA8 *col);
typedef mlkbool (*FilterSubFunc_pixel16)(FilterDrawInfo *info,int x,int y,RGBA16 *col);

typedef void (*FilterSubFunc_color8)(FilterDrawInfo *info,int x,int y,RGBA8 *col);
typedef void (*FilterSubFunc_color16)(FilterDrawInfo *info,int x,int y,RGBA16 *col);

typedef void (*FilterSubFunc_gaussblur_setpix)(int x,int y,void *col,FilterDrawInfo *info);
typedef void (*FilterSubFunc_gaussblur_setsrc)(void *buf,int pixnum,FilterDrawInfo *info);

typedef void (*FilterSubFunc_drawpoint_setpix)(int x,int y,FilterDrawPointInfo *dat);

/* sub */

void FilterSub_getPixelFunc(TileImageSetPixelFunc *func);
void FilterSub_setPixelColFunc(int coltype);
int FilterSub_getImageDPI(void);
void FilterSub_getImageSize(int *w,int *h);
TileImage *FilterSub_getCheckedLayerImage(void);
void FilterSub_getDrawColor_type(FilterDrawInfo *info,int type,void *dst);

void FilterSub_prog_setMax(FilterDrawInfo *info,int max);
void FilterSub_prog_inc(FilterDrawInfo *info);
void FilterSub_prog_substep_begin(FilterDrawInfo *info,int step,int max);
void FilterSub_prog_substep_begin_onestep(FilterDrawInfo *info,int step,int max);
void FilterSub_prog_substep_inc(FilterDrawInfo *info);

void FilterSub_copySrcImage_forPreview(FilterDrawInfo *info);
void FilterSub_getPixelSrc_clip(FilterDrawInfo *info,int x,int y,void *dst);
mlkbool FilterSub_getPixelBuf8(TileImage *img,const mRect *rc,uint8_t *buf,mlkbool clipping);
mlkbool FilterSub_getPixelBuf16(TileImage *img,const mRect *rc,uint16_t *buf,mlkbool clipping);
uint8_t *FilterSub_createShapeBuf_circle(int radius,int *pixnum);

void FilterSub_drawpoint_init(FilterDrawPointInfo *dat,int pixcoltype);
void FilterSub_drawpoint_getPixelFunc(int type,FilterSubFunc_drawpoint_setpix *dst);

void FilterSub_advcol_add8(double *d,uint8_t *buf);
void FilterSub_advcol_add16(double *d,uint16_t *buf);
void FilterSub_advcol_add_weight8(double *d,double weight,uint8_t *buf);
void FilterSub_advcol_add_weight16(double *d,double weight,uint16_t *buf);
void FilterSub_advcol_add_point(TileImage *img,int x,int y,double *d,int bits,mlkbool clipping);
void FilterSub_advcol_add_point_weight8(TileImage *img,int x,int y,double *d,double weight,mlkbool clipping);
void FilterSub_advcol_add_point_weight16(TileImage *img,int x,int y,double *d,double weight,mlkbool clipping);
void FilterSub_advcol_getColor(double *d,double weight_mul,void *buf,int bits);
void FilterSub_advcol_getColor8(double *d,double weight_mul,uint8_t *buf);
void FilterSub_advcol_getColor16(double *d,double weight_mul,uint16_t *buf);

void FilterSub_getLinerColor(TileImage *img,double x,double y,void *dst,int bits,uint8_t flags);
void FilterSub_getLinerColor_bkgnd(TileImage *img,double dx,double dy,int x,int y,void *dst,int bits,int type);

/* 共通描画処理 */

mlkbool FilterSub_proc_pixel(FilterDrawInfo *info,FilterSubFunc_pixel8 func8,FilterSubFunc_pixel16 func16);
mlkbool FilterSub_proc_3x3(FilterDrawInfo *info,Filter3x3Info *dat);

double *FilterSub_createGaussTable(int radius,int range,double *dst_weight);

mlkbool FilterSub_proc_gaussblur8(FilterDrawInfo *info,TileImage *imgsrc,int radius,
	FilterSubFunc_gaussblur_setpix setpix,FilterSubFunc_gaussblur_setsrc setsrc);
mlkbool FilterSub_proc_gaussblur16(FilterDrawInfo *info,TileImage *imgsrc,int radius,
	FilterSubFunc_gaussblur_setpix setpix,FilterSubFunc_gaussblur_setsrc setsrc);

/* カラー処理 */

uint8_t *FilterSub_createColorTable(FilterDrawInfo *info);
void FilterSub_func_color_table_8bit(FilterDrawInfo *info,int x,int y,RGBA8 *col);
void FilterSub_func_color_table_16bit(FilterDrawInfo *info,int x,int y,RGBA16 *col);

void FilterSub_RGBtoYCrCb_8bit(int *val);
void FilterSub_RGBtoYCrCb_16bit(int *val);

void FilterSub_YCrCbtoRGB_8bit(int *val);
void FilterSub_YCrCbtoRGB_16bit(int *val);

void FilterSub_RGBtoHSV_8bit(int *val);
void FilterSub_RGBtoHSV_16bit(int *val);
void FilterSub_HSVtoRGB_8bit(int *val);
void FilterSub_HSVtoRGB_16bit(int *val);

void FilterSub_RGBtoHSL_8bit(int *val);
void FilterSub_RGBtoHSL_16bit(int *val);
void FilterSub_HSLtoRGB_8bit(int *val);
void FilterSub_HSLtoRGB_16bit(int *val);

mlkbool FilterSub_proc_color(FilterDrawInfo *info,FilterSubFunc_color8 func8,FilterSubFunc_color16 func16);

