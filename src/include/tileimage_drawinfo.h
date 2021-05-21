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

/*****************************************************
 * TileImageDrawInfo
 * (TileImage へ描画する際の情報)
 *****************************************************/

typedef struct _BrushDrawParam BrushDrawParam;
typedef struct _ImageMaterial ImageMaterial;


typedef struct _TileImageDrawInfo
{
	mlkerr err;	//エラー

	RGBA8 *pmaskcol;	//色マスクの色
	int maskcol_num;	//色マスクの数

	uint8_t maskcol_type,	//色マスクタイプ (0:なし, 1:MASK, 2:REV)
		alphamask_type;		//アルファマスクタイプ

	mRect rcdraw;			//描画された範囲

	TileImageInfo tileimginfo;	//アンドゥ用:描画開始時のイメージ情報

	TileImage *img_save,	//描画前の保存用
		*img_brush_stroke,	//ブラシストローク濃度用
		*img_mask,			//マスクイメージ (NULL でなし)
		*img_sel;			//選択範囲
	ImageMaterial *texture;		//テクスチャイメージ (NULL でなし)

	BrushDrawParam *brushdp;	//ブラシ描画パラメータ
	uint64_t drawcol;			//ブラシ描画用の描画色

	void (*func_setpixel)(TileImage *,int,int,void *);	//ピクセル描画関数
	void (*func_pixelcol)(TileImage *,void *,void *,void *); //色処理関数
}TileImageDrawInfo;


extern TileImageDrawInfo g_tileimage_dinfo;

void TileImageDrawInfo_clearDrawRect(void);

