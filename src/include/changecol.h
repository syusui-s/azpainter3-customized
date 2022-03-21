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

/*******************************
 * カラー選択時の値
 *******************************/

//[uint32] type(7bit), c1(9bit), c2(8bit), c3(8bit)

enum
{
	CHANGECOL_TYPE_RGB,
	CHANGECOL_TYPE_HSV,
	CHANGECOL_TYPE_HSL
};

#define CHANGECOL_RGB    0
#define CHANGECOL_MAKE(type,c1,c2,c3)  (((type)<<25) | ((c1)<<16) | ((c2)<<8) | (c3))
#define CHANGECOL_GET_TYPE(c)  ((c)>>25)
#define CHANGECOL_GET_C1(c)    (((c)>>16) & 511)
#define CHANGECOL_GET_C2(c)    (((c)>>8) & 255)
#define CHANGECOL_GET_C3(c)    ((c) & 255)
#define CHANGECOL_IS_RGB(c)    (!(c))


typedef struct _mHSVd mHSVd;

int ChangeColor_parse(int *cbuf,uint32_t col);
void ChangeColor_to_HSV(mHSVd *hsv,uint32_t col);
void ChangeColor_to_HSV_int(int *dst,uint32_t col);
void ChangeColor_to_HSL_int(int *dst,uint32_t col);

