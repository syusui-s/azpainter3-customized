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

/***************************************
 * フィルタの保存パラメータ
 ***************************************/

void FilterParam_getConfig(mList *list,mIniRead *ini);
void FilterParam_setConfig(mList *list,void *fp_ptr);

char *FilterParam_getText(mList *list,int cmdid);
void FilterParam_setData(mList *list,int cmdid,const char *text);

mlkbool FilterParam_getValue(const char *text,char id,int *dst);
