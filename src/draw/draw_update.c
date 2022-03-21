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
 * AppDraw: 更新関連
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget.h"
#include "mlk_rectbox.h"

#include "def_widget.h"
#include "def_config.h"
#include "def_draw.h"

#include "imagecanvas.h"
#include "drawpixbuf.h"
#include "layerlist.h"
#include "layeritem.h"
#include "tileimage.h"
#include "panel_func.h"

#include "draw_op_def.h"
#include "draw_main.h"
#include "draw_calc.h"
#include "draw_rule.h"



/** LayerItem から、キャンバス合成用の情報をセット */

void drawUpdate_setCanvasBlendInfo(LayerItem *pi,TileImageBlendSrcInfo *info)
{
	info->opacity = LayerItem_getOpacity_real(pi);
	info->blendmode = pi->blendmode;
	info->img_texture = pi->img_texture;
	info->tone_lines = 0;

	//トーン化

	if((pi->flags & LAYERITEM_F_TONE)
		&& (pi->type == LAYERTYPE_GRAY || pi->type == LAYERTYPE_ALPHA1BIT))
	{
		info->tone_lines = pi->tone_lines;
		info->tone_angle = pi->tone_angle;
		info->tone_density = pi->tone_density;
		info->ftone_white = (LAYERITEM_IS_TONE_WHITE(pi) != 0);
	}
}

/* 通常表示用のキャンバス合成用の情報をセット */

static void _setcanvasblendinfo_normal(TileImageBlendSrcInfo *info)
{
	info->opacity = LAYERITEM_OPACITY_MAX;
	info->blendmode = 0;
	info->img_texture = NULL;
	info->tone_lines = 0;
}


//======================


/** キャンバス領域を更新 */

void drawUpdate_canvas(void)
{
	mWidgetRedraw(MLK_WIDGET(APPWIDGET->canvaspage));
}

/** すべて更新
 *
 * [ 合成イメージ/キャンバス/(panel)キャンバスビュー ] */

void drawUpdate_all(void)
{
	drawUpdate_blendImage_full(APPDRAW, NULL);
	drawUpdate_canvas();

	PanelCanvasView_update();
}

/** すべて + レイヤ一覧を更新 */

void drawUpdate_all_layer(void)
{
	drawUpdate_all();

	PanelLayer_update_all();
}


//===========================
// キャンバスイメージ更新
//===========================


/** キャンバスイメージを更新
 *
 * box: NULL で全体 */

void drawUpdate_blendImage_full(AppDraw *p,const mBox *box)
{
	mBox box1;

	if(!box)
	{
		box1.x = box1.y = 0;
		box1.w = p->imgw, box1.h = p->imgh;

		box = &box1;
	}

	//背景

	if(APPCONF->fview & CONFIG_VIEW_F_BKGND_PLAID)
	{
		//チェック柄
		ImageCanvas_fillPlaidBox(p->imgcanvas, box,
			p->col.checkbkcol, p->col.checkbkcol + 1);
	}
	else
	{
		//指定色

		if(box == &box1)
			ImageCanvas_fill(p->imgcanvas, &p->imgbkcol);
		else
			ImageCanvas_fillBox(p->imgcanvas, box, &p->imgbkcol);
	}

	//レイヤ合成

	drawUpdate_blendImage_layer(p, box);
}

/** レイヤイメージを ImageCanvas に合成
 *
 * [!] 背景は描画しない */

