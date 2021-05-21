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

/*************************************
 * [panel] イメージビューア内部定義
 *************************************/

typedef struct _ImgViewerPage ImgViewerPage;

#define IMGVIEWER_ZOOM_MAX  10000

/* パネルの拡張データ */

typedef struct
{
	Image32 *img;
	mStr strFilename;	//現在の画像ファイル名

	int scrx,scry,		//画像スクロール位置
		zoom;			//表示倍率 (1=0.1%)
	double dscale,		//表示倍率 (1.0=100%)
		dscalediv;		//1.0/dscale
	mDoublePoint ptimgct;	//画像の中心位置
}ImgViewerEx;

/* 内容のコンテナ */

typedef struct
{
	MLK_CONTAINER_DEF

	ImgViewerEx *ex;

	ImgViewerPage *page;
	mIconBar *iconbar;
}ImgViewer;


void ImgViewer_loadImage(ImgViewer *p,const char *filename);

void ImgViewer_image_to_canvas(ImgViewer *p,mPoint *dst,double x,double y);
void ImgViewer_canvas_to_image(ImgViewer *p,mDoublePoint *dst,int x,int y);

void ImgViewer_setImageCenter(ImgViewer *p,mlkbool reset);
void ImgViewer_setZoom_fit(ImgViewer *p);
void ImgViewer_setZoom(ImgViewer *p,int zoom);
void ImgViewer_adjustScroll(ImgViewer *p);

