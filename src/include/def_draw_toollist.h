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
 * AppDraw: ツールリスト用データの定義
 *****************************************/

#include "curve_spline.h"

typedef struct _AppDrawToolList AppDrawToolList;
typedef struct _ToolListItem ToolListItem;
typedef struct _BrushEditData BrushEditData;
typedef struct _BrushDrawParam BrushDrawParam;

#define DRAW_TOOLLIST_REG_NUM  8		//ツールリスト登録アイテム数


struct _AppDrawToolList
{
	mList list_group;	//グループリスト
	mBuf buf_sizelist;	//サイズリストデータ

	ToolListItem *selitem,	//選択アイテム
		*last_selitem,		//選択変更時、前回の選択アイテム
		*regitem[DRAW_TOOLLIST_REG_NUM],	//登録アイテム
		*copyitem;			//コピーしたアイテム [*]

	BrushEditData *brush;		//ブラシ編集用の現在の値 [*:メモリ確保]
	BrushDrawParam *dp_cur,		//現在のブラシの描画用パラメータ [*]
		*dp_reg;				//登録アイテムの描画用パラメータ [*]

	CurveSpline curve_spline;	//筆圧カーブ計算時の作業用
	uint32_t pt_press_comm[CURVE_SPLINE_MAXNUM],	//筆圧カーブ共通設定
		pt_press_reg[CURVE_SPLINE_MAXNUM],		//登録アイテムの筆圧カーブ (前回の値)
		pt_press_copy[CURVE_SPLINE_MAXNUM];		//コピーされた筆圧カーブ
};


