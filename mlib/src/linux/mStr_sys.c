/*$
 Copyright (C) 2013-2020 Azel.

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
 * mStr [Linux]
 *****************************************/

#include "mDef.h"
#include "mStr.h"


/**
@addtogroup str
@{
*/


/** コマンド実行の引数用に特殊文字をエスケープした文字列を取得 */
/* 「 !?()[]<>{};&|^*$`'"\」の文字 */

void mStrEscapeCmdline(mStr *str,const char *text)
{
	//c = 32..127 の各文字のフラグ
	uint8_t flags[12] = {0xd7,0x07,0x00,0xd8,0x00,0x00,0x00,0x78,0x01,0x00,0x00,0x38};
	char c;
	int n;
	
	mStrEmpty(str);

	if(text)
	{
		while((c = *(text++)))
		{
			//フラグONならエスケープ
			
			if(c >= 32 && c <= 127)
			{
				n = c - 32;

				if(flags[n >> 3] & (1 << (n & 7)))
					mStrAppendChar(str, '\\');
			}

			mStrAppendChar(str, c);
		}
	}
}

/** @} */
