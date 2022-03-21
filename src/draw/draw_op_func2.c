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
 * 操作 - いろいろ2
 *****************************************/
/*
 * 選択範囲/切り貼り/矩形編集
 */

#include <stdlib.h>	//abs
#include <math.h>

#include "mlk_gui.h"
#include "mlk_rectbox.h"

#include "def_draw.h"
#include "def_tool_option.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_op_def.h"
#include "draw_op_sub.h"
#include "draw_op_func.h"

#include "tileimage.h"
#include "tileimage_drawinfo.h"
#include "layeritem.h"
#include "layerlist.h"

#include "fillpolygon.h"
#include "drawfill.h"

#include "appcursor.h"
#include "maincanvas.h"
#include "statusbar.h"



//==================================
// 選択範囲 : 図形選択
//==================================
/*
  - 選択範囲イメージは tileimg_sel (1bit)
*/


/* 四角形塗りつぶし(回転あり): 多角形の点セット
 *
 * return: FALSE で失敗 */

static mlkbool _set_drawbox_polygon(AppDraw *p)
{
	mDoublePoint pt[4];
	int i;

	drawOpSub_getDrawRectPoints(p, pt);

	p->w.fillpolygon = FillPolygon_new();
	if(!p->w.fillpolygon) return FALSE;
	
	for(i = 0; i < 4; i++)
		FillPolygon_addPoint(p->w.fillpolygon, pt[i].x, pt[i].y);

	return FillPolygon_closePoint(p->w.fillpolygon);
}

/** 選択範囲を図形からセット
 *
 * type: [0] 四角形 [1] 多角形またはフリーハンド */

void drawOp_setSelect(AppDraw *p,int type)
{
	mBox box;
	mlkbool del,fdraw;

	if(!drawSel_selImage_create(p)) return;

	drawCursor_wait();

	drawOpSub_setDrawInfo_select();

	//+Ctrl = 範囲削除

	del = ((p->w.press_state & MLK_STATE_CTRL) != 0);

	if(del) p->w.drawcol = 0;

	//描画

	TileImageDrawInfo_clearDrawRect();

	if(type == 0 && p->canvas_angle == 0)
	{
		//------ 四角形 & キャンバス回転なしの場合
		
		drawOpSub_getDrawRectBox_angle0(p, &box);

		TileImage_drawFillBox(p->tileimg_sel, box.x, box.y, box.w, box.h, &p->w.drawcol);
	}
	else
	{
		//----- 多角形

		fdraw = TRUE;

		//四角形 (回転あり) の場合

		if(type == 0)
			fdraw = _set_drawbox_polygon(p);

		//描画 (非アンチエイリアス)

		if(fdraw)
		{
			TileImage_drawFillPolygon(p->tileimg_sel, p->w.fillpolygon,
				&p->w.drawcol, FALSE);
		}

		drawOpSub_freeFillPolygon(p);
	}

	//-----

	if(del)
		//範囲削除時
		drawSel_selImage_freeEmpty(p);
	else
		//範囲追加
		mRectUnion(&p->sel.rcsel, &g_tileimage_dinfo.rcdraw);

	//描画部分を更新

	drawUpdateRect_canvaswg_forSelect(p, &g_tileimage_dinfo.rcdraw);

	drawCursor_restore();
}


//=================================
// 自動選択
//=================================


/* 離し */

