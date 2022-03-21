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

/**************************************
 * AppDraw
 * 
 * 選択範囲関連
 **************************************/

#include "mlk_gui.h"
#include "mlk_rectbox.h"
#include "mlk_str.h"
#include "mlk_file.h"

#include "def_draw.h"

#include "draw_main.h"
#include "draw_layer.h"
#include "draw_op_sub.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "layeritem.h"
#include "appconfig.h"

#include "popup_thread.h"


/*
  tileimg_sel : 選択範囲がない時は NULL または空の状態。
  sel.rcsel   : 選択範囲全体を含む矩形 (おおよその範囲)
*/

//---------------

#define _CURSOR_WAIT    drawCursor_wait()
#define _CURSOR_RESTORE drawCursor_restore()

#define _FILENAME_SELCOPY  "selcopy.dat"

//---------------



//======================
// sub
//======================


/* イメージ全体の範囲を取得 */

static void _get_image_rect(AppDraw *p,mRect *rc)
{
	rc->x1 = rc->y1 = 0;
	rc->x2 = p->imgw - 1;
	rc->y2 = p->imgh - 1;
}

/* 現在のレイヤにおける選択範囲の描画範囲を取得 */

static void _get_curlayer_drawrect(AppDraw *p,mRect *rc)
{
	if(drawOpSub_isCurLayer_folder())
		//フォルダの場合はイメージ全体
		_get_image_rect(p, rc);
	else
		//カレントレイヤの描画可能範囲
		TileImage_getCanDrawRect_pixel(p->curlayer->img, rc);
}

/* 選択範囲イメージを、指定範囲で再作成 */

static mlkbool _selimg_recreate(AppDraw *p,const mRect *rc)
{
	TileImage_free(p->tileimg_sel);

	p->tileimg_sel = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1BIT, rc);

	mRectEmpty(&p->sel.rcsel);

	return (p->tileimg_sel != NULL);
}


//======================
//
//======================


/** 選択範囲があるか */

mlkbool drawSel_isHave(void)
{
	return (APPDRAW->tileimg_sel && !mRectIsEmpty(&APPDRAW->sel.rcsel));
}

/** コピー/切り取りが可能な状態か */

mlkbool drawSel_isEnable_copy_cut(mlkbool cut)
{
	int ret;

	//選択範囲なし

	if(!drawSel_isHave()) return FALSE;

	if(cut)
		//切り取り
		// :フォルダ/テキスト/ロック時無効
		ret = drawOpSub_canDrawLayer(APPDRAW, CANDRAWLAYER_F_NO_HIDE);
	else
		//コピー
		// :ロック/テキスト時も有効
		ret = drawOpSub_canDrawLayer(APPDRAW, CANDRAWLAYER_F_ENABLE_READ | CANDRAWLAYER_F_ENABLE_TEXT);

	return (ret == 0);
}

/** 範囲内イメージをファイルに出力が有効か */

mlkbool drawSel_isEnable_outputFile(void)
{
	return (drawSel_isHave()
		&& !drawOpSub_isCurLayer_folder());
}


//======================
// コマンド
//======================


/** 選択範囲解除 */

void drawSel_release(AppDraw *p,mlkbool update)
{
	if(p->tileimg_sel)
	{
		//イメージ削除

		TileImage_free(p->tileimg_sel);
		p->tileimg_sel = NULL;

		//更新

		if(update)
			drawUpdateRect_canvaswg_forSelect(p, &p->sel.rcsel);
	}
}

/** 選択範囲を反転 */

void drawSel_inverse(AppDraw *p)
{
	if(!drawSel_selImage_create(p)) return;

	_CURSOR_WAIT;

	//描画

	TileImageDrawInfo_clearDrawRect();

	TileImage_inverseSelect(p->tileimg_sel);

	//透明部分を解放

	drawSel_selImage_freeEmpty(p);

	//更新

	drawUpdateRect_canvaswg_forSelect(p, &g_tileimage_dinfo.rcdraw);

	_CURSOR_RESTORE;
}

