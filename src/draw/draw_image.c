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
 * AppDraw: イメージ関連
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_popup_progress.h"

#include "def_macro.h"
#include "def_config.h"
#include "def_draw.h"
#include "def_draw_sub.h"

#include "imagecanvas.h"
#include "imagematerial.h"
#include "tileimage.h"
#include "layerlist.h"
#include "layeritem.h"
#include "table_data.h"
#include "undo.h"

#include "mainwindow.h"
#include "statusbar.h"
#include "panel_func.h"
#include "popup_thread.h"

#include "draw_main.h"


/* イメージサイズ変更時 */

static void _change_imagesize(AppDraw *p)
{
	TileImage_global_setImageSize(p->imgw, p->imgh);
}

/** キャンバスが新しくなった時の更新
 *
 * change_file: TRUE=編集ファイルが変わった。FALSE=イメージサイズのみの変更。 */

void drawImage_afterNewCanvas(AppDraw *p,mlkbool change_file)
{
	//選択範囲解除

	drawSel_release(p, FALSE);

	//矩形選択の状態変更

	drawBoxSel_onChangeState(p, 0);

	//ビット数が変わった時、スタンプイメージクリア

	if(p->imgbits != p->stamp.bits)
		drawStamp_clearImage(p);

	//

	_change_imagesize(p);

	//ウィンドウに合わせる

	drawCanvas_fitWindow(p);

	//

	drawUpdate_all_layer();

	PanelCanvasView_changeImageSize();
}

/** 途中でキャンバスサイズを変更する時の処理 */

mlkbool drawImage_changeImageSize(AppDraw *p,int w,int h)
{
	//キャンバスイメージ

	ImageCanvas_free(p->imgcanvas);

	if(!(p->imgcanvas = ImageCanvas_new(w, h, p->imgbits)))
	{
		//失敗した場合、元のサイズで
		p->imgcanvas = ImageCanvas_new(p->imgw, p->imgh, p->imgbits);
		return FALSE;
	}

	//

	p->imgw = w;
	p->imgh = h;

	return TRUE;
}

/** DPI の変更 */

void drawImage_changeDPI(AppDraw *p,int dpi)
{
	p->imgdpi = dpi;

	TileImage_global_setDPI(dpi);
}


//===========================
// 作成
//===========================


/** レイヤ付きファイルを開いた時のイメージ作成
 *
 * イメージ背景色は白。レイヤは作成しない。 */

mlkbool drawImage_newCanvas_openFile(AppDraw *p,int w,int h,int bits,int dpi)
{
	NewCanvasValue cv;

	cv.size.w = w;
	cv.size.h = h;
	cv.bit = bits;
	cv.dpi = dpi;
	cv.layertype = -1;
	cv.imgbkcol = 0xffffff;

	return drawImage_newCanvas(p, &cv);
}

/** レイヤ付きファイルを開いた時のイメージ作成 (背景色付き) */

mlkbool drawImage_newCanvas_openFile_bkcol(AppDraw *p,int w,int h,int bits,int dpi,uint32_t bkcol)
{
	NewCanvasValue cv;

	cv.size.w = w;
	cv.size.h = h;
	cv.bit = bits;
	cv.dpi = dpi;
	cv.layertype = -1;
	cv.imgbkcol = bkcol;

	return drawImage_newCanvas(p, &cv);
}

/** 新規イメージ作成
 *
 * val: NULL で起動時サイズ
 *  (layertype が負の値で、レイヤ作成しない) */

