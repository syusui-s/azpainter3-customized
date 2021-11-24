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

/*****************************************
 * AppDraw
 * 操作 - メイン
 *****************************************/

#include "mlk_gui.h"
#include "mlk_event.h"
#include "mlk_key.h"
#include "mlk_rectbox.h"

#include "def_draw.h"
#include "def_draw_toollist.h"
#include "def_config.h"
#include "def_canvaskey.h"

#include "draw_main.h"
#include "draw_calc.h"
#include "draw_layer.h"
#include "draw_op_def.h"
#include "draw_op_func.h"
#include "draw_op_sub.h"
#include "draw_rule.h"

#include "toollist.h"
#include "appcursor.h"

#include "mainwindow.h"
#include "maincanvas.h"
#include "statusbar.h"
#include <stdio.h>



//============================
// 押し時
//============================


/* (押し時) ブラシ/ドット線 自由線描画 */

static mlkbool _on_press_brushdot_free(AppDraw *p,int toolno)
{
	mlkbool is_brush;

	printf("_on_press_brushdot_free: p->w.brush_regno = %d\n", p->w.brush_regno);

	//ブラシか

	is_brush = (toolno == TOOL_TOOLLIST);

	//-------- 装飾キー

	if(p->w.press_state & MLK_STATE_ALT)
	{
		//+Alt: スポイト (キャンバス色)

		return drawOp_spoit_press(p, 0, FALSE);
	}
	else if(p->rule.type && (p->rule.setting_mode || (p->w.press_state & MLK_STATE_CTRL)))
	{
		//[定規ON時] +Ctrl or 設定モード時: 定規設定

		return drawRule_onPress_setting(p);
	}
	else if(is_brush && p->w.press_state == MLK_STATE_SHIFT)
	{
		//[ブラシ時] +Shift: ブラシサイズ変更

		return drawOp_dragBrushSize_press(p);
	}

	//--------- 自由線描画

	//描画不可

	if(drawOpSub_canDrawLayer_mes(p, 0)) return FALSE;

	//定規セット

	drawRule_onPress(p, !is_brush);

	//開始

	if(is_brush)
		//ブラシ
		return drawOp_brush_free_press(p);
	else
		//ドットペン/指先
		return drawOp_dotpen_free_press(p);
}

/* (押し時) ブラシ/ドットペン/指先 */

static mlkbool _on_press_brush_dot(AppDraw *p,int toolno,int subno)
{
	//自由線以外時、描画可能かチェック

	if(subno != TOOLSUB_DRAW_FREE && drawOpSub_canDrawLayer_mes(p, 0))
		return FALSE;

	//

	mRectEmpty(&p->w.rcdraw);

	switch(subno)
	{
		//自由線
		case TOOLSUB_DRAW_FREE:
			return _on_press_brushdot_free(p, toolno);

		//直線
		case TOOLSUB_DRAW_LINE:
			return drawOpXor_line_press(p, DRAW_OPSUB_DRAW_LINE);

		//四角形
		case TOOLSUB_DRAW_BOX:
			return drawOpXor_rect_canv_press(p, DRAW_OPSUB_DRAW_FRAME);

		//楕円
		case TOOLSUB_DRAW_ELLIPSE:
			return drawOpXor_ellipse_press(p, DRAW_OPSUB_DRAW_FRAME);

		//連続直線
		case TOOLSUB_DRAW_SUCCLINE:
			return drawOpXor_sumline_press(p, DRAW_OPSUB_DRAW_SUCCLINE);

		//集中線
		case TOOLSUB_DRAW_CONCLINE:
			return drawOpXor_sumline_press(p, DRAW_OPSUB_DRAW_CONCLINE);

		//ベジェ曲線
		case TOOLSUB_DRAW_BEZIER:
			return drawOpXor_line_press(p, DRAW_OPSUB_TO_BEZIER);
	}

	return FALSE;
}

/* (押し時) 図形塗り/消し */

static mlkbool _on_press_fillpoly(AppDraw *p,int subno)
{
	if(drawOpSub_canDrawLayer_mes(p, 0)) return FALSE;

	switch(subno)
	{
		//四角形
		case 0:
			return drawOpXor_rect_canv_press(p, DRAW_OPSUB_DRAW_FILL);
		//楕円
		case 1:
			return drawOpXor_ellipse_press(p, DRAW_OPSUB_DRAW_FILL);
		//多角形
		case 2:
			return drawOpXor_polygon_press(p, DRAW_OPSUB_DRAW_FILL);
		//フリーハンド
		default:
			return drawOpXor_lasso_press(p, DRAW_OPSUB_DRAW_FILL);
	}

	return FALSE;
}

