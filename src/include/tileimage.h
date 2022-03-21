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

/************************************
 * TileImage
 ************************************/

#ifndef AZPT_TILEIMAGE_H
#define AZPT_TILEIMAGE_H

typedef struct _TileImage TileImage;
typedef struct _TileImageDrawGradInfo TileImageDrawGradInfo;
typedef struct _ImageCanvas ImageCanvas;
typedef struct _ImageMaterial ImageMaterial;
typedef struct _mPopupProgress mPopupProgress;
typedef struct _CanvasDrawInfo CanvasDrawInfo;
typedef struct _CanvasViewParam CanvasViewParam;
typedef struct _FillPolygon FillPolygon;
typedef union _RGB8 RGB8;
typedef struct _RGBcombo RGBcombo;
typedef struct _RGBAcombo RGBAcombo;

typedef void (*TileImageSetPixelFunc)(TileImage *p,int x,int y,void *col);
typedef void (*TileImagePixelColorFunc)(TileImage *p,void *dst,void *src,void *param);
typedef void (*TileImageDrawGradationFunc)(TileImage *p,int x1,int y1,int x2,int y2,const mRect *rc,const TileImageDrawGradInfo *info);

/* イメージ情報 */

typedef struct _TileImageInfo
{
	int tilew,tileh,
		offx,offy;
}TileImageInfo;

/* 合成用情報 */

typedef struct _TileImageBlendSrcInfo
{
	int opacity,
		tone_lines,	//0 でトーン化なし。1=0.1
		tone_angle,
		tone_density; //0 で濃度固定なし
	uint8_t blendmode,
		ftone_white;	//トーン背景を白に
	ImageMaterial *img_texture;
}TileImageBlendSrcInfo;

/* 処理範囲情報 */

typedef struct
{
	mRect rctile,	//タイル範囲
		rcclip;		//pxクリッピング範囲: x2,y2 は+1
	mPoint pxtop;	//タイル先頭の px 位置
	int tilew,tileh,//タイル範囲の幅
		pitch_tile;	//タイルのY移動幅
}TileImageTileRectInfo;

/* グラデーション描画情報*/

struct _TileImageDrawGradInfo
{
	uint8_t *buf;
	int density,	//0-256
		flags;
};

enum
{
	TILEIMAGE_DRAWGRAD_F_LOOP = 1<<0,
	TILEIMAGE_DRAWGRAD_F_SINGLE_COL = 1<<1,
	TILEIMAGE_DRAWGRAD_F_REVERSE = 1<<2
};

/* カラータイプ */

enum
{
	TILEIMAGE_COLTYPE_RGBA,
	TILEIMAGE_COLTYPE_GRAY,
	TILEIMAGE_COLTYPE_ALPHA,
	TILEIMAGE_COLTYPE_ALPHA1BIT,

	TILEIMAGE_COLTYPE_NUM
};

/* 色処理タイプ */

enum
{
	TILEIMAGE_PIXELCOL_NORMAL,		//通常アルファ合成
	TILEIMAGE_PIXELCOL_COMPARE_A,	//A比較上書き
	TILEIMAGE_PIXELCOL_OVERWRITE,	//上書き
	TILEIMAGE_PIXELCOL_ERASE,		//消しゴム
	TILEIMAGE_PIXELCOL_FINGER,		//指先
	TILEIMAGE_PIXELCOL_DODGE,		//覆い焼き
	TILEIMAGE_PIXELCOL_BURN,		//焼き込み
	TILEIMAGE_PIXELCOL_ADD,			//加算
	TILEIMAGE_PIXELCOL_INVERSE_ALPHA,	//アルファ値を反転(選択範囲用)

	TILEIMAGE_PIXELCOL_NUM
};


/*---- function ----*/

mlkbool TileImage_init(void);
void TileImage_finish(void);