static mlkbool _selfill_release(AppDraw *p)
{
	DrawFill *draw;
	LayerItem *item;
	mPoint pt;
	int type,diff,layer;
	uint32_t val;

	//設定値

	if(p->w.is_toollist_toolopt)
		val = p->w.toollist_toolopt;
	else
		val = p->tool.opt_selfill;

	type = TOOLOPT_FILL_GET_TYPE(val);
	diff = TOOLOPT_FILL_GET_DIFF(val);
	layer = TOOLOPT_FILL_GET_LAYER(val);

	//参照レイヤのリンクセット 
	// :カレントが対象になった場合、フォルダであれば無効

	item = LayerList_setLink_filltool(p->layerlist, p->curlayer, layer);

	if(!item) return TRUE;

	//選択範囲イメージ作成

	if(!drawSel_selImage_create(p)) return TRUE;
	
	//塗りつぶし初期化

	drawOpSub_getImagePoint_int(p, &pt);

	draw = DrawFill_new(p->tileimg_sel, item->img, &pt, type, diff, 100);
	if(!draw) return TRUE;

	//準備

	drawCursor_wait();

	drawOpSub_setDrawInfo_select();

	TileImageDrawInfo_clearDrawRect();

	//塗りつぶし実行

	DrawFill_run(draw, &p->w.drawcol);
	DrawFill_free(draw);

	//範囲を追加

	mRectUnion(&p->sel.rcsel, &g_tileimage_dinfo.rcdraw);

	//更新

	drawUpdateRect_canvaswg_forSelect(p, &g_tileimage_dinfo.rcdraw);

	//

	drawCursor_restore();

	return TRUE;
}

/** 押し */

mlkbool drawOp_selectFill_press(AppDraw *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, _selfill_release, 0);

	return TRUE;
}


//==================================
// 選択範囲 : 範囲の位置移動
//==================================
/*
	pttmp[0] : 開始時のオフセット位置
	pttmp[1] : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動 px 数 (キャンバス座標)
	rcdraw   : 更新範囲 (タイマー用)
*/


/* 移動 */

static void _selmove_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	mRect rc;

	//pt = オフセットの相対移動px数

	if(drawCalc_moveImage_onMotion(p, p->tileimg_sel, state, &pt))
	{
		//オフセット位置移動

		TileImage_moveOffset_rel(p->tileimg_sel, pt.x, pt.y);

		//更新範囲 (移動前+移動後)

		drawCalc_unionRect_relmove(&rc, &p->sel.rcsel, pt.x, pt.y);

		//rcsel 移動

		mRectMove(&p->sel.rcsel, pt.x, pt.y);

		//更新

		mRectUnion(&p->w.rcdraw, &rc);

		MainCanvasPage_setTimer_updateSelectMove();
	}
}

/* 離し */

static mlkbool _selmove_release(AppDraw *p)
{
	//タイマークリア&更新

	MainCanvasPage_clearTimer_updateSelectMove();

	return TRUE;
}

/** 押し時 */

mlkbool drawOp_selectMove_press(AppDraw *p)
{
	if(!drawSel_isHave()) return FALSE;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_selmove_motion, _selmove_release, 0);

	drawOpSub_startMoveImage(p, p->tileimg_sel);

	p->w.drag_cursor_type = APPCURSOR_SEL_MOVE;

	return TRUE;
}


//=========================================
// 選択範囲 : 範囲内イメージの移動/コピー
//=========================================
/*
	pttmp[0] : 開始時のオフセット位置
	pttmp[1] : オフセット位置の総相対移動数
	ptd_tmp[0] : 総相対移動 px 数 (キャンバス座標)
	rcdraw   : 更新範囲 (タイマー用)

	ドラッグ中は、レイヤ合成時に、tileimg_tmp がカレントの上に挟み込まれて表示される。
*/


/* 移動 */

static void _selimgmove_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	mRect rc;

	if(drawCalc_moveImage_onMotion(p, p->tileimg_sel, 0, &pt))
	{
		//オフセット位置移動
		// :コピーイメージと選択範囲を同時に移動する。

		TileImage_moveOffset_rel(p->tileimg_sel, pt.x, pt.y);
		TileImage_moveOffset_rel(p->tileimg_tmp, pt.x, pt.y);
		
		//更新範囲 (移動前+移動後)

		drawCalc_unionRect_relmove(&rc, &p->sel.rcsel, pt.x, pt.y);

		//rcsel 移動

		mRectMove(&p->sel.rcsel, pt.x, pt.y);

		//更新

		mRectUnion(&p->w.rcdraw, &rc);

		MainCanvasPage_setTimer_updateSelectImageMove();
	}
}

