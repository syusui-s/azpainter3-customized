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

/************************************
 * 保存設定ダイアログ
 ************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_label.h"
#include "mlk_button.h"
#include "mlk_checkbutton.h"
#include "mlk_lineedit.h"
#include "mlk_combobox.h"
#include "mlk_groupbox.h"
#include "mlk_event.h"

#include "def_config.h"
#include "def_saveopt.h"
#include "fileformat.h"

#include "widget_func.h"

#include "trid.h"


//----------------------

typedef struct __dialog _dialog;

struct __dialog
{
	MLK_DIALOG_DEF

	mPoint pt_tpcol; //デフォルト位置は(0,0)

	mLineEdit *edit[2];
	mComboBox *combo[1];
	mCheckButton *check[3];

	void (*ok_handle)(_dialog *,ConfigSaveOption *);
};

//----------------------

enum
{
	TRID_TITLE,
	TRID_COMPRESS_LEVEL,
	TRID_ALPHA_CHANNEL,
	TRID_QUALITY,
	TRID_SAMPLING_FACTOR,
	TRID_SAMPLING_LIST,
	TRID_16BIT_COLOR,
	TRID_PROGRESSIVE,
	TRID_UNCOMPRESSED,
	TRID_COMPRESS_TYPE,
	TRID_TRANSPARENT_COL,
	TRID_COLOR_POINT,
	TRID_REVERSIBLE,
	TRID_IRREVERSIBLE,

	TRID_ALPHA_HELP = 50,

	TRID_PSD_TYPE_TOP = 100,
};

enum
{
	WID_BTT_COLPOINT = 100
};

//----------------------

mlkbool CanvasImagePointDlg_run(mWindow *parent,mPoint *ptdst);

//----------------------


//============================
// 共通
//============================


/* 16bit カラーチェック作成 */

static mCheckButton *_create_check_16bit(mWidget *parent,int fon,int bits)
{
	mCheckButton *p;

	p = mCheckButtonCreate(parent, 0, 0, MLK_MAKE32_4(0,8,0,0), 0,
		MLK_TR(TRID_16BIT_COLOR), fon);

	if(bits == 8)
		mWidgetEnable(MLK_WIDGET(p), 0);

	return p;
}

/* アルファチャンネル チェック作成 */

static mCheckButton *_create_check_alpha(mWidget *parent,int fon)
{
	mCheckButton *p;

	p = mCheckButtonCreate(parent, 0, 0, MLK_MAKE32_4(0,5,0,0), 0,
		MLK_TR(TRID_ALPHA_CHANNEL), fon);

	return p;
}

/* 透過色ウィジェット作成 */

static mCheckButton *_create_tpcol(mWidget *parent)
{
	mWidget *ct;
	mCheckButton *ck;

	ct = mContainerCreateHorz(parent, 8, 0, MLK_MAKE32_4(0,8,0,0));

	ck = mCheckButtonCreate(ct, 0, MLF_MIDDLE, 0, 0, MLK_TR(TRID_TRANSPARENT_COL), FALSE);

	mButtonCreate(ct, WID_BTT_COLPOINT, 0, 0, 0, MLK_TR(TRID_COLOR_POINT));

	return ck;
}

//----------------

/* OK 時、透過色のセット */

static void _set_transparent(_dialog *p,int ckno)
{
	if(mCheckButtonIsChecked(p->check[ckno]))
		APPCONF->save.pt_tpcol = p->pt_tpcol;
	else
		APPCONF->save.pt_tpcol.x = -1;
}


//============================
// PNG
//============================


/* ウィジェット作成 */

static void _create_png(_dialog *p,int bits)
{
	mWidget *ct;
	int opt;

	opt = APPCONF->save.png;

	//圧縮レベル

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, 0, 0);

	p->edit[0] = widget_createLabelEditNum(ct, MLK_TR(TRID_COMPRESS_LEVEL), 4,
		0, 9, SAVEOPT_PNG_GET_LEVEL(opt));

	//16bit カラー

	p->check[0] = _create_check_16bit(MLK_WIDGET(p), opt & SAVEOPT_PNG_F_16BIT, bits);
	
	//アルファチャンネル

	p->check[1] = _create_check_alpha(MLK_WIDGET(p), opt & SAVEOPT_PNG_F_ALPHA);

	//透過色

	p->check[2] = _create_tpcol(MLK_WIDGET(p));

	//help

	mLabelCreate(MLK_WIDGET(p), 0, MLK_MAKE32_4(0,12,0,0), MLABEL_S_BORDER, MLK_TR(TRID_ALPHA_HELP));
}

/* OK */