void drawUpdate_blendImage_layer(AppDraw *p,const mBox *box)
{
	TileImage *img_insert = NULL;
	LayerItem *pi,*current;
	TileImageBlendSrcInfo info;

	//挿入イメージ

	if(p->w.optype == DRAW_OPTYPE_SELIMGMOVE
		|| p->w.optype == DRAW_OPTYPE_TMPIMG_MOVE
		|| (p->text.in_dialog && p->text.fpreview))
	{
		//[選択範囲イメージの移動/コピー中]
		//[テキストレイヤのテキスト位置移動中]
		//[テキストダイアログ中のプレビュー]

		img_insert = p->tileimg_tmp;
	}
	else if(p->in_filter_dialog && p->tileimg_filterprev)
	{
		//フィルタダイアログ中、キャンバスで挿入イメージをプレビュー

		img_insert = p->tileimg_filterprev;
	}

	//合成

	pi = LayerList_getItem_bottomVisibleImage(p->layerlist);

	if(img_insert)
	{
		//---- img_insert をカレントの上に挿入

		current = p->curlayer;

		for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		{
			drawUpdate_setCanvasBlendInfo(pi, &info);
		
			TileImage_blendToCanvas(pi->img, p->imgcanvas, box, &info);

			//カレントの上に、同じレイヤパラメータで挿入

			if(pi == current)
				TileImage_blendToCanvas(img_insert, p->imgcanvas, box, &info);
		}
	}
	else
	{
		//----- 通常時
		
		for(; pi; pi = LayerItem_getPrevVisibleImage(pi))
		{
			drawUpdate_setCanvasBlendInfo(pi, &info);
		
			TileImage_blendToCanvas(pi->img, p->imgcanvas, box, &info);
		}
	}

	//(切り貼りツールの貼り付け時) 一番上に描画
	// :描画対象のレイヤは確定時に決定する

	if(p->boxsel.is_paste_mode)
	{
		_setcanvasblendinfo_normal(&info);
	
		TileImage_blendToCanvas(p->boxsel.img, p->imgcanvas, box, &info);
	}
}


//===========================
// キャンバス描画
//===========================


/** キャンバスウィジェットに描画
 *
 * 合成イメージをソースとして mPixbuf に描画。
 *
 * pixbuf: 描画対象のキャンバスエリアの mPixbuf */

void drawUpdate_drawCanvas(AppDraw *p,mPixbuf *pixbuf,const mBox *box)
{
	CanvasDrawInfo di;
	mBox boximg;
	int n;

	//描画情報

	di.boxdst = *box;
	di.originx = p->imgorigin.x;
	di.originy = p->imgorigin.y;
	di.scrollx = p->canvas_scroll.x - (p->canvas_size.w >> 1);
	di.scrolly = p->canvas_scroll.y - (p->canvas_size.h >> 1);
	di.mirror  = p->canvas_mirror;
	di.imgw = p->imgw;
	di.imgh = p->imgh;
	di.bkgndcol = APPCONF->canvasbkcol;
	di.param = &p->viewparam;

	//----- キャンバスイメージ描画

	if(p->canvas_angle)
	{
		//回転あり
		// :キャンバス移動中、または、倍率100%で90度単位の場合は低品質

		n = p->canvas_angle;
		
		if(p->canvas_lowquality
			|| (p->canvas_zoom == 1000 && (n == 9000 || n == 18000 || n == 27000)))
			ImageCanvas_drawPixbuf_rotate(p->imgcanvas, pixbuf, &di);
		else
			ImageCanvas_drawPixbuf_rotate_oversamp(p->imgcanvas, pixbuf, &di);
	}
	else
	{
		//回転なし (100%は除く200%以下時は、高品質で)

		if(!p->canvas_lowquality && p->canvas_zoom != 1000 && p->canvas_zoom < 2000)
			ImageCanvas_drawPixbuf_oversamp(p->imgcanvas, pixbuf, &di);
		else 
			ImageCanvas_drawPixbuf_nearest(p->imgcanvas, pixbuf, &di);
	}

	//------ キャンバス範囲に対応するイメージ範囲をもとに描画

	if(drawCalc_canvas_to_image_box(p, &boximg, &di.boxdst))
	{
		//1pxグリッド

		if((APPCONF->grid.pxgrid_zoom & 0x8000)
			&& p->canvas_zoom >= (APPCONF->grid.pxgrid_zoom & 0xff) * 1000)
		{
			drawpixbuf_grid(pixbuf, &di.boxdst, &boximg, 1, 1, 0x5ac1a9b0, &di);
		}
		
		//グリッド
		
		if(APPCONF->fview & CONFIG_VIEW_F_GRID)
		{
			drawpixbuf_grid(pixbuf, &di.boxdst, &boximg,
				APPCONF->grid.gridw, APPCONF->grid.gridh, APPCONF->grid.col_grid, &di);
		}

		//分割線

		if(APPCONF->fview & CONFIG_VIEW_F_GRID_SPLIT)
		{
			drawpixbuf_grid_split(pixbuf, &di.boxdst, &boximg,
				APPCONF->grid.splith, APPCONF->grid.splitv, APPCONF->grid.col_split, &di);
		}

		//選択範囲

		if(p->tileimg_sel && !mRectIsEmpty(&p->sel.rcsel) && !p->sel.is_hide)
			TileImage_drawSelectEdge(p->tileimg_sel, pixbuf, &di, &boximg);
	}

	//矩形編集枠

	if(p->boxsel.box.w)
	{
		drawpixbuf_boxsel_frame(pixbuf, &p->boxsel.box, &di,
			(p->canvas_angle == 0), p->boxsel.is_paste_mode);
	}

	//定規ガイド

	if(drawRule_isVisibleGuide(p))
		(p->rule.func_draw_guide)(p, pixbuf, box);

	//テキストツール時、選択テキストの枠

	if(p->w.optype == DRAW_OPTYPE_TEXTLAYER_SEL)
	{
		drawpixbuf_rectframe(pixbuf, &p->w.rctmp[0], &di,
			(p->canvas_angle == 0), 0x80ffa200);
	}
}


