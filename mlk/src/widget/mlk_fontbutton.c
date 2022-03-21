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
 * mFontButton
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_fontbutton.h"
#include "mlk_str.h"

#include "mlk_pv_widget.h"



/* ボタンテキストセット */

static void _set_button_text(mFontButton *p)
{
	mStr str = MSTR_INIT;

	mFontInfoGetText_family_size(&str, &p->fbt.info);
	
	mButtonSetText(MLK_BUTTON(p), str.buf);

	mStrFree(&str);

	mButtonHandle_calcHint(MLK_WIDGET(p));
}

/* ボタン押し時のハンドラ */

static void _pressed_handle(mWidget *wg)
{
	mFontButton *p = MLK_FONTBUTTON(wg);

	//フォント選択ダイアログ

	if(mSysDlg_selectFont(wg->toplevel, p->fbt.flags, &p->fbt.info))
	{
		//ボタンテキスト

		_set_button_text(p);
	
		//通知
	
		mWidgetEventAdd_notify(wg, NULL, MFONTBUTTON_N_CHANGE_FONT, 0, 0);
	}
}


//==========================
// main
//==========================


/**@ mFontButton データ解放 */

void mFontButtonDestroy(mWidget *wg)
{
	mFontInfoFree(&MLK_FONTBUTTON(wg)->fbt.info);

	mButtonDestroy(wg);
}

/**@ 作成 */

mFontButton *mFontButtonNew(mWidget *parent,int size,uint32_t fstyle)
{
	mFontButton *p;
	
	if(size < sizeof(mFontButton))
		size = sizeof(mFontButton);
	
	p = (mFontButton *)mButtonNew(parent, size, MBUTTON_S_COPYTEXT);
	if(!p) return NULL;

	p->wg.destroy = mFontButtonDestroy;
	
	p->btt.pressed = _pressed_handle;

	p->fbt.flags = (fstyle & MFONTBUTTON_S_ALL_INFO)?
		MSYSDLG_FONT_F_ALL: MSYSDLG_FONT_F_NORMAL;

	return p;
}

/**@ 作成 */

mFontButton *mFontButtonCreate(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack,uint32_t fstyle)
{
	mFontButton *p;

	p = mFontButtonNew(parent, 0, fstyle);
	if(p)
		__mWidgetCreateInit(MLK_WIDGET(p), id, flayout, margin_pack);

	return p;
}

/**@ フォントダイアログのフラグをセット
 *
 * @d:デフォルトで、拡張情報以外あり */

void mFontButtonSetFlags(mFontButton *p,uint32_t flags)
{
	p->fbt.flags = flags;
}

/**@ テキストからフォント情報をセット */

void mFontButtonSetInfo_text(mFontButton *p,const char *text)
{
	mFontInfoSetFromText(&p->fbt.info, text);

	_set_button_text(p);
}

/**@ フォント情報を文字列フォーマットで取得 */

void mFontButtonGetInfo_text(mFontButton *p,mStr *str)
{
	mFontInfoToText(str, &p->fbt.info);
}

/**@ フォント情報をセット */

void mFontButtonSetInfo(mFontButton *p,const mFontInfo *src)
{
	mFontInfoCopy(&p->fbt.info, src);

	_set_button_text(p);
}

/**@ フォント情報を取得 */

void mFontButtonGetInfo(mFontButton *p,mFontInfo *dst)
{
	mFontInfoCopy(dst, &p->fbt.info);
}


