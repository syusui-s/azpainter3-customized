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
 * 環境設定 内部定義
 ************************************/

/** 作業用データ */

typedef struct
{
	uint8_t fchange;	//バッファなどのデータ変更フラグ

	int img_defbits,
		undo_maxnum,
		undo_maxbufsize,
		canv_zoom_step,
		canv_rotate_step,
		iconsize[3],
		toolbar_btts_size,
		cursor_hotspot[2];
	uint32_t col_canvasbk,
		col_plaid[2],
		col_ruleguide,
		foption;
	uint8_t btt_default[CONFIG_POINTERBTT_NUM],
		btt_pentab[CONFIG_POINTERBTT_NUM],
		*toolbar_btts;
	mStr str_sysdir[3],
		str_font_panel,
		str_cursor_file;
}EnvOptData;

/** fchange */

enum
{
	ENVOPT_CHANGE_F_TOOLBAR_BTTS = 1<<0
};

/** 文字列ID */

enum
{
	TRID_TITLE,

	//リスト
	TRID_LIST_OPT1,
	TRID_LIST_FLAGS,
	TRID_LIST_BTT,
	TRID_LIST_INTERFACE,
	TRID_LIST_SYSTEM,

	//設定1
	TRID_OPT1_CANVAS_BKGND_COL = 100,
	TRID_OPT1_PLAID_COL,
	TRID_OPT1_RULE_GUIDE_COL,
	TRID_OPT1_IMG_DEFBITS,
	TRID_OPT1_UNDO_MAXNUM,
	TRID_OPT1_UNDO_MAXBUFSIZE,
	TRID_OPT1_CANVAS_ZOOM_STEP,
	TRID_OPT1_CANVAS_ROTATE_STEP,

	//フラグ
	TRID_FLAGS_TOP = 150,

	//ボタン
	TRID_BTT_DEVICE_DEFAULT = 200,
	TRID_BTT_DEVICE_PENTAB,
	TRID_BTT_SELECT_COMMAND,
	TRID_BTT_GET_BUTTON,
	TRID_BTT_BTTNAME,
	TRID_BTT_COMMAND,
	TRID_BTT_HELP_GETBTT,
	TRID_BTT_CMD_NONE,
	TRID_BTT_CMD_GROUP_TOOL,
	TRID_BTT_CMD_GROUP_REGIST,
	TRID_BTT_CMD_GROUP_OTHER_OP,
	TRID_BTT_CMD_GROUP_OTHER_CMD,

	//インターフェイス
	TRID_INTERFACE_FONT_PANEL = 250,
	TRID_INTERFACE_ICONSIZE,
	TRID_INTERFACE_ICON_TOOLBAR,
	TRID_INTERFACE_ICON_TOOL,
	TRID_INTERFACE_ICON_OTHER,
	TRID_INTERFACE_TOOLBAR_OPT,

	//システム
	TRID_SYSTEM_TEMPDIR = 300,
	TRID_SYSTEM_BRUSH_DIR,
	TRID_SYSTEM_TEXTURE_DIR,
	TRID_SYSTEM_DRAWCURSOR,
	TRID_SYSTEM_CURSOR_FILE,
	TRID_SYSTEM_HOTSPOT,

	//ほか
	TRID_HELP_RESTART = 1000,
	TRID_MES_TEMPDIR,

	//ボタン名
	TRID_BUTTON_NAME = 1100
};

mWidget *EnvOpt_createContainerView(mPager *pager);