//=============================
// 範囲更新
//=============================


/** キャンバスウィジェットの範囲更新 (GUI の描画タイミングで)
 *
 * boximg: イメージ範囲 */

void drawUpdateBox_canvaswg(AppDraw *p,const mBox *boximg)
{
	mBox boxc;

	if(drawCalc_image_to_canvas_box(p, &boxc, boximg))
	{
		mWidgetRedrawBox(MLK_WIDGET(APPWIDGET->canvaspage), &boxc);
	}
}

/** キャンバスウィジェットの範囲を即時更新 (この後に XOR 描画を行う場合など) */

void drawUpdateBox_canvaswg_direct(AppDraw *p,const mBox *boximg)
{
	mWidget *wg = MLK_WIDGET(APPWIDGET->canvaspage);
	mPixbuf *pixbuf;
	mBox boxc;

	if(drawCalc_image_to_canvas_box(p, &boxc, boximg))
	{
		pixbuf = mWidgetDirectDraw_begin(wg);
		if(pixbuf)
		{
			drawUpdate_drawCanvas(p, pixbuf, &boxc);

			mWidgetDirectDraw_end(wg, pixbuf);
			mWidgetUpdateBox(wg, &boxc);
		}
	}
}

/** キャンバスを範囲更新 (合成イメージ + キャンバスエリア)
 *
 * boximg: [!] イメージ範囲内であること */

void drawUpdateBox_canvas(AppDraw *p,const mBox *boximg)
{
	//合成イメージ

	drawUpdate_blendImage_full(p, boximg);

	//キャンバス

	drawUpdateBox_canvaswg(p, boximg);
}

/** キャンバスを即時更新 */

void drawUpdateBox_canvas_direct(AppDraw *p,const mBox *boximg)
{
	//合成イメージ

	drawUpdate_blendImage_full(p, boximg);

	//キャンバス

	drawUpdateBox_canvaswg_direct(p, boximg);
}


//==============


/** キャンバスを範囲更新
 *
 * rc: イメージ範囲 */

void drawUpdateRect_canvas(AppDraw *p,const mRect *rc)
{
	mBox box;

	if(drawCalc_image_rect_to_box(p, &box, rc))
		drawUpdateBox_canvas(p, &box);
}

/** キャンバスビューを範囲更新 */

void drawUpdateRect_canvasview(AppDraw *p,const mRect *rc)
{
	mBox box;

	if(drawCalc_image_rect_to_box(p, &box, rc))
		PanelCanvasView_updateBox(&box);
}

/** キャンバス + キャンバスビューを範囲更新
 *
 * rc: イメージ範囲 */