mlkbool drawImage_newCanvas(AppDraw *p,NewCanvasValue *val)
{
	ConfigNewCanvas *cf;
	int w,h,dpi,bits,layertype;
	uint32_t imgbkcol;

	if(val)
	{
		w = val->size.w;
		h = val->size.h;
		dpi = val->dpi;
		bits = val->bit;
		layertype = val->layertype;
		imgbkcol = val->imgbkcol;
	}
	else
	{
		cf = &APPCONF->newcanvas_init;
		w = cf->w;
		h = cf->h;
		dpi = cf->dpi;
		bits = cf->bit;
		layertype = 0;
		imgbkcol = 0xffffff;
	}

	if(w < 1) w = 1;
	if(h < 1) h = 1;
	if(dpi <= 0) dpi = 96;

	//UNDO クリア

	Undo_deleteAll();
	Undo_setModifyFlag_off();

	//レイヤリストをクリア

	LayerList_clear(p->layerlist);

	//イメージ情報

	p->imgw = w;
	p->imgh = h;
	p->imgdpi = dpi;
	p->imgbits = bits;

	RGB32bit_to_RGBcombo(&p->imgbkcol, imgbkcol);

	//
	
	p->curlayer = NULL;
	p->masklayer = NULL;

	_change_imagesize(p);

	//ビットなどのセット

	TileImage_global_setDPI(dpi);

	TileImage_global_setImageBits(bits);

	TableData_setToneTable(bits);

	//読み込み時用のフラグ

	p->fnewcanvas = TRUE;

	//キャンバスイメージ

	ImageCanvas_free(p->imgcanvas);

	if(!(p->imgcanvas = ImageCanvas_new(w, h, bits)))
		return FALSE;

	//空の新規レイヤ追加

	if(layertype >= 0)
	{
		if(!(p->curlayer = LayerList_addLayer_newimage(p->layerlist, layertype)))
			return FALSE;
	}

	return TRUE;
}


//=============================
// イメージビット数の変更
//=============================


/** イメージビット数の変更処理
 *
 * エラー時も更新は行うこと。
 *
 * prog: NULL でアンドゥ処理時 */

mlkerr drawImage_changeImageBits_proc(AppDraw *p,mPopupProgress *prog)
{
	int bits;

	bits = (p->imgbits == 8)? 16: 8;

	//キャンバスイメージ
	// :失敗時は元に戻す

	ImageCanvas_free(p->imgcanvas);

	if(!(p->imgcanvas = ImageCanvas_new(p->imgw, p->imgh, bits)))
	{
		p->imgcanvas = ImageCanvas_new(p->imgw, p->imgh, p->imgbits);

		return MLKERR_ALLOC;
	}

	//prog == NULL でアンドゥ処理時

	if(prog)
	{
		Undo_addChangeImageBits();

		//レイヤイメージの変換

		mPopupProgressThreadSetMax(prog, LayerList_getNormalLayerNum(p->layerlist) * 5);

		LayerList_convertImageBits(p->layerlist, bits, prog);
	}

	//トーン化テーブル
	
	TableData_setToneTable(bits);

	//変更

	p->imgbits = bits;

	TileImage_global_setImageBits(p->imgbits);

	return MLKERR_OK;
}

/* スレッド */

static int _thread_changeimgbits(mPopupProgress *prog,void *data)
{
	return drawImage_changeImageBits_proc(APPDRAW, prog);
}

/** イメージビット数変更 */

void drawImage_changeImageBits(AppDraw *p)
{
	int ret;

	//実行

	APPDRAW->in_thread_imgcanvas = TRUE;

	ret = PopupThread_run(NULL, _thread_changeimgbits);

	APPDRAW->in_thread_imgcanvas = FALSE;

	//

	if(ret)
	{
		MainWindow_errmes(ret, NULL);

		//imgcanvas の作成に失敗した場合
		drawUpdate_all();
		return;
	}

	//更新

	drawImage_update_imageBits();
}

/** ビット数変更時の更新 */

void drawImage_update_imageBits(void)
{
	//矩形選択の状態変更

	drawBoxSel_onChangeState(APPDRAW, 1);

	//スタンプイメージクリア

	drawStamp_clearImage(APPDRAW);

	//

	drawUpdate_all();

	StatusBar_setImageInfo();
}


