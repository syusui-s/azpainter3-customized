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
 * キー
 *****************************************/

#include "mlk_gui.h"
#include "mlk_key.h"

#include "mlk_pv_gui.h"


/**@ キー識別子から、矢印キーの移動方向を取得
 *
 * @r:-1 で矢印キーではない。\
 * bit0: 0=左右、1=上下。\
 * bit1: 0=左/上、1=右/下。\
 * 0=左、1=上、2=右、3=下。 */

int mKeyGetArrowDir(uint32_t key)
{
	switch(key)
	{
		case MKEY_LEFT:
		case MKEY_KP_LEFT:
			return 0;
		case MKEY_UP:
		case MKEY_KP_UP:
			return 1;
		case MKEY_RIGHT:
		case MKEY_KP_RIGHT:
			return 2;
		case MKEY_DOWN:
		case MKEY_KP_DOWN:
			return 3;
	}

	return -1;
}

/**@ キー識別子が矢印キーかどうか */

mlkbool mKeyIsArrow(uint32_t key)
{
	return (key == MKEY_LEFT || key == MKEY_KP_LEFT
		|| key == MKEY_RIGHT || key == MKEY_KP_RIGHT
		|| key == MKEY_UP || key == MKEY_KP_UP
		|| key == MKEY_DOWN || key == MKEY_KP_DOWN);
}

/**@ キー識別子がアルファベットかどうか */

mlkbool mKeyIsAlphabet(uint32_t key)
{
	return ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z'));
}

/**@ バックエンドのキーコードから、キー識別子 (MKEY_*) を取得
 *
 * @r:変換できなかった場合、0 */

uint32_t mKeycodeToKey(uint32_t code)
{
	return (MLKAPP->bkend.keycode_to_code)(code);
}

/**@ キー識別子から、キーの名前の文字列を取得
 *
 * @r:成功したか */

mlkbool mKeycodeGetName(mStr *str,uint32_t code)
{
	return (MLKAPP->bkend.keycode_getname)(str, code);
}

