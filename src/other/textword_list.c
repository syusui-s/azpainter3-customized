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

/****************************
 * テキストの単語リスト
 ****************************/

#include <string.h>
#include <stdio.h>

#include "mlk_gui.h"
#include "mlk_list.h"
#include "mlk_str.h"
#include "mlk_file.h"
#include "mlk_stdio.h"

#include "textword_list.h"


#define _FILENAME_WORDLIST  "textword.txt"


/* データの変更時は、AppConfig::fmodify_textword = TRUE にする。 */


/** リストに追加 */

void TextWordList_add(mList *list,mStr *strname,mStr *strtxt)
{
	TextWordItem *pi;

	pi = (TextWordItem *)mListAppendNew(list, sizeof(TextWordItem) + strname->len + 1 + strtxt->len);
	if(!pi) return;

	pi->text_pos = strname->len + 1;

	memcpy(pi->txt, strname->buf, strname->len);

	memcpy(pi->txt + pi->text_pos, strtxt->buf, strtxt->len);
}

/** ファイルから読み込み */

void TextWordList_loadFile(mList *list)
{
	mStr str = MSTR_INIT;
	mFileText *p;
	char *line,*pc;
	TextWordItem *pi;
	mlkerr ret;
	int len_name,len_txt;

	//開く

	mGuiGetPath_config(&str, _FILENAME_WORDLIST);

	ret = mFileText_readFile(&p, str.buf);

	mStrFree(&str);

	if(ret) return;

	//

	while(1)
	{
		line = mFileText_nextLine_skip(p);
		if(!line) break;

		pc = strchr(line, '\t');
		if(!pc) continue;
		
		len_name = pc - line;
		len_txt = strlen(pc + 1);

		if(!len_name || !len_txt) continue;

		//追加
		
		pi = (TextWordItem *)mListAppendNew(list, sizeof(TextWordItem) + len_name + 1 + len_txt);
		if(pi)
		{
			pi->text_pos = len_name + 1;

			memcpy(pi->txt, line, len_name);
			memcpy(pi->txt + pi->text_pos, pc + 1, len_txt);
		}
	}

	//

	mFileText_end(p);
}

/** ファイルに保存 */

void TextWordList_saveFile(mList *list)
{
	FILE *fp;
	TextWordItem *pi;
	mStr str = MSTR_INIT;

	//開く

	mGuiGetPath_config(&str, _FILENAME_WORDLIST);

	fp = mFILEopen(str.buf, "wt");

	mStrFree(&str);

	if(!fp) return;

	//

	MLK_LIST_FOR(*list, pi, TextWordItem)
	{
		fprintf(fp, "%s\t%s\n", pi->txt, pi->txt + pi->text_pos);
	}

	fclose(fp);
}

