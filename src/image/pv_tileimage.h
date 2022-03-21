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

/********************************
 * TileImage 内部用
 ********************************/

#include "blendcolor.h"

#define _TILEIMG_SIMD_ON  1

#define CONV_RGB_TO_LUM(r,g,b)  (((r) * 77 + (g) * 150 + (b) * 29) >> 8)

typedef struct _mRandSFMT mRandSFMT;
typedef struct _ImageMaterial ImageMaterial;
typedef struct _TileImageBlendInfo TileImageBlendInfo;

/* 関数 */

typedef void (*TileImageColFunc_RGBAtoTile)(uint8_t *tile,uint8_t **ppbuf,int x,int w,int h);
typedef mlkbool (*TileImageColFunc_isTransparentTile)(uint8_t *tile);
typedef mlkbool (*TileImageColFunc_isSameColor)(void *col1,void *col2);
typedef void (*TileImageColFunc_blendTile)(TileImage *p,TileImageBlendInfo *info);
typedef void (*TileImageColFunc_getPixel_atTile)(TileImage *p,uint8_t *tile,int x,int y,void *dst);
typedef uint8_t *(*TileImageColFunc_getPixelBuf_atTile)(TileImage *p,uint8_t *tile,int x,int y);
typedef void (*TileImageColFunc_setPixel)(TileImage *p,uint8_t *buf,int x,int y,void *pix);
typedef void (*TileImageColFunc_convert_forSave)(uint8_t *dst,uint8_t *src);
typedef void (*TileImageColFunc_editTile)(uint8_t *dst,uint8_t *src);
typedef void (*TileImageColFunc_getTileRGBA)(TileImage *p,uint8_t *dst,const uint8_t *src);
typedef void (*TileImageColFunc_setTileRGBA)(uint8_t *dst,const uint8_t *src,mlkbool lum_to_a);

/** 合成用情報 */

struct _TileImageBlendInfo
{
	uint8_t *tile,	//タイルバッファ
		**dstbuf;	//キャンバスバッファ (Y位置)
	ImageMaterial *imgtex;	//テクスチャイメージ
	int sx,sy,			//タイル内の開始位置 (64x64 内)
		dx,dy,			//描画先の開始位置 (px 位置)
		w,h,			//処理幅
		opacity,		//不透明度 (0-128)
		tone_repcol,	//トーンをグレイスケール表示時、置き換える色 (-1 で置き換えない)
		tone_density;	//トーン:固定濃度 (0でなし、MAX=255 or 0x8000)
	uint8_t is_tone,	//トーン化あり
		is_tone_bkgnd_tp;	//トーン:背景は透明
	int64_t tone_fx,tone_fy,	//タイル開始位置でのセル位置
		tone_fcos,tone_fsin;
	BlendColorFunc func_blend;
};

#define TILEIMG_TONE_FIX_BITS  28
#define TILEIMG_TONE_FIX_VAL   (1 << TILEIMG_TONE_FIX_BITS)

/** カラータイプ用関数データ */

typedef struct
{
	TileImageColFunc_RGBAtoTile rgba_to_tile;
	TileImageColFunc_isTransparentTile is_transparent_tile;
	TileImageColFunc_isSameColor is_same_rgba;
	TileImageColFunc_getPixel_atTile getpixel_at_tile;
	TileImageColFunc_getPixelBuf_atTile getpixelbuf_at_tile;
	TileImageColFunc_setPixel setpixel;
	TileImageColFunc_blendTile blend_tile;
	TileImageColFunc_convert_forSave convert_from_save,
		convert_to_save;
	TileImageColFunc_editTile flip_horz,
		flip_vert,
		rotate_left,
		rotate_right;
	TileImageColFunc_getTileRGBA gettile_rgba;
	TileImageColFunc_setTileRGBA settile_rgba;
}TileImageColFuncData;

/** ブラシ描画位置 */

typedef struct
{
	double x,y,pressure;
}TileImageCurvePoint;

typedef struct
{
	double x,y,pressure,t;
}TileImageBrushPoint;

/** ブラシ描画の作業用データ */

#define TILEIMAGE_BRUSH_CURVE_NUM  20	//曲線の補間数

typedef struct
{
	int angle;	//最終的な回転角度
	TileImageBrushPoint ptlast[2]; //前回の位置 (線対称の数分)

	TileImageCurvePoint curve_pt[2][4];	//曲線補間:前回の4点 (線対称の数分)
	double curve_param[TILEIMAGE_BRUSH_CURVE_NUM][4];	//曲線補間:補完する位置ごとのパラメータ

	//r,g,b = 0.0-各ビット最大値, a = 0.0-1.0
	RGBAdouble col_water,	//水彩色
		col_water_draw;		//水彩、開始時の描画色
}TileImageBrushWorkData;

/** 作業用データ */

typedef struct
{
	uint8_t *finger_buf;	//指先用、ドット形状内の色
	mRandSFMT *rand;		//ランダム用

	int bits,		//イメージビット数 (8 or 16)
		bits_val,	//現在ビット数での色値
		imgw,imgh,	//イメージのサイズ
		dpi;		//イメージのDPI

	uint8_t is_tone_to_gray;	//トーン化レイヤをグレイスケール化

	TileImageColFuncData colfunc[4];	//各カラータイプの関数 (現在のビット用)
	BlendColorFunc blendfunc[BLENDMODE_NUM];	//色合成関数 (現在のビット用)
	TileImagePixelColorFunc pixcolfunc[TILEIMAGE_PIXELCOL_NUM];	//ピクセルカラー関数 (現在のビット用)

	TileImageBrushWorkData brush; //ブラシ描画用データ

	/* ビットごとの関数 */

	void (*copy_color)(void *dst,const void *src); //色をコピー
	void (*setcol_rgba_transparent)(void *dst);	//RGBA 透明色をセット
	mlkbool (*is_transparent)(const void *buf); //色が透明か
	void (*add_weight_color)(RGBAdouble *dst,void *srcbuf,double weight); //重みを付けて色を加算
	mlkbool (*get_weight_color)(void *dst,const RGBAdouble *col); //重み加算の色から結果を取得 (FALSE=透明)
}TileImageWorkData;

extern TileImageWorkData *g_tileimg_work;
#define TILEIMGWORK  g_tileimg_work

/** 指先用ピクセルパラメータ */

typedef struct
{
	int x,y,bufpos;
}TileImageFingerPixelParam;


//-------------------

/* tileimage_pv.c */

TileImage *__TileImage_create(int type,int tilew,int tileh);
void __TileImage_setTileSize(TileImage *p);

mlkbool __TileImage_allocTileBuf(TileImage *p);
uint8_t **__TileImage_allocTileBuf_new(int w,int h);
mlkbool __TileImage_resizeTileBuf(TileImage *p,int movx,int movy,int neww,int newh);
mlkbool __TileImage_resizeTileBuf_clone(TileImage *p,TileImage *src);

void __TileImage_setBlendInfo(TileImageBlendInfo *info,int px,int py,const mRect *rcclip);

int __TileImage_density_to_colval(int v,mlkbool rev);
void __TileImage_getRotateRect(mRect *rcdst,int width,int height,double dcos,double dsin);

/* tileimage_pixel.c */

uint8_t *__TileImage_getPixelBuf_new(TileImage *p,int x,int y);
mlkbool __TileImage_getDrawSrcColor(TileImage *p,int x,int y,void *dstcol);

