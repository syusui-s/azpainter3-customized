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

/***************************
 * ヘルプ
 ***************************/

#include "mlk_gui.h"
#include "mlk_translation.h"
#include "mlk_sysdlg.h"
#include "mlk_str.h"

#include "apphelp.h"



/* 翻訳データ読み込み */

static mTranslation *_load_trans(void)
{
	mTranslation *trans;
	mStr str = MSTR_INIT;

	mGuiGetPath_data(&str, "tr");

	trans = mTranslationLoadFile_lang(str.buf, NULL, "help-");

	//失敗時は英語
	if(!trans)
		trans = mTranslationLoadFile_lang(str.buf, "en", "help-");

	mStrFree(&str);

	return trans;
}


/** ヘルプをメッセージボックスで表示 */

void AppHelp_message(mWindow *parent,int groupid,int trid)
{
	mTranslation *trans;
	const char *text;

	trans = _load_trans();
	if(!trans) return;

	text = mTranslationGetText2(trans, groupid, trid);

	if(text)
		mMessageBox(parent, "help", text, MMESBOX_OK, MMESBOX_OK);

	mTranslationFree(trans);
}

