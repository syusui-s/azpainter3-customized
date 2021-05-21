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

/************************************
 * フィルタ描画情報
 ************************************/

#include "colorvalue.h"

typedef struct _mPopupProgress mPopupProgress;
typedef struct _mRandSFMT mRandSFMT;
typedef struct _FilterDrawInfo FilterDrawInfo;
typedef struct _TileImage TileImage;

typedef mlkbool (*FilterDrawFunc)(FilterDrawInfo *);


#define FILTER_BAR_NUM       8	//最大8=漫画用集中線など
#define FILTER_CHECKBTT_NUM  3	//最大3=ハーフトーン
#define FILTER_COMBOBOX_NUM  3	//最大3=縁に沿って点描画


struct _FilterDrawInfo
{
	TileImage *imgsrc,	//ソース画像
		*imgdst,		//出力先画像
		*imgref,		//参照イメージの先頭
		*imgsel;		//選択範囲 (カラー処理のプレビュー用)
	mRandSFMT *rand;
	mPopupProgress *prog;

	mRect rc;	//イメージを処理する範囲 (イメージ座標)
	mBox box;	//rc の box 版

	FilterDrawFunc func_draw;

	RGBcombo rgb_drawcol,	//描画色
		rgb_bkgnd;		//背景色

	int imgx,imgy,		//イメージ位置 (キャンバス上のクリック位置)
		val_bar[FILTER_BAR_NUM];	//バーの値

	uint8_t	val_ckbtt[FILTER_CHECKBTT_NUM],	//チェックボタンの値
		val_combo[FILTER_COMBOBOX_NUM];		//選択の値

	uint8_t bits,	//イメージビット数
		in_dialog,	//ダイアログ中は TRUE、実際の描画時は FALSE
		clipping;	//イメージ範囲外をクリッピングするか

	/* フィルタ処理中の作業用 */

	int ntmp[4];	//最大4=RGBずらし
	void *ptmp[1];
};

