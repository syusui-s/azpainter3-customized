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

/*****************************************
 * ツールリストダイアログ
 *****************************************/

typedef struct _CurveSplinePoint CurveSplinePoint;

//グループ設定
typedef struct
{
	mStr strname;
	int colnum;
}ToolListDlg_groupOpt;


mlkbool ToolListDialog_groupOption(mWindow *parent,ToolListDlg_groupOpt *dst,mlkbool is_edit);
mlkbool ToolListDialog_brushSizeOpt(mWindow *parent,int *dst_min,int *dst_max);
void ToolListDialog_toolOptView(mWindow *parent,ToolListItem_tool *item);

void ToolListDialog_edit(mWindow *parent);

mlkbool BrushDialog_pressure(mWindow *parent,uint32_t *pt);

