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
 * MainWindow 関数
 ************************************/

typedef struct _MainWindow MainWindow;
typedef struct _LayerItem  LayerItem;
typedef struct _LoadImageOption LoadImageOption;

/* mainwindow.c */

void MainWindow_quit(void);
void MainWindow_setTitle(MainWindow *p);
void MainWindow_errmes(mlkerr err,const char *detail);
void MainWindow_errmes_win(mWindow *parent,mlkerr err,const char *detail);
mlkbool MainWindow_confirmSave(MainWindow *p);
void MainWindow_updateNewCanvas(MainWindow *p,const char *filename);
mWidget *MainWindow_getProgressBarPos(mBox *box);

/* mainwin_file.c */

void MainWindow_openFileDialog(MainWindow *p,int recentno);
mlkbool MainWindow_loadImage(MainWindow *p,const char *filename,LoadImageOption *opt);

/* mainwin_cmd.c */

void MainWindow_runCanvasKeyCmd(int cmd);
void MainWindow_undo_redo(MainWindow *p,mlkbool redo);

/* mainwin_layer.c */

void MainWindow_layer_new_tone(MainWindow *p,mlkbool type_a1);
void MainWindow_layer_setColor(MainWindow *p,LayerItem *pi);
void MainWindow_layer_setOption(MainWindow *p,LayerItem *pi);


