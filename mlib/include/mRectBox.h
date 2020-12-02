/*$
 Copyright (C) 2013-2020 Azel.

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

#ifndef MLIB_RECTBOX_H
#define MLIB_RECTBOX_H

#ifdef __cplusplus
extern "C" {
#endif

mBool mRectIsEmpty(mRect *rc);
void mRectEmpty(mRect *rc);
void mRectSetByPack(mRect *rc,uint32_t val);
void mRectSetBox_d(mRect *rc,int x,int y,int w,int h);
void mRectSetByBox(mRect *rc,mBox *box);
void mRectSetByPoint(mRect *rc,mPoint *pt);
void mRectSetByPoint_minmax(mRect *rc,mPoint *pt1,mPoint *pt2);
void mRectToBox(mBox *box,mRect *rc);
void mRectSwap(mRect *rc);
void mRectSwapTo(mRect *dst,mRect *src);
void mRectUnion(mRect *dst,mRect *src);
void mRectUnion_box(mRect *dst,mBox *box);
mBool mRectClipBox_d(mRect *rc,int x,int y,int w,int h);
mBool mRectClipRect(mRect *rc,mRect *clip);
void mRectIncPoint(mRect *rc,int x,int y);
void mRectDeflate(mRect *rc,int dx,int dy);
void mRectRelMove(mRect *rc,int mx,int my);

void mBoxSetByPoint(mBox *box,mPoint *pt1,mPoint *pt2);
void mBoxSetByRect(mBox *box,mRect *rc);
void mBoxUnion(mBox *dst,mBox *src);
void mBoxScaleKeepAspect(mBox *box,int boxw,int boxh,mBool noscaleup);
void mBoxSetByPoints(mBox *box,mPoint *pt,int num);
mBool mBoxIsCross(mBox *box1,mBox *box2);
mBool mBoxIsPointIn(mBox *box,int x,int y);

#ifdef __cplusplus
}
#endif

#endif
