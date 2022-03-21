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
 * AppDraw
 * 操作 - テキスト
 *****************************************/

#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_window.h"
#include "mlk_rectbox.h"
#include "mlk_str.h"
#include "mlk_font.h"
#include "mlk_list.h"
#include "mlk_sysdlg.h"
#include "mlk_menu.h"

#include "def_widget.h"
#include "def_draw.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "layeritem.h"
#include "font.h"
#include "undo.h"
#include "appcursor.h"

#include "panel_func.h"
#include "maincanvas.h"

#include "trid.h"


//-----------------

#define _TEXT_MAXNUM  200

enum
{
	TRID_MENU_NEW,
	TRID_MENU_EDIT_SUB,
	TRID_MENU_DEL_SUB,
	TRID_MENU_COPY,
	TRID_MENU_PASTE,
	TRID_MENU_REDRAW,
	TRID_MENU_EDIT_THIS,
	TRID_MENU_DEL_THIS
};

mlkbool TextDialog_run(void);

//-----------------


//=============================
// フォントの点描画関数
//=============================


/* (preview) モノクロ */

static void _setpixel_prev_mono(int x,int y,void *param)
{
	//グレイスケール描画の後でモノクロ描画を行う場合、セットが必要
	
	if(APPDRAW->imgbits == 8)
		*((uint8_t *)&APPDRAW->w.drawcol + 3) = 255;
	else
		*((uint16_t *)&APPDRAW->w.drawcol + 3) = 0x8000;

	TileImage_setPixel_work(APPDRAW->tileimg_tmp, x, y, &APPDRAW->w.drawcol);
}

/* (preview) グレイスケール */

static void _setpixel_prev_gray(int x,int y,int a,void *param)
{
	if(APPDRAW->imgbits == 8)
		*((uint8_t *)&APPDRAW->w.drawcol + 3) = a;
	else
		*((uint16_t *)&APPDRAW->w.drawcol + 3) = (a << 15) / 255;

	TileImage_setPixel_work(APPDRAW->tileimg_tmp, x, y, &APPDRAW->w.drawcol);
}

/* (通常描画) モノクロ */

static void _setpixel_draw_mono(int x,int y,void *param)
{
	(g_tileimage_dinfo.func_setpixel)(APPDRAW->w.dstimg, x, y, &APPDRAW->w.drawcol);
}

/* (通常描画) グレイスケール */

static void _setpixel_draw_gray(int x,int y,int a,void *param)
{
	if(APPDRAW->imgbits == 8)
		*((uint8_t *)&APPDRAW->w.drawcol + 3) = a;
	else
		*((uint16_t *)&APPDRAW->w.drawcol + 3) = (a << 15) / 255;

	(g_tileimage_dinfo.func_setpixel)(APPDRAW->w.dstimg, x, y, &APPDRAW->w.drawcol);
}


//=============================
// sub
//=============================


/* フォント削除 */

static void _font_free(AppDraw *p)
{
	DrawFont_free(p->text.font);
	p->text.font = NULL;
}

/* 押し位置上のテキストアイテムを取得
 *
 * ptimg: NULL 以外で、押しイメージ位置をセット */

static LayerTextItem *_get_textitem_press(AppDraw *p,mPoint *ptimg)
{
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	if(ptimg) *ptimg = pt;

	return LayerItem_getTextItem_atPoint(p->curlayer, pt.x, pt.y);
}

/* インデックスからテキストアイテム取得 */

static LayerTextItem *_get_textitem_at_index(AppDraw *p,int index)
{
	return (LayerTextItem *)mListGetItemAtIndex(&p->curlayer->list_text, index);
}

/* テキストダイアログの実行
 *
 * DrawTextData に情報をセットしておく。
 * 関数終了後、フォントは作成されたままになるため、使用後は削除すること。
 *
 * pttmp[0] : 描画先イメージ位置
 * rcdraw : プレビュー更新範囲
 *
 * return: FALSE でダイアログがキャンセル */

