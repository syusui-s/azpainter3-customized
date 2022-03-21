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

/***************************
 * AppDraw メイン関数
 ***************************/

typedef struct _AppDraw AppDraw;
typedef struct _NewCanvasValue NewCanvasValue;
typedef struct _LoadImageOption LoadImageOption;
typedef struct _LayerItem LayerItem;
typedef struct _LayerTextItem LayerTextItem;
typedef struct _TileImage TileImage;
typedef struct _mPopupProgress mPopupProgress;
typedef struct _TileImageBlendSrcInfo TileImageBlendSrcInfo;


/* init */

int AppDraw_new(void);
void AppDraw_free(void);

void drawInit_loadConfig_before(void);
void drawInit_createWidget_before(void);
void drawInit_beforeShow(void);
void drawEnd_saveConfig(void);

/* color */

uint32_t drawColor_getDrawColor(void);
mlkbool drawColor_setDrawColor(uint32_t col);
void drawColor_setDrawColor_update(uint32_t col);
void drawColor_setBkgndColor(uint32_t col);
void drawColor_toggleDrawCol(AppDraw *p);
void drawColor_changeDrawColor8(void);

void drawColorMask_setType(AppDraw *p,int type);

/* tool */

void drawTool_setTool(AppDraw *p,int no);
void drawTool_setTool_subtype(AppDraw *p,int subno);
mlkbool drawTool_isType_notDraw(int no);
mlkbool drawTool_isType_haveDrawType(int no);

/* cursor */

void drawCursor_wait(void);
void drawCursor_restore(void);
int drawCursor_getToolCursor(int toolno);

/* etc */

mlkbool drawTexture_loadOptionTextureImage(AppDraw *p);

mlkerr drawSub_loadTileImage(TileImage **ppdst,const char *filename,mSize *psize);

void drawStamp_clearImage(AppDraw *p);
mlkerr drawStamp_loadImage(AppDraw *p,const char *filename);

/* image */

void drawImage_afterNewCanvas(AppDraw *p,mlkbool change_file);
mlkbool drawImage_changeImageSize(AppDraw *p,int w,int h);
void drawImage_changeDPI(AppDraw *p,int dpi);

mlkbool drawImage_newCanvas(AppDraw *p,NewCanvasValue *val);
mlkbool drawImage_newCanvas_openFile(AppDraw *p,int w,int h,int bits,int dpi);
mlkbool drawImage_newCanvas_openFile_bkcol(AppDraw *p,int w,int h,int bits,int dpi,uint32_t bkcol);

void drawImage_changeImageBits(AppDraw *p);
void drawImage_update_imageBits(void);
mlkerr drawImage_changeImageBits_proc(AppDraw *p,mPopupProgress *prog);

mlkbool drawImage_resizeCanvas(AppDraw *p,int w,int h,int movx,int movy,int fcrop);
mlkbool drawImage_scaleCanvas(AppDraw *p,int w,int h,int dpi,int method);

void drawImage_blendImageReal_curbits(AppDraw *p,mPopupProgress *prog,int stepnum);
mlkerr drawImage_blendImageReal_normal(AppDraw *p,int dstbits,mPopupProgress *prog,int stepnum);
mlkerr drawImage_blendImageReal_alpha(AppDraw *p,int dstbits,mPopupProgress *prog,int stepnum);

/* loadfile */

mlkerr drawImage_loadFile(AppDraw *p,const char *filename,
	uint32_t format,LoadImageOption *opt,mPopupProgress *prog,char **errmes);
mlkbool drawImage_loadError(AppDraw *p);

/* canvas */

enum
{
	DRAWCANVAS_UPDATE_ZOOM = 1<<0,
	DRAWCANVAS_UPDATE_ANGLE = 1<<1,
	DRAWCANVAS_UPDATE_RESET_SCROLL = 1<<2,		//スクロール位置をリセット (現在の状態から)
	DRAWCANVAS_UPDATE_NO_CANVAS_UPDATE = 1<<3,	//キャンバスを更新しない

	DRAWCANVAS_UPDATE_DEFAULT = 0	//値の変更なし & キャンバス更新
};

void drawCanvas_lowQuality(void);
void drawCanvas_normalQuality(void);