void TileImage_global_setImageBits(int bits);
void TileImage_global_setImageSize(int w,int h);
void TileImage_global_setDPI(int dpi);
void TileImage_global_setToneToGray(int flag);
int TileImage_global_getTileSize(int type,int bits);
uint8_t *TileImage_global_allocTileBitMax(void);
void *TileImage_global_getRand(void);

TileImagePixelColorFunc TileImage_global_getPixelColorFunc(int type);
void TileImage_global_setPixelColorFunc(int type);

mlkbool TileImage_tile_isTransparent_forA(uint8_t *tile,int size);

/* 解放 */

void TileImage_free(TileImage *p);
void TileImage_freeTile(uint8_t **pptile);
void TileImage_freeTile_atPos(TileImage *p,int tx,int ty);
void TileImage_freeTileBuf(TileImage *p);
void TileImage_freeAllTiles(TileImage *p);
mlkbool TileImage_freeEmptyTiles(TileImage *p);
void TileImage_freeEmptyTiles_byUndo(TileImage *p,TileImage *imgundo);

/* 作成 */

TileImage *TileImage_new(int type,int w,int h);
TileImage *TileImage_newFromInfo(int type,TileImageInfo *info);
TileImage *TileImage_newFromRect(int type,const mRect *rc);
TileImage *TileImage_newFromRect_forFile(int type,const mRect *rc);
TileImage *TileImage_newClone(TileImage *src);
TileImage *TileImage_newClone_bits(TileImage *src,int srcbits,int dstbits);

/* */

TileImage *TileImage_createSame(TileImage *p,TileImage *src,int type);
void TileImage_clear(TileImage *p);

/* tilebuf */

mlkbool TileImage_resizeTileBuf_includeCanvas(TileImage *p);
mlkbool TileImage_resizeTileBuf_canvas_point(TileImage *p,int x,int y);
mlkbool TileImage_resizeTileBuf_maxsize(TileImage *p,int w,int h);
mlkbool TileImage_resizeTileBuf_combine(TileImage *p,TileImage *src);
mlkbool TileImage_resizeTileBuf_forUndo(TileImage *p,TileImageInfo *info);

/* tile */

uint8_t *TileImage_allocTile(TileImage *p);
uint8_t *TileImage_allocTile_clear(TileImage *p);
mlkbool TileImage_allocTile_atptr(TileImage *p,uint8_t **ppbuf);
mlkbool TileImage_allocTile_atptr_clear(TileImage *p,uint8_t **ppbuf);
uint8_t *TileImage_getTileAlloc_atpos(TileImage *p,int tx,int ty,mlkbool clear);

void TileImage_clearTile(TileImage *p,uint8_t *tile);
void TileImage_copyTile(TileImage *p,uint8_t *dst,const uint8_t *src);

/* set */

void TileImage_setOffset(TileImage *p,int offx,int offy);
void TileImage_moveOffset_rel(TileImage *p,int x,int y);
void TileImage_setColor(TileImage *p,uint32_t col);

/* calc */

mlkbool TileImage_pixel_to_tile(TileImage *p,int x,int y,int *tilex,int *tiley);
void TileImage_pixel_to_tile_nojudge(TileImage *p,int x,int y,int *tilex,int *tiley);
void TileImage_pixel_to_tile_rect(TileImage *p,mRect *rc);
void TileImage_tile_to_pixel(TileImage *p,int tx,int ty,int *px,int *py);
void TileImage_getAllTilesPixelRect(TileImage *p,mRect *rc);

/* get */

void TileImage_getOffset(TileImage *p,mPoint *pt);
void TileImage_getInfo(TileImage *p,TileImageInfo *info);
void TileImage_getCanDrawRect_pixel(TileImage *p,mRect *rc);
mlkbool TileImage_getTileRect_fromPixelBox(TileImage *p,mRect *rcdst,const mBox *box);
uint8_t **TileImage_getTileRectInfo(TileImage *p,TileImageTileRectInfo *info,const mBox *box);
mlkbool TileImage_getHaveImageRect_pixel(TileImage *p,mRect *rc,uint32_t *tilenum);
int TileImage_getHaveTileNum(TileImage *p);