static mlkbool _run_dialog(AppDraw *p)
{
	mlkbool ret;

	//プレビュー用イメージ作成

	if(!drawOpSub_createTmpImage_curlayer_imgsize(p))
		return FALSE;

	//初期化

	mRectEmpty(&p->w.rcdraw);

	RGBcombo_to_bitcol(&p->w.drawcol, &p->col.drawcol, p->imgbits);

	drawText_createFont(p);

	//ダイアログ

	p->text.in_dialog = TRUE;
	p->is_canvview_update = FALSE;

	ret = TextDialog_run();

	p->text.in_dialog = FALSE;

	//----------

	//プレビュー用イメージ削除

	drawOpSub_freeTmpImage(p);

	//プレビューの描画範囲を元に戻す

	drawUpdateRect_canvas(p, &p->w.rcdraw);

	//ダイアログ中にキャンバスビューが更新された場合、
	//プレビューのイメージが残っているので、更新

	if(p->is_canvview_update)
		PanelCanvasView_update();

	return ret;
}


//=============================
// 通常レイヤ時
//=============================


/* 通常レイヤの描画処理 */

static void _drawtext_normal(AppDraw *p)
{
	mFontDrawInfo fdi;

	//描画準備

	drawOpSub_setDrawInfo(p, TOOL_TEXT, 0);

	fdi.setpix_mono = _setpixel_draw_mono;
	fdi.setpix_gray = _setpixel_draw_gray;

	//描画

	drawOpSub_beginDraw_single(p);

	DrawFont_drawText(p->text.font,
		p->w.pttmp[0].x, p->w.pttmp[0].y, p->imgdpi, &APPDRAW->text.dt, &fdi);

	drawOpSub_endDraw_single(p);
}

/* (押し) 通常レイヤ時
 *
 * テキストダイアログを表示して、直接描画する。 */

static void _press_normal_draw(AppDraw *p)
{
	mlkbool ret;

	//描画可能か

	if(drawOpSub_canDrawLayer_mes(p, 0))
		return;

	//描画位置

	drawOpSub_getImagePoint_int(p, &p->w.pttmp[0]);

	//ダイアログ

	ret = _run_dialog(p);

	//描画

	if(ret && mStrIsnotEmpty(&p->text.dt.str_text))
		_drawtext_normal(p);

	//フォント削除

	_font_free(p);
}


//=============================
// テキストの移動操作
//=============================
/*
	pttmp[0] : 開始時のオフセット位置
	pttmp[1] : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動 px 数 (キャンバス座標)
	rctmp[0] : 前回の範囲
	ptmp : LayerTextItem *
	rcdraw  : 更新範囲 (タイマー用)
*/


/* 移動 */

static void _movetext_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	mRect rc;

	if(drawCalc_moveImage_onMotion(p, p->tileimg_tmp, 0, &pt))
	{
		//オフセット位置移動

		TileImage_moveOffset_rel(p->tileimg_tmp, pt.x, pt.y);

		//更新範囲 (移動前+移動後)

		drawCalc_unionRect_relmove(&rc, &p->w.rctmp[0], pt.x, pt.y);

		//範囲移動

		mRectMove(&p->w.rctmp[0], pt.x, pt.y);

		//更新

		mRectUnion(&p->w.rcdraw, &rc);

		MainCanvasPage_setTimer_updateMove();
	}
}

/* 離し */

static mlkbool _movetext_release(AppDraw *p)
{
	LayerTextItem *text;
	mRect rc,rcup,rcsrc;

	text = (LayerTextItem *)p->w.ptmp;

	//タイマークリア

	MainCanvasPage_clearTimer_updateMove();

	//残っている更新範囲

	rcup = p->w.rcdraw;

	//undo

	Undo_addLayerTextMove(text);

	//位置の移動

	text->x += p->w.pttmp[1].x;
	text->y += p->w.pttmp[1].y;

	rcsrc = text->rcdraw;

	//

	drawOpSub_freeTmpImage(p);

	//新しい位置で描画

	drawText_drawLayerText(p, text, NULL, &rc);

	text->rcdraw = rc;

	//キャンバス更新 (タイマーの残り範囲+描画範囲)

	mRectUnion(&rcup, &rc);

	drawUpdateRect_canvas(p, &rcup);

	//キャンバスビュー更新 (元の範囲+移動先範囲)

	mRectUnion(&rcsrc, &rc);

	drawUpdateRect_canvasview(p, &rcsrc);

	return TRUE;
}

/* 位置移動の開始 */