/* (押し時) 選択範囲 */

static mlkbool _on_press_select(AppDraw *p,int subno)
{
	switch(subno)
	{
		//四角
		case TOOLSUB_SEL_BOX:
			return drawOpXor_rect_canv_press(p, DRAW_OPSUB_SET_SELECT);
		//多角形
		case TOOLSUB_SEL_POLYGON:
			return drawOpXor_polygon_press(p, DRAW_OPSUB_SET_SELECT);
		//フリーハンド
		case TOOLSUB_SEL_LASSO:
			return drawOpXor_lasso_press(p, DRAW_OPSUB_SET_SELECT);
		//イメージ移動/コピー
		case TOOLSUB_SEL_IMGMOVE:
		case TOOLSUB_SEL_IMGCOPY:
			return drawOp_selimgmove_press(p, (subno == TOOLSUB_SEL_IMGMOVE));
		//範囲位置移動
		case TOOLSUB_SEL_POSMOVE:
			return drawOp_selectMove_press(p);
	}

	return FALSE;
}

/* (押し時) ツールの各処理 (ツールリスト以外)
 *
 * is_toollist_toolopt:
 *   FALSE で現在のツールの設定値を使う。
 *   TRUE で toollist_toolopt の値を使う (ツールリストのアイテム時)。
 * return: グラブするか */

static mlkbool _on_press_tool_main(AppDraw *p,int toolno,int subno,mlkbool is_toollist_toolopt)
{
	p->w.optoolno = toolno;
	p->w.optool_subno = subno;

	p->w.is_toollist_toolopt = is_toollist_toolopt;

	mRectEmpty(&p->w.rcdraw);

	//レイヤ名表示

	if(toolno <= TOOL_TEXT &&
		(APPCONF->fview & CONFIG_VIEW_F_CANV_LAYER_NAME))
		MainCanvasPage_showLayerName();

	//

	switch(toolno)
	{
		//ドットペン/消しゴム/指先
		case TOOL_DOTPEN:
		case TOOL_DOTPEN_ERASE:
		case TOOL_FINGER:
			return _on_press_brush_dot(p, toolno, subno);
	
		//図形塗り/消し
		case TOOL_FILL_POLYGON:
		case TOOL_FILL_POLYGON_ERASE:
			return _on_press_fillpoly(p, subno);

		//塗りつぶし
		case TOOL_FILL:
		case TOOL_FILL_ERASE:
			return drawOp_fill_press(p);

		//グラデーション
		case TOOL_GRADATION:
			if(drawOpSub_canDrawLayer_mes(p, 0))
				return FALSE;
			else
				return drawOpXor_line_press(p, DRAW_OPSUB_DRAW_GRADATION);

		//移動
		case TOOL_MOVE:
			return drawOp_movetool_press(p, subno);

		//テキスト
		case TOOL_TEXT:
			return drawOp_drawtext_press(p);

		//自動選択
		case TOOL_SELECT_FILL:
			return drawOp_selectFill_press(p);

		//選択範囲
		case TOOL_SELECT:
			return _on_press_select(p, subno);

		//切り貼り
		case TOOL_CUTPASTE:
			return drawOp_cutpaste_press(p);

		//矩形編集
		case TOOL_BOXEDIT:
			return drawOp_boxsel_press(p);
	
		//スタンプ
		case TOOL_STAMP:
			return drawOp_stamp_press(p, subno);
	
		//キャンバス回転
		case TOOL_CANVAS_ROTATE:
			return drawOp_canvasRotate_press(p);
		//キャンバス移動
		case TOOL_CANVAS_MOVE:
			return drawOp_canvasMove_press(p);
		//スポイト
		case TOOL_SPOIT:
			return drawOp_spoit_press(p, subno, TRUE);
	}

	return FALSE;
}

/* (押し時) ツールリスト/登録アイテムの処理
 *
 * regno: 登録アイテム番号 (-1 で選択アイテム)
 * subno: -1 でツールリストのサブタイプ */

