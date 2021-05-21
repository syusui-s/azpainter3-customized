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
 * ファイルダイアログ
 *****************************************/

#include "mlk_gui.h"
#include "mlk_widget_def.h"
#include "mlk_widget.h"
#include "mlk_window.h"
#include "mlk_event.h"
#include "mlk_pixbuf.h"
#include "mlk_filedialog.h"
#include "mlk_filelistview.h"
#include "mlk_checkbutton.h"
#include "mlk_label.h"
#include "mlk_str.h"

#include "def_config.h"
#include "def_draw_sub.h"

#include "image32.h"
#include "trid.h"


//------------------------

#define _OPENIMGDLG(p)  ((_dlg_openimg *)(p))

/* 画像開く用 */

typedef struct
{
	MLK_FILEDIALOG_DEF

	mWidget *imgprev;
	mCheckButton *ck_8bit,
		*ck_ignore_alpha;
}_dlg_openimg;

enum
{
	OPENIMG_TYPE_NORMAL,//通常画像用
	OPENIMG_TYPE_CANVAS	//キャンバスイメージ用	
};

enum
{
	TRID_IGNORE_ALPHA
};

//------------------------


/*************************************
 * 画像プレビュー
 *************************************/

//mWidget::param1 = (Image32 *)

#define _IMGPREV_SIZE  100	//枠を除くプレビュー部分のサイズ
#define _IMGPREV_BKGND 0x808080


/* 描画 */

static void _imgprev_draw(mWidget *wg,mPixbuf *pixbuf)
{
	mPixbufBox(pixbuf, 0, 0, wg->w, wg->h, 0);

	Image32_putPixbuf((Image32 *)wg->param1, pixbuf, 1, 1);
}

/* 破棄ハンドラ */

static void _imgprev_destroy(mWidget *wg)
{
	Image32_free((Image32 *)wg->param1);
}

/* 画像プレビュー作成 */

static mWidget *_create_imgprev(mWidget *parent)
{
	mWidget *p;
	Image32 *img;

	p = mWidgetNew(parent, 0);

	p->destroy = _imgprev_destroy;
	p->draw = _imgprev_draw;
	p->flayout = MLF_FIX_WH;
	p->w = p->h = _IMGPREV_SIZE + 2;

	//画像

	img = Image32_new(_IMGPREV_SIZE, _IMGPREV_SIZE);

	p->param1 = (intptr_t)img;

	Image32_clear(img, _IMGPREV_BKGND);

	return p;
}

/* 画像変更
 *
 * filename: NULL でクリア */

static void _imgprev_set_image(mWidget *p,const char *filename)
{
	Image32 *imgdst,*imgsrc;

	imgdst = (Image32 *)p->param1;

	//クリア

	Image32_clear(imgdst, _IMGPREV_BKGND);

	//読み込み

	if(filename)
	{
		imgsrc = Image32_loadFile(filename,
			IMAGE32_LOAD_F_BLEND_WHITE | IMAGE32_LOAD_F_THUMBNAIL);

		if(imgsrc)
		{
			Image32_drawResize(imgdst, imgsrc);
			Image32_free(imgsrc);
		}
	}

	mWidgetRedraw(p);
}


/*************************************
 * 通常画像用
 *************************************/


/* ファイルが選択された時 */

static void _openimg_selectfile(mFileDialog *p,const char *path)
{
	if(APPCONF->fview & CONFIG_VIEW_F_FILEDLG_PREVIEW)
	{
		mStr str = MSTR_INIT;
	
		mFileListView_getSelectFileName(p->fdlg.flist, &str, TRUE);

		_imgprev_set_image(_OPENIMGDLG(p)->imgprev, str.buf);

		mStrFree(&str);
	}
}

/* イベントハンドラ (拡張分) */

static int _openimg_event(mWidget *wg,mEvent *ev)
{
	if(ev->type == MEVENT_NOTIFY
		&& ev->notify.id == 100
		&& ev->notify.notify_type == MCHECKBUTTON_N_CHANGE)
	{
		//チェックボタン

		APPCONF->fview ^= CONFIG_VIEW_F_FILEDLG_PREVIEW;

		if(!(APPCONF->fview & CONFIG_VIEW_F_FILEDLG_PREVIEW))
			_imgprev_set_image(_OPENIMGDLG(wg->toplevel)->imgprev, NULL);
	}

	return 1;
}

/* プレビュー作成 */

static void _openimg_create_preview(_dlg_openimg *p,mWidget *parent)
{
	//プレビュー

	p->imgprev = _create_imgprev(parent);

	//(check) プレビュー

	mCheckButtonCreate(parent, 100, MLF_BOTTOM, 0, 0,
		MLK_TR2(TRGROUP_WORD, TRID_WORD_PREVIEW),
		APPCONF->fview & CONFIG_VIEW_F_FILEDLG_PREVIEW);
}

