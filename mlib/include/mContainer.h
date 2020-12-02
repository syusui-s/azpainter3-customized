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

#ifndef MLIB_CONTAINER_H
#define MLIB_CONTAINER_H

#ifdef __cplusplus
extern "C" {
#endif

mWidget *mContainerCreate(mWidget *parent,int type,int gridcols,int sepw,uint32_t fLayout);
mWidget *mContainerCreateGrid(mWidget *parent,int gridcols,int sepcol,int seprow,uint32_t fLayout);

mContainer *mContainerNew(int size,mWidget *parent);

void mContainerSetType(mContainer *p,int type,int grid_cols);
void mContainerSetTypeGrid(mContainer *p,int gridcols,int sepcol,int seprow);

void mContainerSetPadding_one(mContainer *p,int val);
void mContainerSetPadding_b4(mContainer *p,uint32_t val);

mWidget *mContainerCreateOkCancelButton(mWidget *parent);

#ifdef __cplusplus
}
#endif

#endif
