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
 * 共通のウィジェット関数
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_lineedit.h"
#include "mlk_combobox.h"
#include "mlk_checkbutton.h"
#include "mlk_colorbutton.h"
#include "mlk_iconbar.h"
#include "mlk_imgbutton.h"
#include "mlk_sysdlg.h"

#include "appresource.h"

#include "trid.h"


//----------------

//"?" ボタン
static const uint8_t g_img1bit_help[]={
0xf8,0x01,0xfc,0x03,0x0c,0x03,0x0c,0x03,0x0c,0x03,0x00,0x03,0xc0,0x01,0xe0,0x00,
0x60,0x00,0x60,0x00,0x00,0x00,0x60,0x00,0x60,0x00 };

//編集ボタン(15x15)
static const unsigned char g_img1bit_editbtt[]={
0x00,0x38,0xff,0x7c,0xff,0x7e,0x03,0x7f,0x83,0x3f,0xc3,0x1f,0xe3,0x0f,0xf3,0x37,
0xd3,0x33,0x93,0x31,0x7b,0x30,0x1b,0x30,0x03,0x30,0xff,0x3f,0xff,0x3f };

//保存ボタン(15x15)
static const unsigned char g_img1bit_savebtt[]={
0xff,0x1f,0x0f,0x38,0x0f,0x7a,0x0f,0x7a,0x0f,0x78,0xff,0x7f,0xff,0x7f,0xff,0x7f,
0xff,0x7f,0x07,0x70,0xf7,0x77,0x07,0x70,0xf7,0x77,0x07,0x70,0xff,0x7f };

//コピーボタン (15x15)
static const unsigned char g_img1bit_copy[]={
0xff,0x03,0xff,0x03,0x03,0x03,0x03,0x03,0x03,0x7f,0x03,0x7f,0x03,0x63,0x03,0x63,
0x03,0x63,0xff,0x63,0xff,0x63,0x60,0x60,0x60,0x60,0xe0,0x7f,0xe0,0x7f };

//貼り付けボタン(15x15)
static const unsigned char g_img1bit_paste[]={
0xf8,0x01,0x0f,0x0f,0x0f,0x0f,0xff,0x0f,0xff,0x0f,0xff,0x7f,0xff,0x7f,0xff,0x60,
0xff,0x60,0xff,0x60,0xff,0x60,0xff,0x60,0xc0,0x60,0xc0,0x7f,0xc0,0x7f };

//----------------


/** ダイアログ作成 */

mDialog *widget_createDialog(mWindow *parent,int size,const char *title,mFuncWidgetHandle_event event)
{
	mDialog *p;

	p = mDialogNew(parent, size, MTOPLEVEL_S_DIALOG_NORMAL);
	if(!p) return NULL;
	
	p->wg.event = event;

	mContainerSetPadding_same(MLK_CONTAINER(p), 8);

	mToplevelSetTitle(MLK_TOPLEVEL(p), title);

	return p;
}

/** ラベル作成 */

void widget_createLabel(mWidget *parent,const char *text)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, text);
}

/** ラベル作成 (TRID) */

void widget_createLabel_trid(mWidget *parent,int trid)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, MLK_TR(trid));
}

/** ラベル作成 "幅" */

void widget_createLabel_width(mWidget *parent)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_WIDTH));
}

/** ラベル作成 "高さ" */

void widget_createLabel_height(mWidget *parent)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_HEIGHT));
}

/** mLineEdit 数値作成 */

mLineEdit *widget_createEdit_num(mWidget *parent,int wlen,int min,int max,int dig,int val)
{
	mLineEdit *le;

	le = mLineEditCreate(parent, 0, MLF_MIDDLE, 0, MLINEEDIT_S_SPIN);

	mLineEditSetWidth_textlen(le, wlen);
	mLineEditSetNumStatus(le, min, max, dig);
	mLineEditSetNum(le, val);

	return le;
}

/** mLineEdit 数値作成 (ID 指定, MLINEEDIT_S_NOTIFY_CHANGE) */

mLineEdit *widget_createEdit_num_change(mWidget *parent,int id,int wlen,int min,int max,int dig,int val)
{
	mLineEdit *le;

	le = mLineEditCreate(parent, id, 0, 0, MLINEEDIT_S_SPIN | MLINEEDIT_S_NOTIFY_CHANGE);

	mLineEditSetWidth_textlen(le, wlen);
	mLineEditSetNumStatus(le, min, max, dig);
	mLineEditSetNum(le, val);

	return le;
}

//-----------------

/** mImgButton (1bit) の作成
 *
 * minsize: 0 より大きい場合、最小サイズ */

mWidget *widget_createImgButton_1bitText(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4,
	const uint8_t *imgbuf,int size,int minsize)
{
	mImgButton *p;

	p = mImgButtonCreate(parent, id, flayout, margin_pack4, 0);

	p->wg.hintMinW = p->wg.hintMinH = minsize;

	mImgButton_setBitImage(p, MIMGBUTTON_TYPE_1BIT_TP_TEXT, imgbuf, size, size);

	return (mWidget *)p;
}

/** ヘルプボタンの作成 */

