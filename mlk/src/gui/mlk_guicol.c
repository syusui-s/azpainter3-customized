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
 * GUI 色
 *****************************************/

#include <string.h>

#include "mlk_gui.h"
#include "mlk_guicol.h"

#include "mlk_pv_gui.h"


//-------------

static uint32_t g_guicol_pix[MGUICOL_NUM], g_guicol_rgb[MGUICOL_NUM];

static const uint32_t g_guicol_default[] = {
0xffffff, //WHITE

0xeaeaea, //FACE
0xe4e4e4, //FACE_DISABLE
0x4797FF, //FACE_SELECT
0x999999, //FACE_SELECT_UNFOCUS
0x77B5ED, //FACE_SELECT_LIGHT
0xffffff, //FACE_TEXTBOX
0xc0c0c0, //FACE_DARK

0x666666, //FRAME
0xa0a0a0, //FRAME_BOX
0x0D6EE6, //FRAME_FOCUS
0xa8a8a8, //FRAME_DISABLE

0xdddddd, //GRID

0,        //TEXT
0x888888, //TEXT_DISABLE
0xffffff, //TEXT_SELECT

0xE0E0E0, //BUTTON_FACE
0xBABABA, //BUTTON_FACE_PRESS
0xADD4FF, //BUTTON_FACE_DEFAULT

0xd4d4d4, //SCROLLBAR_FACE
0x808080, //SCROLLBAR_GRIP

0x525252, //MENU_FACE
0,        //MENU_FRAME
0xa0a0a0, //MENU_SEP
0x8A9AFF, //MENU_SELECT
0xffffff, //MENU_TEXT
0x9C9C9C, //MENU_TEXT_DISABLE
0xb0b0b0, //MENU_TEXT_SHORTCUT

0x333333, //TOOLTIP_FACE
0,        //TOOLTIP_FRAME
0xffffff  //TOOLTIP_TEXT
};

//-------------



/**@ RGB 色を取得 */

mRgbCol mGuiCol_getRGB(int no)
{
	return g_guicol_rgb[no];
}

/**@ PIX 色を取得 */

mPixCol mGuiCol_getPix(int no)
{
	return g_guicol_pix[no];
}

/**@ 指定色を変更 (RGB & PIX)
 *
 * @p:col RGB 値 */

void mGuiCol_changeColor(int no,uint32_t col)
{
	g_guicol_rgb[no] = col & 0xffffff;
	g_guicol_pix[no] = MLKAPP->bkend.rgbtopix(MLK_RGB_R(col), MLK_RGB_G(col), MLK_RGB_B(col));
}

/**@ 指定 RGB 色を変更 */

void mGuiCol_setRGBColor(int no,uint32_t col)
{
	g_guicol_rgb[no] = col & 0xffffff;
}

/**@ RGB 値をすべてデフォルト色にセット */

void mGuiCol_setDefaultRGB(void)
{
	memcpy(g_guicol_rgb, g_guicol_default, sizeof(g_guicol_default));
}

/**@ RGB 値を元に、すべての PIX 値をセット **/

void mGuiCol_RGBtoPix_all(void)
{
	int i;
	mRgbCol c;
	mPixCol (*func)(uint8_t,uint8_t,uint8_t) = MLKAPP->bkend.rgbtopix;

	for(i = 0; i < MGUICOL_NUM; i++)
	{
		c = g_guicol_rgb[i];
		
		g_guicol_pix[i] = (func)(MLK_RGB_R(c), MLK_RGB_G(c), MLK_RGB_B(c));
	}
}

