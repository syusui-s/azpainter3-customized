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

/**********************************
 * MainWindow ウィジェット定義
 **********************************/

#define MAINWINDOW(p)  ((MainWindow *)(p))

typedef struct _MainWindow
{
	MLK_TOPLEVEL_DEF

	mWidget *ct_main,		//キャンバス部分の親コンテナ
			*splitter[PANEL_PANE_NUM];	//スプリッター (配置順に使用)
	mIconBar *ib_toolbar;

	mMenu *menu_main,
		*menu_recentfile;

	mStr strFilename;		//現在の編集ファイル名 (空で新規作成)
	uint32_t fileformat;	//現在のファイルのフォーマットフラグ (FILEFORMAT_*)
	int fsaved;				//一度でも保存されたか (FALSE で、新規 or 読み込んだ後保存されていない)
}MainWindow;

enum
{
	MAINWIN_CMDID_RECENTFILE = 0x10000	//最近使ったファイル
};