void widget_createHelpButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4)
{
	widget_createImgButton_1bitText(parent, id, flayout, margin_pack4, g_img1bit_help, 13, 0);
}

/** メニューボタンの作成 */

mWidget *widget_createMenuButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4)
{
	return widget_createImgButton_1bitText(parent, id, flayout, margin_pack4,
		AppResource_get1bitImg_menubtt(), APPRES_MENUBTT_SIZE, APPRES_MENUBTT_SIZE + 14);
}

/** 編集ボタンの作成 */

void widget_createEditButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4)
{
	widget_createImgButton_1bitText(parent, id, flayout, margin_pack4, g_img1bit_editbtt, 15, 0);
}

/** 保存ボタンの作成 */

mWidget *widget_createSaveButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4)
{
	return widget_createImgButton_1bitText(parent, id, flayout, margin_pack4, g_img1bit_savebtt, 15, 0);
}

/** コピーボタンの作成 */

mWidget *widget_createCopyButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4,int minsize)
{
	return widget_createImgButton_1bitText(parent, id, flayout, margin_pack4, g_img1bit_copy, 15, minsize);
}

/** 貼り付けボタンの作成 */

mWidget *widget_createPasteButton(mWidget *parent,int id,uint32_t flayout,uint32_t margin_pack4,int minsize)
{
	return widget_createImgButton_1bitText(parent, id, flayout, margin_pack4, g_img1bit_paste, 15, minsize);
}

//-----------------

/** mLabel + mLineEdit 作成 (数値,桁なし) */

mLineEdit *widget_createLabelEditNum(mWidget *parent,const char *label,int wlen,int min,int max,int val)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, label);

	return widget_createEdit_num(parent, wlen, min, max, 0, val);
}

/** mLabel + mComboBox 作成 */

mComboBox *widget_createLabelCombo(mWidget *parent,const char *label,int id)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, label);

	return mComboBoxCreate(parent, id, 0, 0, 0);
}

/** mLabel + mColorButton 作成 (ダイアログあり) */

mColorButton *widget_createLabelColorButton(mWidget *parent,const char *label,int id,uint32_t col)
{
	mLabelCreate(parent, MLF_MIDDLE, 0, 0, label);

	return mColorButtonCreate(parent, id, MLF_MIDDLE, 0, MCOLORBUTTON_S_DIALOG, col);
}

/** カラービット数のラジオボタン作成 (mLabel + [mCheckButton x 2])
 *
 * is_8bit: 初期選択が 8bit か
 * return: 8bit のチェックボタン */

mCheckButton *widget_createColorBits(mWidget *parent,int is_8bit)
{
	mWidget *ct;
	mCheckButton *ckbtt;

	widget_createLabel(parent, MLK_TR2(TRGROUP_WORD, TRID_WORD_COLOR_BITS));

	//

	ct = mContainerCreateHorz(parent, 4, 0, 0);

	ckbtt = mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO, "8bit", is_8bit);
	
	mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO, "16bit", !is_8bit);

	return ckbtt;
}

/** リスト編集用の mIconBar と OK/キャンセル ボタンを作成
 *
 * pbttno: ボタンの番号 (APPRES_LISTEDIT_*)。255 で終了
 * cancel: キャンセルボタンを作成するか */

mIconBar *widget_createListEdit_iconbar_btt(mWidget *parent,const uint8_t *pbttno,mlkbool cancel)
{
	mWidget *ct,*wg;
	mIconBar *ib;
	int n;

	ct = mContainerCreateHorz(parent, 5, MLF_EXPAND_W, MLK_MAKE32_4(0,3,0,0));

	//mIconBar

	ib = mIconBarCreate(ct, 0, MLF_EXPAND_X, 0,
		MICONBAR_S_TOOLTIP | MICONBAR_S_DESTROY_IMAGELIST);

	mIconBarSetImageList(ib, AppResource_createImageList_listedit());
	mIconBarSetTooltipTrGroup(ib, TRGROUP_LISTEDIT);

	for(; *pbttno != 255; pbttno++)
	{
		n = *pbttno;
		mIconBarAdd(ib, n, n, n, 0);
	}

	//OK

	wg = (mWidget *)mButtonCreate(ct, MLK_WID_OK, MLF_BOTTOM, 0, 0, MLK_TR_SYS(MLK_TRSYS_OK));

	wg->fstate |= MWIDGET_STATE_ENTER_SEND;

	//キャンセル

	if(cancel)
		mButtonCreate(ct, MLK_WID_CANCEL, MLF_BOTTOM, 0, 0, MLK_TR_SYS(MLK_TRSYS_CANCEL));

	return ib;
}

/** 確認メッセージを表示 (はい/キャンセル)
 *
 * return: "はい" の場合 TRUE */

mlkbool widget_message_confirm(mWindow *parent,const char *mes)
{
	return (mMessageBox(parent,
		MLK_TR2(TRGROUP_MESSAGE, TRID_MESSAGE_TITLE_CONFIRM), mes,
		MMESBOX_YES | MMESBOX_CANCEL, MMESBOX_YES) == MMESBOX_YES);
}


