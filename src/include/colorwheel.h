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

/******************************
 * カラーホイールウィジェット
 ******************************/

typedef struct _ColorWheel ColorWheel;

enum
{
	COLORWHEEL_TYPE_HSV_TRIANGLE,
	COLORWHEEL_TYPE_HSV_RECTANGLE
};

enum
{
	COLORWHEEL_N_CHANGE_COL	//色が変わった時(param1=RGB, param2=CHANGECOL)
};

ColorWheel *ColorWheel_new(mWidget *parent,int id,uint32_t col);
void ColorWheel_setColor(ColorWheel *p,uint32_t col);