static mlkbool _on_press_toollist(AppDraw *p,int regno,int subno)
{
	ToolListItem *item;
	ToolListItem_tool *pitool;

	//アイテム

	if(regno < 0)
		item = p->tlist->selitem;
	else
		item = p->tlist->regitem[regno];

	if(!item) return FALSE;

	//ツールリストのサブタイプ

	if(subno < 0)
		subno = p->tool.subno[TOOL_TOOLLIST];

	//

	if(item->type == TOOLLIST_ITEMTYPE_TOOL)
	{
		//--- ツール

		pitool = (ToolListItem_tool *)item;

		//サブタイプ

		if(pitool->subno != 255)
			//指定値
			subno = pitool->subno;
		else if(regno >= 0 && drawTool_isType_haveDrawType(pitool->toolno))
			//登録ツールで、描画タイプがある場合、自由線で固定
			subno = TOOLSUB_DRAW_FREE;

		//

		p->w.toollist_toolopt = pitool->toolopt;

		p->w.drag_cursor_type = drawCursor_getToolCursor(pitool->toolno);
		
		printf("_on_press_toollist TOOL_MAIN : regno = %d\n", regno);

		return _on_press_tool_main(p, pitool->toolno, subno, TRUE);
	}
	else
	{
		//--- ブラシ
		
		//レイヤ名表示

		if(APPCONF->fview & CONFIG_VIEW_F_CANV_LAYER_NAME)
			MainCanvasPage_showLayerName();

		//

		p->w.optoolno = TOOL_TOOLLIST;
		p->w.brush_regno = regno;

		if(regno >= 0)
		{
			p->w.drag_cursor_type = APPCURSOR_DRAW;

			//登録ツールの場合、常に自由線
			subno = TOOLSUB_DRAW_FREE;
		}
		printf("_on_press_toollist BRUSH_DOT : regno = %d\n", regno);

		return _on_press_brush_dot(p, TOOL_TOOLLIST, subno);
	}
}

/* (押し時) ツールから判定
 *
 * subno: 負の値で、現在のサブタイプ
 * return: グラブするか */

static mlkbool _on_press_tool(AppDraw *p,int toolno,int subno)
{
	printf("_on_press_tool\n");

	if(subno < 0)
		subno = p->tool.subno[toolno];

	//

	if(toolno != TOOL_TOOLLIST)
		return _on_press_tool_main(p, toolno, subno, FALSE);
	else
	{
		//ツールリスト
		// :テキストレイヤの場合は、テキストツールとして扱う

		if(drawOpSub_isCurLayer_text())
			return _on_press_tool_main(p, TOOL_TEXT, 0, FALSE);
	
		printf("_on_press_tool: w.brash_regno = %d\n", p->w.brush_regno);
		return _on_press_toollist(p, -1, subno);
	}
}


//==============================
// キー/ボタン設定など
//==============================


/* キャンバスキーのキーを押しながら操作時のコマンド取得
 *
 * return: -1 でなし */

static int _get_canvaskey_cmd(void)
{
	int key,mod,cmd;

	//キャンバス上で現在押されているキー

	key = MainCanvasPage_getPressedRawKey();
	printf("key: %d\n", key);
	printf("mod: %d\n", MainCanvasPage_getPressedModifiers());
	if(key == -1) return -1;

	//現在のキーにコマンドが設定されているか

	cmd = APPCONF->canvaskey[key];
	if(cmd == 0) return -1;

	//<キー+操作> のコマンドかどうか
	// :key に、キー押し時に実行されるタイプのコマンドが設定されている場合、
	// :キー押し時に処理をした後、キーが押されている間に左ボタンが押されると、
	// :2回コマンドが実行されてしまうので、+キーのコマンドでなければ弾く。

	if(CANVASKEY_IS_OPERATE(cmd))
		return cmd;
	else
		return -1;
}

/* (押し時) 各ボタンに設定されたコマンドを処理
 *
 * is_pentab: 筆圧情報があるデバイスか
 * return: [0] 何もしない [1] グラブする [-1] 通常動作を行う。 */

