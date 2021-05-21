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
 * フォント
 *****************************************/

typedef struct _DrawFont DrawFont;

void DrawFontInit(void);
void DrawFontFinish(void);

DrawFont *DrawFont_create(DrawTextData *dt,int imgdpi);
void DrawFont_free(DrawFont *p);

void DrawFont_setSize(DrawFont *p,DrawTextData *dt,int imgdpi);
void DrawFont_setRubySize(DrawFont *p,DrawTextData *dt,int imgdpi);
void DrawFont_setInfo(DrawFont *p,DrawTextData *dt);

void DrawFont_drawText(DrawFont *p,int x,int y,int imgdpi,DrawTextData *dt,mFontDrawInfo *info);

