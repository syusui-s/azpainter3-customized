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
 * ツールオプションデータのマクロ
 **********************************/

/* ドットペン/消しゴム/指先
 * [bit22](1)細線 | [bit18](4)shape | [bit14](4)pixelmode | [bit7](7)density{1-100} | [bit0](7)size{1-101}*/

#define TOOLOPT_DOTPEN_DEFAULT    ((100<<7) | 1)
#define TOOLOPT_DOTPEN_1PX_NORMAL ((100<<7) | 1)
#define TOOLOPT_FINGER_DEFAULT    ((25<<7) | 32)

#define TOOLOPT_DOTPEN_GET_SIZE(n)    (n & 127)
#define TOOLOPT_DOTPEN_GET_DENSITY(n) ((n >> 7) & 127)
#define TOOLOPT_DOTPEN_GET_PIXMODE(n) ((n >> 14) & 15)
#define TOOLOPT_DOTPEN_GET_SHAPE(n)   ((n >> 18) & 15)
#define TOOLOPT_DOTPEN_IS_SLIM(n)     ((n & (1<<22)) != 0)

#define TOOLOPT_DOTPEN_SET_SIZE(p,n)    *(p) = (*(p) & ~127) | (n)
#define TOOLOPT_DOTPEN_SET_DENSITY(p,n) *(p) = (*(p) & ~(127<<7)) | ((n) << 7)
#define TOOLOPT_DOTPEN_SET_PIXMODE(p,n) *(p) = (*(p) & ~(15<<14)) | ((n) << 14)
#define TOOLOPT_DOTPEN_SET_SHAPE(p,n)   *(p) = (*(p) & ~(15<<18)) | ((n) << 18)
#define TOOLOPT_DOTPEN_TOGGLE_SLIM(p)   *(p) ^= (1<<22)

/* 図形塗り/消しゴム
 * [bit11](1)antialias | [bit7](4)pixelmode | [bit0](7)density {1-100} */

#define TOOLOPT_FILLPOLY_DEFAULT  ((1<<11) | 100)

#define TOOLOPT_FILLPOLY_GET_DENSITY(n)  (n & 127)
#define TOOLOPT_FILLPOLY_GET_PIXMODE(n)  ((n >> 7) & 15)
#define TOOLOPT_FILLPOLY_IS_ANTIALIAS(n) ((n & (1<<11)) != 0)

#define TOOLOPT_FILLPOLY_SET_DENSITY(p,n)    *(p) = (*(p) & ~127) | (n)
#define TOOLOPT_FILLPOLY_SET_PIXMODE(p,n)    *(p) = (*(p) & ~(15<<7)) | ((n) << 7)
#define TOOLOPT_FILLPOLY_TOGGLE_ANTIALIAS(p) *(p) ^= (1<<11)

/* 塗りつぶし/自動選択
 * [bit21](3)layer | [bit18](3)type | [bit14](4)pixelmode | [bit7](7)誤差{0-99} | [bit0](7)density{1-100}
 */

#define TOOLOPT_FILL_DEFAULT  100
#define TOOLOPT_SELFILL_DEFAULT  0

#define TOOLOPT_FILL_GET_DENSITY(n)  (n & 127)
#define TOOLOPT_FILL_GET_DIFF(n)     ((n >> 7) & 127)
#define TOOLOPT_FILL_GET_PIXMODE(n)  ((n >> 14) & 15)
#define TOOLOPT_FILL_GET_TYPE(n)     ((n >> 18) & 7)
#define TOOLOPT_FILL_GET_LAYER(n)    ((n >> 21) & 7)

#define TOOLOPT_FILL_SET_DENSITY(p,n) *(p) = (*(p) & ~127) | (n)
#define TOOLOPT_FILL_SET_DIFF(p,n)    *(p) = (*(p) & ~(127<<7)) | ((n) << 7)
#define TOOLOPT_FILL_SET_PIXMODE(p,n) *(p) = (*(p) & ~(15<<14)) | ((n) << 14)
#define TOOLOPT_FILL_SET_TYPE(p,n)    *(p) = (*(p) & ~(7<<18)) | ((n) << 18)
#define TOOLOPT_FILL_SET_LAYER(p,n)   *(p) = (*(p) & ~(7<<21)) | ((n) << 21)

/* グラデーション
 * [bit23](1)loop | [bit22](1)reverse | [bit14](8)custom-id | [bit11](3)colortype | [bit7](4)pixmode | [bit0](7)density{1-100}
 */

#define TOOLOPT_GRAD_DEFAULT  100

#define TOOLOPT_GRAD_GET_DENSITY(n)   (n & 127)
#define TOOLOPT_GRAD_GET_PIXMODE(n)   ((n >> 7) & 15)
#define TOOLOPT_GRAD_GET_COLTYPE(n)   ((n >> 11) & 7)
#define TOOLOPT_GRAD_GET_CUSTOM_ID(n) ((n >> 14) & 255)
#define TOOLOPT_GRAD_IS_REVERSE(n)    ((n & (1<<22)) != 0)
#define TOOLOPT_GRAD_IS_LOOP(n)       ((n & (1<<23)) != 0)

#define TOOLOPT_GRAD_SET_DENSITY(p,n)  *(p) = (*(p) & ~127) | (n)
#define TOOLOPT_GRAD_SET_PIXMODE(p,n)  *(p) = (*(p) & ~(15<<7)) | ((n) << 7)
#define TOOLOPT_GRAD_SET_COLTYPE(p,n)  *(p) = (*(p) & ~(7<<11)) | ((n) << 11)
#define TOOLOPT_GRAD_SET_CUSTOM_ID(p,n) *(p) = (*(p) & ~(255<<14)) | ((n) << 14)
#define TOOLOPT_GRAD_TOGGLE_REVERSE(p) *(p) ^= (1<<22)
#define TOOLOPT_GRAD_TOGGLE_LOOP(p)    *(p) ^= (1<<23)

/* 選択範囲 */

#define TOOLOPT_SELECT_DEFAULT  0

#define TOOLOPT_SELECT_IS_HIDE(n)  (n & 1)
#define TOOLOPT_SELECT_TOGGLE_HIDE(p) *(p) ^= 1;

/* スタンプ */

#define TOOLOPT_STAMP_DEFAULT 0

#define TOOLOPT_STAMP_GET_TRANS(n)   (n)
#define TOOLOPT_STAMP_SET_TRANS(p,n)  *(p) = n

/* 切り貼り */

#define TOOLOPT_CUTPASTE_DEFAULT 0

#define TOOLOPT_CUTPASTE_IS_OVERWRITE(n)   (n & 1)
#define TOOLOPT_CUTPASTE_IS_ENABLE_MASK(n) (n & 2)

#define TOOLOPT_CUTPASTE_TOGGLE_OVERWRITE(p)   *(p) ^= 1
#define TOOLOPT_CUTPASTE_TOGGLE_ENABLE_MASK(p) *(p) ^= 2