/* ダイアログ作成 */

static _dlg_openimg *_openimg_create_dialog(mWindow *parent,int openimg_type)
{
	_dlg_openimg *p;
	mWidget *ct,*ct2,*ct3;

	p = (_dlg_openimg *)mFileDialogNew(parent, sizeof(_dlg_openimg), 0, MFILEDIALOG_TYPE_OPENFILE);
	if(!p) return FALSE;

	p->fdlg.on_selectfile = _openimg_selectfile;

	//---- オプション部分

	ct = mContainerCreateHorz(MLK_WIDGET(p), 0, 0, MLK_MAKE32_4(0,10,0,0));

	ct->notify_to = MWIDGET_NOTIFYTO_SELF;
	ct->event = _openimg_event;

	switch(openimg_type)
	{
		//通常画像
		case OPENIMG_TYPE_NORMAL:
			_openimg_create_preview(p, ct);

			mWidgetSetMargin_pack4(p->imgprev, MLK_MAKE32_4(0,0,8,0));
			break;

		//キャンバス用
		case OPENIMG_TYPE_CANVAS:
			//--- プレビュー

			ct2 = mContainerCreateVert(ct, 5, 0, MLK_MAKE32_4(0,0,15,0));

			_openimg_create_preview(p, ct2);

			//--- オプション

			ct2 = mContainerCreateVert(ct, 8, 0, 0);

			//カラー

			ct3 = mContainerCreateHorz(ct2, 5, 0, 0);

			mLabelCreate(ct3, MLF_MIDDLE, MLK_MAKE32_4(0,0,3,0), 0, MLK_TR2(TRGROUP_WORD, TRID_WORD_COLOR_BITS));

			p->ck_8bit = mCheckButtonCreate(ct3, 0, 0, 0, MCHECKBUTTON_S_RADIO, "8bit", (APPCONF->loadimg_default_bits == 8));

			mCheckButtonCreate(ct3, 0, 0, 0, MCHECKBUTTON_S_RADIO, "16bit", (APPCONF->loadimg_default_bits != 8));

			//アルファチャンネル無視

			p->ck_ignore_alpha = mCheckButtonCreate(ct2, 0, 0, 0, 0, MLK_TR2(TRGROUP_DLG_FILEDIALOG, TRID_IGNORE_ALPHA), 0);
			break;
	}

	return p;
}


//=======================


/** ファイルを開く (通常画像) */

mlkbool FileDialog_openImage(mWindow *parent,const char *filter,int def_filter,
	const char *initdir,mStr *strdst)
{
	_dlg_openimg *p;

	p = _openimg_create_dialog(parent, OPENIMG_TYPE_NORMAL);
	if(!p) return FALSE;

	mFileDialogInit(MLK_FILEDIALOG(p), filter, def_filter, initdir, strdst);
	mFileDialogShow(MLK_FILEDIALOG(p));

	return mDialogRun(MLK_DIALOG(p), TRUE);
}

/** 画像開くダイアログ (ディレクトリは strFileDlgDir) */

mlkbool FileDialog_openImage_confdir(mWindow *parent,const char *filter,int def_filter,mStr *strdst)
{
	mlkbool ret;

	ret = FileDialog_openImage(parent, filter, def_filter,
		APPCONF->strFileDlgDir.buf, strdst);

	//ディレクトリ保存
	
	if(ret)
		mStrPathGetDir(&APPCONF->strFileDlgDir, strdst->buf);

	return ret;
}

/** 画像開くダイアログ (キャンバス用) */

mlkbool FileDialog_openImage_forCanvas(mWindow *parent,const char *initdir,
	mStr *strdst,LoadImageOption *opt)
{
	_dlg_openimg *p;
	mlkbool ret;

	p = _openimg_create_dialog(parent, OPENIMG_TYPE_CANVAS);
	if(!p) return FALSE;

	mFileDialogInit(MLK_FILEDIALOG(p),
		"Image Files (APD/ADW/PSD/BMP/PNG/JPEG/GIF/TIFF/WebP)\t*.apd;*.adw;*.psd;*.bmp;*.png;*.jpg;*.jpeg;*.gif;*.tiff;*.tif;*.webp\t"
		"AzPainter File (*.apd)\t*.apd\t"
		"All Files\t*",
		0, initdir, strdst);

	mFileDialogShow(MLK_FILEDIALOG(p));

	ret = mDialogRun(MLK_DIALOG(p), FALSE);

	if(ret)
	{
		opt->bits = (mCheckButtonIsChecked(p->ck_8bit))? 8: 16;
		opt->ignore_alpha = mCheckButtonIsChecked(p->ck_ignore_alpha);
	}

	mWidgetDestroy(MLK_WIDGET(p));

	return ret;
}

