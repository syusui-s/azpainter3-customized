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

/***************************************
 * メニュー定義データ
 ***************************************/

const uint16_t g_mainmenudat[] = {
/* ファイル */
TRMENU_TOP_FILE,
MMENUBAR_ARRAY16_SUB_START,
	TRMENU_FILE_NEW, TRMENU_FILE_OPEN, MMENUBAR_ARRAY16_SEP,
	TRMENU_FILE_SAVE, TRMENU_FILE_SAVE_AS, TRMENU_FILE_SAVE_DUP, MMENUBAR_ARRAY16_SEP,
	TRMENU_FILE_RECENTFILE, MMENUBAR_ARRAY16_SEP,
	TRMENU_FILE_EXIT,
MMENUBAR_ARRAY16_SUB_END,

/* 編集 */
TRMENU_TOP_EDIT,
MMENUBAR_ARRAY16_SUB_START,
	TRMENU_EDIT_UNDO, TRMENU_EDIT_REDO, MMENUBAR_ARRAY16_SEP,
	TRMENU_EDIT_FILL, TRMENU_EDIT_ERASE, MMENUBAR_ARRAY16_SEP,
	TRMENU_EDIT_RESIZE_CANVAS, TRMENU_EDIT_SCALE_CANVAS, MMENUBAR_ARRAY16_SEP,
	TRMENU_EDIT_IMAGE_OPTION, TRMENU_EDIT_DRAWCOL_TO_BKGNDCOL,
MMENUBAR_ARRAY16_SUB_END,

/* レイヤ */
TRMENU_TOP_LAYER,
MMENUBAR_ARRAY16_SUB_START,
	TRMENU_LAYER_NEW, TRMENU_LAYER_NEW_FOLDER, TRMENU_LAYER_NEW_FROM_FILE, TRMENU_LAYER_NEW_ABOVE, MMENUBAR_ARRAY16_SEP,
	TRMENU_LAYER_COPY, TRMENU_LAYER_DELETE, TRMENU_LAYER_ERASE, MMENUBAR_ARRAY16_SEP,
	TRMENU_LAYER_DROP, TRMENU_LAYER_COMBINE, TRMENU_LAYER_COMBINE_MULTI, TRMENU_LAYER_BLEND_ALL, MMENUBAR_ARRAY16_SEP,
	TRMENU_LAYER_TONE_TO_GRAY, MMENUBAR_ARRAY16_SEP,
	TRMENU_LAYER_OUTPUT, MMENUBAR_ARRAY16_SEP,

	//設定
	TRMENU_LAYER_OPTION,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_LAYER_OPT_OPTION, TRMENU_LAYER_OPT_TYPE, TRMENU_LAYER_OPT_COLOR,
	MMENUBAR_ARRAY16_SUB_END,

	//一括変換
	TRMENU_LAYER_BATCH,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_LAYER_BATCH_TONE_LINES,
	MMENUBAR_ARRAY16_SUB_END,

	//編集
	TRMENU_LAYER_EDIT,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_LAYER_EDIT_REV_HORZ, TRMENU_LAYER_EDIT_REV_VERT,
		TRMENU_LAYER_EDIT_ROTATE_LEFT, TRMENU_LAYER_EDIT_ROTATE_RIGHT,
	MMENUBAR_ARRAY16_SUB_END,

	//表示
	TRMENU_LAYER_VIEW,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_LAYER_VIEW_ALL_SHOW, TRMENU_LAYER_VIEW_ALL_HIDE,
		TRMENU_LAYER_VIEW_ONLY_CURRENT, TRMENU_LAYER_VIEW_REV_CHECKED,
		TRMENU_LAYER_VIEW_REV_IMAGE,
	MMENUBAR_ARRAY16_SUB_END,

	//フォルダ
	TRMENU_LAYER_FOLDER,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_LAYER_FOLDER_CHECKED_MOVE, TRMENU_LAYER_FOLDER_CLOSE_EX_CUR, TRMENU_LAYER_FOLDER_ALL_OPEN,
	MMENUBAR_ARRAY16_SUB_END,

	//フラグ
	TRMENU_LAYER_FLAG,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_LAYER_FLAG_OFF_FILL, TRMENU_LAYER_FLAG_OFF_LOCK, TRMENU_LAYER_FLAG_OFF_CHECKED,
	MMENUBAR_ARRAY16_SUB_END,
MMENUBAR_ARRAY16_SUB_END,

/* 選択範囲 */
TRMENU_TOP_SELECT,
MMENUBAR_ARRAY16_SUB_START,
	TRMENU_SEL_RELEASE, MMENUBAR_ARRAY16_SEP,
	TRMENU_SEL_ALL, TRMENU_SEL_INVERSE, TRMENU_SEL_EXPAND, MMENUBAR_ARRAY16_SEP,
	TRMENU_SEL_COPY, TRMENU_SEL_CUT, TRMENU_SEL_PASTE_NEWLAYER, MMENUBAR_ARRAY16_SEP,
	TRMENU_SEL_LAYER_OPACITY, TRMENU_SEL_LAYER_DRAWCOL, MMENUBAR_ARRAY16_SEP,
	TRMENU_SEL_OUTPUT_FILE,
MMENUBAR_ARRAY16_SUB_END,

/* フィルタ */
TRMENU_TOP_FILTER,
MMENUBAR_ARRAY16_SUB_START,
	//カラー
	TRMENU_FILTER_TOP_COLOR,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_COLOR_BRIGHT_CONST, TRMENU_FILTER_COLOR_GAMMA, TRMENU_FILTER_COLOR_LEVEL,
		TRMENU_FILTER_COLOR_RGB, TRMENU_FILTER_COLOR_HSV, TRMENU_FILTER_COLOR_HLS, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_COLOR_NEGA, TRMENU_FILTER_COLOR_GRAYSCALE, TRMENU_FILTER_COLOR_SEPIA,
		TRMENU_FILTER_COLOR_GRADMAP, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_COLOR_THRESHOLD, TRMENU_FILTER_COLOR_THRESHOLD_DITHER,
		TRMENU_FILTER_COLOR_POSTERIZE,
	MMENUBAR_ARRAY16_SUB_END,
	//色置換
	TRMENU_FILTER_TOP_COLREP,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_COLREP_DRAWCOL, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_COLREP_DRAWCOL_TO_TP, TRMENU_FILTER_COLREP_EXDRAWCOL_TO_TP,
		TRMENU_FILTER_COLREP_DRAWCOL_TO_BKGND, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_COLREP_TP_TO_DRAWCOL,
	MMENUBAR_ARRAY16_SUB_END,
	//アルファ操作(チェック)
	TRMENU_FILTER_TOP_ALPHA1,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_ALPHA1_TP_TO_TP, TRMENU_FILTER_ALPHA1_NOTTP_TO_TP,
		TRMENU_FILTER_ALPHA1_COPY, TRMENU_FILTER_ALPHA1_ADD, TRMENU_FILTER_ALPHA1_SUB,
		TRMENU_FILTER_ALPHA1_MUL, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_ALPHA1_LUM_REV, TRMENU_FILTER_ALPHA1_LUM,
	MMENUBAR_ARRAY16_SUB_END,
	//アルファ操作(カレント)
	TRMENU_FILTER_TOP_ALPHA2,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_ALPHA2_LUM_REV, TRMENU_FILTER_ALPHA2_LUM,
		TRMENU_FILTER_ALPHA2_OPAQUE_TO_MAX,
		TRMENU_FILTER_ALPHA2_TEXTURE, TRMENU_FILTER_ALPHA2_CREATE_GRAYSCALE,
	MMENUBAR_ARRAY16_SUB_END,
	//ぼかし
	TRMENU_FILTER_TOP_BLUR,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_BLUR_BLUR, TRMENU_FILTER_BLUR_GAUSS, TRMENU_FILTER_BLUR_MOTION,
		TRMENU_FILTER_BLUR_RADIAL, TRMENU_FILTER_BLUR_LENS,
	MMENUBAR_ARRAY16_SUB_END,
	//描画
	TRMENU_FILTER_TOP_DRAW,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_DRAW_CLOUD, TRMENU_FILTER_DRAW_AMITONE, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_DRAW_RANDOM_POINT, TRMENU_FILTER_DRAW_EDGE_POINT, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_DRAW_FRAME, TRMENU_FILTER_DRAW_HORZVERT_LINE, TRMENU_FILTER_DRAW_PLAID,
	MMENUBAR_ARRAY16_SUB_END,
	//漫画用
	TRMENU_FILTER_TOP_DRAW_COMIC,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_COMIC_AMITONE_CREATE, TRMENU_FILTER_COMIC_TO_AMITONE,
		TRMENU_FILTER_COMIC_SAND_TONE, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_COMIC_CONCLINE, TRMENU_FILTER_COMIC_FLASH, TRMENU_FILTER_COMIC_POPUP_FLASH,
		TRMENU_FILTER_COMIC_UNIFLASH, TRMENU_FILTER_COMIC_UNIFLASH_WAVE,
	MMENUBAR_ARRAY16_SUB_END,
	//ピクセレート
	TRMENU_FILTER_TOP_PIXELATE,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_PIXELATE_MOZAIC, TRMENU_FILTER_PIXELATE_CRYSTAL, TRMENU_FILTER_PIXELATE_HALFTONE,
	MMENUBAR_ARRAY16_SUB_END,
	//効果
	TRMENU_FILTER_TOP_EFFECT,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_EFFECT_GLOW, TRMENU_FILTER_EFFECT_RGBSHIFT, TRMENU_FILTER_EFFECT_OILPAINT,
		TRMENU_FILTER_EFFECT_EMBOSS, TRMENU_FILTER_EFFECT_NOISE,
		TRMENU_FILTER_EFFECT_DIFFUSION, TRMENU_FILTER_EFFECT_SCRATCH,
		TRMENU_FILTER_EFFECT_MEDIAN, TRMENU_FILTER_EFFECT_BLURRING,
	MMENUBAR_ARRAY16_SUB_END,
	//輪郭
	TRMENU_FILTER_TOP_EDGE,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_EDGE_SHARP, TRMENU_FILTER_EDGE_UNSHARPMASK, MMENUBAR_ARRAY16_SEP,
		TRMENU_FILTER_EDGE_SOBEL, TRMENU_FILTER_EDGE_LAPLACIAN, TRMENU_FILTER_EDGE_HIGHPASS,
	MMENUBAR_ARRAY16_SUB_END,
	//変形
	TRMENU_FILTER_TOP_TRANSFORM,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_TRANS_WAVE, TRMENU_FILTER_TRANS_RIPPLE, TRMENU_FILTER_TRANS_POLAR,
		TRMENU_FILTER_TRANS_RADIAL_SHIFT, TRMENU_FILTER_TRANS_SWIRL,
	MMENUBAR_ARRAY16_SUB_END,
	//ほか
	TRMENU_FILTER_TOP_OTHER,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_FILTER_OTHER_LUM_TO_ALPHA, TRMENU_FILTER_OTHER_1PXDOT_THINNING,
		TRMENU_FILTER_OTHER_ANTIALIASING, TRMENU_FILTER_OTHER_HEMMING,
		TRMENU_FILTER_OTHER_3DFRAME, TRMENU_FILTER_OTHER_SHIFT,
	MMENUBAR_ARRAY16_SUB_END,
MMENUBAR_ARRAY16_SUB_END,

/* 表示 */
TRMENU_TOP_VIEW,
MMENUBAR_ARRAY16_SUB_START,
	TRMENU_VIEW_MINIMIZE, MMENUBAR_ARRAY16_SEP,
	//パネル
	TRMENU_VIEW_PANEL_VISIBLE,
	TRMENU_VIEW_PANEL,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_VIEW_PANEL_ALL_WINDOW, TRMENU_VIEW_PANEL_ALL_STORE, MMENUBAR_ARRAY16_SEP,
		TRMENU_VIEW_PANEL_TOOL, TRMENU_VIEW_PANEL_TOOL_LIST, TRMENU_VIEW_PANEL_BRUSHOPT,
		TRMENU_VIEW_PANEL_OPTION, TRMENU_VIEW_PANEL_LAYER,
		TRMENU_VIEW_PANEL_COLOR, TRMENU_VIEW_PANEL_COLOR_WHEEL, TRMENU_VIEW_PANEL_COLOR_PALETTE,
		TRMENU_VIEW_PANEL_CANVAS_CTRL, TRMENU_VIEW_PANEL_CANVAS_VIEW, TRMENU_VIEW_PANEL_IMAGE_VIEWER,
		TRMENU_VIEW_PANEL_FILTER_LIST,
	MMENUBAR_ARRAY16_SUB_END,
	//チェック
	MMENUBAR_ARRAY16_SEP,
	TRMENU_VIEW_CANVAS_MIRROR, TRMENU_VIEW_BKGND_PLAID, TRMENU_VIEW_GRID,
	TRMENU_VIEW_GRID_SPLIT, TRMENU_VIEW_RULE_GUIDE, MMENUBAR_ARRAY16_SEP,
	TRMENU_VIEW_TOOLBAR, TRMENU_VIEW_STATUSBAR, MMENUBAR_ARRAY16_SEP,
	TRMENU_VIEW_CURSOR_POS, TRMENU_VIEW_LAYER_NAME, TRMENU_VIEW_BOXSEL_POS,
	MMENUBAR_ARRAY16_SEP,
	//表示倍率
	TRMENU_VIEW_CANVAS_ZOOM,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_VIEW_CANVASZOOM_UP, TRMENU_VIEW_CANVASZOOM_DOWN, MMENUBAR_ARRAY16_SEP,
		TRMENU_VIEW_CANVASZOOM_ORIGINAL, TRMENU_VIEW_CANVASZOOM_FIT,
	MMENUBAR_ARRAY16_SUB_END,
	//回転
	TRMENU_VIEW_CANVAS_ROTATE,
	MMENUBAR_ARRAY16_SUB_START,
		TRMENU_VIEW_CANVASROTATE_LEFT, TRMENU_VIEW_CANVASROTATE_RIGHT, MMENUBAR_ARRAY16_SEP,
		TRMENU_VIEW_CANVASROTATE_0, TRMENU_VIEW_CANVASROTATE_90,
		TRMENU_VIEW_CANVASROTATE_180, TRMENU_VIEW_CANVASROTATE_270,
	MMENUBAR_ARRAY16_SUB_END,
MMENUBAR_ARRAY16_SUB_END,

/* 設定ほか */
TRMENU_TOP_OPTION,
MMENUBAR_ARRAY16_SUB_START,
	TRMENU_OPT_ENV, TRMENU_OPT_GRID, TRMENU_OPT_CANVASKEY,
	TRMENU_OPT_SHORTCUTKEY, TRMENU_OPT_PANEL, MMENUBAR_ARRAY16_SEP,
	TRMENU_OPT_VERSION,
MMENUBAR_ARRAY16_SUB_END,

MMENUBAR_ARRAY16_END
};
