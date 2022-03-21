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

/************************************
 * MainCanvas 関数
 ************************************/

/* MainCanvas */

void MainCanvas_setScrollInfo(void);
void MainCanvas_setScrollPos(void);

/* MainCanvasPage */

mlkbool MainCanvasPage_isPressed_space(void);
int MainCanvasPage_getPressedRawKey(void);
void MainCanvasPage_showLayerName(void);

void MainCanvasPage_setTimer_updatePage(int time);
void MainCanvasPage_clearTimer_updatePage(void);

void MainCanvasPage_setTimer_updateRect(mBox *boximg);
void MainCanvasPage_clearTimer_updateRect(mlkbool update);

void MainCanvasPage_setTimer_updateMove(void);
void MainCanvasPage_clearTimer_updateMove(void);

void MainCanvasPage_setTimer_updateSelectMove(void);
void MainCanvasPage_clearTimer_updateSelectMove(void);

void MainCanvasPage_setTimer_updateSelectImageMove(void);
mlkbool MainCanvasPage_clearTimer_updateSelectImageMove(void);

void MainCanvasPage_setTimer_updatePasteMove(void);
void MainCanvasPage_clearTimer_updatePasteMove(void);

void MainCanvasPage_changeDrawCursor(void);
void MainCanvasPage_setCursor_forTool(void);
void MainCanvasPage_setCursor(int curno);

