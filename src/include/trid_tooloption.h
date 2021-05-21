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
 * ツールオプションの文字列ID
 *****************************************/

enum
{
	TRID_TOOLOPT_SLIM,	//細線
	TRID_TOOLOPT_SHAPE,	//形状
	TRID_TOOLOPT_STRONG,	//強さ
	TRID_TOOLOPT_FILL_TYPE,	//塗りつぶす範囲
	TRID_TOOLOPT_FILL_TYPE_LIST,
	TRID_TOOLOPT_FILL_DIFF,	//許容誤差
	TRID_TOOLOPT_FILL_LAYER,	//色を参照するレイヤ
	TRID_TOOLOPT_FILL_LAYER_LIST,
	TRID_TOOLOPT_GRAD_COLTYPE_LIST,
	TRID_TOOLOPT_GRAD_REVERSE,
	TRID_TOOLOPT_GRAD_LOOP,
	TRID_TOOLOPT_SEL_HIDE,
	TRID_TOOLOPT_LOAD,
	TRID_TOOLOPT_CLEAR,
	TRID_TOOLOPT_TRANSFORM,
	TRID_TOOLOPT_STAMP_TRANS_LIST,
	TRID_TOOLOPT_OVERWRITE_PASTE,
	TRID_TOOLOPT_PASTE_ENABLE_MASK,
	TRID_TOOLOPT_BOXEDIT_LIST,
	TRID_TOOLOPT_RUN,

	//グラデーションメニュー
	TRID_TOOLOPT_GRAD_MENU_EDIT = 100,
	TRID_TOOLOPT_GRAD_MENU_NEW,
	TRID_TOOLOPT_GRAD_MENU_EDIT_LIST,
	TRID_TOOLOPT_GRAD_MENU_LOAD,
	TRID_TOOLOPT_GRAD_MENU_SAVE
};

