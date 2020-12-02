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
 * mFontButton
 *****************************************/

#include "mDef.h"

#include "mFontButton.h"

#include "mStr.h"
#include "mWidget.h"
#include "mSysDialog.h"


/**********************//**

@defgroup fontbutton mFontButton
@brief フォント選択ボタン

<h3>継承</h3>
mWidget \> mButton \> mFontButton

@ingroup group_widget
@{

@file mFontButton.h
@def M_FONTBUTTON(p)
@struct mFontButtonData
@struct mFontButton
@enum MFONTBUTTON_NOTIFY

@var MFONTBUTTON_NOTIFY::MFONTBUTTON_N_CHANGEFONT
フォントが変更された

***************************/


/** ボタンテキストセット */

static void _set_button_text(mFontButton *p)
{
	mStr str = MSTR_INIT;
	mFontInfo *info;

	info = &p->fbt.info;
	
	//ファミリ

	if(!(info->mask & MFONTINFO_MASK_FAMILY) || mStrIsEmpty(&info->strFamily))
		mStrSetText(&str, "(default)");
	else
		mStrCopy(&str, &info->strFamily);

	//サイズ

	if(info->mask & MFONTINFO_MASK_SIZE)
	{
		mStrAppendText(&str, " | ");
		mStrAppendDouble(&str, info->size, 1);
	}

	mButtonSetText(M_BUTTON(p), str.buf);

	mStrFree(&str);

	mButtonCalcHintHandle(M_WIDGET(p));
}

/** ボタン押し時 */

static void _onpressed(mButton *btt)
{
	mFontButton *p = M_FONTBUTTON(btt);

	//フォント選択ダイアログ

	if(mSysDlgSelectFont(btt->wg.toplevel, &p->fbt.info, p->fbt.mask))
	{
		//ボタンテキスト

		_set_button_text(p);
	
		//通知
	
		mWidgetAppendEvent_notify(NULL, M_WIDGET(p),
			MFONTBUTTON_N_CHANGEFONT, 0, 0);
	}
}


//==========================


/** 破棄ハンドラ */

void mFontButtonDestroyHandle(mWidget *wg)
{
	mFontInfoFree(&M_FONTBUTTON(wg)->fbt.info);

	mButtonDestroyHandle(wg);
}

/** 作成 */

mFontButton *mFontButtonCreate(mWidget *parent,int id,uint32_t style,
	uint32_t fLayout,uint32_t marginB4)
{
	mFontButton *p;

	p = mFontButtonNew(0, parent, style);
	if(!p) return NULL;

	p->wg.id = id;
	p->wg.fLayout = fLayout;

	mWidgetSetMargin_b4(M_WIDGET(p), marginB4);

	return p;
}

/** 作成 */

mFontButton *mFontButtonNew(int size,mWidget *parent,uint32_t style)
{
	mFontButton *p;
	
	if(size < sizeof(mFontButton)) size = sizeof(mFontButton);
	
	p = (mFontButton *)mButtonNew(size, parent, 0);
	if(!p) return NULL;

	p->wg.destroy = mFontButtonDestroyHandle;
	
	p->btt.onPressed = _onpressed;

	p->fbt.mask = MFONTINFO_MASK_ALL;

	return p;
}

/** 取得する項目のマスクをセット
 *
 * @param mask MFONTINFO_MASK_* */

void mFontButtonSetMask(mFontButton *p,uint32_t mask)
{
	p->fbt.mask = mask;
}

/** フォント情報を文字列フォーマットでセット */

void mFontButtonSetInfoFormat(mFontButton *p,const char *text)
{
	mFontFormatToInfo(&p->fbt.info, text);

	_set_button_text(p);
}

/** フォント情報を文字列フォーマットで取得 */

void mFontButtonGetInfoFormat_str(mFontButton *p,mStr *str)
{
	mFontInfoToFormat(str, &p->fbt.info);
}

/** @} */
