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
 * mPixbuf 描画関数
 ************************************/

typedef struct _CanvasDrawInfo CanvasDrawInfo;

void drawpixbuf_line_clip(mPixbuf *p,int x1,int y1,int x2,int y2,uint32_t col,const mRect *rc);
void drawpixbuf_dashBox_mono(mPixbuf *pixbuf,int x,int y,int w,int h);

void drawpixbuf_grid(mPixbuf *pixbuf,mBox *boxdst,mBox *boximg,int gridw,int gridh,uint32_t col,CanvasDrawInfo *info);
void drawpixbuf_grid_split(mPixbuf *pixbuf,mBox *boxdst,mBox *boximg,int splith,int splitv,uint32_t col,CanvasDrawInfo *info);

void drawpixbuf_crossline_blend(mPixbuf *p,int x,int y,int len,uint32_t col);
void drawpixbuf_ellipse_blend(mPixbuf *p,int cx,int cy,double rd,double radio,uint32_t col);

void drawpixbuf_setPixel_selectEdge(mPixbuf *pixbuf,int x,int y,const mRect *rcclip);
void drawpixbuf_line_selectEdge(mPixbuf *pixbuf,int x1,int y1,int x2,int y2,const mRect *rcclip);

void drawpixbuf_boxsel_frame(mPixbuf *pixbuf,const mBox *boximg,CanvasDrawInfo *info,int angle0,mlkbool paste);
void drawpixbuf_rectframe(mPixbuf *pixbuf,const mRect *rcimg,CanvasDrawInfo *info,int angle0,uint32_t col);