static mlkbool _press_start_move(AppDraw *p,LayerTextItem *text)
{
	mRect rc,rc2;

	if(drawOpSub_canDrawLayer_mes(p, CANDRAWLAYER_F_ENABLE_TEXT))
		return FALSE;

	//移動用イメージ作成

	if(!drawOpSub_createTmpImage_curlayer_rect(p, &text->rcdraw))
		return FALSE;

	//現在のテキストを消去

	drawText_clearItemRect(p, p->curlayer, text, &rc);

	//移動用イメージ描画

	drawText_drawLayerText(p, text, p->tileimg_tmp, &rc2);

	//

	p->w.ptmp = text;
	p->w.rctmp[0] = rc2;

	//操作開始

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_TMPIMG_MOVE,
		_movetext_motion, _movetext_release, 0);

	drawOpSub_startMoveImage(p, p->tileimg_tmp);

	p->w.drag_cursor_type = APPCURSOR_MOVE;

	//更新

	mRectUnion(&rc, &rc2);

	drawUpdateRect_canvas(p, &rc);

	return TRUE;
}


//=============================
// テキストレイヤ時
//=============================


/* テキストレイヤの描画処理 (ダイアログ後) */

static void _drawtext_textlayer_dialog(AppDraw *p)
{
	mFontDrawInfo fdi;

	//描画準備

	drawOpSub_setDrawInfo_textlayer(p, FALSE);

	fdi.setpix_mono = _setpixel_draw_mono;
	fdi.setpix_gray = _setpixel_draw_gray;

	//描画

	DrawFont_drawText(p->text.font,
		p->w.pttmp[0].x, p->w.pttmp[0].y, p->imgdpi, &APPDRAW->text.dt, &fdi);

	//フォント削除

	_font_free(p);
}

/* 新規テキスト
 *
 * p->w.pttmp[0] に描画位置 */

static void _textlayer_new_text(AppDraw *p,mlkbool newtext)
{
	if(p->curlayer->list_text.num >= _TEXT_MAXNUM)
		return;

	if(drawOpSub_canDrawLayer_mes(p, CANDRAWLAYER_F_ENABLE_TEXT))
		return;

	//テキストを空に

	if(newtext)
		mStrEmpty(&p->text.dt.str_text);

	//

	if(!_run_dialog(p))
		_font_free(p);
	else
	{
		//カレントレイヤに描画
		
		_drawtext_textlayer_dialog(p);

		//範囲がない

		if(mRectIsEmpty(&g_tileimage_dinfo.rcdraw))
		{
			mMessageBoxOK(MLK_WINDOW(APPWIDGET->mainwin),
				MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_TEXT_NO_ADD));

			return;
		}

		//テキスト追加

		if(LayerItem_addText(p->curlayer, &p->text.dt, &p->w.pttmp[0], &g_tileimage_dinfo.rcdraw))
			Undo_addLayerTextNew(&g_tileimage_dinfo.tileimginfo);

		//キャンバス更新

		drawOpSub_update_rcdraw(p);
	}
}

/* テキスト編集 */

static void _textlayer_edit_text(AppDraw *p,LayerTextItem *pi)
{
	mRect rc;

	if(drawOpSub_canDrawLayer_mes(p, CANDRAWLAYER_F_ENABLE_TEXT))
		return;

	//指定テキストを消去

	drawText_clearItemRect(p, p->curlayer, pi, &rc);

	drawUpdateRect_canvas_canvasview(p, &rc);

	//情報セット

	LayerTextItem_getDrawData(pi, &p->text.dt);

	p->w.pttmp[0].x = pi->x;
	p->w.pttmp[0].y = pi->y;

	//ダイアログ

	if(!_run_dialog(p))
	{
		//キャンセル時、テキストを再描画

		_font_free(p);

		drawText_drawLayerText(p, pi, NULL, &rc);

		drawUpdateRect_canvas_canvasview(p, &rc);
	}
	else
	{
		//Undo

		Undo_addLayerTextEdit(pi);
	
		//カレントレイヤに描画
		
		_drawtext_textlayer_dialog(p);

		//テキスト置き換え

		LayerItem_replaceText(p->curlayer, pi,
			&p->text.dt, &p->w.pttmp[0], &g_tileimage_dinfo.rcdraw);

		//キャンバス更新

		drawOpSub_update_rcdraw(p);
	}
}