static int _on_press_command(AppDraw *p,int btt,int is_pentab_eraser,int is_pentab)
{
	int cmd,ret,can_draw;

	printf("- _on_press_command -----\n");

	//====== コマンドID 取得 (CANVASKEY_*)

	if(btt > MLK_BTT_LEFT)
	{
		//[左ボタン以外]
		// :設定されたコマンド。指定なしなら、何もしない。
		
		if(btt > CONFIG_POINTERBTT_MAXNO) return 0;

		if(is_pentab)
			cmd = APPCONF->pointer_btt_pentab[btt];
		else
			cmd = APPCONF->pointer_btt_default[btt];

		if(!cmd) return 0;
	}
	else if(is_pentab_eraser && APPCONF->pointer_btt_pentab[0])
	{
		//[左ボタン & ペンタブの消しゴム側]
		// :コマンドの指定がなければ、普通の左ボタンとして扱う。
	
		cmd = APPCONF->pointer_btt_pentab[0];
	}
	else
	{
		//[左ボタン]

		//キャンバスキーのキー＋操作のコマンドがある場合、優先

		cmd = _get_canvaskey_cmd();
		printf("_on_press_command LEFT_CLICK: cmd = %d\n", cmd);
		
		if(cmd == -1)
		{
			//キャンバスキー操作がなかった場合、左ボタンに設定されている動作。
			//設定がなければ、通常の左ボタン動作へ。

			if(is_pentab)
				cmd = APPCONF->pointer_btt_pentab[1];
			else
				cmd = APPCONF->pointer_btt_default[1];

			if(!cmd) return -1;
		}
	}

	//======= 各コマンド処理

	//グラブ中のカーソルは、操作処理を行う前に、デフォルト扱いとしてセットする。
	//各操作処理でカーソルが変更される場合は、値が上書きされる。

	//ret : 戻り値はデフォルトで何もしない。

	ret = 0;

	//描画が可能な状態か
	// :切り貼り/矩形編集ツール時は描画不可とする。

	can_draw = (p->tool.no != TOOL_CUTPASTE && p->tool.no != TOOL_BOXEDIT);

	//

	if(cmd >= CANVASKEY_OP_TOOL && cmd < CANVASKEY_OP_TOOL + TOOL_NUM)
	{
		//ツール動作
		// :描画不可時は、キャンバス操作のみ使用可。

		cmd -= CANVASKEY_OP_TOOL;

		if(can_draw || drawTool_isType_notDraw(cmd))
		{
			p->w.drag_cursor_type = drawCursor_getToolCursor(cmd);

			ret = _on_press_tool(p, cmd, -1);
		}
	}
	else if(cmd >= CANVASKEY_OP_DRAWTYPE && cmd < CANVASKEY_OP_DRAWTYPE + TOOLSUB_DRAW_NUM)
	{
		//描画タイプ動作

		if(drawTool_isType_haveDrawType(p->tool.no))
		{
			ret = _on_press_tool(p, p->tool.no, cmd - CANVASKEY_OP_DRAWTYPE);
		}
	}
	else if(cmd >= CANVASKEY_OP_SELECT && cmd < CANVASKEY_OP_SELECT + TOOLSUB_SEL_NUM)
	{
		//選択範囲ツール動作

		p->w.drag_cursor_type = APPCURSOR_SELECT;

		ret = _on_press_select(p, cmd - CANVASKEY_OP_SELECT);
	}
	else if(cmd >= CANVASKEY_OP_REGIST && cmd < CANVASKEY_OP_REGIST + DRAW_TOOLLIST_REG_NUM)
	{
		//登録ツール動作

		ret = _on_press_toollist(p, cmd - CANVASKEY_OP_REGIST, -1);
	}
	else if(cmd >= CANVASKEY_OP_OTHER && cmd < CANVASKEY_OP_OTHER + CANVASKEY_OP_OTHER_NUM)
	{
		//他動作

		cmd -= CANVASKEY_OP_OTHER;

		switch(cmd)
		{
			//表示倍率変更 (上下ドラッグ)
			case CANVASKEY_OP_OTHER_CANVAS_ZOOM:
				ret = drawOp_canvasZoom_press(p);
				break;
			//ブラシサイズ変更
			case CANVASKEY_OP_OTHER_BRUSHSIZE:
				printf("CANVASKEY_OP_OTHER_BRUSHSIZE: brush = %d\n", cmd - CANVASKEY_OP_REGIST);
				ret = drawOp_dragBrushSize_press(p);
				break;
			//掴んだレイヤを選択
			case CANVASKEY_OP_OTHER_GET_LAYER:
				drawOpSub_getImagePoint_int(p, p->w.pttmp);
				drawLayer_selectGrabLayer(p, p->w.pttmp[0].x, p->w.pttmp[0].y);
				break;
		}
	}
	else if(cmd >= CANVASKEY_CMD_OTHER && cmd < CANVASKEY_CMD_OTHER + CANVASKEY_CMD_OTHER_NUM)
	{
		//他コマンド (左ボタン以外のボタン操作時)

		MainWindow_runCanvasKeyCmd(cmd);
	}

	return ret;
}


