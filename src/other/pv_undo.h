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

/********************************
 * アンドゥ内部用
 ********************************/

typedef struct _mZlib mZlib;

typedef struct
{
	mUndo undo;

	UndoUpdateInfo update;	//(作業用) 更新情報
	uint8_t fmodify;		//データが変更されたかのフラグに使う

	uint32_t used_bufsize;	//バッファに確保されたアンドゥデータの総サイズ
	int cur_fileno,			//[file] ファイル番号の現在値
		write_type,			//書き込み時の出力タイプ
		write_tmpsize,		//[buf] 一時バッファに書き込まれたサイズ
		write_remain;		//[buf] 可変サイズ書き込み時の残りバッファサイズ
	uint8_t *writetmpbuf,	//書き込み時の一時出力バッファ
		*workbuf1,			//タイルイメージ用バッファ
		*workbuf2,
		*write_dst,			//[buf] 書き込み位置
		*read_dst;			//[buf] 読み込み位置
	FILE *writefp,
		*readfp;
	int64_t writefpos;		//[file] 書き込み位置記録

	mZlib *zenc,	//[file] レイヤタイル圧縮用
		*zdec;		//[file] レイヤタイル展開用
}AppUndo;

extern AppUndo *g_app_undo;

#define APPUNDO g_app_undo

#define UNDO_WRITETEMP_BUFSIZE  (128 * 1024) //一時出力バッファのサイズ

