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

/********************************************
 * MainCanvas/MainCanvasPage 定義
 ********************************************/

/* MainCanvas */

typedef struct _MainCanvas
{
	MLK_SCROLLVIEW_DEF

	mPoint pt_scroll_base;  //スクロールバーの基準位置
}MainCanvas;

/* MainCanvasPage */

typedef struct _MainCanvasPage
{
	MLK_SCROLLVIEWPAGE_DEF

	mTooltip *ttip_layername;	//レイヤ名表示用
	mCursor cursor_drag_restore;	//ドラッグ中カーソル戻す用
	mBox box_update;			//タイマーでの更新範囲 (x = -1 でなし)
	int pressed_rawkey;			//現在押されているキー (キャンバスキー判定用)。-1 でなし
	int pressed_modifiers;      // 現在おされている修飾キー
	uint8_t is_pressed_space,	//スペースキーが押されているか
		is_have_drag_cursor;	//ドラッグ中のカーソルがセットされているか
}MainCanvasPage;