/* 選択の離し時 */

static mlkbool _textsel_release(AppDraw *p)
{
	//枠を消すため先にセット
	p->w.optype = DRAW_OPTYPE_NONE;

	drawUpdateRect_canvas(p, &p->w.rctmp[0]);

	return TRUE;
}

/* 押し時 */

static mlkbool _press_textlayer(AppDraw *p,mlkbool dblclk)
{
	LayerTextItem *pi;

	pi = _get_textitem_press(p, &p->w.pttmp[0]);

	//テキストがない場合、何もしない

	if(p->w.press_state & MLK_STATE_ALT)
	{
		//+Alt: テキスト編集

		if(pi) _textlayer_edit_text(p, pi);

		return FALSE;
	}
	else if(p->w.press_state & MLK_STATE_CTRL)
	{
		//+Ctrl: 位置移動

		if(!pi)
			return FALSE;
		else
			return _press_start_move(p, pi);
	}

	//----------
	//以下、テキストがない場合は新規

	if(!pi)
		//テキストがなければ新規
		_textlayer_new_text(p, TRUE);
	else
	{
		if(dblclk)
		{
			//ダブルクリック時は編集
			_textlayer_edit_text(p, pi);
		}
		else
		{
			//押している間、枠を表示

			p->w.rctmp[0] = pi->rcdraw;

			drawOpSub_setOpInfo(p, DRAW_OPTYPE_TEXTLAYER_SEL,
				NULL, _textsel_release, 0);

			drawUpdateRect_canvas(p, &pi->rcdraw);

			return TRUE;
		}
	}

	return FALSE;
}


//=============================
// メニュー
//=============================

static uint16_t g_menudat[] = {
	TRID_MENU_NEW, MMENU_ARRAY16_SEP,
	TRID_MENU_EDIT_THIS, TRID_MENU_DEL_THIS, MMENU_ARRAY16_SEP,
	TRID_MENU_COPY, TRID_MENU_PASTE, MMENU_ARRAY16_SEP,
	TRID_MENU_EDIT_SUB, TRID_MENU_DEL_SUB, MMENU_ARRAY16_SEP,
	TRID_MENU_REDRAW,
	MMENU_ARRAY16_END
};


/* メニューの実行 */

static int _run_text_menu(AppDraw *p,LayerTextItem *text)
{
	mMenu *menu,*subedit,*subdel;
	mMenuItem *mi;
	LayerTextItem *pi;
	mStr str = MSTR_INIT;
	int i;
	char m[16];

	MLK_TRGROUP(TRGROUP_TEXTTOOL_MENU);

	menu = mMenuNew();

	mMenuAppendTrText_array16(menu, g_menudat);

	if(!text)
	{
		mMenuSetItemEnable(menu, TRID_MENU_EDIT_THIS, FALSE);
		mMenuSetItemEnable(menu, TRID_MENU_DEL_THIS, FALSE);
		mMenuSetItemEnable(menu, TRID_MENU_COPY, FALSE);
	}

	mMenuSetItemEnable(menu, TRID_MENU_PASTE, p->text.fhave_copy);

	//サブメニュー

	subedit = mMenuNew();
	subdel = mMenuNew();
	i = 0;

	MLK_LIST_FOR(p->curlayer->list_text, pi, LayerTextItem)
	{
		LayerTextItem_getText_limit(pi, &str, 10);

		snprintf(m, 16, "%d: ", i + 1);
		mStrPrependText(&str, m);

		mMenuAppendText_copy(subedit, 1000 + i, str.buf, str.len);
		mMenuAppendText_copy(subdel, 2000 + i, str.buf, str.len);

		i++;
	}

	mStrFree(&str);

	mMenuSetItemSubmenu(menu, TRID_MENU_EDIT_SUB, subedit);
	mMenuSetItemSubmenu(menu, TRID_MENU_DEL_SUB, subdel);

	//

	mi = mMenuPopup(menu, MLK_WIDGET(APPWIDGET->canvaspage),
		p->w.pt_canv_last.x, p->w.pt_canv_last.y, NULL,
		MPOPUP_F_LEFT | MPOPUP_F_TOP | MPOPUP_F_FLIP_XY, NULL);

	i = (mi)? mMenuItemGetID(mi): -1;

	mMenuDestroy(menu);

	return i;
}