//=============================
// キャンバスサイズ変更
//=============================


typedef struct
{
	int w,h,offx,offy;
}_thdata_resizecanvas;


/* 切り取り時スレッド */

static int _thread_resize_canvas(mPopupProgress *prog,void *data)
{
	_thdata_resizecanvas *p = (_thdata_resizecanvas *)data;
	LayerItem *pi;
	TileImage *img;

	mPopupProgressThreadSetMax(prog, LayerList_getNormalLayerNum(APPDRAW->layerlist));

	for(pi = LayerList_getTopItem(APPDRAW->layerlist); pi; pi = LayerItem_getNext(pi))
	{
		if(pi->img)
		{
			if(LAYERITEM_IS_TEXT(pi))
			{
				//テキストレイヤ時は位置のみ移動

				LayerItem_moveImage(pi, p->offx, p->offy);
			}
			else
			{
				img = TileImage_createCropImage(pi->img, p->offx, p->offy, p->w, p->h);

				if(img)
					LayerItem_replaceImage(pi, img, -1);
			}
		}

		mPopupProgressThreadIncPos(prog);
	}

	return TRUE;
}

/** キャンバスサイズ変更 */

mlkbool drawImage_resizeCanvas(AppDraw *p,int w,int h,int movx,int movy,int fcrop)
{
	_thdata_resizecanvas dat;
	int ret,sw,sh;

	sw = p->imgw;
	sh = p->imgh;

	//サイズ変更

	if(!drawImage_changeImageSize(p, w, h))
		return FALSE;

	//

	if(!fcrop)
	{
		//----- 切り取らない
		
		//undo

		Undo_addResizeCanvas_moveOffset(movx, movy, sw, sh);

		//全レイヤのオフセット位置を相対移動

		LayerList_moveOffset_rel_all(p->layerlist, movx, movy);

		ret = TRUE;
	}
	else
	{
		//----- 範囲外を切り取り
		//新しいサイズが元より大きくなる場合でも、
		//レイヤイメージ内には新しいサイズの範囲外にイメージが残る場合があるので、常に処理する。

		//undo

		Undo_addResizeCanvas_crop(movx, movy, sw, sh);

		//

		dat.w = w, dat.h = h;
		dat.offx = movx;
		dat.offy = movy;

		ret = PopupThread_run(&dat, _thread_resize_canvas);
		if(ret == -1) ret = FALSE;
	}

	return ret;
}


//============================
// 画像を統合して拡大縮小
//============================


typedef struct
{
	TileImage *img;
	int w,h,method;
}_thdata_scalecanvas;


static int _thread_scale_canvas(mPopupProgress *prog,void *data)
{
	_thdata_scalecanvas *p = (_thdata_scalecanvas *)data;

	mPopupProgressThreadSetMax(prog, 20 + 50 + 10);

	//ImageCanvas に合成

	drawImage_blendImageReal_curbits(APPDRAW, prog, 20);

	//リサイズ
	// :imgcanvas はリサイズ後のサイズになる。

	APPDRAW->imgcanvas = ImageCanvas_resize(APPDRAW->imgcanvas, p->w, p->h, p->method, prog, 50);

	if(!APPDRAW->imgcanvas)
		return 1;

	//アルファ値を最大に

	ImageCanvas_setAlphaMax(APPDRAW->imgcanvas);

	//ImageCanvas から TileImage にセット (RGBA)

	TileImage_convertFromCanvas(p->img, APPDRAW->imgcanvas, prog, 10);

	return 0;
}

/** 画像を統合して拡大縮小 */