void drawUpdateRect_canvas_canvasview(AppDraw *p,const mRect *rc)
{
	mBox box;

	if(drawCalc_image_rect_to_box(p, &box, rc))
	{
		drawUpdateBox_canvas(p, &box);

		PanelCanvasView_updateBox(&box);
	}
}

/** 指定レイヤの表示部分の範囲を更新
 *
 * フォルダの場合は、フォルダ下の全ての表示レイヤの範囲。
 * 通常レイヤの場合、レイヤが非表示でも、存在する範囲が更新される。
 *
 * item: NULL でカレントレイヤ */

void drawUpdateRect_canvas_canvasview_inLayerHave(AppDraw *p,LayerItem *item)
{
	mRect rc;

	if(!item) item = p->curlayer;

	if(LayerItem_getVisibleImageRect(item, &rc))
		drawUpdateRect_canvas_canvasview(p, &rc);
}

/** キャンバスウィジェットを範囲更新
 *
 * 選択範囲の枠の更新用。
 *
 * rc: イメージ範囲 */

void drawUpdateRect_canvaswg_forSelect(AppDraw *p,const mRect *rc)
{
	mBox box;
	int n;

	if(!mRectIsEmpty(rc))
	{
		mBoxSetRect(&box, rc);

		//キャンバス座標上における 2px 分を拡張
	
		n = (int)(p->viewparam.scalediv * 2 + 0.5);
		if(n == 0) n = 1;

		box.x -= n, box.y -= n;
		n *= 2;
		box.w += n, box.h += n;

		drawUpdateBox_canvaswg(p, &box);
	}
}

/** キャンバスを範囲更新 (選択範囲用)
 *
 * イメージと枠の更新。 */

void drawUpdateRect_canvas_forSelect(AppDraw *p,const mRect *rc)
{
	mBox box;

	//キャンバスイメージ

	if(drawCalc_image_rect_to_box(p, &box, rc))
		drawUpdate_blendImage_full(p, &box);

	//キャンバスウィジェット

	drawUpdateRect_canvaswg_forSelect(p, rc);
}

/** キャンバスウィジェットの範囲更新 (矩形選択用)
 *
 * 範囲の枠のみを更新する場合。
 *
 * direct: 即時更新するか */

void drawUpdateBox_canvaswg_forBoxSel(AppDraw *p,const mBox *boxsel,mlkbool direct)
{
	mBox box;
	int n;

	if(boxsel->w)
	{
		//キャンバス座標上における 4px 分を拡張
	
		n = (int)(p->viewparam.scalediv * 4 + 0.5);
		if(n == 0) n = 1;

		box = *boxsel;

		box.x -= n, box.y -= n;
		n *= 2;
		box.w += n, box.h += n;

		if(direct)
			drawUpdateBox_canvaswg_direct(p, &box);
		else
			drawUpdateBox_canvaswg(p, &box);
	}
}

/** キャンバスの範囲更新 (矩形選択用)
 *
 * 貼り付け中の場合。
 * 枠はイメージ範囲外でもはみ出す形で表示する。
 *
 * rc: NULL で現在の選択範囲 */

void drawUpdateRect_canvas_forBoxSel(AppDraw *p,const mRect *rc)
{
	mRect rc2;
	mBox box;

	if(rc)
		rc2 = *rc;
	else
		mRectSetBox(&rc2, &p->boxsel.box);

	//キャンバスイメージ

	if(drawCalc_image_rect_to_box(p, &box, &rc2))
		drawUpdate_blendImage_full(p, &box);

	//キャンバスウィジェット

	mBoxSetRect(&box, &rc2);

	drawUpdateBox_canvaswg_forBoxSel(p, &box, FALSE);
}

/** 描画終了時の範囲更新
 *
 * boximg: イメージ範囲。x < 0 で範囲なし */

void drawUpdate_endDraw_box(AppDraw *p,const mBox *boximg)
{
	//キャンバスビュー

	if(boximg->x >= 0)
		PanelCanvasView_updateBox(boximg);
}