/* 削除 */

static void _text_delete(AppDraw *p,LayerTextItem *text)
{
	mRect rc;

	//Undo

	Undo_addLayerTextDelete(text);

	//消去

	drawText_clearItemRect(p, p->curlayer, text, &rc);

	//削除

	LayerItem_deleteText(p->curlayer, text);

	//更新

	drawUpdateRect_canvas_canvasview(p, &rc);
}

/* 再描画
 *
 * 現状の更新となるため、アンドゥは行わない。 */

static void _redraw_text(AppDraw *p)
{
	LayerTextItem *pi;
	mRect rc,rc2;

	drawCursor_wait();

	//タイルを解放してクリア
	// [!] アンドゥの整合性を保つため、タイル配列は変更しない。
	
	TileImage_freeAllTiles(p->curlayer->img);

	//再描画

	mRectEmpty(&rc);

	MLK_LIST_FOR(p->curlayer->list_text, pi, LayerTextItem)
	{
		//元の範囲を更新用に追加

		mRectUnion(&rc, &pi->rcdraw);

		//描画
		
		drawText_drawLayerText(p, pi, NULL, &rc2);

		pi->rcdraw = rc2;

		mRectUnion(&rc, &rc2);
	}

	//更新

	drawUpdateRect_canvas_canvasview(p, &rc);

	drawCursor_restore();
}

/* メニュー処理 */

static void _proc_menu(AppDraw *p)
{
	LayerTextItem *text;
	int id;

	text = _get_textitem_press(p, &p->w.pttmp[0]);

	id = _run_text_menu(p, text);
	if(id < 0) return;

	//------

	if(id >= 2000)
	{
		//削除

		text = _get_textitem_at_index(p, id - 2000);
		if(text)
			_text_delete(p, text);
	}
	else if(id >= 1000)
	{
		//編集

		text = _get_textitem_at_index(p, id - 1000);
		if(text)
			_textlayer_edit_text(p, text);
	}
	else
	{
		switch(id)
		{
			//新規テキスト
			case TRID_MENU_NEW:
				_textlayer_new_text(p, TRUE);
				break;
			//このテキストを編集
			case TRID_MENU_EDIT_THIS:
				_textlayer_edit_text(p, text);
				break;
			//このテキストを削除
			case TRID_MENU_DEL_THIS:
				_text_delete(p, text);
				break;
			//コピー
			case TRID_MENU_COPY:
				LayerTextItem_getDrawData(text, &p->text.dt_copy);
				p->text.fhave_copy = TRUE;
				break;
			//貼り付け
			case TRID_MENU_PASTE:
				DrawTextData_copy(&p->text.dt, &p->text.dt_copy);
				
				_textlayer_new_text(p, FALSE);
				break;
			//再描画
			case TRID_MENU_REDRAW:
				_redraw_text(p);
				break;
		}
	}
}


//=============================
// 共通
//=============================


/** 通常ボタン押し */

mlkbool drawOp_drawtext_press(AppDraw *p)
{
	if(LAYERITEM_IS_TEXT(p->curlayer))
		return _press_textlayer(p, FALSE);
	else
	{
		_press_normal_draw(p);
		return FALSE;
	}
}

/** 右ボタン押し */

void drawOp_drawtext_press_rbtt(AppDraw *p)
{
	if(LAYERITEM_IS_TEXT(p->curlayer))
		_proc_menu(p);
}

/** ダブルクリック時 */

void drawOp_drawtext_dblclk(AppDraw *p)
{
	if(LAYERITEM_IS_TEXT(p->curlayer))
		_press_textlayer(p, TRUE);
}


//=============================
// テキストダイアログ時
//=============================


/** DrawTextData からフォントを作成 */

void drawText_createFont(AppDraw *p)
{
	DrawFont_free(p->text.font);

	p->text.font = DrawFont_create(&p->text.dt, p->imgdpi);
}

/** フォントサイズ変更時 */

void drawText_changeFontSize(AppDraw *p)
{
	DrawFont_setSize(p->text.font, &p->text.dt, p->imgdpi);
}

/** 各情報の変更時 */

void drawText_changeInfo(AppDraw *p)
{
	DrawFont_setInfo(p->text.font, &p->text.dt);
}