/* 離し */

static mlkbool _selimgmove_release(AppDraw *p)
{
	mlkbool update,is_hide;

	//枠非表示戻す

	is_hide = p->sel.is_hide;

	p->sel.is_hide = FALSE;

	//タイマークリア & 更新

	update = MainCanvasPage_clearTimer_updateSelectImageMove();

	//ドラッグ中に枠非表示で、タイマークリア時に更新されていない場合、
	//枠が非表示の状態のため、枠を更新。
	// :最終的な更新時にも更新は行われるが、
	// :選択範囲枠と実際のイメージの更新範囲は一致しない場合がある。

	if(is_hide && !update)
		drawUpdateRect_canvaswg_forSelect(p, &p->sel.rcsel);

	//貼り付け (重ね塗り)

	drawOpSub_setDrawInfo_select_paste();

	TileImage_pasteSelImage(p->w.dstimg, p->tileimg_tmp, &p->sel.rcsel);

	//作業用イメージ解放
	// :以下の更新時、tileimg_tmp == NULL にすると通常更新となるので、
	// :更新を行う前に解放すること。

	drawOpSub_freeTmpImage(p);

	//更新
	// :g_tileimg_dinfo.rcdraw の範囲で更新。
	// :切り取り時は、[切り取られた範囲+貼り付け範囲] になっている。
	// :ドラッグ中は tileimg_tmp を挟む形で更新されているが、
	// :最終的な貼り付け後のイメージで再度更新する必要あり。

	drawOpSub_finishDraw_single(p);

	return TRUE;
}

/** 押し時 */

mlkbool drawOp_selimgmove_press(AppDraw *p,mlkbool cut)
{
	if(!drawSel_isHave()
		|| drawOpSub_canDrawLayer_mes(p, 0)
		|| !drawOpSub_isPress_inSelect(p))
		return FALSE;

	//描画準備
	// :この時点では、切り取り用。

	drawOpSub_setDrawInfo(p, -1, 0);
	drawOpSub_setDrawInfo_clearMasks();

	g_tileimage_dinfo.func_pixelcol = TileImage_global_getPixelColorFunc(TILEIMAGE_PIXELCOL_OVERWRITE);

	drawOpSub_beginDraw(p);

	//範囲内イメージ作成 (切り取りも実行)

	p->tileimg_tmp = TileImage_createSelCopyImage(p->curlayer->img,
		p->tileimg_sel, &p->sel.rcsel, cut);

	if(!p->tileimg_tmp) return FALSE;

	//

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_SELIMGMOVE,
		_selimgmove_motion, _selimgmove_release, 0);

	drawOpSub_startMoveImage(p, p->tileimg_sel);

	p->w.drag_cursor_type = APPCURSOR_SEL_MOVE;

	//+Ctrl で枠非表示

	if(TOOLOPT_SELECT_IS_HIDE(p->tool.opt_select)
		|| (p->w.press_state & MLK_STATE_CTRL))
	{
		p->sel.is_hide = TRUE;
	}

	return TRUE;
}


//===========================================
// 貼り付けイメージの移動
//===========================================
/*
  (イメージ移動の作業値と共通)
  rctmp[0] : 前回の範囲
  rcdraw   : 更新用 (タイマーで更新されたらクリアされる)
*/


/* 移動 */

static void _pastemove_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	mRect rc;

	if(drawCalc_moveImage_onMotion(p, p->boxsel.img, state, &pt))
	{
		//範囲を相対移動
		
		TileImage_moveOffset_rel(p->boxsel.img, pt.x, pt.y);

		p->boxsel.box.x += pt.x;
		p->boxsel.box.y += pt.y;

		//更新範囲 (移動前+移動後)

		drawCalc_unionRect_relmove(&rc, &p->w.rctmp[0], pt.x, pt.y);

		//現在の範囲

		mRectMove(&p->w.rctmp[0], pt.x, pt.y);

		//更新

		mRectUnion(&p->w.rcdraw, &rc);

		MainCanvasPage_setTimer_updatePasteMove();
	}
}

