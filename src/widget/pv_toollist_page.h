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

/*****************************************
 * ToolListPage 内部定義
 *****************************************/

/* ボタンが押された時の情報 */

typedef struct
{
	ToolListGroupItem *group;
	ToolListItem *item;
	int width,	//アイテムの表示幅 (右の枠1px含まない。0 でグループアイテムの範囲外)
		x,y;	//アイテムの左上位置 (外枠の位置 [y は -1]。ウィジェット座標)
}_POINTINFO;

/* ToolListPage */

typedef struct
{
	mWidget wg;

	mScrollBar *scr;

	int itemh,		//アイテム一つの高さ (下枠1px含まない)
		headerh,	//グループヘッダの高さ
		fdrag;

	_POINTINFO dnd_info;		//D&D先の情報
	ToolListItem *dnd_srcitem;	//D&D元のアイテム
	mPoint dnd_pt_press;		//D&Dボタン押し時の位置
}ToolListPage;


/* ToolListPage */

void ToolListPage_updateAll(ToolListPage *p);
void ToolListPage_notify_changeValue(ToolListPage *p);
void ToolListPage_notify_updateToolListPanel(ToolListPage *p);
void ToolListPage_drawItemSelFrame(ToolListPage *p,_POINTINFO *info,mlkbool erase);

void ToolListPage_runMenu_empty(ToolListPage *p,int x,int y);
void ToolListPage_runMenu_group(ToolListPage *p,int x,int y,ToolListGroupItem *group);
void ToolListPage_runMenu_item(ToolListPage *p,int x,int y,_POINTINFO *info);

void ToolListPage_cmd_itemOption(ToolListPage *p,ToolListItem *item);