void drawCanvas_scroll_reset(AppDraw *p,const mDoublePoint *origin);
void drawCanvas_scroll_at_imagecenter(AppDraw *p,const mDoublePoint *dpt);
void drawCanvas_scroll_default(AppDraw *p);
void drawCanvas_zoomStep(AppDraw *p,mlkbool zoomup);
void drawCanvas_rotateStep(AppDraw *p,mlkbool left);
void drawCanvas_fitWindow(AppDraw *p);
void drawCanvas_mirror(AppDraw *p);
void drawCanvas_setCanvasSize(AppDraw *p,int w,int h);

void drawCanvas_update_scrollpos(AppDraw *p,mlkbool update);
void drawCanvas_update(AppDraw *p,int zoom,int angle,int flags);

/* update */

void drawUpdate_setCanvasBlendInfo(LayerItem *pi,TileImageBlendSrcInfo *info);

void drawUpdate_canvas(void);

void drawUpdate_all(void);
void drawUpdate_all_layer(void);

void drawUpdate_blendImage_full(AppDraw *p,const mBox *box);
void drawUpdate_blendImage_layer(AppDraw *p,const mBox *box);

void drawUpdate_drawCanvas(AppDraw *p,mPixbuf *pixbuf,const mBox *box);

void drawUpdateBox_canvaswg(AppDraw *p,const mBox *boximg);
void drawUpdateBox_canvaswg_direct(AppDraw *p,const mBox *boximg);

void drawUpdateBox_canvas(AppDraw *p,const mBox *boximg);
void drawUpdateBox_canvas_direct(AppDraw *p,const mBox *boximg);

void drawUpdateRect_canvas(AppDraw *p,const mRect *rc);
void drawUpdateRect_canvasview(AppDraw *p,const mRect *rc);

void drawUpdateRect_canvas_canvasview(AppDraw *p,const mRect *rc);
void drawUpdateRect_canvas_canvasview_inLayerHave(AppDraw *p,LayerItem *item);

void drawUpdateRect_canvaswg_forSelect(AppDraw *p,const mRect *rc);
void drawUpdateRect_canvas_forSelect(AppDraw *p,const mRect *rc);

void drawUpdateBox_canvaswg_forBoxSel(AppDraw *p,const mBox *boxsel,mlkbool direct);
void drawUpdateRect_canvas_forBoxSel(AppDraw *p,const mRect *rc);

void drawUpdate_endDraw_box(AppDraw *p,const mBox *boximg);

/* select */

mlkbool drawSel_isHave(void);
mlkbool drawSel_isEnable_copy_cut(mlkbool cut);
mlkbool drawSel_isEnable_outputFile(void);

void drawSel_release(AppDraw *p,mlkbool update);
void drawSel_inverse(AppDraw *p);
void drawSel_all(AppDraw *p);
void drawSel_expand(AppDraw *p,int cnt);
void drawSel_fill_erase(AppDraw *p,mlkbool erase);
void drawSel_copy_cut(AppDraw *p,mlkbool cut);
void drawSel_paste_newlayer(AppDraw *p);
void drawSel_fromLayer(AppDraw *p,int type);

void drawSel_getFullDrawRect(AppDraw *p,mRect *rc);
mlkbool drawSel_selImage_create(AppDraw *p);
void drawSel_selImage_freeEmpty(AppDraw *p);

/* boxsel */

void drawBoxSel_release(AppDraw *p,mlkbool direct);
void drawBoxSel_setRect(AppDraw *p,const mRect *rc);
void drawBoxSel_clearImage(AppDraw *p);
void drawBoxSel_restoreCursor(AppDraw *p);
void drawBoxSel_cancelPaste(AppDraw *p);
void drawBoxSel_onChangeState(AppDraw *p,int type);

mlkbool drawBoxSel_copy_cut(AppDraw *p,mlkbool cut);
mlkbool drawBoxSel_paste(AppDraw *p,const mPoint *ptstart);

void drawBoxSel_run_cutpaste(AppDraw *p,int no);
void drawBoxSel_run_edit(AppDraw *p,int no);

/* text */

void drawText_createFont(AppDraw *p);
void drawText_drawPreview(AppDraw *p);
void drawText_changeFontSize(AppDraw *p);
void drawText_changeInfo(AppDraw *p);
void drawText_setPoint_inDialog(AppDraw *p,double x,double y);

void drawText_clearItemRect(AppDraw *p,LayerItem *layer,LayerTextItem *item,mRect *rcupdate);
void drawText_drawLayerText(AppDraw *p,LayerTextItem *pi,TileImage *imgdraw,mRect *rcdraw);