mlkbool drawImage_scaleCanvas(AppDraw *p,int w,int h,int dpi,int method)
{
	_thdata_scalecanvas dat;
	TileImage *img;
	LayerItem *item;
	int ret;

	//統合後のイメージを作成

	img = TileImage_new(TILEIMAGE_COLTYPE_RGBA, w, h);
	if(!img) return FALSE;

	//スレッド

	dat.img = img;
	dat.w = w;
	dat.h = h;
	dat.method = method;

	APPDRAW->in_thread_imgcanvas = TRUE;

	ret = PopupThread_run(&dat, _thread_scale_canvas);

	APPDRAW->in_thread_imgcanvas = FALSE;

	//

	if(ret)
	{
		//リサイズ時にバッファ確保失敗した時

		if(!p->imgcanvas)
			p->imgcanvas = ImageCanvas_new(p->imgw, p->imgh, p->imgbits);

		TileImage_free(img);
		
		return FALSE;
	}

	//undo

	Undo_addScaleCanvas();

	//レイヤクリア

	LayerList_clear(p->layerlist);

	//レイヤ追加

	item = LayerList_addLayer(p->layerlist, NULL);

	LayerList_setItemName_curlayernum(p->layerlist, item);

	LayerItem_replaceImage(item, img, LAYERTYPE_RGBA);

	//

	p->curlayer = item;

	//サイズ変更

	p->imgw = w;
	p->imgh = h;

	//DPI 変更

	if(dpi != -1)
		drawImage_changeDPI(p, dpi);

	return TRUE;
}



//=============================
// レイヤ合成イメージ
//=============================


/* 変換テーブルを作成 (16bit 時) */

static mlkbool _create_colconv_table(int dstbits,uint8_t **ppdst8,uint16_t **ppdst16)
{
	*ppdst8 = NULL;
	*ppdst16 = NULL;

	if(APPDRAW->imgbits == 16)
	{
		if(dstbits == 8)
		{
			*ppdst8 = TileImage_create16fixto8_table();
			if(!(*ppdst8)) return FALSE;
		}
		else
		{
			*ppdst16 = TileImage_create16fixto16_table();
			if(!(*ppdst16)) return FALSE;
		}
	}

	return TRUE;
}

/** アルファなしでレイヤ合成 (現在のビット値で) */

void drawImage_blendImageReal_curbits(AppDraw *p,mPopupProgress *prog,int stepnum)
{
	LayerItem *pi;
	mBox box;
	TileImageBlendSrcInfo info;

	box.x = box.y = 0;
	box.w = p->imgw, box.h = p->imgh;

	//背景色でクリア

	ImageCanvas_fill(p->imgcanvas, &p->imgbkcol);

	//レイヤ合成

	mPopupProgressThreadSubStep_begin(prog, stepnum, LayerList_getBlendLayerNum(p->layerlist));

	pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

	for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
	{
		drawUpdate_setCanvasBlendInfo(pi, &info);

		TileImage_blendToCanvas(pi->img, p->imgcanvas, &box, &info);

		mPopupProgressThreadSubStep_inc(prog);
	}
}

/** アルファ無しで合成 (画像保存用)
 *
 * R,G,B 順で、アルファ値はない。 */

mlkerr drawImage_blendImageReal_normal(AppDraw *p,int dstbits,mPopupProgress *prog,int stepnum)
{
	uint8_t **ppbuf,*pd,*ps,*table8;
	uint16_t *table16,*ps16,*pd16;
	int ix,iy,i;

	//現在のビット値で合成
	
	drawImage_blendImageReal_curbits(p, prog, stepnum);

	//変換

	ppbuf = p->imgcanvas->ppbuf;

	if(p->imgbits == 8)
	{
		//----- 8bit: アルファ値を詰める

		for(iy = p->imgh; iy; iy--)
		{
			pd = *(ppbuf++);
			ps = pd;

			for(ix = p->imgw; ix; ix--, ps += 4, pd += 3)
			{
				pd[0] = ps[0];
				pd[1] = ps[1];
				pd[2] = ps[2];
			}
		}
	}
	else
	{
		//---- 16bit: 値を変換 & アルファ値詰める

		if(!_create_colconv_table(dstbits, &table8, &table16))
			return MLKERR_ALLOC;

		for(iy = p->imgh; iy; iy--)
		{
			pd = *(ppbuf++);
			pd16 = (uint16_t *)pd;
			ps16 = (uint16_t *)pd;

			for(ix = p->imgw; ix; ix--, ps16 += 4)
			{
				if(dstbits == 8)
				{
					for(i = 0; i < 3; i++)
						pd[i] = table8[ps16[i]];

					pd += 3;
				}
				else
				{
					for(i = 0; i < 3; i++)
						pd16[i] = table16[ps16[i]];

					pd16 += 3;
				}
			}
		}

		mFree(table8);
		mFree(table16);
	}

	return MLKERR_OK;
}