/* 離し */

static mlkbool _pastemove_release(AppDraw *p)
{
	MainCanvasPage_clearTimer_updatePasteMove();

	return TRUE;
}

/* 押し */

static mlkbool _pastemove_press(AppDraw *p)
{
	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_pastemove_motion, _pastemove_release, 0);

	drawOpSub_startMoveImage(p, p->boxsel.img);

	mRectSetBox(&p->w.rctmp[0], &p->boxsel.box);

	p->w.drag_cursor_type = APPCURSOR_SEL_MOVE;

	return TRUE;
}


//=============================
// 切り貼りツール
//=============================


/* 貼り付け確定 */

static mlkbool _cutpaste_draw_paste(AppDraw *p)
{
	//描画不可

	if(drawOpSub_canDrawLayer_mes(p, CANDRAWLAYER_F_NO_PASTE))
	{
		drawBoxSel_cancelPaste(p);
		return FALSE;
	}

	//

	drawOpSub_setDrawInfo(p, TOOL_CUTPASTE, 0);

	if(TOOLOPT_CUTPASTE_IS_OVERWRITE(p->tool.opt_cutpaste))
		drawOpSub_setDrawInfo_pixelcol(TILEIMAGE_PIXELCOL_OVERWRITE);

	if(!TOOLOPT_CUTPASTE_IS_ENABLE_MASK(p->tool.opt_cutpaste))
		drawOpSub_setDrawInfo_clearMasks();

	//貼り付け

	drawOpSub_beginDraw(p);

	TileImage_pasteBoxSelImage(p->w.dstimg, p->boxsel.img, &p->boxsel.box);

	drawOpSub_finishDraw_single(p);

	//貼り付けモード解除
	// :貼り付け後の更新範囲と、クリアする範囲は一致しない場合があるため、
	// :更新は常に実行。

	drawBoxSel_cancelPaste(p);

	//離されるまでグラブ

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL, NULL, NULL, 0);

	return TRUE;
}

/** 押し時 */

mlkbool drawOp_cutpaste_press(AppDraw *p)
{
	mPoint pt;

	drawOpSub_getImagePoint_int(p, &pt);

	//貼り付けモード時

	if(p->boxsel.is_paste_mode)
	{
		if(!mBoxIsPointIn(&p->boxsel.box, pt.x, pt.y))
			return _cutpaste_draw_paste(p);
		else
			return _pastemove_press(p);
	}

	//選択内が押された時、コピー/切り取りして貼り付け

	if(p->boxsel.box.w
		&& mBoxIsPointIn(&p->boxsel.box, pt.x, pt.y))
	{
		pt.x = p->boxsel.box.x;
		pt.y = p->boxsel.box.y;
	
		if(drawBoxSel_copy_cut(p, FALSE)
			&& drawBoxSel_paste(p, &pt))
			return _pastemove_press(p);
		else
			return FALSE;
	}

	//範囲選択 or リサイズ

	return drawOp_boxsel_press(p);
}


//=============================
// 矩形選択 : 範囲リサイズ
//=============================
/*
	pttmp[0] : 左上位置
	pttmp[1] : 右下位置
	pttmp[2] : 前回のイメージ位置
*/


enum
{
	_BOXSEL_EDGE_LEFT   = 1<<0,
	_BOXSEL_EDGE_TOP    = 1<<1,
	_BOXSEL_EDGE_RIGHT  = 1<<2,
	_BOXSEL_EDGE_BOTTOM = 1<<3
};


/** 非操作時の移動時: カーソル変更 */