static void _ok_png(_dialog *p,ConfigSaveOption *dst)
{
	int opt;

	opt = mLineEditGetNum(p->edit[0]);

	if(mCheckButtonIsChecked(p->check[0])) opt |= SAVEOPT_PNG_F_16BIT;
	if(mCheckButtonIsChecked(p->check[1])) opt |= SAVEOPT_PNG_F_ALPHA;

	dst->png = opt;

	_set_transparent(p, 2);
}


//============================
// JPEG
//============================


/* ウィジェット作成 */

static void _create_jpeg(_dialog *p)
{
	mWidget *ct;
	mComboBox *cb;
	int opt;

	opt = APPCONF->save.jpeg;

	//------

	ct = mContainerCreateGrid(MLK_WIDGET(p), 2, 6, 7, 0, 0);

	//品質

	p->edit[0] = widget_createLabelEditNum(ct, MLK_TR(TRID_QUALITY), 5,
		0, 100, SAVEOPT_JPEG_GET_QUALITY(opt));

	//サンプリング比

	p->combo[0] = cb = widget_createLabelCombo(ct, MLK_TR(TRID_SAMPLING_FACTOR), 0);

	mComboBoxAddItems_sepnull(cb, MLK_TR(TRID_SAMPLING_LIST), 0);

	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, SAVEOPT_JPEG_GET_SAMPLING(opt));

	//---- プログレッシブ

	p->check[0] = mCheckButtonCreate(MLK_WIDGET(p), 0, 0, MLK_MAKE32_4(0,8,0,0), 0,
		MLK_TR(TRID_PROGRESSIVE),
		opt & SAVEOPT_JPEG_F_PROGRESSIVE);
}

/* OK */

static void _ok_jpeg(_dialog *p,ConfigSaveOption *dst)
{
	int opt;

	opt = mLineEditGetNum(p->edit[0]);
	
	opt |= mComboBoxGetItemParam(p->combo[0], -1) << SAVEOPT_JPEG_BIT_SAMPLING;

	if(mCheckButtonIsChecked(p->check[0]))
		opt |= SAVEOPT_JPEG_F_PROGRESSIVE;

	APPCONF->save.jpeg = opt;
}


//============================
// GIF
//============================


/* ウィジェット作成 */

static void _create_gif(_dialog *p)
{
	//透過色

	p->check[0] = _create_tpcol(MLK_WIDGET(p));
}

/* OK */

static void _ok_gif(_dialog *p,ConfigSaveOption *dst)
{
	_set_transparent(p, 0);
}


//============================
// TIFF
//============================


/* ウィジェット作成 */

static void _create_tiff(_dialog *p,int bits)
{
	mWidget *ct;
	mComboBox *cb;
	int opt;

	opt = APPCONF->save.tiff;

	//圧縮タイプ

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, 0, 0);

	p->combo[0] = cb = widget_createLabelCombo(ct, MLK_TR(TRID_COMPRESS_TYPE), 0);

	mComboBoxAddItems_sepnull(cb, "none\0LZW\0PackBits\0Deflate\0", 0);

	mComboBoxSetAutoWidth(cb);
	mComboBoxSetSelItem_atIndex(cb, SAVEOPT_TIFF_GET_COMPTYPE(opt));

	//16bit カラー

	p->check[0] = _create_check_16bit(MLK_WIDGET(p),
		opt & SAVEOPT_TIFF_F_16BIT, bits);
}

/* OK */

static void _ok_tiff(_dialog *p,ConfigSaveOption *dst)
{
	int opt;

	opt = mComboBoxGetItemParam(p->combo[0], -1);

	if(mCheckButtonIsChecked(p->check[0]))
		opt |= SAVEOPT_TIFF_F_16BIT;

	APPCONF->save.tiff = opt;
}


//============================
// WEBP
//============================


/* ウィジェット作成 */

static void _create_webp(_dialog *p)
{
	mWidget *ct;
	int opt,type;

	opt = APPCONF->save.webp;

	type = opt & SAVEOPT_WEBP_F_IRREVERSIBLE;

	//可逆圧縮

	p->check[0] = mCheckButtonCreate(MLK_WIDGET(p), 0, 0, 0, MCHECKBUTTON_S_RADIO,
		MLK_TR(TRID_REVERSIBLE), (type == 0));

	//圧縮レベル

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, 0, MLK_MAKE32_4(12,6,0,0));

	p->edit[0] = widget_createLabelEditNum(ct, MLK_TR(TRID_COMPRESS_LEVEL), 4,
		0, 9, SAVEOPT_WEBP_GET_LEVEL(opt));

	//不可逆圧縮

	p->check[1] = mCheckButtonCreate(MLK_WIDGET(p), 0, 0, MLK_MAKE32_4(0,8,0,0), MCHECKBUTTON_S_RADIO,
		MLK_TR(TRID_IRREVERSIBLE), (type == 1));

	//品質

	ct = mContainerCreateHorz(MLK_WIDGET(p), 6, 0, MLK_MAKE32_4(12,6,0,0));

	p->edit[1] = widget_createLabelEditNum(ct, MLK_TR(TRID_QUALITY), 5,
		0, 100, SAVEOPT_WEBP_GET_QUALITY(opt));
}

