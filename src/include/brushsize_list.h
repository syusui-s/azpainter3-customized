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

/********************************
 * ブラシサイズリストデータ
 ********************************/

int BrushSizeList_getNum(mBuf *p);
int BrushSizeList_getValue(mBuf *p,int pos);

void BrushSizeList_addFromText(mBuf *p,char *text);
void BrushSizeList_deleteAtPos(mBuf *p,int pos);

void BrushSizeList_loadConfigFile(mBuf *p);
void BrushSizeList_saveConfigFile(mBuf *p);
