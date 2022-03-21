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

/**********************************
 * AppDraw 操作関連の定義
 **********************************/

#ifndef AZPT_DRAW_OP_DEF_H
#define AZPT_DRAW_OP_DEF_H

/* AppDraw::w.optype (現在の操作タイプ) */

enum
{
	DRAW_OPTYPE_NONE,
	DRAW_OPTYPE_GENERAL,	//関数のみで完結するため、タイプ指定の必要なし
	DRAW_OPTYPE_DRAW_FREE,	//自由線描画
	DRAW_OPTYPE_SELIMGMOVE,	//選択範囲イメージのコピー/移動
	DRAW_OPTYPE_TMPIMG_MOVE,	//tileimg_tmp のイメージ移動処理
	DRAW_OPTYPE_TEXTLAYER_SEL,	//テキストレイヤのテキスト選択中

	DRAW_OPTYPE_XOR_LINE,		//直線
	DRAW_OPTYPE_XOR_RECT_CANV,	//矩形 (キャンバスに対する)
	DRAW_OPTYPE_XOR_RECT_IMAGE,	//矩形 (イメージに対する)
	DRAW_OPTYPE_XOR_ELLIPSE,	//楕円
	DRAW_OPTYPE_XOR_SUMLINE,	//連続直線/集中線
	DRAW_OPTYPE_XOR_BEZIER,		//ベジェ曲線
	DRAW_OPTYPE_XOR_POLYGON,	//多角形
	DRAW_OPTYPE_XOR_LASSO		//多角形フリーハンド
};

/* AppDraw::w.optype_sub (操作タイプのサブ情報) */

enum
{
	DRAW_OPSUB_DRAW_LINE,		//直線描画
	DRAW_OPSUB_DRAW_FRAME,		//枠描画
	DRAW_OPSUB_DRAW_FILL,		//塗りつぶし描画
	DRAW_OPSUB_DRAW_GRADATION,	//グラデーション描画
	DRAW_OPSUB_DRAW_SUCCLINE,	//連続直線
	DRAW_OPSUB_DRAW_CONCLINE,	//集中線
	DRAW_OPSUB_SET_STAMP,		//スタンプイメージのセット
	DRAW_OPSUB_SET_SELECT,		//選択範囲のセット
	DRAW_OPSUB_SET_BOXSEL,		//矩形範囲をセット

	DRAW_OPSUB_TO_BEZIER,		//直線からベジェ曲線へ切り替え
	DRAW_OPSUB_RULE_SETTING		//定規設定
};

/* func_action() 時のアクション */

enum
{
	DRAW_FUNC_ACTION_RBTT_UP,		//右ボタン離し
	DRAW_FUNC_ACTION_LBTT_DBLCLK,	//左ボタンダブルクリック
	DRAW_FUNC_ACTION_KEY_ENTER,		//ENTER キー押し
	DRAW_FUNC_ACTION_KEY_ESC,		//ESC キー押し
	DRAW_FUNC_ACTION_KEY_BACKSPACE,	//BS キー押し
	DRAW_FUNC_ACTION_UNGRAB			//グラブ解除
};

/* AppDraw::w.opflags (操作中のオプションフラグ) */

enum
{
	DRAW_OPFLAGS_MOTION_DISABLE_STATE = 1<<0,	//ポインタ移動時、装飾キーを無効にする
	DRAW_OPFLAGS_MOTION_POS_INT = 1<<1,			//ポインタ移動時、位置は整数値で判断し、前回と同じ位置なら処理しない
	//DRAW_OPFLAGS_BRUSH_PRESSURE_MAX = 1<<2		//ブラシ描画時、常に筆圧最大
};

/* AppDraw::w.drawinfo_flags */

enum
{
	DRAW_DRAWINFO_F_BRUSH_STROKE = 1<<0  //ブラシ描画でストローク重ね塗りを行う
};

#endif
