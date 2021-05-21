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
 * ドットペン形状
 **********************************/

enum
{
	DOTSHAPE_TYPE_CIRCLE,
	DOTSHAPE_TYPE_CIRCLE_FRAME,
	DOTSHAPE_TYPE_SQUARE,
	DOTSHAPE_TYPE_SQUARE_FRAME,
	DOTSHAPE_TYPE_DIA,	//以降は奇数サイズに調整
	DOTSHAPE_TYPE_DIA_FRAME,
	DOTSHAPE_TYPE_BATU,
	DOTSHAPE_TYPE_CROSS,
	DOTSHAPE_TYPE_KIRAKIRA,

	DOTSHAPE_TYPE_NUM
};

#define DOTSHAPE_MAX_SIZE  101


int DotShape_init(void);
void DotShape_free(void);

void DotShape_create(int type,int size);
int DotShape_getData(uint8_t **ppbuf);
