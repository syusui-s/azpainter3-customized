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

/*************************************
 * [panel] キャンバスビュー内部定義
 *************************************/

typedef struct _CanvViewPage CanvViewPage;

#define CANVVIEW_ZOOM_MAX   20000

#define _IS_TOOLBAR_VISIBLE (APPCONF->canvview.flags & CONFIG_CANVASVIEW_F_TOOLBAR_VISIBLE)
#define _IS_TOOLBAR_ALWAYS  (APPCONF->canvview.flags & CONFIG_CANVASVIEW_F_TOOLBAR_ALWAYS)
#define _IS_ZOOM_FIT        (APPCONF->canvview.flags & CONFIG_CANVASVIEW_F_FIT)

/* パネルの拡張データ */

typedef struct
{
	int zoom,		//表示倍率 (1=0.1%)
		mirror;		//左右反転表示
	double dscale,
		dscalediv;
	mPoint pt_scroll;		//スクロール位置
	mDoublePoint ptimgct;	//イメージの原点位置
}CanvViewEx;

/* 内容のコンテナ */

typedef struct
{
	MLK_CONTAINER_DEF

	CanvViewEx *ex;

	CanvViewPage *page;
	mIconBar *iconbar;
}CanvView;

/* page からの通知 */

enum
{
	PAGE_NOTIFY_TOGGLE_TOOLBAR,	//ツールバー切り替え
	PAGE_NOTIFY_MENU	//メニュー表示 (param1:x, param2:y)
};


CanvViewPage *CanvViewPage_new(mWidget *parent,CanvViewEx *ex);

void CanvView_setZoom_fit(CanvView *p);
void CanvView_setZoom(CanvView *p,int zoom,double dscale);
void CanvView_image_to_canvas(CanvView *p,mPoint *dst,double x,double y);
void CanvView_canvas_to_image(CanvView *p,mDoublePoint *dst,int x,int y);
void CanvView_setImageCenter(CanvView *p,mlkbool reset);
void CanvView_adjustScroll(CanvView *p);