//============================
// メイン処理
//============================


/** ボタン押し時
 *
 * return: グラブするか */

mlkbool drawOp_onPress(AppDraw *p,mEvent *ev)
{
	int ret;

	if(p->w.optype != DRAW_OPTYPE_NONE)
	{
		//======= 操作中

		if(ev->pentab.btt == MLK_BTT_LEFT && p->w.func_press_in_grab)
		{
			//左ボタン押し
			
			p->w.dpt_canv_cur.x = ev->pentab.x;
			p->w.dpt_canv_cur.y = ev->pentab.y;

			(p->w.func_press_in_grab)(p, ev->pentab.state);
		}

		return FALSE;
	}
	else
	{
		//======= 操作開始

		//押し時の位置
		p->w.dpt_canv_press.x = ev->pentab.x;
		p->w.dpt_canv_press.y = ev->pentab.y;
		p->w.dpt_canv_press.pressure = ev->pentab.pressure;

		p->w.dpt_canv_cur = p->w.dpt_canv_last = p->w.dpt_canv_press;

		//押し時の位置 (int)
		p->w.pt_canv_last.x = (int)ev->pentab.x;
		p->w.pt_canv_last.y = (int)ev->pentab.y;

		//押し時のボタンと状態
		p->w.press_btt = ev->pentab.btt;
		p->w.press_state = ev->pentab.state & MLK_STATE_MASK_MODS;

		//ほか
		p->w.opflags = 0;
		p->w.release_btt = ev->pentab.btt;
		p->w.drag_cursor_type = -1;

		//---- 左ボタン+SPACE 操作

		if(ev->pentab.btt == MLK_BTT_LEFT
			&& MainCanvasPage_isPressed_space())
		{
			if(p->w.press_state == MLK_STATE_CTRL)
			{
				//+Ctrl: キャンバス回転

				p->w.opflags |= DRAW_OPFLAGS_MOTION_DISABLE_STATE;
				
				return drawOp_canvasRotate_press(p);
			}
			else if(p->w.press_state == MLK_STATE_SHIFT)
				//+Shift: キャンバス表示倍率
				return drawOp_canvasZoom_press(p);
			else
				//キャンバス移動
				return drawOp_canvasMove_press(p);
		}

		//---- テキストツール:右ボタン

		if(ev->pentab.btt == MLK_BTT_RIGHT
			&& drawOpSub_isRunTextTool())
		{
			drawOp_drawtext_press_rbtt(p);
			return FALSE;
		}

		//----

		ret = _on_press_command(p, ev->pentab.btt,
			ev->pentab.flags & MEVENT_PENTABLET_F_ERASER,
			ev->pentab.flags & MEVENT_PENTABLET_F_HAVE_PRESSURE);

		if(ret != -1)
			return ret;
		else
		{
			//ツールの左ボタンの通常動作

			return _on_press_tool(p, p->tool.no, -1);
		}
	}

	return FALSE;
}

/** ボタン離し時
 *
 * return: グラブ解除するか */

mlkbool drawOp_onRelease(AppDraw *p,mEvent *ev)
{
	mlkbool ungrab = TRUE;

	if(p->w.optype == DRAW_OPTYPE_NONE) return FALSE;

	//現在の位置

	p->w.dpt_canv_cur.x = ev->pentab.x;
	p->w.dpt_canv_cur.y = ev->pentab.y;
	p->w.dpt_canv_cur.pressure = ev->pentab.pressure;

	//右ボタン押し時の処理

	if(ev->pentab.btt == MLK_BTT_RIGHT
		&& p->w.func_action
		&& (p->w.func_action)(p, DRAW_FUNC_ACTION_RBTT_UP))
		goto END;

	//指定ボタン以外なら処理しない

	if(ev->pentab.btt != p->w.release_btt)
		return FALSE;

	//処理

	if(p->w.func_release)
		ungrab = (p->w.func_release)(p);

	//操作終了

END:
	if(ungrab)
		p->w.optype = DRAW_OPTYPE_NONE;

	return ungrab;
}

