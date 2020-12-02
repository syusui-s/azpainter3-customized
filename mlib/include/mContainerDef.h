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

#ifndef MLIB_CONTAINER_DEF_H
#define MLIB_CONTAINER_DEF_H

#include "mWidgetDef.h"

typedef struct
{
	int type,gridCols;
	uint16_t sepW,gridSepCol,gridSepRow;
	mWidgetSpacing padding;
	void (*calcHint)(mWidget *);
}mContainerData;


struct _mContainer
{
	mWidget wg;
	mContainerData ct;
};

enum MCONTAINER_TYPE
{
	MCONTAINER_TYPE_VERT,
	MCONTAINER_TYPE_HORZ,
	MCONTAINER_TYPE_GRID
};

#endif
