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

/****************************
 * ダイアログ
 ****************************/

typedef struct _NewCanvasValue NewCanvasValue;
typedef struct _LayerNewOptInfo LayerNewOptInfo;

typedef struct
{
	int w,h,dpi,method;
}CanvasScaleInfo;

mlkbool NewCanvasDialog_run(mWindow *parent,NewCanvasValue *dst);
mlkbool GridOptDialog_run(mWindow *parent);

mlkbool CanvasDialog_resize(mWindow *parent,mSize *dst_size,int *dst_option);
mlkbool CanvasDialog_scale(mWindow *parent,CanvasScaleInfo *info);

/* イメージ設定 */

enum
{
	IMAGEOPTDLG_F_BKGND_COL = 1<<0,	//背景色変更
	IMAGEOPTDLG_F_BITS = 1<<1		//イメージビット変更
};

int ImageOptionDlg_run(mWindow *parent);

/* レイヤ */

enum
{
	LAYER_NEWOPTDLG_F_NAME = 1<<0,
	LAYER_NEWOPTDLG_F_TEXPATH = 1<<1,
	LAYER_NEWOPTDLG_F_BLENDMODE = 1<<2,
	LAYER_NEWOPTDLG_F_OPACITY = 1<<3,
	LAYER_NEWOPTDLG_F_COLOR = 1<<4,
	LAYER_NEWOPTDLG_F_TONE = 1<<5,
	LAYER_NEWOPTDLG_F_TONE_PARAM = 1<<6,

	LAYER_NEWOPTDLG_F_UPDATE_CANVAS = LAYER_NEWOPTDLG_F_TEXPATH | LAYER_NEWOPTDLG_F_BLENDMODE
		| LAYER_NEWOPTDLG_F_OPACITY | LAYER_NEWOPTDLG_F_COLOR | LAYER_NEWOPTDLG_F_TONE
		| LAYER_NEWOPTDLG_F_TONE_PARAM
};

int LayerDialog_newopt(mWindow *parent,LayerNewOptInfo *info,mlkbool fnew);
mlkbool LayerDialog_color(mWindow *parent,uint32_t *pcol);

uint32_t LayerDialog_batchToneLines(mWindow *parent);
int LayerDialog_layerType(mWindow *parent,int curtype,mlkbool is_text);
int LayerDialog_combineMulti(mWindow *parent,mlkbool cur_folder,mlkbool have_checked);

