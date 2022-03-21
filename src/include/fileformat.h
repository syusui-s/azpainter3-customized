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

/************************************
 * 画像ファイルフォーマット定義
 ************************************/

/* フラグ */
enum
{
	FILEFORMAT_UNKNOWN = 0,
	
	FILEFORMAT_PNG  = 1<<0,
	FILEFORMAT_JPEG = 1<<1,
	FILEFORMAT_GIF  = 1<<2,
	FILEFORMAT_BMP  = 1<<3,
	FILEFORMAT_TIFF = 1<<4,
	FILEFORMAT_WEBP = 1<<5,

	FILEFORMAT_APD = 1<<6,
	FILEFORMAT_ADW = 1<<7,
	FILEFORMAT_PSD = 1<<8,

	/* option */

	FILEFORMAT_APD_v1v2 = 1<<16,
	FILEFORMAT_APD_v3 = 1<<17,
	FILEFORMAT_APD_v4 = 1<<18,

	//------------

	//読み込みの通常画像
	FILEFORMAT_NORMAL_IMAGE = FILEFORMAT_PNG | FILEFORMAT_JPEG | FILEFORMAT_GIF | FILEFORMAT_BMP
		| FILEFORMAT_TIFF | FILEFORMAT_WEBP | FILEFORMAT_PSD,

	//保存設定があるもの
	FILEFORMAT_NEED_SAVEOPTION = FILEFORMAT_PNG | FILEFORMAT_JPEG | FILEFORMAT_GIF
		| FILEFORMAT_TIFF | FILEFORMAT_WEBP | FILEFORMAT_PSD
};

/* 保存時のタイプ
 *
 * [!] 追加・変更時は、
 * mainwin_menu.c:MainWindow_menu_savedup(),
 * mainwin_file.c 内の文字列など変更。 */

enum
{
	SAVE_FORMAT_TYPE_APD,
	SAVE_FORMAT_TYPE_PSD,
	SAVE_FORMAT_TYPE_PNG,
	SAVE_FORMAT_TYPE_JPEG,
	SAVE_FORMAT_TYPE_BMP,
	SAVE_FORMAT_TYPE_GIF,
	SAVE_FORMAT_TYPE_TIFF,
	SAVE_FORMAT_TYPE_WEBP
};

/* draw_loadfile.c */

uint32_t FileFormat_getFromFile(const char *filename);
mlkbool FileFormat_getLoadImageType(uint32_t format,void *dst);