void drawOp_boxsel_nograb_motion(AppDraw *p)
{
	mPoint pt;
	mRect rc,rcsel;
	int w,pos,no;

	//イメージ位置

	drawOpSub_getImagePoint_int(p, &pt);

	//rcsel = 判定用の選択範囲
	// :枠は外側に3pxとして表示されているので、それに合わせる

	mRectSetBox(&rcsel, &p->boxsel.box);

	rcsel.x1--;
	rcsel.y1--;
	rcsel.x2++;
	rcsel.y2++;

	//rc = カーソルと各辺との距離

	rc.x1 = abs(rcsel.x1 - pt.x);
	rc.y1 = abs(rcsel.y1 - pt.y);
	rc.x2 = abs(rcsel.x2 - pt.x);
	rc.y2 = abs(rcsel.y2 - pt.y);

	//判定の半径幅 (イメージの px 単位)

	if(p->canvas_zoom >= 5000)
		w = 1;
	else if(p->canvas_zoom >= 2000)
		w = 2;
	else
		//縮小時はより大きい幅で
		w = (int)(p->viewparam.scalediv * 5 + 0.5);

	//カーソルに近い辺と四隅を判定

	if(rc.x1 < w && rc.y1 < w)
		pos = _BOXSEL_EDGE_LEFT | _BOXSEL_EDGE_TOP; //左上
	else if(rc.x1 < w && rc.y2 < w)
		pos = _BOXSEL_EDGE_LEFT | _BOXSEL_EDGE_BOTTOM; //左下
	else if(rc.x2 < w && rc.y1 < w)
		pos = _BOXSEL_EDGE_RIGHT | _BOXSEL_EDGE_TOP; //右上
	else if(rc.x2 < w && rc.y2 < w)
		pos = _BOXSEL_EDGE_RIGHT | _BOXSEL_EDGE_BOTTOM; //右下
	else if(rc.x1 < w && rcsel.y1 <= pt.y && pt.y <= rcsel.y2)
		pos = _BOXSEL_EDGE_LEFT;	//左
	else if(rc.x2 < w && rcsel.y1 <= pt.y && pt.y <= rcsel.y2)
		pos = _BOXSEL_EDGE_RIGHT;	//右
	else if(rc.y1 < w && rcsel.x1 <= pt.x && pt.x <= rcsel.x2)
		pos = _BOXSEL_EDGE_TOP;	//上
	else if(rc.y2 < w && rcsel.x1 <= pt.x && pt.x <= rcsel.x2)
		pos = _BOXSEL_EDGE_BOTTOM;	//下
	else
		pos = 0;

	//カーソル変更

	if(pos != p->boxsel.flag_resize_cursor)
	{
		p->boxsel.flag_resize_cursor = pos;

		if(pos == 0)
			//なし
			no = APPCURSOR_SELECT;
		else if(pos == (_BOXSEL_EDGE_LEFT | _BOXSEL_EDGE_TOP)
			|| pos == (_BOXSEL_EDGE_RIGHT | _BOXSEL_EDGE_BOTTOM))
		{
			//左上/右下
			no = APPCURSOR_LEFT_TOP;
		}
		else if(pos == (_BOXSEL_EDGE_LEFT | _BOXSEL_EDGE_BOTTOM)
			|| pos == (_BOXSEL_EDGE_RIGHT | _BOXSEL_EDGE_TOP))
		{
			//右上/左下
			no = APPCURSOR_RIGHT_TOP;
		}
		else if(pos == _BOXSEL_EDGE_LEFT || pos == _BOXSEL_EDGE_RIGHT)
			//水平
			no = APPCURSOR_RESIZE_HORZ;
		else
			//垂直
			no = APPCURSOR_RESIZE_VERT;

		MainCanvasPage_setCursor(no);
	}
}

/* (リサイズ中) 移動 */

