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

/***************************************
 * ウィジェットデータの定義
 ***************************************/

typedef struct _MainWindow MainWindow;
typedef struct _MainCanvas MainCanvas;
typedef struct _MainCanvasPage MainCanvasPage;
typedef struct _StatusBar StatusBar;

#define PANEL_PANE_NUM  3	//パネルのペイン数

/** パネルの番号 */

enum
{
	PANEL_TOOL,
	PANEL_TOOLLIST,
	PANEL_BRUSHOPT,
	PANEL_OPTION,
	PANEL_LAYER,
	PANEL_COLOR,
	PANEL_COLOR_WHEEL,
	PANEL_COLOR_PALETTE,
	PANEL_CANVAS_CTRL,
	PANEL_CANVAS_VIEW,
	PANEL_IMAGE_VIEWER,
	PANEL_FILTER_LIST,

	PANEL_NUM
};

/** ウィジェット関連データ */

typedef struct _AppWidgets
{
	MainWindow *mainwin;
	mWidget *pane[PANEL_PANE_NUM];		//パネルのペイン
	MainCanvas *canvas;
	MainCanvasPage *canvaspage;
	StatusBar *statusbar;
	mPanel *panel[PANEL_NUM];

	mFont *font_panel,		//パネルのフォント (NULL で GUI フォント)
		*font_panel_fix;	//パネルのフォント (12px)
}AppWidgets;

//---------

extern AppWidgets *g_app_widgets;
#define APPWIDGET g_app_widgets

