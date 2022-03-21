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
 * MainWindow 内部で使う関数
 ************************************/

//ファイル保存時
enum
{
	SAVEFILE_TYPE_OVERWRITE,
	SAVEFILE_TYPE_RENAME,
	SAVEFILE_TYPE_DUP
};

//

void MainWindow_createToolBar(MainWindow *p,mlkbool init);

/* mainwin_file.c */

void MainWindow_newCanvas(MainWindow *p);
mlkbool MainWindow_saveFile(MainWindow *p,int savetype,int recentno);

/* mainwin_cmd.c */

void MainWindow_cmd_selectExpand(MainWindow *p);
void MainWindow_cmd_envoption(MainWindow *p);
void MainWindow_cmd_panelLayout(MainWindow *p);

void MainWindow_cmd_resizeCanvas(MainWindow *p);
void MainWindow_cmd_scaleCanvas(MainWindow *p);
void MainWindow_cmd_selectOutputFile(MainWindow *p);

void MainWindow_pane_layout(MainWindow *p,mlkbool relayout);
void MainWindow_pane_toggle(MainWindow *p);

void MainWindow_panel_toggle(MainWindow *p,int no);
void MainWindow_panel_all_toggle(MainWindow *p);
void MainWindow_panel_all_toggle_winmode(MainWindow *p,int type);

/* mainwin_layer.c */

void MainWindow_layercmd(MainWindow *p,int id);

/* mainwin_menu.c */

void MainWindow_setMenuKey(MainWindow *p);

void MainWindow_menu_canvasZoom(MainWindow *p,int id);
int MainWindow_menu_opensave_dir(MainWindow *p,mlkbool save);
void MainWindow_menu_recentFile(MainWindow *p);
int MainWindow_menu_savedup(MainWindow *p);

void MainWindow_setMenu_recentfile(MainWindow *p);
void MainWindow_on_menupopup(mMenu *menu,int id);

/* mainwin_filter.c */

void MainWindow_runFilterCommand(int id);