/** アルファ付きで合成
 *
 * - 合成モードはすべて「通常」とする。
 * - トーンレイヤはトーン処理なし。
 *
 * dstbits: 出力ビット数 */

mlkerr drawImage_blendImageReal_alpha(AppDraw *p,int dstbits,mPopupProgress *prog,int stepnum)
{
	LayerItem *pi;
	uint8_t **ppbuf,*pd,*table8;
	uint16_t *pd16,*ps16,*table16;
	int w,h,ix,iy,i,bits,a;
	uint64_t colres,colsrc;
	TileImagePixelColorFunc func_blend;

	ppbuf = p->imgcanvas->ppbuf;
	w = p->imgw;
	h = p->imgh;
	bits = p->imgbits;

	func_blend = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_NORMAL);

	//変換テーブル

	if(!_create_colconv_table(dstbits, &table8, &table16))
		return MLKERR_ALLOC;

	//

	mPopupProgressThreadSubStep_begin(prog, stepnum, h);

	for(iy = 0; iy < h; iy++)
	{
		pd = *(ppbuf++);
		
		for(ix = 0; ix < w; ix++)
		{
			colres = 0;

			//各レイヤ合成

			pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

			for( ; pi; pi = LayerItem_getPrevVisibleImage(pi))
			{
				TileImage_getPixel(pi->img, ix, iy, &colsrc);

				if(bits == 8)
					a = *((uint8_t *)&colsrc + 3);
				else
					a = *((uint16_t *)&colsrc + 3);

				if(a)
				{
					//テクスチャ
					
					if(pi->img_texture)
						a = a * ImageMaterial_getPixel_forTexture(pi->img_texture, ix, iy) / 255;

					//レイヤ不透明度

					a = a * LayerItem_getOpacity_real(pi) >> 7;

					//アルファ合成 (res + src -> res)

					if(a)
					{
						if(bits == 8)
							*((uint8_t *)&colsrc + 3) = a;
						else
							*((uint16_t *)&colsrc + 3) = a;
					
						(func_blend)(pi->img, &colres, &colsrc, NULL);
					}
				}
			}

			//セット

			if(bits == 8)
			{
				//8bit は常に 8bit

				*((uint32_t *)pd) = *((uint32_t *)&colres);

				pd += 4;
			}
			else if(dstbits == 8)
			{
				//16bit -> 8bit

				if(colres == 0)
					*((uint32_t *)pd) = 0;
				else
				{
					ps16 = (uint16_t *)&colres;
					
					for(i = 0; i < 4; i++)
						pd[i] = table8[ps16[i]];
				}

				pd += 4;
			}
			else if(dstbits == 16)
			{
				//16bit(fix15bit) -> 16bit

				if(colres == 0)
					*((uint64_t *)pd) = 0;
				else
				{
					pd16 = (uint16_t *)pd;
					ps16 = (uint16_t *)&colres;

					for(i = 0; i < 4; i++)
						pd16[i] = table16[ps16[i]];
				}

				pd += 8;
			}
		}

		mPopupProgressThreadSubStep_inc(prog);
	}

	mFree(table8);
	mFree(table16);

	return MLKERR_OK;
}

