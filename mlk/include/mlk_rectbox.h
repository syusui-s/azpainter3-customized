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

#ifndef MLK_RECTBOX_H
#define MLK_RECTBOX_H

#ifdef __cplusplus
extern "C" {
#endif

mlkbool mRectIsEmpty(const mRect *rc);
void mRectEmpty(mRect *rc);
void mRectSet(mRect *rc,int x1,int y1,int x2,int y2);
void mRectSetPack(mRect *rc,uint32_t val);
void mRectSetBox_d(mRect *rc,int x,int y,int w,int h);
void mRectSetBox(mRect *rc,const mBox *box);
void mRectSetPoint(mRect *rc,const mPoint *pt);
void mRectSetPoint_minmax(mRect *rc,const mPoint *pt1,const mPoint *pt2);
void mRectSwap_minmax(mRect *rc);
void mRectSwap_minmax_to(mRect *dst,const mRect *src);
void mRectUnion(mRect *dst,const mRect *src);
void mRectUnion_box(mRect *dst,const mBox *box);
mlkbool mRectClipBox_d(mRect *rc,int x,int y,int w,int h);
mlkbool mRectClipRect(mRect *rc,const mRect *clip);
mlkbool mRectIsInRect(mRect *rc,const mRect *rcin);
mlkbool mRectIsPointIn(const mRect *rc,int x,int y);
mlkbool mRectIsCross(const mRect *rc1,const mRect *rc2);
void mRectIncPoint(mRect *rc,int x,int y);
void mRectDeflate(mRect *rc,int dx,int dy);
void mRectMove(mRect *rc,int mx,int my);

void mBoxSet(mBox *box,int x,int y,int w,int h);
void mBoxSetPoint_minmax(mBox *box,const mPoint *pt1,const mPoint *pt2);
void mBoxSetRect(mBox *box,const mRect *rc);
void mBoxSetRect_empty(mBox *box,const mRect *rc);
void mBoxSetPoints(mBox *box,const mPoint *pt,int num);
void mBoxUnion(mBox *dst,const mBox *src);
void mBoxResize_keepaspect(mBox *box,int boxw,int boxh,mlkbool noup);
mlkbool mBoxIsCross(const mBox *box1,const mBox *box2);
mlkbool mBoxIsPointIn(const mBox *box,int x,int y);

#ifdef __cplusplus
}
#endif

#endif
