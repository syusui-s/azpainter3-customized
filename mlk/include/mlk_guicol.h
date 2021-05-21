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

#ifndef MLK_GUICOL_H
#define MLK_GUICOL_H

enum MGUICOL
{
	MGUICOL_WHITE,

	MGUICOL_FACE,
	MGUICOL_FACE_DISABLE,
	MGUICOL_FACE_SELECT,
	MGUICOL_FACE_SELECT_UNFOCUS,
	MGUICOL_FACE_SELECT_LIGHT,
	MGUICOL_FACE_TEXTBOX,
	MGUICOL_FACE_DARK,

	MGUICOL_FRAME,
	MGUICOL_FRAME_BOX,
	MGUICOL_FRAME_FOCUS,
	MGUICOL_FRAME_DISABLE,

	MGUICOL_GRID,

	MGUICOL_TEXT,
	MGUICOL_TEXT_DISABLE,
	MGUICOL_TEXT_SELECT,

	MGUICOL_BUTTON_FACE,
	MGUICOL_BUTTON_FACE_PRESS,
	MGUICOL_BUTTON_FACE_DEFAULT,

	MGUICOL_SCROLLBAR_FACE,
	MGUICOL_SCROLLBAR_GRIP,

	MGUICOL_MENU_FACE,
	MGUICOL_MENU_FRAME,
	MGUICOL_MENU_SEP,
	MGUICOL_MENU_SELECT,
	MGUICOL_MENU_TEXT,
	MGUICOL_MENU_TEXT_DISABLE,
	MGUICOL_MENU_TEXT_SHORTCUT,

	MGUICOL_TOOLTIP_FACE,
	MGUICOL_TOOLTIP_FRAME,
	MGUICOL_TOOLTIP_TEXT,

	MGUICOL_NUM
};

#define MGUICOL_RGB(name) mGuiCol_getRGB(MGUICOL_ ## name)
#define MGUICOL_PIX(name) mGuiCol_getPix(MGUICOL_ ## name)


#ifdef __cplusplus
extern "C" {
#endif

mRgbCol mGuiCol_getRGB(int no);
mPixCol mGuiCol_getPix(int no);

void mGuiCol_changeColor(int no,uint32_t col);
void mGuiCol_setRGBColor(int no,uint32_t col);
void mGuiCol_setDefaultRGB(void);
void mGuiCol_RGBtoPix_all(void);

#ifdef __cplusplus
}
#endif

#endif