/** カーソル移動時 */

void drawOp_onMotion(AppDraw *p,mEvent *ev)
{
	mPoint pt;
	mlkbool flag;

	//pt = イメージ位置 (int)

	drawCalc_canvas_to_image_pt(p, &pt, ev->pentab.x, ev->pentab.y);

	//カーソル位置

	StatusBar_setCursorPos(pt.x, pt.y);

	//

	if(p->w.optype == DRAW_OPTYPE_NONE)
	{
		//------- 非操作中の処理

		if(p->boxsel.box.w && !p->boxsel.is_paste_mode)
		{
			//矩形範囲がある時

			p->w.dpt_canv_cur.x = ev->pentab.x;
			p->w.dpt_canv_cur.y = ev->pentab.y;

			drawOp_boxsel_nograb_motion(p);
		}
	}
	else
	{
		//-------- 操作中の処理

		//現在位置
	
		p->w.dpt_canv_cur.x = ev->pentab.x;
		p->w.dpt_canv_cur.y = ev->pentab.y;
		p->w.dpt_canv_cur.pressure = ev->pentab.pressure;

		//

		pt.x = (int)ev->pentab.x;
		pt.y = (int)ev->pentab.y;

		//位置を int で扱う場合、前回と同じ位置なら処理しない

		flag = ((p->w.opflags & DRAW_OPFLAGS_MOTION_POS_INT)
			&& pt.x == p->w.pt_canv_last.x && pt.y == p->w.pt_canv_last.y);

		//処理

		if(p->w.func_motion && !flag)
		{
			(p->w.func_motion)(p,
				(p->w.opflags & DRAW_OPFLAGS_MOTION_DISABLE_STATE)? 0: ev->pentab.state);
		}

		//位置保存

		p->w.dpt_canv_last = p->w.dpt_canv_cur;
		p->w.pt_canv_last = pt;
	}
}

/** 左ボタンのダブルクリック時
 *
 * return: TRUE でダブルクリックとして処理し、グラブ解除。
 *         FALSE で通常の押しとして処理する (default)。*/

mlkbool drawOp_onLBttDblClk(AppDraw *p,mEvent *ev)
{
	mlkbool release = FALSE;

	if(p->w.optype)
	{
		//操作中
		
		if(p->w.func_action)
			release = (p->w.func_action)(p, DRAW_FUNC_ACTION_LBTT_DBLCLK);
	}
	else
	{
		//非操作中: テキストツール時

		if(drawOpSub_isRunTextTool())
		{
			drawOp_drawtext_dblclk(p);
			release = TRUE;
		}
	}

	//解除

	if(release)
		p->w.optype = DRAW_OPTYPE_NONE;

	return release;
}

/** 操作中のキー押し時
 *
 * return: TRUE でグラブ解除 */

mlkbool drawOp_onKeyDown(AppDraw *p,uint32_t key)
{
	mlkbool release = FALSE;
	int action;

	//現在の操作処理

	if(p->w.func_action)
	{
		action = -1;
	
		switch(key)
		{
			case MKEY_ENTER:
			case MKEY_KP_ENTER:
				action = DRAW_FUNC_ACTION_KEY_ENTER;
				break;
			case MKEY_ESCAPE:
				action = DRAW_FUNC_ACTION_KEY_ESC;
				break;
			case MKEY_BACKSPACE:
				action = DRAW_FUNC_ACTION_KEY_BACKSPACE;
				break;
		}

		if(action != -1)
			release = (p->w.func_action)(p, action);
	}

	//解除

	if(release)
		p->w.optype = DRAW_OPTYPE_NONE;

	return release;
}

/** 想定外のグラブ解除時 */

void drawOp_onUngrab(AppDraw *p)
{
	if(p->w.optype)
	{
		if(p->w.func_action)
			(p->w.func_action)(p, DRAW_FUNC_ACTION_UNGRAB);
		else if(p->w.func_release)
			(p->w.func_release)(p);

		p->w.optype = DRAW_OPTYPE_NONE;
	}
}