/** すべて選択 */

void drawSel_all(AppDraw *p)
{
	mRect rc;

	//イメージ再作成

	drawSel_release(p, FALSE);

	if(!drawSel_selImage_create(p)) return;

	//描画 (キャンバス範囲)

	_get_image_rect(p, &rc);

	g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new;

	p->w.drawcol = (uint64_t)-1;

	_CURSOR_WAIT;

	TileImage_drawFillBox(p->tileimg_sel, rc.x1, rc.y1,
		rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1, &p->w.drawcol);

	//範囲セット

	p->sel.rcsel = rc;

	//更新

	drawUpdateRect_canvaswg_forSelect(p, &rc);

	_CURSOR_RESTORE;
}

/** 範囲の拡張/縮小 */

static int _thread_expand(mPopupProgress *prog,void *data)
{
	TileImage_expandSelect(APPDRAW->tileimg_sel, *((int *)data), prog);

	return 1;
}

void drawSel_expand(AppDraw *p,int cnt)
{
	TileImageDrawInfo_clearDrawRect();

	//スレッド

	PopupThread_run(&cnt, _thread_expand);

	//

	if(cnt > 0)
		//拡張
		p->sel.rcsel = g_tileimage_dinfo.rcdraw;
	else
		//縮小
		drawSel_selImage_freeEmpty(p);

	//更新

	drawUpdateRect_canvaswg_forSelect(p, &g_tileimage_dinfo.rcdraw);
}

/** 塗りつぶし/消去 */

void drawSel_fill_erase(AppDraw *p,mlkbool erase)
{
	mRect rc;

	if(drawOpSub_canDrawLayer_mes(p, 0)) return;

	//範囲

	drawSel_getFullDrawRect(p, &rc);

	//描画
	//[!] beginDraw/endDraw でカーソルは変更される

	drawOpSub_setDrawInfo_fillerase(erase);
	drawOpSub_beginDraw_single(p);

	TileImage_drawFillBox(p->w.dstimg, rc.x1, rc.y1,
		rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1, &p->w.drawcol);

	drawOpSub_endDraw_single(p);
}

/** コピー/切り取り */

void drawSel_copy_cut(AppDraw *p,mlkbool cut)
{
	TileImage *img;
	mStr str = MSTR_INIT;

	if(!drawSel_isEnable_copy_cut(cut))
		return;

	//現在のコピーイメージを削除

	TileImage_free(p->sel.tileimg_copy);
	p->sel.tileimg_copy = NULL;

	//---------

	_CURSOR_WAIT;

	//----- コピー/切り取り

	//切り取り準備

	if(cut)
	{
		drawOpSub_setDrawInfo_select_cut();
		drawOpSub_beginDraw(p);
	}

	//コピー/切り取り

	img = TileImage_createSelCopyImage(p->curlayer->img, p->tileimg_sel,
		&p->sel.rcsel, cut);

	//切り取り後

	if(cut)
		drawOpSub_finishDraw_single(p);

	//---- イメージ保持
	// 一度コピーしたら終了時まで保持されるため、作業用ディレクトリにファイルを保存する。
	// 作業用ディレクトリがない or 保存に失敗した場合は、sel.tileimg_copy に保持。

	if(img)
	{
		p->sel.copyimg_bits = APPDRAW->imgbits;
		
		if(AppConfig_getTempPath(&str, _FILENAME_SELCOPY)
			&& TileImage_saveTmpImageFile(img, str.buf))
		{
			//ファイル出力に成功した場合、イメージ削除

			TileImage_free(img);
		}
		else
		{
			//ファイル出力できない場合、メモリ上に保持

			p->sel.tileimg_copy = img;
		}

		mStrFree(&str);
	}

	//

	_CURSOR_RESTORE;
}

/** 新規レイヤに貼り付け */

