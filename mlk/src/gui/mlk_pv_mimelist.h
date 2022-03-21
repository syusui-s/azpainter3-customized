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

#ifndef MLK_PV_MIMELIST_H
#define MLK_PV_MIMELIST_H

typedef struct
{
	mListItem i;
	void *param;
	char name[1]; //MIME タイプ文字列
}mMimeListItem;

void mMimeListAdd(mList *list,const char *type,void *param);
void mMimeListAdd_check(mList *list,const char *type,void *param);
void mMimeListDelete_param(mList *list,void *param);
mlkbool mMimeListFind_eq_param(mList *list,const char *type,void *param);
mlkbool mMimeListFind_eq(mList *list,const char *type);
char **mMimeListCreateNameList(mList *list);
char **mMimeListCreateNameList_eq_param(mList *list,void *param);

#endif