mlkbool TileImage_clipCanDrawRect(TileImage *p,mRect *rc);

/* etc */

void TileImage_blendToCanvas(TileImage *p,ImageCanvas *dst,const mBox *boxdst,const TileImageBlendSrcInfo *sinfo);
void TileImage_blendXor_pixbuf(TileImage *p,mPixbuf *pixbuf,mBox *boxdst);
void TileImage_drawPreview(TileImage *p,mPixbuf *pixbuf,int x,int y,int boxw,int boxh,int sw,int sh);
void TileImage_drawFilterPreview(TileImage *p,mPixbuf *pixbuf,const mBox *box);
uint32_t *TileImage_getHistogram(TileImage *p);

/* set pixel */

void TileImage_setPixel_draw_direct(TileImage *p,int x,int y,void *colbuf);
void TileImage_setPixel_draw_dot_stroke(TileImage *p,int x,int y,void *colbuf);
void TileImage_setPixel_draw_brush_stroke(TileImage *p,int x,int y,void *colbuf);
void TileImage_setPixel_draw_dotpen_direct(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_draw_dotpen_stroke(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_draw_dotpen_overwrite_square(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_draw_blur(TileImage *p,int x,int y,void *colbuf);
void TileImage_setPixel_draw_finger(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_draw_addsub(TileImage *p,int x,int y,int v);

void TileImage_setPixel_new(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_new_notp(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_new_drawrect(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_new_colfunc(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_work(TileImage *p,int x,int y,void *pix);
void TileImage_setPixel_nolimit(TileImage *p,int x,int y,void *pix);

void TileImage_setFingerBuf(TileImage *p,int x,int y);

/* get pixel */

mlkbool TileImage_isPixel_opaque(TileImage *p,int x,int y);
mlkbool TileImage_isPixel_transparent(TileImage *p,int x,int y);

void TileImage_getPixel(TileImage *p,int x,int y,void *dst);
void TileImage_getPixel_alpha_buf(TileImage *p,int x,int y,void *dst);
void TileImage_getPixel_clip(TileImage *p,int x,int y,void *dst);
void TileImage_getPixel_combo(TileImage *p,int x,int y,RGBAcombo *dst);
void TileImage_getPixel_oversamp(TileImage *p,int x,int y,int *pr,int *pg,int *pb,int *pa);
void TileImage_getColor_oversamp_blendPlaid(TileImage *p,RGB8 *dst,int r,int g,int b,int a,int div,int colno);
mlkbool TileImage_getColor_bicubic(TileImage *p,double dx,double dy,int sw,int sh,uint64_t *dstcol);

/* draw */

void TileImage_drawLineB(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol,mlkbool exclude_start);
void TileImage_drawLineSlim(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol);
void TileImage_drawLine_forFilter(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol,const mRect *rcclip);

void TileImage_drawLine_clip(TileImage *p,int x1,int y1,int x2,int y2,void *col,const mRect *rcclip);
void TileImage_drawLinePoint4(TileImage *p,const mPoint *pt,const mRect *rcclip);

void TileImage_drawEllipse(TileImage *p,double cx,double cy,double xr,double yr,
    void *drawcol,mlkbool dotpen,CanvasViewParam *param,mlkbool mirror);
void TileImage_drawEllipse_dot(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol);

void TileImage_drawBox(TileImage *p,int x,int y,int w,int h,void *pix);
void TileImage_drawBezier_forXor(TileImage *p,mPoint *pt,mlkbool drawline2);
void TileImage_drawBezier(TileImage *p,mDoublePoint *pt,void *drawcol,mlkbool dotpen,uint32_t headtail);

void TileImage_drawFillBox(TileImage *p,int x,int y,int w,int h,void *col);
void TileImage_drawFillEllipse(TileImage *p,
	double cx,double cy,double xr,double yr,void *drawcol,mlkbool antialias,
	CanvasViewParam *param,mlkbool mirror);
void TileImage_drawFillEllipse_dot(TileImage *p,int x1,int y1,int x2,int y2,void *drawcol);
mlkbool TileImage_drawFillPolygon(TileImage *p,FillPolygon *fillpolygon,void *drawcol,mlkbool antialias);

void TileImage_getGradationColor(void *dstcol,double pos,const TileImageDrawGradInfo *info);

void TileImage_drawGradation_line(TileImage *p,int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info);
void TileImage_drawGradation_circle(TileImage *p,int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info);
void TileImage_drawGradation_box(TileImage *p,int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info);
void TileImage_drawGradation_radial(TileImage *p,int x1,int y1,int x2,int y2,const mRect *rcdraw,const TileImageDrawGradInfo *info);

void TileImage_drawPixels_fromA1(TileImage *dst,TileImage *src,void *drawcol);
void TileImage_combine_forA1(TileImage *dst,TileImage *src);
mlkbool TileImage_drawLineH_forA1(TileImage *p,int x1,int x2,int y);
mlkbool TileImage_drawLineV_forA1(TileImage *p,int y1,int y2,int x);

/* draw brush */

void TileImage_drawBrush_beginFree(TileImage *p,int no,double x,double y,double pressure);
void TileImage_drawBrush_beginOther(TileImage *p,double x,double y);

void TileImage_drawBrushFree(TileImage *p,int no,double x,double y,double pressure);
void TileImage_drawBrushFree_finish(TileImage *p,int no);

double TileImage_drawBrushLine(TileImage *p,
	double x1,double y1,double x2,double y2,double press_st,double press_ed,double t_start);

void TileImage_drawBrushLine_headtail(TileImage *p,double x1,double y1,double x2,double y2,uint32_t headtail);
void TileImage_drawBrushBox(TileImage *p,mDoublePoint *pt);

/* edit */

void TileImage_convertColorType(TileImage *p,int newtype,mlkbool lum_to_alpha);
void TileImage_copyTile_convert(TileImage *p,uint8_t *dst,uint8_t *src,void *tblbuf,int bits);
void TileImage_convertBits(TileImage *p,int bits,void *tblbuf,mPopupProgress *prog);

void TileImage_copyImage_rect(TileImage *dst,TileImage *src,const mRect *rc);

void TileImage_replaceColor_tp_to_col(TileImage *p,void *dstcol);
void TileImage_replaceColor_col_to_col(TileImage *p,void *srccol,void *dstcol);

void TileImage_combine(TileImage *dst,TileImage *src,mRect *rc,int opasrc,int opadst,
	int blendmode,mPopupProgress *prog);

TileImage *TileImage_createCropImage(TileImage *src,int offx,int offy,int w,int h);

void TileImage_flipHorz_full(TileImage *p);
void TileImage_flipVert_full(TileImage *p);
void TileImage_rotateLeft_full(TileImage *p);
void TileImage_rotateRight_full(TileImage *p);

void TileImage_flipHorz_box(TileImage *p,const mBox *box);
void TileImage_flipVert_box(TileImage *p,const mBox *box);
void TileImage_rotateLeft_box(TileImage *p,TileImage *src,const mBox *box);
void TileImage_rotateRight_box(TileImage *p,TileImage *src,const mBox *box);
void TileImage_scaleup_nearest(TileImage *p,TileImage *src,const mBox *box,int zoom);
void TileImage_putTiles_full(TileImage *p,TileImage *src,const mBox *box);
void TileImage_putTiles_horz(TileImage *p,TileImage *src,const mBox *box);
void TileImage_putTiles_vert(TileImage *p,TileImage *src,const mBox *box);

TileImage *TileImage_createCopyImage_forTransform(TileImage *src,TileImage *sel,const mBox *box);
void TileImage_restoreCopyImage_forTransform(TileImage *dst,TileImage *src,const mBox *box);
void TileImage_clearBox_forTransform(TileImage *p,TileImage *sel,const mBox *box);

void TileImage_transformAffine(TileImage *dst,TileImage *src,const mBox *box,
	double scalex,double scaley,double dcos,double dsin,double movx,double movy,
	mPopupProgress *prog);
void TileImage_transformAffine_preview(TileImage *p,mPixbuf *pixbuf,const mBox *boxsrc,
	double scalex,double scaley,double dcos,double dsin,mPoint *ptmov,CanvasDrawInfo *canvinfo);

void TileImage_transformHomography(TileImage *dst,TileImage *src,const mRect *rcdst,
	double *param,int sx,int sy,int sw,int sh,mPopupProgress *prog);
void TileImage_transformHomography_preview(TileImage *p,mPixbuf *pixbuf,const mBox *boxdst,
	double *param,mPoint *ptmov,int sx,int sy,int sw,int sh,CanvasDrawInfo *canvinfo);

/* select */

void TileImage_inverseSelect(TileImage *p);
void TileImage_drawPixels_fromImage_opacity(TileImage *dst,TileImage *src,const mRect *rcarea);
void TileImage_drawPixels_fromImage_color(TileImage *dst,TileImage *src,const RGBcombo *scol,const mRect *rcarea);
void TileImage_A1_getHaveRect_real(TileImage *p,mRect *dst);

TileImage *TileImage_createSelCopyImage(TileImage *src,TileImage *sel,const mRect *rcimg,mlkbool cut);
void TileImage_pasteSelImage(TileImage *dst,TileImage *src,const mRect *rcimg);

TileImage *TileImage_createBoxSelCopyImage(TileImage *src,const mBox *box,mlkbool cut);
void TileImage_pasteBoxSelImage(TileImage *dst,TileImage *src,const mBox *box);

TileImage *TileImage_createStampImage(TileImage *src,TileImage *sel,const mRect *rcimg);
void TileImage_pasteStampImage(TileImage *dst,int x,int y,int trans,TileImage *src,int srcw,int srch);

void TileImage_expandSelect(TileImage *p,int pxcnt,mPopupProgress *prog);
void TileImage_drawSelectEdge(TileImage *p,mPixbuf *pixbuf,CanvasDrawInfo *info,const mBox *boximg);

/* imagefile */

void *TileImage_createToBits_table(int bits);
uint16_t *TileImage_create8to16fix_table(void);
uint16_t *TileImage_create16to16fix_table(void);
uint8_t *TileImage_create16fixto8_table(void);
uint16_t *TileImage_create16fixto16_table(void);

void TileImage_loadimgbuf_convert(uint8_t **ppbuf,int width,int height,int srcbits,mlkbool ignore_alpha);
void TileImage_convertFromImage(TileImage *p,uint8_t **ppsrc,int srcw,int srch,mPopupProgress *prog,int prog_subnum);
void TileImage_convertFromCanvas(TileImage *p,ImageCanvas *src,mPopupProgress *prog,int prog_subnum);
mlkerr TileImage_loadFile(TileImage **ppdst,const char *filename,uint32_t format,mSize *dst_size,mPopupProgress *prog);

mlkerr TileImage_savePNG_rgba(TileImage *p,const char *filename,int dpi,mPopupProgress *prog);
mlkerr TileImage_savePNG_select_rgba(TileImage *img,TileImage *selimg,
	const char *filename,int dpi,const mRect *rc,mPopupProgress *prog);

mlkbool TileImage_saveTmpImageFile(TileImage *p,const char *filename);
TileImage *TileImage_loadTmpImageFile(const char *filename,int srcbits,int dstbits);

mlkbool TileImage_setTile_fromSave(TileImage *p,int tx,int ty,uint8_t *src);
void TileImage_converTile_APDv3(TileImage *p,uint8_t *dst,uint8_t *src);
mlkerr TileImage_saveTiles_apd4(TileImage *p,mRect *rc,uint8_t *buf,mlkerr (*func)(TileImage *p,void *param),void *param);
mlkbool TileImage_setChannelImage(TileImage *p,int chno,uint8_t **ppsrc,int srcw,int srch);

#endif