static void _boxsel_resize_motion(AppDraw *p,uint32_t state)
{
	mPoint pt;
	int flags,n,mx,my;

	//移動数

	drawOpSub_getImagePoint_int(p, &pt);

	mx = pt.x - p->w.pttmp[2].x;
	my = pt.y - p->w.pttmp[2].y;

	if(mx == 0 && my == 0) return;

	//移動

	drawOpXor_drawRect_image(p);

	flags = p->boxsel.flag_resize_cursor;

	if(flags & _BOXSEL_EDGE_LEFT)
	{
		//左

		n = p->w.pttmp[0].x + mx;

		if(n < 0) n = 0;
		else if(n > p->w.pttmp[1].x) n = p->w.pttmp[1].x;

		p->w.pttmp[0].x = n;
	}
	else if(flags & _BOXSEL_EDGE_RIGHT)
	{
		//右

		n = p->w.pttmp[1].x + mx;

		if(n < p->w.pttmp[0].x) n = p->w.pttmp[0].x;
		else if(n >= p->imgw) n = p->imgw - 1;

		p->w.pttmp[1].x = n;
	}

	if(flags & _BOXSEL_EDGE_TOP)
	{
		//上

		n = p->w.pttmp[0].y + my;

		if(n < 0) n = 0;
		else if(n > p->w.pttmp[1].y) n = p->w.pttmp[1].y;

		p->w.pttmp[0].y = n;
	}
	else if(flags & _BOXSEL_EDGE_BOTTOM)
	{
		//下

		n = p->w.pttmp[1].y + my;

		if(n < p->w.pttmp[0].y) n = p->w.pttmp[0].y;
		else if(n >= p->imgh) n = p->imgh - 1;

		p->w.pttmp[1].y = n;
	}

	drawOpXor_drawRect_image(p);

	//

	p->w.pttmp[2] = pt;

	//status

	StatusBar_setHelp_selbox(FALSE);
}

/* (リサイズ中) 離し */

static mlkbool _boxsel_resize_release(AppDraw *p)
{
	mBox *boxdst = &p->boxsel.box;

	drawOpXor_drawRect_image(p);

	//範囲セット

	boxdst->x = p->w.pttmp[0].x;
	boxdst->y = p->w.pttmp[0].y;
	boxdst->w = p->w.pttmp[1].x - p->w.pttmp[0].x + 1;
	boxdst->h = p->w.pttmp[1].y - p->w.pttmp[0].y + 1;

	//更新

	drawUpdateBox_canvaswg_forBoxSel(p, boxdst, TRUE);

	//離された位置で、リサイズカーソル再判定

	drawOp_boxsel_nograb_motion(p);

	//status

	StatusBar_setHelp_selbox(TRUE);

	return TRUE;
}

/* 矩形範囲リサイズ: 押し時 */

static mlkbool _boxsel_resize_press(AppDraw *p)
{
	mBox box;

	drawOpSub_setOpInfo(p, DRAW_OPTYPE_GENERAL,
		_boxsel_resize_motion, _boxsel_resize_release, 0);

	//

	box = p->boxsel.box;

	p->w.pttmp[0].x = box.x;
	p->w.pttmp[0].y = box.y;
	p->w.pttmp[1].x = box.x + box.w - 1;
	p->w.pttmp[1].y = box.y + box.h - 1;

	drawOpSub_getImagePoint_int(p, &p->w.pttmp[2]);

	//枠を消して、XOR 描画

	p->boxsel.box.w = 0;

	drawUpdateBox_canvaswg_forBoxSel(p, &box, TRUE);
	
	drawOpXor_drawRect_image(p);

	//status

	StatusBar_setHelp_selbox(FALSE);

	return TRUE;
}


//=========================================
// 矩形範囲の選択
//=========================================


/** 押し */

mlkbool drawOp_boxsel_press(AppDraw *p)
{
	if(p->boxsel.flag_resize_cursor)
		//リサイズ開始
		return _boxsel_resize_press(p);
	else
	{
		//矩形選択開始
		// [!] XOR を行う前はキャンバスを直接更新する
		
		drawBoxSel_release(p, TRUE);

		return drawOpXor_rect_image_press(p, DRAW_OPSUB_SET_BOXSEL);
	}
}