/* OK */

static void _ok_webp(_dialog *p,ConfigSaveOption *dst)
{
	int opt = 0;

	if(mCheckButtonIsChecked(p->check[1]))
		opt |= SAVEOPT_WEBP_F_IRREVERSIBLE;

	opt |= mLineEditGetNum(p->edit[0]) << SAVEOPT_WEBP_BIT_LEVEL;

	opt |= mLineEditGetNum(p->edit[1]) << SAVEOPT_WEBP_BIT_QUALITY;

	APPCONF->save.webp = opt;
}


//============================
// PSD
//============================


/* ウィジェット作成 */

static void _create_psd(_dialog *p,int bits)
{
	mWidget *ct;
	mCheckButton *ck;
	int i,type,opt;

	opt = APPCONF->save.psd;

	type = SAVEOPT_PSD_GET_TYPE(opt);

	//タイプ

	ct = (mWidget *)mGroupBoxCreate(MLK_WIDGET(p), 0, 0, 0, NULL);

	mContainerSetType_vert(MLK_CONTAINER(ct), 3);

	for(i = 0; i < 4; i++)
	{
		ck = mCheckButtonCreate(ct, 0, 0, 0, MCHECKBUTTON_S_RADIO,
			MLK_TR(TRID_PSD_TYPE_TOP + i), (type == i));

		if(i == 0) p->check[0] = ck;
	}

	//16bit

	p->check[1] = _create_check_16bit(MLK_WIDGET(p), opt & SAVEOPT_PSD_F_16BIT, bits);

	//無圧縮

	p->check[2] = mCheckButtonCreate(MLK_WIDGET(p), 0, 0, MLK_MAKE32_4(0,5,0,0), 0,
		MLK_TR(TRID_UNCOMPRESSED), opt & SAVEOPT_PSD_F_UNCOMPRESS);
}

/* OK */

static void _ok_psd(_dialog *p,ConfigSaveOption *dst)
{
	int opt;

	opt = mCheckButtonGetGroupSel(p->check[0]);

	if(mCheckButtonIsChecked(p->check[1])) opt |= SAVEOPT_PSD_F_16BIT;
	if(mCheckButtonIsChecked(p->check[2])) opt |= SAVEOPT_PSD_F_UNCOMPRESS;

	dst->psd = opt;
}


//============================
// main
//============================


/* イベント */

static int _event_handle(mWidget *wg,mEvent *ev)
{
	_dialog *p = (_dialog *)wg;

	switch(ev->type)
	{
		case MEVENT_NOTIFY:
			switch(ev->notify.id)
			{
				//色の位置
				case WID_BTT_COLPOINT:
					CanvasImagePointDlg_run(MLK_WINDOW(p), &p->pt_tpcol); 
					break;
			}
			break;
	}

	return mDialogEventDefault_okcancel(wg, ev);
}

/* ダイアログ作成 */

static _dialog *_create_dialog(mWindow *parent,uint32_t format,int bits)
{
	_dialog *p;

	MLK_TRGROUP(TRGROUP_DLG_SAVEOPT);

	p = (_dialog *)widget_createDialog(parent, sizeof(_dialog),
		MLK_TR(TRID_TITLE), _event_handle);
		
	if(!p) return NULL;

	//ウィジェット

	if(format & FILEFORMAT_PNG)
	{
		_create_png(p, bits);
		p->ok_handle = _ok_png;
	}
	else if(format & FILEFORMAT_JPEG)
	{
		_create_jpeg(p);
		p->ok_handle = _ok_jpeg;
	}
	else if(format & FILEFORMAT_GIF)
	{
		_create_gif(p);
		p->ok_handle = _ok_gif;
	}
	else if(format & FILEFORMAT_TIFF)
	{
		_create_tiff(p, bits);
		p->ok_handle = _ok_tiff;
	}
	else if(format & FILEFORMAT_WEBP)
	{
		_create_webp(p);
		p->ok_handle = _ok_webp;
	}
	else if(format & FILEFORMAT_PSD)
	{
		_create_psd(p, bits);
		p->ok_handle = _ok_psd;
	}

	//OK/cancel

	mContainerCreateButtons_okcancel(MLK_WIDGET(p), MLK_MAKE32_4(0,15,0,0));

	return p;
}

/** 保存設定ダイアログ */

mlkbool SaveOptionDlg_run(mWindow *parent,uint32_t format,int bits)
{
	_dialog *p;
	int ret;

	p = _create_dialog(parent, format, bits);
	if(!p) return FALSE;

	mWindowResizeShow_initSize(MLK_WINDOW(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
		(p->ok_handle)(p, &APPCONF->save);

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

