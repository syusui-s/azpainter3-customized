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

/*******************************
 * カラーパレット用文字列ID
 *******************************/

enum
{
	//メニュー
	TRID_PAL_PALLIST = 0,
	TRID_PAL_OPTION,
	TRID_PAL_EDIT,
	TRID_PAL_FILE,
	TRID_PAL_HELP,
	TRID_PAL_EDIT_PALETTE,
	TRID_PAL_EDIT_ALL_DRAWCOL,
	TRID_PAL_FILE_LOAD,
	TRID_PAL_FILE_LOAD_APPEND,
	TRID_PAL_FILE_LOAD_IMAGE,
	TRID_PAL_FILE_SAVE,

	TRID_PAL_MESSAGE_ALL_DRAWCOL = 100,

	//中間色設定
	TRID_GRAD_TITLE = 1000,
	TRID_GRAD_STEP,
	TRID_GRAD_STEP_HELP,

	//パレットリストダイアログ
	TRID_PALLIST_TITLE = 1100,

	//パレットビューのメニュー
	TRID_PALVIEW_SETCOL = 1200,
	TRID_PALVIEW_GETCOL,

	//パレット設定
	TRID_PALOPT_TITLE = 1300,
	TRID_PALOPT_COLNUM,
	TRID_PALOPT_CELLW,
	TRID_PALOPT_CELLH,
	TRID_PALOPT_XMAXNUM,

	//パレット編集
	TRID_PALEDIT_TITLE = 1400,
	TRID_PALEDIT_RGBINPUT,
	TRID_PALEDIT_RGBINPUT_HELP,
	TRID_PALEDIT_HELP,
	TRID_PALEDIT_ADDINSERT,
	TRID_PALEDIT_TOOLTIP_TOP = 1450
};