void drawSel_paste_newlayer(AppDraw *p)
{
	mStr str = MSTR_INIT;
	TileImage *img;

	//コピーイメージが存在しない

	if(!p->sel.tileimg_copy
		&& !(AppConfig_getTempPath(&str, _FILENAME_SELCOPY) && mIsExistFile(str.buf)) )
	{
		mStrFree(&str);
		return;
	}

	//イメージを複製 (ファイルの場合、読み込み)
	// :コピーした時のビット数と、現在のビット数が異なる場合がある。
	// :その場合は、タイルを変換する。

	if(p->sel.tileimg_copy)
		img = TileImage_newClone_bits(p->sel.tileimg_copy, p->sel.copyimg_bits, APPDRAW->imgbits);
	else
		img = TileImage_loadTmpImageFile(str.buf, p->sel.copyimg_bits, APPDRAW->imgbits);

	mStrFree(&str);

	if(!img) return;

	//新規レイヤ作成して、img をセット

	if(!drawLayer_newLayer_image(p, img))
		TileImage_free(img);
}

/** レイヤ上の色から選択 */

void drawSel_fromLayer(AppDraw *p,int type)
{
	mRect rc;

	if(drawOpSub_canDrawLayer_mes(p,
		CANDRAWLAYER_F_ENABLE_READ | CANDRAWLAYER_F_ENABLE_TEXT))
		return;

	//選択解除

	drawSel_release(p, TRUE);

	//点がある範囲

	if(!TileImage_getHaveImageRect_pixel(p->curlayer->img, &rc, NULL))
		return;

	//イメージ再作成

	if(!_selimg_recreate(p, &rc)) return;

	//描画

	g_tileimage_dinfo.func_setpixel = TileImage_setPixel_new_drawrect;

	TileImageDrawInfo_clearDrawRect();

	_CURSOR_WAIT;

	if(type == 0)
		//不透明部分
		TileImage_drawPixels_fromImage_opacity(p->tileimg_sel, p->curlayer->img, &rc);
	else
		//描画色部分
		TileImage_drawPixels_fromImage_color(p->tileimg_sel, p->curlayer->img, &p->col.drawcol, &rc);

	//セット

	rc = g_tileimage_dinfo.rcdraw;

	if(mRectIsEmpty(&rc))
		drawSel_release(p, FALSE);
	else
	{
		p->sel.rcsel = rc;

		drawUpdateRect_canvaswg_forSelect(p, &rc);
	}

	_CURSOR_RESTORE;
}


//======================
// 作業用処理
//======================


/** イメージ全体に対して描画する際の、描画範囲を取得
 *
 * 選択範囲があれば、その範囲のみ。 */

void drawSel_getFullDrawRect(AppDraw *p,mRect *rc)
{
	if(drawSel_isHave())
		*rc = p->sel.rcsel;
	else
		_get_curlayer_drawrect(p, rc);
}

/** 選択範囲イメージを作成
 *
 * 選択範囲の図形描画前に行う。
 *
 * return: FALSE で失敗 */

mlkbool drawSel_selImage_create(AppDraw *p)
{
	mRect rc;

	if(p->tileimg_sel)
		return TRUE;
	else
	{
		//作成
		
		_get_image_rect(p, &rc);

		p->tileimg_sel = TileImage_newFromRect(TILEIMAGE_COLTYPE_ALPHA1BIT, &rc);

		mRectEmpty(&p->sel.rcsel);

		return (p->tileimg_sel != NULL);
	}
}

/** 選択範囲の一部範囲削除後の処理
 *
 * 透明タイルを解放して、範囲を再計算。
 * すべて透明なら、イメージを削除。 */

void drawSel_selImage_freeEmpty(AppDraw *p)
{
	if(p->tileimg_sel)
	{
		if(TileImage_freeEmptyTiles(p->tileimg_sel))
		{
			//すべて透明なら削除
			
			TileImage_free(p->tileimg_sel);
			p->tileimg_sel = NULL;
		}
		else
		{
			//点が残っていれば、範囲を再計算
			// :タイル単位での範囲となる。

			TileImage_getHaveImageRect_pixel(p->tileimg_sel, &p->sel.rcsel, NULL);
		}
	}
}