/** ダイアログ中にキャンバス上が左クリックされた時 */

void drawText_setPoint_inDialog(AppDraw *p,double x,double y)
{
	//位置

	drawCalc_canvas_to_image_pt(p, &p->w.pttmp[0], x, y);

	//プレビュー

	drawText_drawPreview(p);
}

/** プレビュー描画 */

void drawText_drawPreview(AppDraw *p)
{
	DrawTextData *dt = &p->text.dt;
	mRect rc;
	mFontDrawInfo fdi;

	rc = p->w.rcdraw;

	//前回の範囲がある場合、イメージと範囲をクリア

	if(!mRectIsEmpty(&rc))
	{
		TileImage_freeAllTiles(p->tileimg_tmp);

		mRectEmpty(&p->w.rcdraw);
	}

	//プレビューなし or 空文字列の場合、前回の範囲を更新して終了

	if(!p->text.fpreview || mStrIsEmpty(&dt->str_text))
	{
		drawUpdateRect_canvas(p, &rc);
		return;
	}

	//描画

	fdi.setpix_mono = _setpixel_prev_mono;
	fdi.setpix_gray = _setpixel_prev_gray;

	drawOpSub_setDrawInfo_pixelcol(TILEIMAGE_PIXELCOL_NORMAL);

	TileImageDrawInfo_clearDrawRect();

	DrawFont_drawText(APPDRAW->text.font,
		p->w.pttmp[0].x, p->w.pttmp[0].y, p->imgdpi, dt, &fdi);

	//更新 (前回の範囲と結合)

	mRectUnion(&rc, &g_tileimage_dinfo.rcdraw);

	drawUpdateRect_canvas(p, &rc);

	//範囲を記録

	p->w.rcdraw = g_tileimage_dinfo.rcdraw;
}


//=============================
// ほか
//=============================


/** テキストアイテムの範囲内を消去
 *
 * 消去後、重なる他のテキストを再描画
 *
 * rcupdate: 消去範囲 + 再描画範囲 */

void drawText_clearItemRect(AppDraw *p,LayerItem *layer,LayerTextItem *item,mRect *rcupdate)
{
	LayerTextItem *pi;
	mRect rc,rc2;

	//消去範囲, 再描画対象

	LayerItem_getTextRedrawInfo(layer, &rc, &item->rcdraw);

	//消去

	drawOpSub_setDrawInfo_textlayer(p, TRUE);

	TileImage_drawFillBox(layer->img, rc.x1, rc.y1,
		rc.x2 - rc.x1 + 1, rc.y2 - rc.y1 + 1,
		&p->w.drawcol);

	TileImage_freeEmptyTiles(layer->img);

	//重なるテキストの再描画

	MLK_LIST_FOR(layer->list_text, pi, LayerTextItem)
	{
		if(pi->tmp && pi != item)
		{
			drawText_drawLayerText(p, pi, NULL, &rc2);

			//すでに描画してある状態と、再描画時のフォントが異なる場合があるため、
			//範囲は再セットする。

			pi->rcdraw = rc2;

			mRectUnion(&rc, &rc2);
		}
	}

	*rcupdate = rc;
}

/** LayerTextItem からテキストを描画
 *
 * imgdraw: 描画先。NULL でカレントレイヤのイメージ。
 * rcdraw: 描画範囲が入る */

void drawText_drawLayerText(AppDraw *p,LayerTextItem *pi,TileImage *imgdraw,mRect *rcdraw)
{
	mFontDrawInfo fdi;
	DrawFont *font;
	DrawTextData dt;

	//描画準備

	drawOpSub_setDrawInfo_textlayer(p, FALSE);

	if(imgdraw) p->w.dstimg = imgdraw;

	fdi.setpix_mono = _setpixel_draw_mono;
	fdi.setpix_gray = _setpixel_draw_gray;

	mMemset0(&dt, sizeof(DrawTextData));

	LayerTextItem_getDrawData(pi, &dt);

	//描画

	font = DrawFont_create(&dt, p->imgdpi);

	DrawFont_drawText(font, pi->x, pi->y, p->imgdpi, &dt, &fdi);

	DrawFont_free(font);

	DrawTextData_free(&dt);

	//

	*rcdraw = g_tileimage_dinfo.rcdraw;
}


